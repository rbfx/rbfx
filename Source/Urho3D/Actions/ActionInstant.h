// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{
/// Finite time action.
class URHO3D_API ActionInstant : public FiniteTimeAction
{
    URHO3D_OBJECT(ActionInstant, FiniteTimeAction)

public:
    /// Construct.
    ActionInstant(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

} // namespace Actions

} // namespace Urho3D
