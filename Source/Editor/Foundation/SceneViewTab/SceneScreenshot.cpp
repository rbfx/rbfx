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

#include "../../Foundation/SceneViewTab/SceneScreenshot.h"

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/RenderPipeline/RenderPipeline.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Math/Vector2.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_TakeScreenshot = EditorHotkey{"SceneScreenshot.Take"}.Shift().Press(KEY_P);

class ScreenshotRenderer : public Object
{
    URHO3D_OBJECT(ScreenshotRenderer, Object)

public:
    Signal<void(const Image& image)> OnReady;

    ScreenshotRenderer(Scene* scene, Camera* camera, const IntVector2& resolution);
    ~ScreenshotRenderer() override;

private:
    void OnEndViewRender();

    const bool oldDebugGeometry_{};
    const bool oldAutoAspectRatio_{};
    const float oldAspectRatio_{};
    const WeakPtr<Camera> camera_;

    SharedPtr<Texture2D> texture_;
    SharedPtr<Viewport> viewport_;
};

ScreenshotRenderer::ScreenshotRenderer(Scene* scene, Camera* camera, const IntVector2& resolution)
    : BaseClassName(scene->GetContext())
    , oldDebugGeometry_(camera->GetDrawDebugGeometry())
    , oldAspectRatio_(camera->GetAspectRatio())
    , oldAutoAspectRatio_(camera->GetAutoAspectRatio())
    , camera_(camera)
    , texture_(MakeShared<Texture2D>(context_))
{
    camera_->SetDrawDebugGeometry(false);
    camera_->SetAspectRatio(resolution.x_ / (float)resolution.y_);

    texture_->SetSize(
        resolution.x_, resolution.y_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget);
    SubscribeToEvent(texture_, E_ENDVIEWRENDER, &ScreenshotRenderer::OnEndViewRender);

    viewport_ = MakeShared<Viewport>(context_, scene, camera);

    RenderSurface* surface = texture_->GetRenderSurface();
    surface->SetViewport(0, viewport_);

    auto* renderer = GetSubsystem<Renderer>();
    renderer->QueueRenderSurface(surface);
}

ScreenshotRenderer::~ScreenshotRenderer()
{
    if (camera_)
    {
        camera_->SetDrawDebugGeometry(oldDebugGeometry_);
        camera_->SetAspectRatio(oldAspectRatio_);
        camera_->SetAutoAspectRatio(oldAutoAspectRatio_);
    }
}

void ScreenshotRenderer::OnEndViewRender()
{
    auto self = SharedPtr<ScreenshotRenderer>(this);
    OnReady(this, *texture_->GetImage());

    // Postpone deletion of this object until the end of frame
    auto queue = GetSubsystem<WorkQueue>();
    queue->PostDelayedTaskForMainThread([self]() {});
}

} // namespace

void Foundation_SceneScreenshot(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneScreenshot>();
}

SceneScreenshot::SceneScreenshot(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
    auto hotkeyManager = owner_->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_TakeScreenshot, &SceneScreenshot::TakeScreenshotWithPopup);
}

void SceneScreenshot::TakeScreenshotWithPopup()
{
    openPending_ = true;
}

void SceneScreenshot::TakeScreenshotDelayed(Scene* scene, Camera* camera, const IntVector2& resolution)
{
    auto queue = GetSubsystem<WorkQueue>();
    queue->PostDelayedTaskForMainThread(
        [this, scene = WeakPtr<Scene>(scene), camera = WeakPtr<Camera>(camera), resolution]()
    {
        if (scene && camera)
            TakeScreenshotNow(scene, camera, resolution);
    });
}

void SceneScreenshot::TakeScreenshotNow(Scene* scene, Camera* camera, const IntVector2& resolution)
{
    auto screenshotRenderer = MakeShared<ScreenshotRenderer>(scene, camera, resolution);
    screenshotRenderer->OnReady.Subscribe(this, [this, screenshotRenderer](const Image& image)
    {
        const ea::string fileName = GenerateFileName(image.GetSize().ToIntVector2());
        if (!fileName.empty())
        {
            image.SaveFile(fileName);
            URHO3D_LOGINFO("Screenshot saved to file://{}", fileName);
        }

        // Remove this callback and ScreenshotRenderer with it
        return false;
    });
}

ea::string SceneScreenshot::GenerateFileName(const IntVector2& size) const
{
    auto project = owner_->GetProject();
    auto fileSystem = GetSubsystem<FileSystem>();

    const ea::string& outputPath = project->GetArtifactsPath();
    const ea::string screenshotTime = Time::GetTimeStamp("%Y-%m-%d/%Y-%m-%d_%H-%M-%S");
    const ea::string fileNamePrefix = Format("{}Screenshots/{}_{}x{}", outputPath, screenshotTime, size.x_, size.y_);
    const ea::string fileNameSuffix = ".png";

    static const unsigned MaxAttempts = 100;
    for (unsigned i = 0; i < MaxAttempts; ++i)
    {
        const ea::string_view nameFormat = i != 0 ? "{0}_{1}{2}" : "{0}{2}";
        const ea::string fileName = Format(nameFormat, fileNamePrefix, i, fileNameSuffix);
        if (fileSystem->FileExists(fileName))
            continue;

        return fileName;
    }

    URHO3D_LOGERROR("Screenshot file already exists: {}{}", fileNamePrefix, fileNameSuffix);
    return EMPTY_STRING;
}

