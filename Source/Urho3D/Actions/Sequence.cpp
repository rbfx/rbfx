//
// Copyright (c) 2015 Xamarin Inc.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Sequence.h"

#include "ActionBuilder.h"
#include "ActionManager.h"
#include "Repeat.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/Resource/GraphNode.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
struct SequenceState : public FiniteTimeActionState
{
    SequenceState(Sequence* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        innerActions_[0] = action->GetFirstAction();
        innerActions_[1] = action->GetSecondAction();
        hasInfiniteAction_ = innerActions_[0]->IsInstanceOf(RepeatForever::GetTypeStatic())
            || innerActions_[1]->IsInstanceOf(RepeatForever::GetTypeStatic());
        split_ = innerActions_[0]->GetDuration() / GetDuration();
    }

    /// Gets a value indicating whether this instance is done.
    bool IsDone() const override
    {
        if (hasInfiniteAction_ && innerActions_[last]->IsInstanceOf(RepeatForever::GetTypeStatic()))
            return false;
        return FiniteTimeActionState::IsDone();
    }

    void Update(float time) override
    {
        int found {0};
        float new_t {0.0f};

        if (time < split_)
        {
            // action[0]
            found = 0;
            if (split_ != 0)
                new_t = time / split_;
            else
                new_t = 1;
        }
        else
        {
            // action[1]
            found = 1;
            if (split_ == 1)
                new_t = 1;
            else
                new_t = (time - split_) / (1 - split_);
        }

        if (found == 1)
        {
            if (last == -1)
            {
                // action[0] was skipped, execute it.
                innerActionStates_[0].DynamicCast(StartAction(innerActions_[0], GetTarget()));
                innerActionStates_[0]->Update(1.0f);
                innerActionStates_[0]->Stop();
            }
            else if (last == 0)
            {
                innerActionStates_[0]->Update(1.0f);
                innerActionStates_[0]->Stop();
            }
        }
        else if (found == 0 && last == 1)
        {
            // Reverse mode ?
            // XXX: Bug. this case doesn't contemplate when _last==-1, found=0 and in "reverse mode"
            // since it will require a hack to know if an action is on reverse mode or not.
            // "step" should be overriden, and the "reverseMode" value propagated to inner Sequences.
            innerActionStates_[1]->Update(0);
            innerActionStates_[1]->Stop();
        }

        // Last action found and it is done.
        if (found == last && innerActionStates_[found]->IsDone())
            return;

        // Last action found and it is done
        if (found != last)
            innerActionStates_[found].DynamicCast(StartAction(innerActions_[found], GetTarget()));

        innerActionStates_[found]->Update(new_t);
        last = found;
    }

    void Stop() override
    {
        // Issue #1305
        if (last != -1)
            innerActionStates_[last]->Stop();
    }

    void Step(float dt) override
    {
        if (last > -1 && innerActions_[last]->IsInstanceOf(RepeatForever::GetTypeStatic()))
            innerActionStates_[last]->Step(dt);
        else
            FiniteTimeActionState::Step(dt);
    }

    ea::array<SharedPtr<FiniteTimeActionState>, 2> innerActionStates_;
    ea::array<SharedPtr<FiniteTimeAction>, 2> innerActions_;

    bool hasInfiniteAction_{false};
    float split_{};
    int last{-1};
};

} // namespace

/// Construct.
Sequence::Sequence(Context* context)
    : BaseClassName(context)
{
    actions_[0] = GetOrDefault(nullptr);
    actions_[1] = GetOrDefault(nullptr);
}

/// Set first action in sequence.
void Sequence::SetFirstAction(FiniteTimeAction* action)
{
    actions_[0] = GetOrDefault(action);
    SetDuration(GetDuration());
}
/// Set second action in sequence.
void Sequence::SetSecondAction(FiniteTimeAction* action)
{
    actions_[1] = GetOrDefault(action);
    SetDuration(GetDuration());
}

/// Get action duration.
float Sequence::GetDuration() const { return actions_[0]->GetDuration() + actions_[1]->GetDuration(); }

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

GraphNode* Sequence::ToGraphNode(Graph* graph) const
{
    const auto node = BaseClassName::ToGraphNode(graph);
    const auto first = node->GetOrAddExit("first");
    if (actions_[0])
    {
        auto* target = actions_[0]->ToGraphNode(graph);
        first.GetPin()->ConnectTo(target->GetEnter(0));
    }
    const auto second = node->GetOrAddExit("second");
    if (actions_[1])
    {
        auto* target = actions_[1]->ToGraphNode(graph);
        second.GetPin()->ConnectTo(target->GetEnter(0));
    }
    return node;
}

void Sequence::FromGraphNode(GraphNode* node)
{
    DynamicAction::FromGraphNode(node);
    if (const auto first = node->GetExit("first"))
    {
        const auto internalAction = MakeActionFromGraphNode(first.GetConnectedPin<GraphEnterPin>().GetNode());
        SetFirstAction(dynamic_cast<FiniteTimeAction*>(internalAction.Get()));
    }
    if (const auto second = node->GetExit("second"))
    {
        const auto internalAction = MakeActionFromGraphNode(second.GetConnectedPin<GraphEnterPin>().GetNode());
        SetSecondAction(dynamic_cast<FiniteTimeAction*>(internalAction.Get()));
    }
}

/// Create new action state from the action.
SharedPtr<ActionState> Sequence::StartAction(Object* target) { return MakeShared<SequenceState>(this, target); }

} // namespace Actions
} // namespace Urho3D
