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

#pragma once

#include "BaseAction.h"
#include "FiniteTimeAction.h"

namespace Urho3D
{

/// Action as resource
class URHO3D_API ActionBuilder
{
private:
    /// Construct.
    explicit ActionBuilder(Context* context, Actions::BaseAction* action);

public:
    /// Construct.
    explicit ActionBuilder(Context* context);

    /// Continue with provided action.
    ActionBuilder Then(Actions::FiniteTimeAction* nextAction);

    /// Run action in parallel to current one.
    ActionBuilder Also(Actions::FiniteTimeAction* parallelAction);

    /// Continue with MoveBy action.
    ActionBuilder MoveBy(float duration, const Vector3& offset);

    /// Continue with MoveBy2D action.
    ActionBuilder MoveBy2D(float duration, const Vector2& offset);

    /// Continue with Hide action.
    ActionBuilder Hide();

    /// Continue with Show action.
    ActionBuilder Show();

    /// Continue with Blink action.
    ActionBuilder Blink(float duration, unsigned numOfBlinks);
    
    /// Continue with AttributeBlink action.
    ActionBuilder Blink(float duration, unsigned numOfBlinks, const ea::string_view& attributeName);
    
    /// Combine with BackIn action.
    ActionBuilder BackIn();

    /// Combine with BackOut action.
    ActionBuilder BackOut();

    /// Combine with BackInOut action.
    ActionBuilder BackInOut();

    /// Combine with BounceOut action.
    ActionBuilder BounceOut();

    /// Combine with BounceIn action.
    ActionBuilder BounceIn();

    /// Combine with BounceInOut action.
    ActionBuilder BounceInOut();

    /// Combine with SineOut action.
    ActionBuilder SineOut();

    /// Combine with SineIn action.
    ActionBuilder SineIn();

    /// Combine with SineInOut action.
    ActionBuilder SineInOut();

    /// Combine with ExponentialOut action.
    ActionBuilder ExponentialOut();

    /// Combine with ExponentialIn action.
    ActionBuilder ExponentialIn();

    /// Combine with ExponentialInOut action.
    ActionBuilder ExponentialInOut();

    /// Combine with ElasticIn action.
    ActionBuilder ElasticIn(float period = 0.3f);

    /// Combine with ElasticOut action.
    ActionBuilder ElasticOut(float period = 0.3f);

    /// Combine with ElasticInOut action.
    ActionBuilder ElasticInOut(float period = 0.3f);

    /// Combine with RemoveSelf action.
    ActionBuilder RemoveSelf();

    /// Combine with DelayTime action.
    ActionBuilder DelayTime(float duration);

    /// Complete action building and produce result.
    SharedPtr<Actions::BaseAction> Build() { return action_; }

private:
    /// Urho3D context.
    Context* context_{};
    /// Action on top of stack (current).
    SharedPtr<Actions::BaseAction> action_;
};

} // namespace Urho3D
