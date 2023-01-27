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

#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "AggregatedInput.h"

#include <Urho3D/DebugNew.h>

AggregatedInput::AggregatedInput(Context* context)
    : Sample(context)
    , aggregatedInput_(context)
    , dpadInput_(context)
{
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void AggregatedInput::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create UI elements
    CreateUI();

    // Finally subscribe to the update event. Note that by subscribing events at this point we have already missed some
    // events like the ScreenMode event sent by the Graphics subsystem when opening the application window. To catch
    // those as well we could subscribe in the constructor instead.
    SubscribeToEvents();
}

void AggregatedInput::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto uiRoot = GetUIRoot();

    rawEventsLog_ = uiRoot->CreateChild<Text>();
    rawEventsLog_->SetFont(font, 10);
    rawEventsLog_->SetHorizontalAlignment(HA_LEFT);
    rawEventsLog_->SetVerticalAlignment(VA_CENTER);

    filteredEventsLog_ = uiRoot->CreateChild<Text>();
    filteredEventsLog_->SetFont(font, 10);
    filteredEventsLog_->SetHorizontalAlignment(HA_LEFT);
    filteredEventsLog_->SetVerticalAlignment(VA_CENTER);

    upMarker_ = uiRoot->CreateChild<Sprite>();
    upMarker_->SetSize(40, 40);
    upMarker_->SetColor(Color::YELLOW);
    upMarker_->SetEnabled(false);

    leftMarker_ = uiRoot->CreateChild<Sprite>();
    leftMarker_->SetSize(40, 40);
    leftMarker_->SetColor(Color::BLUE);
    leftMarker_->SetEnabled(false);

    rightMarker_ = uiRoot->CreateChild<Sprite>();
    rightMarker_->SetSize(40, 40);
    rightMarker_->SetColor(Color::RED);
    rightMarker_->SetEnabled(false);

    downMarker_ = uiRoot->CreateChild<Sprite>();
    downMarker_->SetSize(40, 40);
    downMarker_->SetColor(Color::GREEN);
    downMarker_->SetEnabled(false);

    analogPivot_ = uiRoot->CreateChild<Sprite>();
    analogPivot_->SetSize(24, 24);
    analogPivot_->SetColor(Color::GRAY);
    analogMarker_ = uiRoot->CreateChild<Sprite>();
    analogMarker_->SetSize(20, 20);

    // Construct new Text object
    SharedPtr<Text> helloText(MakeShared<Text>(context_));

    // Set String to display

    helloText->SetText("Move marker around with keyboard, joystick or touch");

    // Set font and text color
    helloText->SetFont(font, 15);

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_TOP);

    // Add Text instance to the UI root element
    uiRoot->AddChild(helloText);
}

void AggregatedInput::SubscribeToEvents()
{
    auto* input = context_->GetSubsystem<Input>();
    aggregatedInput_.SetEnabled(true);
    dpadInput_.SetEnabled(true);
    SubscribeToEvent(&dpadInput_, E_KEYUP, URHO3D_HANDLER(AggregatedInput, HandleDPadKeyUp));
    SubscribeToEvent(&dpadInput_, E_KEYDOWN, URHO3D_HANDLER(AggregatedInput, HandleDPadKeyDown));
    SubscribeToEvent(input, E_KEYUP, URHO3D_HANDLER(AggregatedInput, HandleKeyUp));
    SubscribeToEvent(input, E_KEYDOWN, URHO3D_HANDLER(AggregatedInput, HandleKeyDown));
    SubscribeToEvent(input, E_JOYSTICKAXISMOVE, URHO3D_HANDLER(AggregatedInput, HandleJoystickAxisMove));
    SubscribeToEvent(input, E_JOYSTICKHATMOVE, URHO3D_HANDLER(AggregatedInput, HandleJoystickHatMove));
    SubscribeToEvent(input, E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(AggregatedInput, HandleJoystickDisconnected));
    SubscribeToEvent(input, E_TOUCHBEGIN, URHO3D_HANDLER(AggregatedInput, HandleTouchBegin));
    SubscribeToEvent(input, E_TOUCHMOVE, URHO3D_HANDLER(AggregatedInput, HandleTouchMove));
    SubscribeToEvent(input, E_TOUCHEND, URHO3D_HANDLER(AggregatedInput, HandleTouchEnd));
}

void AggregatedInput::Deactivate()
{
    aggregatedInput_.SetEnabled(false);
    dpadInput_.SetEnabled(false);
    UnsubscribeFromAllEvents();
}

void AggregatedInput::AddFilteredEvent(const ea::string& str)
{
    filteredEvents_.push(str);
    if (filteredEvents_.size() > 32)
        filteredEvents_.pop();

    filteredEventsText_.clear();
    filteredEventsText_.append("Filtered events:\n");
    for (int i = filteredEvents_.size() - 1; i >= 0; --i)
    {
        if (!filteredEventsText_.empty())
            filteredEventsText_.append("\n");
        filteredEventsText_.append(filteredEvents_.get_container()[i]);
    }
    filteredEventsLog_->SetText(filteredEventsText_);
}
void AggregatedInput::AddRawEvent(const ea::string& str)
{
    rawEvents_.push(str);
    if (rawEvents_.size() > 32)
        rawEvents_.pop();

    rawEventsText_.clear();
    rawEventsText_.append("Input events:\n");
    for (int i = rawEvents_.size() - 1; i >= 0; --i)
    {
        rawEventsText_.append("\n");
        rawEventsText_.append(rawEvents_.get_container()[i]);
    }
    rawEventsLog_->SetText(rawEventsText_);
}

