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

AggregatedInput::AggregatedInput(Context* context) : Sample(context)
    , aggregatedInput_(context)
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

    // Finally subscribe to the update event. Note that by subscribing events at this point we have already missed some events
    // like the ScreenMode event sent by the Graphics subsystem when opening the application window. To catch those as well we
    // could subscribe in the constructor instead.
    SubscribeToEvents();

}

void AggregatedInput::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();

    auto uiRoot = GetUIRoot();
    pivot_ =  uiRoot->CreateChild<Sprite>();
    pivot_->SetSize(20, 20);
    _marker = uiRoot->CreateChild<Sprite>();
    _marker->SetSize(20, 20);
    // Construct new Text object
    SharedPtr<Text> helloText(MakeShared<Text>(context_));

    // Set String to display
    
    helloText->SetText("Move marker around with keyboard, mouse or touch");

    // Set font and text color
    helloText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_TOP);

    // Add Text instance to the UI root element
    uiRoot->AddChild(helloText);
}

void AggregatedInput::SubscribeToEvents() { aggregatedInput_.SetEnabled(true); }

void AggregatedInput::Deactivate() { aggregatedInput_.SetEnabled(false); }

void AggregatedInput::Update(float timeStep)
{
    auto uiRoot = GetUIRoot();
    auto screenSize = uiRoot->GetSize();

    auto center = Vector2(screenSize.x_ / 2, screenSize.y_ / 2);
    auto quater = Min(screenSize.x_ / 4, screenSize.y_ / 4);

    auto d = aggregatedInput_.GetDirection();
    pivot_->SetPosition(center - Vector2(10,10));
    _marker->SetPosition(center + Vector2(quater, quater) * d - Vector2(10, 10));
}
