//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Glow/LightmapGeometryBaker.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Math/BoundingBox.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Glow/StaticModelForLightmap.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/ModelView.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/View.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Node.h"

#include <EASTL/sort.h>

#include <future>

namespace Urho3D
{

namespace
{

/// Number of multi-tap samples.
const unsigned numMultiTapSamples = 25;

/// Multi-tap offsets.
const Vector2 multiTapOffsets[numMultiTapSamples] =
{
    {  1.0f,  1.0f },
    {  1.0f, -1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },

    {  1.0f,  0.5f },
    {  1.0f, -0.5f },
    { -1.0f,  0.5f },
    { -1.0f, -0.5f },
    {  0.5f,  1.0f },
    {  0.5f, -1.0f },
    { -0.5f,  1.0f },
    { -0.5f, -1.0f },

    {  1.0f,  0.0f },
    { -1.0f,  0.0f },
    {  0.0f,  1.0f },
    {  0.0f, -1.0f },

    {  0.5f,  0.5f },
    {  0.5f, -0.5f },
    { -0.5f,  0.5f },
    { -0.5f, -0.5f },

    {  0.5f,  0.0f },
    { -0.5f,  0.0f },
    {  0.0f,  0.5f },
    {  0.0f, -0.5f },

    {  0.0f,  0.0f },
};

/// Pair of two ordered indices.
using OrderedIndexPair = ea::pair<unsigned, unsigned>;

/// Return edge by two indices.
OrderedIndexPair MakeOrderedIndexPair(unsigned firstIndex, unsigned secondIndex)
{
    if (firstIndex < secondIndex)
        return { firstIndex, secondIndex };
    else
        return { secondIndex, firstIndex };
}

/// Collect seams of the model.
LightmapSeamVector CollectModelSeams(Model* model, unsigned uvChannel)
{
    ModelView modelView(model->GetContext());
    if (!modelView.ImportModel(model))
    {
        URHO3D_LOGERROR("Failed to import model \"{}\"", model->GetName());
        return {};
    }

    const bool sharedLightmapUV = modelView.GetMetadata(LightmapUVGenerationSettings::LightmapSharedUV).GetBool();

    // Calculate bounding box and step for spatial hashing
    const float positionEpsilon = M_LARGE_EPSILON;
    const float positionEpsilonSquared = positionEpsilon * positionEpsilon;
    const float normalEpsilon = M_LARGE_EPSILON;
    const float normalEpsilonSquared = normalEpsilon * normalEpsilon;
    const float uvEpsilon = M_LARGE_EPSILON;
    const float uvEpsilonSquared = uvEpsilon * uvEpsilon;

    const BoundingBox boundingBox = modelView.CalculateBoundingBox();
    const Vector3 hashStep = VectorMax(boundingBox.Size() / M_LARGE_VALUE, Vector3::ONE * positionEpsilon);
    const auto computeHash = [&](const Vector3& position)
    {
        return VectorFloorToInt((position - boundingBox.min_) / hashStep);
    };

    // TODO(glow): Check vertex format
    //const ModelVertexFormat vertexFormat = modelView.GetVertexFormat();

    ea::vector<LightmapSeam> seams;
    for (const GeometryView& geometry : modelView.GetGeometries())
    {
        if (geometry.lods_.empty())
            continue;

        for (const GeometryLODView& geometryLod : geometry.lods_)
        {
            const ea::vector<ModelVertex>& vertices = geometryLod.vertices_;

            // Read all edges
            ea::vector<OrderedIndexPair> geometryEdges;
            for (unsigned faceIndex = 0; faceIndex < geometryLod.indices_.size() / 3; ++faceIndex)
            {
                const unsigned indexA = geometryLod.indices_[faceIndex * 3 + 0];
                const unsigned indexB = geometryLod.indices_[faceIndex * 3 + 1];
                const unsigned indexC = geometryLod.indices_[faceIndex * 3 + 2];

                geometryEdges.push_back(MakeOrderedIndexPair(indexA, indexB));
                geometryEdges.push_back(MakeOrderedIndexPair(indexB, indexC));
                geometryEdges.push_back(MakeOrderedIndexPair(indexC, indexA));
            }

            // Remove duplicates
            ea::sort(geometryEdges.begin(), geometryEdges.end());
            geometryEdges.erase(ea::unique(geometryEdges.begin(), geometryEdges.end()), geometryEdges.end());

            // Make spatial hash for edges.
            ea::unordered_map<IntVector3, ea::vector<OrderedIndexPair>> geometryEdgesHash;
            for (const OrderedIndexPair& edge : geometryEdges)
            {
                // Hash both sides of the edge
                for (const unsigned index : { edge.first, edge.second })
                {
                    const ModelVertex& vertex = vertices[index];
                    const Vector3& position = static_cast<Vector3>(vertex.position_);
                    const IntVector3 hashPosition = computeHash(position);
                    geometryEdgesHash[hashPosition].push_back(edge);
                }
            }

            // Find seams
            ea::vector<OrderedIndexPair> candidatesBuffer;
            for (const OrderedIndexPair& edge : geometryEdges)
            {
                // Find candidates from spatial hash
                candidatesBuffer.clear();
                for (const unsigned index : { edge.first, edge.second })
                {
                    const ModelVertex& vertex = vertices[index];
                    const Vector3& position = static_cast<Vector3>(vertex.position_);
                    const IntVector3 hashPosition = computeHash(position);

                    IntVector3 hashOffset;
                    for (hashOffset.x_ = -1; hashOffset.x_ <= 1; ++hashOffset.x_)
                    {
                        for (hashOffset.y_ = -1; hashOffset.y_ <= 1; ++hashOffset.y_)
                        {
                            for (hashOffset.z_ = -1; hashOffset.z_ <= 1; ++hashOffset.z_)
                            {
                                const auto iter = geometryEdgesHash.find(hashPosition + hashOffset);
                                if (iter != geometryEdgesHash.end())
                                    candidatesBuffer.push_back(iter->second);
                            }
                        }
                    }
                }

                // Remove duplicates
                ea::sort(candidatesBuffer.begin(), candidatesBuffer.end());
                candidatesBuffer.erase(ea::unique(candidatesBuffer.begin(), candidatesBuffer.end()), candidatesBuffer.end());

                // Check for seams
                const Vector3 edgePos0 = static_cast<Vector3>(vertices[edge.first].position_);
                const Vector3 edgePos1 = static_cast<Vector3>(vertices[edge.second].position_);
                const Vector3 edgeNormal0 = static_cast<Vector3>(vertices[edge.first].normal_);
                const Vector3 edgeNormal1 = static_cast<Vector3>(vertices[edge.second].normal_);
                const Vector2 edgeUv0 = static_cast<Vector2>(vertices[edge.first].uv_[uvChannel]);
                const Vector2 edgeUv1 = static_cast<Vector2>(vertices[edge.second].uv_[uvChannel]);

                for (OrderedIndexPair candidate : candidatesBuffer)
                {
                    // Skip self
                    if (candidate == edge)
                        continue;

                    // Swap candidate vertices if needed
                    {
                        const Vector3 candidatePos0 = static_cast<Vector3>(vertices[candidate.first].position_);
                        if ((candidatePos0 - edgePos1).LengthSquared() < positionEpsilonSquared)
                            ea::swap(candidate.first, candidate.second);
                    }

                    const Vector3 candidatePos0 = static_cast<Vector3>(vertices[candidate.first].position_);
                    const Vector3 candidatePos1 = static_cast<Vector3>(vertices[candidate.second].position_);
                    const Vector3 candidateNormal0 = static_cast<Vector3>(vertices[candidate.first].normal_);
                    const Vector3 candidateNormal1 = static_cast<Vector3>(vertices[candidate.second].normal_);
                    const Vector2 candidateUv0 = static_cast<Vector2>(vertices[candidate.first].uv_[uvChannel]);
                    const Vector2 candidateUv1 = static_cast<Vector2>(vertices[candidate.second].uv_[uvChannel]);

                    // Skip if edge geometry is different
                    const bool samePos0 = (edgePos0 - candidatePos0).LengthSquared() < positionEpsilonSquared;
                    const bool samePos1 = (edgePos1 - candidatePos1).LengthSquared() < positionEpsilonSquared;
                    const bool sameNormal0 = (edgeNormal0 - candidateNormal0).LengthSquared() < normalEpsilonSquared;
                    const bool sameNormal1 = (edgeNormal1 - candidateNormal1).LengthSquared() < normalEpsilonSquared;
                    if (!samePos0 || !samePos1 || !sameNormal0 || !sameNormal1)
                        continue;

                    // Skip if not a seam
                    const bool sameUv0 = (edgeUv0 - candidateUv0).LengthSquared() < uvEpsilonSquared;
                    const bool sameUv1 = (edgeUv1 - candidateUv1).LengthSquared() < uvEpsilonSquared;
                    if (sameUv0 && sameUv1)
                        continue;

                    // Skip if belong to the same line: AB x AC = AB x AD = 0
                    const Vector3 edgeUvDelta{ edgeUv1 - edgeUv0, 0.0f };
                    const Vector3 delta00{ candidateUv0 - edgeUv0, 0.0f };
                    const Vector3 delta01{ candidateUv1 - edgeUv0, 0.0f };
                    const bool collinear00 = edgeUvDelta.CrossProduct(delta00).LengthSquared() < uvEpsilonSquared;
                    const bool collinear01 = edgeUvDelta.CrossProduct(delta01).LengthSquared() < uvEpsilonSquared;
                    if (collinear00 && collinear01)
                        continue;

                    // It's a seam!
                    LightmapSeam seam;
                    seam.positions_[0] = edgeUv0;
                    seam.positions_[1] = edgeUv1;
                    seam.otherPositions_[0] = candidateUv0;
                    seam.otherPositions_[1] = candidateUv1;
                    seams.push_back(seam);
                }
            }

            // Skip the rest of lods if UVs are shared
            if (sharedLightmapUV)
                break;
        }
    }
    return seams;
}

/// Read RGBA32 float texture to vector.
void ReadTextureRGBA32Float(Texture* texture, ea::vector<Vector4>& dest)
{
    auto texture2D = dynamic_cast<Texture2D*>(texture);
    const unsigned numElements = texture->GetDataSize(texture->GetWidth(), texture->GetHeight()) / sizeof(Vector4);
    dest.resize(numElements);
    texture2D->GetData(0, dest.data());
}

/// Extract Vector3 from Vector4.
Vector3 ExtractVector3FromVector4(const Vector4& data) { return { data.x_, data.y_, data.z_ }; }

/// Extract w-component as unsigned integer from Vector4.
unsigned ExtractUintFromVector4(const Vector4& data) { return static_cast<unsigned>(data.w_); }

}

SharedPtr<RenderPath> LoadRenderPath(Context* context, const ea::string& renderPathName)
{
    auto renderPath = MakeShared<RenderPath>();
    auto renderPathXml = context->GetCache()->GetResource<XMLFile>(renderPathName);
    if (!renderPath->Load(renderPathXml))
        return nullptr;
    return renderPath;
}

ea::vector<LightmapGeometryBakingScene> GenerateLightmapGeometryBakingScenes(
    Context* context, const ea::vector<StaticModel*>& staticModels,
    unsigned chartSize, const LightmapGeometryBakingSettings& settings)
{
    const Vector2 texelSize{ 1.0f / chartSize, 1.0f / chartSize };

    // Load resources
    SharedPtr<RenderPath> renderPath = LoadRenderPath(context, settings.renderPathName_);
    if (!renderPath)
    {
        URHO3D_LOGERROR("Cannot load render path \"{}\"", settings.renderPathName_);
        return {};
    }

    Material* bakingMaterial = context->GetCache()->GetResource<Material>(settings.materialName_);
    if (!bakingMaterial)
    {
        URHO3D_LOGERROR("Cannot load material \"{}\"", settings.materialName_);
        return {};
    }

    // Collect used models
    ea::hash_set<Model*> usedModels;
    for (StaticModel* staticModel : staticModels)
        usedModels.insert(staticModel->GetModel());

    // Schedule model seams collecting
    ea::vector<std::future<ea::pair<Model*, LightmapSeamVector>>> collectSeamsTasks;
    for (Model* model : usedModels)
    {
        const unsigned uvChannel = settings.uvChannel_;
        collectSeamsTasks.push_back(std::async([model, uvChannel]()
        {
            LightmapSeamVector modelSeams = CollectModelSeams(model, uvChannel);
            return ea::make_pair(model, ea::move(modelSeams));
        }));
    }

    // Cache model seams
    ea::hash_map<Model*, LightmapSeamVector> modelSeamsCache;
    for (auto& task : collectSeamsTasks)
    {
        auto result = task.get();
        modelSeamsCache.emplace(result.first, ea::move(result.second));
    }

    // Zero ID is reserved for invalid texels
    GeometryIDToObjectMappingVector mapping;
    mapping.push_back();

    ea::unordered_map<unsigned, LightmapGeometryBakingScene> bakingScenes;
    for (unsigned objectIndex = 0; objectIndex < staticModels.size(); ++objectIndex)
    {
        StaticModel* staticModel = staticModels[objectIndex];
        Node* node = staticModel->GetNode();
        const unsigned lightmapIndex = staticModel->GetLightmapIndex();
        const Vector4 scaleOffset = staticModel->GetLightmapScaleOffset();
        const Vector2 scale{ scaleOffset.x_, scaleOffset.y_ };
        const Vector2 offset{ scaleOffset.z_, scaleOffset.w_ };

        // Initialize baking scene if first hit
        LightmapGeometryBakingScene& bakingScene = bakingScenes[lightmapIndex];
        if (!bakingScene.context_)
        {
            bakingScene.context_ = context;
            bakingScene.index_ = lightmapIndex;
            bakingScene.width_ = chartSize;
            bakingScene.height_ = chartSize;
            bakingScene.size_ = IntVector2::ONE * static_cast<int>(chartSize);

            bakingScene.scene_ = MakeShared<Scene>(context);
            bakingScene.scene_->CreateComponent<Octree>();

            bakingScene.camera_ = bakingScene.scene_->CreateComponent<Camera>();

            bakingScene.renderPath_ = renderPath;
        }

        // Add seams
        const LightmapSeamVector& modelSeams = modelSeamsCache[staticModel->GetModel()];
        for (const LightmapSeam& seam : modelSeams)
            bakingScene.seams_.push_back(seam.Transformed(scale, offset));

        // Add node
        Node* bakingNode = bakingScene.scene_->CreateChild();
        bakingNode->SetPosition(node->GetWorldPosition());
        bakingNode->SetRotation(node->GetWorldRotation());
        bakingNode->SetScale(node->GetWorldScale());

        auto staticModelForLightmap = bakingNode->CreateComponent<StaticModelForLightmap>();
        const GeometryIDToObjectMappingVector objectMapping = staticModelForLightmap->Initialize(objectIndex,
            staticModel, bakingMaterial, mapping.size(), multiTapOffsets, texelSize, scaleOffset);

        mapping.push_back(objectMapping);
    }

    ea::vector<LightmapGeometryBakingScene> result;
    for (auto& elem : bakingScenes)
        result.push_back(ea::move(elem.second));
    return result;
}

LightmapChartGeometryBuffer BakeLightmapGeometryBuffer(const LightmapGeometryBakingScene& bakingScene)
{
    Context* context = bakingScene.context_;
    Graphics* graphics = context->GetGraphics();
    Renderer* renderer = context->GetRenderer();

    static thread_local ea::vector<Vector4> buffer;

    if (!graphics->BeginFrame())
    {
        URHO3D_LOGERROR("Failed to begin lightmap geometry buffer rendering \"{}\"");
        return {};
    }

    LightmapChartGeometryBuffer geometryBuffer{ bakingScene.index_, bakingScene.width_, bakingScene.height_ };

    // Get render surface
    Texture* renderTexture = renderer->GetScreenBuffer(
        bakingScene.size_.x_, bakingScene.size_.y_,Graphics::GetRGBAFormat(), 1, true, false, false, false);
    RenderSurface* renderSurface = static_cast<Texture2D*>(renderTexture)->GetRenderSurface();

    // Setup viewport
    Viewport viewport(context);
    viewport.SetCamera(bakingScene.camera_);
    viewport.SetRect(IntRect::ZERO);
    viewport.SetRenderPath(bakingScene.renderPath_);
    viewport.SetScene(bakingScene.scene_);

    // Render scene
    View view(context);
    view.Define(renderSurface, &viewport);
    view.Update(FrameInfo());
    view.Render();

    // Store results
    ReadTextureRGBA32Float(view.GetExtraRenderTarget("position"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.geometryPositions_.begin(), ExtractVector3FromVector4);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.geometryIds_.begin(), ExtractUintFromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothposition"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.smoothPositions_.begin(), ExtractVector3FromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("facenormal"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.faceNormals_.begin(), ExtractVector3FromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothnormal"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.smoothNormals_.begin(), ExtractVector3FromVector4);

    graphics->EndFrame();

    geometryBuffer.geometryMapping_ = bakingScene.geometryMapping_;
    geometryBuffer.seams_ = bakingScene.seams_;
    return geometryBuffer;
}

LightmapChartGeometryBufferVector BakeLightmapGeometryBuffers(const ea::vector<LightmapGeometryBakingScene>& bakingScenes)
{
    LightmapChartGeometryBufferVector geometryBuffers;
    for (const LightmapGeometryBakingScene& bakingScene : bakingScenes)
        geometryBuffers.push_back(BakeLightmapGeometryBuffer(bakingScene));
    return geometryBuffers;
}

}
