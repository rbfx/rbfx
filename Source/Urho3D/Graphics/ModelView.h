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
#include "../Graphics/Skeleton.h"
#include "../Math/BoundingBox.h"
#include "../Math/Color.h"

namespace Urho3D
{

class Model;

/// Model vertex format, unpacked for easy editing.
struct URHO3D_API ModelVertexFormat
{
    static const VertexElementType Undefined = MAX_VERTEX_ELEMENT_TYPES;
    static const unsigned MaxColors = 4;
    static const unsigned MaxUVs = 4;

    /// Vertex element formats
    /// @{
    VertexElementType position_{ Undefined };
    VertexElementType normal_{ Undefined };
    VertexElementType tangent_{ Undefined };
    VertexElementType binormal_{ Undefined };
    VertexElementType blendIndices_{ Undefined };
    VertexElementType blendWeights_{ Undefined };
    ea::array<VertexElementType, MaxColors> color_{ Undefined, Undefined, Undefined, Undefined };
    ea::array<VertexElementType, MaxUVs> uv_{ Undefined, Undefined, Undefined, Undefined };
    /// @}

    void MergeFrom(const ModelVertexFormat& rhs);
    unsigned ToHash() const;

    bool operator ==(const ModelVertexFormat& rhs) const;
    bool operator !=(const ModelVertexFormat& rhs) const { return !(*this == rhs); }
};

/// Model vertex, unpacked for easy editing.
/// Warning: ModelVertex must be equivalent to an array of Vector4.
struct URHO3D_API ModelVertex
{
    static const unsigned MaxBones = 4;
    static const unsigned MaxColors = ModelVertexFormat::MaxColors;
    static const unsigned MaxUVs = ModelVertexFormat::MaxUVs;
    /// Vertex elements corresponding to full ModelVertex.
    static const ea::vector<VertexElement> VertexElements;
    using BoneArray = ea::array<ea::pair<unsigned, float>, MaxBones>;

    /// Position.
    Vector4 position_;
    /// Normal. W-component must be zero.
    Vector4 normal_;
    /// Tangent. W-component is the sign of binormal direction.
    Vector4 tangent_;
    /// Binormal. W-component must be zero.
    Vector4 binormal_;
    /// Blend indices for skeletal animations. Must be integers.
    Vector4 blendIndices_;
    /// Blend weights for skeletal animations. Must be in range [0, 1].
    Vector4 blendWeights_;
    /// Colors.
    Vector4 color_[MaxColors];
    /// UV coordinates.
    Vector4 uv_[MaxUVs];

    /// Set position from 3-vector.
    void SetPosition(const Vector3& position) {position_ = position.ToVector4(1.0f); }
    /// Set normal from 3-vector.
    void SetNormal(const Vector3& normal) {normal_ = normal.ToVector4(); }
    /// Set color for given channel.
    void SetColor(unsigned i, const Color& color) { color_[i] = color.ToVector4(); }

    /// Return position as 3-vector.
    Vector3 GetPosition() const {return position_.ToVector3(); }
    /// Return normal as 3-vector.
    Vector3 GetNormal() const { return normal_.ToVector3(); }
    /// Return tangent as 3-vector.
    Vector3 GetTangent() const { return tangent_.ToVector3(); }
    /// Return color from given channel.
    Color GetColor(unsigned i = 0) const { return static_cast<Color>(color_[i]); }
    /// Return blend indices as integers.
    BoneArray GetBlendIndicesAndWeights() const;

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
    /// Prune vertex elements not represented in the format.
    void PruneElements(const ModelVertexFormat& format);

    bool operator ==(const ModelVertex& rhs) const;
    bool operator !=(const ModelVertex& rhs) const { return !(*this == rhs); }
};

/// Morph of ModelVertex.
struct URHO3D_API ModelVertexMorph
{
    unsigned index_{};
    Vector3 positionDelta_;
    Vector3 normalDelta_;
    Vector3 tangentDelta_;