void SceneScreenshot::Render(SceneViewPage& page)
{
    static const char* popupName = "Take Screenshot from Scene";
    if (openPending_)
    {
        ui::OpenPopup(popupName);
        InitializePopup(page);
    }
    openPending_ = false;

    if (ui::BeginPopup(popupName, ImGuiWindowFlags_AlwaysAutoResize))
    {
        RenderPopup();
        ui::EndPopup();
    }
}

Camera* SceneScreenshot::FindCameraInSelection(const SceneSelection& selection) const
{
    // Try component first
    for (Component* component : selection.GetComponents())
    {
        if (auto camera = dynamic_cast<Camera*>(component))
            return camera;
    }

    // Then try to find camera in selected nodes directly, ignoring the Scene
    for (Node* node : selection.GetEffectiveNodes())
    {
        if (node)
        {
            if (auto camera = node->GetComponent<Camera>())
                return camera;
        }
    }

    // Then try to find camera in selected nodes' children, ignoring the Scene
    for (Node* node : selection.GetEffectiveNodes())
    {
        if (node)
        {
            if (auto camera = node->GetComponent<Camera>(true))
                return camera;
        }
    }

    return nullptr;
}

void SceneScreenshot::InitializePopup(SceneViewPage& page)
{
    scene_ = page.scene_;
    sceneCamera_ = FindCameraInSelection(page.selection_);
    editorCamera_ = page.renderer_->GetCamera();
    if (!sceneCamera_)
        useInSceneCamera_ = false;
}

void SceneScreenshot::RenderPopup()
{
    if (!scene_ || !editorCamera_)
    {
        ui::CloseCurrentPopup();
        return;
    }

    const bool hasInSceneCamera = sceneCamera_ != nullptr;
    ui::BeginDisabled(!hasInSceneCamera);
    ui::Checkbox("Use in-Scene Camera", &useInSceneCamera_);
    if (hasInSceneCamera)
    {
        const ColorScopeGuard guard{ImGuiCol_Text, ImVec4(1.00f, 1.00f, 0.35f, 1.00f)};
        ui::Text(sceneCamera_->GetFullNameDebug().c_str());
    }
    ui::EndDisabled();

    ui::Separator();
    static const ea::pair<const char*, IntVector2> resolutions[] = {
        ea::make_pair("Custom", IntVector2::ZERO),
        ea::make_pair("FullHD", IntVector2{1920, 1080}),
        ea::make_pair("FullHD Portrait", IntVector2{1080, 1920}),
        ea::make_pair("4K", IntVector2{3840, 2160}),
        ea::make_pair("4K Portrait", IntVector2{2160, 3840}),
        ea::make_pair("2K Square", IntVector2{2048, 2048}),
        ea::make_pair("Android Feature", IntVector2{1024, 500}),
        ea::make_pair("Android TV", IntVector2{1280, 720}),
        ea::make_pair("Youtube banner", IntVector2{2048, 1152}),
        ea::make_pair("Youtube watermark", IntVector2{150, 150}),
    };

    if (ui::BeginCombo("Resolution", resolutions[resolutionOption_].first))
    {
        if (ui::Selectable(resolutions[0].first, !resolutionOption_))
        {
            resolutionOption_ = 0;
        }
        for (unsigned i=1; i<ea::size(resolutions); ++i)
        {
            if (ui::Selectable(resolutions[i].first, resolutionOption_ == i))
            {
                resolutionOption_ = i;
                resolution_ = resolutions[i].second;
            }
        }
        ui::EndCombo();
    }

    if (!resolutionOption_)
    {
        ui::Text("Resolution: ");
        ui::SameLine();
        ui::PushItemWidth(100);
        ui::InputInt("##width", &resolution_.x_, 0);
        ui::SameLine();
        ui::Text("x");
        ui::SameLine();
        ui::InputInt("##height", &resolution_.y_, 0);
        ui::PopItemWidth();
    }
    else
    {
        ui::Text("Resolution: %dx%d", resolution_.x_, resolution_.y_);
    }

    resolution_ = VectorClamp(resolution_, IntVector2::ONE, IntVector2{8192, 8192});

    ui::Separator();

    ui::Checkbox("Keep This Window Open", &keepPopupOpen_);

    ui::Separator();

    if (ui::Button(ICON_FA_CAMERA " Take") || ui::IsKeyPressed(KEY_RETURN) || ui::IsKeyPressed(KEY_RETURN2))
    {
        TakeScreenshotDelayed(scene_, useInSceneCamera_ ? sceneCamera_ : editorCamera_, resolution_);
        if (!keepPopupOpen_)
            ui::CloseCurrentPopup();
    }
    ui::SameLine();
    if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
        ui::CloseCurrentPopup();
}

void SceneScreenshot::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

bool SceneScreenshot::RenderTabContextMenu()
{
    auto hotkeyManager = owner_->GetHotkeyManager();
    if (ui::MenuItem("Take Screenshot...", hotkeyManager->GetHotkeyLabel(Hotkey_TakeScreenshot).c_str()))
        TakeScreenshotWithPopup();
    return true;
}

}
