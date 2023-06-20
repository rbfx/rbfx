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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Urho3D/Input/InputMap.h"

#include "InputConstants.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Input/Input.h"

namespace Urho3D
{
namespace
{

static const char* scancodeNames[] = {
    "",
    "CTRL",
    "SHIFT",
    "ALT",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "Return",
    "Escape",
    "Backspace",
    "Tab",
    "Space",
    "-",
    "=",
    "[",
    "]",
    "\\",
    "#",
    ";",
    "'",
    "`",
    ",",
    ".",
    "/",
    "CapsLock",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "PrintScreen",
    "ScrollLock",
    "Pause",
    "Insert",
    "Home",
    "PageUp",
    "Delete",
    "End",
    "PageDown",
    "Right",
    "Left",
    "Down",
    "Up",
    "Numlock",
    "Keypad /",
    "Keypad *",
    "Keypad -",
    "Keypad +",
    "Keypad Enter",
    "Keypad 1",
    "Keypad 2",
    "Keypad 3",
    "Keypad 4",
    "Keypad 5",
    "Keypad 6",
    "Keypad 7",
    "Keypad 8",
    "Keypad 9",
    "Keypad 0",
    "Keypad .",
    "Non US Backslash",
    "Application",
    "Power",
    "Keypad =",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "Execute",
    "Help",
    "Menu",
    "Select",
    "Stop",
    "Again",
    "Undo",
    "Cut",
    "Copy",
    "Paste",
    "Find",
    "Mute",
    "VolumeUp",
    "VolumeDown",
    "KP Comma",
    "KP_EQUALSAS400",
    "INTERNATIONAL1",
    "Keypad ,",
    "Keypad = (AS400)",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "AltErase",
    "SysReq",
    "Cancel",
    "Clear",
    "Prior",
    "Return",
    "Separator",
    "Out",
    "Oper",
    "Clear / Again",
    "CrSel",
    "ExSel",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "Keypad 00",
    "Keypad 000",
    "ThousandsSeparator",
    "DecimalSeparator",
    "CurrencyUnit",
    "CurrencySubUnit",
    "Keypad (",
    "Keypad )",
    "Keypad {",
    "Keypad }",
    "Keypad Tab",
    "Keypad Backspace",
    "Keypad A",
    "Keypad B",
    "Keypad C",
    "Keypad D",
    "Keypad E",
    "Keypad F",
    "Keypad XOR",
    "Keypad ^",
    "Keypad %",
    "Keypad <",
    "Keypad >",
    "Keypad &",
    "Keypad &&",
    "Keypad |",
    "Keypad ||",
    "Keypad :",
    "Keypad #",
    "Keypad Space",
    "Keypad @",
    "Keypad !",
    "Keypad MemStore",
    "Keypad MemRecall",
    "Keypad MemClear",
    "Keypad MemAdd",
    "Keypad MemSubtract",
    "Keypad MemMultiply",
    "Keypad MemDivide",
    "Keypad +/-",
    "Keypad Clear",
    "Keypad ClearEntry",
    "Keypad Binary",
    "Keypad Octal",
    "Keypad Decimal",
    "Keypad Hexadecimal",
    NULL,
    NULL,
    "Left Ctrl",
    "Left Shift",
    "Left Alt",
    "Left GUI",
    "Right Ctrl",
    "Right Shift",
    "Right Alt",
    "Right GUI",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "ModeSwitch",
    "AudioNext",
    "AudioPrev",
    "AudioStop",
    "AudioPlay",
    "AudioMute",
    "MediaSelect",
    "WWW",
    "Mail",
    "Calculator",
    "Computer",
    "AC Search",
    "AC Home",
    "AC Back",
    "AC Forward",
    "AC Stop",
    "AC Refresh",
    "AC Bookmarks",
    "BrightnessDown",
    "BrightnessUp",
    "DisplaySwitch",
    "KBDIllumToggle",
    "KBDIllumDown",
    "KBDIllumUp",
    "Eject",
    "Sleep",
    "App1",
    "App2",
    "AudioRewind",
    "AudioFastForward",
    "SoftLeft",
    "SoftRight",
    "Call",
    "EndCall",
    nullptr,
};

} // namespace

namespace Detail
{
KeyboardKeyMapping::KeyboardKeyMapping()
{
}

KeyboardKeyMapping::KeyboardKeyMapping(Scancode scancode)
    : scancode_(scancode)
{
}

void KeyboardKeyMapping::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "scancode", scancode_, SCANCODE_UNKNOWN,
        [&](Archive& archive, const char* name, auto& value)
        { SerializeEnum(archive, name, value, InputMap::GetScanCodeNames()); });
}

ControllerButtonMapping::ControllerButtonMapping()
{
}

ControllerButtonMapping::ControllerButtonMapping(ControllerButton controllerButton)
    : controller_(true)
    , button_(static_cast<unsigned>(controllerButton))
{
}

