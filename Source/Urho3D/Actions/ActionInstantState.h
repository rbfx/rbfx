// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "FiniteTimeActionState.h"

namespace Urho3D
{
class Object;

namespace Actions
{
class ActionInstant;

/// Finite time action state.
class URHO3D_API ActionInstantState : public FiniteTimeActionState
{
public:
    /// Construct.
    ActionInstantState(ActionInstant* action, Object* target);
    /// Destruct.
    ~ActionInstantState() override;

    /// Gets a value indicating whether this instance is done.
    bool IsDone() const override;

    /// Called every frame with it's delta time.
    void Step(float dt) override;
};

} // namespace Actions
} // namespace Urho3D
