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

#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>

#include <SDL/SDL_keyboard.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "KeyBindings.h"

namespace Urho3D
{

KeyBindings::KeyBindings(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_INPUTEND, URHO3D_HANDLER(KeyBindings, OnInputEnd));

    actions_[ActionType::SaveProject] = {"Save project", KEY_S, QUAL_CTRL};
    actions_[ActionType::OpenProject] = {"Open project", KEY_O, QUAL_CTRL};
    actions_[ActionType::Exit] = {"Exit", KEY_F4, QUAL_ALT};

    for (int i = 0; i < ActionType::MaxCount; i++)
    {
        actions_[i].binding_ = KeysToString(actions_[i].qualifiers_, actions_[i].key_);
        defaults_[i] = actions_[i];
    }
}

bool KeyBindings::Serialize(Archive& archive)
{
    Input* input = GetSubsystem<Input>();
    if (auto bindings = archive.OpenSequentialBlock("keyBindings"))
    {
        for (int i = 0; i < ActionType::MaxCount; i++)
        {
            if (auto bind = archive.OpenUnorderedBlock("bind"))
            {
                SerializeValue(archive, "name", actions_[i].title_);
                SerializeValue(archive, "bind", actions_[i].binding_);
                if (archive.IsInput())
                {
                    KeyBoundAction& action = actions_[i];
                    ea::vector<ea::string> parts = action.binding_.split('+');
                    for (const ea::string& part : parts)
                    {
                        // TODO: MacOS
                        if (part == "Shift")
                            action.qualifiers_ |= QUAL_SHIFT;
                        else if (part == "Alt")
                            action.qualifiers_ |= QUAL_ALT;
                        else if (part == "Ctrl")
                            action.qualifiers_ |= QUAL_CTRL;
                        else
                            action.key_ = input->GetKeyFromName(part);
                    }
                }
            }
        }
    }
    return true;
}

void KeyBindings::RenderUI()
{
    const ImGuiIO& io = ui::GetIO();
    // TODO: Use tables when they come out.
    ui::Columns(2);
    for (int i = 0; i < ActionType::MaxCount; i++)
    {
        KeyBoundAction& action = actions_[i];
        ui::TextUnformatted(action.title_.c_str());
        ui::NextColumn();
        ui::PushID(i);
        ui::InputText("##key_binding", const_cast<char*>(action.binding_.data()), action.binding_.length(),
            ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_NoUndoRedo|ImGuiInputTextFlags_AutoSelectAll);
        if (ui::IsItemActive())
        {
            ignoreKeyPresses_ = true;
            for (int k = 0; k < SDL_NUM_SCANCODES; k++)
            {
                auto scancode = static_cast<SDL_Scancode>(k);
                if (scancode == SDL_SCANCODE_LCTRL || scancode == SDL_SCANCODE_RCTRL ||
                    scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT ||
                    scancode == SDL_SCANCODE_LALT || scancode == SDL_SCANCODE_RALT)
                    continue;

                // Input subsystem does not receive key input when any item is active.
                if (ui::IsKeyPressed(scancode))
                {
                    Key pressedKey = static_cast<Key>(SDL_GetKeyFromScancode(scancode));
                    QualifierFlags pressedQualifiers = GetCurrentQualifiers();

                    // Clear existing key binding
                    for (int j = 0; j < ActionType::MaxCount; j++)
                    {
                        if (i == j)
                            continue;

                        if (actions_[j].key_ == pressedKey && actions_[j].qualifiers_ == pressedQualifiers)
                        {
                            actions_[j].key_ = KEY_UNKNOWN;
                            actions_[j].qualifiers_ = QUAL_NONE;
                            actions_[j].binding_.clear();
                            break;
                        }
                    }

                    // Save new key binding
                    if (action.key_ == KEY_ESCAPE)          // ESC clears key bindings
                    {
                        action.key_ = KEY_UNKNOWN;
                        action.qualifiers_ = QUAL_NONE;
                        action.binding_.clear();
                    }
                    else
                    {
                        action.qualifiers_ = pressedQualifiers;
                        action.key_ = pressedKey;
                        action.binding_ = KeysToString(pressedQualifiers, pressedKey);
                    }
                    break;
                }
            }
        }
        ui::SameLine();
        if (ui::Button(ICON_FA_UNDO))
        {
            action.key_ = defaults_[i].key_;
            action.qualifiers_ = defaults_[i].qualifiers_;
            action.binding_ = KeysToString(action.qualifiers_, action.key_);
        }
        ui::PopID();
        ui::NextColumn();
    }
    ui::Columns();
    if (ui::Button(ICON_FA_UNDO " Restore Defaults"))
    {
        for (int i = 0; i < ActionType::MaxCount; i++)
        {
            KeyBoundAction& action = actions_[i];
            action.key_ = defaults_[i].key_;
            action.qualifiers_ = defaults_[i].qualifiers_;
            action.binding_ = KeysToString(action.qualifiers_, action.key_);
        }
    }
}

void KeyBindings::OnInputEnd(StringHash, VariantMap&)
{
    if (ignoreKeyPresses_)
    {
        ignoreKeyPresses_ = false;
        return;
    }

    for (KeyBoundAction& action : actions_)
    {
        QualifierFlags qualifiersDown = GetCurrentQualifiers() & action.qualifiers_;
        bool keyDown = false;
        if (qualifiersDown == action.qualifiers_)
        {
            bool keyPressed = ui::IsKeyPressed(action.key_);
            keyDown = keyPressed || ui::IsKeyDown(action.key_);
            if (keyPressed)
                action.onPressed_(this);
        }
        action.isDown_ = keyDown;
    }
}

ea::string KeyBindings::KeysToString(QualifierFlags qualifiers, Key key)
{
    Input* input = GetSubsystem<Input>();
    ea::string name;
    if (qualifiers & QUAL_SHIFT)
        name += "Shift+";
    if (qualifiers & QUAL_CTRL)
        name += "Ctrl+";
    if (qualifiers & QUAL_ALT)
        name += "Alt+";
    name += input->GetKeyName(key);
    return name;
}

const char* KeyBindings::GetKeyCombination(ActionType actionType)
{
    return actions_[actionType].binding_.c_str();
}

QualifierFlags KeyBindings::GetCurrentQualifiers() const
{
    const ImGuiIO& io = ui::GetIO();
    QualifierFlags  pressedQualifiers = QUAL_NONE;
    if (io.KeyShift)
        pressedQualifiers |= QUAL_SHIFT;
    if (io.KeyCtrl)
        pressedQualifiers |= QUAL_CTRL;
    if (io.KeyAlt)
        pressedQualifiers |= QUAL_ALT;
    return pressedQualifiers;
}

}
