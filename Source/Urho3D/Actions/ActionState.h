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

#include "../Core/Object.h"
#include "BaseAction.h"

namespace Urho3D
{
class ActionManager;
namespace Actions
{

class ActionState : public RefCounted
{
protected:
    ActionState(BaseAction* action, Object* target);

public:
    /// Called once per frame.
    /// Time value is between 0 and 1
    /// For example:
    /// 0 means that the action just started
    /// 0.5 means that the action is in the middle
    /// 1 means that the action is over
    virtual void Update(float time);

    /// Gets the target.
    /// Will be set with the 'StartAction' method of the corresponding Action.
    /// When the 'Stop' method is called, Target will be set to null.
    Object* GetTarget() const { return _target; }
    /// Gets the original target.
    Object* GetOriginalTarget() const { return _originalTarget; }
    /// Gets the action.
    BaseAction* GetAction() const { return _action; }
    /// Gets a value indicating whether this instance is done.
    virtual bool IsDone() const { return true; }

    /// Called after the action has finished.
    /// It will set the 'Target' to null.
    /// IMPORTANT: You should never call this method manually. Instead, use: "target.StopAction(actionState);"
    virtual void Stop();

protected:
    /// Called every frame with it's delta time.
    /// DON'T override unless you know what you are doing.
    virtual void Step(float dt);

    /// Call StartAction on an action.
    SharedPtr<ActionState> StartAction(BaseAction* action, Object* target) const;

private:
    SharedPtr<BaseAction> _action;
    /// Active target reference. Set to nullptr when action is complete.
    WeakPtr<Object> _target;
    /// Original target reference. To track target references in ActionManager.
    WeakPtr<Object> _originalTarget;

    friend class Urho3D::ActionManager;
};

} // namespace Actions
} // namespace Urho3D
