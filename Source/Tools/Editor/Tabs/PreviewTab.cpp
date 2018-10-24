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
#include <Urho3D/Scene/SceneManager.h>
#include <Urho3D/Core/Timer.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "EditorEventsPrivate.h"
#include "EditorEvents.h"
#include "Editor.h"
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
        if (sceneTab_.Expired())
            return;

        if (auto* resource = GetEventSender()->Cast<Resource>())
        {
            if (resource->GetName().StartsWith("RenderPaths/") || resource->GetName().StartsWith("PostProcess/"))
            {
                if (auto* manager = sceneTab_->GetScene()->GetOrCreateComponent<SceneManager>())
                {
                    auto& viewportComponents = manager->GetCameraViewportComponents();
                    for (auto& component : viewportComponents)
                        component->RebuildRenderPath();
                    Clear();
                }
            }
        }
    });

    // On plugin code reload all scene state is serialized, plugin library is reloaded and scene state is unserialized.
    // This way scene recreates all plugin-provided components on reload and gets to use new versions of them.
    SubscribeToEvent(E_EDITORUSERCODERELOADSTART, [&](StringHash, VariantMap&) {
        if (sceneTab_.Expired())
            return;

        sceneTab_->GetUndo().SetTrackingEnabled(false);
        sceneTab_->SceneStateSave(sceneReloadState_);
        sceneTab_->GetScene()->RemoveAllChildren();
        sceneTab_->GetScene()->RemoveAllComponents();
    });
    SubscribeToEvent(E_EDITORUSERCODERELOADEND, [&](StringHash, VariantMap&) {
        if (sceneTab_.Expired())
            return;

        sceneTab_->SceneStateRestore(sceneReloadState_);
        sceneTab_->GetUndo().SetTrackingEnabled(true);
    });
}

