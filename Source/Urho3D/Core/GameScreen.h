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
#include "../Graphics/Viewport.h"

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
    virtual void Activate();

    /// Deactivate game screen. Executed by Application.
    virtual void Deactivate();

    /// Handle the logic update event.
    virtual void Update(float timeStep);

    /// Get activation flag. Returns true if game screen is active.
    bool GetIsActive() const { return active_; }

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

    /// Return root UI element.
    /// @property
    UIElement* GetRoot() const { return rootElement_; }

    /// Set number of backbuffer viewports to render.
    /// @property
    void SetNumViewports(unsigned num);
    /// Set a backbuffer viewport.
    /// @property{set_viewports}
    void SetViewport(unsigned index, Viewport* viewport);
    /// Return backbuffer viewport by index.
    /// @property{get_viewports}
    Viewport* GetViewport(unsigned index) const;
    /// Return nth backbuffer viewport associated to a scene. Index 0 returns the first.
    Viewport* GetViewportForScene(Scene* scene, unsigned index) const;

private:
    /// Initialize mouse mode on non-web platform.
    void InitMouseMode();

    /// Handle request for mouse mode on web platform.
    void HandleMouseModeRequest(StringHash eventType, VariantMap& eventData);
    /// Handle request for mouse mode change on web platform.
    void HandleMouseModeChange(StringHash eventType, VariantMap& eventData);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

private:
    /// Is the game screen active.
    bool active_{false};
    /// UI root element.
    SharedPtr<UIElement> rootElement_{};
    /// UI root element saved upon acivation to be restored at deactivation.
    SharedPtr<UIElement> prevRootElement_{};
    /// Backbuffer viewports.
    ea::vector<SharedPtr<Viewport>> viewports_;
    /// Operating system mouse cursor visible flag.
    bool mouseVisible_{true};
    /// Flag to indicate the mouse is being grabbed by an operation. Subsystems like UI that uses mouse should
    /// temporarily ignore the mouse hover or click events.
    bool mouseGrabbed_{false};
    /// Determines the mode of mouse behaviour.
    MouseMode mouseMode_{MM_FREE};
};

class URHO3D_API GameScreenContainer
{
public:
    /// Set current game screen.
    void SetGameScreen(GameScreen* gameScreen);

    /// Get current game screen.
    GameScreen* GetGameScreen() const;

private:
    /// Current active game screen.
    SharedPtr<GameScreen> gameScreen_;
};

}
