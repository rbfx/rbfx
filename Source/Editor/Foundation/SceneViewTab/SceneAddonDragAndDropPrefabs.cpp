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

#include "../../Foundation/SceneViewTab/SceneAddonDragAndDropPrefabs.h"

#include "../../Project/Project.h"
#include "../../Project/ResourceEditorTab.h"
#include "../../Editor/Foundation/InspectorTab/NodeComponentInspector.h"

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Utility/SceneSelection.h>
#include <Urho3D/Utility/PackedSceneData.h>
#include <nativefiledialog/nfd.h>

namespace Urho3D
{

void Foundation_SceneDragAndDropPrefabs(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneDragAndDropPrefabs>();
}

SceneDragAndDropPrefabs::SceneDragAndDropPrefabs(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
    owner->OnEditMenuRequest.Subscribe(this, &SceneDragAndDropPrefabs::OnEditMenuRequest);
}

void SceneDragAndDropPrefabs::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    Scene* scene = scenePage.scene_;
    Camera* camera = scenePage.renderer_->GetCamera();

    if (!mouseConsumed)
    {
        if (ui::BeginDragDropTarget() && ui::IsMouseReleased(MOUSEB_LEFT))
        {
            mouseConsumed = true;
            if (ea::optional<RayQueryResult> result = QuerySingleRayQueryResult(scene, camera))
            {
                DragAndDropPrefabsToSceneView(scenePage, result.value());
            }
            ui::EndDragDropTarget();
        }
    }
}

void SceneDragAndDropPrefabs::DragAndDropPrefabsToSceneView(SceneViewPage& scenePage, RayQueryResult& result)
{
    if (auto payload = static_cast<ResourceDragDropPayload*>(DragDropPayload::Get()))
    {
        for (const ResourceFileDescriptor& desc : payload->resources_)
        {
            if (desc.HasObjectType<Scene>())
            {
                ResourceCache* cache = owner_->GetContext()->GetSubsystem<ResourceCache>();
                if (XMLFile* prefabFile = cache->GetResource<XMLFile>(desc.resourceName_))
                {
                    auto prefabNode = scenePage.scene_->CreateChild(GetFileName(desc.localName_), CreateMode::LOCAL);

                    SharedPtr<PrefabReference> prefabRef{
                        prefabNode->CreateComponent<PrefabReference>(CreateMode::LOCAL)};
                    prefabRef->SetPrefab(prefabFile);
                    prefabNode->SetPosition(result.position_);

                    if (ui::IsKeyDown(KEY_ALT))
                    {
                        Quaternion q;
                        q.FromLookRotation(result.normal_);
                        prefabNode->SetRotation(q * Quaternion(90, 0, 0));
                    }

                    const bool toggle = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
                    const bool append = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);
                    SelectNode(scenePage.selection_, prefabNode, toggle, append);

                    owner_->PushAction<CreateRemoveNodeAction>(prefabNode, false);
                }
            }
            else if (desc.HasObjectType<Model>())
            {
                CreateNodeWithModel(scenePage, desc, result);
            }
            else if (desc.HasObjectType<Material>())
            {
                AssignMaterialToDrawable(scenePage, desc, result);
            }
        }
    }
}

void SceneDragAndDropPrefabs::CreatePrefabFile(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        if (SceneViewPage* page = owner_->GetPage(activeNode->GetScene()))
        {
            eastl::string prefabFileName = activeNode->GetName();

            if (prefabFileName.length() < 1)
                prefabFileName = Format("PrefabNodeID_{}", activeNode->GetID());

            Vector3 oldPrefabPosition = activeNode->GetWorldPosition();
            activeNode->SetWorldPosition(Vector3::ZERO);
            auto xmlFile = MakeShared<XMLFile>(context_);
            {
                XMLElement xmlRoot = xmlFile->CreateRoot("scene");
                xmlRoot.SetAttribute("id", "1");
                XMLElement xmlNode = xmlRoot.CreateChild("node");
                activeNode->SaveXML(xmlNode);
            }

            ea::optional<ea::string> selectedPath = SelectPrefabPath();
            if (selectedPath)
                xmlFile->SaveFile(selectedPath.value() + prefabFileName + ".xml");

            activeNode->SetWorldPosition(oldPrefabPosition);
        }
    }
}

ea::optional<ea::string> SceneDragAndDropPrefabs::SelectPrefabPath()
{
    nfdchar_t* outFilePath = nullptr;

    if (NFD_PickFolder("", &outFilePath) == NFD_OKAY)
    {
        ea::string resultPath(outFilePath);
        NFD_FreePath(outFilePath);
        return ea::string(resultPath + "\\");
    }

    return ea::nullopt;
}
void SceneDragAndDropPrefabs::OnEditMenuRequest(SceneSelection& selection, const ea::string_view& editMenuItemName)
{
    if (editMenuItemName == "Create Prefab")
        CreatePrefabFile(selection);
}

