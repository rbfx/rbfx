//
// Copyright (c) 2018 Rokas Kupstys
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

Scene* SceneManager::CreateScene(const String& name)
{
    if (GetScene(name) != nullptr)
    {
        URHO3D_LOGERRORF("Scene '%s' already exists.", name.CString());
        return nullptr;
    }
    SharedPtr<Scene> scene(context_->CreateObject<Scene>());
    scene->SetName(name);
    scenes_.Push(scene);
    return scene.Get();
}

Scene* SceneManager::GetScene(const String& name)
{
    for (auto& scene : scenes_)
    {
        if (scene->GetName() == name)
            return scene.Get();
    }
    return nullptr;
}

Scene* SceneManager::GetOrCreateScene(const String& name)
{
    if (Scene* scene = GetScene(name))
        return scene;
    return CreateScene(name);
}

void SceneManager::UnloadScene(Scene* scene)
{
    auto it = scenes_.Find(SharedPtr<Scene>(scene));
    if (it != scenes_.End())
        scenes_.Erase(it);
    if (activeScene_.Expired())
        UpdateViewports();
}

void SceneManager::UnloadScene(const String& name)
{
    scenes_.Remove(SharedPtr<Scene>(GetScene(name)));
    if (activeScene_.Expired())
        UpdateViewports();
}

void SceneManager::UnloadAllButActiveScene()
{
    auto scenesCopy = scenes_;
    for (auto& scene : scenesCopy)
    {
        if (scene != activeScene_)
            UnloadScene(scene);
    }
}

void SceneManager::SetActiveScene(Scene* scene)
{
    if (scene != nullptr)
    {
        scene->GetOrCreateComponent<Octree>();
        scene->GetOrCreateComponent<SceneMetadata>(LOCAL);
    }

    using namespace SceneActivated;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OLDSCENE] = activeScene_.Get();
    eventData[P_NEWSCENE] = scene;
    activeScene_ = scene;
    missingMetadataWarned_ = false;
    SendEvent(E_SCENEACTIVATED, eventData);
    UpdateViewports();
}

void SceneManager::SetActiveScene(const String& name)
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
    if (renderSurface_.Expired())
        GetRenderer()->SetNumViewports(0);
    else
        renderSurface_->SetNumViewports(0);

    if (activeScene_.Expired())
        return;

    SceneMetadata* meta = activeScene_->GetComponent<SceneMetadata>();
    if (meta == nullptr)
    {
        if (!missingMetadataWarned_)
        {
            URHO3D_LOGERRORF("Viewports can not be updated active scene does not have '%s' component.",
                SceneMetadata::GetTypeNameStatic().CString());
            missingMetadataWarned_ = true;
        }
        return;
    }

    unsigned index = 0;
    const auto& viewportComponents = meta->GetCameraViewportComponents();
    if (renderSurface_.Expired())
        GetRenderer()->SetNumViewports(viewportComponents.Size());
    else
        renderSurface_->SetNumViewports(viewportComponents.Size());

    for (const auto& cameraViewport : viewportComponents)
    {
        // Trigger resizing of underlying viewport
        cameraViewport->SetNormalizedRect(cameraViewport->GetNormalizedRect());
        cameraViewport->GetViewport()->SetDrawDebug(false); // TODO: make this configurable maybe?
        if (renderSurface_.Expired())
            GetRenderer()->SetViewport(index++, cameraViewport->GetViewport());
        else
            renderSurface_->SetViewport(index++, cameraViewport->GetViewport());
    }
}

}
