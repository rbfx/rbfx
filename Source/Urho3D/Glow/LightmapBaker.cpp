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

#include "../Glow/LightmapBaker.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/View.h"
#include "../Graphics/Viewport.h"
#include "../Math/AreaAllocator.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/sort.h>

// TODO: Use thread pool?
#include <future>

namespace Urho3D
{

struct KDNearestNeighbour
{
    unsigned index_{};
    float distanceSquared_{};
    bool operator < (const KDNearestNeighbour& rhs) const { return distanceSquared_ < rhs.distanceSquared_; }
};

/// K-D Tree.
template <class T>
class KDTree
{
private:
public:
    void Initialize(ea::vector<T> elements)
    {
        // Initialize points and bounding box.
        elements_ = ea::move(elements);

        BoundingBox boundingBox = {};
        for (const T& element : elements_)
            boundingBox.Merge(static_cast<Vector3>(element));

        CalculateDepthAndCapacity(elements_.size(), maxDepth_, capacity_);
        tree_.clear();
        tree_.resize(capacity_, M_MAX_UNSIGNED);

        // Build the tree
        ea::vector<ea::pair<unsigned, unsigned>> rangeQueue;
        ea::vector<ea::pair<unsigned, unsigned>> nextRangeQueue;

        rangeQueue.emplace_back(0, elements_.size());
        unsigned index = 0;
        for (unsigned depth = 0; depth < maxDepth_; ++depth)
        {
            const unsigned axis = depth % 3;
            for (const auto& range : rangeQueue)
            {
                const unsigned firstElement = range.first;
                const unsigned lastElement = range.second;
                if (firstElement != lastElement)
                {
                    const unsigned medianElement = (firstElement + lastElement) / 2;
                    tree_[index] = medianElement;

                    auto begin = elements_.begin();
                    ea::nth_element(begin + firstElement, begin + medianElement, begin + lastElement, GetComparator(axis));

                    nextRangeQueue.emplace_back(firstElement, medianElement);
                    nextRangeQueue.emplace_back(medianElement + 1, lastElement);
                }

                ++index;
            }

            ea::swap(rangeQueue, nextRangeQueue);
            nextRangeQueue.clear();
        }
    }

    template <class Container, class Predicate>
    void CollectNearestElements(Container& heap, const Vector3& point, float maxDistance, unsigned maxElements, Predicate predicate)
    {
        heap.clear();
        float maxDistanceSquared = maxDistance * maxDistance;
        CollectNearestElements(heap, 0, 0, point, maxDistanceSquared, maxElements, predicate);
    }

    const T& operator [](unsigned index) const { return elements_[index]; }

private:
    static auto GetComparator(unsigned axis)
    {
        return [=](const T& lhs, const T& rhs)
        {
            const Vector3 lhsPosition = static_cast<Vector3>(lhs);
            const Vector3 rhsPosition = static_cast<Vector3>(rhs);
            switch (axis)
            {
            case 0: return lhsPosition.x_ < rhsPosition.x_;
            case 1: return lhsPosition.y_ < rhsPosition.y_;
            case 2: return lhsPosition.z_ < rhsPosition.z_;
            default:
                assert(0);
                return false;
            }
        };
    }

    static void CalculateDepthAndCapacity(unsigned size, unsigned& depth, unsigned& capacity)
    {
        depth = 0;
        capacity = 0;
        while (size > capacity)
        {
            capacity += 1 << depth;
            ++depth;
        }
    }

    static float CalculateSignedDistance(const Vector3& point, const Vector3& nodePosition, unsigned axis)
    {
        return point.Data()[axis] - nodePosition.Data()[axis];
    }

    template <class Container, class Predicate>
    void CollectNearestElements(Container& heap, unsigned depth, unsigned sparseIndex, const Vector3& point, float& maxDistanceSquared, unsigned maxElements, Predicate& predicate)
    {
        const unsigned arrayIndex = tree_[sparseIndex];
        if (arrayIndex == M_MAX_UNSIGNED)
            return;

        const Vector3 nodePosition = static_cast<Vector3>(elements_[arrayIndex]);
        const unsigned nodeAxis = depth % 3;

        // Examine children if any
        if (depth + 1 < maxDepth_)
        {
            // Check left or right plane first basing on signed distance
            const float signedDistance = CalculateSignedDistance(point, nodePosition, nodeAxis);
            const float signedDistanceSqared = signedDistance * signedDistance;
            if (signedDistance < 0)
            {
                CollectNearestElements(heap, depth + 1, (sparseIndex + 1) * 2 - 1, point, maxDistanceSquared, maxElements, predicate);
                if (signedDistanceSqared < maxDistanceSquared)
                    CollectNearestElements(heap, depth + 1, (sparseIndex + 1) * 2, point, maxDistanceSquared, maxElements, predicate);
            }
            else
            {
                CollectNearestElements(heap, depth + 1, (sparseIndex + 1) * 2, point, maxDistanceSquared, maxElements, predicate);
                if (signedDistanceSqared < maxDistanceSquared)
                    CollectNearestElements(heap, depth + 1, (sparseIndex + 1) * 2 - 1, point, maxDistanceSquared, maxElements, predicate);
            }
        }

        // Process node
        const float distanceFromNodeToPointSquared = (point - nodePosition).LengthSquared();
        KDNearestNeighbour nearestNeighbour{ arrayIndex, distanceFromNodeToPointSquared };
        if (distanceFromNodeToPointSquared < maxDistanceSquared && predicate(nearestNeighbour))
        {
            // If saturated, narrow max distance and pop furthest element
            if (heap.size() == maxElements)
            {
                maxDistanceSquared = heap.begin()->distanceSquared_;
                ea::pop_heap(heap.begin(), heap.end());
                heap.pop_back();
            }

            // Add new element
            heap.push_back(nearestNeighbour);
            ea::push_heap(heap.begin(), heap.end());
        }
    }

