
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
    , rootElement_(context->CreateObject<UIElement>())
{
}

ApplicationState::~ApplicationState() = default;

void ApplicationState::RegisterObject(Context* context) { context->AddFactoryReflection<ApplicationState>(); }

/// Activate game screen. Executed by Application.
void ApplicationState::Activate(VariantMap& bundle)
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
        ui->SetRoot(rootElement_);
        ui->SetCustomSize(rootCustomSize_);
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

    {
        auto* ui = GetSubsystem<UI>();
        rootCustomSize_ = ui->GetCustomSize();
        ui->SetRoot(savedRootElement_);
        ui->SetCustomSize(savedRootCustomSize_);
    }
    {
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

void ApplicationState::InitMouseMode()
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
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    Update(timeStep);
}

/// Construct. Parse default engine parameters from the command line, and create the engine in an uninitialized
/// state.

StateManager::StateManager(Context* context)
    : BaseClassName(context)
{
    SubscribeToEvent(E_SETAPPLICATIONSTATE, URHO3D_HANDLER(StateManager, HandleSetApplicationState));
}

StateManager::~StateManager()
{
}

/// Set current game state.
void StateManager::EnqueueState(ApplicationState* gameScreen, VariantMap& bundle)
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
    VariantMap bundle;
    EnqueueState(gameScreen, bundle);
}

/// Transition to the application state.
void StateManager::EnqueueState(StringHash type, VariantMap& bundle)
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
    VariantMap bundle;
    EnqueueState(type, bundle);
}

/// Set current transition state and initialize related values.
void StateManager::SetTransitionState(TransitionState state)
{
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
        if (!fadeOverlay_)
        {
            fadeOverlay_ = MakeShared<Window>(context_);
            fadeOverlay_->SetLayout(LM_FREE);
            fadeOverlay_->SetAlignment(HA_CENTER, VA_CENTER);
            fadeOverlay_->SetColor(Color(0, 0, 0, 1));
            fadeOverlay_->SetPriority(ea::numeric_limits<int>::max());
            fadeOverlay_->BringToFront();
        }
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
    }
}

/// Update fade overlay size and transparency.
void StateManager::UpdateFadeOverlay(float t)
{
    auto* ui = context_->GetSubsystem<UI>();
    auto* root = ui->GetRoot();
    if (root && fadeOverlay_->GetParent() != root)
    {
        fadeOverlay_->Remove();
        root->AddChild(fadeOverlay_);
        fadeOverlay_->BringToFront();
    }
    t = Clamp(t, 0.0f, 1.0f);
    if (transitionState_ == TransitionState::FadeIn)
    {
        t = 1.0f - t;
    }
    fadeOverlay_->SetOpacity(t);
    fadeOverlay_->SetSize(ui->GetSize());
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
        return activeState_ ? activeState_->GetType() : StringHash::ZERO;
    }
    return stateQueue_.back().stateType_;
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

/// Handle SetApplicationState event and add the state to the queue.
void StateManager::HandleSetApplicationState(StringHash eventName, VariantMap& args)
{
    using namespace SetApplicationState;
    EnqueueState(args[P_STATE].GetStringHash(), args);
}

/// Handle update event to animate state transitions.
void StateManager::HandleUpdate(StringHash eventName, VariantMap& args)
{
    using namespace Update;
    auto timeStep = args[P_TIMESTEP].GetFloat();

    fadeTime_ += timeStep;

    switch (transitionState_)
    {
    case TransitionState::WaitToExit:
        if (activeState_ && activeState_->CanLeaveState())
            SetTransitionState(TransitionState::FadeOut);
        break;
    case TransitionState::FadeIn:
        if (fadeTime_ >= fadeInDuration_)
        {
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
        }
        break;
    case TransitionState::FadeOut:
        if (fadeTime_ >= fadeOutDuration_)
        {
            CreateNextState();
        }
        else
        {
            UpdateFadeOverlay(fadeTime_ / fadeOutDuration_);
        }
        break;
    }
}

/// Dequeue and set next state as active.
void StateManager::CreateNextState()
{
    if (activeState_)
    {
        activeState_->Deactivate();
    }
    activeState_.Reset();

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
                nextState = stateCacheIt->second;
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
        stateCache_[nextState->GetType()] = nextState;
        activeState_ = nextState;
        SetTransitionState(TransitionState::FadeIn);
        activeState_->Activate(nextQueueItem.bundle_);
        UpdateFadeOverlay(0.0f);
        return;
    }

    SetTransitionState(TransitionState::Sustain);
}


} // namespace Urho3D
