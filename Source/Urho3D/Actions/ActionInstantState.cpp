// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "ActionInstantState.h"

#include "../Core/Context.h"
#include "ActionInstant.h"

namespace Urho3D
{
namespace Actions
{

/// Construct.
ActionInstantState::ActionInstantState(ActionInstant* action, Object* target)
    : FiniteTimeActionState(action, target)
{
}

/// Destruct.
ActionInstantState::~ActionInstantState() {}

bool ActionInstantState::IsDone() const { return true; }

void ActionInstantState::Step(float dt) { Update(1.0f); }

} // namespace Actions
} // namespace Urho3D
