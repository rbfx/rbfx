// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
