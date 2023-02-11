
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

#include "../Precompiled.h"

#include "../Engine/StateManager.h"
#include "../Engine/StateManagerEvents.h"
#include "../Core/CoreEvents.h"
#include "../Core/Thread.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Zone.h"
#include "../UI/UI.h"
#include "../Resource/ResourceCache.h"
#if URHO3D_SYSTEMUI
    #include "../SystemUI/Console.h"
#endif

namespace Urho3D
{

ApplicationState::ApplicationState(Context* context)
    : Object(context)
    , rootElement_(MakeShared<UIElement>(context))
#if URHO3D_ACTIONS
    , actionManager_(MakeShared<ActionManager>(context, false))
#endif
{
}

ApplicationState::~ApplicationState() = default;

void ApplicationState::RegisterObject(Context* context) { context->AddFactoryReflection<ApplicationState>(); }

/// Activate game screen. Executed by Application.
void ApplicationState::Activate(StringVariantMap& bundle)
{
    if (active_)
    {
        return;
    }

    active_ = true;

    // Subscribe HandleUpdate() method for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ApplicationState, HandleUpdate));

    {
        auto* input = GetSubsystem<Input>();
        InitMouseMode();
        input->SetMouseVisible(mouseVisible_);
        input->SetMouseGrabbed(mouseGrabbed_);
    }
    {
        auto* ui = GetSubsystem<UI>();
        savedRootElement_ = ui->GetRoot();
        savedRootCustomSize_ = ui->GetCustomSize();
        savedCursor_ = ui->GetCursor();
        ui->SetRoot(rootElement_);
        ui->SetCustomSize(rootCustomSize_);
        ui->SetCursor(cursor_);
    }
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            savedFogColor_ = renderer->GetDefaultZone()->GetFogColor();
            renderer->GetDefaultZone()->SetFogColor(fogColor_);

            if (!viewports_.empty())
            {
                renderer->SetNumViewports(viewports_.size());
                for (unsigned i = 0; i < viewports_.size(); ++i)
                {
                    renderer->SetViewport(i, viewports_[i]);
                }
            }
        }
    }
}


/// Transition into the state complete. Executed by StateManager.
void ApplicationState::TransitionComplete()
{
}

/// Transition out of the state started. Executed by StateManager.
void ApplicationState::TransitionStarted()
{
}

/// Return true if state is ready to be deactivated. Executed by StateManager.
bool ApplicationState::CanLeaveState() const
{
    return true;
}


/// Handle the logic update event.
void ApplicationState::Update(float timeStep) {}

/// Deactivate game screen. Executed by Application.
void ApplicationState::Deactivate()
{
    if (!active_)
    {
        return;
    }
    active_ = false;

    // Subscribe HandleUpdate() method for processing update events
    UnsubscribeFromEvent(E_UPDATE);

    auto* ui = GetSubsystem<UI>();
    if (ui)
    {
        rootCustomSize_ = ui->GetCustomSize();
        cursor_ = ui->GetCursor();
        ui->SetRoot(savedRootElement_);
        ui->SetCustomSize(savedRootCustomSize_);
        ui->SetCursor(savedCursor_);
    }
    auto* renderer = GetSubsystem<Renderer>();
    if (renderer)
    {
        fogColor_ = renderer->GetDefaultZone()->GetFogColor();
        renderer->GetDefaultZone()->SetFogColor(savedFogColor_);

        if (!viewports_.empty())
        {
            renderer->SetNumViewports(0);
        }
    }
}

/// Set whether the operating system mouse cursor is visible.
void ApplicationState::SetMouseVisible(bool enable)
{
    mouseVisible_ = enable;
    if (IsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseVisible(enable);
    }
}

/// Set whether the mouse is currently being grabbed by an operation.
void ApplicationState::SetMouseGrabbed(bool grab)
{
    mouseGrabbed_ = grab;
    if (IsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseGrabbed(mouseGrabbed_);
    }
}

/// Set the mouse mode.
void ApplicationState::SetMouseMode(MouseMode mode)
{
    mouseMode_ = mode;
    if (IsActive())
    {
        InitMouseMode();
    }
}

