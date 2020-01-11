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

// Embree includes must be first
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>
#define _SSIZE_T_DEFINED

#include "../Graphics/Model.h"
#include "../Graphics/ModelView.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#include "../Glow/EmbreeScene.h"
#include "../Glow/Helpers.h"
#include "../IO/Log.h"

#include <future>

namespace Urho3D
{

namespace
{

/// Paramters for raytracing geometry creation.
struct RaytracingGeometryCreateParams
{
    /// Transform from geometry to world space.
    Matrix3x4 worldTransform_;
    /// Rotation from geometry to world space.
    Quaternion worldRotation_;
    /// Unpacked geometry data.
    const GeometryLODView* geometry_{};
    /// Lightmap UV scale.
    Vector2 lightmapUVScale_;
    /// Lightmap UV offset.
    Vector2 lightmapUVOffset_;
    /// UV channel used for lightmap UV.
    unsigned lightmapUVChannel_;
    /// Whether to store main texture UV.
    bool storeUV_{};
    /// Transform for U coordinate.
    Vector4 uOffset_;
    /// Transform for V coordinate.
    Vector4 vOffset_;
};

/// Parsed model key and value.
struct ParsedModelKeyValue
{
    Model* model_{};
    SharedPtr<ModelView> parsedModel_;
};

/// Parse model data.
ParsedModelKeyValue ParseModelForEmbree(Model* model)
{
    auto modelView = MakeShared<ModelView>(model->GetContext());
    modelView->ImportModel(model);

    return { model, modelView };
}

/// Create Embree geometry from geometry view.
RTCGeometry CreateEmbreeGeometry(RTCDevice embreeDevice, const RaytracingGeometryCreateParams& params, unsigned mask)
{
    const unsigned numVertices = params.geometry_->vertices_.size();
    const unsigned numIndices = params.geometry_->indices_.size();
    const ea::vector<ModelVertex>& sourceVertices = params.geometry_->vertices_;
    const unsigned numAttributes = params.storeUV_ ? 3 : 2;

    RTCGeometry embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    rtcSetGeometryVertexAttributeCount(embreeGeometry, numAttributes);

    float* vertices = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX,
        0, RTC_FORMAT_FLOAT3, sizeof(Vector3), numVertices));

    float* lightmapUVs = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        EmbreeScene::LightmapUVAttribute, RTC_FORMAT_FLOAT2, sizeof(Vector2), numVertices));

    float* smoothNormals = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        EmbreeScene::NormalAttribute, RTC_FORMAT_FLOAT3, sizeof(Vector3), numVertices));

    float* uvs = nullptr;
    if (params.storeUV_)
    {
        uvs = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            EmbreeScene::UVAttribute, RTC_FORMAT_FLOAT2, sizeof(Vector2), numVertices));
    }

    for (unsigned i = 0; i < numVertices; ++i)
    {
        const Vector3 localPosition = static_cast<Vector3>(sourceVertices[i].position_);
        const Vector3 localNormal = static_cast<Vector3>(sourceVertices[i].normal_);
        const Vector2 lightmapUV = static_cast<Vector2>(sourceVertices[i].uv_[params.lightmapUVChannel_]);
        const Vector2 lightmapUVScaled = lightmapUV * params.lightmapUVScale_ + params.lightmapUVOffset_;
        const Vector3 worldPosition = params.worldTransform_ * localPosition;
        const Vector3 worldNormal = params.worldRotation_ * localNormal;

        vertices[i * 3 + 0] = worldPosition.x_;
        vertices[i * 3 + 1] = worldPosition.y_;
        vertices[i * 3 + 2] = worldPosition.z_;
        lightmapUVs[i * 2 + 0] = lightmapUVScaled.x_;
        lightmapUVs[i * 2 + 1] = lightmapUVScaled.y_;
        smoothNormals[i * 3 + 0] = worldNormal.x_;
        smoothNormals[i * 3 + 1] = worldNormal.y_;
        smoothNormals[i * 3 + 2] = worldNormal.z_;

        if (uvs)
        {
            const Vector2 uv = static_cast<Vector2>(sourceVertices[i].uv_[0]);
            uvs[i * 2 + 0] = uv.DotProduct(static_cast<Vector2>(params.uOffset_)) + params.uOffset_.w_;
            uvs[i * 2 + 1] = uv.DotProduct(static_cast<Vector2>(params.vOffset_)) + params.vOffset_.w_;
        }
    }

    unsigned* indices = reinterpret_cast<unsigned*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX,
        0, RTC_FORMAT_UINT3, sizeof(unsigned) * 3, numIndices / 3));

    for (unsigned i = 0; i < numIndices; ++i)
        indices[i] = params.geometry_->indices_[i];

    rtcSetGeometryMask(embreeGeometry, mask);
    rtcCommitGeometry(embreeGeometry);
    return embreeGeometry;
}