ea::optional<RayQueryResult> SceneDragAndDropPrefabs::QuerySingleRayQueryResult(Scene* scene, Camera* camera) const
{
    ImGuiIO& io = ui::GetIO();

    const ImRect viewportRect{ui::GetItemRectMin(), ui::GetItemRectMax()};
    const auto pos = ToVector2((io.MousePos - viewportRect.Min) / viewportRect.GetSize());
    const Ray cameraRay = camera->GetScreenRay(pos.x_, pos.y_);

    ea::vector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
    scene->GetComponent<Octree>()->Raycast(query);

    if (results.size())
        if (results[0].drawable_->GetScene() != nullptr)
            return results[0];

    return ea::nullopt;
}

void SceneDragAndDropPrefabs::SelectNode(SceneSelection& selection, Node* node, bool toggle, bool append) const
{
    selection.ConvertToNodes();

    if (toggle)
        selection.SetSelected(node, !selection.IsSelected(node));
    else if (append)
        selection.SetSelected(node, true);
    else
    {
        selection.Clear();
        selection.SetSelected(node, true);
    }
}
void SceneDragAndDropPrefabs::CreateNodeWithModel(
    SceneViewPage& scenePage, const ResourceFileDescriptor& desc, const RayQueryResult& result) const
{
    const ea::string defaultResourceMaterial("Materials/Constant/MattWhite.xml");
    ResourceCache* cache = owner_->GetContext()->GetSubsystem<ResourceCache>();
    if (Model* model = cache->GetResource<Model>(desc.resourceName_))
    {
        auto node = scenePage.scene_->CreateChild(GetFileName(desc.localName_), CreateMode::LOCAL);
        SharedPtr<StaticModel> spawnedModel;
        SharedPtr<CollisionShape> shape(node->CreateComponent<CollisionShape>(CreateMode::LOCAL));
        if (!model->GetSkeleton().GetNumBones())
        {
            spawnedModel = SharedPtr<StaticModel>(node->CreateComponent<StaticModel>(CreateMode::LOCAL));
            shape->SetShapeType(ShapeType::SHAPE_TRIANGLEMESH);
            shape->SetModel(model);
        }
        else
        {
            SharedPtr<AnimationController> animController(node->CreateComponent<AnimationController>(CreateMode::LOCAL));
            spawnedModel = SharedPtr<AnimatedModel>(node->CreateComponent<AnimatedModel>(CreateMode::LOCAL));
            shape->SetShapeType(ShapeType::SHAPE_CAPSULE);
            Vector3 hs = model->GetBoundingBox().HalfSize();
            Vector3 s = model->GetBoundingBox().Size();
            shape->SetCapsule(s.x_, s.y_, Vector3(0,hs.y_, 0));

        }
        spawnedModel->SetModel(model);
        spawnedModel->SetCastShadows(true);
        spawnedModel->ApplyMaterialList();

        // Check if model's material attr is set,
        // if not set then create copy list and fill empty attr's with default material
        if (Material* defaultMaterial = cache->GetResource<Material>(defaultResourceMaterial))
        {
            ResourceRefList oldMaterialList = spawnedModel->GetMaterialsAttr();
            StringVector newMaterialList;
            bool needUpdateAttr = false;
            for (ea::string name : oldMaterialList.names_)
            {
                if (name.empty())
                {
                    needUpdateAttr = true;
                    newMaterialList.push_back(defaultResourceMaterial);
                }
                else
                    newMaterialList.push_back(name); 
            }
            if (needUpdateAttr)
            {
                const ResourceRefList materialsAttr(Material::GetTypeStatic(), newMaterialList);
                spawnedModel->SetMaterialsAttr(materialsAttr);
            }
        }

        node->SetPosition(result.position_);
        {
            BoundingBox bb = spawnedModel->GetWorldBoundingBox();
            if (result.position_.DistanceToPoint(bb.Center()) < 0.05f)
                node->SetPosition(result.position_ + Vector3(0, bb.HalfSize().y_, 0));
        }

        if (ui::IsKeyDown(KEY_ALT))
        {
            Quaternion q;
            q.FromLookRotation(result.normal_);
            node->SetRotation(q * Quaternion(90, 0, 0));
        }
        
        const bool toggle = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
        const bool append = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);
        SelectNode(scenePage.selection_, node, toggle, append);

        owner_->PushAction<CreateRemoveNodeAction>(node, false);
    }
}
void SceneDragAndDropPrefabs::AssignMaterialToDrawable(
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
} // namespace Urho3D
