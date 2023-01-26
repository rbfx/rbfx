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

#include "../../Foundation/SceneViewTab/SceneDragAndDropPrefab.h"

#include "../../Project/Project.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

void Foundation_SceneDragAndDropPrefab(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneDragAndDropPrefab>();
}

SceneDragAndDropPrefab::SceneDragAndDropPrefab(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
}

bool SceneDragAndDropPrefab::IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    if (!resourcePayload || resourcePayload->resources_.empty())
        return false;

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];
    return desc.HasObjectType<PrefabResource>() || desc.HasObjectType<Model>();
}

void SceneDragAndDropPrefab::BeginDragDrop(SceneViewPage& page, DragDropPayload* payload)
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    URHO3D_ASSERT(resourcePayload);

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];

    if (desc.HasObjectType<PrefabResource>())
        CreateNodeFromPrefab(page.scene_, desc);
    else if (desc.HasObjectType<Model>())
        CreateNodeFromModel(page.scene_, desc);
    else
        URHO3D_ASSERT(0);

    currentPage_ = &page;
}

void SceneDragAndDropPrefab::UpdateDragDrop(DragDropPayload* payload)
{
    if (!currentPage_ || !temporaryNode_)
        return;

    const HitResult hit = QueryHoveredGeometry(currentPage_->scene_, currentPage_->cameraRay_);

    temporaryNode_->SetPosition(hit.position_);

    if (ui::IsKeyDown(KEY_ALT))
        temporaryNode_->SetRotation(Quaternion{Vector3::UP, hit.normal_});
    else
        temporaryNode_->SetRotation(Quaternion::IDENTITY);
}

void SceneDragAndDropPrefab::CompleteDragDrop(DragDropPayload* payload)
{
    if (!temporaryNode_)
        return;

    if (currentPage_)
    {
        currentPage_->selection_.Clear();
        currentPage_->selection_.SetSelected(temporaryNode_, true);
    }

    owner_->PushAction(nodeActionBuilder_->Build(temporaryNode_));

    nodeActionBuilder_ = nullptr;
    temporaryNode_ = nullptr;
}

void SceneDragAndDropPrefab::CancelDragDrop()
{
    if (temporaryNode_)
        temporaryNode_->Remove();
    nodeActionBuilder_ = nullptr;
    temporaryNode_ = nullptr;
}

void SceneDragAndDropPrefab::CreateNodeFromPrefab(Scene* scene, const ResourceFileDescriptor& desc)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto prefabFile = cache->GetResource<PrefabResource>(desc.resourceName_);
    if (!prefabFile)
        return;

    const NodePrefab& nodePrefab = prefabFile->GetNodePrefab();

    nodeActionBuilder_ = ea::make_unique<CreateNodeActionBuilder>(scene, nodePrefab.GetEffectiveScopeHint(context_));
    temporaryNode_ = scene->CreateChild(GetFileName(desc.localName_));

    auto prefabReference = temporaryNode_->CreateComponent<PrefabReference>();
    prefabReference->SetPrefab(prefabFile);

    if (!nodePrefab.IsEmpty())
        nodePrefab.GetNode().Export(temporaryNode_);
}

void SceneDragAndDropPrefab::CreateNodeFromModel(Scene* scene, const ResourceFileDescriptor& desc)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto model = cache->GetResource<Model>(desc.resourceName_);
    auto defaultMaterial = cache->GetResource<Material>("Materials/DefaultWhite.xml");
    if (!model)
        return;

    nodeActionBuilder_ = ea::make_unique<CreateNodeActionBuilder>(scene, AttributeScopeHint::Attribute);
    temporaryNode_ = scene->CreateChild(GetFileName(desc.localName_));

    StaticModel* staticModel = nullptr;
    if (!model->GetSkeleton().GetNumBones())
    {
        staticModel = temporaryNode_->CreateComponent<StaticModel>();

        // TODO: Revisit this place, physical components may be harmful for bug models.
        // Also, ConvexHull vs TriangleMesh?
        /*auto rigidBody = temporaryNode_->CreateComponent<RigidBody>();
        auto collisionShape = temporaryNode_->CreateComponent<CollisionShape>();
        collisionShape->SetTriangleMesh(model);*/
    }
    else
    {
        staticModel = temporaryNode_->CreateComponent<AnimatedModel>();
    }

    staticModel->SetModel(model);
    staticModel->SetCastShadows(true);
    staticModel->ApplyMaterialList();

    if (defaultMaterial)
    {
        for (unsigned i = 0; i < staticModel->GetNumGeometries(); ++i)
        {
            if (!staticModel->GetMaterial(i))
                staticModel->SetMaterial(i, defaultMaterial);
        }
    }
}

SceneDragAndDropPrefab::HitResult SceneDragAndDropPrefab::QueryHoveredGeometry(Scene* scene, const Ray& cameraRay)
{
    const auto results = QueryGeometriesFromScene(scene, cameraRay);

    const auto isGeometryExceptSelf = [&](const RayQueryResult& result)
    {
        return result.node_ && result.node_->GetScene() != nullptr && result.node_ != temporaryNode_ && !result.node_->IsChildOf(temporaryNode_);
    };
    const auto iter = ea::find_if(results.begin(), results.end(), isGeometryExceptSelf);
    const RayQueryResult* queryResult = iter != results.end() ? &*iter : nullptr;

    HitResult hit;
    hit.hit_ = queryResult != nullptr;
    hit.distance_ = queryResult ? queryResult->distance_ : DefaultDistance;
    hit.position_ = cameraRay.origin_ + cameraRay.direction_ * hit.distance_;
    hit.normal_ = queryResult ? queryResult->normal_ : Vector3::UP;
    return hit;
}

#if 0
void SceneDragAndDropPrefab::AssignMaterialToDrawable(
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