    ea::vector<T> elements_;
    ea::vector<unsigned> tree_;
    unsigned maxDepth_{};
    unsigned capacity_{};
    BoundingBox boundingBox_;
};

/// Size of Embree ray packed.
static const unsigned RayPacketSize = 16;

/// Description of lightmap region.
struct LightmapRegion
{
    /// Construct default.
    LightmapRegion() = default;
    /// Construct actual region.
    LightmapRegion(unsigned index, const IntVector2& position, const IntVector2& size, unsigned maxSize)
        : lightmapIndex_(index)
        , lightmapTexelRect_(position, position + size)
    {
        lightmapUVRect_.min_ = static_cast<Vector2>(lightmapTexelRect_.Min()) / static_cast<float>(maxSize);
        lightmapUVRect_.max_ = static_cast<Vector2>(lightmapTexelRect_.Max()) / static_cast<float>(maxSize);
    }
    /// Get lightmap offset vector.
    Vector4 GetScaleOffset() const
    {
        const Vector2 offset = lightmapUVRect_.Min();
        const Vector2 size = lightmapUVRect_.Size();
        return { size.x_, size.y_, offset.x_, offset.y_ };
    }

    /// Lightmap index.
    unsigned lightmapIndex_;
    /// Lightmap rectangle (in texels).
    IntRect lightmapTexelRect_;
    /// Lightmap rectangle (UV).
    Rect lightmapUVRect_;
};

/// Description of lightmap receiver.
struct LightReceiver
{
    /// Node.
    Node* node_;
    /// Static model.
    StaticModel* staticModel_{};
    /// Lightmap region.
    LightmapRegion region_;
};

/// Lightmap description.
struct LightmapDesc
{
    /// Area allocator for lightmap texture.
    AreaAllocator allocator_;
    /// Baking bakingScene.
    SharedPtr<Scene> bakingScene_;
    /// Baking camera.
    Camera* bakingCamera_{};
};

/// Calculate bounding box of all light receivers.
BoundingBox CalculateBoundingBoxOfNodes(const ea::vector<Node*>& nodes)
{
    BoundingBox boundingBox;
    for (Node* node : nodes)
    {
        if (auto staticModel = node->GetComponent<StaticModel>())
            boundingBox.Merge(staticModel->GetWorldBoundingBox());
        else if (auto terrain = node->GetComponent<Terrain>())
        {
            const IntVector2 numPatches = terrain->GetNumPatches();
            for (unsigned i = 0; i < numPatches.x_ * numPatches.y_; ++i)
            {
                if (TerrainPatch* terrainPatch = terrain->GetPatch(i))
                    boundingBox.Merge(terrainPatch->GetWorldBoundingBox());
            }
        }
    }
    return boundingBox;
}

/// Photon data.
/// TODO: Optimize and extend
struct PhotonData
{
    /// Photon position.
    Vector3 position_;
    /// Surface normal.
    Vector3 normal_;
    /// Energy.
    float energy_;

    /// Get position.
    explicit operator Vector3() const { return position_; }
};

/// Implementation of lightmap baker.
struct LightmapBakerImpl
{
    /// Construct.
    LightmapBakerImpl(Context* context, const LightmapBakingSettings& settings, Scene* scene,
        const ea::vector<Node*>& lightReceivers, const ea::vector<Node*>& lightObstacles, const ea::vector<Node*>& lights)
        : context_(context)
        , settings_(settings)
        , lightReceivers_(lightReceivers.size())
        , lightObstacles_(lightObstacles)
        , lights_(lights)
        , lightReceiversBoundingBox_(CalculateBoundingBoxOfNodes(lightReceivers))
        , lightObstaclesBoundingBox_(CalculateBoundingBoxOfNodes(lightObstacles))
    {
        for (unsigned i = 0; i < lightReceivers.size(); ++i)
            lightReceivers_[i].node_ = lightReceivers[i];
    }
    /// Destruct.
    ~LightmapBakerImpl()
    {
        if (embreeScene_)
            rtcReleaseScene(embreeScene_);
        if (embreeDevice_)
            rtcReleaseDevice(embreeDevice_);
    }
    /// Validate settings and whatever.
    bool Validate() const
    {
        if (settings_.lightmapSize_ % settings_.numParallelChunks_ != 0)
            return false;
        if (settings_.lightmapSize_ % RayPacketSize != 0)
            return false;
        return true;
    }

    /// Context.
    Context* context_{};

    /// Settings.
    const LightmapBakingSettings settings_;
    /// Scene.
    Scene* scene_{};
    /// Light receivers.
    ea::vector<LightReceiver> lightReceivers_;
    /// Light obstacles.
    ea::vector<Node*> lightObstacles_;
    /// Lights.
    ea::vector<Node*> lights_;
    /// Bounding box of light receivers.
    BoundingBox lightReceiversBoundingBox_;
    /// Bounding box of light obstacles.
    BoundingBox lightObstaclesBoundingBox_;

    /// Max length of the ray.
    float maxRayLength_{};
    /// Lightmaps.
    ea::vector<LightmapDesc> lightmaps_;
    /// Baking render path.
    SharedPtr<RenderPath> bakingRenderPath_;
    /// Embree device.
    RTCDevice embreeDevice_{};
    /// Embree scene.
    RTCScene embreeScene_{};
    /// Render texture placeholder.
    SharedPtr<Texture2D> renderTexturePlaceholder_;

    /// Photon map.
    KDTree<PhotonData> photonMap_;

