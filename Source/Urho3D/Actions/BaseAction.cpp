// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/BaseAction.h"

#include "Urho3D/Actions/ActionManager.h"
#include "Urho3D/Actions/ActionState.h"
#include "Urho3D/Actions/FiniteTimeAction.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Archive.h"

namespace Urho3D
{
namespace Actions
{
namespace
{
struct NoActionState : public ActionState
{
    NoActionState(BaseAction* action, Object* target)
        : ActionState(action, target)
    {
    }
};

} // namespace
/// Construct.
BaseAction::BaseAction(Context* context)
    : Serializable(context)
{
}

/// Destruct.
BaseAction::~BaseAction() {}

/// Get action from argument or empty action.
BaseAction* BaseAction::GetOrDefault(BaseAction* action) const
{
    if (action)
        return action;
    return context_->GetSubsystem<Urho3D::ActionManager>()->GetEmptyAction();
}

/// Serialize content from/to archive. May throw ArchiveException.
void BaseAction::SerializeInBlock(Archive& archive) {}

/// Create new action state from the action.
SharedPtr<ActionState> BaseAction::StartAction(Object* target) { return MakeShared<NoActionState>(this, target); }

} // namespace Actions
} // namespace Urho3D
