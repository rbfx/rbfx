//
// Copyright (c) 2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Sequence.h"

#include "ActionBuilder.h"
#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace
{
struct SequenceState : public FiniteTimeActionState
{
    SequenceState(Sequence* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        innerActionStates_[0].DynamicCast(StartAction(action->GetFirstAction(), target));
        innerActionStates_[1].DynamicCast(StartAction(action->GetSecondAction(), target));
    }

    ea::array<SharedPtr<FiniteTimeAction>, 2> innerActionStates_;
};

} // namespace

/// Construct.
Sequence::Sequence(Context* context)
    : BaseClassName(context)
{
}

/// Set first action in sequence.
void Sequence::SetFirstAction(FiniteTimeAction* action) { actions_[0] = action; }
/// Set second action in sequence.
void Sequence::SetSecondAction(FiniteTimeAction* action) { actions_[1] = action; }

/// Create reversed action.
SharedPtr<FiniteTimeAction> Sequence::Reverse() const
{
    auto result = MakeShared<Sequence>(context_);
    result->SetFirstAction(GetSecondAction());
    result->SetSecondAction(GetFirstAction());
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void Sequence::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeValue(archive, "first", actions_[0]);
    SerializeValue(archive, "second", actions_[1]);
}

/// Create new action state from the action.
SharedPtr<ActionState> Sequence::StartAction(Object* target) { return MakeShared<SequenceState>(this, target); }

} // namespace Urho3D
