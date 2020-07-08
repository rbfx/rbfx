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

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/SoftwareModelAnimator.h"
#include "../Graphics/VertexBuffer.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

Vector3 TransformNormal(const Matrix3x4& m, const Vector3& v)
{
    return {
        m.m00_ * v.x_ + m.m01_ * v.y_ + m.m02_ * v.z_,
        m.m10_ * v.x_ + m.m11_ * v.y_ + m.m12_ * v.z_,
        m.m20_ * v.x_ + m.m21_ * v.y_ + m.m22_ * v.z_
    };
}

}

SoftwareModelAnimator::SoftwareModelAnimator(Context* context) : Object(context) {}

SoftwareModelAnimator::~SoftwareModelAnimator() {}

void SoftwareModelAnimator::RegisterObject(Context* context)
{
    context->RegisterFactory<SoftwareModelAnimator>();
}

void SoftwareModelAnimator::Initialize(Model* model, bool skinned, unsigned numBones)
{
    originalModel_ = model;
    skinned_ = skinned;
    numBones_ = numBones;
    CloneModelGeometries();
    InitializeAnimationData();
}

void SoftwareModelAnimator::ResetAnimation()
{
    // Copy vertices from original vertex buffers
    for (unsigned i = 0; i < vertexBuffers_.size(); ++i)
    {
        VertexBuffer* clonedBuffer = vertexBuffers_[i];
        if (!clonedBuffer)
            continue;

        VertexBuffer* originalBuffer = originalModel_->GetVertexBuffers()[i];
        const unsigned vertexStart = skinned_ ? 0 : originalModel_->GetMorphRangeStart(i);
        const unsigned vertexCount = skinned_ ? originalBuffer->GetVertexCount() : originalModel_->GetMorphRangeCount(i);
        const unsigned char* sourceData = originalBuffer->GetShadowData() + vertexStart * originalBuffer->GetVertexSize();
        unsigned char* destData = clonedBuffer->GetShadowData() + vertexStart * clonedBuffer->GetVertexSize();

        CopyMorphVertices(destData, sourceData, vertexCount, clonedBuffer, originalBuffer);
    }
}

void SoftwareModelAnimator::ApplyMorphs(ea::span<const ModelMorph> morphs)
{
    for (const ModelMorph& morph : morphs)
    {
        if (morph.weight_ == 0.0f)
            continue;

        for (const auto& bufferMorph : morph.buffers_)
        {
            VertexBuffer* clonedBuffer = vertexBuffers_[bufferMorph.first];
            if (!clonedBuffer)
                continue;

            ApplyMorph(clonedBuffer, bufferMorph.second, morph.weight_);
        }
    }
}

void SoftwareModelAnimator::ApplySkinning(ea::span<const Matrix3x4> worldTransforms)
{
    if (!skinned_)
        return;

    for (unsigned bufferIndex = 0; bufferIndex < vertexBuffers_.size(); ++bufferIndex)
    {
        VertexBuffer* clonedBuffer = vertexBuffers_[bufferIndex];
        const VertexBufferAnimationData& animationData = vertexBuffersData_[bufferIndex];
        if (!clonedBuffer || !animationData.hasSkeletalAnimation_)
            continue;

        if (!animationData.skinNormals_ && !animationData.skinTangents_)
            ApplyVertexBufferSkinning<false, false>(clonedBuffer, animationData, worldTransforms);
        else if (animationData.skinNormals_ && !animationData.skinTangents_)
            ApplyVertexBufferSkinning<true, false>(clonedBuffer, animationData, worldTransforms);
        else if (animationData.skinNormals_ && animationData.skinTangents_)
            ApplyVertexBufferSkinning<true, true>(clonedBuffer, animationData, worldTransforms);
        else
            ApplyVertexBufferSkinning<false, true>(clonedBuffer, animationData, worldTransforms); // this is really weird case
    }
}

