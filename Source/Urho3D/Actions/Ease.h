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

namespace Urho3D
{

/// Base action state.
class URHO3D_API ActionEase : public FiniteTimeAction
{
    URHO3D_FINITETIMEACTION(ActionEase, FiniteTimeAction)

public:
    /// Construct.
    explicit ActionEase(Context* context, FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_; }

private:
    SharedPtr<FiniteTimeAction> innerAction_;
};

/// Base action state.
class URHO3D_API EaseBackIn : public ActionEase
{
    URHO3D_FINITETIMEACTION(EaseBackIn, ActionEase)

public:
    /// Construct.
    explicit EaseBackIn(Context* context, FiniteTimeAction* action);
};

/// Base action state.
class URHO3D_API EaseBackOut : public ActionEase
{
    URHO3D_FINITETIMEACTION(EaseBackOut, ActionEase)

public:
    /// Construct.
    explicit EaseBackOut(Context* context, FiniteTimeAction* action);
};


} // namespace Urho3D
