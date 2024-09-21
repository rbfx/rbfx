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

#include "../Precompiled.h"

#include "../Graphics/ModelView.h"

#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/Tangent.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/Material.h"
#include "../IO/Log.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

/// Compare two vertex elements by semantic and index.
static bool CompareVertexElementSemantics(const VertexElement& lhs, const VertexElement& rhs)
{
    return lhs.semantic_ == rhs.semantic_ && lhs.index_ == rhs.index_;
}

/// Read vertex buffer data.
ea::vector<ModelVertex> GetVertexBufferData(const VertexBuffer* vertexBuffer, unsigned start, unsigned count)
{
    const ea::vector<Vector4> unpackedData = vertexBuffer->GetUnpackedData(start, count);

    ea::vector<ModelVertex> result(count);
    VertexBuffer::ShuffleUnpackedVertexData(count, unpackedData.data(), vertexBuffer->GetElements(),
        reinterpret_cast<Vector4*>(result.data()), ModelVertex::VertexElements);

    return result;
}

/// Write vertex buffer data.
void SetVertexBufferData(VertexBuffer* vertexBuffer, const ea::vector<ModelVertex>& data)
{
    const auto vertexCount = ea::min<unsigned>(vertexBuffer->GetVertexCount(), data.size());
    const ea::vector<VertexElement>& vertexElements = vertexBuffer->GetElements();

    ea::vector<Vector4> buffer(vertexElements.size() * vertexCount);
    VertexBuffer::ShuffleUnpackedVertexData(vertexCount,
        reinterpret_cast<const Vector4*>(data.data()), ModelVertex::VertexElements, buffer.data(), vertexElements);

    vertexBuffer->SetUnpackedData(buffer.data(), 0, vertexCount);
}

/// Parse vertex elements into simpler format.
ModelVertexFormat ParseVertexElements(const ea::vector<VertexElement>& elements)
{
    ModelVertexFormat result;
    for (const VertexElement& element : elements)
    {
        switch (element.semantic_)
        {
        case SEM_POSITION:
            result.position_ = element.type_;
            break;
        case SEM_NORMAL:
            result.normal_ = element.type_;
            break;
        case SEM_BINORMAL:
            result.binormal_ = element.type_;
            break;
        case SEM_TANGENT:
            result.tangent_ = element.type_;
            break;
        case SEM_TEXCOORD:
            if (element.index_ < ModelVertex::MaxUVs)
                result.uv_[element.index_] = element.type_;
            break;
        case SEM_COLOR:
            if (element.index_ < ModelVertex::MaxColors)
                result.color_[element.index_] = element.type_;
            break;

        case SEM_BLENDWEIGHTS:
            result.blendWeights_ = element.type_;
            break;

        case SEM_BLENDINDICES:
            result.blendIndices_ = element.type_;
            break;

        case SEM_OBJECTINDEX:
        default:
            assert(0);
            break;
        }
    }
    return result;
}

/// Convert model vertex format to array of vertex elements.
ea::vector<VertexElement> CollectVertexElements(const ModelVertexFormat& vertexFormat)
{
    ea::vector<VertexElement> elements;

    if (vertexFormat.position_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.position_, SEM_POSITION, 0 });

    if (vertexFormat.normal_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.normal_, SEM_NORMAL, 0 });

    if (vertexFormat.binormal_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.binormal_, SEM_BINORMAL, 0 });

    if (vertexFormat.tangent_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.tangent_, SEM_TANGENT, 0 });

    if (vertexFormat.blendWeights_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.blendWeights_, SEM_BLENDWEIGHTS, 0 });

    if (vertexFormat.blendIndices_ != ModelVertexFormat::Undefined)
        elements.push_back({ vertexFormat.blendIndices_, SEM_BLENDINDICES, 0 });

    for (unsigned i = 0; i < ModelVertex::MaxUVs; ++i)
    {
        if (vertexFormat.uv_[i] != ModelVertexFormat::Undefined)
            elements.push_back({ vertexFormat.uv_[i], SEM_TEXCOORD, static_cast<unsigned char>(i) });
    }

    for (unsigned i = 0; i < ModelVertex::MaxColors; ++i)
    {
        if (vertexFormat.color_[i] != ModelVertexFormat::Undefined)
            elements.push_back({ vertexFormat.color_[i], SEM_COLOR, static_cast<unsigned char>(i) });
    }

    VertexBuffer::UpdateOffsets(elements);

    return elements;
}

/// Check whether the index is large. 0xffff is reserved for triangle strip reset.
bool IsLargeIndex(unsigned index)
{
    return index >= 0xffff;
}

/// Check whether the index buffer has large indices.
bool HasLargeIndices(const ea::vector<unsigned>& indices)
{
    return ea::any_of(indices.begin(), indices.end(), IsLargeIndex);
}

/// Check if vertex elements can be imported into ModelVertex.
bool CheckVertexElements(const ea::vector<VertexElement>& elements)
{
    for (const VertexElement& element : elements)
    {
        if (element.semantic_ == SEM_OBJECTINDEX)
        {
            return false;
        }

        if (element.semantic_ == SEM_COLOR)
        {
            if (element.index_ >= ModelVertex::MaxColors)
            {
                return false;
            }
        }
        else if (element.semantic_ == SEM_TEXCOORD)
        {
            if (element.index_ >= ModelVertex::MaxUVs)
            {
                return false;
            }
        }
        else
        {
            if (element.index_ > 0)
            {
                return false;
            }
        }
    }
    return true;
}

bool CheckIndexCount(PrimitiveType primitiveType, unsigned count)
{
    switch (primitiveType)
    {
    case TRIANGLE_LIST:
        return count % 3 == 0;
    case LINE_LIST:
        return count % 2 == 0;
    case POINT_LIST:
        return true;
    case TRIANGLE_STRIP:
    case TRIANGLE_FAN:
        return count == 0 || count >= 3;
    case LINE_STRIP:
        return count == 0 || count >= 2;
    default:
        return false;
    }
}

