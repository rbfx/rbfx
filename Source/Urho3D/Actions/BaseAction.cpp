//
// Copyright (c) 2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "BaseAction.h"

#include "ActionManager.h"
#include "ActionState.h"
#include "FiniteTimeAction.h"

#include "../Core/Context.h"
#include "../IO/Archive.h"

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
