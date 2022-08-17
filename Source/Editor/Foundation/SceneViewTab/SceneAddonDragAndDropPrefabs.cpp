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

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/ResourceCache.h>


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

void SceneDragAndDropPrefabs::Render(SceneViewPage& scenePage)
{
    DragAndDropPrefabsToSceneView(scenePage);
}

void SceneDragAndDropPrefabs::DragAndDropPrefabsToSceneView(SceneViewPage& page)
{
    if (ui::BeginDragDropTarget() && ui::IsMouseReleased(0))
    {
        if (auto payload = static_cast<ResourceDragDropPayload*>(DragDropPayload::Get()))
        {
            for (const ResourceFileDescriptor& desc : payload->resources_)
            {
                bool CanBeDroppedTo = desc.HasObjectType<Scene>();

                if (CanBeDroppedTo)
                {
                    ResourceCache* cache = owner_->GetContext()->GetSubsystem<ResourceCache>();
                    if (XMLFile* prefabFile = cache->GetResource<XMLFile>(desc.resourceName_))
                    {
                        auto prefabNode = page.scene_->CreateChild(GetFileName(desc.localName_), CreateMode::LOCAL);

                        SharedPtr<PrefabReference> prefabRef{
                            prefabNode->CreateComponent<PrefabReference>(CreateMode::LOCAL)};
                        prefabRef->SetPrefab(prefabFile);

                        Camera* camera = page.renderer_->GetCamera();
                        ImGuiIO& io = ui::GetIO();

                        const ImRect viewportRect{ui::GetItemRectMin(), ui::GetItemRectMax()};
                        const auto pos = ToVector2((io.MousePos - viewportRect.Min) / viewportRect.GetSize());
                        const Ray cameraRay = camera->GetScreenRay(pos.x_, pos.y_);

                        ea::vector<RayQueryResult> results;
                        RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
                        page.scene_->GetComponent<Octree>()->RaycastSingle(query);

                        for (const RayQueryResult& result : results)
                        {
                            if (result.drawable_->GetScene() != nullptr)
                            {
                                prefabNode->SetPosition(result.position_);
                                if (ui::IsKeyDown(Urho3D::Key::KEY_ALT))
                                {
                                    Quaternion q;
                                    q.FromLookRotation(result.normal_);
                                    prefabNode->SetRotation(q * Quaternion(90, 0, 0));
                                }
                            }
                        }

                        page.selection_.Clear();
                        page.selection_.SetSelected(prefabNode, true);
                        owner_->PushAction<CreateRemoveNodeAction>(prefabNode, false);
                    }
                }
            }
        }
        ui::EndDragDropTarget();
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
} // namespace Urho3D