VertexBufferMorph CreateVertexBufferMorph(ModelVertexMorphVector morphVector)
{
    NormalizeModelVertexMorphVector(morphVector);

    const bool hasPosition = ea::any_of(morphVector.begin(), morphVector.end(),
        ea::mem_fn(&ModelVertexMorph::HasPosition));
    const bool hasNormal = ea::any_of(morphVector.begin(), morphVector.end(),
        ea::mem_fn(&ModelVertexMorph::HasNormal));
    const bool hasTangent = ea::any_of(morphVector.begin(), morphVector.end(),
        ea::mem_fn(&ModelVertexMorph::HasTangent));

    VertexBufferMorph result;
    if (!hasPosition && !hasNormal && !hasTangent)
        return result;

    unsigned stride = sizeof(unsigned);
    if (hasPosition)
    {
        result.elementMask_ |= MASK_POSITION;
        stride += sizeof(Vector3);
    }
    if (hasNormal)
    {
        result.elementMask_ |= MASK_NORMAL;
        stride += sizeof(Vector3);
    }
    if (hasTangent)
    {
        result.elementMask_ |= MASK_TANGENT;
        stride += sizeof(Vector3);
    }

    result.vertexCount_ = morphVector.size();
    result.dataSize_ = result.vertexCount_ * stride;
    result.morphData_ = new unsigned char[result.dataSize_];

    for (unsigned i = 0; i < result.vertexCount_; ++i)
    {
        const ModelVertexMorph& src = morphVector[i];
        unsigned char* dest = &result.morphData_[i * stride];

        std::memcpy(dest, &src.index_, sizeof(src.index_));
        dest += sizeof(src.index_);

        if (hasPosition)
        {
            std::memcpy(dest, &src.positionDelta_, sizeof(src.positionDelta_));
            dest += sizeof(src.positionDelta_);
        }

        if (hasNormal)
        {
            std::memcpy(dest, &src.normalDelta_, sizeof(src.normalDelta_));
            dest += sizeof(src.normalDelta_);
        }

        if (hasTangent)
        {
            std::memcpy(dest, &src.tangentDelta_, sizeof(src.tangentDelta_));
            dest += sizeof(src.tangentDelta_);
        }
    }

    return result;
}

ModelVertexMorph ReadVertexMorph(const VertexBufferMorph& vertexBufferMorph, unsigned i)
{
    unsigned stride = sizeof(unsigned);
    if (vertexBufferMorph.elementMask_ & MASK_POSITION)
        stride += sizeof(Vector3);
    if (vertexBufferMorph.elementMask_ & MASK_NORMAL)
        stride += sizeof(Vector3);
    if (vertexBufferMorph.elementMask_ & MASK_TANGENT)
        stride += sizeof(Vector3);

    const unsigned char* ptr = &vertexBufferMorph.morphData_[i * stride];

    ModelVertexMorph vertexMorph;

    memcpy(&vertexMorph.index_, ptr, sizeof(unsigned));
    ptr += sizeof(unsigned);

    if (vertexBufferMorph.elementMask_ & MASK_POSITION)
    {
        memcpy(&vertexMorph.positionDelta_, ptr, sizeof(Vector3));
        ptr += sizeof(Vector3);
    }

    if (vertexBufferMorph.elementMask_ & MASK_NORMAL)
    {
        memcpy(&vertexMorph.normalDelta_, ptr, sizeof(Vector3));
        ptr += sizeof(Vector3);
    }

    if (vertexBufferMorph.elementMask_ & MASK_TANGENT)
    {
        memcpy(&vertexMorph.tangentDelta_, ptr, sizeof(Vector3));
        ptr += sizeof(Vector3);
    }

    return vertexMorph;
}

}

static_assert(ModelVertex::MaxColors == 4 && ModelVertex::MaxUVs == 4, "Update ModelVertex::VertexElements!");

const ea::vector<VertexElement> ModelVertex::VertexElements =
{
    VertexElement{ TYPE_VECTOR4, SEM_POSITION },
    VertexElement{ TYPE_VECTOR4, SEM_NORMAL },
    VertexElement{ TYPE_VECTOR4, SEM_TANGENT },
    VertexElement{ TYPE_VECTOR4, SEM_BINORMAL },
    VertexElement{ TYPE_VECTOR4, SEM_BLENDINDICES },
    VertexElement{ TYPE_VECTOR4, SEM_BLENDWEIGHTS },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 0 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 1 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 2 },
    VertexElement{ TYPE_VECTOR4, SEM_COLOR, 3 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 0 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 1 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 2 },
    VertexElement{ TYPE_VECTOR4, SEM_TEXCOORD, 3 },
};

bool ModelVertex::operator ==(const ModelVertex& rhs) const
{
    for (unsigned i = 0; i < MaxColors; ++i)
    {
        if (!color_[i].Equals(rhs.color_[i]))
            return false;
    }

    for (unsigned i = 0; i < MaxUVs; ++i)
    {
        if (!uv_[i].Equals(rhs.uv_[i]))
            return false;
    }

    return position_.Equals(rhs.position_)
        && normal_.Equals(rhs.normal_)
        && tangent_.Equals(rhs.tangent_)
        && blendIndices_.Equals(rhs.blendIndices_)
        && blendWeights_.Equals(rhs.blendWeights_)
        && binormal_.Equals(rhs.binormal_);
}

bool ModelVertexMorph::operator ==(const ModelVertexMorph& rhs) const
{
    return positionDelta_.Equals(rhs.positionDelta_)
        && normalDelta_.Equals(rhs.normalDelta_)
        && tangentDelta_.Equals(rhs.tangentDelta_);
}

void NormalizeModelVertexMorphVector(ModelVertexMorphVector& morphVector)
{
    static const auto isLess = [](const ModelVertexMorph& lhs, const ModelVertexMorph& rhs) { return lhs.index_ < rhs.index_; };
    static const auto isEqual = [](const ModelVertexMorph& lhs, const ModelVertexMorph& rhs)  { return lhs.index_ == rhs.index_; };
    static const auto isEmpty = ea::mem_fn(&ModelVertexMorph::IsEmpty);

    // Remove duplicate indices
    ea::sort(morphVector.begin(), morphVector.end(), isLess);
    morphVector.erase(ea::unique(morphVector.begin(), morphVector.end(), isEqual), morphVector.end());

    // Remove empty elements
    ea::erase_if(morphVector, isEmpty);
}

ea::vector<VertexElement> ModelVertexFormat::ToVertexElements() const
{
    return CollectVertexElements(*this);
}

void ModelVertexFormat::MergeFrom(const ModelVertexFormat& rhs)
{
    static const auto mergeFrom = [](VertexElementType& lhs, const VertexElementType rhs)
    {
        if (rhs != Undefined)
            lhs = rhs;
    };

    mergeFrom(position_, rhs.position_);
    mergeFrom(normal_, rhs.normal_);
    mergeFrom(tangent_, rhs.tangent_);
    mergeFrom(binormal_, rhs.binormal_);
    mergeFrom(blendIndices_, rhs.blendIndices_);
    mergeFrom(blendWeights_, rhs.blendWeights_);
    for (unsigned i = 0; i < MaxColors; ++i)
        mergeFrom(color_[i], rhs.color_[i]);
    for (unsigned i = 0; i < MaxUVs; ++i)
        mergeFrom(uv_[i], rhs.uv_[i]);
}

