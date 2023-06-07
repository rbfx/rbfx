//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../Precompiled.h"

#include "../SystemUI/ImGui.h"

#include "Urho3D/Graphics/Texture2D.h"

#include <SDL.h>

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
    return IsKeyDown(SDL_GetScancodeFromKey(key));
}

bool ui::IsKeyPressed(Urho3D::Key key, bool repeat)
{
    return IsKeyPressed(SDL_GetScancodeFromKey(key), repeat);
}

bool ui::IsKeyReleased(Urho3D::Key key)
{
    return IsKeyReleased(SDL_GetScancodeFromKey(key));
}

int ui::GetKeyPressedAmount(Urho3D::Key key, float repeatDelay, float rate)
{
    return GetKeyPressedAmount(SDL_GetScancodeFromKey(key), repeatDelay, rate);
}

float ui::GetMouseWheel()
{
    ImGuiContext& g = *GImGui;
    return g.IO.MouseWheel;
}