/// Create Embree geometry from parsed model.
ea::vector<EmbreeGeometry> CreateEmbreeGeometriesForModel(
    RTCDevice embreeDevice, ModelView* modelView, StaticModel* staticModel, unsigned objectIndex, unsigned lightmapUVChannel)
{
    Node* node = staticModel->GetNode();
    const unsigned lightmapIndex = staticModel->GetLightmapIndex();
    const Vector4 lightmapUVScaleOffset = staticModel->GetLightmapScaleOffset();
    const Vector2 lightmapUVScale{ lightmapUVScaleOffset.x_, lightmapUVScaleOffset.y_ };
    const Vector2 lightmapUVOffset{ lightmapUVScaleOffset.z_, lightmapUVScaleOffset.w_ };

    ea::vector<EmbreeGeometry> result;

    const ea::vector<GeometryView> geometries = modelView->GetGeometries();
    for (unsigned geometryIndex = 0; geometryIndex < geometries.size(); ++geometryIndex)
    {
        Material* material = staticModel->GetMaterial(geometryIndex);

        const GeometryView& geometryView = geometries[geometryIndex];
        for (unsigned lodIndex = 0; lodIndex < geometryView.lods_.size(); ++lodIndex)
        {
            const GeometryLODView& geometryLODView = geometryView.lods_[lodIndex];
            const unsigned mask = lodIndex == 0 ? EmbreeScene::PrimaryLODGeometry : EmbreeScene::SecondaryLODGeometry;

            EmbreeGeometry embreeGeometry;
            embreeGeometry.objectIndex_ = objectIndex;
            embreeGeometry.geometryIndex_ = geometryIndex;
            embreeGeometry.lodIndex_ = lodIndex;
            embreeGeometry.numLods_ = geometryView.lods_.size();
            embreeGeometry.lightmapIndex_ = lightmapIndex;
            embreeGeometry.embreeGeometryId_ = M_MAX_UNSIGNED;

            Vector4 uOffset;
            Vector4 vOffset;
            embreeGeometry.opaque_ = !material || IsMaterialOpaque(material);
            if (!embreeGeometry.opaque_)
            {
                const Color diffuseColor = GetMaterialDiffuseColor(material);
                embreeGeometry.diffuseColor_ = diffuseColor.ToVector3();
                embreeGeometry.alpha_ = diffuseColor.a_;

                Texture* diffuseTexture = GetMaterialDiffuseTexture(material, uOffset, vOffset);
                if (diffuseTexture)
                    embreeGeometry.diffuseImageName_ = diffuseTexture->GetName();
            }

            RaytracingGeometryCreateParams params;
            params.worldTransform_ = node->GetWorldTransform();
            params.worldRotation_ = node->GetWorldRotation();
            params.geometry_ = &geometryLODView;
            params.lightmapUVScale_ = lightmapUVScale;
            params.lightmapUVOffset_ = lightmapUVOffset;
            params.lightmapUVChannel_ = lightmapUVChannel;
            if (!embreeGeometry.diffuseImageName_.empty())
            {
                params.storeUV_ = true;
                params.uOffset_ = uOffset;
                params.vOffset_ = vOffset;
            }

            embreeGeometry.embreeGeometry_ = CreateEmbreeGeometry(embreeDevice, params, mask);
            result.push_back(embreeGeometry);
        }
    }
    return result;
}

}