/// Set cursor UI element.
void ApplicationState::SetCursor(Cursor* cursor)
{
    cursor_ = cursor;
    if (IsActive())
    {
        auto* ui = GetSubsystem<UI>();
        ui->SetCursor(cursor);
    }
}

void ApplicationState::SetUICustomSize(const IntVector2& size)
{
    rootCustomSize_ = size;
    if (IsActive())
    {
        auto* ui = GetSubsystem<UI>();
        ui->SetCustomSize(size);
    }
}

void ApplicationState::SetUICustomSize(int width, int height) { SetUICustomSize(IntVector2(width, height)); }

void ApplicationState::SetNumViewports(unsigned num)
{
    viewports_.resize(num);
    if (active_)
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            renderer->SetNumViewports(num);
        }
    }
}

void ApplicationState::SetViewport(unsigned index, Viewport* viewport)
{
    if (index >= viewports_.size())
        viewports_.resize(index + 1);

    viewports_[index] = viewport;
    if (active_)
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            renderer->SetViewport(index, viewport);
        }
    }
}

void ApplicationState::SetDefaultFogColor(const Color& color)
{
    fogColor_ = color;
    if (active_)
    {
        const auto* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            renderer->GetDefaultZone()->SetFogColor(color);
        }
    }
}

Viewport* ApplicationState::GetViewport(unsigned index) const
{
    return index < viewports_.size() ? viewports_[index] : nullptr;
}

Viewport* ApplicationState::GetViewportForScene(Scene* scene, unsigned index) const
{
    for (unsigned i = 0; i < viewports_.size(); ++i)
    {
        Viewport* viewport = viewports_[i];
        if (viewport && viewport->GetScene() == scene)
        {
            if (index == 0)
                return viewport;
            else
                --index;
        }
    }
    return nullptr;
}

#if URHO3D_ACTIONS
/// Add action to the state's action manager.
Actions::ActionState* ApplicationState::AddAction(Actions::BaseAction* action, Object* target, bool paused)
{
    return actionManager_->AddAction(action, target, paused);
}
#endif

void ApplicationState::InitMouseMode()
{
    Input* input = GetSubsystem<Input>();

    if (GetPlatform() != PlatformId::Web)
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
        SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(ApplicationState, HandleMouseModeRequest));
        SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(ApplicationState, HandleMouseModeChange));
    }
}

// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void ApplicationState::HandleMouseModeRequest(StringHash /*eventType*/, VariantMap& eventData)
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

void ApplicationState::HandleMouseModeChange(StringHash /*eventType*/, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}

void ApplicationState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Update can be still executed if Deacivate was called during calling Update subscribers.
    if (!active_)
    {
        return;
    }

    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

#if URHO3D_ACTIONS
    actionManager_->Update(timeStep);
#endif

    // Move the camera, scale movement with time step
    Update(timeStep);
}

/// Construct. Parse default engine parameters from the command line, and create the engine in an uninitialized
/// state.

StateManager::StateManager(Context* context)
    : BaseClassName(context)
{
}

StateManager::~StateManager()
{
    Reset();
}
/// Start transition out of current state.
void StateManager::StartTransition()
{
    if (activeState_)
    {
        originState_ = activeState_->GetType();
        activeState_->TransitionStarted();
    }
    else
    {
        originState_ = StringHash::Empty;
    }
    Notify(E_STATETRANSITIONSTARTED);
}

/// Complete transition into the current state.
void StateManager::CompleteTransition()
{
    if (activeState_)
    {
        activeState_->TransitionComplete();
    }
    Notify(E_STATETRANSITIONCOMPLETE);
}

/// Deactivate state.
void StateManager::DeactivateState()
{
    if (activeState_)
    {
        Notify(E_LEAVINGAPPLICATIONSTATE);

        activeState_->Deactivate();
        activeState_.Reset();
    }
}

/// Hard reset of state manager. Current state will be set to nullptr and the queue is purged.
void StateManager::Reset()
{
    const bool hasState = activeState_;
    if (hasState)
    {
        destinationState_ = StringHash::Empty;
        StartTransition();
        DeactivateState();
    }
    ea::queue<QueueItem> emptyQueue;
    std::swap(stateQueue_, emptyQueue);

    SetTransitionState(TransitionState::Sustain);
    if (hasState)
    {
        CompleteTransition();
    }
}

