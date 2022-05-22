//
// Copyright (c) 2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//
#include "../CommonUtils.h"
#include "Urho3D/Actions/AttributeFromTo.h"
#include "Urho3D/Actions/ShaderParameterFromTo.h"

#include <Urho3D/Actions/MoveTo.h>
#include <Urho3D/Actions/MoveTo2D.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/Actions/MoveBy2D.h>
#include <Urho3D/Actions/ActionManager.h>
#include <Urho3D/Actions/MoveBy.h>

using namespace Urho3D;

TEST_CASE("MoveBy tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<MoveBy>(context, 2.0f, Vector3(10, 0, 0));
    auto node = MakeShared<Node>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(node));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, node);
    CHECK(1 == actionManager->GetNumActions(node));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(node->GetPosition().Equals(Vector3::ZERO));

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(node->GetPosition().Equals(Vector3(2.5f, 0, 0)));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(node->GetPosition().Equals(Vector3(10.0f, 0, 0)));
    CHECK(0 == actionManager->GetNumActions(node));
}

TEST_CASE("MoveBy2D tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<MoveBy2D>(context, 2.0f, Vector2(12, 0));
    auto uiElement = MakeShared<UIElement>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(uiElement));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, uiElement);
    CHECK(1 == actionManager->GetNumActions(uiElement));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(uiElement->GetPosition() == IntVector2::ZERO);

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(uiElement->GetPosition() == IntVector2(3, 0));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(uiElement->GetPosition() == IntVector2(12, 0));
    CHECK(0 == actionManager->GetNumActions(uiElement));
}

TEST_CASE("MoveTo tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<MoveTo>(context, 2.0f, Vector3(10, 0, 0));
    auto node = MakeShared<Node>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(node));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, node);
    CHECK(1 == actionManager->GetNumActions(node));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(node->GetPosition().Equals(Vector3::ZERO));

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(node->GetPosition().Equals(Vector3(2.5f, 0, 0)));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(node->GetPosition().Equals(Vector3(10.0f, 0, 0)));
    CHECK(0 == actionManager->GetNumActions(node));
}

TEST_CASE("MoveTo2D tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<MoveTo2D>(context, 2.0f, Vector2(12, 0));
    auto uiElement = MakeShared<UIElement>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(uiElement));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, uiElement);
    CHECK(1 == actionManager->GetNumActions(uiElement));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(uiElement->GetPosition() == IntVector2::ZERO);

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(uiElement->GetPosition() == IntVector2(3, 0));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(uiElement->GetPosition() == IntVector2(12, 0));
    CHECK(0 == actionManager->GetNumActions(uiElement));
}


TEST_CASE("AttributeFromTo tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<AttributeFromTo>(context, 2.0f, "Color", Color::BLACK, Color::WHITE);
    auto uiElement = MakeShared<UIElement>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(uiElement));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, uiElement);
    CHECK(1 == actionManager->GetNumActions(uiElement));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(uiElement->GetColorAttr() == Color::BLACK);

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(uiElement->GetColorAttr() == Color(0.25f, 0.25f, 0.25f, 1.0f));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(0 == actionManager->GetNumActions(uiElement));
}

TEST_CASE("ShaderParameterFromTo tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = MakeShared<ShaderParameterFromTo>(context, 2.0f,"MatDiffColor", Color::BLACK, Color::WHITE);
    auto material = MakeShared<Material>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(material));

    // Add action.
    auto* state = actionManager->AddAction(moveBy, material);
    CHECK(1 == actionManager->GetNumActions(material));

    // First tick doesn't move as it saves the start position
    actionManager->Update(0.5f);
    CHECK(material->GetShaderParameter("MatDiffColor") == Color::BLACK);

    // Next tick moves the node
    actionManager->Update(0.5f);
    CHECK(material->GetShaderParameter("MatDiffColor") == Color(0.25f, 0.25f, 0.25f, 1.0f));

    // Advance beyond the end of animation.
    actionManager->Update(2.5f);
    CHECK(0 == actionManager->GetNumActions(material));
}
