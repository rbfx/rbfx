//
// Copyright (c) 2017-2021 the rbfx project.
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

#include "ModelUtils.h"

namespace Tests
{

ModelVertex MakeModelVertex(const Vector3& position, const Vector3& normal, const Color& color)
{
    ModelVertex vertex;
    vertex.SetPosition(position);
    vertex.SetNormal(normal);
    vertex.color_[0] = color.ToVector4();
    return vertex;
}

void AppendQuad(GeometryLODView& dest,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color)
{
    static constexpr unsigned NumVertices = 4;
    static constexpr unsigned NumIndices = 2 * 3;

    const Vector3 vertexPositions[NumVertices] = {
        { -size.x_ / 2, -size.y_ / 2, 0.0f },
        {  size.x_ / 2, -size.y_ / 2, 0.0f },
        { -size.x_ / 2,  size.y_ / 2, 0.0f },
        {  size.x_ / 2,  size.y_ / 2, 0.0f },
    };
    const Vector3 vertexNorm = { 0.0f, 0.0f, -1.0f };

    const unsigned baseIndex = dest.vertices_.size();
    for (unsigned i = 0; i < NumVertices; ++i)
    {
        const Vector3 pos = rotation * vertexPositions[i] + position;
        const Vector3 norm = rotation * vertexNorm;
        dest.vertices_.push_back(MakeModelVertex(pos, norm, color));
    }

    const unsigned indices[NumIndices] = {
        0, 2, 1,
        1, 2, 3
    };
    for (unsigned i = 0; i < NumIndices; ++i)
        dest.indices_.push_back(baseIndex + indices[i]);
}

void AppendSkinnedQuad(GeometryLODView& dest, const Vector4& blendIndices, const Vector4& blendWeights,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color)
{
    const unsigned beginVertex = dest.vertices_.size();
    AppendQuad(dest, position, rotation, size, color);
    const unsigned endVertex = dest.vertices_.size();

    for (unsigned i = beginVertex; i < endVertex; ++i)
    {
        dest.vertices_[i].blendIndices_ = blendIndices;
        dest.vertices_[i].blendWeights_ = blendWeights;
    }
}

AnimationKeyFrame MakeTranslationKeyFrame(float time, const Vector3& position)
{
    AnimationKeyFrame keyFrame;
    keyFrame.time_ = time;
    keyFrame.position_ = position;
    return keyFrame;
}

AnimationKeyFrame MakeRotationKeyFrame(float time, const Quaternion& rotation)
{
    AnimationKeyFrame keyFrame;
    keyFrame.time_ = time;
    keyFrame.rotation_ = rotation;
    return keyFrame;
}

SharedPtr<ModelView> CreateSkinnedQuad_Model(Context* context)
{
    auto modelView = MakeShared<ModelView>(context);

    // Set vertex format
    ModelVertexFormat format;
    format.position_ = TYPE_VECTOR3;
    format.normal_ = TYPE_VECTOR3;
    format.color_[0] = TYPE_UBYTE4_NORM;
    format.blendIndices_ = TYPE_UBYTE4;
    format.blendWeights_ = TYPE_UBYTE4_NORM;
    modelView->SetVertexFormat(format);

    // Create geometry
    auto& geometries = modelView->GetGeometries();
    geometries.resize(1);
    geometries[0].lods_.resize(1);
    auto& geometry = geometries[0].lods_[0];

    // Create bones
    auto& bones = modelView->GetBones();
    bones.resize(3);

    bones[0].name_ = "Root";
    bones[0].parentIndex_ = M_MAX_UNSIGNED;

    bones[1].name_ = "Quad 1";
    bones[1].parentIndex_ = 0;
    bones[1].SetInitialTransform({ 0.0f, 0.0f, 0.0f });
    bones[1].SetLocalBoundingBox({ Vector3{ -0.5f, 0.0f, 0.0f }, Vector3{ 0.5f, 1.0f, 0.0f } });
    bones[1].RecalculateOffsetMatrix();

    bones[2].name_ = "Quad 2";
    bones[2].parentIndex_ = 1;
    bones[2].SetInitialTransform({ 0.0f, 1.0f, 0.0f });
    bones[2].SetLocalBoundingBox({ Vector3{ -0.5f, 0.0f, 0.0f }, Vector3{ 0.5f, 1.0f, 0.0f } });
    bones[2].RecalculateOffsetMatrix();

    // Set geometry data
    AppendSkinnedQuad(geometry, { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.5f, 0.0f }, { 0.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::WHITE);
    AppendSkinnedQuad(geometry, { 0.0f, 2.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.5f, 0.0f }, { 0.0f, Vector3::UP }, { 1.0f, 1.0f }, Color::WHITE);

    return modelView;
}

SharedPtr<Animation> CreateSkinnedQuad_Animation_2TX(Context* context)
{
    auto animation = MakeShared<Animation>(context);
    animation->SetName("@/Models/SkinnedQuad_2TX.ani");
    animation->SetLength(2.0f);

    auto track = animation->CreateTrack("Quad 2");
    track->channelMask_ = CHANNEL_POSITION;

    track->AddKeyFrame(MakeTranslationKeyFrame(0.0f, { 0.0f, 1.0f, 0.0f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(0.5f, { -0.5f, 1.0f, 0.0f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(1.5f, { 0.5f, 1.0f, 0.0f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(2.0f, { 0.0f, 1.0f, 0.0f }));

    return animation;
}

SharedPtr<Animation> CreateSkinnedQuad_Animation_2TZ(Context* context)
{
    auto animation = MakeShared<Animation>(context);
    animation->SetName("@/Models/SkinnedQuad_2TZ.ani");
    animation->SetLength(2.0f);

    auto track = animation->CreateTrack("Quad 2");
    track->channelMask_ = CHANNEL_POSITION;

    track->AddKeyFrame(MakeTranslationKeyFrame(0.0f, { 0.0f, 1.0f, 0.0f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(0.5f, { 0.0f, 1.0f, -0.5f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(1.5f, { 0.0f, 1.0f, 0.5f }));
    track->AddKeyFrame(MakeTranslationKeyFrame(2.0f, { 0.0f, 1.0f, 0.0f }));

    return animation;
}

SharedPtr<Animation> CreateSkinnedQuad_Animation_1RY(Context* context)
{
    auto animation = MakeShared<Animation>(context);
    animation->SetName("@/Models/SkinnedQuad_1RY.ani");
    animation->SetLength(1.0f);

    auto track = animation->CreateTrack("Quad 1");
    track->channelMask_ = CHANNEL_ROTATION;

    track->AddKeyFrame(MakeRotationKeyFrame(0.0f, { 0.0f, Vector3::UP }));
    track->AddKeyFrame(MakeRotationKeyFrame(0.25f, { 90.0f, Vector3::UP }));
    track->AddKeyFrame(MakeRotationKeyFrame(0.5f, { 180.0f, Vector3::UP }));
    track->AddKeyFrame(MakeRotationKeyFrame(0.75f, { -90.0f, Vector3::UP }));
    track->AddKeyFrame(MakeRotationKeyFrame(1.0f, { 0.0f, Vector3::UP }));

    return animation;
}
}
