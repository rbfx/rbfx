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

#include "Urho3D/Core/Context.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Input/InputConstants.h"
#include "Urho3D/Input/InputMap.h"
#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/UI/UI.h"

namespace Urho3D
{
namespace
{

static ea::array<ea::string_view, 291> scancodeNames{{
    "", // 0
    "Ctrl",
    "Shift",
    "Alt",
    "A", // 4
    "B",
    "C",
    "D",
    "E",
    "F",
    "G", // 10
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q", // 20
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "1", // 30
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "Return", // 40
    "Escape",
    "Backspace",
    "Tab",
    "Space",
    "-",
    "=",
    "[",
    "]",
    "\\",
    "#", // 50
    ";",
    "'",
    "`",
    ",",
    ".",
    "/",
    "CapsLock",
    "F1",
    "F2",
    "F3", // 60
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "PrintScreen", // 70
    "ScrollLock",
    "Pause",
    "Insert",
    "Home",
    "PageUp",
    "Delete",
    "End",
    "PageDown",
    "Right",
    "Left", // 80
    "Down",
    "Up",
    "Numlock",
    "Keypad /",
    "Keypad *",
    "Keypad -",
    "Keypad +",
    "Keypad Enter",
    "Keypad 1",
    "Keypad 2", // 90
    "Keypad 3",
    "Keypad 4",
    "Keypad 5",
    "Keypad 6",
    "Keypad 7",
    "Keypad 8",
    "Keypad 9",
    "Keypad 0",
    "Keypad .",
    "Non US Backslash", // 100
    "Application",
    "Power",
    "Keypad =",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19", // 110
    "F20",
    "F21",
    "F22",
    "F23",
    "F24", // 115
    "Execute",
    "Help",
    "Menu",
    "Select",
    "Stop", // 120
    "Again",
    "Undo",
    "Cut",
    "Copy",
    "Paste",
    "Find",
    "Mute",
    "Volume Up",
    "Volume Down",
    "Locking Caps Lock", // 130
    "Locking Num Lock",
    "Locking Scroll Lock",
    "KP Comma", // 133
    "KP EQUALSAS400",
    "International1", // 135
    "International2",
    "International3",
    "International4",
    "International5",
    "International6", // 140
    "International7",
    "International8",
    "International9",
    "Lang1",
    "Lang2",
    "Lang3",
    "Lang4",
    "Lang5",
    "Lang6",
    "Lang7", // 150
    "Lang8",
    "Lang9",
    "AltErase",
    "SysReq",
    "Cancel",
    "Clear",
    "Prior",
    "Return",
    "Separator",
    "Out", // 160
    "Oper",
    "Clear / Again",
    "CrSel",
    "ExSel", // 164
    "165",
    "166",
    "167",
    "168",
    "169",
    "170", // 170
    "171",
    "172",
    "173",
    "174",
    "175",
    "Keypad 00", // 176
    "Keypad 000",
    "ThousandsSeparator",
    "DecimalSeparator",
    "CurrencyUnit", // 180
    "CurrencySubUnit",
    "Keypad (",
    "Keypad )",
    "Keypad {",
    "Keypad }",
    "Keypad Tab",
    "Keypad Backspace",
    "Keypad A",
    "Keypad B",
    "Keypad C", // 190
    "Keypad D",
    "Keypad E",
    "Keypad F",
    "Keypad XOR",
    "Keypad ^",
    "Keypad %",
    "Keypad <",
    "Keypad >",
    "Keypad &",
    "Keypad &&", // 200
    "Keypad |",
    "Keypad ||",
    "Keypad :",
    "Keypad #",
    "Keypad Space",
    "Keypad @",
    "Keypad !",
    "Keypad MemStore",
    "Keypad MemRecall",
    "Keypad MemClear", // 210
    "Keypad MemAdd",
    "Keypad MemSubtract",
    "Keypad MemMultiply",
    "Keypad MemDivide",
    "Keypad +/-",
    "Keypad Clear",
    "Keypad ClearEntry",
    "Keypad Binary",
    "Keypad Octal",
    "Keypad Decimal", // 220
    "Keypad Hexadecimal",
    "222",
    "223",
    "Left Ctrl",
    "Left Shift",
    "Left Alt",
    "Left GUI",
    "Right Ctrl",
    "Right Shift",
    "Right Alt", // 230
    "Right GUI",
    "232",
    "233",
    "234",
    "235",
    "236",
    "237",
    "238",
    "239",
    "240",
    "241",
    "242",
    "243",
    "244",
    "245",
    "246",
    "247",
    "248",
    "249",
    "250",
    "251",
    "252",
    "253",
    "254",
    "255",
    "256",
    "ModeSwitch", // 257
    "AudioNext",
    "AudioPrev",
    "AudioStop", // 260
    "AudioPlay",
    "AudioMute",
    "MediaSelect",
    "WWW",
    "Mail",
    "Calculator",
    "Computer",
    "AC Search",
    "AC Home",
    "AC Back", // 270
    "AC Forward",
    "AC Stop",
    "AC Refresh",
    "AC Bookmarks",
    "BrightnessDown",
    "BrightnessUp",
    "DisplaySwitch",
    "KBDIllumToggle",
    "KBDIllumDown",
    "KBDIllumUp", // 280
    "Eject",
    "Sleep",
    "App1",
    "App2",
    "AudioRewind",
    "AudioFastForward",
    "SoftLeft",
    "SoftRight",
    "Call",
    "EndCall" // 290
}};

static ea::array<ea::string_view, 21> controllerButtonNames{
    {"A", "B", "X", "Y", "Back", "Guide", "Start", "LeftStick", "RightStick", "LeftShoulder", "RightShoulder", "Up",
        "Down", "Left", "Right", "Misc1", "Paddle1", "Paddle2", "Paddle3", "Paddle4", "Touchpad"}};

static ea::array<ea::string_view, 4> controllerHatNames{{"Up", "Right", "Down", "Left"}};

static ea::array<ea::string_view, 5> mouseButtonNames{{"Left", "Middle", "Right", "X1", "X2"}};
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
    if (controller_)
    {
        SerializeOptionalValue(archive, "button", button_, 0,
            [&](Archive& archive, const char* name, auto& value)
            { SerializeEnum<unsigned, unsigned>(archive, name, value, InputMap::GetControllerButtonNames()); });
    }
    else
    {
        SerializeOptionalValue(archive, "button", button_, 0);
    }
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
    SerializeOptionalValue(archive, "hat", hatPosition_, 0,
        [&](Archive& archive, const char* name, auto& value)
        { SerializeEnum<unsigned, unsigned>(archive, name, value, InputMap::GetControllerHatNames()); });
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
    SerializeOptionalValue(archive, "button", mouseButton_, 0,
        [&](Archive& archive, const char* name, auto& value)
        { SerializeEnum<unsigned, unsigned>(archive, name, value, InputMap::GetMouseButtonNames()); });
}

MouseButtonFlags MouseButtonMapping::GetMask() const
{
    return static_cast<MouseButtonFlags>(1 << mouseButton_);
}

ScreenButtonMapping::ScreenButtonMapping()
{
}

ScreenButtonMapping::ScreenButtonMapping(const ea::string& elementName)
    : elementName_(elementName)
{
}

void ScreenButtonMapping::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "elementName", elementName_, EMPTY_STRING);
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
    SerializeOptionalValue(archive, "screenButtons", screenButtons_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "element"); });
}