EmbreeScene::~EmbreeScene()
{
    if (scene_)
        rtcReleaseScene(scene_);
    if (device_)
        rtcReleaseDevice(device_);
}

SharedPtr<EmbreeScene> CreateEmbreeScene(Context* context, const ea::vector<StaticModel*>& staticModels, unsigned lightmapUVChannel)
{
    // Queue models for parsing
    ea::hash_set<Model*> modelsToParse;
    for (StaticModel* staticModel : staticModels)
        modelsToParse.insert(staticModel->GetModel());

    // Start model parsing
    ea::vector<std::future<ParsedModelKeyValue>> modelParseTasks;
    for (Model* model : modelsToParse)
        modelParseTasks.push_back(std::async(ParseModelForEmbree, model));

    // Finish model parsing
    ea::unordered_map<Model*, SharedPtr<ModelView>> parsedModelCache;
    for (auto& task : modelParseTasks)
    {
        const ParsedModelKeyValue& parsedModel = task.get();
        parsedModelCache.emplace(parsedModel.model_, parsedModel.parsedModel_);
    }

    // Prepare Embree scene
    const RTCDevice device = rtcNewDevice("");
    const RTCScene scene = rtcNewScene(device);
    rtcSetSceneFlags(scene, RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION);

    ea::vector<std::future<ea::vector<EmbreeGeometry>>> createEmbreeGeometriesTasks;
    for (unsigned objectIndex = 0; objectIndex < staticModels.size(); ++objectIndex)
    {
        StaticModel* staticModel = staticModels[objectIndex];

        ModelView* parsedModel = parsedModelCache[staticModel->GetModel()];
        createEmbreeGeometriesTasks.push_back(std::async(CreateEmbreeGeometriesForModel,
            device, parsedModel, staticModel, objectIndex, lightmapUVChannel));
    }

    // Collect and attach Embree geometries
    ea::hash_map<ea::string, SharedPtr<Image>> diffuseImages;
    ea::vector<EmbreeGeometry> geometryIndex;
    for (auto& task : createEmbreeGeometriesTasks)
    {
        const ea::vector<EmbreeGeometry> embreeGeometriesArray = task.get();
        for (const EmbreeGeometry& embreeGeometry : embreeGeometriesArray)
        {
            const unsigned geomID = rtcAttachGeometry(scene, embreeGeometry.embreeGeometry_);
            rtcReleaseGeometry(embreeGeometry.embreeGeometry_);

            geometryIndex.resize(geomID + 1);
            geometryIndex[geomID] = embreeGeometry;
            geometryIndex[geomID].embreeGeometryId_ = geomID;
            diffuseImages[embreeGeometry.diffuseImageName_] = nullptr;
        }
    }

    // Finalize scene
    rtcCommitScene(scene);

    // Load images
    auto cache = context->GetCache();
    for (auto& nameAndImage : diffuseImages)
    {
        if (!nameAndImage.first.empty())
        {
            auto image = cache->GetResource<Image>(nameAndImage.first);
            nameAndImage.second = image->GetDecompressedImage();
        }
    }

    for (EmbreeGeometry& embreeGeometry : geometryIndex)
    {
        embreeGeometry.diffuseImage_ = diffuseImages[embreeGeometry.diffuseImageName_];
        if (embreeGeometry.diffuseImage_)
        {
            embreeGeometry.diffuseImageWidth_ = embreeGeometry.diffuseImage_->GetWidth();
            embreeGeometry.diffuseImageHeight_ = embreeGeometry.diffuseImage_->GetHeight();
        }
    }

    // Calculate max distance between objects
    BoundingBox boundingBox;
    for (StaticModel* staticModel : staticModels)
        boundingBox.Merge(staticModel->GetWorldBoundingBox());

    const Vector3 sceneSize = boundingBox.Size();
    const float maxDistance = ea::max({ sceneSize.x_, sceneSize.y_, sceneSize.z_ });

    return MakeShared<EmbreeScene>(context, device, scene, ea::move(geometryIndex), maxDistance);
}

}
