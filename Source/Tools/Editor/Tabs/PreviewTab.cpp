//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Scene/CameraViewport.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SceneMetadata.h>
#include <Urho3D/SystemUI/Console.h>
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
    SetID("d75264a1-4179-4350-8e9f-ec4e4a15a7fa");
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
        Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();
        if (scene == nullptr)
            return;

        if (auto* resource = GetEventSender()->Cast<Resource>())
        {
            if (resource->GetName().starts_with("RenderPaths/") || resource->GetName().starts_with("PostProcess/"))
            {
                if (auto* manager = scene->GetOrCreateComponent<SceneMetadata>())
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

        SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();
        if (tab == nullptr || tab->GetScene() == nullptr)
            return;

        tab->GetUndo().SetTrackingEnabled(false);
        tab->SaveState(sceneReloadState_);
        tab->GetScene()->RemoveAllChildren();
        tab->GetScene()->RemoveAllComponents();
    });
    SubscribeToEvent(E_EDITORUSERCODERELOADEND, [&](StringHash, VariantMap&) {
        SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();
        if (tab == nullptr || tab->GetScene() == nullptr)
            return;

        tab->RestoreState(sceneReloadState_);
        tab->GetUndo().SetTrackingEnabled(true);
    });
    SubscribeToEvent(E_ENDALLVIEWSRENDER, [this](StringHash, VariantMap&) {
        RenderUI();
    });
    SubscribeToEvent(E_SCENEACTIVATED, [this](StringHash, VariantMap&) {
        UpdateViewports();
    });
    SubscribeToEvent(E_ENDRENDERINGSYSTEMUI, [this](StringHash, VariantMap&) {
        if (simulationStatus_ == SCENE_SIMULATION_STOPPED)
            dim_ = Max(dim_ - GetTime()->GetTimeStep() * 10, 0.f);
        else
            dim_ = Min(dim_ + GetTime()->GetTimeStep() * 6, 1.f);

        if (dim_ > M_EPSILON)
        {
            // Dim other windows except for preview.
            ImGuiContext& g = *ui::GetCurrentContext();
            const ea::string& sceneTabName = GetSubsystem<Editor>()->GetTab<SceneTab>()->GetUniqueTitle();
            for (int i = 0; i < g.Windows.Size; i++)
            {
                ImGuiWindow* window = g.Windows[i];
                if (window->ParentWindow != nullptr && window->DockNode != nullptr)
                {
                    // Ignore any non-leaf windows
                    if (window->DockNode->ChildNodes[0] || window->DockNode->ChildNodes[1])
                        continue;
                    if (!window->DockTabIsVisible)
                        continue;
                    // Editor scene viewport is not dimmed.
                    if (strcmp(window->Name, sceneTabName.c_str()) == 0)
                        continue;
                    // Game preview viewport is not dimmed.
                    if (strcmp(window->Name, GetUniqueTitle().c_str()) == 0)
                        continue;
                    ImDrawList* drawLists = ui::GetBackgroundDrawList(window->Viewport);
                    const ImU32 color = ui::GetColorU32(ImGuiCol_ModalWindowDimBg, dim_);
                    drawLists->AddRectFilled(window->Pos, window->Pos + window->Size, color);
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
        view_->SetSize(tabRect.Width(), tabRect.Height(), Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        view_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
        static_cast<RootUIElement*>(GetUI()->GetRoot())->SetOffset(tabRect.Min());
        UpdateViewports();
    }
    return tabRect;
}

bool PreviewTab::RenderWindowContent()
{
    if (GetSubsystem<SceneManager>()->GetActiveScene() == nullptr)
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

    if (GetSubsystem<SceneManager>()->GetActiveScene() == nullptr)
        return;

    if (RenderSurface* surface = view_->GetRenderSurface())
        GetSubsystem<SceneManager>()->SetRenderSurface(surface);
}

void PreviewTab::OnComponentUpdated(Component* component)
{
    Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();

    if (component == nullptr || scene == nullptr)
        return;

    if (component->GetScene() != scene)
        return;

    if (component->IsInstanceOf<CameraViewport>())
        UpdateViewports();
}

void PreviewTab::RenderButtons()
{
    Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();

    if (scene == nullptr)
    {
        if (simulationStatus_ != SCENE_SIMULATION_STOPPED)
            simulationStatus_ = SCENE_SIMULATION_STOPPED;
        return;
    }

    switch (simulationStatus_)
    {
    case SCENE_SIMULATION_RUNNING:
    {
        float timeStep = GetTime()->GetTimeStep();
        scene->Update(timeStep);
        GetUI()->Update(timeStep);
    }
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
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    switch (simulationStatus_)
    {
    case SCENE_SIMULATION_STOPPED:
    {
        // Scene was not running. Allow scene to set up input parameters.
        tab->GetUndo().SetTrackingEnabled(false);
        tab->SaveState(sceneState_);
        GetUI()->SetBlockEvents(false);
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
        GetUI()->SetBlockEvents(false);
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
    GetUI()->SetBlockEvents(true);
}

void PreviewTab::Toggle()
{
    Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();
    if (scene == nullptr)
        return;

    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
        Pause();
    else
        Play();
}

void PreviewTab::Step(float timeStep)
{
    Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();
    if (scene == nullptr)
        return;

    if (simulationStatus_ == SCENE_SIMULATION_STOPPED)
        Play();

    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
        Pause();

    scene->Update(timeStep);
    GetUI()->Update(timeStep);
}

void PreviewTab::Stop()
{
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    if (IsScenePlaying())
    {
        SendEvent(E_SIMULATIONSTOP);
        simulationStatus_ = SCENE_SIMULATION_STOPPED;
        tab->RestoreState(sceneState_);
        tab->GetUndo().SetTrackingEnabled(true);
        GetUI()->SetBlockEvents(true);
    }
}

void PreviewTab::Snapshot()
{
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    tab->GetUndo().Clear();
    tab->SaveState(sceneState_);
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

void PreviewTab::RenderUI()
{
    Graphics* graphics = GetGraphics();

    if (RenderSurface* surface = view_->GetRenderSurface())
    {
        GetUI()->SetCustomSize(surface->GetWidth(), surface->GetHeight());
        graphics->ResetRenderTargets();
        graphics->SetDepthStencil(surface->GetLinkedDepthStencil());
        graphics->SetRenderTarget(0, surface);
        graphics->SetViewport(IntRect(0, 0, surface->GetWidth(), surface->GetHeight()));
        GetUI()->Render();
    }
}

}