    bool HasPosition() const { return positionDelta_ != Vector3::ZERO; }
    bool HasNormal() const { return normalDelta_ != Vector3::ZERO; }
    bool HasTangent() const { return tangentDelta_ != Vector3::ZERO; }
    bool IsEmpty() const { return !HasPosition() && !HasNormal() && !HasTangent(); }

    bool operator ==(const ModelVertexMorph& rhs) const;
    bool operator !=(const ModelVertexMorph& rhs) const { return !(*this == rhs); }
};

using ModelVertexMorphVector = ea::vector<ModelVertexMorph>;

URHO3D_API void NormalizeModelVertexMorphVector(ModelVertexMorphVector& morphVector);

/// Level of detail of Model geometry, unpacked for easy editing.
struct URHO3D_API GeometryLODView
{
    PrimitiveType primitiveType_{};
    ea::vector<ModelVertex> vertices_;
    ea::vector<unsigned> indices_;
    float lodDistance_{};
    ModelVertexFormat vertexFormat_;
    ea::unordered_map<unsigned, ModelVertexMorphVector> morphs_;

    /// Getters
    /// @{
    unsigned GetNumPrimitives() const;
    bool IsTriangleGeometry() const;
    bool IsLineGeometry() const;
    bool IsPointGeometry() const;
    /// @}

    /// Calculate center of vertices' bounding box.
    Vector3 CalculateCenter() const;
    /// Calculate number of morphs in the model.
    unsigned CalculateNumMorphs() const;
    /// All equivalent views should be literally equal after normalization.
    void Normalize();

    void InvalidateNormalsAndTangents();
    void RecalculateFlatNormals();
    void RecalculateSmoothNormals();
    void RecalculateTangents();

    /// Iterate all triangles in primitive. Callback is called with three vertex indices.
    template <class T>
    void ForEachTriangle(T callback)
    {
        if (!IsTriangleGeometry())
        {
            assert(0);
            return;
        }

        const unsigned numPrimitives = GetNumPrimitives();
        switch (primitiveType_)
        {
        case TRIANGLE_LIST:
            for (unsigned i = 0; i < numPrimitives; ++i)
                callback(indices_[i * 3], indices_[i * 3 + 1], indices_[i * 3 + 2]);
            break;

        case TRIANGLE_STRIP:
            for (unsigned i = 0; i < numPrimitives; ++i)
            {
                if (i % 2 == 0)
                    callback(indices_[i], indices_[i + 1], indices_[i + 2]);
                else
                    callback(indices_[i], indices_[i + 2], indices_[i + 1]);
            }
            break;

        case TRIANGLE_FAN:
            for (unsigned i = 0; i < numPrimitives; ++i)
                callback(indices_[0], indices_[i + 1], indices_[i + 2]);
            break;

        default:
            break;
        }
    }

    bool operator ==(const GeometryLODView& rhs) const;
    bool operator !=(const GeometryLODView& rhs) const { return !(*this == rhs); }
};

/// Model geometry, unpacked for easy editing.
struct URHO3D_API GeometryView
{
    /// LODs.
    ea::vector<GeometryLODView> lods_;
    /// Material resource name.
    ea::string material_;

    /// Calculate number of morphs in the model.
    unsigned CalculateNumMorphs() const;
    /// All equivalent views should be literally equal after normalization.
    void Normalize();

    bool operator ==(const GeometryView& rhs) const;
    bool operator !=(const GeometryView& rhs) const { return !(*this == rhs); }
};

/// Bone of Model skeleton, unpacked for easy editing.
struct URHO3D_API BoneView
{
    ea::string name_;
    /// Index of parent bone in the array. Should be undefined for exactly one root bone.
    unsigned parentIndex_{ M_MAX_UNSIGNED };

    /// Initial bone transform
    /// @{
    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_{ Vector3::ONE };
    /// Inverted value of bone transform corresponding to default vertex position.
    Matrix3x4 offsetMatrix_;
    /// @}

    /// Bone dimensions for bounding box evaluation
    /// @{
    BoneCollisionShapeFlags shapeFlags_;
    float boundingSphereRadius_{};
    BoundingBox localBoundingBox_;
    /// @}

