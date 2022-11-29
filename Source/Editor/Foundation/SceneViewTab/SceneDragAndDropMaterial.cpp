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

#include "../../Foundation/SceneViewTab/SceneDragAndDropMaterial.h"

#include "../../Project/Project.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/PrefabReference.h>
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

    const auto [drawable, subObject] = QueryHoveredGeometry(currentPage_->scene_, currentPage_->cameraRay_);
    if (temporaryAssignment_.drawable_ == drawable && temporaryAssignment_.subObject_ == subObject)
        return;

    if (temporaryAssignment_.drawable_)
        ClearAssignment();
    if (drawable)
        CreateAssignment(drawable, subObject);
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

void SceneDragAndDropMaterial::CreateAssignment(Drawable* drawable, unsigned subObject)
{
    const ResourceRef materialRef{Material::GetTypeStatic(), material_->GetName()};

    temporaryAssignment_.drawable_ = drawable;
    temporaryAssignment_.subObject_ = subObject;
    temporaryAssignment_.oldMaterial_ = drawable->GetAttribute(MaterialAttr);

    if (temporaryAssignment_.oldMaterial_.GetType() == VAR_RESOURCEREFLIST)
    {
        ResourceRefList list = temporaryAssignment_.oldMaterial_.GetResourceRefList();
        if (subObject < list.names_.size())
            list.names_[subObject] = materialRef.name_;
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
        if (material.GetType() == VAR_RESOURCEREF || material.GetType() == VAR_RESOURCEREFLIST)
            return {drawable, results[0].subObject_};
    }
    return {nullptr, 0};
}

#if 0
void SceneDragAndDropMaterial::AssignMaterialToDrawable(
    SceneViewPage& scenePage, const ResourceFileDescriptor& desc, const RayQueryResult& result) const
{
    ResourceCache* cache = owner_->GetContext()->GetSubsystem<ResourceCache>();
    if (Material* material = cache->GetResource<Material>(desc.resourceName_))
    {
        if (StaticModel* model = result.drawable_->Cast<StaticModel>())
        {
            StringVector sv;
            for (int i = 0; i < model->GetNumGeometries(); i++)
                sv.push_back(desc.resourceName_);

            const ResourceRefList materialsAttr(Material::GetTypeStatic(), sv);
            model->SetMaterialsAttr(materialsAttr);
        }
    }
}
#endif

} // namespace Urho3D
