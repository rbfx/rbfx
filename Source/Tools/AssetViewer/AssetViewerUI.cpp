// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "AssetViewer.h"

namespace Urho3D
{

using namespace AssetViewerDetail;

namespace
{

// Text colors of the viewer UI.
const ImVec4 ErrorColor(1.0f, 0.35f, 0.25f, 1.0f);

/// Return the item width that fills the row and leaves room for the label at the right.
float FillWidthWithLabel(const char* label)
{
    return -(ui::CalcTextSize(label).x + ui::GetStyle().ItemInnerSpacing.x);
}

} // namespace

void AssetViewer::SetParticlesEmitting(bool emitting)
{
    particlesEmitting_ = emitting;
    for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
    {
        if (emitter)
            emitter->SetEmitting(emitting);
    }
    for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
    {
        if (emitter)
            emitter->SetEmitting(emitting);
    }
    for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
    {
        if (emitter)
            emitter->SetEmitting(emitting);
    }
}

void AssetViewer::RestartParticles()
{
    for (const WeakPtr<ParticleEmitter>& emitter : legacyParticleEmitters_)
    {
        if (emitter)
            emitter->Reset();
    }
    for (const WeakPtr<ParticleGraphEmitter>& emitter : graphParticleEmitters_)
    {
        if (emitter)
            emitter->Reset();
    }
    for (const WeakPtr<ParticleEmitter2D>& emitter : particleEmitters2D_)
    {
        if (emitter)
        {
            emitter->SetEmitting(false);
            emitter->SetEmitting(true);
        }
    }
    particlesEmitting_ = true;
    BeginParticleAutoFrame();
}

void AssetViewer::RenderParticleControls()
{
    const unsigned numEmitters =
        legacyParticleEmitters_.size() + graphParticleEmitters_.size() + particleEmitters2D_.size();
    if (numEmitters == 0)
        return;

    if (ui::Checkbox("Emit particles", &particlesEmitting_))
        SetParticlesEmitting(particlesEmitting_);
    ui::SameLine();
    if (ui::Button("Restart particles"))
        RestartParticles();
    ui::TextDisabled("%u emitter%s: %u legacy, %u graph, %u 2D", numEmitters, numEmitters == 1 ? "" : "s",
        legacyParticleEmitters_.size(), graphParticleEmitters_.size(), particleEmitters2D_.size());
}

void AssetViewer::RenderAnimationControls()
{
    if (animations_.empty())
        return;

    Animation* selected = GetSelectedAnimation();
    const ea::string selectedLabel =
        selected ? GetAnimationLabel(selected) + " [" + GetAnimationSource(selected) + "]" : "Select animation";
    ui::SetNextItemWidth(FillWidthWithLabel("Animation"));
    if (ui::BeginCombo("Animation", selectedLabel.c_str()))
    {
        for (unsigned i = 0; i < animations_.size(); ++i)
        {
            Animation* animation = animations_[i];
            const ea::string label = GetAnimationLabel(animation);
            const ea::string source = GetAnimationSource(animation);
            const ea::string shown = label + " [" + source + "]";
            ui::PushID(static_cast<int>(i));
            if (ui::Selectable(shown.c_str(), selectedAnimation_ == i))
                SetSelectedAnimation(i);
            if (selectedAnimation_ == i)
                ui::SetItemDefaultFocus();
            if (ui::IsItemHovered() && animation)
                ui::SetTooltip("Source: %s\n%s", source.c_str(), animation->GetName().c_str());
            ui::PopID();
        }
        ui::EndCombo();
    }

    if (animationControllers_.empty())
    {
        ui::TextDisabled("No AnimationController in the viewed content.");
        return;
    }

    UpdateAnimationTimeFromController();
    // Play, pause and loop are in the toolbar.
    ui::TextDisabled("%u clip%s", static_cast<unsigned>(animations_.size()), animations_.size() == 1 ? "" : "s");

    if (selected)
    {
        ui::SetNextItemWidth(FillWidthWithLabel("Time"));
        if (ui::SliderFloat("Time", &animationTime_, 0.0f, Max(0.001f, selected->GetLength()), "%.3f s"))
            SetAnimationTime(animationTime_);
    }
    ui::SetNextItemWidth(FillWidthWithLabel("Speed"));
    if (ui::DragFloat("Speed", &animationSpeed_, 0.05f, -10.0f, 10.0f, "%.2fx"))
        SetAnimationSpeed(animationSpeed_);
}

namespace
{

constexpr float StatusBarHeight = 24.0f;
constexpr float SplitterWidth = 6.0f;
constexpr float MinSidePanelWidth = 320.0f;
constexpr float DefaultSidePanelWidth = 420.0f;

/// Return the toolbar height. It is one frame height plus the window padding above and below.
/// The same value sizes the toolbar window and places the viewport and the sidebar.
float GetToolbarHeight()
{
    return ImCeil(ui::GetFrameHeight() + ui::GetStyle().WindowPadding.y * 2.0f);
}

/// Draw a vertical separator between two groups of toolbar items.
void ToolbarSeparator()
{
    ui::SameLine();
    ui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ui::SameLine();
}

const ImGuiWindowFlags FixedWindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;


ea::string GetShortError(const ea::string& error)
{
    constexpr unsigned maxLength = 120;
    ea::string result = error;
    const unsigned lineEnd = result.find('\n');
    if (lineEnd != ea::string::npos)
        result.resize(lineEnd);
    if (result.length() > maxLength)
    {
        result.resize(maxLength);
        result += "...";
    }
    return result;
}

/// Draw a toolbar button. An active button uses the highlighted colour.
bool ToolbarToggleButton(const char* label, const char* tooltip, bool active)
{
    if (active)
    {
        // Use a colour that differs from the hover colour of an inactive button.
        ui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.55f, 0.48f, 1.0f));
        ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.66f, 0.58f, 1.0f));
        ui::PushStyleColor(ImGuiCol_Border, ImVec4(0.75f, 0.95f, 0.90f, 1.0f));
        ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }
    const bool result = ui::Button(label);
    if (active)
    {
        ui::PopStyleVar();
        ui::PopStyleColor(3);
    }
    if (tooltip && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ui::SetTooltip("%s", tooltip);
    return result;
}

} // namespace

