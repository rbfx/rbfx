//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"
#include "../Foundation/ActionViewTab.h"

#include "Urho3D/Actions/FiniteTimeAction.h"

#include <Urho3D/Actions/ActionManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/GraphNode.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

namespace
{
}

void Foundation_ActionViewTab(Context* context, Project* project)
{
    auto tab = MakeShared<ActionViewTab>(context);
    tab->Close();
    project->AddTab(tab);
}

ActionViewTab::ActionViewTab(Context* context)
    : BaseClassName(context, "Action", "23C3DC77-AA8F-4DF1-B410-9CB62384B34D",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
    , actionTypes_(ea::move(context_->GetSubsystem<ActionManager>()->GetObjectReflections().values()))
{
    ea::erase_if(actionTypes_, [](const SharedPtr<ObjectReflection>& a) { return !a->HasObjectFactory(); });
    ea::sort(actionTypes_.begin(), actionTypes_.end(),
        [](const SharedPtr<ObjectReflection>& l, const SharedPtr<ObjectReflection>& r)
        {
        if (l->GetCategory() < r->GetCategory())
            return true;
        if (l->GetCategory() > r->GetCategory())
            return false;
        return l->GetTypeName() < r->GetTypeName();
    });
}

ActionViewTab::~ActionViewTab()
{
}

bool ActionViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    auto name = desc.typeNames_.begin()->c_str();
    return desc.HasObjectType<ActionSet>();
}

void ActionViewTab::RenderTitle()
{
    BaseClassName::RenderTitle();

    auto* cache = GetSubsystem<ResourceCache>();

    static const StringVector allowedActionTypes{
        ActionSet::GetTypeNameStatic(),
    };

    StringHash actionType = actionSet_ ? actionSet_->GetType() : Texture2D::GetTypeStatic();
    ea::string actionName = actionSet_ ? actionSet_->GetName() : "";
    if (Widgets::EditResourceRef(actionType, actionName, &allowedActionTypes))
    {
        actionSet_ = cache->GetResource<ActionSet>(actionName);
    }
}

SharedPtr<GraphNode> ActionViewTab::CreateNewNodePopup() const
{
    for (auto& actionReflection : actionTypes_)
    {
        if (ImGui::MenuItem(actionReflection->GetTypeName().c_str()))
        {
            SharedPtr<Actions::FiniteTimeAction> action;
            action.DynamicCast(actionReflection->CreateObject());
            if (action)
            {
                const auto graph = MakeShared<Graph>(context_);
                const auto node = action->ToGraphNode(graph);
                return SharedPtr<GraphNode>{node};
            }
        }
    }
    return {};
}

void ActionViewTab::RenderContent()
{
    if (!actionSet_)
        return;

    BaseClassName::RenderContent();
}

void ActionViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    const auto cache = GetSubsystem<ResourceCache>();
    actionSet_ = cache->GetResource<ActionSet>(resourceName);

    Reset();
    if (!actionSet_)
        return;
    auto* graphView = GetGraphView();
    
    graphView->Populate(actionSet_->ToGraph());
}

void ActionViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    actionSet_.Reset();
}

void ActionViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void ActionViewTab::OnResourceSaved(const ea::string& resourceName)
{
    const auto graph = graph_.BuildGraph(context_);

    VectorBuffer buffer;
    buffer.SetName(resourceName);
    actionSet_->FromGraph(graph);
    actionSet_->Save(buffer);
    const auto sharedBuffer = ea::make_shared<ByteVector>(ea::move(buffer.GetBuffer()));

    const auto project = GetProject();

    project->SaveFileDelayed(actionSet_->GetAbsoluteFileName(), resourceName, sharedBuffer,
        [this](const ea::string& _, const ea::string& resourceName, bool& needReload) {});
}

void ActionViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