float ActionMapping::Evaluate(Input* input, UI* ui, float deadZone, int ignoreJoystickId) const
{
    const UIElement* elementInFocus = (ui != nullptr) ? ui->GetFocusElement(): nullptr;

    if (!elementInFocus)
    {
        for (auto& key : keyboardKeys_)
        {
            switch (key.scancode_)
            {
            case 1:
                if (input->GetKeyDown(KEY_LCTRL) || input->GetKeyDown(KEY_RCTRL))
                    return 1.0f;
                break;
            case 2:
                if (input->GetKeyDown(KEY_LSHIFT) || input->GetKeyDown(KEY_RSHIFT))
                    return 1.0f;
                break;
            case 3:
                if (input->GetKeyDown(KEY_LALT) || input->GetKeyDown(KEY_RALT))
                    return 1.0f;
                break;
            }
            if (input->GetScancodeDown(key.scancode_))
            {
                return 1.0f;
            }
        }
    }
    for (auto& button : mouseButtons_)
    {
        if (input->GetMouseButtonDown(button.GetMask()))
        {
            return 1.0f;
        }
    }
    if (controllerAxes_.empty() && controllerButtons_.empty() && controllerHats_.empty())
    {
        return 0.0f;
    }

    if (ui != nullptr && !screenButtons_.empty())
    {
        const unsigned numTouches = input->GetNumTouches();
        for (unsigned i = 0; i < numTouches; ++i)
        {
            const auto touch = input->GetTouch(i);
            if (touch && touch->GetTouchedElement())
            {
                for (auto& screenButton : screenButtons_)
                {
                    if (screenButton.elementName_ == touch->GetTouchedElement()->GetName())
                    {
                        return 1.0f;
                    }
                }
            }
        }
    }

    const unsigned numJoysticks = input->GetNumJoysticks();
    float sum = 0.0f;
    for (unsigned i = 0; i < numJoysticks; ++i)
    {
        const auto state = input->GetJoystickByIndex(i);
        if (state)
        {
            // Ignore accelerometer.
            if (state->joystickID_ == ignoreJoystickId)
                continue;

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
                const auto hatPos = state->GetHatPosition(0);
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
                    const auto pos = state->GetAxisPosition(axis.axis_);
                    sum += axis.Translate(pos, deadZone);
                }
            }
        }
    }
    return Clamp(sum, 0.0f, 1.0f);
}

} // namespace Detail

