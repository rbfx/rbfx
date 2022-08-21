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

#include <EASTL/shared_array.h>

#include "../Container/Ptr.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/Skeleton.h"
#include "../Math/BoundingBox.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

class Geometry;
class IndexBuffer;
class Graphics;
class VertexBuffer;

/// Vertex buffer morph data.
struct VertexBufferMorph
{
    /// Vertex elements.
    VertexMaskFlags elementMask_;
    /// Number of vertices.
    unsigned vertexCount_{};
    /// Morphed vertices data size as bytes.
    unsigned dataSize_{};
    /// Morphed vertices. Stored packed as <index, data> pairs.
    ea::shared_array<unsigned char> morphData_;
};

/// Definition of a model's vertex morph.
struct ModelMorph
{
    /// Morph name.
    ea::string name_;
    /// Morph name hash.
    StringHash nameHash_;
    /// Current morph weight.
    float weight_{};
    /// Morph data per vertex buffer.
    ea::unordered_map<unsigned, VertexBufferMorph> buffers_;
};

/// Description of vertex buffer data for asynchronous loading.
struct VertexBufferDesc
{
    /// Vertex count.
    unsigned vertexCount_;
    /// Vertex declaration.
    ea::vector<VertexElement> vertexElements_;
    /// Vertex data size.
    unsigned dataSize_;
    /// Vertex data.
    ea::shared_array<unsigned char> data_;
};

/// Description of index buffer data for asynchronous loading.
struct IndexBufferDesc
{
    /// Index count.
    unsigned indexCount_;
    /// Index size.
    unsigned indexSize_;
    /// Index data size.
    unsigned dataSize_;
    /// Index data.
    ea::shared_array<unsigned char> data_;
};

/// Description of a geometry for asynchronous loading.
struct GeometryDesc
{
    /// Primitive type.
    PrimitiveType type_;
    /// Vertex buffer ref.
    unsigned vbRef_;
    /// Index buffer ref.
    unsigned ibRef_;
    /// Index start.
    unsigned indexStart_;
    /// Index count.
    unsigned indexCount_;
};

/// 3D model resource.
class URHO3D_API Model : public ResourceWithMetadata
{
    URHO3D_OBJECT(Model, ResourceWithMetadata);

public:
    /// Construct.
    explicit Model(Context* context);
    /// Destruct.
    ~Model() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;

    /// Set local-space bounding box.
    /// @property
    void SetBoundingBox(const BoundingBox& box);
    /// Set vertex buffers and their morph ranges.
    bool SetVertexBuffers(const ea::vector<SharedPtr<VertexBuffer> >& buffers, const ea::vector<unsigned>& morphRangeStarts,
        const ea::vector<unsigned>& morphRangeCounts);
    /// Set index buffers.
    bool SetIndexBuffers(const ea::vector<SharedPtr<IndexBuffer> >& buffers);
    /// Set number of geometries.
    /// @property
    void SetNumGeometries(unsigned num);
    /// Set number of LOD levels in a geometry.
    /// @property
    bool SetNumGeometryLodLevels(unsigned index, unsigned num);
    /// Set geometry.
    bool SetGeometry(unsigned index, unsigned lodLevel, Geometry* geometry);
    /// Set geometry center.
    /// @property{set_geometryCenters}
    bool SetGeometryCenter(unsigned index, const Vector3& center);
    /// Set skeleton.
    void SetSkeleton(const Skeleton& skeleton);
    /// Set bone mappings when model has more bones than the skinning shader can handle.
    void SetGeometryBoneMappings(const ea::vector<ea::vector<unsigned> >& geometryBoneMappings);
    /// Set vertex morphs.
    void SetMorphs(const ea::vector<ModelMorph>& morphs);
    /// Clone the model. The geometry data is deep-copied and can be modified in the clone without affecting the original.
    SharedPtr<Model> Clone(const ea::string& cloneName = EMPTY_STRING) const;

    /// Return bounding box.
    /// @property
    const BoundingBox& GetBoundingBox() const { return boundingBox_; }

    /// Return skeleton.
    /// @property
    Skeleton& GetSkeleton() { return skeleton_; }