unsigned ModelVertexFormat::ToHash() const
{
    unsigned hash = 0;
    CombineHash(hash, MakeHash(position_));
    CombineHash(hash, MakeHash(normal_));
    CombineHash(hash, MakeHash(tangent_));
    CombineHash(hash, MakeHash(binormal_));
    CombineHash(hash, MakeHash(blendIndices_));
    CombineHash(hash, MakeHash(blendWeights_));
    for (unsigned i = 0; i < MaxColors; ++i)
        CombineHash(hash, MakeHash(color_[i]));
    for (unsigned i = 0; i < MaxUVs; ++i)
        CombineHash(hash, MakeHash(uv_[i]));
    return hash;
}

bool ModelVertexFormat::operator ==(const ModelVertexFormat& rhs) const
{
    return position_ == rhs.position_
        && normal_ == rhs.normal_
        && tangent_ == rhs.tangent_
        && binormal_ == rhs.binormal_
        && blendIndices_ == rhs.blendIndices_
        && blendWeights_ == rhs.blendWeights_
        && color_ == rhs.color_
        && uv_ == rhs.uv_;
}

bool GeometryLODView::operator ==(const GeometryLODView& rhs) const
{
    return vertices_ == rhs.vertices_
        && indices_ == rhs.indices_
        && lodDistance_ == rhs.lodDistance_
        && vertexFormat_ == rhs.vertexFormat_
        && morphs_ == rhs.morphs_;
}

bool GeometryView::operator ==(const GeometryView& rhs) const
{
    return lods_ == rhs.lods_
        && material_ == rhs.material_;
}

void BoneView::SetInitialTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    position_ = position;
    rotation_ = rotation;
    scale_ = scale;
}

void BoneView::RecalculateOffsetMatrix()
{
    offsetMatrix_ = Matrix3x4(position_, rotation_, scale_).Inverse();
}

void BoneView::ResetBoundingVolume()
{
    shapeFlags_ = BONECOLLISION_NONE;
}

void BoneView::SetLocalBoundingBox(const BoundingBox& boundingBox)
{
    shapeFlags_ = BONECOLLISION_BOX;
    localBoundingBox_ = boundingBox;
}

void BoneView::SetLocalBoundingSphere(float radius)
{
    shapeFlags_ = BONECOLLISION_SPHERE;
    boundingSphereRadius_ = radius;
}

bool BoneView::operator ==(const BoneView& rhs) const
{
    return name_ == rhs.name_
        && parentIndex_ == rhs.parentIndex_

        && position_.Equals(rhs.position_)
        && rotation_.Equals(rhs.rotation_)
        && scale_.Equals(rhs.scale_)
        && offsetMatrix_.Equals(rhs.offsetMatrix_)

        && shapeFlags_ == rhs.shapeFlags_
        && Equals(boundingSphereRadius_, rhs.boundingSphereRadius_)
        && localBoundingBox_.min_.Equals(rhs.localBoundingBox_.min_)
        && localBoundingBox_.max_.Equals(rhs.localBoundingBox_.max_);
}

ModelVertex::BoneArray ModelVertex::GetBlendIndicesAndWeights() const
{
    BoneArray result;
    for (unsigned i = 0; i < MaxBones; ++i)
    {
        const float index = blendIndices_.Data()[i];
        const float weight = blendWeights_.Data()[i];
        if (index >= 0 && index <= 16777216)
            result[i] = { static_cast<unsigned>(index), weight };
        else
            result[i] = { 0, 0.0f };
    }
    return result;
}

bool ModelVertex::ReplaceElement(const ModelVertex& source, const VertexElement& element)
{
    switch (element.semantic_)
    {
    case SEM_POSITION:
        position_ = source.position_;
        return true;

    case SEM_NORMAL:
        normal_ = source.normal_;
        return true;

    case SEM_BINORMAL:
        binormal_ = source.binormal_;
        return true;

    case SEM_TANGENT:
        tangent_ = source.tangent_;
        return true;

    case SEM_TEXCOORD:
        if (element.index_ >= MaxUVs)
            return false;
        uv_[element.index_] = source.uv_[element.index_];
        return true;

    case SEM_COLOR:
        if (element.index_ >= MaxColors)
            return false;
        color_[element.index_] = source.color_[element.index_];
        return true;

    case SEM_BLENDWEIGHTS:
        blendWeights_ = source.blendWeights_;
        return true;

    case SEM_BLENDINDICES:
        blendIndices_ = source.blendIndices_;
        return true;

    case SEM_OBJECTINDEX:
    default:
        assert(0);
        return false;
    }
}

void ModelVertex::Repair()
{
    normal_.w_ = 0;
    binormal_.w_ = 0;

    if (HasNormal())
    {
        if (HasTangent())
        {
            const bool hasBinormal = HasBinormal();
            const bool hasTangentBinormalCombined = HasTangentBinormalCombined();

            if (hasTangentBinormalCombined && !hasBinormal)
            {
                // Repair binormal from tangent and normal
                const Vector3 normal3 = tangent_.ToVector3();
                const Vector3 tangent3 = normal_.ToVector3();
                const Vector3 binormal3 = tangent_.w_ * normal3.CrossProduct(tangent3);
                binormal_ = binormal3.Normalized().ToVector4();
            }
            else if (hasBinormal && !hasTangentBinormalCombined)
            {
                // Repair tangent W component from binormal, tangent and normal
                const Vector3 normal3 = tangent_.ToVector3();
                const Vector3 tangent3 = normal_.ToVector3();
                const Vector3 binormal3 = binormal_.ToVector3();
                const Vector3 crossBinormal = normal3.CrossProduct(tangent3);
                tangent_.w_ = crossBinormal.DotProduct(binormal3) >= 0 ? 1.0f : -1.0f;
            }
        }
        else
        {
            // Reset binormal if tangent is missing
            binormal_ = Vector4::ZERO;
        }
    }
    else
    {
        // Reset tangent and binormal if normal is missing
        tangent_ = Vector4::ZERO;
        binormal_ = Vector4::ZERO;
    }
}

void ModelVertex::PruneElements(const ModelVertexFormat& format)
{
    static const auto pruneElement = [](Vector4& element, VertexElementType type)
    {
        if (type == ModelVertexFormat::Undefined)
            element = Vector4::ZERO;
    };

    pruneElement(position_, format.position_);
    pruneElement(normal_, format.normal_);
    pruneElement(tangent_, format.tangent_);
    pruneElement(binormal_, format.binormal_);
    pruneElement(blendIndices_, format.blendIndices_);
    pruneElement(blendWeights_, format.blendWeights_);
    pruneElement(position_, format.position_);
    for (unsigned i = 0; i < MaxColors; ++i)
        pruneElement(color_[i], format.color_[i]);
    for (unsigned i = 0; i < MaxUVs; ++i)
        pruneElement(uv_[i], format.uv_[i]);
}