    /// Set initial bone transform. Doesn't change offset matrix.
    void SetInitialTransform(const Vector3& position,
        const Quaternion& rotation = Quaternion::IDENTITY, const Vector3& scale = Vector3::ONE);
    /// Recalculate offset matrix from initial bone transform.
    void RecalculateOffsetMatrix();
    /// Reset bounding volume.
    void ResetBoundingVolume();
    /// Reset bounding volume to local bounding box.
    void SetLocalBoundingBox(const BoundingBox& boundingBox);
    /// Reset bounding volume to local bounding sphere.
    void SetLocalBoundingSphere(float radius);

    bool operator ==(const BoneView& rhs) const;
    bool operator !=(const BoneView& rhs) const { return !(*this == rhs); }
};

/// Represents metadata of model morph.
struct URHO3D_API ModelMorphView
{
    ea::string name_;
    float initialWeight_{};
};

/// Represents Model in editable form.
/// Some features are not supported for sake of API simplicity:
/// - Multiple vertex and index buffers;
/// - Vertex and index buffer reuse for different geometries and LODs;
/// - Multiple root bones for skinned models.
class URHO3D_API ModelView : public Object
{
    URHO3D_OBJECT(ModelView, Object);

public:
    ModelView(Context* context) : Object(context) {}
    void Clear();

    /// Import and export from/to native Model.
    /// @{
    bool ImportModel(const Model* model);
    void ExportModel(Model* model) const;
    SharedPtr<Model> ExportModel(const ea::string& name = EMPTY_STRING) const;
    ResourceRefList ExportMaterialList() const;
    /// @}

    /// Calculate bounding box.
    BoundingBox CalculateBoundingBox() const;
    /// All equivalent views should be literally equal after normalization.
    void Normalize();
    /// Mirror geometries along X axis. Useful for conversion between left-handed and right-handed systems.
    /// Note: Does not affect bones!
    void MirrorGeometriesX();
    /// Scale geometries. Useful for conversion between units.
    /// Note: Does not affect bones!
    void ScaleGeometries(float scale);
    /// Calculate normals for geometries without normals in vertex format. Resets tangents for affected geometries.
    void CalculateMissingNormals(bool flatNormals = false);
    /// Calculate tangents for geometries without tangents in vertex format.
    void CalculateMissingTangents();
    /// Normalize bone weights and cleanup invalid bones. Ignored if there's no bones.
    void RepairBoneWeights();
    /// Recalculate bounding boxes for bones.
    void RecalculateBoneBoundingBoxes();

    /// Set contents
    /// @{
    void SetName(const ea::string& name) { name_ = name; }
    void SetGeometries(ea::vector<GeometryView> geometries) { geometries_ = ea::move(geometries); }
    void SetBones(ea::vector<BoneView> bones) { bones_ = ea::move(bones); }
    void SetMorphs(ea::vector<ModelMorphView> morphs) { morphs_ = ea::move(morphs); }
    void SetMorph(unsigned index, const ModelMorphView& morph);
    void AddMetadata(const ea::string& key, const Variant& variant);
    /// @}

    /// Return contents
    /// @{
    const ea::string& GetName() const { return name_; }
    const ea::vector<GeometryView>& GetGeometries() const { return geometries_; }
    ea::vector<GeometryView>& GetGeometries() { return geometries_; }
    const ea::vector<BoneView>& GetBones() const { return bones_; }
    ea::vector<BoneView>& GetBones() { return bones_; }
    const ea::vector<ModelMorphView>& GetMorphs() const { return morphs_; }
    ea::vector<ModelMorphView>& GetMorphs() { return morphs_; }
    const Variant& GetMetadata(const ea::string& key) const;
    /// @}

private:
    ea::string name_;
    ea::vector<GeometryView> geometries_;
    ea::vector<BoneView> bones_;
    ea::vector<ModelMorphView> morphs_;
    StringVariantMap metadata_;
};

}