void AssetViewer::RenderMainMenu()
{
    if (!ui::BeginMainMenuBar())
        return;

    if (ui::BeginMenu("File"))
    {
        if (ui::MenuItem("Reload", nullptr, false, !sourceFile_.empty() && !loading_))
        {
            const ea::string fileName = sourceFile_;
            LoadAsset(fileName);
        }
        if (ui::MenuItem("Reset View Settings"))
            ResetViewSettings();
        if (ui::MenuItem("Exit", "Esc"))
            engine_->Exit();
        ui::EndMenu();
    }
    if (ui::BeginMenu("View"))
    {
        if (ui::MenuItem("Frame content", "F", false, contentRoot_ != nullptr))
        {
            particleAutoFramePending_ = false;
            FrameContent();
        }
        ui::MenuItem("Free look", nullptr, &freeLookEnabled_, contentRoot_ != nullptr && !is2DPreview_);
        ui::Separator();
        if (ui::BeginMenu("Lighting"))
        {
            for (const LightingMode mode : {LightingMode::Auto, LightingMode::SceneLights, LightingMode::ViewerRig})
            {
                if (ui::MenuItem(ViewerEnvironment::GetLightingModeName(mode), nullptr,
                        environment_.GetLightingMode() == mode))
                    environment_.SetLightingMode(mode);
            }
            ui::EndMenu();
        }
        bool skybox = environment_.GetSkybox();
        if (ui::MenuItem("Skybox", "K", &skybox))
            environment_.SetSkybox(skybox);
        bool grid = environment_.GetGrid();
        if (ui::MenuItem("Grid", "G", &grid))
            environment_.SetGrid(grid);
        bool ground = environment_.GetGround();
        if (ui::MenuItem("Ground", "O", &ground))
            environment_.SetGround(ground);
        bool bounds = environment_.GetBounds();
        if (ui::MenuItem("Bounds", "B", &bounds))
            environment_.SetBounds(bounds);
        bool wireframe = environment_.GetWireframe();
        if (ui::MenuItem("Wireframe", "Z", &wireframe))
            environment_.SetWireframe(wireframe);
        if (ui::BeginMenu("Materials"))
        {
            if (ui::MenuItem("Authored", "M", !neutralMaterials_))
                SetNeutralMaterials(false);
            if (ui::MenuItem("Neutral grey", "M", neutralMaterials_))
                SetNeutralMaterials(true);
            ui::EndMenu();
        }
        ui::EndMenu();
    }
    if (ui::BeginMenu("Help"))
    {
        ui::TextUnformatted("Drop an asset file on the window to open it.");
        ui::Separator();
        ui::TextUnformatted("RMB drag: orbit the camera");
        ui::TextUnformatted("MMB drag: pan the camera");
        ui::TextUnformatted("Mouse wheel: zoom");
        ui::TextUnformatted("Free look: RMB + WASD/QE, Shift boosts");
        ui::TextUnformatted("F: frame the content");
        ui::TextUnformatted("L: lighting, K: skybox, G: grid, O: ground, B: bounds, Z: wireframe");
        ui::TextUnformatted("M: materials, authored or neutral grey");
        ui::TextUnformatted("Drop a second file on a skinned model to add its animations");
        ui::TextUnformatted("Space: play or pause the selected animation");
        ui::TextUnformatted("Esc: exit");
        ui::EndMenu();
    }
    ui::EndMainMenuBar();
}