unsigned GeometryLODView::GetNumPrimitives() const
{
    assert(CheckIndexCount(primitiveType_, indices_.size()));
    switch (primitiveType_)
    {
    case TRIANGLE_LIST:
        return indices_.size() / 3;

    case LINE_LIST:
        return indices_.size() / 2;

    case POINT_LIST:
        return indices_.size();

    case TRIANGLE_STRIP:
    case TRIANGLE_FAN:
        return indices_.size() >= 3 ? indices_.size() - 2 : 0;

    case LINE_STRIP:
        return indices_.size() >= 2 ? indices_.size() - 1 : 0;

    default:
        return 0;
    }
}

bool GeometryLODView::IsTriangleGeometry() const
{
    return primitiveType_ == TRIANGLE_LIST
        || primitiveType_ == TRIANGLE_STRIP
        || primitiveType_ == TRIANGLE_FAN;
}

bool GeometryLODView::IsLineGeometry() const
{
    return primitiveType_ == LINE_LIST
        || primitiveType_ == LINE_STRIP;
}

bool GeometryLODView::IsPointGeometry() const
{
    return primitiveType_ == POINT_LIST;
}

Vector3 GeometryLODView::CalculateCenter() const
{
    Vector3 center;
    for (const ModelVertex& vertex : vertices_)
        center += vertex.position_.ToVector3();
    return vertices_.empty() ? Vector3::ZERO : center / static_cast<float>(vertices_.size());
}

unsigned GeometryLODView::CalculateNumMorphs() const
{
    unsigned numMorphs = 0;
    for (const auto& [morphIndex, morphVector] : morphs_)
        numMorphs = ea::max(numMorphs, morphIndex + 1);
    return numMorphs;
}

void GeometryLODView::Normalize()
{
    for (ModelVertex& vertex : vertices_)
        vertex.PruneElements(vertexFormat_);

    if (indices_.empty())
    {
        indices_.resize(vertices_.size());
        ea::iota(indices_.begin(), indices_.end(), 0);
    }

    for (auto& [index, morphVector] : morphs_)
        NormalizeModelVertexMorphVector(morphVector);
}

void GeometryLODView::InvalidateNormalsAndTangents()
{
    for (ModelVertex& vertex : vertices_)
    {
        vertex.normal_ = Vector4::ZERO;
        vertex.tangent_ = Vector4::ZERO;
    }

    for (auto& [morphIndex, morphData] : morphs_)
    {
        for (ModelVertexMorph& vertexMorph : morphData)
        {
            vertexMorph.normalDelta_ = Vector3::ZERO;
            vertexMorph.tangentDelta_ = Vector3::ZERO;
        }
    }
}

void GeometryLODView::RecalculateFlatNormals()
{
    if (!IsTriangleGeometry())
    {
        assert(0);
        return;
    }

    InvalidateNormalsAndTangents();

    ea::vector<ModelVertex> newVertices;
    ea::unordered_multimap<unsigned, unsigned> oldToNewVertex;
    ForEachTriangle([&](unsigned i0, unsigned i1, unsigned i2)
    {
        ModelVertex v0 = vertices_[i0];
        ModelVertex v1 = vertices_[i1];
        ModelVertex v2 = vertices_[i2];

        const auto p0 = v0.position_.ToVector3();
        const auto p1 = v1.position_.ToVector3();
        const auto p2 = v2.position_.ToVector3();
        const Vector3 normal = (p1 - p0).CrossProduct(p2 - p0).Normalized();

        v0.normal_ = normal.ToVector4();
        v1.normal_ = normal.ToVector4();
        v2.normal_ = normal.ToVector4();

        const unsigned newIndex = newVertices.size();
        newVertices.push_back(v0);
        newVertices.push_back(v1);
        newVertices.push_back(v2);

        oldToNewVertex.emplace(i0, newIndex);
        oldToNewVertex.emplace(i1, newIndex + 1);
        oldToNewVertex.emplace(i2, newIndex + 2);
    });

    primitiveType_ = TRIANGLE_LIST;
    vertices_ = newVertices;
    indices_.resize(vertices_.size());
    ea::iota(indices_.begin(), indices_.end(), 0);

    for (auto& [morphIndex, morphVector] : morphs_)
    {
        ModelVertexMorphVector newMorphVector;
        for (const ModelVertexMorph& vertexMorph : morphVector)
        {
            const auto range = oldToNewVertex.equal_range(vertexMorph.index_);
            if (range.first == range.second)
                continue;

            for (auto iter = range.first; iter != range.second; ++iter)
            {
                ModelVertexMorph newVertexMorph = vertexMorph;
                newVertexMorph.index_ = iter->second;
                newMorphVector.push_back(newVertexMorph);
            }
        }
        morphVector = newMorphVector;
    }
}

void GeometryLODView::RecalculateSmoothNormals()
{
    if (!IsTriangleGeometry())
    {
        assert(0);
        return;
    }

    InvalidateNormalsAndTangents();
    ForEachTriangle([&](unsigned i0, unsigned i1, unsigned i2)
    {
        ModelVertex& v0 = vertices_[i0];
        ModelVertex& v1 = vertices_[i1];
        ModelVertex& v2 = vertices_[i2];

        const auto p0 = v0.position_.ToVector3();
        const auto p1 = v1.position_.ToVector3();
        const auto p2 = v2.position_.ToVector3();
        const Vector3 normal = (p1 - p0).CrossProduct(p2 - p0).Normalized();

        v0.normal_ += normal.ToVector4();
        v1.normal_ += normal.ToVector4();
        v2.normal_ += normal.ToVector4();
    });

    for (ModelVertex& vertex : vertices_)
        vertex.normal_ = vertex.normal_.ToVector3().Normalized().ToVector4();
}

void GeometryLODView::RecalculateTangents()
{
    if (!IsTriangleGeometry())
    {
        assert(0);
        return;
    }

    GenerateTangents(vertices_.data(), sizeof(ModelVertex),
        indices_.data(), sizeof(unsigned), 0, indices_.size(),
        offsetof(ModelVertex, normal_), offsetof(ModelVertex, uv_), offsetof(ModelVertex, tangent_));
}

unsigned GeometryView::CalculateNumMorphs() const
{
    unsigned numMorphs = 0;
    for (const GeometryLODView& lodView : lods_)
        numMorphs = ea::max(numMorphs, lodView.CalculateNumMorphs());
    return numMorphs;
}

void GeometryView::Normalize()
{
    for (GeometryLODView& lodView : lods_)
        lodView.Normalize();
}

void ModelView::Clear()
{
    geometries_.clear();
    bones_.clear();
    metadata_.clear();
}

