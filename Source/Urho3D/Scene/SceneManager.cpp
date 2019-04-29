//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderSurface.h"
#include "../IO/Log.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/SceneManager.h"
#include "../Scene/SceneMetadata.h"


namespace Urho3D
{

SceneManager::SceneManager(Context* context)
    : Object(context)
{
}

void SceneManager::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneManager>();
}

Scene* SceneManager::CreateScene(const ea::string& name)
{
    if (GetScene(name) != nullptr)
    {
        URHO3D_LOGERRORF("Scene '%s' already exists.", name.c_str());
        return nullptr;
    }
    ea::shared_ptr<Scene> scene(context_->CreateObject<Scene>());
    scene->SetName(name);
    scene->GetOrCreateComponent<Octree>();
    scene->GetOrCreateComponent<SceneMetadata>(LOCAL);
    scenes_.push_back(scene);
    return scene;
}

Scene* SceneManager::GetScene(const ea::string& name)
{
    for (auto& scene : scenes_)
    {
        if (scene->GetName() == name)
            return scene;
    }
    return nullptr;
}

Scene* SceneManager::GetOrCreateScene(const ea::string& name)
{
    if (Scene* scene = GetScene(name))
        return scene;
    return CreateScene(name);
}

void SceneManager::UnloadScene(Scene* scene)
{
    auto it = scenes_.find(ea::shared_ptr<Scene>(scene));
    if (it != scenes_.end())
        scenes_.erase(it);
    if (activeScene_.expired())
        UpdateViewports();
}

void SceneManager::UnloadScene(const ea::string& name)
{
    scenes_.erase_first(ea::shared_ptr<Scene>(GetScene(name)));
    if (activeScene_.expired())
        UpdateViewports();
}

void SceneManager::UnloadAll()
{
    SetActiveScene(nullptr);
    auto scenesCopy = scenes_;
    for (auto& scene : scenesCopy)
        UnloadScene(scene.get());
}

void SceneManager::UnloadAllButActiveScene()
{
    auto scenesCopy = scenes_;
    for (auto& scene : scenesCopy)
    {
        if (scene != activeScene_.get())
            UnloadScene(scene.get());
    }
}

void SceneManager::SetActiveScene(Scene* scene)
{
    using namespace SceneActivated;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OLDSCENE] = activeScene_;
    eventData[P_NEWSCENE] = scene;

    if (!activeScene_.expired())
        activeScene_->SetUpdateEnabled(false);

    activeScene_ = scene;
    missingMetadataWarned_ = false;
    SendEvent(E_SCENEACTIVATED, eventData);
    UpdateViewports();
}

void SceneManager::SetActiveScene(const ea::string& name)
{
    SetActiveScene(GetScene(name));
}

void SceneManager::SetRenderSurface(RenderSurface* surface)
{
    renderSurface_ = surface;
    UpdateViewports();
}

void SceneManager::UpdateViewports()
{
    if (renderSurface_.expired())
        GetRenderer()->SetNumViewports(0);
    else
        renderSurface_->SetNumViewports(0);

    if (activeScene_.expired())
        return;

    SceneMetadata* meta = activeScene_->GetComponent<SceneMetadata>();
    if (meta == nullptr)
    {
        if (!missingMetadataWarned_)
        {
            URHO3D_LOGERRORF("Viewports can not be updated active scene does not have '%s' component.",
                SceneMetadata::GetTypeNameStatic().c_str());
            missingMetadataWarned_ = true;
        }
        return;
    }

    unsigned index = 0;
    const auto& viewportComponents = meta->GetCameraViewportComponents();
    if (renderSurface_.expired())
        GetRenderer()->SetNumViewports(viewportComponents.size());
    else
        renderSurface_->SetNumViewports(viewportComponents.size());

    for (const auto& cameraViewport : viewportComponents)
    {
        if (cameraViewport.expired())
            continue;

        // Trigger resizing of underlying viewport
        if (renderSurface_.expired())
            cameraViewport->SetScreenRect({0, 0, GetGraphics()->GetWidth(), GetGraphics()->GetHeight()});
        else
            cameraViewport->SetScreenRect({0, 0, renderSurface_->GetWidth(), renderSurface_->GetHeight()});
        cameraViewport->UpdateViewport();
        cameraViewport->GetViewport()->SetDrawDebug(false); // TODO: make this configurable maybe?
        if (renderSurface_.expired())
            GetRenderer()->SetViewport(index++, cameraViewport->GetViewport());
        else
            renderSurface_->SetViewport(index++, cameraViewport->GetViewport());
    }
}

}