void AssetViewer::RenderToolbar(float top, float width)
{
    ui::SetNextWindowPos(ImVec2(0.0f, top));
    ui::SetNextWindowSize(ImVec2(width, GetToolbarHeight()));
    // The window size is exact. A window minimum size must not make the strip taller than the layout expects.
    ui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    const ImGuiWindowFlags flags = FixedWindowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ui::Begin("##Toolbar", nullptr, flags))
    {
        // The default window padding and frame height center the row of widgets in the strip.
        ui::BeginDisabled(!contentRoot_);
        if (ToolbarToggleButton("Frame", "Frame the content (F)", false))
        {
            particleAutoFramePending_ = false;
            FrameContent();
        }
        ui::SameLine();
        ui::BeginDisabled(is2DPreview_);
        if (ToolbarToggleButton("Free look", "Free look camera: RMB + WASD/QE", freeLookEnabled_))
        {
            freeLookEnabled_ = !freeLookEnabled_;
        }
        ui::EndDisabled();
        ui::EndDisabled();

        ToolbarSeparator();
        ui::SetNextItemWidth(ui::CalcTextSize("Scene lights").x + ui::GetFrameHeight() + ui::GetStyle().FramePadding.x * 2.0f);
        if (ui::BeginCombo("##Lighting", ViewerEnvironment::GetLightingModeName(environment_.GetLightingMode())))
        {
            for (const LightingMode mode : {LightingMode::Auto, LightingMode::SceneLights, LightingMode::ViewerRig})
            {
                if (ui::Selectable(
                        ViewerEnvironment::GetLightingModeName(mode), environment_.GetLightingMode() == mode))
                    environment_.SetLightingMode(mode);
            }
            ui::EndCombo();
        }
        if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ui::IsPopupOpen("##Lighting"))
            ui::SetTooltip("Lighting mode (L cycles)");

        ToolbarSeparator();
        if (ToolbarToggleButton("Skybox", "Skybox (K)", environment_.GetSkybox()))
            environment_.SetSkybox(!environment_.GetSkybox());
        ui::SameLine();
        if (ToolbarToggleButton("Grid", "Grid (G)", environment_.GetGrid()))
            environment_.SetGrid(!environment_.GetGrid());
        ui::SameLine();
        if (ToolbarToggleButton("Ground", "Ground plane (O)", environment_.GetGround()))
            environment_.SetGround(!environment_.GetGround());
        ui::SameLine();
        if (ToolbarToggleButton("Bounds", "Bounding boxes (B)", environment_.GetBounds()))
            environment_.SetBounds(!environment_.GetBounds());
        ui::SameLine();
        if (ToolbarToggleButton("Wireframe", "Wireframe (Z)", environment_.GetWireframe()))
            environment_.SetWireframe(!environment_.GetWireframe());

        ToolbarSeparator();
        if (ToolbarToggleButton(neutralMaterials_ ? "Materials: Neutral grey" : "Materials: Authored",
                "Materials: authored or neutral grey (M)", neutralMaterials_))
            SetNeutralMaterials(!neutralMaterials_);

        if (lodLevelCount_ > 1)
        {
            ToolbarSeparator();
            const ea::string preview = selectedLod_ < 0 ? ea::string("Auto") : Format("LOD {}", selectedLod_);
            ui::SetNextItemWidth(ui::CalcTextSize("LOD 00").x + ui::GetFrameHeight() + ui::GetStyle().FramePadding.x * 2.0f);
            if (ui::BeginCombo("##Lod", preview.c_str()))
            {
                // Auto uses the distance to the camera. Only the geometry LOD levels of a model support it.
                if (!lodModels_.empty() && ui::Selectable("Auto", selectedLod_ < 0))
                {
                    SetSelectedLod(-1);
                    environment_.Update(GetContentBoundingBox(true));
                }
                for (unsigned level = 0; level < lodLevelCount_; ++level)
                {
                    const ea::string label = Format("LOD {}", level);
                    if (ui::Selectable(label.c_str(), selectedLod_ == static_cast<int>(level)))
                    {
                        SetSelectedLod(static_cast<int>(level));
                        environment_.Update(GetContentBoundingBox(true));
                    }
                }
                ui::EndCombo();
            }
            if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ui::IsPopupOpen("##Lod"))
                ui::SetTooltip("Level of detail to show (LOD 0 is the most detailed)");
        }

        if (!animations_.empty() && !animationControllers_.empty())
        {
            ToolbarSeparator();
            if (ToolbarToggleButton(animationsPlaying_ ? "Pause" : "Play", "Play or pause the selected animation (Space)",
                    animationsPlaying_))
                SetAnimationsPlaying(!animationsPlaying_);
            ui::SameLine();
            if (ToolbarToggleButton("Loop", "Loop the selected animation", animationLooped_))
            {
                animationLooped_ = !animationLooped_;
                UpdateAnimationTimeFromController();
                ApplySelectedAnimation(animationsPlaying_);
            }
        }
    }
    ui::End();
    ui::PopStyleVar(3);
}

