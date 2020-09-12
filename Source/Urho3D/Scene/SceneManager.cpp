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

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderSurface.h"
#include "../IO/Log.h"
#include "../Scene/CameraViewport.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/SceneManager.h"


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
    SharedPtr<Scene> scene(context_->CreateObject<Scene>());
    scene->SetName(name);
    scene->CreateComponentIndex<CameraViewport>();
    scene->GetOrCreateComponent<Octree>();
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
    if (scene == nullptr)
        return;

    if (activeScene_ == scene)
        SetActiveScene(nullptr);

    scenes_.erase_first(SharedPtr<Scene>(scene));
}

void SceneManager::UnloadScene(const ea::string& name)
{
    SharedPtr<Scene> scene(GetScene(name));
    UnloadScene(scene);
}

void SceneManager::UnloadAll()
{
    SetActiveScene(nullptr);
    auto scenesCopy = scenes_;
    for (auto& scene : scenesCopy)
        UnloadScene(scene.Get());
}

void SceneManager::UnloadAllButActiveScene()
{
    auto scenesCopy = scenes_;
    for (auto& scene : scenesCopy)
    {
        if (scene != activeScene_.Get())
            UnloadScene(scene.Get());
    }
}

void SceneManager::SetActiveScene(Scene* scene)
{
    if (activeScene_ == scene)
        return;

    using namespace SceneActivated;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OLDSCENE] = activeScene_;
    eventData[P_NEWSCENE] = scene;

    if (!activeScene_.Expired())
        activeScene_->SetUpdateEnabled(false);

    activeScene_ = scene;
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
    if (renderSurface_.Expired())
        context_->GetSubsystem<Renderer>()->SetNumViewports(0);
    else
        renderSurface_->SetNumViewports(0);

    if (activeScene_.Expired())
        return;

    unsigned index = 0;
    const SceneComponentIndex& viewportComponents = activeScene_->GetComponentIndex<CameraViewport>();
    if (renderSurface_.Expired())
        context_->GetSubsystem<Renderer>()->SetNumViewports(viewportComponents.size());
    else
        renderSurface_->SetNumViewports(viewportComponents.size());

    for (Component* const component : viewportComponents)
    {
        auto* const cameraViewport = static_cast<CameraViewport* const>(component);
        // Trigger resizing of underlying viewport
        if (renderSurface_.Expired())
            cameraViewport->SetScreenRect({0, 0, context_->GetSubsystem<Graphics>()->GetWidth(), context_->GetSubsystem<Graphics>()->GetHeight()});
        else
            cameraViewport->SetScreenRect({0, 0, renderSurface_->GetWidth(), renderSurface_->GetHeight()});
        cameraViewport->UpdateViewport();
        cameraViewport->GetViewport()->SetDrawDebug(false); // TODO: make this configurable maybe?
        if (renderSurface_.Expired())
            context_->GetSubsystem<Renderer>()->SetViewport(index++, cameraViewport->GetViewport());
        else
            renderSurface_->SetViewport(index++, cameraViewport->GetViewport());
    }
}

}
