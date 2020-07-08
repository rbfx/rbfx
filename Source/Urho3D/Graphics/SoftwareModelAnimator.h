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

#pragma once

#include "../Graphics/Model.h"
#include "../Graphics/Skeleton.h"

#include <EASTL/span.h>

namespace Urho3D
{

/// Container for vertex buffer animation data.
struct VertexBufferAnimationData
{
    /// Whether the buffer is animated by skeleton.
    bool hasSkeletalAnimation_{};
    /// Whether the buffer has normals affected by skeletal animation.
    bool skinNormals_{};
    /// Whether the buffer has tangents affected by skeletal animation.
    bool skinTangents_{};
    /// Blend weights. Size is number of bones used times number of vertices.
    ea::vector<float> blendWeights_;
    /// Blend indices.
    ea::vector<unsigned char> blendIndices_;
};

/// Class for software model animation (morphing and skinning).
class URHO3D_API SoftwareModelAnimator : public Object
{
    URHO3D_OBJECT(SoftwareModelAnimator, Object);

public:
    /// Max number of bones.
    static const unsigned MaxBones = 4;

    /// Construct.
    explicit SoftwareModelAnimator(Context* context);
    /// Destruct.
    ~SoftwareModelAnimator() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Initialize with model. Shall be manually called on model reload.
    void Initialize(Model* model, bool skinned, unsigned numBones);

    /// Reset morph and/or skeletal animation. Safe to call from worker thread.
    void ResetAnimation();
    /// Apply morphs. Safe to call from worker thread.
    void ApplyMorphs(ea::span<const ModelMorph> morphs);
    /// Apply skinning.
    void ApplySkinning(ea::span<const Matrix3x4> worldTransforms);
    /// Commit data to GPU.
    void Commit();

    /// Return animated geometries.
    const ea::vector<ea::vector<SharedPtr<Geometry>>>& GetGeometries() const { return geometries_; }

    /// Return all cloned vertex buffers.
    const ea::vector<SharedPtr<VertexBuffer> >& GetVertexBuffers() const { return vertexBuffers_; }

private:
    /// Return morph mask.
    VertexMaskFlags GetMorphElementMask() const;
    /// Clone model geometries.
    void CloneModelGeometries();
    /// Initialize skeletal animation data.
    void InitializeAnimationData();
    /// Copy morph vertices.
    void CopyMorphVertices(void* destVertexData, const void* srcVertexData, unsigned vertexCount,
        VertexBuffer* destBuffer, VertexBuffer* srcBuffer) const;
    /// Apply a vertex buffer morph.
    void ApplyMorph(VertexBuffer* buffer, const VertexBufferMorph& morph, float weight);
    /// Apply skinning for given vertex buffer.
    template <bool SkinNormals, bool SkinTangents>
    void ApplyVertexBufferSkinning(VertexBuffer* clonedBuffer, const VertexBufferAnimationData& animationData,
        ea::span<const Matrix3x4> worldTransforms) const;

    /// Original model.
    SharedPtr<Model> originalModel_;
    /// Animated model vertex buffers.
    ea::vector<SharedPtr<VertexBuffer>> vertexBuffers_;
    /// Animated model geometries.
    ea::vector<ea::vector<SharedPtr<Geometry>>> geometries_;

    /// Whether CPU skinning is applied.
    bool skinned_{};
    /// Number of bones used for skeletal animation.
    unsigned numBones_{};
    /// Animation data for vertex buffers.
    ea::vector<VertexBufferAnimationData> vertexBuffersData_;
};

}