void AssetViewer::RenderStatusBar(float top, float width)
{
    ui::SetNextWindowPos(ImVec2(0.0f, top));
    ui::SetNextWindowSize(ImVec2(width, StatusBarHeight));
    // The default window minimum size is larger than the status bar. It would grow the bar over the panels.
    ui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ui::Begin("##StatusBar", nullptr, FixedWindowFlags | ImGuiWindowFlags_NoScrollbar))
    {
        const bool failed = !loading_ && !error_.empty();
        const char* state = loading_ ? "Loading" : failed ? "Failed" : sourceFile_.empty() ? "Idle" : "Loaded";
        const ea::string& shownFile = loading_ ? loadingFile_ : failed && !lastFailedFile_.empty() ? lastFailedFile_ : sourceFile_;
        const ea::string fileName = GetFileNameAndExtension(shownFile);
        ui::Text("%s", fileName.empty() ? "No file" : fileName.c_str());
        ui::SameLine();
        ui::TextDisabled("|");
        ui::SameLine();
        if (failed)
            ui::TextColored(ErrorColor, "%s", state);
        else
            ui::TextUnformatted(state);
        if (failed)
        {
            ui::SameLine();
            ui::TextColored(ErrorColor, "%s", GetShortError(error_).c_str());
        }
        if (!queuedFile_.empty())
        {
            ui::SameLine();
            ui::TextDisabled("| Queued: %s", GetFileNameAndExtension(queuedFile_).c_str());
        }
        if (!failed && !loading_ && !statusMessage_.empty())
        {
            ui::SameLine();
            ui::TextDisabled("|");
            ui::SameLine();
            ui::TextUnformatted(statusMessage_.c_str());
        }
        if (!loading_ && !missingTextures_.empty())
        {
            ui::SameLine();
            ui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "| %u texture%s missing",
                static_cast<unsigned>(missingTextures_.size()),
                missingTextures_.size() == 1 ? "" : "s");
            if (ui::IsItemHovered())
            {
                ea::string list;
                for (const ea::string& name : missingTextures_)
                    list += name + "\n";
                ui::SetTooltip("Missing textures (missing base colour textures are shown magenta):\n%s", list.c_str());
            }
        }

        const char* hint = "F frame  L lighting  M materials  Space play";
        const float hintWidth = ui::CalcTextSize(hint).x;
        ui::SameLine(Max(ui::GetCursorPosX(), width - hintWidth - 12.0f));
        ui::TextDisabled("%s", hint);
    }
    ui::End();
    ui::PopStyleVar(4);
}

