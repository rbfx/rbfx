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

#include "Parallel.h"

#include "ActionBuilder.h"
#include "ActionManager.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/IO/ArchiveSerializationContainer.h"

#include <numeric>

namespace Urho3D
{
namespace Actions
{

namespace
{

struct StateAndDuration
{
    SharedPtr<FiniteTimeActionState> state_;
    float timeScale_;
};
struct ParallelState : public FiniteTimeActionState
{
    ParallelState(Parallel* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        const unsigned numActions = action->GetNumActions();
        innerActionStates_.reserve(numActions);
        const auto totalDuration = action->GetDuration();
        for (unsigned i = 0; i < numActions; ++i)
        {
            FiniteTimeAction* innerAction = action->GetAction(i);
            const auto innerDuration = innerAction->GetDuration();
            auto action = StartAction(innerAction, target);
            if (action)
            {
                innerActionStates_.emplace_back(StateAndDuration{action, totalDuration / innerDuration});
            }
        }
    }

    void Update(float time) override
    {
        for (auto& state : innerActionStates_)
        {
            state.state_->Update(Clamp(time * state.timeScale_, 0.0f, 1.0f));
        }
    }

    void Stop() override
    {
        for (auto& state : innerActionStates_)
            state.state_->Stop();
        FiniteTimeActionState::Stop();
    }

    ea::fixed_vector<StateAndDuration, 4> innerActionStates_;
};

} // namespace

/// Construct.
Parallel::Parallel(Context* context)
    : BaseClassName(context)
{
}

/// Set number of actions.
void Parallel::SetNumActions(unsigned num)
{
    if (num < actions_.size())
        actions_.resize(num);
    else
        SetAction(num, nullptr);
}

/// Get action by index.
void Parallel::SetAction(unsigned index, FiniteTimeAction* action)
{
    if (index < actions_.size())
    {
        actions_.reserve(index + 1);
        auto empty = GetOrDefault(nullptr);
        while (index < actions_.size())
        {
            actions_[index] = empty;
        }
    }
    actions_[index] = GetOrDefault(action);
}

/// Add action.
void Parallel::AddAction(FiniteTimeAction* action)
{
    actions_.emplace_back(action);
}

/// Get action by index.
FiniteTimeAction* Parallel::GetAction(unsigned index) const
{
    if (index >= actions_.size())
        return context_->GetSubsystem<ActionManager>()->GetEmptyAction();
    return actions_[index];
}

/// Get action duration.
float Parallel::GetDuration() const {
    auto duration = ea::numeric_limits<float>::epsilon();
    for (auto& action : actions_)
        duration = Max(duration, action->GetDuration());
    return duration;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> Parallel::Reverse() const
{
    auto result = MakeShared<Parallel>(context_);

    result->actions_.reserve(actions_.size());
    for (auto& action: actions_)
    {
        result->actions_.push_back(action->Reverse());
    }
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void Parallel::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeVector(archive, "actions", actions_, "action");
}

/// Create new action state from the action.
SharedPtr<ActionState> Parallel::StartAction(Object* target) { return MakeShared<ParallelState>(this, target); }

} // namespace Actions
} // namespace Urho3D
