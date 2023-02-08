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

#include "../Engine/Application.h"
#include "../Core/Context.h"
#include "../Input/Input.h"
#include "../Graphics/Viewport.h"
#include "Urho3D/UI/Window.h"
#if URHO3D_ACTIONS
#include "../Actions/ActionManager.h"
#endif

#include <EASTL/queue.h>

namespace Urho3D
{
class ResourceCache;
class UI;

/// Base class for an application state. Examples of a state would be a loading screen, a menu or a game screen.
class URHO3D_API ApplicationState : public Object
{
    URHO3D_OBJECT(ApplicationState, Object)

public:
    /// Construct.
    explicit ApplicationState(Context* context);
    /// Destruct.
    virtual ~ApplicationState() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Activate game state. Executed by StateManager.
    virtual void Activate(StringVariantMap& bundle);

    /// Transition into the state complete. Executed by StateManager.
    virtual void TransitionComplete();

    /// Return true if state is ready to be deactivated. Executed by StateManager.
    virtual bool CanLeaveState() const;

    /// Transition out of the state started. Executed by StateManager.
    virtual void TransitionStarted();

    /// Deactivate game state. Executed by StateManager.
    virtual void Deactivate();

    /// Handle the logic update event.
    virtual void Update(float timeStep);

    /// Get activation flag. Returns true if game screen is active.
    bool IsActive() const { return active_; }

    /// Set whether the operating system mouse cursor is visible.
    void SetMouseVisible(bool enable);
    /// Set whether the mouse is currently being grabbed by an operation.
    void SetMouseGrabbed(bool grab);
    /// Set the mouse mode.
    void SetMouseMode(MouseMode mode);

    /// Set cursor UI element.
    void SetCursor(Cursor* cursor);
    /// Return cursor.
    Cursor* GetCursor() const { return cursor_; }

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
    UIElement* GetUIRoot() const { return rootElement_; }
    /// Set custom size of the root element. This disables automatic resizing of the root element according to window
    /// size. Set custom size 0,0 to return to automatic resizing.
    /// @property
    void SetUICustomSize(const IntVector2& size);
    /// Set custom size of the root element.
    void SetUICustomSize(int width, int height);
    /// Return root element custom size. Returns 0,0 when custom size is not being used and automatic resizing according
    /// to window size is in use instead (default).
    /// @property
    const IntVector2& GetUICustomSize() const { return rootCustomSize_; }

    /// Set number of backbuffer viewports to render.
    /// @property
    void SetNumViewports(unsigned num);
    /// Set a backbuffer viewport.
    /// @property{set_viewports}
    void SetViewport(unsigned index, Viewport* viewport);
    /// Set default zone fog color.
    void SetDefaultFogColor(const Color& color);

    /// Return backbuffer viewport by index.
    /// @property{get_viewports}
    Viewport* GetViewport(unsigned index) const;
    /// Return nth backbuffer viewport associated to a scene. Index 0 returns the first.
    Viewport* GetViewportForScene(Scene* scene, unsigned index) const;
    /// Get default zone fog color.
    const Color& GetDefaultFogColor() const { return fogColor_; }

#if URHO3D_ACTIONS
    /// Return application state's action manager.
    ActionManager* GetActionManager() const { return actionManager_; }

    /// Add action to the state's action manager.
    Actions::ActionState* AddAction(Actions::BaseAction* action, Object* target, bool paused = false);
#endif

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
    /// UI root element saved upon activation to be restored at deactivation.
    SharedPtr<UIElement> savedRootElement_{};
    /// UI cursor.
    SharedPtr<Cursor> cursor_{};
    /// UI cursor saved from previous state.
    SharedPtr<Cursor> savedCursor_{};
    /// UI root element custom size.
    IntVector2 rootCustomSize_{};
    /// UI root element custom size saved upon activation to be restored at deactivation.
    IntVector2 savedRootCustomSize_{};
    /// Backbuffer viewports.
    ea::vector<SharedPtr<Viewport>> viewports_;
    /// Operating system mouse cursor visible flag.
    bool mouseVisible_{true};
    /// Flag to indicate the mouse is being grabbed by an operation. Subsystems like UI that uses mouse should
    /// temporarily ignore the mouse hover or click events.
    bool mouseGrabbed_{false};
    /// Determines the mode of mouse behaviour.
    MouseMode mouseMode_{MM_FREE};
    /// Fog color.
    Color fogColor_{0.0f, 0.0f, 0.0f};
    /// Saved fog color to be restored at deactivation.
    Color savedFogColor_{};
#if URHO3D_ACTIONS
    /// Local action manager.
    SharedPtr<ActionManager> actionManager_;
#endif
};

class URHO3D_API StateManager: public Object
{
    URHO3D_OBJECT(StateManager, Object);

