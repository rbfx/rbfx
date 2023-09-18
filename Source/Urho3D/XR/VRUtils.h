// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/XR/VirtualReality.h"

namespace Urho3D
{
    /// TODO: Revisit these helpers, they may duplicate Input classes like DirectionalPadAdapter.

    /// Calculates a motion vector based on the head. Optionally locked to XZ plane or normalized (which means no diagonal turbo speed).
    URHO3D_API Vector3 SmoothLocomotionHead(Node* rigNode, XRBinding* joystickBinding, float deadZone, bool xzPlanar = true, bool normalized = true);

    /// Calculates a motion vector based on a controller aim direction. Optionally locked to XZ plane or normalized (which means no diagonal turbo speed).
    URHO3D_API Vector3 SmoothLocomotionAim(Node* rigNode, XRBinding* joystickBinding, VRHand whichHand, float deadZone, bool xzPlanar = true, bool normalized = false);

    /// Compares old and new positions of the given node to calculate a motion vector, nothing fancy, zero out Y or XZ as required for walk/climb/etc.
    URHO3D_API Vector3 GrabLocomotion(Node* handNode);

    /// Wraps treating the trackpad as a 4 button d-pad with an optional center if centerRadius > 0. Buttons are labeled clockwise from the top starting at 1 and center at 5.
    /// Will work fine with joysticks if a click input is provided, ie. to do chorded input checks or stick press.
    /// Optional output for whether the trackpad is down or not.
    URHO3D_API int TrackpadAsDPad(XRBinding* trackpadPosition, XRBinding* trackpadClick, float centerRadius, bool* trackpadDown = nullptr);

    /// Wraps treating the joystick as a D-PAD, ie. such as to do snap turning or constant rate turning.
    /// Same return value conventions as TrackpadAsDPad without a Center.
    URHO3D_API int JoystickAsDPad(XRBinding* joystickPosition, float centerDeadzone);

    /// Wraps treating the trackpad as 2 buttons, Up and Inside are used unless upDownMode in which case Up and Down are used. Much the same as DPAD but eliminates checking for left vs right.
    URHO3D_API int TrackpadAsTwoButton(XRBinding* trackpadPosition, XRBinding* trackpadClick, float centerDeadZoneRadius, VRHand hand, bool upDownMode = false, bool* trackPadDown = nullptr);

    /// Returns true when current code transitions into a no-code (0 / released), rolling over to a new non-zero code means a "shift" like you've rocked your thumb from X to Y to correct a mistake.
    URHO3D_API bool ButtonClicked(int targetCode, int* currentCode, int nextCode);

    /// Manages the behaviour of a held "button code" and returns true once the hold duration has elapsed.
    /// Inputs:
    ///     nextCode = the code we've just received from input query
    ///     deltaTime = time in seconds elapsed
    ///     holdForDuration = how long the button needs to be depressed
    /// In/Out:
    ///     currentCode = button code currently pressed
    ///     time = current duration of time this code has been in effect
    ///     alreadyDone = initialize as false, when true indicates we've hit the duration and should skip processing, reset when a new code is passed in
    ///     fraction = 0.0 - 1.0 ratio of how far into the long press it has progressed
    /// Optional output for fraction.
    URHO3D_API bool ButtonLongPress(int targetCode, int* currentCode, float* time, bool* alreadyDone, int nextCode, float deltaTime, float holdForDuration, float* fraction = nullptr);

    /// Similar button held but if the code changes before the time passes it will return 1, and 2 if the duration has elapsed.
    /// Returns 1 on a short press and 2 on a long press.
    /// Optional output to detect if inside long-press (ie. for UI purposes float fraction = (time - shortPressWindow) / (holdForDuration - shortPressWindow)).
    /// Optional output to fraction within the span between the shortPressWindow and holdForDuration.
    /// If short press window is > 0 then that will be the time duration after which short press is no longer possible.
    URHO3D_API int ButtonShortOrLongPress(int targetCode, int* currentCode, float* time, bool* alreadyDone, int nextCode, float deltaTime, float holdForDuration, float shortPressWindow, bool* inLongPress = nullptr, float* fraction = nullptr);

    /// The button may be quickly tapped or held past a shortPressWindow for a different input so long as as it is held down.
    URHO3D_API int ButtonTapOrHold(int targetCode, int* currentCode, float* time, int nextCode, float deltaTime, float shortPressWindow);

    /// Helper struct to manage variables for making calls to the above functions to manage variable coded button presses, reduces boiler-plate involving state variables.
    /// Not intended for varying usage, only one of the check methods should be used for a given ButtonCommand instance.
    /// Changing the method that will be called requires calling Reset() to flush the state.
    /// To use for a real button, set targetCode_ to true and use bool to int conversion in the check functions.
    struct URHO3D_API ButtonCommand
    {
        /// Target code we're looking for.
        int targetCode_;
        /// Time length after which short-press is forbidden.
        float shortPressWindow_;
        /// Time length to constitute a long-press.
        float holdDuration_;

        /// State, current action code received from input.
        int currentCode_ = { 0 };
        /// State, current time in hold.
        float time_ = { 0.0f };
        /// State, flag for not to respond.
        bool alreadyDone_ = { false };
        /// State, current hold duration fraction. If zero then holding is not active.
        float fraction_ = { 0.0f };

        ButtonCommand(int targetCode, float holdDuration = 3.0f, float shortPressWindow = 1.0f) :
            targetCode_(targetCode), holdDuration_(holdDuration), shortPressWindow_(shortPressWindow)
        {

        }

        /// Necessary to reset anytime you intend to change your mechanism of use.
        void Reset()
        {
            currentCode_ = 0;
            time_ = 0.0f;
            fraction_ = 0.0f;
            alreadyDone_ = false;
        }

        /// Check for only caring about whether down or not, ie. this is a modal toggle that affects something else.
        bool CheckDown(int newCode)
        {
            currentCode_ = newCode;
            return currentCode_ == targetCode_;
        }

        /// Check for a strict once activation.
        bool CheckStrict(int newCode)
        {
            if (targetCode_ == newCode && currentCode_ != newCode)
            {
                alreadyDone_ = true;
                currentCode_ = newCode;
                return true;
            }
            else if (newCode == 0)
            {
                alreadyDone_ = false;
                currentCode_ = newCode;
            }
            else
                currentCode_ = newCode;
            return false;
        }

        /// Check for activate on release, time down is irrelevant.
        bool CheckClick(int newCode)
        {
            return ButtonClicked(targetCode_, &currentCode_, newCode);
        }

        /// Check for a long press.
        bool CheckLongPress(int newCode, float deltaTime)
        {
            return ButtonLongPress(targetCode_, &currentCode_, &time_, &alreadyDone_, newCode, deltaTime, holdDuration_, &fraction_);
        }

        /// Check for a dual input that is short or long pressed.
        int CheckShortOrLongPress(int newCode, float deltaTime)
        {
            return ButtonShortOrLongPress(targetCode_, &currentCode_, &time_, &alreadyDone_, newCode, deltaTime, holdDuration_, shortPressWindow_, nullptr, &fraction_);
        }

        /// Check for a quick tap or a steady hold.
        int CheckTapOrHold(int newCode, float deltaTime)
        {
            return ButtonTapOrHold(targetCode_, &currentCode_, &time_, newCode, deltaTime, shortPressWindow_);
        }
    };

}
