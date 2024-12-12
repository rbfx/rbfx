// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Engine/StateManager.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Engine/StateManagerEvents.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Zone.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/UI/UI.h"
#if URHO3D_SYSTEMUI
    #include "Urho3D/SystemUI/Console.h"
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

void ApplicationState::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ApplicationState>();
}

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

void ApplicationState::TransitionComplete()
{
}

void ApplicationState::TransitionStarted()
{
}

bool ApplicationState::CanLeaveState() const
{
    return true;
}

void ApplicationState::Update(float timeStep)
{
}

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

void ApplicationState::SetMouseVisible(bool enable)
{
    mouseVisible_ = enable;
    if (IsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseVisible(enable);
    }
}

void ApplicationState::SetMouseGrabbed(bool grab)
{
    mouseGrabbed_ = grab;
    if (IsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseGrabbed(mouseGrabbed_);
    }
}

void ApplicationState::SetMouseMode(MouseMode mode)
{
    mouseMode_ = mode;
    if (IsActive())
    {
        InitMouseMode();
    }
}

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

void ApplicationState::SetUICustomSize(int width, int height)
{
    SetUICustomSize(IntVector2(width, height));
}

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

StateManager::StateManager(Context* context)
    : BaseClassName(context)
{
}

StateManager::~StateManager()
{
    Reset();
}

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

void StateManager::CompleteTransition()
{
    if (activeState_)
    {
        activeState_->TransitionComplete();
    }
    Notify(E_STATETRANSITIONCOMPLETE);
}

void StateManager::DeactivateState()
{
    if (activeState_)
    {
        Notify(E_LEAVINGAPPLICATIONSTATE);

        activeState_->Deactivate();
        activeState_.Reset();
    }
}

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

void StateManager::EnqueueState(ApplicationState* gameScreen)
{
    StringVariantMap bundle;
    EnqueueState(gameScreen, bundle);
}

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

void StateManager::EnqueueState(StringHash type)
{
    StringVariantMap bundle;
    EnqueueState(type, bundle);
}

ApplicationState* StateManager::GetCachedState(StringHash type) const
{
    const auto iter = stateCache_.find(type);
    return iter != stateCache_.end() ? iter->second : nullptr;
}

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
    default: break;
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

ApplicationState* StateManager::GetState() const
{
    return activeState_;
}

StringHash StateManager::GetTargetState() const
{
    if (stateQueue_.empty())
    {
        return activeState_ ? activeState_->GetType() : StringHash::Empty;
    }
    return stateQueue_.back().stateType_;
}

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

void StateManager::SetFadeInDuration(float durationInSeconds)
{
    fadeInDuration_ = Clamp(durationInSeconds, ea::numeric_limits<float>::epsilon(), ea::numeric_limits<float>::max());
}

void StateManager::SetFadeOutDuration(float durationInSeconds)
{
    fadeOutDuration_ = Clamp(durationInSeconds, ea::numeric_limits<float>::epsilon(), ea::numeric_limits<float>::max());
}

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
        case TransitionState::Sustain: //
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

void StateManager::CacheState(ApplicationState* state)
{
    stateCache_[state->GetType()] = state;
}

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
