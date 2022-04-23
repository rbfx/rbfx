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
#pragma once

#include "../Core/Context.h"
#include "../Input/Input.h"

namespace Urho3D
{
class Application;

    /// Base class for game "screen" - a unit of game state. Tipical samples of a game screen would be loading screen, menu or a game screen.
class URHO3D_API GameScreen : public Object
{
    URHO3D_OBJECT(GameScreen, Object);

public:
    /// Construct.
    explicit GameScreen(Context* context);
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Activate game screen. Executed by Application.
    virtual void Activate(Application* application);

    /// Deactivate game screen. Executed by Application.
    virtual void Deactivate();

    /// Get activation flag. Returns true if game screen is active.
    bool GetIsActive() const { return isActive; }
    /// Get current application.
    Application* GetApplication() const { return appication_; }

    /// Set whether the operating system mouse cursor is visible.
    void SetMouseVisible(bool enable);
    /// Set whether the mouse is currently being grabbed by an operation.
    void SetMouseGrabbed(bool grab);
    /// Set the mouse mode.
    void SetMouseMode(MouseMode mode);

    /// Return whether the operating system mouse cursor is visible.
    /// @property
    bool IsMouseVisible() const { return mouseVisible_; }
    /// Return whether the mouse is currently being grabbed by an operation.
    /// @property
    bool IsMouseGrabbed() const { return mouseGrabbed_; }
    /// Return the mouse mode.
    /// @property
    MouseMode GetMouseMode() const { return mouseMode_; }

private:
    /// Initialize mouse mode on non-web platform.
    void InitMouseMode();

    /// Handle request for mouse mode on web platform.
    void HandleMouseModeRequest(StringHash eventType, VariantMap& eventData);
    /// Handle request for mouse mode change on web platform.
    void HandleMouseModeChange(StringHash eventType, VariantMap& eventData);


private:
    bool isActive{false};
    WeakPtr<Application> appication_{};
    bool mouseVisible_{true};
    bool mouseGrabbed_{false};
    MouseMode mouseMode_{MM_FREE};
};

}
