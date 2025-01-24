// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/RenderPathInspector.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

ea::optional<StringHash> RenderCreatePass(Context* context)
{
    const auto& typesByCategory = context->GetObjectCategories();
    const auto iter = typesByCategory.find(Category_RenderPass);
    if (iter == typesByCategory.end())
        return ea::nullopt;

    const auto& types = iter->second;

    ea::optional<StringHash> result;
    for (const StringHash type : types)
    {
        const ObjectReflection* reflection = context->GetReflection(type);
        if (!reflection->HasObjectFactory())
            continue;
        if (ui::MenuItem(reflection->GetTypeName().c_str()))
            result = type;
    }
    return result;
}

} // namespace

void Foundation_RenderPathInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<RenderPathInspector>(inspectorTab->GetProject());
}

RenderPathInspector::RenderPathInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.SubscribeWithSender(this, &RenderPathInspector::OnProjectRequest);
}

void RenderPathInspector::OnProjectRequest(RefCounted* senderTab, ProjectRequest* request)
{
    auto inspectedTab = dynamic_cast<EditorTab*>(senderTab);
    if (!inspectedTab)
        return;

    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest)
        return;

    const auto& resources = inspectResourceRequest->GetResources();
    if (resources.size() != 1 || !resources[0].HasObjectType<RenderPath>())
        return;

    request->QueueProcessCallback([=]()
    {
        if (resourceName_ != resources[0].resourceName_)
        {
            resourceName_ = resources[0].resourceName_;
            InspectObjects();
        }
        OnActivated(this);
    });
}

void RenderPathInspector::OnResourceReloaded()
{
    if (suppressReloadCallback_)
        return;

    inspectorWidgets_.clear();
    passes_.clear();
}

void RenderPathInspector::InspectObjects()
{
    auto cache = GetSubsystem<ResourceCache>();
    resource_ = cache->GetResource<RenderPath>(resourceName_);

    UnsubscribeFromEvent(E_RELOADFINISHED);
    if (resource_)
        SubscribeToEvent(resource_, E_RELOADFINISHED, &RenderPathInspector::OnResourceReloaded);

    OnResourceReloaded();
}

bool RenderPathInspector::HasPendingChanges() const
{
    return pendingAction_ && !pendingAction_->IsComplete();
}

void RenderPathInspector::BeginChange()
{
    if (!pendingAction_ || pendingAction_->IsComplete())
    {
        pendingAction_ = MakeShared<ModifyResourceAction>(project_);
        pendingAction_->AddResource(resource_);
        pendingAction_->SaveOnComplete();

        auto undoManager = project_->GetUndoManager();
        undoManager->PushAction(pendingAction_);
    }
}

void RenderPathInspector::BeginEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
    BeginChange();
}

void RenderPathInspector::EndEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
}

void RenderPathInspector::EnsureInitialized()
{
    if (!resource_)
        return;

    const auto passes = resource_->GetPasses();
    const ea::vector<WeakPtr<RenderPass>> newPasses{passes.begin(), passes.end()};
    if (passes_ == newPasses)
        return;

    passes_ = newPasses;
    inspectorWidgets_.clear();
    for (RenderPass* pass : passes_)
    {
        const WeakPtr<RenderPass> weakPass{pass};
        const auto widget = MakeShared<SerializableInspectorWidget>(context_, WeakSerializableVector{weakPass});
        widget->OnEditAttributeBegin.Subscribe(this, &RenderPathInspector::BeginEditAttribute);
        widget->OnEditAttributeEnd.Subscribe(this, &RenderPathInspector::EndEditAttribute);
        inspectorWidgets_.push_back(widget);
    }
}

void RenderPathInspector::RenderContent()
{
    if (!resource_)
        return;

    EnsureInitialized();

    auto assetManager = project_->GetAssetManager();

    ui::Text("%s", resourceName_.c_str());

    pendingRemoves_.clear();
    pendingAdds_.clear();
    pendingReorders_.clear();

    for (unsigned index = 0; index < inspectorWidgets_.size(); ++index)
        RenderInspector(index, inspectorWidgets_[index]);
    RenderAddPass();

    for (const auto& [pass, newIndex] : pendingReorders_)
    {
        if (!pass)
            continue;

        BeginChange();
        resource_->ReorderPass(pass, newIndex);
        OnResourceReloaded();
    }

    for (RenderPass* pass : pendingRemoves_)
    {
        if (!pass)
            continue;

        BeginChange();
        resource_->RemovePass(pass);
        OnResourceReloaded();
    }

    for (StringHash type : pendingAdds_)
    {
        const auto pass = DynamicCast<RenderPass>(context_->CreateObject(type));
        if (!pass)
        {
            URHO3D_LOGERROR("Failed to create RenderPass");
            continue;
        }

        BeginChange();
        resource_->AddPass(pass);
        OnResourceReloaded();
    }

    if (HasPendingChanges())
    {
        suppressReloadCallback_ = true;
        resource_->SendEvent(E_RELOADFINISHED);
        suppressReloadCallback_ = false;
    }
}

void RenderPathInspector::RenderInspector(unsigned index, SerializableInspectorWidget* inspector)
{
    const IdScopeGuard guard{inspector};

    if (ui::Button(ICON_FA_TRASH_CAN "##RemovePass"))
    {
        for (Serializable* serializable : inspector->GetObjects())
        {
            if (const WeakPtr<RenderPass> pass{dynamic_cast<RenderPass*>(serializable)})
                pendingRemoves_.push_back(pass);
        }
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove this pass");
    ui::SameLine();

    ui::BeginDisabled(index == 0);
    if (ui::Button(ICON_FA_ARROW_UP "##MovePassUp"))
    {
        for (Serializable* serializable : inspector->GetObjects())
        {
            if (const WeakPtr<RenderPass> pass{dynamic_cast<RenderPass*>(serializable)})
                pendingReorders_.emplace_back(pass, index - 1);
        }
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Move this pass up");
    ui::EndDisabled();
    ui::SameLine();

    ui::BeginDisabled(index + 1 == inspectorWidgets_.size());
    if (ui::Button(ICON_FA_ARROW_DOWN "##MovePassDown"))
    {
        for (Serializable* serializable : inspector->GetObjects())
        {
            if (const WeakPtr<RenderPass> pass{dynamic_cast<RenderPass*>(serializable)})
                pendingReorders_.emplace_back(pass, index + 1);
        }
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Move this pass down");
    ui::EndDisabled();
    ui::SameLine();

    const ea::string& title = inspector->GetObjects()[0]->Cast<RenderPass>()->GetPassName();
    if (ui::CollapsingHeader(title.c_str()))
    {
        inspector->RenderContent();
    }
}

void RenderPathInspector::RenderAddPass()
{
    if (ui::Button(ICON_FA_SQUARE_PLUS " Add Pass"))
        ui::OpenPopup("##AddPass");
    if (ui::BeginPopup("##AddPass"))
    {
        if (const auto passType = RenderCreatePass(context_))
        {
            pendingAdds_.push_back(*passType);
            ui::CloseCurrentPopup();
        }
        ui::EndPopup();
    }
}

void RenderPathInspector::RenderContextMenuItems()
{
}

void RenderPathInspector::RenderMenu()
{
}

void RenderPathInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

} // namespace Urho3D
