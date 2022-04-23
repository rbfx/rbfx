//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Engine/Application.h"
#include "../Engine/GameScreen.h"
#if URHO3D_SYSTEMUI
#include "../SystemUI/Console.h"
#endif

namespace Urho3D
{

GameScreen::GameScreen(Context* context)
    : Object(context)
{
}

void GameScreen::RegisterObject(Context* context)
{
    context->AddFactoryReflection<GameScreen>();
}

/// Activate game screen. Executed by Application.
void GameScreen::Activate(Application* application)
{
    if (isActive)
    {
        return;
    }

    isActive = true;

    appication_ = application;

    auto* input = GetSubsystem<Input>();

    InitMouseMode();
    input->SetMouseVisible(mouseVisible_);
    input->SetMouseGrabbed(mouseGrabbed_);

    //// Subscribe to keyboard messages.
    //SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(GameScreen, HandleLogMessage));
}

/// Deactivate game screen. Executed by Application.
void GameScreen::Deactivate()
{
    if (!isActive)
    {
        return;
    }
    isActive = false;
    appication_ = nullptr;
}

/// Set whether the operating system mouse cursor is visible.
void GameScreen::SetMouseVisible(bool enable) {
    mouseVisible_ = enable;
    if (GetIsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseVisible(enable);
    }
}

/// Set whether the mouse is currently being grabbed by an operation.
void GameScreen::SetMouseGrabbed(bool grab) {
    mouseGrabbed_ = grab;
    if (GetIsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseGrabbed(mouseGrabbed_);
    }
}

/// Set the mouse mode.
void GameScreen::SetMouseMode(MouseMode mode) {
    mouseMode_ = mode;
    if (GetIsActive())
    {
        InitMouseMode();
    }
}

void GameScreen::InitMouseMode()
{
    Input* input = GetSubsystem<Input>();

    if (GetPlatform() != "Web")
    {
        input->SetMouseMode(mouseMode_);

        if (mouseMode_ != MM_ABSOLUTE)
        {
#if URHO3D_SYSTEMUI
            Console* console = GetSubsystem<Console>();
            if (console && console->IsVisible())
                input->SetMouseMode(MM_ABSOLUTE, true);
#endif
        }
    }
    else
    {
        input->SetMouseVisible(true);
        SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(GameScreen, HandleMouseModeRequest));
        SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(GameScreen, HandleMouseModeChange));
    }
}


// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void GameScreen::HandleMouseModeRequest(StringHash /*eventType*/, VariantMap& eventData)
{
#if URHO3D_SYSTEMUI
    Console* console = GetSubsystem<Console>();
    if (console && console->IsVisible())
        return;
#endif
    Input* input = GetSubsystem<Input>();
    if (mouseMode_ == MM_ABSOLUTE)
        input->SetMouseVisible(false);
    else if (mouseMode_ == MM_FREE)
        input->SetMouseVisible(true);
    input->SetMouseMode(mouseMode_);
}

void GameScreen::HandleMouseModeChange(StringHash /*eventType*/, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}

}