    /// Calculation: current lightmap index.
    unsigned currentLightmapIndex_{ M_MAX_UNSIGNED };
    /// Calculation: texel positions
    ea::vector<Vector4> positionBuffer_;
    /// Calculation: texel smooth positions
    ea::vector<Vector4> smoothPositionBuffer_;
    /// Calculation: texel face normals
    ea::vector<Vector4> faceNormalBuffer_;
    /// Calculation: texel smooth normals
    ea::vector<Vector4> smoothNormalBuffer_;
};

/// Calculate model lightmap size.
static IntVector2 CalculateModelLightmapSize(const LightmapBakingSettings& settings,
    Model* model, const Vector3& scale)
{
    const Variant& lightmapSizeVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapSizeKey);
    const Variant& lightmapDensityVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapDensityKey);

    const auto modelLightmapSize = static_cast<Vector2>(lightmapSizeVar.GetIntVector2());
    const unsigned modelLightmapDensity = lightmapDensityVar.GetUInt();

    const float nodeScale = scale.DotProduct(DOT_SCALE);
    const float rescaleFactor = nodeScale * static_cast<float>(settings.texelDensity_) / modelLightmapDensity;
    const float clampedRescaleFactor = ea::max(settings.minLightmapScale_, rescaleFactor);

    return VectorCeilToInt(modelLightmapSize * clampedRescaleFactor);
}

/// Allocate lightmap region.
static LightmapRegion AllocateLightmapRegion(const LightmapBakingSettings& settings,
    ea::vector<LightmapDesc>& lightmaps, const IntVector2& size)
{
    const int padding = static_cast<int>(settings.lightmapPadding_);
    const IntVector2 paddedSize = size + 2 * padding * IntVector2::ONE;

    // Try existing maps
    unsigned lightmapIndex = 0;
    for (LightmapDesc& lightmapDesc : lightmaps)
    {
        IntVector2 paddedPosition;
        if (lightmapDesc.allocator_.Allocate(paddedSize.x_, paddedSize.y_, paddedPosition.x_, paddedPosition.y_))
        {
            const IntVector2 position = paddedPosition + padding * IntVector2::ONE;
            return { lightmapIndex, position, size, settings.lightmapSize_ };
        }
        ++lightmapIndex;
    }

    // Create new map
    const int lightmapSize = static_cast<int>(settings.lightmapSize_);
    LightmapDesc& lightmapDesc = lightmaps.push_back();

    // Allocate dedicated map for this specific region
    if (size.x_ > lightmapSize || size.y_ > lightmapSize)
    {
        const int sizeX = (size.x_ + RayPacketSize - 1) / RayPacketSize * RayPacketSize;

        lightmapDesc.allocator_.Reset(sizeX, size.y_, 0, 0, false);

        IntVector2 position;
        const bool success = lightmapDesc.allocator_.Allocate(sizeX, size.y_, position.x_, position.y_);

        assert(success);
        assert(position == IntVector2::ZERO);

        return { lightmapIndex, IntVector2::ZERO, size, settings.lightmapSize_ };
    }

    // Allocate chunk from new map
    lightmapDesc.allocator_.Reset(lightmapSize, lightmapSize, 0, 0, false);

    IntVector2 paddedPosition;
    const bool success = lightmapDesc.allocator_.Allocate(paddedSize.x_, paddedSize.y_, paddedPosition.x_, paddedPosition.y_);

    assert(success);
    assert(paddedPosition == IntVector2::ZERO);

    const IntVector2 position = paddedPosition + padding * IntVector2::ONE;
    return { lightmapIndex, position, size, settings.lightmapSize_ };
}

/// Allocate lightmap regions for light receivers.
static void AllocateLightmapRegions(const LightmapBakingSettings& settings,
    ea::vector<LightReceiver>& lightReceivers, ea::vector<LightmapDesc>& lightmaps)
{
    for (LightReceiver& lightReceiver : lightReceivers)
    {
        Node* node = lightReceiver.node_;

        if (auto staticModel = node->GetComponent<StaticModel>())
        {
            Model* model = staticModel->GetModel();
            const IntVector2 nodeLightmapSize = CalculateModelLightmapSize(settings, model, node->GetWorldScale());

            lightReceiver.staticModel_ = staticModel;
            lightReceiver.region_ = AllocateLightmapRegion(settings, lightmaps, nodeLightmapSize);
        }
    }
}

/// Load render path.
SharedPtr<RenderPath> LoadRenderPath(Context* context, const ea::string& renderPathName)
{
    auto renderPath = MakeShared<RenderPath>();
    auto renderPathXml = context->GetCache()->GetResource<XMLFile>(renderPathName);
    if (!renderPath->Load(renderPathXml))
        return nullptr;
    return renderPath;
}

/// Initialize camera from bounding box.
void InitializeCameraBoundingBox(Camera* camera, const BoundingBox& boundingBox)
{
    Node* node = camera->GetNode();

    const float zNear = 1.0f;
    const float zFar = boundingBox.Size().z_ + zNear;
    Vector3 position = boundingBox.Center();
    position.z_ = boundingBox.min_.z_ - zNear;

    node->SetPosition(position);
    node->SetDirection(Vector3::FORWARD);

    camera->SetOrthographic(true);
    camera->SetOrthoSize(Vector2(boundingBox.Size().x_, boundingBox.Size().y_));
    camera->SetNearClip(zNear);
    camera->SetFarClip(zFar);
}

