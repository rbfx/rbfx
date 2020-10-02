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

#include <Urho3D/Audio/Audio.h>
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
#include <Urho3D/SystemUI/Console.h>
#if URHO3D_RMLUI
#   include <Urho3D/RmlUI/RmlUI.h>
#endif
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
    texture_ = context_->CreateObject<Texture2D>();
    noContentPadding_ = true;
#if URHO3D_RMLUI
    auto* ui = context->GetSubsystem<RmlUI>();
    ui->UnsubscribeFromEvent(E_ENDALLVIEWSRENDER);
    ui->mouseMoveEvent_.Subscribe(this, &PreviewTab::RemapUICursorPos);
#endif

    SubscribeToEvent(E_CAMERAVIEWPORTRESIZED, &PreviewTab::OnCameraViewportResized);
    SubscribeToEvent(E_COMPONENTADDED, &PreviewTab::OnComponentAdded);
    SubscribeToEvent(E_COMPONENTREMOVED, &PreviewTab::OnComponentRemoved);
    SubscribeToEvent(E_RELOADFINISHED, &PreviewTab::OnReloadFinished);
    SubscribeToEvent(E_EDITORUSERCODERELOADSTART, &PreviewTab::OnEditorUserCodeReloadStart);
    SubscribeToEvent(E_EDITORUSERCODERELOADEND, &PreviewTab::OnEditorUserCodeReloadEnd);
    SubscribeToEvent(E_ENDALLVIEWSRENDER, &PreviewTab::OnEndAllViewsRender);
    SubscribeToEvent(E_SCENEACTIVATED, &PreviewTab::OnSceneActivated);
    SubscribeToEvent(E_ENDRENDERINGSYSTEMUI, &PreviewTab::OnEndRenderingSystemUI);
}

void PreviewTab::OnCameraViewportResized(StringHash type, VariantMap& args)
{
    // Ensure parts of texture are not left dirty when viewport does not cover entire texture.
    Clear();
}

void PreviewTab::OnComponentAdded(StringHash type, VariantMap& args)
{
    // Ensure views are updated upon component addition or removal.
    OnComponentUpdated(static_cast<Component*>(args[ComponentAdded::P_COMPONENT].GetPtr()));
}

void PreviewTab::OnComponentRemoved(StringHash type, VariantMap& args)
{
    // Ensure views are updated upon component addition or removal.
    OnComponentUpdated(static_cast<Component*>(args[ComponentRemoved::P_COMPONENT].GetPtr()));
}

void PreviewTab::OnReloadFinished(StringHash type, VariantMap& args)
{
    // Reload viewports when renderpath or postprocess was modified.
    using namespace ReloadFinished;
    Scene* scene = GetSubsystem<SceneManager>()->GetActiveScene();
    if (scene == nullptr)
        return;

    if (auto* resource = GetEventSender()->Cast<Resource>())
    {
        if (resource->GetName().starts_with("RenderPaths/") || resource->GetName().starts_with("PostProcess/"))
        {
            for (Component* const component : scene->GetComponentIndex<CameraViewport>())
            {
                auto* const cameraViewport = static_cast<CameraViewport* const>(component);
                cameraViewport->RebuildRenderPath();
            }
            Clear();
        }
    }
}

void PreviewTab::OnEditorUserCodeReloadStart(StringHash type, VariantMap& args)
{
    // On plugin code reload all scene state is serialized, plugin library is reloaded and scene state is unserialized.
    // This way scene recreates all plugin-provided components on reload and gets to use new versions of them.
    auto* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();
    if (tab == nullptr || tab->GetScene() == nullptr)
        return;

    undo_->SetTrackingEnabled(false);
    tab->SaveState(sceneReloadState_);
    tab->GetScene()->RemoveAllChildren();
    tab->GetScene()->RemoveAllComponents();
}

void PreviewTab::OnEditorUserCodeReloadEnd(StringHash type, VariantMap& args)
{
    auto* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();
    if (tab == nullptr || tab->GetScene() == nullptr)
        return;

    tab->RestoreState(sceneReloadState_);
    undo_->SetTrackingEnabled(true);
}

void PreviewTab::OnEndAllViewsRender(StringHash type, VariantMap& args)
{
    RenderUI();
}

void PreviewTab::OnSceneActivated(StringHash type, VariantMap& args)
{
    UpdateViewports();
}

void PreviewTab::OnEndRenderingSystemUI(StringHash type, VariantMap& args)
{
    if (simulationStatus_ == SCENE_SIMULATION_STOPPED)
        dim_ = Max(dim_ - context_->GetSubsystem<Time>()->GetTimeStep() * 10, 0.f);
    else
        dim_ = Min(dim_ + context_->GetSubsystem<Time>()->GetTimeStep() * 6, 1.f);

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
}