IntRect PreviewTab::UpdateViewRect()
{
    IntRect tabRect = BaseClassName::UpdateViewRect();
    if (viewRect_ != tabRect)
    {
        viewRect_ = tabRect;
        view_->SetSize(tabRect.Width(), tabRect.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        view_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
        UpdateViewports();
    }
    return tabRect;
}

bool PreviewTab::RenderWindowContent()
{
    if (sceneTab_.Expired())
        return true;

    IntRect rect = UpdateViewRect();
    ui::Image(view_.Get(), ImVec2{static_cast<float>(rect.Width()), static_cast<float>(rect.Height())});
    if (!inputGrabbed_ && simulationStatus_ == SCENE_SIMULATION_RUNNING && ui::IsItemHovered() &&
        ui::IsAnyMouseDown() && GetInput()->IsMouseVisible())
        GrabInput();

    return true;
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

void PreviewTab::OnBeforeBegin()
{
    // Allow viewport texture to cover entire window
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
}

void PreviewTab::OnAfterEnd()
{
    ui::PopStyleVar();  // ImGuiStyleVar_WindowPadding
}

void PreviewTab::UpdateViewports()
{
    Clear();
    if (sceneTab_.Expired())
        return;

    if (RenderSurface* surface = view_->GetRenderSurface())
    {
        if (auto* manager = sceneTab_->GetScene()->GetComponent<SceneManager>())
            manager->SetSceneActive(surface);
    }
}

void PreviewTab::OnComponentUpdated(Component* component)
{
    if (component == nullptr || sceneTab_.Expired())
        return;

    if (component->GetScene() != sceneTab_->GetScene())
        return;

    if (component->IsInstanceOf<CameraViewport>())
        UpdateViewports();
}

void PreviewTab::RenderButtons()
{
    if (Tab* activeTab = GetSubsystem<Editor>()->GetActiveTab())
    {
        if (SceneTab* tab = activeTab->Cast<SceneTab>())
        {
            if (!IsScenePlaying() && sceneTab_ != tab)
            {
                // Switch to another scene only if there was no previous scene that was played. Only one scene can be played
                // at a time.
                sceneTab_ = tab;
                UpdateViewports();
            }
        }
    }

    if (sceneTab_.Expired())
    {
        if (simulationStatus_ != SCENE_SIMULATION_STOPPED)
            simulationStatus_ = SCENE_SIMULATION_STOPPED;
        return;
    }

    switch (simulationStatus_)
    {
    case SCENE_SIMULATION_RUNNING:
        sceneTab_->GetScene()->Update(GetTime()->GetTimeStep());
    case SCENE_SIMULATION_PAUSED:
    {
        if (GetInput()->GetKeyPress(KEY_ESCAPE))
        {
            if (Time::GetSystemTime() - lastEscPressTime_ > 300)
                lastEscPressTime_ = Time::GetSystemTime();
            else
                ReleaseInput();
        }
    }
    default:
        break;
    }

    if (ui::EditorToolbarButton(ICON_FA_FAST_BACKWARD, "Restore"))
        Stop();

    bool isSimulationRunning = simulationStatus_ == SCENE_SIMULATION_RUNNING;
    if (ui::EditorToolbarButton(isSimulationRunning ? ICON_FA_PAUSE : ICON_FA_PLAY,
                                isSimulationRunning ? "Pause" : "Play", simulationStatus_ != SCENE_SIMULATION_STOPPED))
        Toggle();

    if (ui::EditorToolbarButton(ICON_FA_STEP_FORWARD, "Simulate one frame"))
        Step(1.f / 60.f);

    if (ui::EditorToolbarButton(ICON_FA_SAVE, "Save current state as master state.\n" ICON_FA_EXCLAMATION_TRIANGLE " Clears scene undo state!"))
    Snapshot();
}

void PreviewTab::Play()
{
    if (sceneTab_.Expired())
        return;

    switch (simulationStatus_)
    {
    case SCENE_SIMULATION_STOPPED:
    {
        // Scene was not running. Allow scene to set up input parameters.
        sceneTab_->GetUndo().SetTrackingEnabled(false);
        sceneTab_->SceneStateSave(sceneState_);
        simulationStatus_ = SCENE_SIMULATION_RUNNING;
        SendEvent(E_SIMULATIONSTART);
        inputGrabbed_ = true;
        ReleaseInput();
        break;
    }
    case SCENE_SIMULATION_PAUSED:
    {
        // Scene was paused. When resuming restore saved scene input parameters.
        simulationStatus_ = SCENE_SIMULATION_RUNNING;
        break;
    }
    default:
        break;
    }
}

void PreviewTab::Pause()
{
    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
        simulationStatus_ = SCENE_SIMULATION_PAUSED;
}

void PreviewTab::Toggle()
{
    if (sceneTab_.Expired())
        return;

    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
        Pause();
    else
        Play();
}

void PreviewTab::Step(float timeStep)
{
    if (sceneTab_.Expired())
        return;

    if (simulationStatus_ == SCENE_SIMULATION_STOPPED)
        Play();

    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
        Pause();

    sceneTab_->GetScene()->Update(timeStep);
}

void PreviewTab::Stop()
{
    if (sceneTab_.Expired())
        return;

    if (IsScenePlaying())
    {
        SendEvent(E_SIMULATIONSTOP);
        simulationStatus_ = SCENE_SIMULATION_STOPPED;
        sceneTab_->SceneStateRestore(sceneState_);
        sceneTab_->GetUndo().SetTrackingEnabled(true);
    }
}

void PreviewTab::Snapshot()
{
    if (sceneTab_.Expired())
        return;

    sceneTab_->GetUndo().Clear();
    sceneState_.Clear();
    sceneTab_->SceneStateSave(sceneState_);
}

void PreviewTab::GrabInput()
{
    if (inputGrabbed_)
        return;

    inputGrabbed_ = true;
    GetInput()->SetMouseVisible(sceneMouseVisible_);
    GetInput()->SetMouseMode(sceneMouseMode_);
    GetInput()->SetShouldIgnoreInput(false);
}

void PreviewTab::ReleaseInput()
{
    if (!inputGrabbed_)
        return;

    inputGrabbed_ = false;
    sceneMouseVisible_ = GetInput()->IsMouseVisible();
    sceneMouseMode_ = GetInput()->GetMouseMode();
    GetInput()->SetMouseVisible(true);
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetShouldIgnoreInput(true);
}

}
