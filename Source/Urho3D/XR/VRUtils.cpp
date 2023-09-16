// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/VRUtils.h"

#include "Urho3D/Scene/Node.h"
#include "Urho3D/XR/VirtualReality.h"

#define NORMALIZE(v, a, b) ((v - a) / (b - a))
#define DENORMALIZE(v, a, b) (v * (b-a) + a)

namespace Urho3D
{

Vector3 SmoothLocomotionHead(Node* rigNode, XRBinding* joystickBinding, float deadZone, bool xzPlanar, bool normalized)
{
    if (rigNode == nullptr || joystickBinding == nullptr)
        return Vector3::ZERO;

    Vector3 fore = Vector3::ZERO;
    Vector3 right = Vector3::ZERO;

    auto lEye = rigNode->GetChild("Left_Eye", true);
    auto rEye = rigNode->GetChild("Right_Eye", true);
    if (lEye == nullptr || rEye == nullptr)
        return Vector3::ZERO;

    // so ... head transform is always IDENTITY
    fore = lEye->GetWorldDirection() + rEye->GetWorldDirection();
    right = lEye->GetWorldRight() + rEye->GetWorldRight();

    if (xzPlanar)
    {
        fore.y_ = 0;
        right.y_ = 0;
    }

    fore.Normalize();
    right.Normalize();

    auto stickMotion = joystickBinding->GetVector2();
    const auto signX = Sign(stickMotion.x_);
    const auto signY = Sign(stickMotion.y_);
    stickMotion.x_ = Abs(stickMotion.x_) < deadZone ? 0.0f : signX * InverseLerp(deadZone, 1.0f, Abs(stickMotion.x_));
    stickMotion.y_ = Abs(stickMotion.y_) < deadZone ? 0.0f : signY * InverseLerp(deadZone, 1.0f, Abs(stickMotion.y_));

    auto vec = fore * stickMotion.y_ + right * stickMotion.x_;
    return normalized ? vec.Normalized() : vec;
}

Vector3 SmoothLocomotionAim(Node* rigNode, XRBinding* joystickBinding, VRHand whichHand, float deadZone, bool xzPlanar, bool normalized)
{
    if (rigNode == nullptr || joystickBinding == nullptr)
        return Vector3::ZERO;

    auto vr = rigNode->GetSubsystem<VirtualReality>();

    Vector3 fore = Vector3::ZERO;
    Vector3 right = Vector3::ZERO;

    auto trans = vr->GetHandAimTransform(whichHand);
    fore = trans * Vector3::FORWARD;
    right = trans * Vector3::RIGHT;

    if (xzPlanar)
    {
        fore.y_ = 0;
        right.y_ = 0;
    }

    fore.Normalize();
    right.Normalize();

    auto stickMotion = joystickBinding->GetVector2();
    const auto signX = Sign(stickMotion.x_);
    const auto signY = Sign(stickMotion.x_);
    stickMotion.x_ = Abs(stickMotion.x_) < deadZone ? 0.0f : signX * DENORMALIZE(Abs(stickMotion.x_), deadZone, 1.0f);
    stickMotion.y_ = Abs(stickMotion.y_) < deadZone ? 0.0f : signY * DENORMALIZE(Abs(stickMotion.y_), deadZone, 1.0f);

    auto vec = fore * stickMotion.y_ + right * stickMotion.x_;
    return normalized ? vec.Normalized() : vec;
}

Vector3 GrabLocomotion(Node* handNode)
{
    if (handNode == nullptr)
        return Vector3::ZERO;

    const auto newPos = handNode->GetWorldPosition();
    auto var = handNode->GetVar("LastTransformWS");
    if (var.GetType() == VAR_MATRIX3X4)
    {
        auto oldTrans = var.GetMatrix3x4();
        auto delta = newPos - oldTrans.Translation();
        return delta;
    }
    return Vector3::ZERO;
}

int TrackpadAsDPad(XRBinding* trackpadPosition, XRBinding* trackpadClick, float centerRadius, bool* trackpadDown)
{
    if (!trackpadClick->IsActive() || !trackpadClick->IsBound())
        return 0;

    if (!trackpadPosition->IsActive() || !trackpadPosition->IsBound())
        return 0;

    if (trackpadDown)
        *trackpadDown = trackpadClick->GetBool();

    if (trackpadClick->GetBool())
    {
        auto pos = trackpadPosition->GetVector2();
        if (pos.x_ < centerRadius && pos.x_ > -centerRadius && pos.y_ < centerRadius && pos.y_ > -centerRadius)
            return 5; // center

        if (Abs(pos.x_) > Abs(pos.y_)) // left/right
        {
            if (pos.x_ > 0)
                return 2; // right
            return 4; // left
        }
        else
        {
            if (pos.y_ > 0)
                return 1; // up
            return 3; // down
        }
    }
    return 0;
}

int JoystickAsDPad(XRBinding* joystickPosition, float centerDeadzone)
{
    if (!joystickPosition->IsActive())
        return 0;

    auto pos = joystickPosition->GetVector2();
    if (Abs(pos.x_) < centerDeadzone && Abs(pos.y_) < centerDeadzone)
        return 0; // inside deadzone

    if (Abs(pos.x_) > Abs(pos.y_)) // left/right
    {
        if (pos.x_ > 0)
            return 2; // right
        return 4; // left
    }
    else
    {
        if (pos.y_ > 0)
            return 1; // up
        return 3; // down
    }
    return 0;
}

bool ButtonClicked(int targetCode, int* currentCode, int nextCode)
{
    if (*currentCode == targetCode && *currentCode != nextCode && nextCode == 0)
    {
        *currentCode = nextCode;
        return true;
    }

    *currentCode = nextCode;
    return false;
}


int TrackpadAsTwoButton(XRBinding* trackpadPosition, XRBinding* trackpadClick, float centerDeadzone, VRHand hand, bool upDownMode, bool* trackpadDown)
{
    if (!trackpadClick->IsActive() || !trackpadClick->IsBound())
        return 0;

    if (!trackpadPosition->IsActive() || !trackpadPosition->IsBound())
        return 0;

    if (trackpadDown)
        *trackpadDown = trackpadClick->GetBool();

    auto pos = trackpadPosition->GetVector2();

    if (Abs(pos.x_) < centerDeadzone && Abs(pos.y_) < centerDeadzone)
        return 0;

    if (trackpadClick->GetBool())
    {
        if (upDownMode)
        {
            if (pos.y_ > 0)
                return 1; // up
            return 2; // down
        }
        else
        {
            if (pos.y_ > 0 && pos.y_ > Abs(pos.x_))
                return 1; // up up and away
            else
            {
                if (hand == VRHand::Left && pos.x_ > 0)
                    return 2; // inside toward body median plane
                else if (hand == VRHand::Right && pos.x_ < 0)
                    return 2; // inside toward body median plane
            }

            return 0;
        }
    }

    return 0;
}

bool ButtonLongPress(int targetCode, int* currentCode, float* time, bool* banned, int nextCode, float deltaTime, float holdForDuration, float* fraction)
{
    if (fraction)
        *fraction = 0;

    if (*currentCode != nextCode)
    {
        // reset state
        *currentCode = nextCode;
        *time = 0.0f;
        *banned = false;
        if (fraction)
            *fraction = 0.0f;
        return false;
    }

    if (*currentCode == nextCode && nextCode == targetCode)
    {
        if (*banned) // we've already triggered
            return false;

        *time += deltaTime;
        if (*time > holdForDuration)
        {
            *time = 0;
            *banned = true;
            if (fraction)
                *fraction = 1.0f;
            return true;
        }

        if (fraction)
            *fraction = *time / holdForDuration;
    }

    return false;
}

int ButtonShortOrLongPress(int targetCode, int* currentCode, float* time, bool* alreadyDone, int nextCode, float deltaTime, float holdForDuration, float shortPressWindow, bool* inLongPress, float* fraction)
{
    if (fraction)
        *fraction = 0.0f;

    // code is changing
    if (*currentCode != nextCode)
    {
        // check for short press
        int retVal = (targetCode == *currentCode) ? 1 : 0;
        if (shortPressWindow != 0 && *time > shortPressWindow && *currentCode == targetCode)
            retVal = 0;

        *currentCode = nextCode;
        *time = 0.0f;
        *alreadyDone = false;
        if (inLongPress)
            *inLongPress = false;
        if (fraction)
            *fraction = 0.0f;
        return 0;
    }

    if (*currentCode == nextCode && nextCode == targetCode)
    {
        if (*alreadyDone) // already fired
            return 0;

        float& t = *time;
        t += deltaTime;

        // check if we've passed the short-press window
        if (inLongPress && t > shortPressWindow)
            *inLongPress = true;

        // long press
        if (t > holdForDuration)
        {
            t = 0;
            *alreadyDone = true;
            if (inLongPress)
                *inLongPress = false;

            if (fraction)
                *fraction = 1.0f;
            return 2;
        }

        if (fraction && holdForDuration != shortPressWindow) // check for div by zero in case of bad inputs
            *fraction = t >= shortPressWindow ? (t - shortPressWindow) / (holdForDuration - shortPressWindow) : 0.0f;
    }

    return 0;
}

int ButtonTapOrHold(int targetCode, int* currentCode, float* time, int nextCode, float deltaTime, float shortPressWindow)
{
    // check for short tap
    if (*currentCode == targetCode && *currentCode != nextCode && *time < shortPressWindow)
    {
        *time = 0;
        *currentCode = nextCode;
        return 1;
    }

    if (targetCode == nextCode && *currentCode == targetCode)
    {
        // have we been holding for long enough?
        if (*time > shortPressWindow)
        {
            *time += deltaTime;
            return 2;
        }

        *time += deltaTime;
    }
    else
    {
        *time = 0;
        *currentCode = nextCode;
    }

    return 0;
}

}
