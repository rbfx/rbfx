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

#include "ActionState.h"
#include "BaseAction.h"

#include "../Core/Context.h"

namespace Urho3D
{
namespace Actions
{

ActionState::ActionState(BaseAction* action, Object* target)
    : _action(action)
    , _target(target)
    , _originalTarget(target)
{
}

void ActionState::Update(float time) {}

void ActionState::Stop() { _target.Reset(); }

void ActionState::Step(float dt) {}

SharedPtr<ActionState> ActionState::StartAction(BaseAction* action, Object* target) const
{
    if (action)
        return action->StartAction(target);
    return {};
}

} // namespace Actions
} // namespace Urho3D
