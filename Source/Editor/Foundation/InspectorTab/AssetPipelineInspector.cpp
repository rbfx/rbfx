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

#include "../../Foundation/InspectorTab/AssetPipelineInspector.h"

#include "../../Project/AssetManager.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

ea::optional<StringHash> RenderCreateTransformer(Context* context)
{
    const auto& typesByCategory = context->GetObjectCategories();
    const auto iter = typesByCategory.find(Category_Transformer);
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

}

void Foundation_AssetPipelineInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<AssetPipelineInspector>(inspectorTab->GetProject());
}

AssetPipelineInspector::AssetPipelineInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.SubscribeWithSender(this, &AssetPipelineInspector::OnProjectRequest);
}

void AssetPipelineInspector::OnProjectRequest(RefCounted* senderTab, ProjectRequest* request)
{
    auto inspectedTab = dynamic_cast<EditorTab*>(senderTab);
    if (!inspectedTab)
        return;

    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest)
        return;

    const auto& resources = inspectResourceRequest->GetResources();
    if (resources.size() != 1 || !resources[0].HasObjectType<AssetPipeline>())
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

void AssetPipelineInspector::OnResourceReloaded()
{
    inspectorWidgets_.clear();
    transformers_.clear();
}

void AssetPipelineInspector::InspectObjects()
{
    auto cache = GetSubsystem<ResourceCache>();
    resource_ = cache->GetResource<AssetPipeline>(resourceName_);

    UnsubscribeFromEvent(E_RELOADFINISHED);
    if (resource_)
        SubscribeToEvent(resource_, E_RELOADFINISHED, &AssetPipelineInspector::OnResourceReloaded);

    OnResourceReloaded();
}

bool AssetPipelineInspector::HasPendingChanges() const
{
    return pendingAction_ && !pendingAction_->IsComplete();
}

void AssetPipelineInspector::BeginChange()
{
    if (!pendingAction_ || pendingAction_->IsComplete())
    {
        pendingAction_ = MakeShared<ModifyResourceAction>(project_);
        pendingAction_->AddResource(resource_);
        pendingAction_->DisableAutoComplete();
        pendingAction_->SaveOnComplete();

        auto undoManager = project_->GetUndoManager();
        undoManager->PushAction(pendingAction_);
    }
}

void AssetPipelineInspector::Apply()
{
    if (HasPendingChanges())
    {
        pendingAction_->Complete(true);
    }
}

void AssetPipelineInspector::Discard()
{
    if (HasPendingChanges())
    {
        auto undoManager = project_->GetUndoManager();
        undoManager->Undo();
    }
}

void AssetPipelineInspector::BeginEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
    BeginChange();
}

void AssetPipelineInspector::EndEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
}

void AssetPipelineInspector::EnsureInitialized()
{
    if (!resource_)
        return;

    const auto transformers = resource_->GetTransformers();
    const ea::vector<WeakPtr<AssetTransformer>> newTransformers{transformers.begin(), transformers.end()};
    if (transformers_ == newTransformers)
        return;

    transformers_ = newTransformers;
    inspectorWidgets_.clear();
    for (AssetTransformer* transformer : transformers_)
    {
        const WeakPtr<AssetTransformer> weakTransformer{transformer};
        const auto widget = MakeShared<SerializableInspectorWidget>(context_, WeakSerializableVector{weakTransformer});
        widget->OnEditAttributeBegin.Subscribe(this, &AssetPipelineInspector::BeginEditAttribute);
        widget->OnEditAttributeEnd.Subscribe(this, &AssetPipelineInspector::EndEditAttribute);
        inspectorWidgets_.push_back(widget);
    }
}

void AssetPipelineInspector::RenderContent()
{
    if (!resource_)
        return;

    EnsureInitialized();

    auto assetManager = project_->GetAssetManager();

    ui::Text("%s", resourceName_.c_str());

    if (ui::Button(ICON_FA_ARROWS_ROTATE " Update Assets"))
        assetManager->MarkCacheDirty(GetPath(resource_->GetName()));
    if (ui::IsItemHovered())
        ui::SetTooltip("Re-import all assets potentially affected by this pipeline");

    ui::SameLine();
    ui::Text(assetManager->IsProcessing() ? ICON_FA_TRIANGLE_EXCLAMATION " Assets are cooking now" : "");

    pendingRemoves_.clear();
    pendingAdds_.clear();

    for (SerializableInspectorWidget* inspector : inspectorWidgets_)
        RenderInspector(inspector);
    RenderAddTransformer();

    for (AssetTransformer* transformer : pendingRemoves_)
    {
        if (!transformer)
            continue;

        BeginChange();
        resource_->RemoveTransformer(transformer);
        OnResourceReloaded();
    }

    for (StringHash type : pendingAdds_)
    {
        const auto transformer = DynamicCast<AssetTransformer>(context_->CreateObject(type));
        if (!transformer)
        {
            URHO3D_LOGERROR("Failed to create AssetTransformer");
            continue;
        }

        BeginChange();
        resource_->AddTransformer(transformer);
        OnResourceReloaded();
    }

    RenderFinalButtons();
}

void AssetPipelineInspector::RenderInspector(SerializableInspectorWidget* inspector)
{
    const IdScopeGuard guard{inspector};

    if (ui::Button(ICON_FA_TRASH_CAN "##RemoveTransformer"))
    {
        for (Serializable* serializable : inspector->GetObjects())
        {
            if (const WeakPtr<AssetTransformer> transformer{dynamic_cast<AssetTransformer*>(serializable)})
                pendingRemoves_.push_back(transformer);
        }
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove this transformer");
    ui::SameLine();

    if (ui::CollapsingHeader(inspector->GetTitle().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        inspector->RenderContent();
    }
}

void AssetPipelineInspector::RenderAddTransformer()
{
    if (ui::Button(ICON_FA_SQUARE_PLUS " Add Transformer"))
        ui::OpenPopup("##AddTransformer");
    if (ui::BeginPopup("##AddTransformer"))
    {
        if (const auto transformerType = RenderCreateTransformer(context_))
        {
            pendingAdds_.push_back(*transformerType);
            ui::CloseCurrentPopup();
        }
        ui::EndPopup();
    }
}

void AssetPipelineInspector::RenderFinalButtons()
{
    const bool hasChanges = HasPendingChanges();

    ui::BeginDisabled(!hasChanges);
    if (ui::Button(ICON_FA_SQUARE_CHECK " Apply"))
        Apply();
    ui::SameLine();
    if (ui::Button(ICON_FA_SQUARE_XMARK " Discard"))
        Discard();
    ui::EndDisabled();

    if (hasChanges)
        ui::Text(ICON_FA_TRIANGLE_EXCLAMATION " Some changes are not applied yet!");
    else
        ui::NewLine();
}

void AssetPipelineInspector::RenderContextMenuItems()
{
}

void AssetPipelineInspector::RenderMenu()
{
}

void AssetPipelineInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
