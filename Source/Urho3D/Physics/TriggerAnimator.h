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

#include "Urho3D/Scene/Component.h"
#include "Urho3D/Graphics/Animation.h"

#include <EASTL/hash_set.h>

namespace Urho3D
{
/// %Component that runs animation when triggered.
class URHO3D_API TriggerAnimator : public Component
{
    URHO3D_OBJECT(TriggerAnimator, Component)

public:
    /// Construct.
    explicit TriggerAnimator(Context* context);
    /// Destruct.
    ~TriggerAnimator() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set enter animation attribute.
    void SetEnterAnimationAttr(const ResourceRef& value);
    /// Return enter animation attribute.
    ResourceRef GetEnterAnimationAttr() const;
    /// Set enter animation.
    void SetEnterAnimation(Animation* value);
    /// Return enter animation.
    Animation* GetEnterAnimation() const { return enterAnimation_; }
    /// Set exit animation attribute.
    void SetExitAnimationAttr(const ResourceRef& value);
    /// Return exit animation attribute.
    ResourceRef GetExitAnimationAttr() const;
    /// Set exit animation.
    void SetExitAnimation(Animation* value);
    /// Return exit animation.
    Animation* GetExitAnimation() const { return exitAnimation_; }

    /// Executed when first compatible body enters the trigger.
    virtual void OnEnter();
    /// Executed when last compatible body leaves the trigger.
    virtual void OnExit();
    /// Filter entering node. Returns true if the trigger should react on the object.
    virtual bool Filter(Node* node);

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    void OnSetEnabled() override;
    void RegisterEnter(Node* node);
    void RegisterExit(Node* node);
    /// Handle trigger been entered.
    void HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData);
    /// Handle trigger been exited.
    void HandleNodeCollisionEnd(StringHash eventType, VariantMap& eventData);
    /// Update subscriptions.
    void UpdateSubscriptions();
    /// Start selected animation. The argument should be either enter or exit animation.
    void StartAnimation(Animation* animation);

private:
    /// Is subscribed to events.
    bool isSubscribed_{};
    /// Enter animation.
    SharedPtr<Animation> enterAnimation_;
    /// Exit animation.
    SharedPtr<Animation> exitAnimation_;
    /// Set of active collisions.
    ea::hash_set<Node*> activeCollisions_;
    /// Last known state: true in entered.
    bool isEntered_{};
};

}