void AssetViewer::RenderSplitter(float left, float top, float height, float windowWidth)
{
    // ImGui asserts on a zero-size button. A tiny or minimized window has no room for the splitter.
    if (height < 1.0f || windowWidth < 1.0f)
        return;
    ui::SetNextWindowPos(ImVec2(left, top));
    ui::SetNextWindowSize(ImVec2(SplitterWidth, height));
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // The default minimum window size is wider than the splitter. Without this, the window covers the panel.
    ui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    // The splitter has no background, and it must not move above the panel when it gets focus.
    const ImGuiWindowFlags flags = FixedWindowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    if (ui::Begin("##Splitter", nullptr, flags))
    {
        ui::InvisibleButton("##SplitterDrag", ImVec2(SplitterWidth, height));
        const bool active = ui::IsItemActive();
        const bool hovered = ui::IsItemHovered();
        splitterDragging_ = active;
        if (hovered || active)
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        // Nothing else draws or clears the strip. Draw it every frame, or old highlight pixels stay on the screen.
        const ImGuiCol color = active ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_WindowBg;
        ui::GetWindowDrawList()->AddRectFilled(ui::GetItemRectMin(), ui::GetItemRectMax(), ui::GetColorU32(color));
    }
    ui::End();
    ui::PopStyleVar(3);
}

void AssetViewer::RenderViewport(const ImVec2& position, const ImVec2& size)
{
    Graphics* graphics = GetSubsystem<Graphics>();
    sceneHovered_ = false;
    if (size.x >= 1.0f && size.y >= 1.0f)
    {
        const int left = Clamp(RoundToInt(position.x), 0, Max(0, graphics->GetWidth() - 1));
        const int top = Clamp(RoundToInt(position.y), 0, Max(0, graphics->GetHeight() - 1));
        const int right = Clamp(RoundToInt(position.x + size.x), left + 1, graphics->GetWidth());
        const int bottom = Clamp(RoundToInt(position.y + size.y), top + 1, graphics->GetHeight());
        const IntRect rect(left, top, right, bottom);
        viewport_->SetRect(rect);
        camera_->SetAspectRatio(static_cast<float>(right - left) / (bottom - top));
    }

    ImGuiWindowFlags flags = FixedWindowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (!previewTexture_)
        flags |= ImGuiWindowFlags_NoBackground;
    ui::SetNextWindowPos(position);
    ui::SetNextWindowSize(size);
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    const bool viewportVisible = ui::Begin("##Viewport", nullptr, flags);
    // The viewport is the backmost window. Focus or creation order never lifts it above the panel or a menu.
    ui::BringWindowToDisplayBack(ui::GetCurrentWindow());
    if (viewportVisible && size.x >= 1.0f && size.y >= 1.0f)
    {
        if (previewTexture_)
        {
            const ImVec2 original(
                static_cast<float>(previewTexture_->GetWidth()), static_cast<float>(previewTexture_->GetHeight()));
            Widgets::Image(previewTexture_, Widgets::FitContent(size, original));
        }
        else
        {
            ui::InvisibleButton("##SceneViewport", size,
                ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
                    | ImGuiButtonFlags_MouseButtonMiddle);
            sceneHovered_ = ui::IsItemHovered();
            if (sourceFile_.empty() && !loading_ && error_.empty())
            {
                const char* text = "Drop a model, scene, material or texture here";
                const ImVec2 textSize = ui::CalcTextSize(text);
                const ImVec2 textPos(position.x + (size.x - textSize.x) * 0.5f, position.y + (size.y - textSize.y) * 0.5f);
                ui::GetForegroundDrawList()->AddText(textPos, IM_COL32(200, 200, 200, 220), text);
            }
        }
    }
    ui::End();
    ui::PopStyleVar(3);

    RenderViewportOverlay(position, size);
}

