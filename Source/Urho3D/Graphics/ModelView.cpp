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

#include "../Graphics/ModelView.h"

#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/Log.h"

namespace Urho3D
{

namespace
{

/// Compare two vertex elements by semantic and index.
static bool CompareVertexElementSemantics(const VertexElement& lhs, const VertexElement& rhs)
{
    return lhs.semantic_ == rhs.semantic_ && lhs.index_ == rhs.index_;
}

/// Get vertex elements of all vertex buffers.
const ea::vector<VertexElement> GetEffectiveVertexElements(const Model* model)
{
    ea::vector<VertexElement> elements;
    for (const VertexBuffer* vertexBuffer : model->GetVertexBuffers())
    {
        for (const VertexElement& element : vertexBuffer->GetElements())
        {
            if (ea::find(elements.begin(), elements.end(), element, CompareVertexElementSemantics) == elements.end())
                elements.push_back(element);
        }
    }
    return elements;
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
    const unsigned vertexCount = vertexBuffer->GetVertexCount();
    const ea::vector<VertexElement>& vertexElements = vertexBuffer->GetElements();

    ea::vector<Vector4> buffer(vertexElements.size() * vertexCount);
    VertexBuffer::ShuffleUnpackedVertexData(vertexCount,
        reinterpret_cast<const Vector4*>(data.data()), ModelVertex::VertexElements, buffer.data(), vertexElements);

    vertexBuffer->SetUnpackedData(buffer.data());
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
    return index >= 0xfffe;
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
        && lodDistance_ == rhs.lodDistance_;
}

bool GeometryView::operator ==(const GeometryView& rhs) const
{
    return lods_ == rhs.lods_;
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
                const Vector3 normal3 = static_cast<Vector3>(tangent_);
                const Vector3 tangent3 = static_cast<Vector3>(normal_);
                const Vector3 binormal3 = tangent_.w_ * normal3.CrossProduct(tangent3);
                binormal_ = { binormal3.Normalized(), 0};
            }
            else if (hasBinormal && !hasTangentBinormalCombined)
            {
                // Repair tangent W component from binormal, tangent and normal
                const Vector3 normal3 = static_cast<Vector3>(tangent_);
                const Vector3 tangent3 = static_cast<Vector3>(normal_);
                const Vector3 binormal3 = static_cast<Vector3>(binormal_);
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

Vector3 GeometryLODView::CalculateCenter() const
{
    Vector3 center;
    for (const ModelVertex& vertex : vertices_)
        center += static_cast<Vector3>(vertex.position_);
    return vertices_.empty() ? Vector3::ZERO : center / static_cast<float>(vertices_.size());
}

void ModelView::Clear()
{
    vertexFormat_ = {};
    geometries_.clear();
    bones_.clear();
    metadata_.clear();
}

bool ModelView::ImportModel(const Model* model)
{
    Clear();

    // Read vertex format
    vertexFormat_ = ParseVertexElements(GetEffectiveVertexElements(model));

    // Read metadata
    for (const ea::string& key : model->GetMetadataKeys())
        metadata_.emplace(key, model->GetMetadata(key));

    const auto& modelVertexBuffers = model->GetVertexBuffers();
    const auto& modelIndexBuffers = model->GetIndexBuffers();
    const auto& modelGeometries = model->GetGeometries();

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

            if (modelGeometry->GetPrimitiveType() != TRIANGLE_LIST)
            {
                URHO3D_LOGERROR("Only TriangleList models are supported now.");
                return false;
            }

            // Copy indices
            if (modelGeometry->GetIndexCount() % 3 != 0)
            {
                URHO3D_LOGERROR("Incorrect number of geometry indices");
                return false;
            }

            const IndexBuffer* modelIndexBuffer = modelGeometry->GetIndexBuffer();
            const unsigned numIndices = modelGeometry->GetIndexCount();
            geometry.indices_ = modelIndexBuffer->GetUnpackedData(
                modelGeometry->GetIndexStart(), modelGeometry->GetIndexCount());

            // Adjust indices
            for (unsigned& index : geometry.indices_)
                index -= modelGeometry->GetVertexStart();

            // Copy vertices
            const unsigned vertexCount = modelGeometry->GetVertexCount();
            geometry.vertices_.resize(vertexCount);
            for (const VertexBuffer* modelVertexBuffer : modelGeometry->GetVertexBuffers())
            {
                if (!CheckVertexElements(modelVertexBuffer->GetElements()))
                {
                    URHO3D_LOGERROR("Unsupported vertex elements are present");
                    return false;
                }

                const auto vertexBufferData = GetVertexBufferData(modelVertexBuffer,
                    modelGeometry->GetVertexStart(), vertexCount);
                const auto vertexElements = modelVertexBuffer->GetElements();
                for (unsigned i = 0; i < vertexCount; ++i)
                {
                    for (const VertexElement& element : vertexElements)
                        geometry.vertices_[i].ReplaceElement(vertexBufferData[i], element);
                }
            }

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

void ModelView::ExportModel(Model* model) const
{
    // Collect vertices and indices
    ea::vector<ModelVertex> vertexBufferData;
    ea::vector<unsigned> indexBufferData;

    for (const GeometryView& sourceGeometry : geometries_)
    {
        for (const GeometryLODView& sourceGeometryLod : sourceGeometry.lods_)
        {
            const unsigned startVertex = vertexBufferData.size();
            const unsigned startIndex = indexBufferData.size();

            vertexBufferData.append(sourceGeometryLod.vertices_);
            indexBufferData.append(sourceGeometryLod.indices_);

            for (unsigned i = startIndex; i < indexBufferData.size(); ++i)
                indexBufferData[i] += startVertex;
        }
    }

    // Create vertex and index buffers
    auto modelVertexBuffer = MakeShared<VertexBuffer>(context_);
    modelVertexBuffer->SetShadowed(true);
    modelVertexBuffer->SetSize(vertexBufferData.size(), CollectVertexElements(vertexFormat_));
    SetVertexBufferData(modelVertexBuffer, vertexBufferData);

    const bool largeIndices = HasLargeIndices(indexBufferData);
    auto modelIndexBuffer = MakeShared<IndexBuffer>(context_);
    modelIndexBuffer->SetShadowed(true);
    modelIndexBuffer->SetSize(indexBufferData.size(), largeIndices);
    modelIndexBuffer->SetUnpackedData(indexBufferData.data());

    // Create model
    for (const auto& var : metadata_)
        model->AddMetadata(var.first, var.second);

    model->SetBoundingBox(CalculateBoundingBox());
    model->SetVertexBuffers({ modelVertexBuffer }, {}, {});
    model->SetIndexBuffers({ modelIndexBuffer });

    // Write geometries
    unsigned indexStart = 0;
    unsigned vertexStart = 0;

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
            const unsigned indexCount = sourceGeometryLod.indices_.size();
            const unsigned vertexCount = sourceGeometryLod.vertices_.size();

            SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);

            geometry->SetNumVertexBuffers(1);
            geometry->SetVertexBuffer(0, modelVertexBuffer);
            geometry->SetIndexBuffer(modelIndexBuffer);
            geometry->SetLodDistance(sourceGeometryLod.lodDistance_);
            geometry->SetDrawRange(TRIANGLE_LIST, indexStart, indexCount, vertexStart, vertexCount);

            model->SetGeometry(geometryIndex, lodIndex, geometry);

            indexStart += indexCount;
            vertexStart += vertexCount;
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

    model->SetSkeleton(skeleton);
}

SharedPtr<Model> ModelView::ExportModel(const ea::string& name) const
{
    auto model = MakeShared<Model>(context_);
    model->SetName(name);
    ExportModel(model);
    return model;
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
                boundingBox.Merge(static_cast<Vector3>(vertex.position_));
        }
    }
    return boundingBox;
}

}
