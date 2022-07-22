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

#include "../../Project/CreateComponentMenu.h"

#include <Urho3D/Container/TransformedSpan.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

template <class T>
struct WeakToRawStaticCaster
{
    template <class U>
    T operator() (const U& x) const { return static_cast<T>(x.Get()); }
};

template <class T>
auto CastVectorTo(const ea::vector<WeakPtr<Serializable>>& objects)
{
    ea::span<const WeakPtr<Serializable>> objectsSpan{objects};
    return TransformedSpan<const WeakPtr<Serializable>, T, WeakToRawStaticCaster<T>>(objectsSpan);
}

}

void Foundation_NodeComponentInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<NodeComponentInspector>(inspectorTab->GetProject());
}

NodeComponentInspector::NodeComponentInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.SubscribeWithSender(this, &NodeComponentInspector::OnProjectRequest);
}

void NodeComponentInspector::OnProjectRequest(RefCounted* senderTab, ProjectRequest* request)
{
    auto inspectedTab = dynamic_cast<EditorTab*>(senderTab);
    if (!inspectedTab)
        return;

    auto inspectNodeComponentRequest = dynamic_cast<InspectNodeComponentRequest*>(request);
    if (!inspectNodeComponentRequest || inspectNodeComponentRequest->IsEmpty())
        return;

    Scene* commonScene = inspectNodeComponentRequest->GetCommonScene();
    if (!commonScene)
        return;

    request->QueueProcessCallback([=]()
    {
        if (nodes_ != inspectNodeComponentRequest->GetNodes()
            || components_ != inspectNodeComponentRequest->GetComponents()
            || inspectedTab_ != inspectedTab)
        {
            nodes_ = inspectNodeComponentRequest->GetNodes();
            components_ = inspectNodeComponentRequest->GetComponents();
            inspectedTab_ = inspectedTab;
            scene_ = commonScene;
            InspectObjects();
        }
        OnActivated(this);
    });
}

NodeComponentInspector::NodeVector NodeComponentInspector::CollectNodes() const
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

NodeComponentInspector::SerializableVector NodeComponentInspector::CollectComponents() const
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

        nodeWidget_->OnEditNodeAttributeBegin.Subscribe(this, &NodeComponentInspector::BeginEditNodeAttribute);
        nodeWidget_->OnEditNodeAttributeEnd.Subscribe(this, &NodeComponentInspector::EndEditNodeAttribute);
        nodeWidget_->OnEditComponentAttributeBegin.Subscribe(this, &NodeComponentInspector::BeginEditComponentAttribute);
        nodeWidget_->OnEditComponentAttributeEnd.Subscribe(this, &NodeComponentInspector::EndEditComponentAttribute);
        nodeWidget_->OnComponentRemoved.Subscribe(this, &NodeComponentInspector::RemoveComponent);
    }
    else if (const auto components = CollectComponents(); !components.empty())
    {
        nodeWidget_ = nullptr;
        componentWidget_ = MakeShared<SerializableInspectorWidget>(context_, components);
        componentSummary_.clear();

        componentWidget_->OnEditAttributeBegin.Subscribe(this, &NodeComponentInspector::BeginEditComponentAttribute);
        componentWidget_->OnEditAttributeEnd.Subscribe(this, &NodeComponentInspector::EndEditComponentAttribute);
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

void NodeComponentInspector::BeginEditNodeAttribute(const SerializableVector& objects, const AttributeInfo* attribute)
{
    if (objects.empty())
        return;

    const auto nodes = CastVectorTo<Node*>(objects);
    oldValues_.clear();
    for (Node* node : nodes)
        oldValues_.push_back(node->GetAttribute(attribute->name_));
}

void NodeComponentInspector::EndEditNodeAttribute(const SerializableVector& objects, const AttributeInfo* attribute)
{
    if (objects.empty())
        return;

    const auto nodes = CastVectorTo<Node*>(objects);
    newValues_.clear();
    for (Node* node : nodes)
        newValues_.push_back(node->GetAttribute(attribute->name_));

    inspectedTab_->PushAction<ChangeNodeAttributesAction>(scene_, attribute->name_, nodes, oldValues_, newValues_);
}

void NodeComponentInspector::BeginEditComponentAttribute(const SerializableVector& objects, const AttributeInfo* attribute)
{
    if (objects.empty())
        return;

    const auto components = CastVectorTo<Component*>(objects);
    oldValues_.clear();
    for (Component* component : components)
        oldValues_.push_back(component->GetAttribute(attribute->name_));
}

void NodeComponentInspector::EndEditComponentAttribute(const SerializableVector& objects, const AttributeInfo* attribute)
{
    if (objects.empty())
        return;

    const auto components = CastVectorTo<Component*>(objects);
    newValues_.clear();
    for (Component* component : components)
        newValues_.push_back(component->GetAttribute(attribute->name_));

    inspectedTab_->PushAction<ChangeComponentAttributesAction>(scene_, attribute->name_, components, oldValues_, newValues_);
}

void NodeComponentInspector::AddComponentToNodes(StringHash componentType)
{
    if (!nodeWidget_)
        return;

    for (Node* node : nodeWidget_->GetNodes())
    {
        if (auto component = node->CreateComponent(componentType))
            inspectedTab_->PushAction<CreateRemoveComponentAction>(component, false);
    }
}

void NodeComponentInspector::RemoveComponent(Component* component)
{
    inspectedTab_->PushAction<CreateRemoveComponentAction>(component, true);
}

void NodeComponentInspector::RenderContent()
{
    if (nodeWidget_)
    {
        nodeWidget_->RenderTitle();
        ui::Separator();
        nodeWidget_->RenderContent();
        ui::Separator();
        RenderAddComponent();
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

void NodeComponentInspector::RenderAddComponent()
{
    if (ui::Button("Add Component"))
        ui::OpenPopup("##AddComponent");
    if (ui::BeginPopup("##AddComponent"))
    {
        if (const auto componentType = RenderCreateComponentMenu(context_))
        {
            AddComponentToNodes(*componentType);
            ui::CloseCurrentPopup();
        }
        ui::EndPopup();
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