/// Initialize lightmap baking scenes.
void InitializeLightmapBakingScenes(Context* context, Material* bakingMaterial, const BoundingBox& sceneBoundingBox,
    ea::vector<LightmapDesc>& lightmaps, ea::vector<LightReceiver>& lightReceivers)
{
    // Allocate lightmap baking scenes
    for (LightmapDesc& lightmapDesc : lightmaps)
    {
        auto bakingScene = MakeShared<Scene>(context);
        bakingScene->CreateComponent<Octree>();

        auto camera = bakingScene->CreateComponent<Camera>();
        InitializeCameraBoundingBox(camera, sceneBoundingBox);

        lightmapDesc.bakingCamera_ = camera;
        lightmapDesc.bakingScene_ = bakingScene;
    }

    // Prepare baking scenes
    for (const LightReceiver& receiver : lightReceivers)
    {
        LightmapDesc& lightmapDesc = lightmaps[receiver.region_.lightmapIndex_];
        Scene* bakingScene = lightmapDesc.bakingScene_;

        if (receiver.staticModel_)
        {
            auto material = bakingMaterial->Clone();
            material->SetShaderParameter("LMOffset", receiver.region_.GetScaleOffset());

            Node* node = bakingScene->CreateChild();
            node->SetPosition(receiver.node_->GetWorldPosition());
            node->SetRotation(receiver.node_->GetWorldRotation());
            node->SetScale(receiver.node_->GetWorldScale());

            StaticModel* staticModel = node->CreateComponent<StaticModel>();
            staticModel->SetModel(receiver.staticModel_->GetModel());
            staticModel->SetMaterial(material);
        }
    }
}
/// Parsed model key and value.
struct ParsedModelKeyValue
{
    Model* model_{};
    SharedPtr<ModelView> parsedModel_;
};

/// Parse model data.
ParsedModelKeyValue ParseModelForEmbree(Model* model)
{
    NativeModelView nativeModelView(model->GetContext());
    nativeModelView.ImportModel(model);

    auto modelView = MakeShared<ModelView>(model->GetContext());
    modelView->ImportModel(nativeModelView);

    return { model, modelView };
}

/// Embree geometry desc.
struct EmbreeGeometry
{
    /// Node.
    Node* node_{};
    /// Geometry index.
    unsigned geometryIndex_{};
    /// Geometry LOD.
    unsigned geometryLOD_{};
    /// Embree geometry.
    RTCGeometry embreeGeometry_;
};

/// Create Embree geometry from geometry view.
RTCGeometry CreateEmbreeGeometry(RTCDevice embreeDevice, const GeometryLODView& geometryLODView, Node* node)
{
    const Matrix3x4 worldTransform = node->GetWorldTransform();
    RTCGeometry embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX,
        0, RTC_FORMAT_FLOAT3, sizeof(Vector3), geometryLODView.vertices_.size()));

    for (unsigned i = 0; i < geometryLODView.vertices_.size(); ++i)
    {
        const Vector3 localPosition = static_cast<Vector3>(geometryLODView.vertices_[i].position_);
        const Vector3 worldPosition = worldTransform * localPosition;
        vertices[i * 3 + 0] = worldPosition.x_;
        vertices[i * 3 + 1] = worldPosition.y_;
        vertices[i * 3 + 2] = worldPosition.z_;
    }

    unsigned* indices = reinterpret_cast<unsigned*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX,
        0, RTC_FORMAT_UINT3, sizeof(unsigned) * 3, geometryLODView.faces_.size()));

    for (unsigned i = 0; i < geometryLODView.faces_.size(); ++i)
    {
        indices[i * 3 + 0] = geometryLODView.faces_[i].indices_[0];
        indices[i * 3 + 1] = geometryLODView.faces_[i].indices_[1];
        indices[i * 3 + 2] = geometryLODView.faces_[i].indices_[2];
    }

    rtcCommitGeometry(embreeGeometry);
    return embreeGeometry;
}

/// Create Embree geometry from parsed model.
ea::vector<EmbreeGeometry> CreateEmbreeGeometryArray(RTCDevice embreeDevice, ModelView* modelView, Node* node)
{
    ea::vector<EmbreeGeometry> result;

    unsigned geometryIndex = 0;
    for (const GeometryView& geometryView : modelView->GetGeometries())
    {
        unsigned geometryLod = 0;
        for (const GeometryLODView& geometryLODView : geometryView.lods_)
        {
            const RTCGeometry embreeGeometry = CreateEmbreeGeometry(embreeDevice, geometryLODView, node);
            result.push_back(EmbreeGeometry{ node, geometryIndex, geometryLod, embreeGeometry });
            ++geometryLod;
        }
        ++geometryIndex;
    }
    return result;
}

