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

#include <Urho3D/IO/Log.h>
#include <ImGui/imgui.h>
#include <ImGui/imgui_stdlib.h>
#include "EditorEvents.h"
#include "InspectorTab.h"
#include "Editor.h"


namespace Urho3D
{

InspectorTab::InspectorTab(Context* context)
    : Tab(context)
{
    SetID("6e62fa62-811c-4bf2-9b85-bffaf7be239f");
    SetTitle("Inspector");
    isUtility_ = true;
}

bool InspectorTab::RenderWindowContent()
{
    ui::PushItemWidth(-1);
    ui::InputText("###Filter", &filter_);
    ui::PopItemWidth();
    if (ui::IsItemHovered())
        ui::SetTooltip("Filter attributes by name.");

    if (provider_.first.NotNull())
        provider_.second->RenderInspector(filter_.c_str());

    return true;
}

void InspectorTab::SetProvider(IInspectorProvider* provider)
{
    if (provider_.first.NotNull() && provider_.second != provider)
        provider_.second->ClearSelection();

    if (auto* ptr = dynamic_cast<RefCounted*>(provider))
    {
        provider_.first = ptr;
        provider_.second = provider;
    }
    else
        URHO3D_LOGERROR("Classes that inherit IInspectorProvider must also inherit RefCounted.");
}

}
