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

#include "../Graphics/ModelView.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#include "../Glow/EmbreeScene.h"

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
    NativeModelView nativeModelView(model->GetContext());
    nativeModelView.ImportModel(model);

    auto modelView = MakeShared<ModelView>(model->GetContext());
    modelView->ImportModel(nativeModelView);

    return { model, modelView };
}

/// Create Embree geometry from geometry view.
RTCGeometry CreateEmbreeGeometry(RTCDevice embreeDevice, const GeometryLODView& geometryLODView, Node* node,
    const Vector2& lightmapUVScale, const Vector2& lightmapUVOffset)
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
        const Vector4 lightmapUV = geometryLODView.vertices_[i].uv_[1];
        const Vector2 lightmapUVScaled = Vector2{ lightmapUV.x_, lightmapUV.y_ } * lightmapUVScale + lightmapUVOffset;
        const Vector3 worldPosition = worldTransform * localPosition;
        vertices[i * 3 + 0] = worldPosition.x_;
        vertices[i * 3 + 1] = worldPosition.y_;
        vertices[i * 3 + 2] = worldPosition.z_;
        lightmapUVs[i * 2 + 0] = lightmapUVScaled.x_;
        lightmapUVs[i * 2 + 1] = lightmapUVScaled.y_;
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
ea::vector<EmbreeGeometry> CreateEmbreeGeometryArray(RTCDevice embreeDevice, ModelView* modelView, Node* node,
    const Vector2& lightmapUVScale, const Vector2& lightmapUVOffset)
{
    ea::vector<EmbreeGeometry> result;

    unsigned geometryIndex = 0;
    for (const GeometryView& geometryView : modelView->GetGeometries())
    {
        unsigned geometryLod = 0;
        for (const GeometryLODView& geometryLODView : geometryView.lods_)
        {
            const RTCGeometry embreeGeometry = CreateEmbreeGeometry(embreeDevice, geometryLODView,
                node, lightmapUVScale, lightmapUVOffset);
            result.push_back(EmbreeGeometry{ node, geometryIndex, geometryLod, embreeGeometry });
            ++geometryLod;
        }
        ++geometryIndex;
    }
    return result;
}

}

BoundingBox CalculateBoundingBoxOfNodes(const ea::vector<Node*>& nodes, bool padIfZero)
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

SharedPtr<EmbreeScene> CreateEmbreeScene(Context* context, const ea::vector<Node*>& nodes)
{
    // Load models
    ea::vector<std::future<ParsedModelKeyValue>> asyncParsedModels;
    for (Node* node : nodes)
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
    const RTCDevice device = rtcNewDevice("");
    const RTCScene scene = rtcNewScene(device);

    ea::vector<std::future<ea::vector<EmbreeGeometry>>> asyncEmbreeGeometries;
    for (Node* node : nodes)
    {
        if (auto staticModel = node->GetComponent<StaticModel>())
        {
            const Vector4 lightmapUVScaleOffset = staticModel->GetLightmapScaleOffset();
            const Vector2 lightmapUVScale{ lightmapUVScaleOffset.x_, lightmapUVScaleOffset.y_ };
            const Vector2 lightmapUVOffset{ lightmapUVScaleOffset.z_, lightmapUVScaleOffset.w_ };

            ModelView* parsedModel = parsedModelCache[staticModel->GetModel()];
            asyncEmbreeGeometries.push_back(std::async(CreateEmbreeGeometryArray,
                device, parsedModel, node, lightmapUVScale, lightmapUVOffset));
        }
    }

    // Collect and attach Embree geometries
    ea::vector<EmbreeGeometry> geometryIndex;
    for (auto& asyncGeometry : asyncEmbreeGeometries)
    {
        const ea::vector<EmbreeGeometry> embreeGeometriesArray = asyncGeometry.get();
        for (const EmbreeGeometry& embreeGeometry : embreeGeometriesArray)
        {
            const unsigned geomID = rtcAttachGeometry(scene, embreeGeometry.embreeGeometry_);
            rtcReleaseGeometry(embreeGeometry.embreeGeometry_);

            geometryIndex.resize(geomID + 1);
            geometryIndex[geomID] = embreeGeometry;
        }
    }

    // Finalize scene
    rtcCommitScene(scene);

    // Calculate max distance between objects
    const Vector3 sceneSize = CalculateBoundingBoxOfNodes(nodes).Size();
    const float maxDistance = ea::max({ sceneSize.x_, sceneSize.y_, sceneSize.z_ });

    return MakeShared<EmbreeScene>(context, device, scene, ea::move(geometryIndex), maxDistance);
}

}
