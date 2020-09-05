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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsUtils.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <SDL/SDL_log.h>

#include "MovingPlatform.h"
#include "KinematicCharacter.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
MovingPlatform::MovingPlatform(Context* context)
    : LogicComponent(context)
    , maxLiftSpeed_(5.0f)
    , minLiftSpeed_(1.5f)
    , curLiftSpeed_(0.0f)
{
    SetUpdateEventMask(USE_NO_EVENT);
}

MovingPlatform::~MovingPlatform()
{
}

void MovingPlatform::RegisterObject(Context* context)
{
    context->RegisterFactory<MovingPlatform>();
}

void MovingPlatform::Initialize(Node *platformNode, const Vector3 &finishPosition, bool updateBodyOnPlatform)
{
    // get other lift components
    platformNode_ = platformNode;
    platformVolumdNode_ = platformNode_->GetChild("PlatformVolume", true);

    assert( platformNode_ && platformVolumdNode_ && "missing nodes!" );

    // positions
    initialPosition_   = platformNode_->GetWorldPosition();
    finishPosition_    = finishPosition;
    directionToFinish_ = (finishPosition_ - initialPosition_).Normalized();

    // state
    platformState_ = PLATFORM_STATE_MOVETO_FINISH;
    curLiftSpeed_ = maxLiftSpeed_;

    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void MovingPlatform::FixedUpdate(float timeStep)
{
    Vector3 platformPos = platformNode_->GetPosition();
    Vector3 newPos = platformPos;

    // move platform
    if (platformState_ == PLATFORM_STATE_MOVETO_FINISH)
    {
        Vector3 curDistance  = finishPosition_ - platformPos;
        Vector3 curDirection = curDistance.Normalized();
        float dist = curDistance.Length();
        float dotd = directionToFinish_.DotProduct(curDirection);

        if (dotd > 0.0f)
        {
            // slow down near the end
            if (dist < 1.0f)
            {
                curLiftSpeed_ *= 0.92f;
            }
            curLiftSpeed_ = Clamp(curLiftSpeed_, minLiftSpeed_, maxLiftSpeed_);
            newPos += curDirection * curLiftSpeed_ * timeStep;
        }
        else
        {
            newPos = finishPosition_;
            curLiftSpeed_ = maxLiftSpeed_;
            platformState_ = PLATFORM_STATE_MOVETO_START;
        }
        platformNode_->SetPosition(newPos);
    }
    else if (platformState_ == PLATFORM_STATE_MOVETO_START)
    {
        Vector3 curDistance  = initialPosition_ - platformPos;
        Vector3 curDirection = curDistance.Normalized();
        float dist = curDistance.Length();
        float dotd = directionToFinish_.DotProduct(curDirection);

        if (dotd < 0.0f)
        {
            // slow down near the end
            if (dist < 1.0f)
            {
                curLiftSpeed_ *= 0.92f;
            }
            curLiftSpeed_ = Clamp(curLiftSpeed_, minLiftSpeed_, maxLiftSpeed_);
            newPos += curDirection * curLiftSpeed_ * timeStep;
        }
        else
        {
            newPos = initialPosition_;
            curLiftSpeed_ = maxLiftSpeed_;
            platformState_ = PLATFORM_STATE_MOVETO_FINISH;
        }

        platformNode_->SetPosition(newPos);
    }
}




