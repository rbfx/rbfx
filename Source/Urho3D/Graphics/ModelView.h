//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Graphics/VertexBuffer.h"
#include "../Math/BoundingBox.h"
#include "../Math/Color.h"

namespace Urho3D
{

class Model;

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

    /// Set position from 3-vector.
    void SetPosition(const Vector3& position) { position_ = Vector4(position, 1.0f); }
    /// Set normal from 3-vector.
    void SetNormal(const Vector3& normal) { normal_ = Vector4(normal, 0.0f); }
    /// Set color for given channel.
    void SetColor(unsigned i, const Color& color) { color_[i] = color.ToVector4(); }

    /// Return position as 3-vector.
    Vector3 GetPosition() const { return static_cast<Vector3>(position_); }
    /// Return color from given channel.
    Color GetColor(unsigned i = 0) const { return static_cast<Color>(color_[i]); }

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

    bool operator ==(const ModelVertex& rhs) const;
    bool operator !=(const ModelVertex& rhs) const { return !(*this == rhs); }
};

/// Model vertex format.
struct URHO3D_API ModelVertexFormat
{
    /// Undefined format used to disable corresponding component.
    static const VertexElementType Undefined = MAX_VERTEX_ELEMENT_TYPES;

    /// Vertex element formats
    /// @{
    VertexElementType position_{ Undefined };
    VertexElementType normal_{ Undefined };
    VertexElementType tangent_{ Undefined };
    VertexElementType binormal_{ Undefined };
    ea::array<VertexElementType, ModelVertex::MaxColors> color_{ Undefined, Undefined, Undefined, Undefined };
    ea::array<VertexElementType, ModelVertex::MaxUVs> uv_{ Undefined, Undefined, Undefined, Undefined };
    /// @}

    bool operator ==(const ModelVertexFormat& rhs) const;
    bool operator !=(const ModelVertexFormat& rhs) const { return !(*this == rhs); }
};

/// Model geometry LOD view.
struct URHO3D_API GeometryLODView
{
    /// Vertices.
    ea::vector<ModelVertex> vertices_;
    /// Faces: 3 indices per triangle.
    ea::vector<unsigned> indices_;
    /// LOD distance.
    float lodDistance_{};

    /// Calculate center of vertices' bounding box.
    Vector3 CalculateCenter() const;

    bool operator ==(const GeometryLODView& rhs) const;
    bool operator !=(const GeometryLODView& rhs) const { return !(*this == rhs); }
};

/// Model geometry view.
struct URHO3D_API GeometryView
{
    /// LODs.
    ea::vector<GeometryLODView> lods_;

    bool operator ==(const GeometryView& rhs) const;
    bool operator !=(const GeometryView& rhs) const { return !(*this == rhs); }
};

/// Represents Model in editable form.
class URHO3D_API ModelView : public Object
{
    URHO3D_OBJECT(ModelView, Object);

public:
    /// Construct.
    ModelView(Context* context) : Object(context) {}

    /// Import from native view.
    bool ImportModel(const Model* model);
    /// Export to existing native view.
    void ExportModel(Model* model) const;
    /// Export to new native view.
    SharedPtr<Model> ExportModel() const;

    /// Calculate bounding box.
    BoundingBox CalculateBoundingBox() const;

    /// Set vertex format.
    void SetVertexFormat(const ModelVertexFormat& vertexFormat) { vertexFormat_ = vertexFormat; }
    /// Set geometries.
    void SetGeometries(ea::vector<GeometryView> geometries) { geometries_ = ea::move(geometries); }
    /// Add metadata.
    void AddMetadata(const ea::string& key, const Variant& variant) { metadata_.insert_or_assign(key, variant); }
    /// Return vertex format.
    const ModelVertexFormat& GetVertexFormat() const { return vertexFormat_; }
    /// Return geometries.
    const ea::vector<GeometryView>& GetGeometries() const { return geometries_; }
    /// Return metadata.
    const Variant& GetMetadata(const ea::string& key) const;
    /// Return mutable geometries.
    ea::vector<GeometryView>& GetGeometries() { return geometries_; }

private:
    /// Vertex format.
    ModelVertexFormat vertexFormat_;
    /// Geometries.
    ea::vector<GeometryView> geometries_;
    /// Metadata.
    StringVariantMap metadata_;
};

}
