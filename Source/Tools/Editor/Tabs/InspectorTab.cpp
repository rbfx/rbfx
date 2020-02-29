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

#include <Urho3D/IO/Log.h>
#include <Urho3D/SystemUI/SystemUI.h>
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
    context_->RegisterSubsystem(this);
}

bool InspectorTab::RenderWindowContent()
{
    ui::PushItemWidth(-1);
    ui::InputText("###Filter", &filter_);
    ui::PopItemWidth();
    if (ui::IsItemHovered())
        ui::SetTooltip("Filter attributes by name.");

    auto* editor = GetSubsystem<Editor>();
    InspectArgs args;
    args.filter_ = filter_;
    for (const auto& pair : inspected_)
    {
        args.object_ = pair.first;
        args.eventSender_ = pair.second;
        args.handledTimes_ = 0;
        editor->onInspect_(this, args);
    }

    return true;
}

void InspectorTab::Clear()
{
    inspected_.clear();
}

void InspectorTab::Inspect(Object* object, Object* eventSender)
{
    if (object == nullptr)
    {
        URHO3D_LOGERROR("Editor can not inspect a null object.");
        return;
    }
    if (eventSender == nullptr)
        eventSender = object;
    inspected_.push_back(ea::make_pair(WeakPtr(object), WeakPtr(eventSender)));
}

bool InspectorTab::IsInspected(Object* object) const
{
    if (object == nullptr)
        return false;

    for (const auto& pair : inspected_)
    {
        if (pair.first.Get() == object)
            return true;
    }

    return false;
}

}