    /// Queue item
    struct QueueItem
    {
        /// Target state if set by pointer to an object.
        SharedPtr<ApplicationState> state_;
        /// Target state if set by type StringHash.
        StringHash stateType_;
        /// Target state arguments.
        StringVariantMap bundle_;
    };

    enum class TransitionState
    {
        Sustain,
        FadeOut,
        FadeIn,
        WaitToExit,
    };

public:
    /// Construct. Parse default engine parameters from the command line, and create the engine in an uninitialized
    /// state.
    explicit StateManager(Context* context);

    virtual ~StateManager() override;

    /// Hard reset of state manager. Current state will be set to nullptr and the queue is purged.
    void Reset();
    /// Update state manager. This is called automatically by the engine every frame.
    void Update(float timeStep);

    /// Transition to the application state.
    /// @{
    void EnqueueState(ApplicationState* gameScreen, StringVariantMap& bundle);
    void EnqueueState(ApplicationState* gameScreen);
    void EnqueueState(StringHash type, StringVariantMap& bundle);
    void EnqueueState(StringHash type);
    template <typename T> void EnqueueState(StringVariantMap& bundle);
    template <typename T> void EnqueueState();
    /// @}

    /// Get current application state.
    ApplicationState* GetState() const;
    /// Get target application state.
    StringHash GetTargetState() const;

    /// Get fade overlay
    Window* GetFadeOverlay();

    /// Set fade in animation duration;
    void SetFadeInDuration(float durationInSeconds);
    /// Set fade out animation duration;
    void SetFadeOutDuration(float durationInSeconds);
    /// Get fade in animation duration;
    float GetFadeInDuration() const { return fadeInDuration_; }
    /// Get fade out animation duration;
    float GetFadeOutDuration() const { return fadeOutDuration_; }

private:
    /// Initiate state transition if necessary.
    void InitiateTransition();

    /// Handle update event to animate state transitions.
    void HandleUpdate(StringHash eventName, VariantMap& args);

    /// Set current transition state and initialize related values.
    void SetTransitionState(TransitionState state);

    /// Update fade overlay size and transparency.
    void UpdateFadeOverlay(float t);

    /// Start transition out of the current state.
    void StartTransition();

    /// Complete transition into the current state.
    void CompleteTransition();

    /// Deactivate state.
    void DeactivateState();

    /// Dequeue and set next state as active.
    void CreateNextState();

    /// Notify subscribers about transition state updates.
    void Notify(StringHash eventType);

    /// Origin state.
    StringHash originState_{StringHash::Empty};
    /// Destination state.
    StringHash destinationState_{StringHash::Empty};

    /// Cache of previously created states.
    ea::unordered_map<StringHash, WeakPtr<ApplicationState>> stateCache_;

    /// Queue of application states.
    ea::queue<QueueItem> stateQueue_;

    /// Current active game screen.
    SharedPtr<ApplicationState> activeState_;

    /// Fade animation time.
    float fadeTime_{};

    /// Fade in duration in seconds.
    float fadeInDuration_{ea::numeric_limits<float>::epsilon()};
    /// Fade out duration in seconds.
    float fadeOutDuration_{ea::numeric_limits<float>::epsilon()};

    /// Fade overlay window.
    SharedPtr<Window> fadeOverlay_;

    /// State transition state.
    TransitionState transitionState_{TransitionState::Sustain};
};

template <class T> void StateManager::EnqueueState(StringVariantMap& bundle)
{
    EnqueueState(T::GetTypeStatic(), bundle);
}

template <class T> void StateManager::EnqueueState()
{
    StringVariantMap bundle;
    EnqueueState<T>(bundle);
}

}
