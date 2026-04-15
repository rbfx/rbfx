// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/SceneViewTab/SceneDragAndDropAnimation.h"

#include "../../Project/Project.h"

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

void Foundation_SceneDragAndDropAnimation(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneDragAndDropAnimation>();
}

SceneDragAndDropAnimation::SceneDragAndDropAnimation(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
}

bool SceneDragAndDropAnimation::IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    if (!resourcePayload || resourcePayload->resources_.empty())
        return false;

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];
    return desc.HasObjectType<Animation>();
}

void SceneDragAndDropAnimation::BeginDragDrop(SceneViewPage& page, DragDropPayload* payload)
{
    const auto resourcePayload = dynamic_cast<ResourceDragDropPayload*>(payload);
    URHO3D_ASSERT(resourcePayload);

    const ResourceFileDescriptor& desc = resourcePayload->resources_[0];

    auto cache = GetSubsystem<ResourceCache>();
    animation_ = cache->GetResource<Animation>(desc.resourceName_);

    currentPage_ = &page;
}

void SceneDragAndDropAnimation::UpdateDragDrop(DragDropPayload* payload)
{
    if (!currentPage_ || !animation_)
        return;

    ea::tie(hoveredController_, hoveredDrawable_) =
        QueryHoveredController(currentPage_->scene_, currentPage_->cameraRay_);

    if (hoveredDrawable_)
    {
        if (auto debugRenderer = hoveredDrawable_->GetScene()->GetComponent<DebugRenderer>())
            hoveredDrawable_->DrawDebugGeometry(debugRenderer, false);
    }
}

void SceneDragAndDropAnimation::CompleteDragDrop(DragDropPayload* payload)
{
    if (!currentPage_ || !animation_ || !hoveredController_)
        return;

    const auto oldValue = hoveredController_->GetAnimationsAttr();
    hoveredController_->SetAnimationsAttr({});
    hoveredController_->AddAnimation(AnimationParameters{animation_}.Looped());
    const auto newValue = hoveredController_->GetAnimationsAttr();

    Scene* scene = hoveredController_->GetScene();
    const auto components = {hoveredController_};
    owner_->PushAction<ChangeComponentAttributesAction>(
        scene, "Animations", components, VariantVector{Variant{oldValue}}, VariantVector{Variant{newValue}});
}

void SceneDragAndDropAnimation::CancelDragDrop()
{
    animation_ = nullptr;
}

ea::pair<AnimationController*, Drawable*> SceneDragAndDropAnimation::QueryHoveredController(
    Scene* scene, const Ray& cameraRay)
{
    const auto results = QueryGeometriesFromScene(scene, cameraRay);
    const auto drawable = !results.empty() ? results[0].drawable_ : nullptr;
    const auto node = drawable ? drawable->GetNode() : nullptr;
    const auto controller = node ? node->GetComponent<AnimationController>() : nullptr;
    return controller && drawable ? ea::make_pair(controller, drawable) : ea::make_pair(nullptr, nullptr);
}

} // namespace Urho3D
