//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../Graphics/Drawable.h"

namespace Urho3D
{

class Model;

/// Static model per-geometry extra data.
struct StaticModelGeometryData
{
    /// Geometry center.
    Vector3 center_;
    /// Current LOD level.
    unsigned lodLevel_;
};

/// Static model component.
class URHO3D_API StaticModel : public Drawable
{
    URHO3D_OBJECT(StaticModel, Drawable);

public:
    /// Construct.
    explicit StaticModel(Context* context);
    /// Destruct.
    ~StaticModel() override;
    /// Register object factory. Drawable must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Process raycast with custom transform.
    void ProcessCustomRayQuery(const RayOctreeQuery& query, const BoundingBox& worldBoundingBox,
        const Matrix3x4& worldTransform, ea::vector<RayQueryResult>& results);
    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Return the geometry for a specific LOD level.
    Geometry* GetLodGeometry(unsigned batchIndex, unsigned level) override;
    /// Return number of occlusion geometry triangles.
    unsigned GetNumOccluderTriangles() override;
    /// Draw to occlusion buffer. Return true if did not run out of triangles.
    bool DrawOcclusion(OcclusionBuffer* buffer) override;

    /// Set model.
    /// @manualbind
    virtual void SetModel(Model* model);
    /// Set material on all geometries.
    /// @property
    virtual void SetMaterial(Material* material);
    /// Set material on one geometry. Return true if successful.
    /// @property{set_materials}
    virtual bool SetMaterial(unsigned index, Material* material);
    /// Set occlusion LOD level. By default (M_MAX_UNSIGNED) same as visible.
    /// @property
    void SetOcclusionLodLevel(unsigned level);
    /// Apply default materials from a material list file. If filename is empty (default), the model's resource name with extension .txt will be used.
    void ApplyMaterialList(const ea::string& fileName = EMPTY_STRING);

    /// Return model.
    /// @property
    Model* GetModel() const { return model_; }

    /// Return number of geometries.
    /// @property
    unsigned GetNumGeometries() const { return geometries_.size(); }

    /// Return material from the first geometry, assuming all the geometries use the same material.
    /// @property
    virtual Material* GetMaterial() const { return GetMaterial(0); }
    /// Return material by geometry index.
    /// @property{get_materials}
    virtual Material* GetMaterial(unsigned index) const;

    /// Return occlusion LOD level.
    /// @property
    unsigned GetOcclusionLodLevel() const { return occlusionLodLevel_; }

    /// Determines if the given world space point is within the model geometry.
    bool IsInside(const Vector3& point) const;
    /// Determines if the given local space point is within the model geometry.
    bool IsInsideLocal(const Vector3& point) const;

    /// Set model attribute.
    void SetModelAttr(const ResourceRef& value);
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Return model attribute.
    ResourceRef GetModelAttr() const;
    /// Return materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;

    /// Set whether the lightmap is baked for this object.
    void SetBakeLightmap(bool bakeLightmap) { bakeLightmap_ = bakeLightmap; UpdateBatchesLightmaps(); }
    /// Return whether the lightmap is baked for this object.
    bool GetBakeLightmap() const { return bakeLightmap_; }
    /// Return whether the lightmap is baked for this object. Return false for objects with zero scale in lightmap.
    bool GetBakeLightmapEffective() const { return bakeLightmap_ && scaleInLightmap_ > 0.0f; }
    /// Set scale in lightmap.
    void SetScaleInLightmap(float scale) { scaleInLightmap_ = scale; }
    /// Return scale in lightmap.
    float GetScaleInLightmap() const { return scaleInLightmap_; }
    /// Set lightmap index.
    void SetLightmapIndex(unsigned idx) { lightmapIndex_ = idx; UpdateBatchesLightmaps(); }
    /// Return lightmap index.
    unsigned GetLightmapIndex() const { return lightmapIndex_; }
    /// Set lightmap scale and offset.
    void SetLightmapScaleOffset(const Vector4& scaleOffset) { lightmapScaleOffset_ = scaleOffset; UpdateBatchesLightmaps(); }
    /// Return lightmap scale and offset.
    const Vector4& GetLightmapScaleOffset() const { return lightmapScaleOffset_; }

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Set local-space bounding box.
    void SetBoundingBox(const BoundingBox& box);
    /// Set number of geometries.
    void SetNumGeometries(unsigned num);
    /// Reset LOD levels.
    void ResetLodLevels();
    /// Choose LOD levels based on distance.
    void CalculateLodLevels();
    /// Update lightmaps in batches.
    void UpdateBatchesLightmaps();

    /// Extra per-geometry data.
    ea::vector<StaticModelGeometryData> geometryData_;
    /// All geometries.
    ea::vector<ea::vector<SharedPtr<Geometry> > > geometries_;
    /// Model.
    SharedPtr<Model> model_;
    /// Occlusion LOD level.
    unsigned occlusionLodLevel_;
    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;

    /// Whether the lightmap is enabled.
    bool bakeLightmap_{};
    /// Texel density scale in lightmap.
    float scaleInLightmap_{ 1.0f };
    /// Lightmap index.
    unsigned lightmapIndex_{};
    /// Lightmap scale and offset.
    Vector4 lightmapScaleOffset_{ 1.0f, 1.0f, 0.0f, 0.0f };

private:
    /// Handle model reload finished.
    void HandleModelReloadFinished(StringHash eventType, VariantMap& eventData);
};

}