bool PreviewTab::RenderWindowContent()
{
    if (GetSubsystem<SceneManager>()->GetActiveScene() == nullptr || texture_.Null())
        return true;

    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuiViewport* viewport = window->Viewport;

    ImRect rect = ImRound(window->ContentRegionRect);
#if 0
    const float dpi = viewport->DpiScale;
#else
    const float dpi = 1.0f;
#endif

    IntVector2 textureSize{
        static_cast<int>(IM_ROUND(rect.GetWidth() * dpi)),
        static_cast<int>(IM_ROUND(rect.GetHeight() * dpi))
    };
    if (textureSize.x_ != texture_->GetWidth() || textureSize.y_ != texture_->GetHeight())
    {
        texture_->SetSize(textureSize.x_, textureSize.y_, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        if (auto* surface = texture_->GetRenderSurface())
            surface->SetUpdateMode(SURFACE_UPDATEALWAYS);
        UpdateViewports();

#if URHO3D_RMLUI
        auto* ui = GetSubsystem<RmlUI>();
        ui->SetRenderTarget(texture_);
#endif
    }

    ui::Image(texture_.Get(), rect.GetSize());
    ui::SetCursorScreenPos(ui::GetItemRectMin());
    ui::InvisibleButton("###preview", rect.GetSize());

    viewportRect_.min_ = ui::GetItemRectMin();
    viewportRect_.max_ = ui::GetItemRectMax();
    if (!window->ViewportOwned)
    {
        viewportRect_.min_ -= static_cast<Vector2>(viewport->Pos);
        viewportRect_.max_ -= static_cast<Vector2>(viewport->Pos);
    }

    if (simulationStatus_ == SCENE_SIMULATION_RUNNING)
    {
        if (ui::IsWindowFocused() && !inputGrabbed_)
            GrabInput();
        else if (!ui::IsWindowFocused() && inputGrabbed_)
            ReleaseInput();
    }

    return true;
}

void PreviewTab::Clear()
{
    if (texture_->GetWidth() > 0 && texture_->GetHeight() > 0)
    {
        Image black(context_);
        black.SetSize(texture_->GetWidth(), texture_->GetHeight(), 3);
        black.Clear(Color::BLACK);
        texture_->SetData(&black);
    }
}

void PreviewTab::UpdateViewports()
{
    Clear();

    if (GetSubsystem<SceneManager>()->GetActiveScene() == nullptr)
        return;

    if (RenderSurface* surface = texture_->GetRenderSurface())
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
        float timeStep = context_->GetSubsystem<Time>()->GetTimeStep();
        scene->Update(timeStep);
#if URHO3D_RMLUI
        if (auto* ui = GetSubsystem<RmlUI>())
            ui->Update(timeStep);
#endif
    }
    case SCENE_SIMULATION_PAUSED:
    {
        if (ui::IsKeyPressed(KEY_ESCAPE))
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

    ui::BeginButtonGroup();
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
    ui::EndButtonGroup();
}

void PreviewTab::Play()
{
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    switch (simulationStatus_)
    {
    case SCENE_SIMULATION_STOPPED:
    {
        // Scene was not running. Allow scene to set up input parameters.
        undo_->SetTrackingEnabled(false);
        tab->SaveState(sceneState_);
#if URHO3D_RMLUI
        if (auto* ui = GetSubsystem<RmlUI>())
            ui->SetBlockEvents(false);
#endif
        context_->GetSubsystem<Audio>()->Play();
        simulationStatus_ = SCENE_SIMULATION_RUNNING;
        SendEvent(E_SIMULATIONSTART);
        break;
    }
    case SCENE_SIMULATION_PAUSED:
    {
        // Scene was paused. When resuming restore saved scene input parameters.
        simulationStatus_ = SCENE_SIMULATION_RUNNING;
#if URHO3D_RMLUI
        if (auto* ui = GetSubsystem<RmlUI>())
            ui->SetBlockEvents(false);
#endif
        context_->GetSubsystem<Audio>()->Play();
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
#if URHO3D_RMLUI
    if (auto* ui = GetSubsystem<RmlUI>())
        ui->SetBlockEvents(true);
#endif
    context_->GetSubsystem<Audio>()->Stop();
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
#if URHO3D_RMLUI
    if (auto* ui = GetSubsystem<RmlUI>())
        ui->Update(timeStep);
#endif
    context_->GetSubsystem<Audio>()->Update(timeStep);
}

void PreviewTab::Stop()
{
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    if (IsScenePlaying())
    {
        SendEvent(E_SIMULATIONSTOP);
        simulationStatus_ = SCENE_SIMULATION_STOPPED;
        tab->RestoreState(sceneState_);
        undo_->SetTrackingEnabled(true);
#if URHO3D_RMLUI
        if (auto* ui = GetSubsystem<RmlUI>())
            ui->SetBlockEvents(true);
#endif
        context_->GetSubsystem<Audio>()->Stop();
    }
}

void PreviewTab::Snapshot()
{
    SceneTab* tab = GetSubsystem<Editor>()->GetTab<SceneTab>();

    tab->SaveState(sceneState_);
}

void PreviewTab::GrabInput()
{
    if (inputGrabbed_)
        return;

    Input* input = context_->GetSubsystem<Input>();
    SystemUI* systemUI = context_->GetSubsystem<SystemUI>();
    input->SetMouseVisible(sceneMouseVisible_);
    input->SetMouseMode(sceneMouseMode_);
    input->SetEnabled(true);
    systemUI->SetPassThroughEvents(true);
    inputGrabbed_ = true;
}

void PreviewTab::ReleaseInput()
{
    if (!inputGrabbed_)
        return;

    Input* input = context_->GetSubsystem<Input>();
    SystemUI* systemUI = context_->GetSubsystem<SystemUI>();
    inputGrabbed_ = false;
    sceneMouseVisible_ = input->IsMouseVisible();
    sceneMouseMode_ = input->GetMouseMode();
    input->SetMouseVisible(true);
    input->SetMouseMode(MM_ABSOLUTE);
    input->SetEnabled(false);
    systemUI->SetPassThroughEvents(false);
}

void PreviewTab::RenderUI()
{
#if URHO3D_RMLUI
    if (auto* ui = GetSubsystem<RmlUI>())
        ui->Render();
#endif
}

void PreviewTab::RemapUICursorPos(IntVector2& pos)
{
    pos.x_ -= viewportRect_.min_.x_;
    pos.y_ -= viewportRect_.min_.y_;
}

}