bool ModelView::ImportModel(const Model* model)
{
    Clear();

    // Read name
    name_ = model->GetName();

    // Read metadata
    for (const ea::string& key : model->GetMetadataKeys())
        metadata_.emplace(key, model->GetMetadata(key));

    const auto& modelVertexBuffers = model->GetVertexBuffers();
    const auto& modelIndexBuffers = model->GetIndexBuffers();
    const auto& modelGeometries = model->GetGeometries();
    const auto& modelMorphs = model->GetMorphs();

    // Read morphs metadata
    morphs_.resize(modelMorphs.size());
    for (unsigned morphIndex = 0; morphIndex < modelMorphs.size(); ++morphIndex)
    {
        morphs_[morphIndex].name_ = modelMorphs[morphIndex].name_;
        morphs_[morphIndex].initialWeight_ = modelMorphs[morphIndex].weight_;
    }

    // Read geometries
    const unsigned numGeometries = modelGeometries.size();
    geometries_.resize(numGeometries);
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        const unsigned numLods = modelGeometries[geometryIndex].size();
        geometries_[geometryIndex].lods_.resize(numLods);
        for (unsigned lodIndex = 0; lodIndex < numLods; ++lodIndex)
        {
            const Geometry* modelGeometry = modelGeometries[geometryIndex][lodIndex];

            GeometryLODView geometry;
            geometry.lodDistance_ = modelGeometry->GetLodDistance();
            geometry.primitiveType_ = modelGeometry->GetPrimitiveType();

            // Copy indices
            if (const IndexBuffer* modelIndexBuffer = modelGeometry->GetIndexBuffer())
            {
                const unsigned numIndices = modelGeometry->GetIndexCount();
                if (!CheckIndexCount(geometry.primitiveType_, numIndices))
                {
                    URHO3D_LOGERROR("Incorrect number of geometry indices");
                    return false;
                }

                geometry.indices_ = modelIndexBuffer->GetUnpackedData(
                    modelGeometry->GetIndexStart(), modelGeometry->GetIndexCount());
            }
            else
            {
                const unsigned numIndices = modelGeometry->GetVertexCount();
                if (!CheckIndexCount(geometry.primitiveType_, numIndices))
                {
                    URHO3D_LOGERROR("Incorrect number of geometry vertices");
                    return false;
                }

                geometry.indices_.resize(numIndices);
                ea::iota(geometry.indices_.begin(), geometry.indices_.end(), 0);
            }

            // Adjust indices
            for (unsigned& index : geometry.indices_)
                index -= modelGeometry->GetVertexStart();

            // Copy vertices and read vertex format
            const unsigned vertexStart = modelGeometry->GetVertexStart();
            const unsigned vertexCount = modelGeometry->GetVertexCount();
            geometry.vertices_.resize(vertexCount);
            for (VertexBuffer* modelVertexBuffer : modelGeometry->GetVertexBuffers())
            {
                if (!CheckVertexElements(modelVertexBuffer->GetElements()))
                {
                    URHO3D_LOGERROR("Unsupported vertex elements are present");
                    return false;
                }

                const auto vertexBufferData = GetVertexBufferData(modelVertexBuffer,
                    vertexStart, vertexCount);
                const auto vertexElements = modelVertexBuffer->GetElements();
                for (unsigned i = 0; i < vertexCount; ++i)
                {
                    for (const VertexElement& element : vertexElements)
                        geometry.vertices_[i].ReplaceElement(vertexBufferData[i], element);
                }

                const auto vertexFormat = ParseVertexElements(vertexElements);
                geometry.vertexFormat_.MergeFrom(vertexFormat);

                // Read morphs for this vertex buffer
                for (unsigned morphIndex = 0; morphIndex < modelMorphs.size(); ++morphIndex)
                {
                    const ModelMorph& modelMorph = modelMorphs[morphIndex];

                    const unsigned vertexBufferIndex = modelVertexBuffers.index_of(
                        SharedPtr<VertexBuffer>(modelVertexBuffer));
                    const auto iter = modelMorph.buffers_.find(vertexBufferIndex);
                    if (iter == modelMorph.buffers_.end())
                        continue;

                    ModelVertexMorphVector& morphData = geometry.morphs_[morphIndex];
                    const VertexBufferMorph& vertexBufferMorph = iter->second;
                    for (unsigned i = 0; i < vertexBufferMorph.vertexCount_; ++i)
                    {
                        const ModelVertexMorph vertexMorph = ReadVertexMorph(vertexBufferMorph, i);
                        if (vertexMorph.index_ >= vertexStart && vertexMorph.index_ < vertexStart + vertexCount)
                            morphData.push_back(vertexMorph);
                    }
                }
            }

            // Cleanup morphs
            for (auto& [morphIndex, morphVector] : geometry.morphs_)
            {
                static const auto isEqual = [](const ModelVertexMorph& lhs, const ModelVertexMorph& rhs)
                {
                    return lhs.index_ == rhs.index_;
                };
                ea::stable_sort(morphVector.begin(), morphVector.end(), isEqual);

                ModelVertexMorphVector newMorphVector;
                for (const ModelVertexMorph& vertexMorph : morphVector)
                {
                    if (!newMorphVector.empty())
                    {
                        ModelVertexMorph& prevVertexMorph = newMorphVector.back();
                        if (prevVertexMorph.index_ == vertexMorph.index_)
                        {
                            if (vertexMorph.HasPosition())
                                prevVertexMorph.positionDelta_ = vertexMorph.positionDelta_;
                            if (vertexMorph.HasNormal())
                                prevVertexMorph.normalDelta_ = vertexMorph.normalDelta_;
                            if (vertexMorph.HasTangent())
                                prevVertexMorph.tangentDelta_ = vertexMorph.tangentDelta_;
                            continue;
                        }
                    }

                    newMorphVector.push_back(vertexMorph);
                }

                morphVector = ea::move(newMorphVector);

                for (ModelVertexMorph& vertexMorph : morphVector)
                    vertexMorph.index_ -= vertexStart;
            }

            ea::erase_if(geometry.morphs_,
                [](const auto& elem) { return elem.second.empty(); });

            geometries_[geometryIndex].lods_[lodIndex] = ea::move(geometry);
        }
    }

    // Read bones
    const Skeleton& skeleton = model->GetSkeleton();
    const auto& modelBones = skeleton.GetBones();
    const unsigned numBones = modelBones.size();

    bool hasRootBone = false;
    bones_.resize(numBones);
    for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
    {
        const Bone& modelBone = modelBones[boneIndex];
        const bool isRootBone = modelBone.parentIndex_ == boneIndex;

        if (isRootBone)
        {
            if (hasRootBone)
            {
                URHO3D_LOGERROR("Multiple root bones are present");
                return false;
            }

            hasRootBone = true;
        }

        BoneView& bone = bones_[boneIndex];
        bone.name_ = modelBone.name_;
        bone.parentIndex_ = isRootBone ? M_MAX_UNSIGNED : modelBone.parentIndex_;

        bone.position_ = modelBone.initialPosition_;
        bone.rotation_ = modelBone.initialRotation_;
        bone.scale_ = modelBone.initialScale_;
        bone.offsetMatrix_ = modelBone.offsetMatrix_;

        bone.shapeFlags_ = modelBone.collisionMask_;
        bone.boundingSphereRadius_ = modelBone.radius_;
        bone.localBoundingBox_ = modelBone.boundingBox_;
    }

    return true;
}

