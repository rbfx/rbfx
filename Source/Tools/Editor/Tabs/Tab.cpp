//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Input/Input.h>
#include "EditorEvents.h"
#include "Editor.h"
#include "Tab.h"


namespace Urho3D
{


Tab::Tab(Context* context)
    : Object(context)
    , inspector_(context)
{
    SetID(GenerateUUID());

    SubscribeToEvent(E_EDITORPROJECTSAVING, [&](StringHash, VariantMap& args) {
        using namespace EditorProjectSaving;
        JSONValue& root = *(JSONValue*)args[P_ROOT].GetVoidPtr();
        auto& tabs = root["tabs"];
        JSONValue tab;
        OnSaveProject(tab);
        tabs.Push(tab);
    });
}

void Tab::Initialize(const String& title, const Vector2& initSize, ui::DockSlot initPosition, const String& afterDockName)
{
    initialSize_ = initSize;
    placePosition_ = initPosition;
    placeAfter_ = afterDockName;
    title_ = title;
    SetID(GenerateUUID());
}

Tab::~Tab()
{
    SendEvent(E_EDITORTABCLOSED, EditorTabClosed::P_TAB, this);
}

bool Tab::RenderWindow()
{
    Input* input = GetSubsystem<Input>();
    if (input->IsMouseVisible())
        lastMousePosition_ = input->GetMousePosition();

    ui::SetNextDockPos(placeAfter_.Empty() ? nullptr : placeAfter_.CString(), placePosition_, ImGuiCond_FirstUseEver);
    if (ui::BeginDock(uniqueTitle_.CString(), &open_, windowFlags_, ToImGui(initialSize_)))
    {
        if (open_)
        {
            IntRect tabRect = ToIntRect(ui::GetCurrentWindow()->InnerRect);
            if (tabRect.IsInside(lastMousePosition_) == INSIDE)
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

            open_ = RenderWindowContent();

            isRendered_ = true;
        }
    }
    else
    {
        isActive_ = false;
        isRendered_ = false;
    }
    ui::EndDock();

    return open_;
}

void Tab::SetTitle(const String& title)
{
    title_ = title;
    UpdateUniqueTitle();
}

void Tab::UpdateUniqueTitle()
{
    uniqueTitle_ = ToString("%s###%s", title_.CString(), id_.CString());
}

IntRect Tab::UpdateViewRect()
{
    IntRect tabRect = ToIntRect(ui::GetCurrentWindow()->InnerRect);
    tabRect += IntRect(0, static_cast<int>(ui::GetCursorPosY()), 0, 0);
    return tabRect;
}

void Tab::OnSaveProject(JSONValue& tab)
{
    tab["type"] = GetTypeName();
    tab["uuid"] = GetID();
}

void Tab::OnLoadProject(const JSONValue& tab)
{
    SetID(tab["uuid"].GetString());
}

void Tab::AutoPlace()
{
    String afterTabName;
    auto placement = ui::Slot_None;
    auto tabs = GetSubsystem<Editor>()->GetContentTabs();

    // Need a separate loop because we prefer consile (as per default layout) but it may come after hierarchy in tabs list.
    for (const auto& openTab : tabs)
    {
        if (openTab == this)
            continue;

        if (openTab->GetTitle() == "Console")
        {
            if (afterTabName.Empty())
            {
                // Place after hierarchy if no content tab exist
                afterTabName = openTab->GetUniqueTitle();
                placement = ui::Slot_Top;
            }
        }
    }

    for (const auto& openTab : tabs)
    {
        if (openTab == this)
            continue;

        if (openTab->GetTitle() == "Hierarchy")
        {
            if (afterTabName.Empty())
            {
                // Place after hierarchy if no content tab exist
                afterTabName = openTab->GetUniqueTitle();
                placement = ui::Slot_Right;
            }
        }
        else if (!openTab->IsUtility())
        {
            // Place after content tab
            afterTabName = openTab->GetUniqueTitle();
            placement = ui::Slot_Tab;
        }
    }

    initialSize_ = {-1, GetGraphics()->GetHeight() * 0.9f};
    placeAfter_ = afterTabName;
    placePosition_ = placement;
}

}