void AggregatedInput::HandleDPadKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    const auto* input = context_->GetSubsystem<Input>();

    AddFilteredEvent(Format("KeyDown: Key {}, Scancode {}{}", input->GetKeyName(static_cast<Key>(args[P_KEY].GetUInt())),
            input->GetScancodeName(static_cast<Scancode>(args[P_SCANCODE].GetUInt())),
            args[P_REPEAT].GetBool() ? ", R" : ""));
}

void AggregatedInput::HandleDPadKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    const auto* input = context_->GetSubsystem<Input>();

    AddFilteredEvent(Format("KeyUp: Key {}, Scancode {}", input->GetKeyName(static_cast<Key>(args[P_KEY].GetUInt())),
        input->GetScancodeName(static_cast<Scancode>(args[P_SCANCODE].GetUInt()))));
}

void AggregatedInput::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    const auto* input = context_->GetSubsystem<Input>();

    if (args[P_REPEAT].GetBool())
        return;

    AddRawEvent(Format("KeyDown: Key {}, Scancode {}", input->GetKeyName(static_cast<Key>(args[P_KEY].GetUInt())),
        input->GetScancodeName(static_cast<Scancode>(args[P_SCANCODE].GetUInt()))));
}

void AggregatedInput::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    const auto* input = context_->GetSubsystem<Input>();

    AddRawEvent(Format("KeyUp: Key {}, Scancode {}", input->GetKeyName(static_cast<Key>(args[P_KEY].GetUInt())),
        input->GetScancodeName(static_cast<Scancode>(args[P_SCANCODE].GetUInt()))));
}

void AggregatedInput::HandleJoystickAxisMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickAxisMove;
    const auto* input = context_->GetSubsystem<Input>();

    AddRawEvent(Format("AxisMove: Axis {}, Value {}", args[P_AXIS].GetUInt(), args[P_POSITION].GetFloat()));
}

void AggregatedInput::HandleJoystickHatMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickHatMove;
    const auto* input = context_->GetSubsystem<Input>();

    const char* posName = "";
    switch ((HatPosition)args[P_POSITION].GetInt())
    {
    case HAT_RIGHT: posName = "Right"; break;
    case HAT_LEFT: posName = "Left"; break;
    case HAT_UP: posName = "Up"; break;
    case HAT_DOWN: posName = "Down"; break;
    case HAT_CENTER: posName = "Center"; break;
    case HAT_RIGHTDOWN: posName = "Right Down"; break;
    case HAT_LEFTDOWN: posName = "Left Down"; break;
    case HAT_RIGHTUP: posName = "Right Up"; break;
    case HAT_LEFTUP: posName = "Left Up"; break;
    }
    AddRawEvent(Format("HatMove: Hat {}, Value {}", args[P_HAT].GetUInt(), posName));
}

void AggregatedInput::HandleJoystickDisconnected(StringHash eventType, VariantMap& args) { using namespace KeyDown; }
void AggregatedInput::HandleTouchBegin(StringHash eventType, VariantMap& args) { using namespace KeyDown; }
void AggregatedInput::HandleTouchMove(StringHash eventType, VariantMap& args) { using namespace KeyDown; }
void AggregatedInput::HandleTouchEnd(StringHash eventType, VariantMap& args) { using namespace KeyDown; }

void AggregatedInput::Update(float timeStep)
{
    const auto uiRoot = GetUIRoot();
    const auto screenSize = uiRoot->GetSize();

    const auto widthQuater = screenSize.x_ / 4;
    const auto unit = Min(widthQuater / 1.5f, screenSize.y_ / 2);

    rawEventsLog_->SetPosition(0, 32);
    rawEventsLog_->SetSize(widthQuater, screenSize.y_ - 32);

    filteredEventsLog_->SetPosition(widthQuater * 3, 32);
    filteredEventsLog_->SetSize(widthQuater, screenSize.y_ - 32);

    const auto center = Vector2(widthQuater * 2.0f, screenSize.y_ / 2);
    {
        const auto d = aggregatedInput_.GetDirection();
        analogPivot_->SetPosition(center - analogPivot_->GetSize().ToVector2() * 0.5f);
        analogMarker_->SetPosition(center + Vector2(unit, unit) * d * 0.6f - analogMarker_->GetSize().ToVector2() * 0.5f);
    }
    {
        downMarker_->SetPosition(center + Vector2(0, unit) - downMarker_->GetSize().ToVector2() * 0.5f);
        upMarker_->SetPosition(center + Vector2(0, -unit) - upMarker_->GetSize().ToVector2() * 0.5f);
        rightMarker_->SetPosition(center + Vector2(unit, 0) - rightMarker_->GetSize().ToVector2() * 0.5f);
        leftMarker_->SetPosition(center + Vector2(-unit, 0) - leftMarker_->GetSize().ToVector2() * 0.5f);
    }

    downMarker_->SetVisible(dpadInput_.GetScancodeDown(SCANCODE_DOWN));
    leftMarker_->SetVisible(dpadInput_.GetScancodeDown(SCANCODE_LEFT));
    rightMarker_->SetVisible(dpadInput_.GetScancodeDown(SCANCODE_RIGHT));
    upMarker_->SetVisible(dpadInput_.GetScancodeDown(SCANCODE_UP));
}
