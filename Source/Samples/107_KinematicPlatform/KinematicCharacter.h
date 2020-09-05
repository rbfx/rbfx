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

#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
class AnimationController;
class RigidBody;
class CollisionShape;
}

using namespace Urho3D;
class CharacterController;

//=============================================================================
//=============================================================================
const int CTRL_FORWARD = 1;
const int CTRL_BACK = 2;
const int CTRL_LEFT = 4;
const int CTRL_RIGHT = 8;
const int CTRL_JUMP = 16;

const float MOVE_FORCE = 0.2f;
const float INAIR_MOVE_FORCE = 0.06f;
const float BRAKE_FORCE = 0.2f;
const float JUMP_FORCE = 7.0f;
const float YAW_SENSITIVITY = 0.1f;
const float INAIR_THRESHOLD_TIME = 0.1f;

//=============================================================================
//=============================================================================
struct MovingData
{
    MovingData() : node_(0){}
    
    MovingData& operator =(const MovingData& rhs)
    {
        node_ = rhs.node_;
        transform_ = rhs.transform_;
        return *this;
    }
    bool operator == (const MovingData& rhs)
    {
        return (node_ && node_ == rhs.node_);
    }

    Node *node_;
    Matrix3x4 transform_;
};

//=============================================================================
//=============================================================================
/// Character component, responsible for physical movement according to controls, as well as animation.
class KinematicCharacter : public LogicComponent
{
    URHO3D_OBJECT(KinematicCharacter, LogicComponent);

public:
    /// Construct.
    KinematicCharacter(Context* context);
    
    /// Register object factory and attributes.
    static void RegisterObject(Context* context);
    
    virtual void DelayedStart();

    /// Handle startup. Called by LogicComponent base class.
    virtual void Start();
    /// Handle physics world update. Called by LogicComponent base class.
    virtual void FixedUpdate(float timeStep);
    virtual void FixedPostUpdate(float timeStep);
    
    void SetOnMovingPlatform(RigidBody *platformBody)
    { 
        //onMovingPlatform_ = (platformBody != NULL);
        //platformBody_ = platformBody; 
    }

public:
    /// Movement controls. Assigned by the main program each frame.
    Controls controls_;

protected:
    bool IsNodeMovingPlatform(Node *node) const;
    void NodeOnMovingPlatform(Node *node);
    /// Handle physics collision event.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
    
protected:
    bool onGround_;
    bool okToJump_;
    float inAirTimer_;

    // extra vars
    Vector3 curMoveDir_;
    bool isJumping_;
    bool jumpStarted_;

    WeakPtr<CollisionShape> collisionShape_;
    WeakPtr<AnimationController> animController_;
    WeakPtr<CharacterController> kinematicController_;

    // moving platform data
    MovingData movingData_[2];
};
