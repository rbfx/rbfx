// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "ActionInstant.h"
#include "ActionInstantState.h"

#include "../IO/ArchiveSerializationBasic.h"
#include "../Core/Context.h"

namespace Urho3D
{
namespace Actions
{

/// Construct.
ActionInstant::ActionInstant(Context* context)
    : BaseClassName(context)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void ActionInstant::SerializeInBlock(Archive& archive)
{
    // Skip FiniteTimeAction::SerializeInBlock because duration is always 0
    BaseAction::SerializeInBlock(archive);
}

/// Get action duration.
float ActionInstant::GetDuration() const { return ea::numeric_limits<float>::epsilon(); }

/// Create reversed action.
SharedPtr<FiniteTimeAction> ActionInstant::Reverse() const
{
    return MakeShared<ActionInstant>(context_);
}

/// Create new action state from the action.
SharedPtr<ActionState> ActionInstant::StartAction(Object* target)
{
    return MakeShared<ActionInstantState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
