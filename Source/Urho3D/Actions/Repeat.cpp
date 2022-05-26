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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Repeat.h"

#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace Actions
{
/// ------------------------------------------------------------
namespace
{
class RepeatState : public FiniteTimeActionState
{
    SharedPtr<FiniteTimeAction> innerAction_;
    SharedPtr<FiniteTimeActionState> innerState_;
    unsigned times_;
    unsigned total_;
    float nextDt_;
    float slice_;

public:
    RepeatState(Repeat* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        innerAction_ = action->GetInnerAction();
        times_ = action->GetTimes();

        slice_ = nextDt_ = innerAction_->GetDuration() / action->GetDuration();

        innerState_ = StartAction(innerAction_, target);
        total_ = 0;
    }

    bool IsDone() const override { return total_ >= times_; }

    void Update(float time) override
    {
        if (time >= nextDt_)
        {
            while (time > nextDt_ && total_ < times_)
            {
                innerState_->Update(1.0f);
                innerState_->Stop();
                total_++;

                innerState_ = StartAction(innerAction_, GetOriginalTarget());
                nextDt_ = (total_ + 1.0f) * slice_;
            }
            if (total_ == times_)
            {
                innerState_->Update(1.0f);
                innerState_->Stop();
            }
            else
            {
                innerState_->Update((time - (nextDt_ - slice_)) / slice_);
            }
        }
        else
        {
            innerState_->Update(Mod(time * times_, 1.0f));
        }
    }

    void Stop() override
    {
        innerState_->Stop();
        FiniteTimeActionState::Stop();
    }
};

} // namespace

/// Construct.
Repeat::Repeat(Context* context)
    : BaseClassName(context)
{
    innerAction_ = GetOrDefault(nullptr);
}
/// Get action duration.
float Repeat::GetDuration() const { return innerAction_->GetDuration() * times_; }

/// Set inner action.
void Repeat::SetInnerAction(FiniteTimeAction* action)
{
    innerAction_ = GetOrDefault(action);
    SetDuration(GetDuration());
}

/// Set number of repetitions.
void Repeat::SetTimes(unsigned times)
{
    times_ = times;
    SetDuration(GetDuration());
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> Repeat::Reverse() const
{
    auto result = MakeShared<Repeat>(context_);
    result->SetTimes(times_);
    result->SetInnerAction(innerAction_->Reverse());
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void Repeat::SerializeInBlock(Archive& archive)
{
    // Skipping FiniteTimeAction::SerializeInBlock on purpose to skip duration serialization
    BaseAction::SerializeInBlock(archive);
    SerializeValue(archive, "innerAction", innerAction_);
}

/// Create new action state from the action.
SharedPtr<ActionState> Repeat::StartAction(Object* target) { return MakeShared<RepeatState>(this, target); }

/// ------------------------------------------------------------
namespace
{
class RepeatForeverState : public FiniteTimeActionState
{
    SharedPtr<FiniteTimeAction> innerAction_;
    SharedPtr<FiniteTimeActionState> innerState_;

public:
    RepeatForeverState(RepeatForever* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        innerAction_ = action->GetInnerAction();
        innerState_ = StartAction(innerAction_, target);
    }

    bool IsDone() const override { return false; }

    void Step(float dt) override
    {
        innerState_->Step(dt);
        if (innerState_->IsDone())
        {
            float dt = innerState_->GetDuration() - innerState_->GetElapsed();
            innerState_ = StartAction(innerAction_, GetOriginalTarget());
            innerState_->Step(0);
            innerState_->Step(dt);
        }
    }

    void Stop() override
    {
        innerState_->Stop();
        FiniteTimeActionState::Stop();
    }
};

} // namespace

/// Construct.
RepeatForever::RepeatForever(Context* context)
    : BaseClassName(context)
{
    innerAction_ = GetOrDefault(nullptr);
}
/// Get action duration.
float RepeatForever::GetDuration() const { return ea::numeric_limits<float>::max(); }

/// Set inner action.
void RepeatForever::SetInnerAction(FiniteTimeAction* action) { innerAction_ = GetOrDefault(action); }

/// Create reversed action.
SharedPtr<FiniteTimeAction> RepeatForever::Reverse() const
{
    auto result = MakeShared<RepeatForever>(context_);
    result->SetInnerAction(innerAction_->Reverse());
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void RepeatForever::SerializeInBlock(Archive& archive)
{
    // Skipping FiniteTimeAction::SerializeInBlock on purpose to skip duration serialization
    BaseAction::SerializeInBlock(archive);
    SerializeValue(archive, "innerAction", innerAction_);
}

/// Create new action state from the action.
SharedPtr<ActionState> RepeatForever::StartAction(Object* target)
{
    return MakeShared<RepeatForeverState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
