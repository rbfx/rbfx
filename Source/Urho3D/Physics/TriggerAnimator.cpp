//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Physics/PhysicsEvents.h"
#include "Urho3D/Physics/TriggerAnimator.h"

#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Resource/ResourceCache.h"

namespace Urho3D
{

TriggerAnimator::TriggerAnimator(Context* context)
    : BaseClassName(context)
{
}

TriggerAnimator::~TriggerAnimator()
{
}

void TriggerAnimator::RegisterObject(Context* context)
{
    context->AddFactoryReflection<TriggerAnimator>(Category_Physics);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Enter Animation", GetEnterAnimationAttr, SetEnterAnimationAttr, ResourceRef,
        ResourceRef(Animation::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Exit Animation", GetExitAnimationAttr, SetExitAnimationAttr, ResourceRef,
        ResourceRef(Animation::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Entered", bool, isEntered_, false, AM_DEFAULT);
    
}

void TriggerAnimator::SetEnterAnimationAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    enterAnimation_ = cache->GetResource<Animation>(value.name_);
}

ResourceRef TriggerAnimator::GetEnterAnimationAttr() const
{
    return GetResourceRef(enterAnimation_, Animation::GetTypeStatic());
}

void TriggerAnimator::SetEnterAnimation(Animation* value)
{
    enterAnimation_ = value;
}

void TriggerAnimator::SetExitAnimationAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    exitAnimation_ = cache->GetResource<Animation>(value.name_);
}

ResourceRef TriggerAnimator::GetExitAnimationAttr() const
{
    return GetResourceRef(exitAnimation_, Animation::GetTypeStatic());
}

void TriggerAnimator::SetExitAnimation(Animation* value)
{
    exitAnimation_ = value;
}

void TriggerAnimator::OnEnter()
{
}

void TriggerAnimator::OnExit()
{
}

bool TriggerAnimator::Filter(Node* node)
{
    return true;
}

void TriggerAnimator::OnNodeSet(Node* previousNode, Node* currentNode)
{
    Component::OnNodeSet(previousNode, currentNode);
    UpdateSubscriptions();
}

void TriggerAnimator::OnSetEnabled()
{
    Component::OnSetEnabled();
    UpdateSubscriptions();
}

void TriggerAnimator::RegisterEnter(Node* node)
{
    if (!node)
        return;

    if (!Filter(node))
        return;

    if (activeCollisions_.empty())
    {
        if (!isEntered_)
        {
            isEntered_ = true;
            StartAnimation(enterAnimation_);
            OnEnter();
        }
    }
    activeCollisions_.insert(node);
}

void TriggerAnimator::RegisterExit(Node* node)
{
    if (!node)
        return;
    activeCollisions_.erase(node);
    if (activeCollisions_.empty())
    {
        if (isEntered_)
        {
            isEntered_ = false;
            StartAnimation(exitAnimation_);
            OnExit();
        }
    }
}

void TriggerAnimator::HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollisionStart;

    Node* other = static_cast<Node*>(eventData[P_OTHERNODE].GetVoidPtr());
    RegisterEnter(other);
}

void TriggerAnimator::HandleNodeCollisionEnd(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollisionEnd;

    const auto other = static_cast<Node*>(eventData[P_OTHERNODE].GetVoidPtr());
    RegisterExit(other);
}

void TriggerAnimator::UpdateSubscriptions()
{
    const bool subscribe = GetNode() != nullptr && IsEnabledEffective();
    if (subscribe != isSubscribed_)
    {
        isSubscribed_ = subscribe;
        if (isSubscribed_)
        {
            SubscribeToEvent(GetNode(), E_NODECOLLISIONSTART, URHO3D_HANDLER(TriggerAnimator, HandleNodeCollisionStart));
            SubscribeToEvent(GetNode(), E_NODECOLLISIONEND, URHO3D_HANDLER(TriggerAnimator, HandleNodeCollisionEnd));
        }
        else
        {
            UnsubscribeFromEvent(E_PHYSICSCOLLISIONSTART);
            UnsubscribeFromEvent(E_PHYSICSCOLLISIONEND);
        }
    }
}

void TriggerAnimator::StartAnimation(Animation* animation)
{
    if (!animation)
        return;

    const auto animationController = GetNode()->GetComponent<AnimationController>();
    if (!animationController)
    {
        URHO3D_LOGERROR("TriggerAnimator can't start animation: no AnimationController found");
        return;
    }

    AnimationParameters animationParameters{animation};
    animationParameters.removeOnZeroWeight_ = true;

    if (animationController->GetNumAnimations() > 0)
    {
        auto otherAnimation = (animation == enterAnimation_) ? exitAnimation_ : enterAnimation_;
        auto existingAnimationIndex = animationController->FindLastAnimation(otherAnimation);
        if (existingAnimationIndex != M_MAX_UNSIGNED)
        {
            auto state = animationController->GetAnimationParameters(existingAnimationIndex);
            auto time = state.time_.Value() / (state.animation_->GetLength()+ea::numeric_limits<float>::epsilon());

            animationParameters.Time((1.0f - time) * animation->GetLength());
        }
    }
    animationController->PlayExistingExclusive(animationParameters);
}

} // namespace Urho3D
