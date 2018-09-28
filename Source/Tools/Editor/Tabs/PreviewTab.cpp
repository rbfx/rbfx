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

#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Scene/CameraViewport.h>
#include <Urho3D/Scene/SceneMetadata.h>
#include "EditorEvents.h"
#include "PreviewTab.h"

namespace Urho3D
{

PreviewTab::PreviewTab(Context* context)
    : Tab(context)
{
    SetTitle("Game");
    isUtility_ = true;
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    view_ = context_->CreateObject<Texture2D>();

    // Ensure parts of texture are not left dirty when viewport does not cover entire texture.
    SubscribeToEvent(E_CAMERAVIEWPORTRESIZED, [this](StringHash, VariantMap& args) { Clear(); });
    // Ensure views are updated upon component addition or removal.
    SubscribeToEvent(E_COMPONENTADDED, [this](StringHash, VariantMap& args) {
        OnComponentUpdated(static_cast<Component*>(args[ComponentAdded::P_COMPONENT].GetPtr()));
    });
    SubscribeToEvent(E_COMPONENTREMOVED, [this](StringHash, VariantMap& args) {
        OnComponentUpdated(static_cast<Component*>(args[ComponentRemoved::P_COMPONENT].GetPtr()));
    });
    // Reload viewports when renderpath or postprocess was modified.
    SubscribeToEvent(E_RELOADFINISHED, [this](StringHash, VariantMap& args) {
        using namespace ReloadFinished;
        if (scene_.Null())
            return;

        if (auto* resource = dynamic_cast<Resource*>(GetEventSender()))
        {
            if (resource->GetName().StartsWith("RenderPaths/") || resource->GetName().StartsWith("PostProcess/"))
            {
                if (auto* metadata = scene_->GetOrCreateComponent<SceneMetadata>())
                {
                    auto& viewportComponents = metadata->GetCameraViewportComponents();
                    for (auto& component : viewportComponents)
                        component->RebuildRenderPath();
                    Clear();
                }
            }
        }
    });
}

IntRect PreviewTab::UpdateViewRect()
{
    IntRect tabRect = BaseClassName::UpdateViewRect();
    if (viewRect_ != tabRect)
    {
        viewRect_ = tabRect;
        context_->SetGlobalVar("__GameScreenSize__", viewRect_.Size());
        view_->SetSize(tabRect.Width(), tabRect.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        view_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
        UpdateViewports();
    }
    return tabRect;
}

bool PreviewTab::RenderWindowContent()
{
    if (scene_.Null())
        return true;

    IntRect rect = UpdateViewRect();
    ui::Image(view_.Get(), ImVec2{static_cast<float>(rect.Width()), static_cast<float>(rect.Height())});

    return true;
}

void PreviewTab::SetPreviewScene(Scene* scene)
{
    scene_ = scene;
    UpdateViewports();
}

void PreviewTab::Clear()
{
    if (view_->GetWidth() > 0 && view_->GetHeight() > 0)
    {
        Image black(context_);
        black.SetSize(view_->GetWidth(), view_->GetHeight(), 3);
        black.Clear(Color::BLACK);
        view_->SetData(&black);
    }
}

void PreviewTab::UpdateViewports()
{
    Clear();
    if (scene_.Expired())
        return;

    if (RenderSurface* surface = view_->GetRenderSurface())
    {
        surface->SetNumViewports(0);        // New scenes need all viewports cleared
        if (auto* metadata = scene_->GetComponent<SceneMetadata>())
        {
            unsigned index = 0;
            const auto& viewportComponents = metadata->GetCameraViewportComponents();
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
}

void PreviewTab::OnComponentUpdated(Component* component)
{
    if (component == nullptr)
        return;

    if (component->GetScene() != scene_)
        return;

    if (component->IsInstanceOf<CameraViewport>())
        UpdateViewports();
}

}
