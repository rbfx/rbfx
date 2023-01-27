//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "InputLogger.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UI.h>

#include <EASTL/unordered_set.h>

URHO3D_DEFINE_PLUGIN_MAIN(Urho3D::InputLogger);

namespace Urho3D
{

InputLogger::InputLogger(Context* context)
    : MainPluginApplication(context)
{
}

void InputLogger::Load()
{
}

void InputLogger::Unload()
{
}

InputLogger::ViewportData InputLogger::CreateViewport(const Color& color, const IntRect& rect) const
{
    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();

    // Create zone
    Node* zoneNode = scene->CreateChild("Zone");
    auto zone = zoneNode->CreateComponent<Zone>();
    zone->SetFogColor(color);

    // Create camera
    Node* cameraNode = scene->CreateChild("Camera");
    auto camera = cameraNode->CreateComponent<Camera>();

    auto viewport = MakeShared<Viewport>(context_, scene, camera, rect);
    return {viewport, scene, camera};
}

void InputLogger::Start(bool isMain)
{
    if (!isMain)
        return;

    auto cache = GetSubsystem<ResourceCache>();
    auto input = GetSubsystem<Input>();
    auto renderer = GetSubsystem<Renderer>();
    auto ui = GetSubsystem<UI>();

    // Create viewports
    viewports_[0] = CreateViewport(0x0047ab_rgb, IntRect::ZERO);
    viewports_[1] = CreateViewport(0x001167_rgb, IntRect{350, 50, 450, 100});

    // Create UI
    auto uiRoot = ui->GetRoot();
    auto font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    text_ = uiRoot->CreateChild<Text>();
    text_->SetFont(font, 12);
    text_->SetHorizontalAlignment(HA_LEFT);
    text_->SetVerticalAlignment(VA_TOP);

    // Setup engine state
    renderer->SetNumViewports(2);
    renderer->SetViewport(0, viewports_[0].viewport_);
    renderer->SetViewport(1, viewports_[1].viewport_);
    input->SetMouseVisible(true);
    input->SetMouseMode(MM_FREE);
    //input->SetMouseMode(MM_WRAP);

    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap&) { Update(); });

    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_TEXTINPUT, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_TEXTEDITING, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKCONNECTED, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKBUTTONUP, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_GESTURERECORDED, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_GESTUREINPUT, URHO3D_HANDLER(InputLogger, OnInputEvent));
    SubscribeToEvent(E_MULTIGESTURE, URHO3D_HANDLER(InputLogger, OnInputEvent));
}

void InputLogger::OnInputEvent(StringHash eventType, VariantMap& eventData)
{
    const LoggedEvent event = DecodeEvent(eventType, eventData);
    if (!MergeEvent(event))
        AddEvent(event);
    UpdateText();
}

InputLogger::LoggedEvent InputLogger::DecodeEvent(StringHash eventType, VariantMap& eventData) const
{
    static const ea::unordered_set<StringHash> enabledParams{
        MouseButtonDown::P_BUTTON,
        MouseButtonDown::P_CLICKS,
        MouseMove::P_X,
        MouseMove::P_Y,
        MouseWheel::P_WHEEL,
        KeyDown::P_KEY,
        KeyDown::P_REPEAT,
        TextInput::P_TEXT,
        JoystickConnected::P_JOYSTICKID,
        JoystickAxisMove::P_AXIS,
        JoystickAxisMove::P_POSITION,
        JoystickHatMove::P_HAT,
        TouchBegin::P_TOUCHID,
        TouchBegin::P_PRESSURE,
        GestureRecorded::P_GESTUREID,
        GestureInput::P_CENTERX,
        GestureInput::P_CENTERY,
        GestureInput::P_NUMFINGERS,
        GestureInput::P_ERROR,
        MultiGesture::P_DTHETA,
        MultiGesture::P_DDIST,
    };

    LoggedEvent event;
    event.count_ = 1;
    event.timeStamp_ = Time::GetSystemTime();
    event.eventType_ = GetEventNameRegister().GetString(eventType);
    for (const auto& [nameHash, value] : eventData)
    {
        if (enabledParams.contains(nameHash))
        {
            const ea::string& name = GetEventParamRegister().GetString(nameHash);
            event.parameters_[name] = value.ToString();
        }
    }
    return event;
}

