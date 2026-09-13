// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Actions/FiniteTimeAction.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
namespace Actions
{

/// Set of actions to be executed in parallel.
class URHO3D_API Parallel : public FiniteTimeAction
{
    URHO3D_OBJECT(Parallel, FiniteTimeAction)
public:
    /// Construct.
    explicit Parallel(Context* context);

    /// Set number of actions.
    void SetNumActions(unsigned num);
    /// Set number of actions.
    unsigned GetNumActions() const { return actions_.size(); };
    /// Set action by index.
    void SetAction(unsigned index, FiniteTimeAction* action);
    /// Add action.
    void AddAction(FiniteTimeAction* action);

    /// Get action duration.
    float GetDuration() const override;

    /// Get action by index.
    FiniteTimeAction* GetAction(unsigned index) const;

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    ea::fixed_vector<SharedPtr<FiniteTimeAction>, 4> actions_;
};

} // namespace Actions
} // namespace Urho3D
