// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ActionState.h"

namespace Urho3D
{
class Object;

namespace Actions
{
class FiniteTimeAction;

/// Finite time action state.
class URHO3D_API FiniteTimeActionState : public ActionState
{
public:
    /// Construct.
    FiniteTimeActionState(FiniteTimeAction* action, Object* target);
    /// Destruct.
    ~FiniteTimeActionState() override;

    /// Gets a value indicating whether this instance is done.
    bool IsDone() const override { return elapsed_ >= duration_; }

    /// Called every frame with it's delta time.
    void Step(float dt) override;

    /// Get action duration.
    float GetDuration() const { return duration_; }
    /// Get action elapsed time.
    float GetElapsed() const { return elapsed_; }

protected:
    /// Call StartAction on an action.
    SharedPtr<FiniteTimeActionState> StartAction(FiniteTimeAction* action, Object* target) const;

private:
    float duration_{};
    float elapsed_{};
    bool firstTick_{true};
};

} // namespace Actions
} // namespace Urho3D
