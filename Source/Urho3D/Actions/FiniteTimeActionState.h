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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "ActionState.h"

namespace Urho3D
{
class Object;

namespace Actions
{
class FiniteTimeAction;

/// Finite time action state.
class URHO3D_API FiniteTimeActionState : public ActionState
{
public:
    /// Construct.
    FiniteTimeActionState(FiniteTimeAction* action, Object* target);
    /// Destruct.
    ~FiniteTimeActionState() override;

    /// Gets a value indicating whether this instance is done.
    bool IsDone() const override { return elapsed_ >= duration_; }

    /// Called every frame with it's delta time.
    void Step(float dt) override;

    /// Get action duration.
    float GetDuration() const { return duration_; }
    /// Get action elapsed time.
    float GetElapsed() const { return elapsed_; }

protected:
    /// Call StartAction on an action.
    SharedPtr<FiniteTimeActionState> StartAction(FiniteTimeAction* action, Object* target) const;

private:
    float duration_{};
    float elapsed_{};
    bool firstTick_{true};
};

} // namespace Actions
} // namespace Urho3D
