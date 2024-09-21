// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/AnimationState.h"
#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

class AnimationStateSource : public Component
{
    URHO3D_OBJECT(AnimationStateSource, Component);

public:
    AnimationStateSource(Context* context)
        : Component(context)
    {
    }

    /// Mark that animation state tracks are dirty and should be reconnected.
    /// Should be called on every substantial change in animated structure.
    virtual void MarkAnimationStateTracksDirty() = 0;
    /// Return animations states for AnimatedModel.
    const AnimationStateVector& GetAnimationStates() const { return animationStates_; }
    /// Return whether animation throttling is disabled.
    bool IsAnimationThrottlingAllowed() const { return !resetSkeleton_; }

protected:
    /// Animation states. Shared with AnimatedModel when possible.
    AnimationStateVector animationStates_;
    /// Whether skeleton is reset on every frame.
    bool resetSkeleton_{};
};

} // namespace Urho3D
