// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/FiniteTimeAction.h"

#include "Urho3D/Actions/ActionManager.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
/// "No-operation" finite time action for irreversible actions.
class NoAction : public FiniteTimeAction
{
public:
    /// Construct.
    NoAction(Context* context, FiniteTimeAction* reversed);
    /// Destruct.
    ~NoAction() override;
    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

private:
    SharedPtr<FiniteTimeAction> reversed_;
};
} // namespace

/// Construct.
FiniteTimeAction::FiniteTimeAction(Context* context)
    : BaseClassName(context)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void FiniteTimeAction::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "duration", duration_, ea::numeric_limits<float>::epsilon());
}

/// Get action from argument or empty action.
FiniteTimeAction* FiniteTimeAction::GetOrDefault(FiniteTimeAction* action) const
{
    if (action)
        return action;
    return context_->GetSubsystem<ActionManager>()->GetEmptyAction();
}

float FiniteTimeAction::GetDuration() const { return duration_; }

void FiniteTimeAction::SetDuration(float duration)
{
    // Prevent division by 0
    if (duration < ea::numeric_limits<float>::epsilon())
        duration = ea::numeric_limits<float>::epsilon();

    duration_ = duration;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> FiniteTimeAction::Reverse() const
{
    return MakeShared<NoAction>(context_, const_cast<FiniteTimeAction*>(this));
}

/// Construct.
NoAction::NoAction(Context* context, FiniteTimeAction* reversed)
    : FiniteTimeAction(context)
    , reversed_(reversed)
{
}

/// Destruct.
NoAction::~NoAction() {}

/// Create reversed action.
SharedPtr<FiniteTimeAction> NoAction::Reverse() const { return reversed_; }

} // namespace Actions
} // namespace Urho3D
