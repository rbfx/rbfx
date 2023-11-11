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
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Actions/ActionBuilder.h>
#include <Urho3D/Actions/ActionManager.h>

#include "ActionDemo.h"

#include <Urho3D/DebugNew.h>

ActionDemo::ActionDemo(Context* context)
    : Sample(context)
{
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void ActionDemo::Start()
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

ActionDemo::DemoElement& ActionDemo::AddElement(const IntVector2& pos, const SharedPtr<Actions::BaseAction>& action)
{
    auto uiRoot = GetUIRoot();
    auto& element = markers_.push_back();
    element.element_ = uiRoot->CreateChild<Sprite>();
    element.element_->SetEnabled(true);
    element.element_->SetSize(20, 20);
    element.element_->SetPosition(pos);
    element.action_ = action;
    return element;
}

void ActionDemo::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto uiRoot = GetUIRoot();

    // Construct new Text object
    SharedPtr<Text> helloText(MakeShared<Text>(context_));

    // Set String to display

    helloText->SetText("Click on quads to trigger actions");

    // Set font and text color
    helloText->SetFont(font, 15);

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_TOP);

    // Add Text instance to the UI root element
    uiRoot->AddChild(helloText);

    IntVector2 pos{100, 64};
    IntVector2 step{32, 0};
    Vector2 offset{0.0f, 100.0f};
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).Blink(2.0f, 10, "Is Visible").Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BackIn().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BackOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BackInOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BounceOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BounceIn().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).BounceInOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).SineOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).SineIn().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).SineInOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ExponentialOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ExponentialIn().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ExponentialInOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ElasticIn().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ElasticOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).MoveBy(1.0f, offset).ElasticInOut().DelayTime(2.0f).JumpBy(-offset).Build());
    pos += step;
    AddElement(pos,
        ActionBuilder(context_)
            .MoveByQuadratic(1.0f, offset * 0.5f + Vector2(+40,0), offset)
            .DelayTime(2.0f)
            .JumpBy(-offset)
            .Build());
    pos += step;
    AddElement(pos, ActionBuilder(context_).RemoveSelf().Build());
    pos += step;
    AddElement(
        pos, ActionBuilder(context_).ShakeBy(1.0f, Vector3(10,10,0)).Build());
}

void ActionDemo::SubscribeToEvents()
{
    auto* input = context_->GetSubsystem<Input>();
    SubscribeToEvent(E_UIMOUSECLICK, URHO3D_HANDLER(ActionDemo, HandleMouseClick));
}

void ActionDemo::HandleMouseClick(StringHash eventType, VariantMap& eventData)
{
    // Get control that was clicked
    auto* clicked = static_cast<UIElement*>(eventData[UIMouseClick::P_ELEMENT].GetPtr());
    for (auto& element : markers_)
    {
        if (element.element_ == clicked)
        {
            ActionManager* am = GetActionManager();
            am->AddAction(element.action_, clicked);
        }
    }
}

void ActionDemo::Deactivate() { UnsubscribeFromAllEvents(); }

void ActionDemo::Update(float timeStep)
{
    const auto uiRoot = GetUIRoot();
    const auto screenSize = uiRoot->GetSize();

    const auto widthQuater = screenSize.x_ / 4;
    const auto unit = Min(widthQuater / 1.5f, screenSize.y_ / 2);
}