/// Create render surface texture for lightmap.
SharedPtr<Texture2D> CreateRenderTextureForLightmap(Context* context, int width, int height)
{
    auto texture = MakeShared<Texture2D>(context);
    texture->SetSize(width, height, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    return texture;
}

/// Read RGBA32 float texture to vector.
void ReadTextureRGBA32Float(Texture* texture, ea::vector<Vector4>& dest)
{
    auto texture2D = dynamic_cast<Texture2D*>(texture);
    const unsigned numElements = texture->GetDataSize(texture->GetWidth(), texture->GetHeight()) / sizeof(Vector4);
    dest.resize(numElements);
    texture2D->GetData(0, dest.data());
}

LightmapBaker::LightmapBaker(Context* context)
    : Object(context)
{
}

LightmapBaker::~LightmapBaker()
{
}

bool LightmapBaker::Initialize(const LightmapBakingSettings& settings, Scene* scene,
    const ea::vector<Node*>& lightReceivers, const ea::vector<Node*>& lightObstacles, const ea::vector<Node*>& lights)
{
    impl_ = ea::make_unique<LightmapBakerImpl>(context_, settings, scene, lightReceivers, lightObstacles, lights);
    if (!impl_->Validate())
        return false;

    // Prepare metadata and baking scenes
    AllocateLightmapRegions(impl_->settings_, impl_->lightReceivers_, impl_->lightmaps_);

    impl_->maxRayLength_ = impl_->lightObstaclesBoundingBox_.Size().Length();

    impl_->bakingRenderPath_ = LoadRenderPath(context_, impl_->settings_.bakingRenderPath_);

    Material* bakingMaterial = context_->GetCache()->GetResource<Material>(settings.bakingMaterial_);
    InitializeLightmapBakingScenes(context_, bakingMaterial, impl_->lightReceiversBoundingBox_,
        impl_->lightmaps_, impl_->lightReceivers_);

    // Create render surfaces
    const int lightmapSize = impl_->settings_.lightmapSize_;
    impl_->renderTexturePlaceholder_ = CreateRenderTextureForLightmap(context_, lightmapSize, lightmapSize);

    return true;
}

void LightmapBaker::CookRaytracingScene()
{
    // Load models
    ea::vector<std::future<ParsedModelKeyValue>> asyncParsedModels;
    for (Node* node : impl_->lightObstacles_)
    {
        if (auto staticModel = node->GetComponent<StaticModel>())
            asyncParsedModels.push_back(std::async(ParseModelForEmbree, staticModel->GetModel()));
    }

    // Prepare model cache
    ea::unordered_map<Model*, SharedPtr<ModelView>> parsedModelCache;
    for (auto& asyncModel : asyncParsedModels)
    {
        const ParsedModelKeyValue& parsedModel = asyncModel.get();
        parsedModelCache.emplace(parsedModel.model_, parsedModel.parsedModel_);
    }

    // Prepare Embree scene
    impl_->embreeDevice_ = rtcNewDevice("");
    impl_->embreeScene_ = rtcNewScene(impl_->embreeDevice_);

    ea::vector<std::future<ea::vector<EmbreeGeometry>>> asyncEmbreeGeometries;
    for (Node* node : impl_->lightObstacles_)
    {
        if (auto staticModel = node->GetComponent<StaticModel>())
        {
            ModelView* parsedModel = parsedModelCache[staticModel->GetModel()];
            asyncEmbreeGeometries.push_back(std::async(CreateEmbreeGeometryArray, impl_->embreeDevice_, parsedModel, node));
        }
    }

    // Collect and attach Embree geometries
    for (auto& asyncGeometry : asyncEmbreeGeometries)
    {
        const ea::vector<EmbreeGeometry> embreeGeometriesArray = asyncGeometry.get();
        for (const EmbreeGeometry& embreeGeometry : embreeGeometriesArray)
        {
            rtcAttachGeometry(impl_->embreeScene_, embreeGeometry.embreeGeometry_);
            rtcReleaseGeometry(embreeGeometry.embreeGeometry_);
        }
    }

    rtcCommitScene(impl_->embreeScene_);
}

unsigned LightmapBaker::GetNumLightmaps() const
{
    return impl_->lightmaps_.size();
}

static void RandomDirection(Vector3& result)
{
    float  len;

    do
    {
        result.x_ = (Random(1.0f) * 2.0f - 1.0f);
        result.y_ = (Random(1.0f) * 2.0f - 1.0f);
        result.z_ = (Random(1.0f) * 2.0f - 1.0f);
        len = result.Length();

    } while (len > 1.0f);

    result /= len;
}

static void RandomHemisphereDirection(Vector3& result, const Vector3& normal)
{
    RandomDirection(result);

    if (result.DotProduct(normal) < 0)
        result = -result;
}

static const float PHOTON_HASH_STEP = 0.4f;

bool LightmapBaker::BuildPhotonMap()
{
    Vector3 lightDirection;
    for (Node* lightNode : impl_->lights_)
    {
        if (auto light = lightNode->GetComponent<Light>())
        {
            if (light->GetLightType() == LIGHT_DIRECTIONAL)
            {
                lightDirection = lightNode->GetWorldDirection();
                break;
            }
        }
    }

    RTCRayHit rayHit;
    RTCIntersectContext rayContext;
    rtcInitIntersectContext(&rayContext);
    const unsigned numPhotons = 0 * 100000;
    const float radius = impl_->lightObstaclesBoundingBox_.Size().Length() / 2.0f;
    const float photonEnergy = radius * radius / numPhotons;
    const Vector3 basePosition = impl_->lightObstaclesBoundingBox_.Center();
    const Quaternion rotation{ Vector3::FORWARD, lightDirection };
    const Vector3 xAxis = rotation * Vector3::LEFT;
    const Vector3 yAxis = rotation * Vector3::UP;

    ea::vector<PhotonData> photons;
    auto emitPhoton = [&](const RTCRayHit& rayHit)
    {
        const Vector3 photonPosition{ rayHit.ray.org_x, rayHit.ray.org_y, rayHit.ray.org_z };
        const Vector3 photonNormal{ rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z };
        photons.push_back(PhotonData{ photonPosition, photonNormal.Normalized(), photonEnergy });
    };

    for (unsigned i = 0; i < numPhotons; ++i)
    {
        const Vector3 rayOrigin = basePosition
            + Random(-1.0f, 1.0f) * xAxis * radius
            + Random(-1.0f, 1.0f) * yAxis * radius
            - lightDirection * radius;
        rayHit.ray.org_x = rayOrigin.x_;
        rayHit.ray.org_y = rayOrigin.y_;
        rayHit.ray.org_z = rayOrigin.z_;
        rayHit.ray.dir_x = lightDirection.x_;
        rayHit.ray.dir_y = lightDirection.y_;
        rayHit.ray.dir_z = lightDirection.z_;
        rayHit.ray.tnear = 0.0f;
        rayHit.ray.tfar = impl_->maxRayLength_ * 2;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;
        rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(impl_->embreeScene_, &rayContext, &rayHit);

        if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
        {
            rayHit.ray.org_x += rayHit.ray.dir_x * rayHit.ray.tfar;
            rayHit.ray.org_y += rayHit.ray.dir_y * rayHit.ray.tfar;
            rayHit.ray.org_z += rayHit.ray.dir_z * rayHit.ray.tfar;

            if (Random(1.0f) < 0.5f)
            {
                //emitPhoton(rayHit);
            }
            else
            {
                rayHit.ray.tfar = impl_->maxRayLength_;
                rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

                Vector3 newDirection;
                RandomHemisphereDirection(newDirection, { rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z });
                rayHit.ray.org_x += newDirection.x_ * 0.001f;
                rayHit.ray.org_y += newDirection.y_ * 0.001f;
                rayHit.ray.org_z += newDirection.z_ * 0.001f;
                rayHit.ray.dir_x = newDirection.x_;
                rayHit.ray.dir_y = newDirection.y_;
                rayHit.ray.dir_z = newDirection.z_;

                rtcIntersect1(impl_->embreeScene_, &rayContext, &rayHit);

                if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
                {
                    rayHit.ray.org_x += rayHit.ray.dir_x * rayHit.ray.tfar;
                    rayHit.ray.org_y += rayHit.ray.dir_y * rayHit.ray.tfar;
                    rayHit.ray.org_z += rayHit.ray.dir_z * rayHit.ray.tfar;

                    emitPhoton(rayHit);
                }
            }

        }
    }
    impl_->photonMap_.Initialize(ea::move(photons));
    return true;
}

bool LightmapBaker::RenderLightmapGBuffer(unsigned index)
{
    if (index >= GetNumLightmaps())
        return false;

    Graphics* graphics = GetGraphics();
    ResourceCache* cache = GetCache();
    const LightmapDesc& lightmapDesc = impl_->lightmaps_[index];

    // Prepare render surface
    const int lightmapWidth = lightmapDesc.allocator_.GetWidth();
    const int lightmapHeight = lightmapDesc.allocator_.GetHeight();
    SharedPtr<Texture2D> renderTexture = impl_->renderTexturePlaceholder_;
    if (impl_->settings_.lightmapSize_ != lightmapWidth || impl_->settings_.lightmapSize_ != lightmapHeight)
        renderTexture = CreateRenderTextureForLightmap(context_, lightmapWidth, lightmapHeight);
    RenderSurface* renderSurface = renderTexture->GetRenderSurface();

    if (!graphics->BeginFrame())
        return false;

    // Setup viewport
    Viewport viewport(context_);
    viewport.SetCamera(lightmapDesc.bakingCamera_);
    viewport.SetRect(IntRect::ZERO);
    viewport.SetRenderPath(impl_->bakingRenderPath_);
    viewport.SetScene(lightmapDesc.bakingScene_);

    // Render bakingScene
    View view(context_);
    view.Define(renderSurface, &viewport);
    view.Update(FrameInfo());
    view.Render();

    graphics->EndFrame();

    // Fill temporary buffers
    impl_->currentLightmapIndex_ = index;

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("position"), impl_->positionBuffer_);
    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothposition"), impl_->smoothPositionBuffer_);
    ReadTextureRGBA32Float(view.GetExtraRenderTarget("facenormal"), impl_->faceNormalBuffer_);
    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothnormal"), impl_->smoothNormalBuffer_);

    return true;
}

