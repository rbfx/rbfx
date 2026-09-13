// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/SceneViewTab/SceneDragAndDropMaterial.h"

#include "../../Project/Project.h"

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

namespace
{

const ea::string MaterialAttr{"Material"};

}

void Foundation_SceneDragAndDropMaterial(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneDragAndDropMaterial>();
}

SceneDragAndDropMaterial::SceneDragAndDropMaterial(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
}

bool SceneDragAndDropMaterial::IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    if (!resourcePayload || resourcePayload->resources_.empty())
        return false;

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];
    return desc.HasObjectType<Material>();
}

void SceneDragAndDropMaterial::BeginDragDrop(SceneViewPage& page, DragDropPayload* payload)
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    URHO3D_ASSERT(resourcePayload);

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];

    auto cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(desc.resourceName_);

    currentPage_ = &page;
}

void SceneDragAndDropMaterial::UpdateDragDrop(DragDropPayload* payload)
{
    if (!currentPage_ || !material_)
        return;

    const auto [drawable, materialIndex] = QueryHoveredGeometry(currentPage_->scene_, currentPage_->cameraRay_);
    if (temporaryAssignment_.drawable_ == drawable && temporaryAssignment_.materialIndex_ == materialIndex)
        return;

    if (temporaryAssignment_.drawable_)
        ClearAssignment();
    if (drawable)
        CreateAssignment(drawable, materialIndex);
}

void SceneDragAndDropMaterial::CompleteDragDrop(DragDropPayload* payload)
{
    if (!currentPage_ || !material_)
        return;

    if (temporaryAssignment_.drawable_)
    {
        Scene* scene = temporaryAssignment_.drawable_->GetScene();
        const auto components = {temporaryAssignment_.drawable_};
        owner_->PushAction<ChangeComponentAttributesAction>(scene, MaterialAttr, components,
            VariantVector{temporaryAssignment_.oldMaterial_},
            VariantVector{temporaryAssignment_.newMaterial_});
    }
    temporaryAssignment_ = {};
}

void SceneDragAndDropMaterial::CancelDragDrop()
{
    ClearAssignment();
    material_ = nullptr;
}

void SceneDragAndDropMaterial::ClearAssignment()
{
    if (temporaryAssignment_.drawable_)
    {
        temporaryAssignment_.drawable_->SetAttribute(MaterialAttr, temporaryAssignment_.oldMaterial_);
        temporaryAssignment_.drawable_ = nullptr;
    }
}

void SceneDragAndDropMaterial::CreateAssignment(Drawable* drawable, unsigned materialIndex)
{
    const ResourceRef materialRef{Material::GetTypeStatic(), material_->GetName()};

    temporaryAssignment_.drawable_ = drawable;
    temporaryAssignment_.materialIndex_ = materialIndex;
    temporaryAssignment_.oldMaterial_ = drawable->GetAttribute(MaterialAttr);

    if (temporaryAssignment_.oldMaterial_.GetType() == VAR_RESOURCEREFLIST)
    {
        ResourceRefList list = temporaryAssignment_.oldMaterial_.GetResourceRefList();
        if (materialIndex < list.names_.size())
            list.names_[materialIndex] = materialRef.name_;
        temporaryAssignment_.newMaterial_ = list;
    }
    else
    {
        temporaryAssignment_.newMaterial_ = materialRef;
    }

    drawable->SetAttribute(MaterialAttr, temporaryAssignment_.newMaterial_);
}

ea::pair<Drawable*, unsigned> SceneDragAndDropMaterial::QueryHoveredGeometry(Scene* scene, const Ray& cameraRay)
{
    const auto results = QueryGeometriesFromScene(scene, cameraRay);

    if (!results.empty())
    {
        Drawable* drawable = results[0].drawable_;
        const Variant& material = drawable->GetAttribute(MaterialAttr);

        if (material.GetType() == VAR_RESOURCEREF)
            return {drawable, 0u};
        else if (material.GetType() == VAR_RESOURCEREFLIST)
        {
            // TODO: This is a hack
            const unsigned numMaterials = material.GetResourceRefList().names_.size();
            const unsigned materialIndex = ea::min(results[0].subObject_, ea::max(numMaterials, 1u) - 1u);
            return {drawable, materialIndex};
        }
    }
    return {nullptr, 0};
}

} // namespace Urho3D
