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

#include "../Precompiled.h"

#include "../SystemUI/NodeInspectorWidget.h"

#include "../Core/IteratorRange.h"
#include "../Scene/Component.h"
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/numeric.h>

namespace Urho3D
{

NodeInspectorWidget::NodeInspectorWidget(Context* context, const NodeVector& nodes)
    : Object(context)
    , nodes_(nodes)
    , nodeInspector_(MakeShared<SerializableInspectorWidget>(context_, WeakSerializableVector{nodes.begin(), nodes.end()}))
{
    URHO3D_ASSERT(!nodes_.empty());
    nodeInspector_->OnEditAttributeBegin.Subscribe(this, ea::ref(OnEditNodeAttributeBegin));
    nodeInspector_->OnEditAttributeEnd.Subscribe(this, ea::ref(OnEditNodeAttributeEnd));
    nodeInspector_->OnActionBegin.Subscribe(this, ea::ref(OnActionBegin));
    nodeInspector_->OnActionEnd.Subscribe(this, ea::ref(OnActionEnd));
}

NodeInspectorWidget::~NodeInspectorWidget()
{
}

void NodeInspectorWidget::RenderTitle()
{
    nodeInspector_->RenderTitle();
}

void NodeInspectorWidget::RenderContent()
{
    ea::erase_if(nodes_, [] (Node* node) { return node == nullptr; });
    if (nodes_.empty())
        return;

    pendingRemoveComponents_.clear();

    const auto allComponents = GetAllComponents();
    if (components_ != allComponents)
    {
        components_ = allComponents;
        componentInspectors_.clear();

        const auto sharedComponents = GetSharedComponents();
        for (const auto& components : sharedComponents)
        {
            componentInspectors_.emplace_back(MakeShared<SerializableInspectorWidget>(
                context_, WeakSerializableVector{components.begin(), components.end()}));
        }

        const unsigned numSharedComponents = ea::accumulate(sharedComponents.begin(), sharedComponents.end(), 0u,
            [](unsigned sum, const ea::vector<WeakPtr<Component>>& components) { return sum + components.size(); });
        numSkippedComponents_ = components_.size() - numSharedComponents;

        for (SerializableInspectorWidget* inspector : componentInspectors_)
        {
            inspector->OnEditAttributeBegin.Subscribe(this, ea::ref(OnEditComponentAttributeBegin));
            inspector->OnEditAttributeEnd.Subscribe(this, ea::ref(OnEditComponentAttributeEnd));
            inspector->OnActionBegin.Subscribe(this, ea::ref(OnActionBegin));
            inspector->OnActionEnd.Subscribe(this, ea::ref(OnActionEnd));
        }
    }

    nodeInspector_->RenderContent();

    for (SerializableInspectorWidget* componentInspector : componentInspectors_)
    {
        Serializable* firstComponent = componentInspector->GetObjects().front();
        if (!firstComponent)
            continue;

        const IdScopeGuard guard{static_cast<void*>(firstComponent)};

        if (ui::Button(ICON_FA_TRASH_CAN "##RemoveComponent"))
        {
            for (Serializable* serializable : componentInspector->GetObjects())
            {
                if (const WeakPtr<Component> component{dynamic_cast<Component*>(serializable)})
                    pendingRemoveComponents_.push_back(component);
            }
        }
        if (ui::IsItemHovered())
            ui::SetTooltip("Remove this component from all selected nodes");
        ui::SameLine();

        if (ui::CollapsingHeader(componentInspector->GetTitle().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            componentInspector->RenderContent();
        }
    }

    if (numSkippedComponents_ > 0)
    {
        ui::Separator();
        ui::Text("%u other components skipped", numSkippedComponents_);
    }

    for (Component* component : pendingRemoveComponents_)
    {
        if (WeakPtr<Component> weakComponent{component})
        {
            OnComponentRemoved(this, component);
            if (weakComponent)
                weakComponent->Remove();
        }
    }
}

NodeInspectorWidget::NodeComponentVector NodeInspectorWidget::GetAllComponents() const
{
    NodeComponentVector components;
    for (const auto& node : nodes_)
    {
        for (const auto& component : node->GetComponents())
            components.emplace_back(node, component);
    }
    return components;
}

NodeInspectorWidget::ComponentVectorsByType NodeInspectorWidget::GetSharedComponents() const
{
    NodeInspectorWidget::ComponentVectorsByType result;

    // Fill result from the first node as-is
    const auto& components = nodes_[0]->GetComponents();
    for (Component* component : components)
        result.emplace_back().emplace_back(component);

    // Prune all components missing in any of the other nodes
    for (Node* node : MakeIteratorRange(nodes_.begin() + 1, nodes_.end()))
    {
        const auto& components = node->GetComponents();

        // If at least one node has no components, result is empty
        if (components.empty())
        {
            result.clear();
            break;
        }

        unsigned index = 0;
        for (auto& sharedComponents : result)
        {
            const StringHash sharedType = sharedComponents.front()->GetType();

            if (index < components.size())
            {
                // Try to find component with the same type in the current node
                const auto matchingComponentIter = ea::find_if(components.begin() + index, components.end(),
                    [&](Component* component) { return component->GetType() == sharedType; });
                if (matchingComponentIter != components.end())
                {
                    sharedComponents.emplace_back(*matchingComponentIter);
                    index = (matchingComponentIter - components.begin()) + 1;
                    continue;
                }
            }

            // Otherwise, prune the shared entry
            sharedComponents.clear();
        }

        ea::erase_if(result, [](const ea::vector<WeakPtr<Component>>& components) { return components.empty(); });
    }
    return result;
}

}
