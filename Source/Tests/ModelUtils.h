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

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/ModelView.h>

using namespace Urho3D;

namespace Tests
{

/// Create simple primitives for ModelView
/// @{
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
