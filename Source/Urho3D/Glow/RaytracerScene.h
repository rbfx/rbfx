// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Container/Ptr.h"
#include "../Glow/BakedSceneBackground.h"
#include "../Glow/EmbreeForward.h"
#include "../Graphics/LightBakingSettings.h"
#include "../Math/BoundingBox.h"
#include "../Math/Color.h"
#include "../Resource/Image.h"
#include "../Resource/ImageCube.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Context;
class Node;
class Component;

/// Material of raytracing geometry.
struct RaytracingGeometryMaterial
{
    /// Whether the material is opaque.
    bool opaque_{};
    /// Diffuse color.
    Vector3 diffuseColor_{};
    /// Alpha value.
    float alpha_{};

    /// Whether to store main texture UV.
    bool storeUV_{};
    /// Transform for U coordinate.
    Vector4 uOffset_;
    /// Transform for V coordinate.
    Vector4 vOffset_;

    /// Resource name of diffuse image.
    ea::string diffuseImageName_;
    /// Diffuse image.
    SharedPtr<Image> diffuseImage_;
    /// Diffuse image width.
    int diffuseImageWidth_{};
    /// Diffuse image height.
    int diffuseImageHeight_{};

    /// Return transformed UV coordinates.
    Vector2 ConvertUV(const Vector2& uv) const
    {
        const float u = uv.DotProduct(uOffset_.ToVector2()) + uOffset_.w_;
        const float v = uv.DotProduct(vOffset_.ToVector2()) + vOffset_.w_;
        return { u, v };
    }

    /// Return diffuse value at UV.
    Color SampleDiffuse(const Vector2& uv) const
    {
        const int x = Clamp(RoundToInt(uv.x_ * diffuseImageWidth_), 0, diffuseImageWidth_ - 1);
        const int y = Clamp(RoundToInt(uv.y_ * diffuseImageHeight_), 0, diffuseImageHeight_ - 1);
        return diffuseImage_->GetPixel(x, y);
    }
};

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
    embree3::RTCGeometry embreeGeometry_{};
    /// Material.
    RaytracingGeometryMaterial material_;
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
    /// Max number of vertex attributes.
    static const unsigned MaxAttributes = 3;

    /// Mask for lightmapped geometry, LOD 0.
    static const unsigned PrimaryLODGeometry = 0x00000001;
    /// Mask for lightmapped geometry, LODs 1..N.
    static const unsigned SecondaryLODGeometry = 0x00000002;
    /// Mask for non-lightmapped geometry, LOD 0.
    static const unsigned DirectShadowOnlyGeometry = 0x00000004;
    /// Mask for all geometry.
    static const unsigned AllGeometry = 0xffffffff;

    /// Construct.
    RaytracerScene(Context* context, embree3::RTCDevice embreeDevice, embree3::RTCScene raytracerScene,
        ea::vector<RaytracerGeometry> geometries, const BakedSceneBackgroundArrayPtr& backgrounds, float maxDistance)
        : context_(context)
        , device_(embreeDevice)
        , scene_(raytracerScene)
        , geometries_(ea::move(geometries))
        , backgrounds_(backgrounds)
        , maxDistance_(maxDistance)
    {
    }
    /// Destruct.
    ~RaytracerScene() override;

    /// Return context.
    Context* GetContext() const { return context_; }
    /// Return Embree device.
    embree3::RTCDevice GetEmbreeDevice() const { return device_; }
    /// Return Embree scene.
    embree3::RTCScene GetEmbreeScene() const { return scene_; }
    /// Return geometries.
    const ea::vector<RaytracerGeometry>& GetGeometries() const { return geometries_; }
    /// Return background.
    const BakedSceneBackgroundArrayPtr& GetBackgrounds() const { return backgrounds_; }
    /// Return max distance between two points.
    float GetMaxDistance() const { return maxDistance_; }

private:
    /// Context.
    Context* context_{};
    /// Embree device.
    embree3::RTCDevice device_{};
    /// Embree scene.
    embree3::RTCScene scene_{};
    /// Geometries.
    ea::vector<RaytracerGeometry> geometries_;
    /// Background.
    BakedSceneBackgroundArrayPtr backgrounds_;
    /// Max distance between two points.
    float maxDistance_{};
};

// Create scene for raytracing.
URHO3D_API SharedPtr<RaytracerScene> CreateRaytracingScene(Context* context,
    const ea::vector<Component*>& geometries, unsigned uvChannel, const BakedSceneBackgroundArrayPtr& background);

}
