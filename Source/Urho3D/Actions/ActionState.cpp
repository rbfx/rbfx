// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/ActionState.h"
#include "Urho3D/Actions/BaseAction.h"

#include "Urho3D/Core/Context.h"

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
