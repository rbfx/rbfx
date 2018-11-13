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
#include "PreviewTab.h"


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

Tab::~Tab()
{
    SendEvent(E_EDITORTABCLOSED, EditorTabClosed::P_TAB, this);
}

bool Tab::RenderWindow()
{
    Input* input = GetSubsystem<Input>();
    if (input->IsMouseVisible())
        lastMousePosition_ = input->GetMousePosition();

    if (autoPlace_)
    {
        autoPlace_ = false;

        // Find empty dockspace
        std::function<ImGuiDockNode*(ImGuiDockNode*)> returnTargetDockspace = [&](ImGuiDockNode* dock) -> ImGuiDockNode* {
            if (dock == nullptr)
                return nullptr;
            if (dock->IsCentralNode)
                return dock;
            else if (auto* node = returnTargetDockspace(dock->ChildNodes[0]))
                return node;
            else if (auto* node = returnTargetDockspace(dock->ChildNodes[1]))
                return node;
            return nullptr;
        };

        ImGuiID targetID = 0;
        ImGuiDockNode* dockspaceRoot = ui::DockBuilderGetNode(GetSubsystem<Editor>()->GetDockspaceID());
        ImGuiDockNode* currentRoot = returnTargetDockspace(dockspaceRoot);
        if (currentRoot->Windows.empty())
        {
            // Free space exists, dock new window there.
            targetID = currentRoot->ID;
        }
        else
        {
            // Find biggest window and dock to it as a tab.
            auto tabs = GetSubsystem<Editor>()->GetContentTabs();
            float maxSize = 0;
            for (auto& tab : tabs)
            {
                if (tab->GetUniqueTitle() == uniqueTitle_)
                    continue;

                if (auto* window = ui::FindWindowByName(tab->GetUniqueTitle().CString()))
                {
                    float thisWindowSize = window->Size.x * window->Size.y;
                    if (thisWindowSize > maxSize)
                    {
                        maxSize = thisWindowSize;
                        targetID = window->DockId;
                    }
                }
            }
        }

        if (targetID)
            ui::SetNextWindowDockId(targetID, ImGuiCond_Once);
    }
    bool wasRendered = isRendered_;
    wasOpen_ = open_;
    if (open_)
    {
        OnBeforeBegin();

        if (IsModified())
            windowFlags_ |= ImGuiWindowFlags_UnsavedDocument;
        else
            windowFlags_ &= ~ImGuiWindowFlags_UnsavedDocument;

        ui::Begin(uniqueTitle_.CString(), &open_, windowFlags_);
        {
            OnAfterBegin();
            if (!ui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (!wasRendered)                                                                                   // Just activated
                    ui::SetWindowFocus();
                else if (input->IsMouseVisible() && ui::IsAnyMouseDown())
                {
                    if (ui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))                                        // Interacting
                        ui::SetWindowFocus();
                }
            }

            isActive_ = ui::IsWindowFocused();
            bool shouldBeOpen = RenderWindowContent();
            if (open_)
                open_ = shouldBeOpen;
            else
            {
                // Tab is possibly closing, lets not override that condition.
            }
            isRendered_ = true;
            OnBeforeEnd();
        }

        ui::End();
        OnAfterEnd();
    }
    else
    {
        isActive_ = false;
        isRendered_ = false;
    }

    if (activateTab_)
    {
        ui::SetWindowFocus();
        open_ = true;
        isActive_ = true;
        activateTab_ = false;
    }

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
    IntRect tabRect = ToIntRect(ui::GetCurrentWindow()->InnerClipRect);
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

bool Tab::LoadResource(const String& resourcePath)
{
    // Resource loading is only allowed when scene is not playing.
    return GetSubsystem<Editor>()->GetTab<PreviewTab>()->GetSceneSimulationStatus() == SCENE_SIMULATION_STOPPED;
}

bool Tab::SaveResource()
{
    // Resource loading is only allowed when scene is not playing.
    return GetSubsystem<Editor>()->GetTab<PreviewTab>()->GetSceneSimulationStatus() == SCENE_SIMULATION_STOPPED;
}


}