void ModelView::ExportModel(Model* model, ModelViewExportFlags flags) const
{
    struct VertexBufferData
    {
        unsigned vertexBufferIndex_{M_MAX_UNSIGNED};
        ea::vector<ModelVertex> vertices_;
        ea::vector<ModelVertexMorphVector> morphs_;

        SharedPtr<VertexBuffer> buffer_;
        unsigned morphRangeStart_{};
        unsigned morphRangeCount_{};
    };

    // Collect vertices and indices
    ea::unordered_map<ModelVertexFormat, VertexBufferData> vertexBuffersData;
    ea::vector<unsigned> indexBufferData;
    SharedPtr<IndexBuffer> indexBuffer;

    for (const GeometryView& sourceGeometry : geometries_)
    {
        for (const GeometryLODView& sourceGeometryLod : sourceGeometry.lods_)
        {
            const unsigned newVertexBufferIndex = vertexBuffersData.size();

            VertexBufferData& vertexBufferData = vertexBuffersData[sourceGeometryLod.vertexFormat_];
            const unsigned startVertex = vertexBufferData.vertices_.size();
            const unsigned startIndex = indexBufferData.size();

            if (vertexBufferData.vertexBufferIndex_ == M_MAX_UNSIGNED)
                vertexBufferData.vertexBufferIndex_ = newVertexBufferIndex;

            vertexBufferData.vertices_.append(sourceGeometryLod.vertices_);
            indexBufferData.append(sourceGeometryLod.indices_);

            for (unsigned i = startIndex; i < indexBufferData.size(); ++i)
                indexBufferData[i] += startVertex;

            for (const auto& [morphIndex, morphData] : sourceGeometryLod.morphs_)
            {
                if (morphIndex >= vertexBufferData.morphs_.size())
                    vertexBufferData.morphs_.resize(morphIndex + 1);

                ModelVertexMorphVector& vertexBufferMorph = vertexBufferData.morphs_[morphIndex];
                const unsigned startMorphVertex = vertexBufferMorph.size();
                vertexBufferMorph.append(morphData);

                for (unsigned i = startMorphVertex; i < vertexBufferMorph.size(); ++i)
                    vertexBufferMorph[i].index_ += startVertex;
            }
        }
    }

    const unsigned numVertexBuffers = vertexBuffersData.size();
    const bool largeIndices = HasLargeIndices(indexBufferData);

    const bool inplace = flags.Test(ModelViewExportFlag::Inplace);
    if (inplace)
    {
        // Validate inplace export
        const auto& vertexBuffers = model->GetVertexBuffers();
        for (auto& [vertexFormat, vertexBufferData] : vertexBuffersData)
        {
            const unsigned index = vertexBufferData.vertexBufferIndex_;
            VertexBuffer* originalVertexBuffer = index < vertexBuffers.size() ? vertexBuffers[index] : nullptr;
            if (!originalVertexBuffer)
            {
                URHO3D_LOGERROR("Cannot create Model inplace: Vertex Buffer {} is not found", index);
                return;
            }

            if (originalVertexBuffer->GetVertexCount() < vertexBufferData.vertices_.size())
            {
                URHO3D_LOGERROR(
                    "Cannot create Model inplace: Vertex Buffer {} has only {} vertices and {} are required", index,
                    originalVertexBuffer->GetVertexCount(), vertexBufferData.vertices_.size());
                return;
            }

            if (vertexFormat.ToVertexElements() != originalVertexBuffer->GetElements())
            {
                URHO3D_LOGERROR("Cannot create Model inplace: Vertex Buffer {} elements don't match", index);
                return;
            }

            vertexBufferData.buffer_ = originalVertexBuffer;
        }

        const auto& indexBuffers = model->GetIndexBuffers();
        IndexBuffer* originalIndexBuffer = !indexBuffers.empty() ? indexBuffers.front() : nullptr;
        if (!originalIndexBuffer)
        {
            URHO3D_LOGERROR("Cannot create Model inplace: Index Buffer is not found");
            return;
        }

        if (originalIndexBuffer->GetIndexCount() < indexBufferData.size())
        {
            URHO3D_LOGERROR("Cannot create Model inplace: Index Buffer has only {} indices and {} are required",
                originalIndexBuffer->GetIndexCount(), indexBufferData.size());
            return;
        }

        if (largeIndices != (originalIndexBuffer->GetIndexSize() == 4))
        {
            URHO3D_LOGERROR("Cannot create Model inplace: Index Buffer index size does not match");
            return;
        }

        indexBuffer = originalIndexBuffer;
    }
    else
    {
        // Create vertex buffers
        for (auto& [vertexFormat, vertexBufferData] : vertexBuffersData)
        {
            const auto vertexElements = CollectVertexElements(vertexFormat);
            if (vertexElements.empty())
                URHO3D_LOGERROR("No vertex elements in vertex buffer");

            auto vertexBuffer = MakeShared<VertexBuffer>(context_);
            vertexBuffer->SetDebugName(Format("Model '{}' Vertex Buffer", name_));
            vertexBuffer->SetShadowed(true);
            vertexBuffer->SetSize(vertexBufferData.vertices_.size(), vertexElements);

            vertexBufferData.buffer_ = vertexBuffer;
        }

        // Create index buffer
        indexBuffer = MakeShared<IndexBuffer>(context_);
        indexBuffer->SetDebugName(Format("Model '{}' Index Buffer", name_));
        indexBuffer->SetShadowed(true);
        indexBuffer->SetSize(indexBufferData.size(), largeIndices);
    }

    // Copy data
    for (auto& [_, vertexBufferData] : vertexBuffersData)
        SetVertexBufferData(vertexBufferData.buffer_, vertexBufferData.vertices_);
    indexBuffer->SetUnpackedData(indexBufferData.data(), 0, indexBufferData.size());

    // Initialize morph info
    for (auto& [vertexFormat, vertexBufferData] : vertexBuffersData)
    {
        unsigned minMorphVertex = M_MAX_UNSIGNED;
        unsigned maxMorphVertex = 0;
        for (const ModelVertexMorphVector& morphData : vertexBufferData.morphs_)
        {
            for (const ModelVertexMorph& vertexMorph : morphData)
            {
                minMorphVertex = ea::min(minMorphVertex, vertexMorph.index_);
                maxMorphVertex = ea::max(maxMorphVertex, vertexMorph.index_);
            }
        }

        if (minMorphVertex <= maxMorphVertex)
        {
            vertexBufferData.morphRangeStart_ = minMorphVertex;
            vertexBufferData.morphRangeCount_ = maxMorphVertex - minMorphVertex + 1;
        }
    }

    // Extract vertex buffers info
    ea::vector<SharedPtr<VertexBuffer>> vertexBuffers(numVertexBuffers);
    ea::vector<unsigned> morphRangeStarts(numVertexBuffers);
    ea::vector<unsigned> morphRangeCounts(numVertexBuffers);
    for (auto& [_, vertexBufferData] : vertexBuffersData)
    {
        const unsigned index = vertexBufferData.vertexBufferIndex_;
        vertexBuffers[index] = vertexBufferData.buffer_;
        morphRangeStarts[index] = vertexBufferData.morphRangeStart_;
        morphRangeCounts[index] = vertexBufferData.morphRangeCount_;
    }

    // Create morphs
    ea::vector<ModelMorph> morphs(morphs_.size());
    for (unsigned i = 0; i < morphs_.size(); ++i)
    {
        morphs[i].name_ = morphs_[i].name_;
        morphs[i].nameHash_ = morphs_[i].name_;
        morphs[i].weight_ = morphs_[i].initialWeight_;
    }

    for (auto& [vertexFormat, vertexBufferData] : vertexBuffersData)
    {
        const unsigned numMorphsForVertexBuffer = vertexBufferData.morphs_.size();
        if (morphs.size() < numMorphsForVertexBuffer)
            morphs.resize(numMorphsForVertexBuffer);

        for (unsigned i = 0; i < numMorphsForVertexBuffer; ++i)
        {
            const ModelVertexMorphVector& morphDataForBuffer = vertexBufferData.morphs_[i];
            ModelMorph& modelMorph = morphs[i];

            const unsigned vertexBufferIndex = vertexBuffers.index_of(vertexBufferData.buffer_);
            VertexBufferMorph vertexBufferMorph = CreateVertexBufferMorph(morphDataForBuffer);
            if (vertexBufferMorph.vertexCount_ > 0)
                modelMorph.buffers_[vertexBufferIndex] = ea::move(vertexBufferMorph);
        }
    }

    // Create model
    model->SetName(name_);
    for (const auto& var : metadata_)
        model->AddMetadata(var.first, var.second);

    model->SetBoundingBox(CalculateBoundingBox());
    model->SetVertexBuffers(vertexBuffers, morphRangeStarts, morphRangeCounts);
    model->SetIndexBuffers({ indexBuffer });
    model->SetMorphs(morphs);

    // Write geometries
    unsigned indexStart = 0;
    ea::unordered_map<ModelVertexFormat, unsigned> vertexStart;

    const unsigned numGeometries = geometries_.size();
    model->SetNumGeometries(numGeometries);
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        const GeometryView& sourceGeometry = geometries_[geometryIndex];
        if (sourceGeometry.lods_.empty())
            continue;

        const unsigned numLods = sourceGeometry.lods_.size();
        const Vector3 geometryCenter = sourceGeometry.lods_[0].CalculateCenter();
        model->SetGeometryCenter(geometryIndex, geometryCenter);
        model->SetNumGeometryLodLevels(geometryIndex, numLods);
        for (unsigned lodIndex = 0; lodIndex < numLods; ++lodIndex)
        {
            const GeometryLODView& sourceGeometryLod = sourceGeometry.lods_[lodIndex];
            const ModelVertexFormat& vertexFormat = sourceGeometryLod.vertexFormat_;
            const unsigned indexCount = sourceGeometryLod.indices_.size();
            const unsigned vertexCount = sourceGeometryLod.vertices_.size();

            SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);

            geometry->SetNumVertexBuffers(1);
            geometry->SetVertexBuffer(0, vertexBuffersData[vertexFormat].buffer_);
            geometry->SetIndexBuffer(indexBuffer);
            geometry->SetLodDistance(sourceGeometryLod.lodDistance_);
            geometry->SetDrawRange(sourceGeometryLod.primitiveType_, indexStart, indexCount, vertexStart[vertexFormat], vertexCount);

            model->SetGeometry(geometryIndex, lodIndex, geometry);

            indexStart += indexCount;
            vertexStart[vertexFormat] += vertexCount;
        }
    }

    // Write bones
    const unsigned numBones = bones_.size();

    Skeleton skeleton;
    skeleton.SetNumBones(numBones);

    for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
    {
        const BoneView& sourceBone = bones_[boneIndex];
        const bool isRootBone = sourceBone.parentIndex_ == M_MAX_UNSIGNED;
        Bone& bone = *skeleton.GetBone(boneIndex);

        bone.name_ = sourceBone.name_;
        bone.nameHash_ = sourceBone.name_;
        bone.parentIndex_ = isRootBone ? boneIndex : sourceBone.parentIndex_;

        bone.initialPosition_ = sourceBone.position_;
        bone.initialRotation_ = sourceBone.rotation_;
        bone.initialScale_ = sourceBone.scale_;
        bone.offsetMatrix_ = sourceBone.offsetMatrix_;

        bone.collisionMask_ = sourceBone.shapeFlags_;
        bone.radius_ = sourceBone.boundingSphereRadius_;
        bone.boundingBox_ = sourceBone.localBoundingBox_;

        if (isRootBone)
            skeleton.SetRootBoneIndex(boneIndex);
    }

    skeleton.UpdateBoneOrder();
    model->SetSkeleton(skeleton);
}

