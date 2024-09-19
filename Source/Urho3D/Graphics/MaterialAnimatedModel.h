////
//// Copyright (c) 2023-2023 the rbfx project.
////
//// Permission is hereby granted, free of charge, to any person obtaining a copy
//// of this software and associated documentation files (the "Software"), to deal
//// in the Software without restriction, including without limitation the rights
//// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//// copies of the Software, and to permit persons to whom the Software is
//// furnished to do so, subject to the following conditions:
////
//// The above copyright notice and this permission notice shall be included in
//// all copies or substantial portions of the Software.
////
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//// THE SOFTWARE.
////
//
//#pragma once
//
//#include "../Graphics/StaticModel.h"
//
//namespace Urho3D
//{
//
///// Renders and animates model via updating material parameters.
//class URHO3D_API MaterialAnimatedModel : public StaticModel
//{
//    URHO3D_OBJECT(MaterialAnimatedModel, StaticModel);
//
//public:
//    /// Construct.
//    explicit MaterialAnimatedModel(Context* context);
//    /// Destruct.
//    ~MaterialAnimatedModel() override;
//    /// Register object factory. StaticModel must be registered first.
//    /// @nobind
//    static void RegisterObject(Context* context);
//
//    void OnSceneSet(Scene* scene) override;
//
//    /// Set to remove either the emitter component or its owner node from the scene automatically on particle effect
//    /// completion. Disabled by default.
//    void SetAutoRemoveMode(AutoRemoveMode mode);
//    /// Set loop on/off. If loop is enabled, sets the full sound as loop range.
//    void SetLooped(bool enable);
//
//    /// Return automatic removal mode on particle effect completion.
//    AutoRemoveMode GetAutoRemoveMode() const { return autoRemove_; }
//    /// Return whether is looped.
//    bool IsLooped() const { return looped_; }
//
//    /// Animate material parameter.
//    void Play(const ea::string& shaderParameter, float length, const Variant& from, const Variant& to);
//
//private:
//    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
//
//    /// Name of the shader parameter to animate.
//    ea::string shaderParameter_{};
//    /// Length of animation in seconds.
//    float animationLength_{1.0f};
//    /// Elapsed time in seconds.
//    float elapsedTime_{};
//    /// Shader parameter start value.
//    Variant from_{0.0f};
//    /// Shader parameter end value.
//    Variant to_{1.0f};
//    /// Automatic removal mode.
//    AutoRemoveMode autoRemove_{REMOVE_DISABLED};
//    /// Looped flag.
//    bool looped_{true};
//};
//
//} // namespace Urho3D