template <bool SkinNormals, bool SkinTangents>
void SoftwareModelAnimator::ApplyVertexBufferSkinning(VertexBuffer* clonedBuffer, const VertexBufferAnimationData& animationData,
    ea::span<const Matrix3x4> worldTransforms) const
{
    const unsigned clonedVertexSize = clonedBuffer->GetVertexSize();
    const unsigned normalOffset = clonedBuffer->GetElementOffset(TYPE_VECTOR3, SEM_NORMAL);
    const unsigned tangentOffset = clonedBuffer->GetElementOffset(TYPE_VECTOR4, SEM_TANGENT);

    unsigned char* clonedBufferData = clonedBuffer->GetShadowData();

    unsigned char* positionsData = clonedBufferData;
    unsigned char* normalsData = SkinNormals ? clonedBufferData + normalOffset : nullptr;
    unsigned char* tangentsData = SkinTangents ? clonedBufferData + tangentOffset : nullptr;

    const unsigned char* indicesData = animationData.blendIndices_.data();
    const float* weightsData = animationData.blendWeights_.data();

    const unsigned numVertices = clonedBuffer->GetVertexCount();
    Matrix3x4 matrix;
    for (unsigned vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
    {
        matrix = worldTransforms[indicesData[0]] * weightsData[0];
        for (unsigned boneIndex = 1; boneIndex < numBones_; ++boneIndex)
            matrix = matrix + worldTransforms[indicesData[boneIndex]] * weightsData[boneIndex];

        Vector3& position = *reinterpret_cast<Vector3*>(positionsData);
        position = matrix * position;

        if (SkinNormals)
        {
            Vector3& normal = *reinterpret_cast<Vector3*>(normalsData);
            normal = TransformNormal(matrix, normal);
        }

        if (SkinTangents)
        {
            Vector3& tangent = *reinterpret_cast<Vector3*>(tangentsData);
            tangent = TransformNormal(matrix, tangent);
        }

        // Advance
        indicesData += numBones_;
        weightsData += numBones_;

        positionsData += clonedVertexSize;
        if (SkinNormals)
            normalsData += clonedVertexSize;
        if (SkinTangents)
            tangentsData += clonedVertexSize;
    }

}

void SoftwareModelAnimator::Commit()
{
    for (VertexBuffer* clonedVertexBuffer : vertexBuffers_)
    {
        if (clonedVertexBuffer)
            clonedVertexBuffer->SetData(clonedVertexBuffer->GetShadowData());
    }
}

VertexMaskFlags SoftwareModelAnimator::GetMorphElementMask() const
{
    if (skinned_)
        return MASK_POSITION | MASK_NORMAL | MASK_TANGENT;

    VertexMaskFlags morphElementMask = MASK_NONE;
    for (const ModelMorph& morph : originalModel_->GetMorphs())
    {
        for (const auto& morphedBuffer : morph.buffers_)
            morphElementMask |= morphedBuffer.second.elementMask_;
    }
    return morphElementMask;
}

void SoftwareModelAnimator::CloneModelGeometries()
{
    ea::unordered_map<VertexBuffer*, SharedPtr<VertexBuffer>> originalToClonedMapping;
    const VertexMaskFlags morphElementMask = GetMorphElementMask();

    // Clone vertex buffers
    const auto& originalVertexBuffers = originalModel_->GetVertexBuffers();
    vertexBuffers_.resize(originalVertexBuffers.size());

    for (unsigned i = 0; i < originalVertexBuffers.size(); ++i)
    {
        // Skip buffer if not needed
        if (!skinned_ && !originalModel_->GetMorphRangeCount(i))
            continue;

        // Validate formats
        VertexBuffer* originalVertexBuffer = originalVertexBuffers[i];
        const bool hasPositionVector3 = originalVertexBuffer->HasElement(TYPE_VECTOR3, SEM_POSITION);
        const bool hasPositionVector4 = originalVertexBuffer->HasElement(TYPE_VECTOR4, SEM_POSITION);
        const bool hasNormalVector3 = originalVertexBuffer->HasElement(TYPE_VECTOR3, SEM_NORMAL);
        const bool hasTangentVector4 = originalVertexBuffer->HasElement(TYPE_VECTOR4, SEM_TANGENT);

        const VertexMaskFlags clonedBufferMask = morphElementMask & originalVertexBuffer->GetElementMask();
        if ((clonedBufferMask & MASK_POSITION) && !(hasPositionVector3 || hasPositionVector4))
        {
            URHO3D_LOGERROR("Position must be Vector3 or Vector4 for software skinning and morphing");
            continue;
        }
        if ((clonedBufferMask & MASK_NORMAL) && !hasNormalVector3)
        {
            URHO3D_LOGERROR("Normal must be Vector3 for software skinning and morphing");
            continue;
        }
        if ((clonedBufferMask & MASK_TANGENT) && !hasTangentVector4)
        {
            URHO3D_LOGERROR("Tangent must be Vector4 for software skinning and morphing");
            continue;
        }

        if (!clonedBufferMask)
            continue;

        // Validate offsets
        if (originalVertexBuffer->GetElementOffset(SEM_POSITION) != 0)
        {
            URHO3D_LOGERROR("Position must be at offest 0 for software skinning and morphing");
            continue;
        }
        if (originalVertexBuffer->GetVertexSize() % alignof(float) != 0)
        {
            URHO3D_LOGERROR("Vertex size must be aligned to 4 for software skinning and morphing");
            continue;
        }
        if (originalVertexBuffer->GetElementOffset(SEM_NORMAL) % alignof(float) != 0)
        {
            URHO3D_LOGERROR("Normal offset within vertex must be aligned to 4 for software skinning and morphing");
            continue;
        }
        if (originalVertexBuffer->GetElementOffset(SEM_TANGENT) % alignof(float) != 0)
        {
            URHO3D_LOGERROR("Tangent offset within vertex must be aligned to 4 for software skinning and morphing");
            continue;
        }

        // Clone buffer
        auto clonedVertexBuffer = MakeShared<VertexBuffer>(context_);
        clonedVertexBuffer->SetShadowed(true);
        clonedVertexBuffer->SetSize(originalVertexBuffer->GetVertexCount(), clonedBufferMask, true);
        CopyMorphVertices(clonedVertexBuffer->GetShadowData(), originalVertexBuffer->GetShadowData(),
            originalVertexBuffer->GetVertexCount(), clonedVertexBuffer, originalVertexBuffer);
        clonedVertexBuffer->SetData(clonedVertexBuffer->GetShadowData());
        originalToClonedMapping[originalVertexBuffer] = clonedVertexBuffer;
        vertexBuffers_[i] = clonedVertexBuffer;
    }

    // Clone geometries
    geometries_ = originalModel_->GetGeometries();
    for (auto& geometryLods : geometries_)
    {
        for (SharedPtr<Geometry>& geometry : geometryLods)
        {
            SharedPtr<Geometry> originalGeometry = geometry;
            SharedPtr<Geometry> cloneGeometry = MakeShared<Geometry>(context_);

            // Append vertex buffers with morphed data
            // Note: array grows inside loop
            ea::vector<SharedPtr<VertexBuffer>> vertexBuffers = originalGeometry->GetVertexBuffers();
            const unsigned numVertexBuffers = vertexBuffers.size();
            for (unsigned i = 0; i < numVertexBuffers; ++i)
            {
                VertexBuffer* originalBuffer = vertexBuffers[i];
                const auto clonedBufferIter = originalToClonedMapping.find(originalBuffer);
                if (clonedBufferIter != originalToClonedMapping.end())
                    vertexBuffers.push_back(clonedBufferIter->second);
            }

            cloneGeometry->SetIndexBuffer(originalGeometry->GetIndexBuffer());
            cloneGeometry->SetVertexBuffers(vertexBuffers);
            cloneGeometry->SetDrawRange(originalGeometry->GetPrimitiveType(),
                originalGeometry->GetIndexStart(), originalGeometry->GetIndexCount());
            cloneGeometry->SetLodDistance(originalGeometry->GetLodDistance());

            geometry = cloneGeometry;
        }
    }
}

void SoftwareModelAnimator::InitializeAnimationData()
{
    const unsigned numBuffers = vertexBuffers_.size();
    vertexBuffersData_.resize(numBuffers);
    for (VertexBufferAnimationData& animationData : vertexBuffersData_)
    {
        animationData.hasSkeletalAnimation_ = false;

        animationData.blendIndices_.clear();
        animationData.blendWeights_.clear();
    }

    if (!skinned_)
        return;

    for (unsigned bufferIndex = 0; bufferIndex < numBuffers; ++bufferIndex)
    {
        VertexBuffer* originalBuffer = originalModel_->GetVertexBuffers()[bufferIndex];
        VertexBuffer* clonedBuffer = vertexBuffers_[bufferIndex];

        const unsigned originalVertexSize = originalBuffer->GetVertexSize();
        const unsigned indicesOffset = originalBuffer->GetElementOffset(TYPE_UBYTE4, SEM_BLENDINDICES);
        const unsigned vector4weightsOffset = originalBuffer->GetElementOffset(TYPE_VECTOR4, SEM_BLENDWEIGHTS);
        const unsigned ubyte4weightsOffset = originalBuffer->GetElementOffset(TYPE_UBYTE4_NORM, SEM_BLENDWEIGHTS);
        const unsigned weightsOffset = vector4weightsOffset != M_MAX_UNSIGNED ? vector4weightsOffset : ubyte4weightsOffset;

        if (indicesOffset == M_MAX_UNSIGNED || weightsOffset == M_MAX_UNSIGNED)
            continue;

        const unsigned numVertices = originalBuffer->GetVertexCount();
        VertexBufferAnimationData& animationData = vertexBuffersData_[bufferIndex];
        animationData.hasSkeletalAnimation_ = true;
        animationData.skinNormals_ = clonedBuffer->HasElement(SEM_NORMAL);
        animationData.skinTangents_ = clonedBuffer->HasElement(SEM_TANGENT);
        animationData.blendIndices_.resize(numVertices * numBones_);
        animationData.blendWeights_.resize(numVertices * numBones_);

        const unsigned char* originalBufferData = originalBuffer->GetShadowData();

        const unsigned char* indicesData = originalBufferData + indicesOffset;
        const unsigned char* weightsData = originalBufferData + weightsOffset;

        ea::array<ea::pair<float, unsigned char>, MaxBones> bones;
        for (unsigned vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
        {
            // Copy indices
            for (unsigned boneIndex = 0; boneIndex < MaxBones; ++boneIndex)
                bones[boneIndex].second = indicesData[boneIndex];

            // Copy weights
            if (vector4weightsOffset != M_MAX_UNSIGNED)
            {
                for (unsigned boneIndex = 0; boneIndex < MaxBones; ++boneIndex)
                    memcpy(&bones[boneIndex].first, weightsData + sizeof(float) * boneIndex, sizeof(float));
            }
            else
            {
                for (unsigned boneIndex = 0; boneIndex < MaxBones; ++boneIndex)
                    bones[boneIndex].first = weightsData[boneIndex] / 255.0f;
            }

            // Store indices and weights in arrays
            if (numBones_ == MaxBones)
            {
                for (unsigned boneIndex = 0; boneIndex < MaxBones; ++boneIndex)
                {
                    animationData.blendIndices_[vertexIndex * numBones_ + boneIndex] = bones[boneIndex].second;
                    animationData.blendWeights_[vertexIndex * numBones_ + boneIndex] = bones[boneIndex].first;
                }
            }
            else
            {
                ea::sort(bones.begin(), bones.end(), ea::greater<>{});
                float totalWeight = 0.0f;
                for (unsigned boneIndex = 0; boneIndex < numBones_; ++boneIndex)
                    totalWeight += bones[boneIndex].first;

                for (unsigned boneIndex = 0; boneIndex < numBones_; ++boneIndex)
                {
                    animationData.blendIndices_[vertexIndex * numBones_ + boneIndex] = bones[boneIndex].second;
                    animationData.blendWeights_[vertexIndex * numBones_ + boneIndex] = bones[boneIndex].first / totalWeight;
                }
            }

            // Advance
            indicesData += originalVertexSize;
            weightsData += originalVertexSize;
        }
    }
}

void SoftwareModelAnimator::CopyMorphVertices(void* destVertexData, const void* srcVertexData, unsigned vertexCount,
    VertexBuffer* destBuffer, VertexBuffer* srcBuffer) const
{
    unsigned mask = destBuffer->GetElementMask() & srcBuffer->GetElementMask();
    unsigned normalOffset = srcBuffer->GetElementOffset(SEM_NORMAL);
    unsigned tangentOffset = srcBuffer->GetElementOffset(SEM_TANGENT);
    unsigned vertexSize = srcBuffer->GetVertexSize();
    auto dest = reinterpret_cast<float*>(destVertexData);
    auto src = reinterpret_cast<const unsigned char*>(srcVertexData);

    while (vertexCount--)
    {
        if (mask & MASK_POSITION)
        {
            memcpy(dest, src, sizeof(float) * 3);
            dest += 3;
        }
        if (mask & MASK_NORMAL)
        {
            memcpy(dest, src + normalOffset, sizeof(float) * 3);
            dest += 3;
        }
        if (mask & MASK_TANGENT)
        {
            memcpy(dest, src + tangentOffset, sizeof(float) * 4);
            dest += 4;
        }

        src += vertexSize;
    }
}

void SoftwareModelAnimator::ApplyMorph(VertexBuffer* buffer, const VertexBufferMorph& morph, float weight)
{
    const VertexMaskFlags elementMask = morph.elementMask_ & buffer->GetElementMask();
    unsigned vertexCount = morph.vertexCount_;
    unsigned normalOffset = buffer->GetElementOffset(SEM_NORMAL);
    unsigned tangentOffset = buffer->GetElementOffset(SEM_TANGENT);
    unsigned vertexSize = buffer->GetVertexSize();

    unsigned char* srcData = morph.morphData_.get();
    unsigned char* destData = buffer->GetShadowData();

    while (vertexCount--)
    {
        const unsigned vertexIndex = *reinterpret_cast<const unsigned*>(srcData);
        srcData += sizeof(unsigned);

        if (elementMask & MASK_POSITION)
        {
            auto dest = reinterpret_cast<float*>(destData + vertexIndex * vertexSize);
            auto src = reinterpret_cast<const float*>(srcData);
            dest[0] += src[0] * weight;
            dest[1] += src[1] * weight;
            dest[2] += src[2] * weight;
            srcData += 3 * sizeof(float);
        }
        if (elementMask & MASK_NORMAL)
        {
            auto dest = reinterpret_cast<float*>(destData + vertexIndex * vertexSize + normalOffset);
            auto src = reinterpret_cast<const float*>(srcData);
            dest[0] += src[0] * weight;
            dest[1] += src[1] * weight;
            dest[2] += src[2] * weight;
            srcData += 3 * sizeof(float);
        }
        if (elementMask & MASK_TANGENT)
        {
            auto dest = reinterpret_cast<float*>(destData + vertexIndex * vertexSize + tangentOffset);
            auto src = reinterpret_cast<const float*>(srcData);
            dest[0] += src[0] * weight;
            dest[1] += src[1] * weight;
            dest[2] += src[2] * weight;
            srcData += 3 * sizeof(float);
        }
    }
}

}
