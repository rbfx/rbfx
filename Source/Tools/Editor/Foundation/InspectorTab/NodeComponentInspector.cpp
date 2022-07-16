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

#include "../../Foundation/InspectorTab/NodeComponentInspector.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_NodeComponentInspector(Context* context, InspectorTab_* inspectorTab)
{
    inspectorTab->RegisterAddon<NodeComponentInspector>(inspectorTab->GetProject());
}

NodeComponentInspector::NodeComponentInspector(ProjectEditor* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.Subscribe(this, &NodeComponentInspector::OnProjectRequest);
}

void NodeComponentInspector::OnProjectRequest(ProjectRequest* request)
{
    // TODO(editor): Support components
    auto inspectNodeComponentRequest = dynamic_cast<InspectNodeComponentRequest*>(request);
    if (!inspectNodeComponentRequest || !inspectNodeComponentRequest->HasNodes())
        return;

    request->QueueProcessCallback([=]()
    {
        if (nodes_ != inspectNodeComponentRequest->GetNodes()
            || components_ != inspectNodeComponentRequest->GetComponents())
        {
            nodes_ = inspectNodeComponentRequest->GetNodes();
            components_ = inspectNodeComponentRequest->GetComponents();
            InspectObjects();
        }
        OnActivated(this);
    });
}

void NodeComponentInspector::InspectObjects()
{
    auto cache = GetSubsystem<ResourceCache>();

    if (nodes_.empty())
    {
        widget_ = nullptr;
        return;
    }

    SerializableInspectorWidget::SerializableVector objects;
    ea::copy(nodes_.begin(), nodes_.end(), ea::back_inserter(objects));

    widget_ = MakeShared<SerializableInspectorWidget>(context_, objects);
    widget_->OnEditBegin.Subscribe(this, &NodeComponentInspector::BeginEdit);
    widget_->OnEditEnd.Subscribe(this, &NodeComponentInspector::EndEdit);
}

void NodeComponentInspector::BeginEdit()
{

}

void NodeComponentInspector::EndEdit()
{

}

void NodeComponentInspector::RenderContent()
{
    if (!widget_)
        return;

    widget_->RenderTitle();
    ui::Separator();
    widget_->RenderContent();
}

void NodeComponentInspector::RenderContextMenuItems()
{
}

void NodeComponentInspector::RenderMenu()
{
}

void NodeComponentInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