/// Set current game state.
void StateManager::EnqueueState(ApplicationState* gameScreen, StringVariantMap& bundle)
{
    if (!gameScreen)
    {
        URHO3D_LOGERROR("No target state provided");
        return;
    }
    if (!Thread::IsMainThread())
    {
        URHO3D_LOGERROR("State transition could only be scheduled from the main thread");
        return;
    }
    stateQueue_.push(QueueItem{SharedPtr<ApplicationState>(gameScreen), gameScreen->GetType(), bundle});
    InitiateTransition();
}

/// Transition to the application state.
void StateManager::EnqueueState(ApplicationState* gameScreen)
{
    StringVariantMap bundle;
    EnqueueState(gameScreen, bundle);
}

/// Transition to the application state.
void StateManager::EnqueueState(StringHash type, StringVariantMap& bundle)
{
    if (!Thread::IsMainThread())
    {
        URHO3D_LOGERROR("State transition could only be scheduled from the main thread");
        return;
    }
    stateQueue_.push(QueueItem{SharedPtr<ApplicationState>(nullptr), type, bundle});
    InitiateTransition();
}

/// Transition to the application state.
void StateManager::EnqueueState(StringHash type)
{
    StringVariantMap bundle;
    EnqueueState(type, bundle);
}

/// Set current transition state and initialize related values.
void StateManager::SetTransitionState(TransitionState state)
{
    if (transitionState_ == state)
    {
        return;
    }

    transitionState_ = state;

    switch (transitionState_)
    {
    case TransitionState::Sustain:
        UnsubscribeFromEvent(E_UPDATE);
        if (fadeOverlay_)
        {
            fadeOverlay_->Remove();
        }
        break;
    case TransitionState::FadeIn:
    case TransitionState::FadeOut:
        fadeTime_ = 0.0f;
        UpdateFadeOverlay(0.0f);
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(StateManager, HandleUpdate));
        break;
    case TransitionState::WaitToExit:
        if (fadeOverlay_)
        {
            fadeOverlay_->Remove();
        }
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(StateManager, HandleUpdate));
        break;
    default:
        break;
    }
    if (transitionState_ == TransitionState::FadeOut)
    {
        if (stateQueue_.empty())
            destinationState_ = StringHash::Empty;
        else
            destinationState_ = stateQueue_.front().stateType_;

        StartTransition();
    }
}

/// Update fade overlay size and transparency.
void StateManager::UpdateFadeOverlay(float t)
{
    Window* overlay = GetFadeOverlay();
    auto* ui = context_->GetSubsystem<UI>();
    auto* root = ui->GetRoot();
    if (root && overlay->GetParent() != root)
    {
        overlay->Remove();
        root->AddChild(overlay);
        overlay->BringToFront();
    }
    t = Clamp(t, 0.0f, 1.0f);
    if (transitionState_ == TransitionState::FadeIn)
    {
        t = 1.0f - t;
    }
    overlay->SetOpacity(t);
    overlay->SetSize(ui->GetSize());
}


/// Notify subscribers about transition state updates.
void StateManager::Notify(StringHash eventType)
{
    // All transition events has same set of arguments so we can re-use one from LeavingApplicationState.
    using namespace LeavingApplicationState;
    auto& data = context_->GetEventDataMap();
    data[P_FROM] = originState_;
    data[P_TO] = destinationState_;
    SendEvent(eventType, data);

    if (eventType == E_STATETRANSITIONCOMPLETE)
    {
        originState_ = destinationState_;
        destinationState_ = StringHash::Empty;
    }
}

/// Initiate state transition if necessary.
void StateManager::InitiateTransition()
{
    if (stateQueue_.empty())
    {
        SetTransitionState(TransitionState::Sustain);
        return;
    }

    if (transitionState_ == TransitionState::Sustain)
    {
        if (activeState_ != nullptr)
        {
            SetTransitionState(TransitionState::FadeOut);
        }
        else
        {
            CreateNextState();
        }
    }
}

ApplicationState* StateManager::GetState() const { return activeState_; }

/// Get target application state.
StringHash StateManager::GetTargetState() const
{
    if (stateQueue_.empty())
    {
        return activeState_ ? activeState_->GetType() : StringHash::Empty;
    }
    return stateQueue_.back().stateType_;
}

