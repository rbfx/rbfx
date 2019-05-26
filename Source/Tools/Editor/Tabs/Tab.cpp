//
// Copyright (c) 2017-2019 the rbfx project.
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
            if (dock->IsCentralNode())
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

                if (auto* window = ui::FindWindowByName(tab->GetUniqueTitle().c_str()))
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
            ui::SetNextWindowDockID(targetID, ImGuiCond_Once);
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

        ui::Begin(uniqueTitle_.c_str(), &open_, windowFlags_);
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

void Tab::SetTitle(const ea::string& title)
{
    title_ = title;
    UpdateUniqueTitle();
}

void Tab::UpdateUniqueTitle()
{
    uniqueTitle_ = ToString("%s###%s", title_.c_str(), id_.c_str());
}

IntRect Tab::UpdateViewRect()
{
    IntRect tabRect = ToIntRect(ui::GetCurrentWindow()->ContentsRegionRect);
    return tabRect;
}

void Tab::OnSaveUISettings(ImGuiTextBuffer* buf)
{
    buf->appendf("\n[Project][%s###%s]\n", GetTypeName().c_str(), GetID().c_str());
}

void Tab::OnLoadUISettings(const char* name, const char* line)
{
    SetID(ea::string(name).split('#')[1]);
}

bool Tab::LoadResource(const ea::string& resourcePath)
{
    // Resource loading is only allowed when scene is not playing.
    if (auto* tab = GetSubsystem<Editor>()->GetTab<PreviewTab>())
        return tab->GetSceneSimulationStatus() == SCENE_SIMULATION_STOPPED;
    return true;
}

bool Tab::SaveResource()
{
    // Resource loading is only allowed when scene is not playing.
    if (auto* tab = GetSubsystem<Editor>()->GetTab<PreviewTab>())
        return tab->GetSceneSimulationStatus() == SCENE_SIMULATION_STOPPED;
    return true;
}

void Tab::SetID(const ea::string& id)
{
    id_ = id;
    uniqueName_ = GetTypeName() + "###" + id;
    UpdateUniqueTitle();
}


}
