//
// Copyright (c) 2023-2023 the rbfx project.
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

#include <Urho3D/Resource/XMLArchive.h>
#include <Urho3D/Input/MoveAndOrbitController.h>
#include <Urho3D/Input/InputMap.h>

using namespace Urho3D;

TEST_CASE("Build MoveAndOrbit config")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    const auto map = MakeShared<InputMap>(context);

    // Uncomment this to override default sensitiviy:
    //map->AddMetadata(MoveAndOrbitController::MOUSE_SENSITIVITY, MoveAndOrbitController::DEFAULT_MOUSE_SENSITIVITY);
    //map->AddMetadata(
    //    MoveAndOrbitController::TOUCH_MOVEMENT_SENSITIVITY, MoveAndOrbitController::DEFAULT_TOUCH_MOVEMENT_SENSITIVITY);
    //map->AddMetadata(
    //    MoveAndOrbitController::TOUCH_ROTATION_SENSITIVITY, MoveAndOrbitController::DEFAULT_TOUCH_ROTATION_SENSITIVITY);

    map->MapKeyboardKey(MoveAndOrbitController::ACTION_FORWARD, SCANCODE_W);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_FORWARD, SCANCODE_UP);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_FORWARD, CONTROLLER_AXIS_LEFTY, 0.0f, -1.0f);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_BACK, SCANCODE_S);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_BACK, SCANCODE_DOWN);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_BACK, CONTROLLER_AXIS_LEFTY, 0.0f, 1.0f);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_LEFT, SCANCODE_A);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_LEFT, SCANCODE_LEFT);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_LEFT, CONTROLLER_AXIS_LEFTX, 0.0f, -1.0f);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_RIGHT, SCANCODE_D);
    map->MapKeyboardKey(MoveAndOrbitController::ACTION_RIGHT, SCANCODE_RIGHT);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_RIGHT, CONTROLLER_AXIS_LEFTX, 0.0f, 1.0f);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_TURNLEFT, CONTROLLER_AXIS_RIGHTX, 0.0f, -1.0f);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_TURNRIGHT, CONTROLLER_AXIS_RIGHTX, 0.0f, 1.0f);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_LOOKUP, CONTROLLER_AXIS_RIGHTY, 0.0f, -1.0f);
    map->MapControllerAxis(MoveAndOrbitController::ACTION_LOOKDOWN, CONTROLLER_AXIS_RIGHTY, 0.0f, 1.0f);

    // Uncomment this to save the file:
    //map->SaveFile(FileIdentifier("", "Input/MoveAndOrbit.inputmap"));
}

TEST_CASE("ActionMapping serialization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    Detail::ActionMapping expectedMapping;
    expectedMapping.keyboardKeys_.emplace_back(Scancode::SCANCODE_F1);
    expectedMapping.mouseButtons_.emplace_back(1);
    expectedMapping.controllerHats_.emplace_back(1);
    expectedMapping.controllerButtons_.emplace_back(ControllerButton::CONTROLLER_BUTTON_B);
    expectedMapping.controllerButtons_.emplace_back(4);
    expectedMapping.controllerAxes_.emplace_back(ControllerAxis::CONTROLLER_AXIS_RIGHTX, 1.0f, -1.0f);
    expectedMapping.controllerAxes_.emplace_back(4, 1.0f, -1.0f);

    auto xmlFile = MakeShared<XMLFile>(context);
    XMLElement root = xmlFile->CreateRoot("root");
    XMLOutputArchive xmlOutputArchive{context, root};

    SerializeValue(xmlOutputArchive, "root", expectedMapping);

    XMLInputArchive xmlInputArchive{context, root};
    Detail::ActionMapping actualMapping;

    SerializeValue(xmlInputArchive, "root", actualMapping);

    CHECK(expectedMapping.controllerAxes_.size() == actualMapping.controllerAxes_.size());
    for (unsigned i = 0; i < expectedMapping.controllerAxes_.size(); ++i)
    {
        CHECK(expectedMapping.controllerAxes_[i].controller_ == actualMapping.controllerAxes_[i].controller_);
        CHECK(expectedMapping.controllerAxes_[i].axis_ == actualMapping.controllerAxes_[i].axis_);
        CHECK(Equals(expectedMapping.controllerAxes_[i].neutral_, actualMapping.controllerAxes_[i].neutral_));
        CHECK(Equals(expectedMapping.controllerAxes_[i].pressed_, actualMapping.controllerAxes_[i].pressed_));
    }
    CHECK(expectedMapping.controllerButtons_.size() == actualMapping.controllerButtons_.size());
    for (unsigned i = 0; i < expectedMapping.controllerButtons_.size(); ++i)
    {
        CHECK(expectedMapping.controllerButtons_[i].controller_ == actualMapping.controllerButtons_[i].controller_);
        CHECK(expectedMapping.controllerButtons_[i].button_ == actualMapping.controllerButtons_[i].button_);
    }
    CHECK(expectedMapping.controllerHats_.size() == actualMapping.controllerHats_.size());
    for (unsigned i = 0; i < expectedMapping.controllerHats_.size(); ++i)
    {
        CHECK(expectedMapping.controllerHats_[i].hatPosition_ == actualMapping.controllerHats_[i].hatPosition_);
    }
    CHECK(expectedMapping.mouseButtons_.size() == actualMapping.mouseButtons_.size());
    for (unsigned i = 0; i < expectedMapping.mouseButtons_.size(); ++i)
    {
        CHECK(expectedMapping.mouseButtons_[i].mouseButton_ == actualMapping.mouseButtons_[i].mouseButton_);
    }
    CHECK(expectedMapping.keyboardKeys_.size() == actualMapping.keyboardKeys_.size());
    for (unsigned i = 0; i < expectedMapping.keyboardKeys_.size(); ++i)
    {
        CHECK(expectedMapping.keyboardKeys_[i].scancode_ == actualMapping.keyboardKeys_[i].scancode_);
    }
}

TEST_CASE("ControllerAxisMapping OverlapsWith")
{
    Detail::ControllerAxisMapping testRange(0, 0.2f, 0.4f);
    CHECK(!testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.0f, 0.1f)));
    CHECK(!testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.1f, 0.2f)));
    CHECK(testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.1f, 0.3f)));
    CHECK(testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.1f, 0.5f)));
    CHECK(testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.25f, 0.35f)));
    CHECK(testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.3f, 0.5f)));
    CHECK(!testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.4f, 0.5f)));
    CHECK(!testRange.OverlapsWith(Detail::ControllerAxisMapping(0, 0.5f, 0.6f)));
}

TEST_CASE("ControllerAxisMapping Translate")
{
    {
        Detail::ControllerAxisMapping testRange(0, 0.1f, 1.2f);
        CHECK(Equals(testRange.Translate(0, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(0.1, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(0.2, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(0.7, 0.1f), 0.5f));
        CHECK(Equals(testRange.Translate(1.2, 0.1f), 1.0f));
        CHECK(Equals(testRange.Translate(1.3, 0.1f), 0.0f));
    }
    {
        Detail::ControllerAxisMapping testRange(0, -0.1f, -1.2f);
        CHECK(Equals(testRange.Translate(0, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(-0.1, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(-0.2, 0.1f), 0.0f));
        CHECK(Equals(testRange.Translate(-0.7, 0.1f), 0.5f));
        CHECK(Equals(testRange.Translate(-1.2, 0.1f), 1.0f));
        CHECK(Equals(testRange.Translate(-1.3, 0.1f), 0.0f));
    }
}
