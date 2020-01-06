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
#include "../IO/Log.h"

#include <future>

namespace Urho3D
{

namespace
{

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
RTCGeometry CreateEmbreeGeometry(RTCDevice embreeDevice, const GeometryLODView& geometryLODView, Node* node,
    const Vector2& lightmapUVScale, const Vector2& lightmapUVOffset, unsigned uvChannel, unsigned mask)
{
    const Matrix3x4 worldTransform = node->GetWorldTransform();
    RTCGeometry embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);

    float* vertices = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX,
        0, RTC_FORMAT_FLOAT3, sizeof(Vector3), geometryLODView.vertices_.size()));

    rtcSetGeometryVertexAttributeCount(embreeGeometry, 1);
    float* lightmapUVs = reinterpret_cast<float*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        0, RTC_FORMAT_FLOAT2, sizeof(Vector2), geometryLODView.vertices_.size()));

    for (unsigned i = 0; i < geometryLODView.vertices_.size(); ++i)
    {
        const Vector3 localPosition = static_cast<Vector3>(geometryLODView.vertices_[i].position_);
        const Vector4 lightmapUV = geometryLODView.vertices_[i].uv_[uvChannel];
        const Vector2 lightmapUVScaled = Vector2{ lightmapUV.x_, lightmapUV.y_ } * lightmapUVScale + lightmapUVOffset;
        const Vector3 worldPosition = worldTransform * localPosition;
        vertices[i * 3 + 0] = worldPosition.x_;
        vertices[i * 3 + 1] = worldPosition.y_;
        vertices[i * 3 + 2] = worldPosition.z_;
        lightmapUVs[i * 2 + 0] = lightmapUVScaled.x_;
        lightmapUVs[i * 2 + 1] = lightmapUVScaled.y_;
    }

    unsigned* indices = reinterpret_cast<unsigned*>(rtcSetNewGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX,
        0, RTC_FORMAT_UINT3, sizeof(unsigned) * 3, geometryLODView.indices_.size() / 3));

    for (unsigned i = 0; i < geometryLODView.indices_.size(); ++i)
        indices[i] = geometryLODView.indices_[i];

    rtcSetGeometryMask(embreeGeometry, mask);
    rtcCommitGeometry(embreeGeometry);
    return embreeGeometry;
}

/// Create Embree geometry from parsed model.
ea::vector<EmbreeGeometry> CreateEmbreeGeometriesForModel(
    RTCDevice embreeDevice, ModelView* modelView, Node* node, unsigned objectIndex,
    unsigned lightmapIndex, const Vector2& lightmapUVScale, const Vector2& lightmapUVOffset, unsigned uvChannel)
{
    ea::vector<EmbreeGeometry> result;

    const ea::vector<GeometryView> geometries = modelView->GetGeometries();
    for (unsigned geometryIndex = 0; geometryIndex < geometries.size(); ++geometryIndex)
    {
        const GeometryView& geometryView = geometries[geometryIndex];
        for (unsigned lodIndex = 0; lodIndex < geometryView.lods_.size(); ++lodIndex)
        {
            const GeometryLODView& geometryLODView = geometryView.lods_[lodIndex];
            const unsigned mask = lodIndex == 0 ? EmbreeScene::PrimaryLODGeometry : EmbreeScene::SecondaryLODGeometry;
            const RTCGeometry embreeGeometry = CreateEmbreeGeometry(embreeDevice, geometryLODView,
                node, lightmapUVScale, lightmapUVOffset, uvChannel, mask);
            result.push_back(EmbreeGeometry{ objectIndex, geometryIndex, lodIndex,
                lightmapIndex, M_MAX_UNSIGNED, embreeGeometry });
        }
    }
    return result;
}

}

BoundingBox CalculateBoundingBoxOfNodes(const ea::vector<Node*>& nodes, bool padIfZero)
{
    ea::vector<StaticModel*> staticModels;
    ea::vector<Terrain*> terrains;

    BoundingBox boundingBox;
    for (Node* node : nodes)
    {
        node->GetComponents(staticModels);
        node->GetComponents(terrains);

        for (StaticModel* staticModel : staticModels)
            boundingBox.Merge(staticModel->GetWorldBoundingBox());

        for (Terrain* terrain : terrains)
        {
            const IntVector2 numPatches = terrain->GetNumPatches();
            for (unsigned i = 0; i < numPatches.x_ * numPatches.y_; ++i)
            {
                if (TerrainPatch* terrainPatch = terrain->GetPatch(i))
                    boundingBox.Merge(terrainPatch->GetWorldBoundingBox());
            }
        }
    }

    // Pad bounding box
    if (padIfZero)
    {
        const Vector3 size = boundingBox.Size();
        if (size.x_ < M_EPSILON)
            boundingBox.max_.x_ += M_LARGE_EPSILON;
        if (size.y_ < M_EPSILON)
            boundingBox.max_.y_ += M_LARGE_EPSILON;
        if (size.z_ < M_EPSILON)
            boundingBox.max_.z_ += M_LARGE_EPSILON;
    }

    return boundingBox;
}

EmbreeScene::~EmbreeScene()
{
    if (scene_)
        rtcReleaseScene(scene_);
    if (device_)
        rtcReleaseDevice(device_);
}

SharedPtr<EmbreeScene> CreateEmbreeScene(Context* context, const ea::vector<StaticModel*>& staticModels, unsigned uvChannel)
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

        const unsigned lightmapIndex = staticModel->GetLightmapIndex();
        const Vector4 lightmapUVScaleOffset = staticModel->GetLightmapScaleOffset();
        const Vector2 lightmapUVScale{ lightmapUVScaleOffset.x_, lightmapUVScaleOffset.y_ };
        const Vector2 lightmapUVOffset{ lightmapUVScaleOffset.z_, lightmapUVScaleOffset.w_ };

        ModelView* parsedModel = parsedModelCache[staticModel->GetModel()];
        createEmbreeGeometriesTasks.push_back(std::async(CreateEmbreeGeometriesForModel,
            device, parsedModel, staticModel->GetNode(),
            objectIndex, lightmapIndex, lightmapUVScale, lightmapUVOffset, uvChannel));
    }

    // Collect and attach Embree geometries
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
        }
    }

    // Finalize scene
    rtcCommitScene(scene);

    // Calculate max distance between objects
    BoundingBox boundingBox;
    for (StaticModel* staticModel : staticModels)
        boundingBox.Merge(staticModel->GetWorldBoundingBox());
    const Vector3 sceneSize = boundingBox.Size();
    const float maxDistance = ea::max({ sceneSize.x_, sceneSize.y_, sceneSize.z_ });

    return MakeShared<EmbreeScene>(context, device, scene, ea::move(geometryIndex), maxDistance);
}

}