template <class T>
void LightmapBaker::ParallelForEachRow(T callback)
{
    const LightmapDesc& lightmapDesc = impl_->lightmaps_[impl_->currentLightmapIndex_];
    const unsigned lightmapHeight = static_cast<unsigned>(lightmapDesc.allocator_.GetHeight());
    const unsigned chunkHeight = lightmapHeight / impl_->settings_.numParallelChunks_;

    // Start tasks
    ea::vector<std::future<void>> tasks;
    for (unsigned parallelChunkIndex = 0; parallelChunkIndex < impl_->settings_.numParallelChunks_; ++parallelChunkIndex)
    {
        tasks.push_back(std::async([=]()
        {
            const unsigned fromY = parallelChunkIndex * chunkHeight;
            const unsigned toY = ea::min((parallelChunkIndex + 1) * chunkHeight, lightmapHeight);
            for (unsigned y = fromY; y < toY; ++y)
                callback(y);
        }));
    }

    // Wait for async tasks
    for (auto& task : tasks)
        task.wait();
}

bool LightmapBaker::BakeLightmap(LightmapBakedData& data)
{
    const LightmapDesc& lightmapDesc = impl_->lightmaps_[impl_->currentLightmapIndex_];
    const int lightmapWidth = lightmapDesc.allocator_.GetWidth();
    const int lightmapHeight = lightmapDesc.allocator_.GetHeight();

    // Prepare output buffers
    data.lightmapSize_ = { lightmapWidth, lightmapHeight };
    data.backedLighting_.resize(lightmapWidth * lightmapHeight);
    ea::fill(data.backedLighting_.begin(), data.backedLighting_.end(), Color::WHITE);

    Vector3 lightDirection;
    for (Node* lightNode : impl_->lights_)
    {
        if (auto light = lightNode->GetComponent<Light>())
        {
            if (light->GetLightType() == LIGHT_DIRECTIONAL)
            {
                lightDirection = lightNode->GetWorldDirection();
                break;
            }
        }
    }
    const Vector3 lightRayDirection = -lightDirection.Normalized();

    // Process rows in multiple threads
    const unsigned numBounces = 1;
    const unsigned numRayPackets = static_cast<unsigned>(lightmapWidth / RayPacketSize);
    ParallelForEachRow([=, &data](unsigned y)
    {
    #if 0
        alignas(64) RTCRayHit16 rayHit16;
        alignas(64) int rayValid[RayPacketSize];
        float diffuseLight[RayPacketSize];
        float indirectLight[RayPacketSize];
        ea::fixed_vector<KDNearestNeighbour, 128> nearestPhotons;
    #endif
        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        for (unsigned rayPacketIndex = 0; rayPacketIndex < numRayPackets; ++rayPacketIndex)
        {
            const unsigned fromX = rayPacketIndex * RayPacketSize;
            const unsigned baseIndex = y * lightmapWidth + fromX;

            for (unsigned i = 0; i < RayPacketSize; ++i)
            {
                const unsigned index = baseIndex + i;
                const Vector3 position = static_cast<Vector3>(impl_->positionBuffer_[index]);
                const Vector3 smoothNormal = static_cast<Vector3>(impl_->smoothNormalBuffer_[index]);
                const unsigned geometryId = static_cast<unsigned>(impl_->positionBuffer_[index].w_);

                if (!geometryId)
                    continue;

                // Cast direct ray
                rayHit.ray.org_x = position.x_ + lightRayDirection.x_ * 0.001f;
                rayHit.ray.org_y = position.y_ + lightRayDirection.y_ * 0.001f;
                rayHit.ray.org_z = position.z_ + lightRayDirection.z_ * 0.001f;
                rayHit.ray.dir_x = lightRayDirection.x_;
                rayHit.ray.dir_y = lightRayDirection.y_;
                rayHit.ray.dir_z = lightRayDirection.z_;
                rayHit.ray.tnear = 0.0f;
                rayHit.ray.tfar = impl_->maxRayLength_ * 2;
                rayHit.ray.time = 0.0f;
                rayHit.ray.id = 0;
                rayHit.ray.mask = 0xffffffff;
                rayHit.ray.flags = 0xffffffff;
                rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIntersect1(impl_->embreeScene_, &rayContext, &rayHit);

                const float directShadow = rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID ? 1.0f : 0.0f;
                const float directLighting = directShadow * ea::max(0.0f, smoothNormal.DotProduct(lightRayDirection));

                float indirectLighting = 0.0f;
                Vector3 currentPosition = position;
                Vector3 currentNormal = smoothNormal;
                for (unsigned j = 0; j < numBounces; ++j)
                {
                    // Get new ray direction
                    Vector3 rayDirection;
                    RandomHemisphereDirection(rayDirection, currentNormal);

                    rayHit.ray.org_x = currentPosition.x_;
                    rayHit.ray.org_y = currentPosition.y_;
                    rayHit.ray.org_z = currentPosition.z_;
                    rayHit.ray.dir_x = rayDirection.x_;
                    rayHit.ray.dir_y = rayDirection.y_;
                    rayHit.ray.dir_z = rayDirection.z_;
                    rayHit.ray.tnear = 0.0f;
                    rayHit.ray.tfar = impl_->maxRayLength_ * 2;
                    rayHit.ray.time = 0.0f;
                    rayHit.ray.id = 0;
                    rayHit.ray.mask = 0xffffffff;
                    rayHit.ray.flags = 0xffffffff;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(impl_->embreeScene_, &rayContext, &rayHit);

                    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                        continue;

                    // Cast direct ray
                    rayHit.ray.org_x += rayHit.ray.dir_x * rayHit.ray.tfar;
                    rayHit.ray.org_y += rayHit.ray.dir_y * rayHit.ray.tfar;
                    rayHit.ray.org_z += rayHit.ray.dir_z * rayHit.ray.tfar;
                    rayHit.ray.dir_x = lightRayDirection.x_;
                    rayHit.ray.dir_y = lightRayDirection.y_;
                    rayHit.ray.dir_z = lightRayDirection.z_;
                    rayHit.ray.tnear = 0.0f;
                    rayHit.ray.tfar = impl_->maxRayLength_ * 2;
                    rayHit.ray.time = 0.0f;
                    rayHit.ray.id = 0;
                    rayHit.ray.mask = 0xffffffff;
                    rayHit.ray.flags = 0xffffffff;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(impl_->embreeScene_, &rayContext, &rayHit);

                    if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
                        continue;

                    const float incoming = 1.0f;
                    const float probability = 1 / (2 * M_PI);
                    const float cosTheta = rayDirection.DotProduct(currentNormal);
                    const float reflectance = 1 / M_PI;
                    const float brdf = reflectance / M_PI;

                    indirectLighting = incoming * brdf * cosTheta / probability;
                }

                data.backedLighting_[index] = Color::WHITE * (directLighting + indirectLighting);
            }

#if 0
            unsigned numValidRays = 0;
            for (unsigned i = 0; i < RayPacketSize; ++i)
            {
                const unsigned index = baseIndex + i;

                const unsigned geometryId = static_cast<unsigned>(impl_->positionBuffer_[index].w_);
                if (!geometryId)
                {
                    rayValid[i] = 0;
                    rayHit16.ray.tnear[i] = 0.0f;
                    rayHit16.ray.tfar[i] = -1.0f;
                    rayHit16.hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;
                    continue;
                }

                const Vector3 position = static_cast<Vector3>(impl_->positionBuffer_[index]);
                const Vector3 smoothNormal = static_cast<Vector3>(impl_->smoothNormalBuffer_[index]);

                indirectLight[i] = 0.0f;
                diffuseLight[i] = ea::max(0.0f, smoothNormal.DotProduct(rayDirection));

                impl_->photonMap_.CollectNearestElements(nearestPhotons, position, PHOTON_HASH_STEP, nearestPhotons.max_size(),
                    [&](const KDNearestNeighbour& neighbourPhoton)
                {
                    const PhotonData& photon = impl_->photonMap_[neighbourPhoton.index_];
                    const float hemiFactor = smoothNormal.DotProduct(photon.normal_);
                    if (hemiFactor > 0.5f)
                        return true;
                    return false;
                });
                for (const KDNearestNeighbour& neighbourPhoton : nearestPhotons)
                {
                    const PhotonData& photon = impl_->photonMap_[neighbourPhoton.index_];
                    const Vector3 delta = photon.position_ - position;
                    const float distance = Sqrt(neighbourPhoton.distanceSquared_);
                    const float maxDistance = Sqrt(nearestPhotons.front().distanceSquared_);
                    const float hackFactor = 5.0f;
                    const float area = M_PI * maxDistance * maxDistance;
                    indirectLight[i] += hackFactor * (1.0f - distance / maxDistance) * photon.energy_ / area;
                }

                const Vector3 rayOrigin = position + rayDirection * 0.001f;

                rayValid[i] = -1;
                rayHit16.ray.org_x[i] = rayOrigin.x_;
                rayHit16.ray.org_y[i] = rayOrigin.y_;
                rayHit16.ray.org_z[i] = rayOrigin.z_;
                rayHit16.ray.dir_x[i] = rayDirection.x_;
                rayHit16.ray.dir_y[i] = rayDirection.y_;
                rayHit16.ray.dir_z[i] = rayDirection.z_;
                rayHit16.ray.tnear[i] = 0.0f;
                rayHit16.ray.tfar[i] = impl_->maxRayLength_;
                rayHit16.ray.time[i] = 0.0f;
                rayHit16.ray.id[i] = 0;
                rayHit16.ray.mask[i] = 0xffffffff;
                rayHit16.ray.flags[i] = 0xffffffff;
                rayHit16.hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;

                ++numValidRays;
            }

            if (numValidRays == 0)
                continue;

            RTCIntersectContext rayContext;
            rtcInitIntersectContext(&rayContext);
            rtcIntersect16(rayValid, impl_->embreeScene_, &rayContext, &rayHit16);

            for (unsigned i = 0; i < RayPacketSize; ++i)
            {
                if (rayValid[i])
                {
                    const unsigned index = baseIndex + i;
                    const float shadow = rayHit16.hit.geomID[i] == RTC_INVALID_GEOMETRY_ID ? 1.0f : 0.0f;
                    data.backedLighting_[index] = Color::WHITE * (diffuseLight[i] * shadow + indirectLight[i]);
                    //data.backedLighting_[index] = Color::WHITE * indirectLight[i];
                }
            }
#endif
        }
    });

    IntVector2 offsets[25];
    for (int i = 0; i < 25; ++i)
        offsets[i] = IntVector2(i % 5 - 2, i / 5 - 2);
    float kernel[25];
    int kernel1D[5] = { 1, 4, 6, 4, 1 };
    for (int i = 0; i < 25; ++i)
        kernel[i] = kernel1D[i % 5] * kernel1D[i / 5] / 256.0f;

    const float colorPhi = 1.0f;
    const float normalPhi = 4.0f;
    const float positionPhi = 1.0f;

    for (unsigned passIndex = 0; passIndex < 3; ++passIndex)
    {
        const int offsetScale = 1 << passIndex;
        const float colorPhiScaled = colorPhi / offsetScale;
        auto colorCopy = data.backedLighting_;
        ParallelForEachRow([=, &data](unsigned y)
        {
            for (unsigned x = 0; x < lightmapWidth; ++x)
            {
                const IntVector2 sourceTexel{ static_cast<int>(x), static_cast<int>(y) };
                const IntVector2 minOffset = -sourceTexel;
                const IntVector2 maxOffset = IntVector2{ lightmapWidth, lightmapHeight } - sourceTexel - IntVector2::ONE;

                const unsigned index = y * lightmapWidth + x;

                const Vector4 baseColor = colorCopy[index].ToVector4();
                const Vector3 basePosition = static_cast<Vector3>(impl_->smoothPositionBuffer_[index]);
                const Vector3 baseNormal = static_cast<Vector3>(impl_->smoothNormalBuffer_[index]);
                const unsigned geometryId = static_cast<unsigned>(impl_->positionBuffer_[index].w_);
                if (!geometryId)
                    continue;

                Vector4 colorSum;
                float weightSum = 0.0f;
                for (unsigned i = 0; i < 25; ++i)
                {
                    const IntVector2 offset = offsets[i] * offsetScale;
                    const IntVector2 clampedOffset = VectorMax(minOffset, VectorMin(offset, maxOffset));
                    const unsigned otherIndex = index + clampedOffset.y_ * lightmapWidth + clampedOffset.x_;

                    const Vector4 otherColor = colorCopy[otherIndex].ToVector4();
                    const Vector4 colorDelta = baseColor - otherColor;
                    const float colorDeltaSquared = colorDelta.DotProduct(colorDelta);
                    const float colorWeight = ea::min(std::exp(-colorDeltaSquared / colorPhiScaled), 1.0f);

                    const Vector3 otherPosition = static_cast<Vector3>(impl_->smoothPositionBuffer_[otherIndex]);
                    const Vector3 positionDelta = basePosition - otherPosition;
                    const float positionDeltaSquared = positionDelta.DotProduct(positionDelta);
                    const float positionWeight = ea::min(std::exp(-positionDeltaSquared / positionPhi), 1.0f);

                    const Vector3 otherNormal = static_cast<Vector3>(impl_->smoothNormalBuffer_[otherIndex]);
                    const float normalDeltaCos = ea::max(0.0f, baseNormal.DotProduct(otherNormal));
                    const float normalWeight = std::pow(normalDeltaCos, normalPhi);

                    const float weight = colorWeight * positionWeight * normalWeight * kernel[i];
                    colorSum += otherColor * weight;
                    weightSum += weight;
                }

                const Vector4 result = colorSum / weightSum;
                data.backedLighting_[index] = Color(result.x_, result.y_, result.z_, result.w_);
            }
        });
    }

    return true;
}

void LightmapBaker::ApplyLightmapsToScene(unsigned baseLightmapIndex)
{
    for (const LightReceiver& receiver : impl_->lightReceivers_)
    {
        if (receiver.staticModel_)
        {
            receiver.staticModel_->SetLightmap(true);
            receiver.staticModel_->SetLightmapIndex(baseLightmapIndex + receiver.region_.lightmapIndex_);
            receiver.staticModel_->SetLightmapScaleOffset(receiver.region_.GetScaleOffset());
        }
    }
}

}