/// Get fade overlay
Window* StateManager::GetFadeOverlay()
{
    if (!fadeOverlay_)
    {
        fadeOverlay_ = MakeShared<Window>(context_);
        fadeOverlay_->SetLayout(LM_FREE);
        fadeOverlay_->SetAlignment(HA_CENTER, VA_CENTER);
        fadeOverlay_->SetColor(Color(0, 0, 0, 1));
        fadeOverlay_->SetPriority(ea::numeric_limits<int>::max());
        fadeOverlay_->BringToFront();
    }
    return fadeOverlay_;
}

/// Set fade in animation duration;
void StateManager::SetFadeInDuration(float durationInSeconds)
{
    fadeInDuration_ = Clamp(durationInSeconds, ea::numeric_limits<float>::epsilon(), ea::numeric_limits<float>::max());
}
/// Set fade out animation duration;
void StateManager::SetFadeOutDuration(float durationInSeconds)
{
    fadeOutDuration_ = Clamp(durationInSeconds, ea::numeric_limits<float>::epsilon(), ea::numeric_limits<float>::max());
}

/// Handle update event to animate state transitions.
void StateManager::HandleUpdate(StringHash eventName, VariantMap& args)
{
    using namespace Update;
    const auto timeStep = args[P_TIMESTEP].GetFloat();
    Update(timeStep);
}

void StateManager::Update(float timeStep)
{
    int iterationCount{0};
    do
    {
        switch (transitionState_)
        {
        case TransitionState::Sustain:
            return;
        case TransitionState::WaitToExit:
            if (activeState_ && activeState_->CanLeaveState())
                SetTransitionState(TransitionState::FadeOut);
            else
                return;
            break;
        case TransitionState::FadeIn:
            fadeTime_ += timeStep;
            if (fadeTime_ >= fadeInDuration_)
            {
                timeStep = fadeTime_ - fadeInDuration_;
                CompleteTransition();

                if (stateQueue_.empty())
                    SetTransitionState(TransitionState::Sustain);
                else if (activeState_ && !activeState_->CanLeaveState())
                    SetTransitionState(TransitionState::WaitToExit);
                else
                    SetTransitionState(TransitionState::FadeOut);
            }
            else
            {
                UpdateFadeOverlay(fadeTime_ / fadeInDuration_);
                return;
            }
            break;
        case TransitionState::FadeOut:
            fadeTime_ += timeStep;
            if (fadeTime_ >= fadeOutDuration_)
            {
                timeStep = fadeTime_ - fadeInDuration_;
                CreateNextState();
            }
            else
            {
                UpdateFadeOverlay(fadeTime_ / fadeOutDuration_);
                return;
            }
            break;
        }
        // Limit number of actions per frame
        ++iterationCount;
    } while (timeStep > 0.0f && iterationCount < 16);
}

/// Dequeue and set next state as active.
void StateManager::CreateNextState()
{
    DeactivateState();

    while (!stateQueue_.empty())
    {
        QueueItem nextQueueItem = stateQueue_.front();
        stateQueue_.pop();
        SharedPtr<ApplicationState> nextState = nextQueueItem.state_;
        if (!nextState)
        {
            auto stateCacheIt = stateCache_.find(nextQueueItem.stateType_);
            if (stateCacheIt != stateCache_.end() && !stateCacheIt->second.Expired())
            {
                nextState = stateCacheIt->second.Lock();
            }
            else
            {
                nextState.DynamicCast(context_->CreateObject(nextQueueItem.stateType_));
                if (!nextState)
                {
                    URHO3D_LOGERROR("Can't create application state object");
                    continue;
                }
            }
        }
        destinationState_ = nextState->GetType();
        stateCache_[destinationState_] = nextState;

        if (originState_ == StringHash::Empty)
        {
            StartTransition();
        }
        activeState_ = nextState;
        Notify(E_ENTERINGAPPLICATIONSTATE);

        SetTransitionState(TransitionState::FadeIn);
        activeState_->Activate(nextQueueItem.bundle_);
        UpdateFadeOverlay(0.0f);
        return;
    }
    destinationState_ = StringHash::Empty;
    SetTransitionState(TransitionState::Sustain);
    CompleteTransition();
}


} // namespace Urho3D
