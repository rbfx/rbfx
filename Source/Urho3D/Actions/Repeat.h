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

#pragma once

#include "FiniteTimeAction.h"
#include <EASTL/array.h>

namespace Urho3D
{
namespace Actions
{

/// Repeat inner action several times.
class URHO3D_API Repeat : public FiniteTimeAction
{
    URHO3D_OBJECT(Repeat, FiniteTimeAction)
public:
    /// Construct.
    explicit Repeat(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_.Get(); }

    /// Set number of repetitions.
    void SetTimes(unsigned times);

    /// Get number of repetitions.
    unsigned GetTimes() const { return times_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    unsigned times_;
    SharedPtr<FiniteTimeAction> innerAction_;
};

/// Repeat inner action forever.
class URHO3D_API RepeatForever : public FiniteTimeAction
{
    URHO3D_OBJECT(RepeatForever, FiniteTimeAction)
public:
    /// Construct.
    explicit RepeatForever(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_.Get(); }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    SharedPtr<FiniteTimeAction> innerAction_;
};

} // namespace Actions
} // namespace Urho3D