ControllerButtonMapping::ControllerButtonMapping(unsigned buttonIndex)
    : controller_(false)
    , button_(buttonIndex)
{
}

void ControllerButtonMapping::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "controller", controller_, false);
    SerializeOptionalValue(archive, "button", button_, 0);
}

ControllerAxisMapping::ControllerAxisMapping()
    : neutral_(0.0f)
    , pressed_(1.0f)
{
}

ControllerAxisMapping::ControllerAxisMapping(ControllerAxis controllerAxis, float neutral, float pressed)
    : controller_(true)
    , axis_(static_cast<unsigned>(controllerAxis))
    , neutral_(neutral)
    , pressed_(pressed)
{
}

ControllerAxisMapping::ControllerAxisMapping(unsigned axisIndex, float neutral, float pressed)
    : controller_(false)
    , axis_(axisIndex)
    , neutral_(neutral)
    , pressed_(pressed)
{
}

void ControllerAxisMapping::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "controller", controller_, false);
    SerializeOptionalValue(archive, "axis", axis_, 0);
    SerializeOptionalValue(archive, "neutral", neutral_, 0.0f);
    SerializeOptionalValue(archive, "pressed", pressed_, 1.0f);
}

bool ControllerAxisMapping::OverlapsWith(const ControllerAxisMapping& mapping) const
{
    if (mapping.controller_ != controller_ || mapping.axis_ != axis_)
        return false;

    Vector2 thisRange{
        Min(neutral_, pressed_),
        Max(neutral_, pressed_),
    };
    Vector2 otherRange{
        Min(mapping.neutral_, mapping.pressed_),
        Max(mapping.neutral_, mapping.pressed_),
    };
    return !(thisRange.y_ <= otherRange.x_ || otherRange.y_ <= thisRange.x_);
}

float ControllerAxisMapping::Translate(float pos, float deadZone) const
{
    if (Equals(pos, pressed_))
        return 1.0f;
    const bool positive = pressed_ >= neutral_;
    const auto neutral = positive ? Min(neutral_ + deadZone, pressed_) : Max(neutral_ - deadZone, pressed_);
    const auto min = Min(neutral, pressed_);
    const auto max = Max(neutral, pressed_);
    if (pos < min || pos > max)
        return 0.0f;
    if (positive)
        return (pos - min) / Max(max - min, ea::numeric_limits<float>::epsilon());
    return (max - pos) / Max(max - min, ea::numeric_limits<float>::epsilon());
}

ControllerHatMapping::ControllerHatMapping()
{
}

ControllerHatMapping::ControllerHatMapping(unsigned hatPosition)
    : hatPosition_(hatPosition)
{
}

void ControllerHatMapping::SerializeInBlock(Archive& archive)
{
    static const char* hatNames[] = {"Up", "Right", "Down", "Left", nullptr};
    SerializeOptionalValue(archive, "hat", hatPosition_, 0,
        [&](Archive& archive, const char* name, auto& value)
        { SerializeEnum<unsigned, unsigned>(archive, name, value, hatNames); });
}

MouseButtonMapping::MouseButtonMapping()
{
}

MouseButtonMapping::MouseButtonMapping(unsigned mouseButton)
    : mouseButton_(mouseButton)
{
}

void MouseButtonMapping::SerializeInBlock(Archive& archive)
{
    static const char* buttonNames[] = {"Left", "Middle", "Right", "X1", "X2", nullptr};
    SerializeOptionalValue(archive, "button", mouseButton_, 0,
        [&](Archive& archive, const char* name, auto& value)
        { SerializeEnum<unsigned, unsigned>(archive, name, value, buttonNames); });
}

void ActionMapping::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "keys", keyboardKeys_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "key"); });
    SerializeOptionalValue(archive, "buttons", controllerButtons_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "button"); });
    SerializeOptionalValue(archive, "axes", controllerAxes_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "axis"); });
    SerializeOptionalValue(archive, "hats", controllerHats_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "hat"); });
    SerializeOptionalValue(archive, "mouseButtons", mouseButtons_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "button"); });
}

