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

#include "../Engine/SingleStateApplication.h"
#include "../Graphics/Renderer.h"
#include "../UI/UI.h"
#include "../Core/CoreEvents.h"
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

void ApplicationState::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ApplicationState>();
}

/// Activate game screen. Executed by Application.
void ApplicationState::Activate(SingleStateApplication* application)
{
    if (active_)
    {
        return;
    }

    active_ = true;
    application_ = application;

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
        prevRootElement_ = ui->GetRoot();
        ui->SetRoot(rootElement_);
    }
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer && !viewports_.empty())
        {
            renderer->SetNumViewports(viewports_.size());
            for (unsigned i = 0; i < viewports_.size();++i)
            {
                renderer->SetViewport(i, viewports_[i]);
            }
        }
    }
}
/// Handle the logic update event.
void ApplicationState::Update(float timeStep)
{
}

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
        ui->SetRoot(prevRootElement_);
    }
    {
        auto* renderer = GetSubsystem<Renderer>();
        if (renderer && !viewports_.empty())
        {
            renderer->SetNumViewports(0);
        }
    }
}

/// Set whether the operating system mouse cursor is visible.
void ApplicationState::SetMouseVisible(bool enable) {
    mouseVisible_ = enable;
    if (GetIsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseVisible(enable);
    }
}

/// Set whether the mouse is currently being grabbed by an operation.
void ApplicationState::SetMouseGrabbed(bool grab) {
    mouseGrabbed_ = grab;
    if (GetIsActive())
    {
        auto* input = GetSubsystem<Input>();
        input->SetMouseGrabbed(mouseGrabbed_);
    }
}

/// Set the mouse mode.
void ApplicationState::SetMouseMode(MouseMode mode) {
    mouseMode_ = mode;
    if (GetIsActive())
    {
        InitMouseMode();
    }
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
SingleStateApplication::SingleStateApplication(Context* context) : Application(context)
{
}

void SingleStateApplication::SetState(ApplicationState* gameScreen)
{
    if (gameScreen_)
    {
        gameScreen_->Deactivate();
    }

    gameScreen_ = gameScreen;

    if (gameScreen_)
    {
        gameScreen_->Activate(this);
    }
}

ApplicationState* SingleStateApplication::GetState() const { return gameScreen_; }


} // namespace Urho3D
