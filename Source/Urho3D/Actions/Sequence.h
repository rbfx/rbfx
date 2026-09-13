// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "FiniteTimeAction.h"
#include <EASTL/array.h>

namespace Urho3D
{
namespace Actions
{

/// Sequence of actions
class URHO3D_API Sequence : public FiniteTimeAction
{
    URHO3D_OBJECT(Sequence, FiniteTimeAction)
public:
    /// Construct.
    explicit Sequence(Context* context);

    /// Set first action in sequence.
    void SetFirstAction(FiniteTimeAction* action);
    /// Set second action in sequence.
    void SetSecondAction(FiniteTimeAction* action);

    /// Get action duration.
    float GetDuration() const override;
    /// Get first action.
    FiniteTimeAction* GetFirstAction() const { return actions_[0]; }
    /// Get second action.
    FiniteTimeAction* GetSecondAction() const { return actions_[1]; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    ea::array<SharedPtr<FiniteTimeAction>, 2> actions_;
};

} // namespace Actions
} // namespace Urho3D
