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
#include "Urho3D/Actions/Action.h"
#include "Urho3D/Actions/ActionBuilder.h"
#include "Urho3D/Actions/Attribute.h"
#include "Urho3D/Actions/ShaderParameterFromTo.h"
#include "Urho3D/Math/EaseMath.h"
#include "Urho3D/Scene/Scene.h"

#include <Urho3D/Actions/ActionManager.h>
#include <Urho3D/Actions/Move.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/UI/UIElement.h>

using namespace Urho3D;

TEST_CASE("BackIn tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto actionManager = context->GetSubsystem<ActionManager>();

    auto backIn = ActionBuilder(context).MoveBy(1.0f, Vector3(1.0f, 0, 0)).BackIn().Build();
    auto node = MakeShared<Node>(context);
    auto* state = actionManager->AddAction(backIn, node);
    actionManager->Update(0.0f);

    actionManager->Update(0.2f);
    CHECK(Equals(BackIn(0.2f), node->GetPosition().x_));

    actionManager->Update(0.2f);
    CHECK(Equals(BackIn(0.4f), node->GetPosition().x_));

    actionManager->Update(0.8f);
    CHECK(Equals(BackIn(1.0f), node->GetPosition().x_));
}

TEST_CASE("MoveBy tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto moveBy = ActionBuilder(context).MoveBy(2.0f, Vector3(10.0f, 0, 0)).Build();
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

    auto moveBy = MakeShared<Actions::MoveBy2D>(context);
    moveBy->SetDuration(2.0f);
    moveBy->SetPositionDelta(Vector2(12, 0));
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

    auto moveBy = MakeShared<Actions::AttributeTo>(context);
    moveBy->SetDuration(2.0f);
    moveBy->SetAttributeName("Position");
    moveBy->SetTo(Vector3(10, 0, 0));
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

    auto moveBy = MakeShared<Actions::AttributeTo>(context);
    moveBy->SetDuration(2.0f);
    moveBy->SetAttributeName("Position");
    moveBy->SetTo(IntVector2(12, 0));
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

    auto moveBy = MakeShared<Actions::AttributeFromTo>(context);
    moveBy->SetDuration(2.0f);
    moveBy->SetAttributeName("Color");
    moveBy->SetFrom(Color::BLACK);
    moveBy->SetTo(Color::WHITE);
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

TEST_CASE("RemoveSelf action deletes node")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();
    
    auto scene = MakeShared<Scene>(context);
    WeakPtr<Node> weakNode = WeakPtr<Node>(scene->CreateChild("", REPLICATED, 0, false));

    auto action = ActionBuilder(context).RemoveSelf().DelayTime(10).Build();
    actionManager->AddAction(action, weakNode);

    // Tick the manager. It should trigger RemoveSelf and remove the node.
    actionManager->Update(0.5f);
    CHECK(0 == scene->GetNumChildren());
    CHECK(weakNode.Expired());
}

TEST_CASE("ShaderParameterFromTo tweening")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto actionManager = context->GetSubsystem<ActionManager>();

    auto shaderParameterFromTo = MakeShared<Actions::ShaderParameterFromTo>(context);
    shaderParameterFromTo->SetDuration(2.0f);
    shaderParameterFromTo->SetName("MatDiffColor");
    shaderParameterFromTo->SetFrom(Color::BLACK);
    shaderParameterFromTo->SetTo(Color::WHITE);
    auto material = MakeShared<Material>(context);

    // Initial state - no actions added
    CHECK(0 == actionManager->GetNumActions(material));

    // Add action.
    auto* state = actionManager->AddAction(shaderParameterFromTo, material);
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

TEST_CASE("Serialize Action")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto action = MakeShared<Action>(context);
    SharedPtr<Actions::BaseAction> innerAction = ActionBuilder(context).MoveBy(2.0f, Vector3(1, 2, 3)).Build();
    action->SetAction(innerAction);

    VectorBuffer buf;
    action->Save(buf);
    // ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());

    buf.Seek(0);
    auto action2 = MakeShared<Action>(context);
    action2->Load(buf);

    CHECK(action->GetAction()->GetType() == action2->GetAction()->GetType());
    auto expected = static_cast<Actions::MoveBy*>(action->GetAction());
    auto actual = static_cast<Actions::MoveBy*>(action2->GetAction());
    CHECK(Equals(expected->GetDuration(), actual->GetDuration()));
    CHECK(expected->GetPositionDelta().Equals(actual->GetPositionDelta()));
}
