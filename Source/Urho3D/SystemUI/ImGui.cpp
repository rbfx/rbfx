// Copyright (c) 2008-2017 the Urho3D project.
// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/ImGui.h"

#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Input/Input.h"

#include <SDL.h>

extern ImGuiKey ImGui_ImplSDL2_KeyEventToImGuiKey(SDL_Keycode keycode, SDL_Scancode scancode);

namespace Urho3D
{

namespace
{

template <class T>
bool AnyMouseButton(MouseButton buttons, const T& callback)
{
    bool result = false;
    for (MouseButton button : {MOUSEB_LEFT, MOUSEB_RIGHT, MOUSEB_MIDDLE, MOUSEB_X1, MOUSEB_X2})
        result |= (buttons & button) && callback(button);
    return result;
}

}

int ToImGui(MouseButton button)
{
    switch (button)
    {
    case MOUSEB_LEFT:
        return ImGuiMouseButton_Left;
    case MOUSEB_MIDDLE:
        return ImGuiMouseButton_Middle;
    case MOUSEB_RIGHT:
        return ImGuiMouseButton_Right;
    case MOUSEB_X1:
        return 3;
    case MOUSEB_X2:
        return 4;
    default:
        return -1;
    }
}

ImTextureID ToImTextureID(Texture2D* texture)
{
    return texture->GetHandles().srv_;
}

}

bool ui::IsMouseDown(Urho3D::MouseButton buttons)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsMouseDown(Urho3D::ToImGui(button)); });
}

bool ui::IsMouseDoubleClicked(Urho3D::MouseButton buttons)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsMouseDoubleClicked(Urho3D::ToImGui(button)); });
}

bool ui::IsMouseDragPastThreshold(Urho3D::MouseButton buttons, float lockThreshold)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsMouseDragPastThreshold(Urho3D::ToImGui(button), lockThreshold); });
}

bool ui::IsMouseReleased(Urho3D::MouseButton buttons)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsMouseReleased(Urho3D::ToImGui(button)); });
}

bool ui::IsMouseClicked(Urho3D::MouseButton buttons, bool repeat)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsMouseClicked(Urho3D::ToImGui(button), repeat); });
}

bool ui::IsItemClicked(Urho3D::MouseButton buttons)
{
    return Urho3D::AnyMouseButton(buttons,
        [&](Urho3D::MouseButton button) { return ui::IsItemClicked(Urho3D::ToImGui(button)); });
}

ImVec2 ui::GetMouseDragDelta(Urho3D::MouseButton button, float lockThreshold)
{
    return ui::GetMouseDragDelta(Urho3D::ToImGui(button), lockThreshold);
}

void ui::ResetMouseDragDelta(Urho3D::MouseButton button)
{
    ResetMouseDragDelta(Urho3D::ToImGui(button));
}

bool ui::IsKeyDown(Urho3D::Key key)
{
    return IsKeyDown(ImGui_ImplSDL2_KeyEventToImGuiKey((SDL_Keycode)key, (SDL_Scancode)Urho3D::Input::GetScancodeFromKey(key)));
}

bool ui::IsKeyPressed(Urho3D::Key key, bool repeat)
{
    return IsKeyPressed(ImGui_ImplSDL2_KeyEventToImGuiKey((SDL_Keycode)key, (SDL_Scancode)Urho3D::Input::GetScancodeFromKey(key)), repeat);
}

bool ui::IsKeyReleased(Urho3D::Key key)
{
    return IsKeyReleased(ImGui_ImplSDL2_KeyEventToImGuiKey((SDL_Keycode)key, (SDL_Scancode)Urho3D::Input::GetScancodeFromKey(key)));
}

int ui::GetKeyPressedAmount(Urho3D::Key key, float repeatDelay, float rate)
{
    return GetKeyPressedAmount(ImGui_ImplSDL2_KeyEventToImGuiKey((SDL_Keycode)key, (SDL_Scancode)Urho3D::Input::GetScancodeFromKey(key)), repeatDelay, rate);
}

bool ui::IsKeyDown(Urho3D::Scancode scancode)
{
    return IsKeyDown(Urho3D::Input::GetKeyFromScancode(scancode));
}

bool ui::IsKeyPressed(Urho3D::Scancode scancode, bool repeat)
{
    return IsKeyPressed(Urho3D::Input::GetKeyFromScancode(scancode), repeat);
}

bool ui::IsKeyReleased(Urho3D::Scancode scancode)
{
    return IsKeyReleased(Urho3D::Input::GetKeyFromScancode(scancode));
}

int ui::GetKeyPressedAmount(Urho3D::Scancode scancode, float repeatDelay, float rate)
{
    return GetKeyPressedAmount(Urho3D::Input::GetKeyFromScancode(scancode), repeatDelay, rate);
}

float ui::GetMouseWheel()
{
    ImGuiContext& g = *GImGui;
    return g.IO.MouseWheel;
}
