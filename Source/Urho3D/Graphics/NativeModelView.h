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

#include "../Core/Context.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/Model.h"

#include <EASTL/algorithm.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

/// Model vertex.
struct URHO3D_API ModelVertex
{
    /// Max number of colors.
    static const unsigned MaxColors = 4;
    /// Max number of UV.
    static const unsigned MaxUVs = 4;
    /// Vertex elements.
    static const ea::vector<VertexElement> VertexElements;

    /// Position.
    Vector4 position_;
    /// Normal. W-component must be zero.
    Vector4 normal_;
    /// Tangent. W-component is the sign of binormal direction.
    Vector4 tangent_;
    /// Binormal. W-component must be zero.
    Vector4 binormal_;
    /// Colors.
    Vector4 color_[MaxColors];
    /// UV coordinates.
    Vector4 uv_[MaxUVs];

    /// Return whether the vertex has normal.
    bool HasNormal() const { return normal_ != Vector4::ZERO; }
    /// Return whether the vertex has tangent.
    bool HasTangent() const { return tangent_ != Vector4::ZERO; }
    /// Return whether the vertex has binormal.
    bool HasBinormal() const { return binormal_ != Vector4::ZERO; }
    /// Return whether the vertex has tangent and binormal combined.
    bool HasTangentBinormalCombined() const { return tangent_ != Vector4::ZERO && tangent_.w_ != 0; }

    /// Replace given semantics from another vector.
    bool ReplaceElement(const ModelVertex& source, const VertexElement& element);
    /// Repair missing vertex elements if possible.
    void Repair();
};

/// Represents Model in editable form preserving internal structure.
class URHO3D_API NativeModelView : public Object
{
    URHO3D_OBJECT(NativeModelView, Object);

public:
    /// Vertex buffer data.
    struct VertexBufferData
    {
        /// Unpacked vertices.
        ea::vector<ModelVertex> vertices_;
        /// Vertex elements.
        ea::vector<VertexElement> elements_;
    };

    /// Index buffer data.
    struct IndexBufferData
    {
        /// Unpacked indices.
        ea::vector<unsigned> indices_;
        /// Check whether the index is large. 0xffff is reserved for triangle strip reset.
        static bool IsLargeIndex(unsigned index) { return index >= 0xfffe; }
        /// Check whether the index buffer has large indices.
        bool HasLargeIndices() const { return ea::any_of(indices_.begin(), indices_.end(), IsLargeIndex); }
    };

    /// Geometry LOD data.
    struct GeometryLODData
    {
        /// Vertex buffers.
        ea::vector<unsigned> vertexBuffers_;
        /// Index buffer.
        unsigned indexBuffer_{ M_MAX_UNSIGNED };
        /// Start index.
        unsigned indexStart_{};
        /// Number of indices.
        unsigned indexCount_{};
        /// First used vertex.
        unsigned vertexStart_{};
        /// Number of used vertices.
        unsigned vertexCount_{};
        /// LOD distance.
        float lodDistance_{};
    };

    /// Geometry data.
    struct GeometryData
    {
        /// LODs.
        ea::vector<GeometryLODData> lods_;
        /// Center.
        Vector3 center_{};
    };

public:
    /// Construct.
    NativeModelView(Context* context) : Object(context) {}
    /// Initialize content.
    void Initialize(const BoundingBox& boundingBox,
        const ea::vector<VertexBufferData>& vertexBuffers, const ea::vector<IndexBufferData>& indexBuffers,
        const ea::vector<GeometryData>& geometries)
    {
        boundingBox_ = boundingBox;
        vertexBuffers_ = vertexBuffers;
        indexBuffers_ = indexBuffers;
        geometries_ = geometries;
    }
    /// Set metadata.
    void SetMetadata(const ea::unordered_map<ea::string, Variant>& metadata) { metadata_ = metadata; }

    /// Import from resource.
    bool ImportModel(Model* model);
    /// Export to existing resource.
    void ExportModel(Model* model);
    /// Export to resource.
    SharedPtr<Model> ExportModel(ea::string name = EMPTY_STRING);

    /// Return effective vertex elements set.
    const ea::vector<VertexElement> GetEffectiveVertexElements() const;
    /// Return vertex buffers.
    const ea::vector<VertexBufferData>& GetVertexBuffers() const { return vertexBuffers_; }
    /// Return index buffers.
    const ea::vector<IndexBufferData>& GetIndexBuffers() const { return indexBuffers_; }
    /// Return geometries.
    const ea::vector<GeometryData>& GetGeometries() const { return geometries_; }
    /// Return metadata.
    const ea::unordered_map<ea::string, Variant>& GetMetadata() const { return metadata_; }

private:
    /// Bounding box.
    BoundingBox boundingBox_;
    /// Vertex buffers.
    ea::vector<VertexBufferData> vertexBuffers_;
    /// Index buffers.
    ea::vector<IndexBufferData> indexBuffers_;
    /// Geometries.
    ea::vector<GeometryData> geometries_;
    /// Metadata.
    ea::unordered_map<ea::string, Variant> metadata_;
};

}
