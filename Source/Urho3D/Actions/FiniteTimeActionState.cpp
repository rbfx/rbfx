// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "FiniteTimeActionState.h"

#include "../Core/Context.h"
#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{

/// Construct.
FiniteTimeActionState::FiniteTimeActionState(FiniteTimeAction* action, Object* target)
    : ActionState(action, target)
{
    duration_ = action->GetDuration();
}

/// Destruct.
FiniteTimeActionState::~FiniteTimeActionState() {}

void FiniteTimeActionState::Step(float dt)
{
    if (firstTick_)
    {
        firstTick_ = false;
        elapsed_ = 0.0f;
    }
    else
    {
        elapsed_ += dt;
    }

    Update(Clamp(elapsed_ / Max(duration_, ea::numeric_limits<float>::epsilon()), 0.0f, 1.0f));
}

SharedPtr<FiniteTimeActionState> FiniteTimeActionState::StartAction(FiniteTimeAction* action, Object* target) const
{
    if (action)
    {
        SharedPtr<FiniteTimeActionState> res;
        res.DynamicCast(ActionState::StartAction(action, target));
        return res;
    }
    return {};
}

} // namespace Actions
} // namespace Urho3D
