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
    if (!inspectNodeComponentRequest || inspectNodeComponentRequest->IsEmpty())
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

NodeInspectorWidget::NodeVector NodeComponentInspector::CollectNodes() const
{
    const auto sceneIter = ea::find_if(nodes_.begin(), nodes_.end(),
        [](const Node* node) { return node->GetType() == Scene::GetTypeStatic(); });

    // If scene is selected, inspect only it
    if (sceneIter != nodes_.end())
        return {*sceneIter};

    // If nodes are present, inspect nodes
    if (!nodes_.empty())
        return {nodes_.begin(), nodes_.end()};

    return {};
}

SerializableInspectorWidget::SerializableVector NodeComponentInspector::CollectComponents() const
{
    // If components are of the same type, inspect components
    const bool sameType = ea::all_of(components_.begin(), components_.end(),
        [&](const Component* component) { return component->GetType() == components_[0]->GetType(); });
    if (sameType)
        return {components_.begin(), components_.end()};

    return {};
}

void NodeComponentInspector::InspectObjects()
{
    if (const auto nodes = CollectNodes(); !nodes.empty())
    {
        nodeWidget_ = MakeShared<NodeInspectorWidget>(context_, nodes);
        componentWidget_ = nullptr;
        componentSummary_.clear();
    }
    else if (const auto components = CollectComponents(); !components.empty())
    {
        nodeWidget_ = nullptr;
        componentWidget_ = MakeShared<SerializableInspectorWidget>(context_, components);
        componentSummary_.clear();
    }
    else
    {
        nodeWidget_ = nullptr;
        componentWidget_ = nullptr;
        componentSummary_.clear();

        for (const Component* component : components_)
            ++componentSummary_[component->GetTypeName()];
    }
}

void NodeComponentInspector::BeginEdit()
{

}

void NodeComponentInspector::EndEdit()
{

}

void NodeComponentInspector::RenderContent()
{
    if (nodeWidget_)
    {
        nodeWidget_->RenderTitle();
        ui::Separator();
        nodeWidget_->RenderContent();
    }
    else if (componentWidget_)
    {
        componentWidget_->RenderTitle();
        ui::Separator();
        componentWidget_->RenderContent();
    }
    else
    {
        RenderComponentSummary();
    }
}

void NodeComponentInspector::RenderComponentSummary()
{
    if (ui::BeginTable("##Components", 2))
    {
        ui::TableSetupColumn("Component");
        ui::TableSetupColumn("Count");
        ui::TableHeadersRow();

        for (const auto& [typeName, count] : componentSummary_)
        {
            ui::TableNextRow();
            ui::TableNextColumn();
            ui::Text("%s", typeName.c_str());
            ui::TableNextColumn();
            ui::Text("%d", count);
        }
        ui::EndTable();
    }
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