    /// Return skeleton.
    const Skeleton& GetSkeleton() const { return skeleton_; }

    /// Return vertex buffers.
    const ea::vector<SharedPtr<VertexBuffer> >& GetVertexBuffers() const { return vertexBuffers_; }

    /// Return index buffers.
    const ea::vector<SharedPtr<IndexBuffer> >& GetIndexBuffers() const { return indexBuffers_; }

    /// Return number of geometries.
    /// @property
    unsigned GetNumGeometries() const { return geometries_.size(); }

    /// Return number of LOD levels in geometry.
    /// @property
    unsigned GetNumGeometryLodLevels(unsigned index) const;

    /// Return geometry pointers.
    const ea::vector<ea::vector<SharedPtr<Geometry> > >& GetGeometries() const { return geometries_; }

    /// Return geometry center points.
    const ea::vector<Vector3>& GetGeometryCenters() const { return geometryCenters_; }

    /// Return geometry by index and LOD level. The LOD level is clamped if out of range.
    Geometry* GetGeometry(unsigned index, unsigned lodLevel) const;

    /// Return geometry center by index.
    /// @property{get_geometryCenters}
    const Vector3& GetGeometryCenter(unsigned index) const
    {
        return index < geometryCenters_.size() ? geometryCenters_[index] : Vector3::ZERO;
    }

    /// Return geometery bone mappings.
    const ea::vector<ea::vector<unsigned> >& GetGeometryBoneMappings() const { return geometryBoneMappings_; }

    /// Return vertex morphs.
    const ea::vector<ModelMorph>& GetMorphs() const { return morphs_; }

    /// Return number of vertex morphs.
    /// @property
    unsigned GetNumMorphs() const { return morphs_.size(); }

    /// Return vertex morph by index.
    const ModelMorph* GetMorph(unsigned index) const;
    /// Return vertex morph by name.
    const ModelMorph* GetMorph(const ea::string& name) const;
    /// Return vertex morph by name hash.
    const ModelMorph* GetMorph(StringHash nameHash) const;
    /// Return vertex buffer morph range start.
    unsigned GetMorphRangeStart(unsigned bufferIndex) const;
    /// Return vertex buffer morph range vertex count.
    unsigned GetMorphRangeCount(unsigned bufferIndex) const;

    /// Return morph range starts for each vertex buffer.
    const ea::vector<unsigned>& GetMorphRangeStarts() const { return morphRangeStarts_; }

    /// Return morph range vertex counts for each vertex buffer.
    const ea::vector<unsigned>& GetMorphRangeCounts() const { return morphRangeCounts_; }

private:
    /// Class versions (used for serialization)
    /// @{
    static const unsigned legacyVersion = 1; // Fake version for legacy unversioned UMDL/UMD2 file
    static const unsigned morphWeightVersion = 2; // Initial morph weights support added here

    static const unsigned currentVersion = morphWeightVersion;
    /// @}

    /// Bounding box.
    BoundingBox boundingBox_;
    /// Skeleton.
    Skeleton skeleton_;
    /// Vertex buffers.
    ea::vector<SharedPtr<VertexBuffer> > vertexBuffers_;
    /// Index buffers.
    ea::vector<SharedPtr<IndexBuffer> > indexBuffers_;
    /// Geometries.
    ea::vector<ea::vector<SharedPtr<Geometry> > > geometries_;
    /// Geometry bone mappings.
    ea::vector<ea::vector<unsigned> > geometryBoneMappings_;
    /// Geometry centers.
    ea::vector<Vector3> geometryCenters_;
    /// Vertex morphs.
    ea::vector<ModelMorph> morphs_;
    /// Vertex buffer morph range start.
    ea::vector<unsigned> morphRangeStarts_;
    /// Vertex buffer morph range vertex count.
    ea::vector<unsigned> morphRangeCounts_;
    /// Vertex buffer data for asynchronous loading.
    ea::vector<VertexBufferDesc> loadVBData_;
    /// Index buffer data for asynchronous loading.
    ea::vector<IndexBufferDesc> loadIBData_;
    /// Geometry definitions for asynchronous loading.
    ea::vector<ea::vector<GeometryDesc> > loadGeometries_;
};

}
