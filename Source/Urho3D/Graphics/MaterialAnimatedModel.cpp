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
//#include "../Precompiled.h"
//
//#include "../Core/Context.h"
//#include "../Graphics/MaterialAnimatedModel.h"
//
//#include "Material.h"
//#include "Urho3D/IO/Log.h"
//#include "Urho3D/Scene/SceneEvents.h"
//
//namespace Urho3D
//{
//
//MaterialAnimatedModel::MaterialAnimatedModel(Context* context)
//    : BaseClassName(context)
//{
//}
//
//MaterialAnimatedModel::~MaterialAnimatedModel()
//{
//}
//
//void MaterialAnimatedModel::RegisterObject(Context* context)
//{
//    context->AddFactoryReflection<MaterialAnimatedModel>(Category_Geometry);
//
//    URHO3D_COPY_BASE_ATTRIBUTES(StaticModel);
//    URHO3D_ACCESSOR_ATTRIBUTE("Is Looped", IsLooped, SetLooped, bool, true, AM_DEFAULT);
//    URHO3D_ACCESSOR_ATTRIBUTE("Length", GetAnimationLength, SetAnimationLength, float, 1.0f, AM_DEFAULT);
//    URHO3D_ACCESSOR_ATTRIBUTE("Elapsed Time", GetElapsedTime, SetElapsedTime, float, 0.0f, AM_DEFAULT);
//    //URHO3D_ATTRIBUTE("From", from_, Variant, 0.0f, AM_DEFAULT);
//}
//
//void MaterialAnimatedModel::OnSceneSet(Scene* scene)
//{
//    if (scene)
//    {
//        SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(MaterialAnimatedModel, HandleSceneUpdate));
//    }
//    else
//    {
//        UnsubscribeFromEvent(E_SCENEUPDATE);
//    }
//}
//
//void MaterialAnimatedModel::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
//{
//    using namespace SceneUpdate;
//
//    const auto timeStep = eventData[P_TIMESTEP].GetFloat();
//    elapsedTime_ += timeStep;
//    if (elapsedTime_ >= animationLength_)
//    {
//        if (animationLength_ > M_EPSILON)
//            elapsedTime_ = Floor(elapsedTime_ / animationLength_) * animationLength_;
//        else
//            elapsedTime_ = 0.0f;
//        DoAutoRemove(autoRemove_);
//    }
//
//    for (auto& batch: batches_)
//    {
//        if (batch.material_)
//        {
//            batch.material_->SetShaderParameterAnimation()
//        }
//    }
//}
//
//void MaterialAnimatedModel::SetAutoRemoveMode(AutoRemoveMode mode)
//{
//    autoRemove_ = mode;
//}
//
//void MaterialAnimatedModel::SetLooped(bool enable)
//{
//    looped_ = enable;
//}
//
//void MaterialAnimatedModel::Play(const ea::string& shaderParameter, float length, const Variant& from, const Variant& to)
//{
//    shaderParameter_ = shaderParameter;
//    animationLength_ = length;
//    elapsedTime_ = 0;
//    from_ = from;
//    to_ = to;
//    if (from_.GetType() != to_.GetType())
//    {
//        URHO3D_LOGERROR("Animation values type mismatch");
//    }
//}
//
//}