void AssetViewer::RenderViewportOverlay(const ImVec2& position, const ImVec2& size)
{
    if (size.x < 1.0f || size.y < 1.0f)
        return;

    const ImVec2 center(position.x + size.x * 0.5f, position.y + size.y * 0.5f);
    ImDrawList* drawList = ui::GetForegroundDrawList();

    if (loading_)
    {
        drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), IM_COL32(0, 0, 0, 150));

        const float time = static_cast<float>(ui::GetTime());
        const float radius = 16.0f;
        const ImVec2 spinnerCenter(center.x, center.y - 30.0f);
        const float start = time * 6.0f;
        drawList->PathClear();
        drawList->PathArcTo(spinnerCenter, radius, start, start + 4.5f, 24);
        drawList->PathStroke(IM_COL32(230, 230, 230, 255), 0, 3.0f);

        const ea::string name = GetFileNameAndExtension(loadingFile_);
        const unsigned dots = static_cast<unsigned>(time * 3.0f) % 4;
        const ea::string text = Format("Loading {}", name) + ea::string(dots, '.');
        // Center the text with all dots so that the text does not jitter.
        const ImVec2 fullSize = ui::CalcTextSize(Format("Loading {}...", name).c_str());
        drawList->AddText(
            ImVec2(center.x - fullSize.x * 0.5f, center.y + 4.0f), IM_COL32(240, 240, 240, 255), text.c_str());
        if (!queuedFile_.empty())
        {
            const ea::string queued = Format("Queued: {}", GetFileNameAndExtension(queuedFile_));
            const ImVec2 queuedSize = ui::CalcTextSize(queued.c_str());
            drawList->AddText(ImVec2(center.x - queuedSize.x * 0.5f, center.y + 28.0f), IM_COL32(200, 200, 200, 220), queued.c_str());
        }
        return;
    }

    if (error_.empty())
        return;

    const char* hint = "Drop another file to try again";
    const ImVec2 hintSize = ui::CalcTextSize(hint);
    drawList->AddText(ImVec2(center.x - hintSize.x * 0.5f, position.y + size.y - hintSize.y - 24.0f),
        IM_COL32(200, 200, 200, 220), hint);

    // A normal window, not a modal. The mouse still reaches the viewport.
    const float width = Clamp(size.x - 40.0f, 120.0f, 560.0f);
    const float maxHeight = Max(100.0f, size.y - 100.0f);
    ui::SetNextWindowPos(ImVec2(center.x, center.y - 20.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ui::SetNextWindowSize(ImVec2(width, 0.0f));
    ui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, maxHeight));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
    ui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ui::PushStyleColor(ImGuiCol_Border, ErrorColor);
    if (ui::Begin("##LoadError", nullptr, flags))
    {
        const ea::string failedName = GetFileNameAndExtension(lastFailedFile_);
        ui::PushStyleColor(ImGuiCol_Text, ErrorColor);
        ui::TextWrapped("Could not load %s", failedName.empty() ? "file" : failedName.c_str());
        ui::PopStyleColor();
        ui::Separator();
        ui::TextWrapped("%s", error_.c_str());
        if (!lastFailedFile_.empty())
        {
            ui::Spacing();
            ui::PushStyleColor(ImGuiCol_Text, ui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ui::TextWrapped("%s", lastFailedFile_.c_str());
            ui::PopStyleColor();
        }
        ui::Spacing();
        const bool canRetry = !lastFailedFile_.empty();
        if (!canRetry)
            ui::BeginDisabled();
        const bool retry = ui::Button("Retry");
        if (!canRetry)
            ui::EndDisabled();
        ui::SameLine();
        const bool dismiss = ui::Button("Dismiss");
        if (retry)
        {
            const ea::string file = lastFailedFile_;
            LoadAsset(file);
        }
        else if (dismiss)
            DismissError();
    }
    ui::End();
    ui::PopStyleColor();
    ui::PopStyleVar(2);
}