float ActionMapping::Evaluate(Input* input) const
{
    for (auto& key: keyboardKeys_)
    {
        if (input->GetScancodeDown(key.scancode_))
        {
            return 1.0f;
        }
    }
    for (auto& button : mouseButtons_)
    {
        if (input->GetMouseButtonDown(static_cast<MouseButtonFlags>(1 << button.mouseButton_)))
        {
            return 1.0f;
        }
    }
    if (controllerAxes_.empty() && controllerButtons_.empty() && controllerHats_.empty())
    {
        return 0.0f;
    }
    const unsigned numJoysticks = input->GetNumJoysticks();
    float sum = 0.0f;
    for (unsigned i = 0; i < numJoysticks; ++i)
    {
        const auto state = input->GetJoystickByIndex(i);
        if (state)
        {
            const auto isController = state->IsController();
            for (auto& button : controllerButtons_)
            {
                if (button.controller_ == isController && state->GetButtonDown(button.button_))
                {
                    return 1.0f;
                }
            }
            if (state->GetNumHats() > 0)
            {
                auto hatPos = state->GetHatPosition(0);
                for (auto& hat : controllerHats_)
                {
                    if (0 != (hatPos & (1 << hat.hatPosition_)))
                    {
                        return 1.0f;
                    }
                }
            }

            for (auto& axis : controllerAxes_)
            {
                if (axis.controller_ == isController && state->HasAxisPosition(axis.axis_))
                {
                    auto pos = state->GetAxisPosition(axis.axis_);
                    sum += axis.Translate(pos, 0.1f);
                }
            }
        }
    }
    return sum;
}

} // namespace Detail

InputMap::InputMap(Context* context)
    : BaseClassName(context)
{
}

InputMap::~InputMap()
{
}

void InputMap::RegisterObject(Context* context)
{
    context->AddFactoryReflection<InputMap>();
}

void InputMap::SerializeInBlock(Archive& archive)
{
    SerializeMap(archive, "actions", actions_, "action");
}

void InputMap::MapKeyboardKey(const ea::string& action, Scancode scancode)
{
    for (auto kv : actions_)
    {
        ea::erase_if(
            kv.second.keyboardKeys_, [=](const Detail::KeyboardKeyMapping& m) { return m.scancode_ == scancode; });
    }
    auto& map = GetOrAddMapping(action);
    map.keyboardKeys_.emplace_back(scancode);
}

void InputMap::MapControllerButton(const ea::string& action, ControllerButton button)
{
    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.controllerButtons_,
            [=](const Detail::ControllerButtonMapping& m) { return m.controller_ && m.button_ == button; });
    }
    auto& map = GetOrAddMapping(action);
    map.controllerButtons_.emplace_back(button);
}

void InputMap::MapJoystickButton(const ea::string& action, unsigned buttonIndex)
{
    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.controllerButtons_,
            [=](const Detail::ControllerButtonMapping& m) { return !m.controller_ && m.button_ == buttonIndex; });
    }
    auto& map = GetOrAddMapping(action);
    map.controllerButtons_.emplace_back(buttonIndex);
}

void InputMap::MapControllerAxis(const ea::string& action, ControllerAxis axis, float neutral, float pressed)
{
    const Detail::ControllerAxisMapping mapping{axis, neutral, pressed};
    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.controllerAxes_,
            [=](const Detail::ControllerAxisMapping& m) { return m.OverlapsWith(mapping); });
    }
    auto& map = GetOrAddMapping(action);
    map.controllerAxes_.push_back(mapping);
}

void InputMap::MapJoystickAxis(const ea::string& action, unsigned axis, float neutral, float pressed)
{
    const Detail::ControllerAxisMapping mapping{axis, neutral, pressed};
    for (auto kv : actions_)
    {
        ea::erase_if(
            kv.second.controllerAxes_, [=](const Detail::ControllerAxisMapping& m) { return m.OverlapsWith(mapping); });
    }
    auto& map = GetOrAddMapping(action);
    map.controllerAxes_.push_back(mapping);
}

void InputMap::MapHat(const ea::string& action, HatPosition hatPosition)
{
    for (auto kv : actions_)
    {
        ea::erase_if(
            kv.second.controllerHats_, [=](const Detail::ControllerHatMapping& m) { return m.hatPosition_ == hatPosition; });
    }
    auto& map = GetOrAddMapping(action);
    map.controllerHats_.emplace_back(hatPosition);
}

void InputMap::MapMouseButton(const ea::string& action, MouseButton mouseButton)
{
    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.mouseButtons_,
            [=](const Detail::MouseButtonMapping& m) { return m.mouseButton_ == mouseButton; });
    }
    auto& map = GetOrAddMapping(action);
    map.mouseButtons_.emplace_back(mouseButton);
}

const Detail::ActionMapping& InputMap::GetMapping(const ea::string& action)
{
    static Detail::ActionMapping empty;
    auto it = actions_.find(action);
    if (it == actions_.end())
        return empty;
    return it->second;
}

float InputMap::Evaluate(const ea::string& action)
{
    static Detail::ActionMapping empty;
    auto it = actions_.find(action);
    if (it == actions_.end())
        return 0.0f;
    return it->second.Evaluate(context_->GetSubsystem<Input>());
}

Detail::ActionMapping& InputMap::GetOrAddMapping(const ea::string& action)
{
    return actions_[action];
}


const char* const* InputMap::GetScanCodeNames()
{
    return scancodeNames;
}


} // namespace Urho3D
