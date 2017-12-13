//
// Copyright (c) 2008-2017 the Urho3D project.
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
#include "Tab.h"


namespace Urho3D
{


Tab::Tab(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position)
    : Object(context)
    , inspector_(context)
    , placeAfter_(afterDockName)
    , placePosition_(position)
    , id_(id)
{

}

bool Tab::RenderWindow()
{
    bool open = true;

    Input* input = GetSubsystem<Input>();
    if (input->IsMouseVisible())
        lastMousePosition_ = input->GetMousePosition();

    ui::SetNextDockPos(placeAfter_.CString(), placePosition_, ImGuiCond_FirstUseEver);
    if (ui::BeginDock(uniqueTitle_.CString(), &open, windowFlags_))
    {
        if (tabRect_.IsInside(lastMousePosition_) == INSIDE)
        {
            if (!ui::IsWindowFocused() && ui::IsItemHovered() && input->GetMouseButtonDown(MOUSEB_RIGHT))
                ui::SetWindowFocus();

            if (ui::IsDockActive())
                isActive_ = ui::IsWindowFocused();
            else
                isActive_ = false;
        }
        else
            isActive_ = false;

        open = RenderWindowContent();

        isRendered_ = true;
    }
    else
    {
        isActive_ = false;
        isRendered_ = false;
    }
    ui::EndDock();

    return open;
}

void Tab::SetTitle(const String& title)
{
    title_ = title;
    uniqueTitle_ = ToString("%s###%s", title.CString(), id_.ToString().CString());
}

}
