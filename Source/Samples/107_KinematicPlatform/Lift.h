//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

namespace Urho3D
{
class Controls;
}

//=============================================================================
//=============================================================================
class Lift : public LogicComponent
{
    URHO3D_OBJECT(Lift, LogicComponent);
public:

    Lift(Context* context);
    virtual ~Lift();

    static void RegisterObject(Context* context);

    virtual void Start();
    virtual void FixedUpdate(float timeStep);

    void Initialize(Node *liftNode, const Vector3 &finishPosition);
    void SetLiftSpeed(float speed) { maxLiftSpeed_ = speed; }

protected:
    void SetTransitionCompleted(int toState);
    void ButtonPressAnimate(bool pressed);
    void HandleButtonStartCollision(StringHash eventType, VariantMap& eventData);
    void HandleButtonEndCollision(StringHash eventType, VariantMap& eventData);

protected:
    WeakPtr<Node>       liftNode_;
    WeakPtr<Node>       liftButtonNode_;

    Vector3 initialPosition_;
    Vector3 finishPosition_;
    Vector3 directionToFinish_;
    float   totalDistance_;
    float   maxLiftSpeed_;
    float   minLiftSpeed_;
    float   curLiftSpeed_;

    bool    buttonPressed_;
    float   buttonPressedHeight_;
    bool    standingOnButton_;

    // states
    int liftButtonState_;
    enum LiftButtonStateType 
    { 
        LIFT_BUTTON_UP, 
        LIFT_BUTTON_POPUP, 
        LIFT_BUTTON_DOWN 
    };

    int liftState_;
    enum LiftStateType 
    { 
        LIFT_STATE_START, 
        LIFT_STATE_MOVETO_FINISH, 
        LIFT_STATE_MOVETO_START, 
        LIFT_STATE_FINISH 
    };
};




