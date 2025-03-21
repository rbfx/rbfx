//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include "../Urho2D/StaticSprite2D.h"

#ifdef URHO3D_SPINE
struct spAnimationState;
struct spAnimationStateData;
struct spSkeleton;
#endif

namespace Urho3D
{

/// Loop mode.
enum LoopMode2D
{
    /// Default, use animation's value.
    LM_DEFAULT = 0,
    /// Force looped.
    LM_FORCE_LOOPED,
    /// Force clamped.
    LM_FORCE_CLAMPED
};

namespace Spriter
{
    class SpriterInstance;
}

class AnimationSet2D;

/// Animated sprite component, it uses to play animation created by Spine (http://www.esotericsoftware.com) and Spriter (http://www.brashmonkey.com/).
class URHO3D_API AnimatedSprite2D : public StaticSprite2D
{
    URHO3D_OBJECT(AnimatedSprite2D, StaticSprite2D);

public:
    /// Construct.
    explicit AnimatedSprite2D(Context* context);
    /// Destruct.
    ~AnimatedSprite2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set animation set.
    /// @property
    void SetAnimationSet(AnimationSet2D* animationSet);
    /// Set entity name (skin name for spine, entity name for spriter).
    /// @property
    void SetEntity(const ea::string& entity);
    /// Set animation by name and loop mode.
    void SetAnimation(const ea::string& name, LoopMode2D loopMode = LM_DEFAULT);
    /// Set loop mode.
    /// @property
    void SetLoopMode(LoopMode2D loopMode);
    /// Set speed.
    /// @property
    void SetSpeed(float speed);

    /// Return animation.
    /// @property
    AnimationSet2D* GetAnimationSet() const;
    /// Return entity name.
    /// @property
    const ea::string& GetEntity() const { return entity_; }
    /// Return animation name.
    /// @property
    const ea::string& GetAnimation() const { return animationName_; }
    /// Return loop mode.
    /// @property
    LoopMode2D GetLoopMode() const { return loopMode_; }
    /// Return speed.
    /// @property
    float GetSpeed() const { return speed_; }

    /// Set animation set attribute.
    void SetAnimationSetAttr(const ResourceRef& value);
    /// Return animation set attribute.
    ResourceRef GetAnimationSetAttr() const;
    /// Set animation by name.
    /// @property{set_animation}
    void SetAnimationAttr(const ea::string& name);

protected:
    /// Handle scene being assigned.
    void OnSceneSet(Scene* previousScene, Scene* scene) override;
    /// Handle update vertices.
    void UpdateSourceBatches() override;
    /// Handle scene post update.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Update animation.
    void UpdateAnimation(float timeStep);
#ifdef URHO3D_SPINE
    /// Handle set spine animation.
    void SetSpineAnimation();
    /// Update spine animation.
    void UpdateSpineAnimation(float timeStep);
    /// Update vertices for spine animation.
    void UpdateSourceBatchesSpine();
#endif
    /// Handle set spriter animation.
    void SetSpriterAnimation();
    /// Update spriter animation.
    void UpdateSpriterAnimation(float timeStep);
    /// Update vertices for spriter animation.
    void UpdateSourceBatchesSpriter();
    /// Dispose.
    void Dispose();

    /// Speed.
    float speed_;
    /// Entity name.
    ea::string entity_;
    /// Animation set.
    SharedPtr<AnimationSet2D> animationSet_;
    /// Animation name.
    ea::string animationName_;
    /// Loop mode.
    LoopMode2D loopMode_;

#ifdef URHO3D_SPINE
    /// Skeleton.
    spSkeleton* skeleton_;
    /// Animation state data.
    spAnimationStateData* animationStateData_;
    /// Animation state.
    spAnimationState* animationState_;
#endif

    /// Spriter instance.
    ea::unique_ptr<Spriter::SpriterInstance> spriterInstance_;
};

}