SharedPtr<Model> ModelView::ExportModel(const ea::string& name) const
{
    auto model = MakeShared<Model>(context_);
    ExportModel(model);
    if (!name.empty())
        model->SetName(name);
    return model;
}

ResourceRefList ModelView::ExportMaterialList() const
{
    ResourceRefList result(Material::GetTypeStatic());

    for (const GeometryView& geometry : geometries_)
        result.names_.push_back(geometry.material_);
    return result;
}

const Variant& ModelView::GetMetadata(const ea::string& key) const
{
    auto it = metadata_.find(key);
    if (it != metadata_.end())
        return it->second;

    return Variant::EMPTY;
}

BoundingBox ModelView::CalculateBoundingBox() const
{
    BoundingBox boundingBox;
    for (const GeometryView& sourceGeometry : geometries_)
    {
        for (const GeometryLODView& sourceGeometryLod : sourceGeometry.lods_)
        {
            for (const ModelVertex& vertex : sourceGeometryLod.vertices_)
                boundingBox.Merge(vertex.position_.ToVector3());
        }
    }
    return boundingBox;
}

void ModelView::Normalize()
{
    for (GeometryView& geometryView : geometries_)
        geometryView.Normalize();

    unsigned numMorphs = 0;
    for (GeometryView& geometryView : geometries_)
        numMorphs = ea::max(numMorphs, geometryView.CalculateNumMorphs());
    if (morphs_.size() < numMorphs)
        morphs_.resize(numMorphs);
}