InputMap::InputMap(Context* context)
    : BaseClassName(context)
{
    ignoreJoystickId_ = GetSubsystem<Input>()->FindAccelerometerJoystickId();
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
    SerializeOptionalValue(archive, "metadata", metadata_, EmptyObject{});
    SerializeOptionalValue(archive, "deadzone", deadZone_, DEFAULT_DEADZONE);
    SerializeMap(archive, "actions", actions_, "action");
}

void InputMap::SetDeadZone(float deadZone)
{
    deadZone_ = Max(0.0f, deadZone);
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
    unsigned value;

    switch (mouseButton)
    {
    case MOUSEB_LEFT: value = SDL_BUTTON_LEFT - 1; break;
    case MOUSEB_RIGHT: value = SDL_BUTTON_RIGHT - 1; break;
    case MOUSEB_MIDDLE: value = SDL_BUTTON_MIDDLE - 1; break;
    case MOUSEB_X1: value = SDL_BUTTON_X1 - 1; break;
    case MOUSEB_X2: value = SDL_BUTTON_X2 - 1; break;
        break;
    default: URHO3D_LOGERROR("Can't map mouse button. Invalid MouseButton value.");
        return;
    }

    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.mouseButtons_,
            [=](const Detail::MouseButtonMapping& m) { return m.mouseButton_ == mouseButton; });
    }
    auto& map = GetOrAddMapping(action);
    map.mouseButtons_.emplace_back(mouseButton);
}

void InputMap::MapScreenButton(const ea::string& action, const ea::string& elementName)
{
    for (auto kv : actions_)
    {
        ea::erase_if(kv.second.screenButtons_,
            [=](const Detail::ScreenButtonMapping& m) { return m.elementName_ == elementName; });
    }
    auto& map = GetOrAddMapping(action);
    map.screenButtons_.emplace_back(elementName);
}

const Detail::ActionMapping& InputMap::GetMapping(const ea::string& action) const
{
    static Detail::ActionMapping empty;
    const auto iter = actions_.find(action);
    if (iter == actions_.end())
        return empty;
    return iter->second;
}

const Detail::ActionMapping& InputMap::GetMappingByHash(StringHash actionHash) const
{
    static Detail::ActionMapping empty;
    const auto iter = actions_.find_by_hash(actionHash.Value());
    if (iter == actions_.end())
        return empty;
    return iter->second;
}


void InputMap::AddMetadata(const ea::string& name, const Variant& value)
{
    metadata_.insert_or_assign(name, value);
}

void InputMap::RemoveMetadata(const ea::string& name)
{
    metadata_.erase(name);
}

void InputMap::RemoveAllMetadata()
{
    metadata_.clear();
}

const Variant& InputMap::GetMetadata(const ea::string& name) const
{
    auto it = metadata_.find(name);
    if (it != metadata_.end())
        return it->second;

    return Variant::EMPTY;
}

bool InputMap::HasMetadata() const
{
    return !metadata_.empty();
}

float InputMap::Evaluate(const ea::string& action)
{
    static Detail::ActionMapping empty;
    auto iter = actions_.find(action);
    if (iter == actions_.end())
        return 0.0f;
    const auto input = context_->GetSubsystem<Input>();
    const auto ui = context_->GetSubsystem<UI>();
    return iter->second.Evaluate(input, ui, deadZone_, ignoreJoystickId_);
}

float InputMap::EvaluateByHash(StringHash actionHash)
{
    const auto iter = actions_.find_by_hash(actionHash.Value());
    if (iter != actions_.end())
        return 0.0f;
    const auto input = context_->GetSubsystem<Input>();
    const auto ui = context_->GetSubsystem<UI>();
    return iter->second.Evaluate(input, ui, deadZone_, ignoreJoystickId_);
}

Detail::ActionMapping& InputMap::GetOrAddMapping(const ea::string& action)
{
    return actions_[action];
}


ea::span<ea::string_view> InputMap::GetScanCodeNames()
{
    return scancodeNames;
}

ea::span<ea::string_view> InputMap::GetControllerButtonNames()
{
    return controllerButtonNames;
}

ea::span<ea::string_view> InputMap::GetControllerHatNames()
{
    return controllerHatNames;
}

ea::span<ea::string_view> InputMap::GetMouseButtonNames()
{
    return mouseButtonNames;
}

SharedPtr<InputMap> InputMap::Load(Context* context, const ea::string& name)
{
    SharedPtr<InputMap> result;
    if (name.empty())
    {
        return result;
    }

    auto* cache = context->GetSubsystem<ResourceCache>();
    const auto* vfs = context->GetSubsystem<VirtualFileSystem>();

    const auto configFileId = FileIdentifier("config", name);
    if (vfs->Exists(configFileId))
    {
        result = MakeShared<InputMap>(context);
        result->SetName(name);
        if (result->LoadFile(configFileId))
            return result;
    }
    result = cache->GetResource<InputMap>(name);
    return result;
}

} // namespace Urho3D
