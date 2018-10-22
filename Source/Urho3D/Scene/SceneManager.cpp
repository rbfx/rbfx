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
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderSurface.h"
#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "SceneManager.h"


namespace Urho3D
{

SceneManager::SceneManager(Context* context)
    : Component(context)
{
    SetTemporary(true);
}

void SceneManager::RegisterComponent(Component* component)
{
    if (auto* viewportComponent = component->Cast<CameraViewport>())
        viewportComponents_.Push(WeakPtr<CameraViewport>(viewportComponent));
}

void SceneManager::UnregisterComponent(Component* component)
{
    if (auto* viewportComponent = component->Cast<CameraViewport>())
        viewportComponents_.Remove(WeakPtr<CameraViewport>(viewportComponent));
}

void SceneManager::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneManager>();
}

void SceneManager::SetSceneActive()
{
    unsigned index = 0;
    const auto& viewportComponents = GetCameraViewportComponents();
    GetRenderer()->SetNumViewports(viewportComponents.Size());
    for (const auto& cameraViewport : viewportComponents)
    {
        // Trigger resizing of underlying viewport
        cameraViewport->SetNormalizedRect(cameraViewport->GetNormalizedRect());
        cameraViewport->GetViewport()->SetDrawDebug(false);
        GetRenderer()->SetViewport(index++, cameraViewport->GetViewport());
    }
}

void SceneManager::SetSceneActive(RenderSurface* surface)
{
    unsigned index = 0;
    const auto& viewportComponents = GetCameraViewportComponents();
    surface->SetNumViewports(viewportComponents.Size());
    for (const auto& cameraViewport : viewportComponents)
    {
        // Trigger resizing of underlying viewport
        cameraViewport->SetNormalizedRect(cameraViewport->GetNormalizedRect());
        cameraViewport->GetViewport()->SetDrawDebug(false);
        surface->SetViewport(index++, cameraViewport->GetViewport());
    }
}

}
