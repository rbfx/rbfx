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

/// \file

#pragma once

#include "../Container/Ptr.h"
#include "../Glow/EmbreeForward.h"
#include "../Graphics/LightmapSettings.h"
#include "../Math/BoundingBox.h"
#include "../Math/Color.h"
#include "../Resource/Image.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Context;
class Node;
class StaticModel;

/// Geometry for ray tracing.
struct RaytracerGeometry
{
    /// Object index.
    unsigned objectIndex_{};
    /// Geometry index.
    unsigned geometryIndex_{};
    /// LOD index.
    unsigned lodIndex_{};
    /// Number of LODs.
    unsigned numLods_{};
    /// Lightmap chart index.
    unsigned lightmapIndex_{};
    /// Raytracer geometry ID, aka index of this structure in the array of geometries.
    unsigned raytracerGeometryId_{};
    /// Internal geometry pointer.
    RTCGeometry embreeGeometry_{};
    /// Whether the geometry is opaque.
    bool opaque_{};
    /// Diffuse color of geometry.
    Vector3 diffuseColor_{};
    /// Alpha value.
    float alpha_{};
    /// Resource name of diffuse image.
    ea::string diffuseImageName_;
    /// Diffuse image.
    SharedPtr<Image> diffuseImage_;
    /// Diffuse image width.
    int diffuseImageWidth_{};
    /// Diffuse image height.
    int diffuseImageHeight_{};
};

/// Compare Embree geometries by objects (less).
inline bool CompareRaytracerGeometryByObject(const RaytracerGeometry& lhs, const RaytracerGeometry& rhs)
{
    if (lhs.objectIndex_ != rhs.objectIndex_)
        return lhs.objectIndex_ < rhs.objectIndex_;

    if (lhs.geometryIndex_ != rhs.geometryIndex_)
        return lhs.geometryIndex_ < rhs.geometryIndex_;

    return lhs.lodIndex_ < rhs.lodIndex_;
}

/// Scene for ray tracing.
class URHO3D_API RaytracerScene : public RefCounted
{
public:
    /// Vertex attribute for lightmap UV.
    static const unsigned LightmapUVAttribute = 0;
    /// Vertex attribute for smooth normal.
    static const unsigned NormalAttribute = 1;
    /// Vertex attribute for primary UV.
    static const unsigned UVAttribute = 2;

    /// Mask for LOD 0.
    static const unsigned PrimaryLODGeometry = 0x00000001;
    /// Mask for LODs 1..N.
    static const unsigned SecondaryLODGeometry = 0x00000002;
    /// Mask for all geometry.
    static const unsigned AllGeometry = 0xffffffff;

    /// Construct.
    RaytracerScene(Context* context, RTCDevice embreeDevice, RTCScene raytracerScene,
        ea::vector<RaytracerGeometry> geometries, float maxDistance)
        : context_(context)
        , device_(embreeDevice)
        , scene_(raytracerScene)
        , geometries_(ea::move(geometries))
        , maxDistance_(maxDistance)
    {
    }
    /// Destruct.
    ~RaytracerScene() override;

    /// Return context.
    Context* GetContext() const { return context_; }
    /// Return Embree device.
    RTCDevice GetEmbreeDevice() const { return device_; }
    /// Return Embree scene.
    RTCScene GetEmbreeScene() const { return scene_; }
    /// Return geometries.
    const ea::vector<RaytracerGeometry>& GetGeometries() const { return geometries_; }
    /// Return max distance between two points.
    float GetMaxDistance() const { return maxDistance_; }

private:
    /// Context.
    Context* context_{};
    /// Embree device.
    RTCDevice device_{};
    /// Embree scene.
    RTCScene scene_{};
    /// Geometries.
    ea::vector<RaytracerGeometry> geometries_;
    /// Max distance between two points.
    float maxDistance_{};
};

// Create scene for raytracing.
URHO3D_API SharedPtr<RaytracerScene> CreateRaytracingScene(Context* context,
    const ea::vector<StaticModel*>& staticModels, unsigned uvChannel);

}