void AssetViewer::RenderInspector()
{
    // The panel has the same top and bottom edge as the 3D viewport. Square corners keep the edges exact.
    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    const bool visible = ui::Begin("Asset Inspector", nullptr, FixedWindowFlags);
    {
        const ImVec2 position = ui::GetWindowPos();
        const ImVec2 size = ui::GetWindowSize();
        const IntRect rect(RoundToInt(position.x), RoundToInt(position.y), RoundToInt(position.x + size.x),
            RoundToInt(position.y + size.y));
    }
    if (visible)
    {
        ui::PushTextWrapPos(0.0f);
        BeginPropertyFrame();

        if (!animations_.empty() && ui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderAnimationControls();
            ui::Spacing();
        }
        const bool hasEmitters =
            !legacyParticleEmitters_.empty() || !graphParticleEmitters_.empty() || !particleEmitters2D_.empty();
        if (hasEmitters && ui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderParticleControls();
            ui::Spacing();
        }

        if (loading_)
        {
            ui::PushStyleColor(ImGuiCol_Text, DimColor);
            ui::TextWrapped("Loading %s...", GetFileNameAndExtension(loadingFile_).c_str());
            ui::PopStyleColor();
            ui::Spacing();
        }
        if (!error_.empty())
        {
            ui::PushStyleColor(ImGuiCol_Text, ErrorColor);
            ui::TextWrapped("%s", error_.c_str());
            ui::PopStyleColor();
            ui::Spacing();
        }
        if (sourceFile_.empty())
        {
            ui::PushStyleColor(ImGuiCol_Text, DimColor);
            ui::TextWrapped("No asset loaded.");
            ui::PopStyleColor();
        }
        else if (ui::BeginTabBar("InspectorTabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ui::BeginTabItem("Overview"))
            {
                ui::Spacing();
                DrawOverview();
                EndProperties();
                ui::EndTabItem();
            }
            if (contentRoot_ && ui::BeginTabItem("Scene"))
            {
                ui::Spacing();
                DrawNode(contentRoot_);
                EndProperties();
                ui::EndTabItem();
            }
            if (ui::BeginTabItem("Resources"))
            {
                ui::Spacing();
                DrawResources();
                EndProperties();
                ui::EndTabItem();
            }
            if (gltfModel_ && ui::BeginTabItem("glTF Source"))
            {
                ui::Spacing();
                DrawGLTF();
                EndProperties();
                ui::EndTabItem();
            }
            if (!sourceJson_.empty() && ui::BeginTabItem("Raw JSON"))
            {
                ui::Spacing();
                ui::InputTextMultiline("##RawJson", &sourceJson_,
                    ImVec2(-FLT_MIN, Max(200.0f, ui::GetContentRegionAvail().y)), ImGuiInputTextFlags_ReadOnly);
                ui::EndTabItem();
            }
            if (!sourceText_.empty() && ui::BeginTabItem("Raw Source"))
            {
                ui::Spacing();
                ui::InputTextMultiline("##RawSource", &sourceText_,
                    ImVec2(-FLT_MIN, Max(200.0f, ui::GetContentRegionAvail().y)), ImGuiInputTextFlags_ReadOnly);
                ui::EndTabItem();
            }
            ui::EndTabBar();
        }

        ui::PopTextWrapPos();
    }
    ui::End();
    ui::PopStyleVar();
}

void AssetViewer::DrawOverviewSummary(unsigned long long fileSize)
{
    unsigned long long triangles = 0;
    unsigned long long vertices = 0;
    unsigned numModels = 0;
    unsigned numMaterials = 0;
    ea::unordered_set<const Model*> visibleModels;
    if (contentRoot_ && !lodNodes_.empty())
    {
        ea::vector<StaticModel*> staticModels;
        contentRoot_->FindComponents<StaticModel>(staticModels, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
        for (StaticModel* staticModel : staticModels)
        {
            if (staticModel->IsEnabledEffective())
                visibleModels.insert(staticModel->GetModel());
        }
    }
    for (const SharedPtr<Resource>& resource : assetResources_)
    {
        if (const auto* model = dynamic_cast<const Model*>(resource.Get()))
        {
            // Skip a model that only hidden LOD nodes use.
            if (!lodNodes_.empty() && visibleModels.find(model) == visibleModels.end())
                continue;
            ++numModels;
            // With Auto, the camera distance selects the level. Count the geometries that the component draws.
            const StaticModel* autoModel = nullptr;
            if (selectedLod_ < 0)
            {
                for (const LodModel& entry : lodModels_)
                {
                    if (entry.model_ && entry.model_->GetModel() == model)
                    {
                        autoModel = entry.model_;
                        break;
                    }
                }
            }
            // Count the shown LOD level of every geometry. A model without the level uses its last level.
            for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
            {
                const unsigned numLevels = model->GetNumGeometryLodLevels(i);
                if (numLevels == 0)
                    continue;
                const Geometry* geometry = autoModel && i < autoModel->GetBatches().size()
                    ? autoModel->GetBatches()[i].geometry_
                    : model->GetGeometry(i, Min(static_cast<unsigned>(Max(selectedLod_, 0)), numLevels - 1));
                if (!geometry)
                    continue;
                vertices += geometry->GetVertexCount();
                const unsigned elements = geometry->GetIndexCount() ? geometry->GetIndexCount() : geometry->GetVertexCount();
                triangles += elements / 3;
            }
        }
        else if (dynamic_cast<const Material*>(resource.Get()))
            ++numMaterials;
    }

    const BoundingBox box = GetContentBoundingBox();
    const ea::string extension = GetExtension(sourceFile_);

    PropertyRow("File", GetFileNameAndExtension(sourceFile_), &SuccessColor);
    PropertyBytes("Size", fileSize);
    PropertyRow("Type", extension.empty() ? ea::string("unknown") : extension.substr(1).to_upper());
    if (numModels > 0)
    {
        Property("Triangles", triangles);
        Property("Vertices", vertices);
    }
    if (IsFiniteBoundingBox(box))
        Property("Bounds size", box.Size());
    if (lodLevelCount_ > 1)
        PropertyRow("LOD", selectedLod_ < 0 ? ea::string("Auto") : Format("LOD {} of {}", selectedLod_, lodLevelCount_));
    Property("Animations", animations_.size());
    Property("Materials", numMaterials);
    EndProperties();
}

void AssetViewer::RenderUI()
{
    RenderMainMenu();

    const ImGuiViewport* mainViewport = ui::GetMainViewport();
    const float windowWidth = mainViewport->Size.x;
    const float windowHeight = mainViewport->Size.y;
    if (windowWidth < 1.0f || windowHeight < 1.0f)
        return;
    // Whole pixel edges keep the viewport rect and the panel rect equal.
    const float menuHeight = ImCeil(ui::GetFrameHeight());
    const float toolbarTop = menuHeight;
    const float contentTop = toolbarTop + GetToolbarHeight();
    const float statusTop = windowHeight - StatusBarHeight;
    const float contentHeight = Max(0.0f, statusTop - contentTop);

    const float maxPanelWidth = Max(MinSidePanelWidth, windowWidth * 0.5f);
    // Apply the drag before the layout. The splitter, the panel and the viewport use the same width in this frame.
    if (splitterDragging_ && ui::IsMouseDown(ImGuiMouseButton_Left))
        sidePanelWidth_ = windowWidth - ui::GetIO().MousePos.x - SplitterWidth * 0.5f;
    else
        splitterDragging_ = false;
    if (!IsNaN(sidePanelWidth_))
        sidePanelWidth_ = Clamp(sidePanelWidth_, MinSidePanelWidth, maxPanelWidth);
    else
        sidePanelWidth_ = DefaultSidePanelWidth;
    sidePanelWidth_ = ImFloor(sidePanelWidth_);
    const float panelLeft = ImFloor(windowWidth) - sidePanelWidth_;
    const float splitterLeft = panelLeft - SplitterWidth;

    RenderToolbar(toolbarTop, windowWidth);
    RenderStatusBar(statusTop, windowWidth);
    RenderSplitter(splitterLeft, contentTop, contentHeight, windowWidth);

    if (contentHeight >= 1.0f)
    {
        ui::SetNextWindowPos(ImVec2(panelLeft, contentTop));
        ui::SetNextWindowSize(ImVec2(sidePanelWidth_, contentHeight));
        RenderInspector();
    }

    RenderViewport(ImVec2(0.0f, contentTop), ImVec2(Max(0.0f, splitterLeft), contentHeight));
}

} // namespace Urho3D