bool InputLogger::MergeEvent(const LoggedEvent& event)
{
    const auto sameEventIter = ea::find_if(eventLog_.begin(), eventLog_.end(),
        [&](const LoggedEvent& existingEvent) { return CanMergeEventWith(event, existingEvent); });
    if (sameEventIter == eventLog_.end())
        return false;

    ++sameEventIter->count_;
    sameEventIter->parameters_ = event.parameters_;
    sameEventIter->timeStamp_ = event.timeStamp_;
    return true;
}

bool InputLogger::CanMergeEventWith(const LoggedEvent& event, const LoggedEvent& existingEvent) const
{
    static const ea::unordered_set<StringHash> mergeableEvents{
        E_KEYDOWN,
        E_TEXTINPUT,
        E_MOUSEMOVE,
        E_MOUSEWHEEL,
        E_JOYSTICKBUTTONDOWN,
        E_JOYSTICKAXISMOVE,
        E_JOYSTICKHATMOVE,
        E_TOUCHMOVE,
    };

    static const unsigned mergeThresholdMs = 250;
    if (event.eventType_ != existingEvent.eventType_ || !mergeableEvents.contains(event.eventType_))
        return false;
    if (event.timeStamp_ - existingEvent.timeStamp_ > mergeThresholdMs)
        return false;

    static const ea::unordered_set<StringHash> ignoredParams{
        MouseButtonDown::P_CLICKS,
        MouseMove::P_X,
        MouseMove::P_Y,
        JoystickAxisMove::P_POSITION,
    };

    for (const auto& [name, oldValue] : existingEvent.parameters_)
    {
        if (ignoredParams.contains(StringHash{name}))
            continue;
        const auto newIter = event.parameters_.find(name);
        if (newIter == event.parameters_.end() || newIter->second != oldValue)
            return false;
    }

    return true;
}

void InputLogger::AddEvent(const LoggedEvent& event)
{
    eventLog_.push_front(event);
    while (eventLog_.size() > 100)
        eventLog_.pop_back();
}

void InputLogger::Update()
{
    auto ui = GetSubsystem<UI>();
    auto uiRoot = ui->GetRoot();
    const auto screenSize = uiRoot->GetSize();

    const float padding = 32;
    text_->SetPosition(padding, padding);
    text_->SetSize(screenSize.x_ - padding, screenSize.y_ - padding);
}

void InputLogger::UpdateText()
{
    const auto input = GetSubsystem<Input>();
    const IntVector2 mousePosition = input->GetMousePosition();
    const IntVector2 backbufferSize = input->GetBackbufferSize();

    ea::string text;

    text += Format("Mouse Position: {} {} / {} {}\n",
        mousePosition.x_, mousePosition.y_, backbufferSize.x_, backbufferSize.y_);

    for (int index : {0, 1})
    {
        const Vector2 position = viewports_[index].camera_->GetMousePosition();
        const bool isInside = Rect::POSITIVE.IsInside(position) != OUTSIDE;
        text += Format("- relative to Viewport #{}: ({}) {}\n", index, isInside ? "in" : "out", position.ToString());
    }

    text += "\n";

    for (const LoggedEvent& event : eventLog_)
    {
        text += event.eventType_;
        if (event.count_ > 1)
            text += Format(" x{}", event.count_);
        if (!event.parameters_.empty())
        {
            text += " (";
            bool firstParam = true;
            for (const auto& [name, value] : event.parameters_)
            {
                if (!firstParam)
                    text += ", ";
                text += name;
                text += "=";
                text += value;
                firstParam = false;
            }
            text += ")";
        }
        text += "\n";
    }

    text_->SetText(text);
}

void InputLogger::Stop()
{
    auto renderer = GetSubsystem<Renderer>();
    renderer->SetNumViewports(0);

    viewports_ = {};
}

}
