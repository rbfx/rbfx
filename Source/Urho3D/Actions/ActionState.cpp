// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "ActionState.h"
#include "BaseAction.h"

#include "../Core/Context.h"

namespace Urho3D
{
namespace Actions
{

ActionState::ActionState(BaseAction* action, Object* target)
    : _action(action)
    , _target(target)
    , _originalTarget(target)
{
}

void ActionState::Update(float time) {}

void ActionState::Stop() { _target.Reset(); }

void ActionState::Step(float dt) {}

SharedPtr<ActionState> ActionState::StartAction(BaseAction* action, Object* target) const
{
    if (action)
        return action->StartAction(target);
    return {};
}

} // namespace Actions
} // namespace Urho3D