void ModelView::MirrorGeometriesX()
{
    for (GeometryView& geometryView : geometries_)
    {
        for (GeometryLODView& lodView : geometryView.lods_)
        {
            for (ModelVertex& vertex : lodView.vertices_)
            {
                vertex.position_.x_ = -vertex.position_.x_;
                vertex.normal_.x_ = -vertex.normal_.x_;
                vertex.tangent_.x_ = -vertex.tangent_.x_;
            }

            for (auto& [morphIndex, morphVector] : lodView.morphs_)
            {
                for (ModelVertexMorph& vertexMorph : morphVector)
                {
                    vertexMorph.positionDelta_.x_ = -vertexMorph.positionDelta_.x_;
                    vertexMorph.normalDelta_.x_ = -vertexMorph.normalDelta_.x_;
                    vertexMorph.tangentDelta_.x_ = -vertexMorph.tangentDelta_.x_;
                }
            }

            if (lodView.IsTriangleGeometry())
            {
                const unsigned numPrimitives = lodView.GetNumPrimitives();
                switch (lodView.primitiveType_)
                {
                case TRIANGLE_LIST:
                    for (unsigned i = 0; i < numPrimitives; ++i)
                        ea::swap(lodView.indices_[i * 3 + 1], lodView.indices_[i * 3 + 2]);
                    break;

                case TRIANGLE_STRIP:
                    if (numPrimitives > 0 && numPrimitives % 2 == 0)
                        lodView.indices_.push_back(lodView.indices_.back());
                    ea::reverse(lodView.indices_.begin(), lodView.indices_.end());
                    break;

                case TRIANGLE_FAN:
                    if (numPrimitives >= 1)
                        ea::reverse(lodView.indices_.begin() + 1, lodView.indices_.end());
                    break;

                default:
                    break;
                }
            }
        }
    }
}

void ModelView::ScaleGeometries(float scale)
{
    for (GeometryView& geometryView : geometries_)
    {
        for (GeometryLODView& lodView : geometryView.lods_)
        {
            for (ModelVertex& vertex : lodView.vertices_)
                vertex.position_ = (scale * vertex.GetPosition()).ToVector4(vertex.position_.w_);

            for (auto& [morphIndex, morphVector] : lodView.morphs_)
            {
                for (ModelVertexMorph& vertexMorph : morphVector)
                    vertexMorph.positionDelta_ *= scale;
            }
        }
    }
}

void ModelView::CalculateMissingNormals(bool flatNormals)
{
    for (GeometryView& geometryView : geometries_)
    {
        for (GeometryLODView& lodView : geometryView.lods_)
        {
            if (!lodView.IsTriangleGeometry())
                continue;
            if (lodView.vertexFormat_.normal_ != ModelVertexFormat::Undefined)
                continue;

            lodView.vertexFormat_.normal_ = TYPE_VECTOR3;
            lodView.vertexFormat_.tangent_ = ModelVertexFormat::Undefined;
            if (flatNormals)
                lodView.RecalculateFlatNormals();
            else
                lodView.RecalculateSmoothNormals();
        }
    }
}

void ModelView::CalculateMissingTangents()
{
    for (GeometryView& geometryView : geometries_)
    {
        for (GeometryLODView& lodView : geometryView.lods_)
        {
            if (!lodView.IsTriangleGeometry())
                continue;
            if (lodView.vertexFormat_.tangent_ != ModelVertexFormat::Undefined)
                continue;

            lodView.vertexFormat_.tangent_ = TYPE_VECTOR4;
            lodView.RecalculateTangents();
        }
    }
}

void ModelView::RepairBoneWeights()
{
    if (bones_.empty())
        return;

    for (GeometryView& geometryView : geometries_)
    {
        for (GeometryLODView& lodView : geometryView.lods_)
        {
            for (ModelVertex& vertex : lodView.vertices_)
            {
                // Reset invalid bones
                for (unsigned i = 0; i < ModelVertex::MaxBones; ++i)
                {
                    const float index = vertex.blendIndices_[i];
                    const float weight = vertex.blendWeights_[i];
                    if (index < 0 || index >= bones_.size() || weight < 0.0f)
                    {
                        vertex.blendIndices_[i] = 0;
                        vertex.blendWeights_[i] = 0.0f;
                    }
                }

                // Skip if okay
                const float weightSum = vertex.blendWeights_.DotProduct(Vector4::ONE);
                if (Equals(weightSum, 1.0f))
                    continue;

                // Revert if degenerate
                if (weightSum < M_EPSILON)
                {
                    vertex.blendIndices_ = Vector4::ZERO;
                    vertex.blendWeights_ = { 1.0f, 0.0f, 0.0f, 0.0f };
                    continue;
                }

                // Normalize otherwise
                vertex.blendWeights_ /= weightSum;
            }
        }
    }
}

void ModelView::RecalculateBoneBoundingBoxes()
{
    if (bones_.empty())
        return;

    for (BoneView& bone : bones_)
        bone.SetLocalBoundingBox(BoundingBox{});

    for (const GeometryView& geometryView : geometries_)
    {
        for (const GeometryLODView& lodView : geometryView.lods_)
        {
            for (const ModelVertex& vertex : lodView.vertices_)
            {
                for (auto [boneIndex, boneWeight] : vertex.GetBlendIndicesAndWeights())
                {
                    if (boneIndex >= bones_.size() || boneWeight < M_LARGE_EPSILON)
                        continue;

                    BoneView& bone = bones_[boneIndex];
                    bone.localBoundingBox_.Merge(bone.offsetMatrix_ * vertex.GetPosition());
                }
            }
        }
    }

    for (BoneView& bone : bones_)
    {
        if (!bone.localBoundingBox_.Defined())
            bone.ResetBoundingVolume();
    }

}

void ModelView::SetMorph(unsigned index, const ModelMorphView& morph)
{
    if (morphs_.size() <= index)
        morphs_.resize(index + 1);
    morphs_[index] = morph;
}

void ModelView::AddMetadata(const ea::string& key, const Variant& variant)
{
    metadata_.insert_or_assign(key, variant);
}

}
