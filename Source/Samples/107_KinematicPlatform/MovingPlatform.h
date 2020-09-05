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
class MovingPlatform : public LogicComponent
{
    URHO3D_OBJECT(MovingPlatform, LogicComponent);
public:

    MovingPlatform(Context* context);
    virtual ~MovingPlatform();

    static void RegisterObject(Context* context);

    virtual void FixedUpdate(float timeStep);

    void Initialize(Node *liftNode, const Vector3 &finishPosition, bool updateBodyOnPlatform=true);
    void SetPlatformSpeed(float speed) { maxLiftSpeed_ = speed; }

protected:
    WeakPtr<Node> platformNode_;
    WeakPtr<Node> platformVolumdNode_;

    Vector3 initialPosition_;
    Vector3 finishPosition_;
    Vector3 directionToFinish_;
    float maxLiftSpeed_;
    float minLiftSpeed_;
    float curLiftSpeed_;

    int platformState_;
    enum PlatformStateType 
    { 
        PLATFORM_STATE_START, 
        PLATFORM_STATE_MOVETO_FINISH, 
        PLATFORM_STATE_MOVETO_START, 
        PLATFORM_STATE_FINISH 
    };
};




