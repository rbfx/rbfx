// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/ModelView.h>

using namespace Urho3D;

namespace Tests
{

/// Create simple primitives for ModelView
/// @{
ModelVertexFormat GetVertexFormat();
ModelVertex MakeModelVertex(const Vector3& position, const Vector3& normal, const Color& color);
void AppendQuad(GeometryLODView& dest,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color);
void AppendSkinnedQuad(GeometryLODView& dest, const Vector4& blendIndices, const Vector4& blendWeights,
    const Vector3& position, const Quaternion& rotation, const Vector2& size, const Color& color);
/// @}

/// Create simple primitives for Animation
/// @{
AnimationKeyFrame MakeTranslationKeyFrame(float time, const Vector3& position);
AnimationKeyFrame MakeRotationKeyFrame(float time, const Quaternion& rotation);
SharedPtr<Animation> CreateLoopedTranslationAnimation(Context* context,
    const ea::string& animationName, const ea::string& boneName,
    const Vector3& origin, const Vector3& magnitude, float duration);
SharedPtr<Animation> CreateLoopedRotationAnimation(Context* context,
    const ea::string& animationName, const ea::string& boneName,
    const Vector3& axis, float duration);
SharedPtr<Animation> CreateCombinedAnimation(Context* context,
    const ea::string& animationName, std::initializer_list<Animation*> animations);
/// @}

/// Create test skinned model:
/// 0: Root bone w/o any geometry;
/// |-1: First 1x1 quad at Y=0.5;
///   |-2: Second 1x1 quad at Y=1.5.
SharedPtr<ModelView> CreateSkinnedQuad_Model(Context* context);

}
