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

#include "../Graphics/NativeModelView.h"

namespace Urho3D
{

/// Model face.
struct ModelFace
{
    /// Vertex indices.
    unsigned indices_[3]{};
};

/// Model vertex format.
struct ModelVertexFormat
{
    /// Undefined format.
    static const VertexElementType Undefined = MAX_VERTEX_ELEMENT_TYPES;
    /// Position format.
    VertexElementType position_{ Undefined };
    /// Normal format.
    VertexElementType normal_{ Undefined };
    /// Tangent format.
    VertexElementType tangent_{ Undefined };
    /// Binormal format.
    VertexElementType binormal_{ Undefined };
    /// Color formats.
    VertexElementType color_[ModelVertex::MaxColors]{ Undefined, Undefined, Undefined, Undefined };
    /// UV formats.
    VertexElementType uv_[ModelVertex::MaxUVs]{ Undefined, Undefined, Undefined, Undefined };
};

/// Model geometry LOD view.
struct URHO3D_API GeometryLODView
{
    /// Vertices.
    ea::vector<ModelVertex> vertices_;
    /// Faces.
    ea::vector<ModelFace> faces_;
    /// LOD distance.
    float lodDistance_{};
    /// Calculate center.
    Vector3 CalculateCenter() const;
};

/// Model geometry view.
struct GeometryView
{
    /// LODs.
    ea::vector<GeometryLODView> lods_;
};

/// Represents Model in editable form.
class URHO3D_API ModelView : public Object
{
    URHO3D_OBJECT(ModelView, Object);

public:
    /// Construct.
    ModelView(Context* context) : Object(context) {}

    /// Import from native view.
    bool ImportModel(const NativeModelView& nativeView);
    /// Export to existing native view.
    void ExportModel(NativeModelView& nativeView) const;
    /// Export to new native view.
    SharedPtr<NativeModelView> ExportModel() const;

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

private:
    /// Vertex format.
    ModelVertexFormat vertexFormat_;
    /// Geometries.
    ea::vector<GeometryView> geometries_;
    /// Metadata.
    ea::unordered_map<ea::string, Variant> metadata_;
};

}
