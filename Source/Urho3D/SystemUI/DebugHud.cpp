//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../Precompiled.h"

#include "../SystemUI/DebugHud.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Engine/Engine.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Renderer.h"
#include "../IO/Log.h"
#include "../RenderAPI/RenderDevice.h"
#include "../SystemUI/SystemUI.h"
#include "../UI/UI.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

static const char* qualityTexts[] =
{
    "Low",
    "Med",
    "High"
};

static const char* filterModeTexts[] = {
    "Nearest",
    "Bilinear",
    "Trilinear",
    "Anisotropic",
};

static const unsigned FPS_UPDATE_INTERVAL_MS = 500;

DebugHud::DebugHud(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_ENDALLVIEWSRENDER, URHO3D_HANDLER(DebugHud, OnRenderDebugUI));
}

DebugHud::~DebugHud()
{
    UnsubscribeFromAllEvents();
}

void DebugHud::SetMode(DebugHudModeFlags mode)
{
    mode_ = mode;
}

void DebugHud::CycleMode()
{
    switch (mode_.AsInteger())
    {
    case DEBUGHUD_SHOW_NONE:
        SetMode(DEBUGHUD_SHOW_STATS);
        break;
    case DEBUGHUD_SHOW_STATS:
        SetMode(DEBUGHUD_SHOW_MODE);
        break;
    case DEBUGHUD_SHOW_MODE:
        SetMode(DEBUGHUD_SHOW_ALL);
        break;
    case DEBUGHUD_SHOW_ALL:
    default:
        SetMode(DEBUGHUD_SHOW_NONE);
        break;
    }
}

void DebugHud::Toggle(DebugHudModeFlags mode)
{
    SetMode(GetMode() ^ mode);
}

void DebugHud::ToggleAll()
{
    Toggle(DEBUGHUD_SHOW_ALL);
}

void DebugHud::SetAppStats(const ea::string& label, const Variant& stats)
{
    SetAppStats(label, stats.ToString());
}

void DebugHud::SetAppStats(const ea::string& label, const ea::string& stats)
{
    appStats_[label] = stats;
}

bool DebugHud::ResetAppStats(const ea::string& label)
{
    return appStats_.erase(label);
}

void DebugHud::ClearAppStats()
{
    appStats_.clear();
}

void DebugHud::RenderUI(DebugHudModeFlags mode)
{
    if (mode == DEBUGHUD_SHOW_NONE)
        return;

    auto renderer = GetSubsystem<Renderer>();
    auto graphics = GetSubsystem<Graphics>();
    auto renderDevice = GetSubsystem<RenderDevice>();

    if (mode & DEBUGHUD_SHOW_STATS)
    {
        const FrameStatistics& stats = renderer->GetFrameStats();

        if (fpsTimer_.GetMSec(false) > FPS_UPDATE_INTERVAL_MS)
        {
            fps_ = static_cast<unsigned int>(Round(context_->GetSubsystem<Time>()->GetFramesPerSecond()));
            ea::swap(numChangedAnimations_[0], numChangedAnimations_[1]);
            numChangedAnimations_[1] = 0;
            fpsTimer_.Reset();
        }

        numChangedAnimations_[1] += stats.changedAnimations_;

        float left_offset = ui::GetCursorPos().x;

        ui::Text("FPS %d", fps_);
        ui::SetCursorPosX(left_offset);
        ui::Text("Triangles %u", renderDevice->GetMaxStats().numPrimitives_);
        ui::SetCursorPosX(left_offset);
        ui::Text("Draws %u", renderDevice->GetMaxStats().numDraws_);
        ui::SetCursorPosX(left_offset);
        ui::Text("Dispatches %u", renderDevice->GetMaxStats().numDispatches_);
        ui::SetCursorPosX(left_offset);
        ui::Text("Views %u", renderer->GetNumViews());
        ui::SetCursorPosX(left_offset);
        ui::Text("Lights %u", renderer->GetNumLights());
        ui::SetCursorPosX(left_offset);
        ui::Text("Shadowmaps %u", renderer->GetNumShadowMaps());
        ui::SetCursorPosX(left_offset);
        ui::Text("Occluders %u", renderer->GetNumOccluders());
        ui::SetCursorPosX(left_offset);
        ui::Text("Animations %u(%u)", stats.animations_, numChangedAnimations_[0]);
        ui::SetCursorPosX(left_offset);

        for (auto i = appStats_.begin(); i != appStats_.end(); ++i)
        {
            ui::Text("%s %s", i->first.c_str(), i->second.c_str());
            ui::SetCursorPosX(left_offset);
        }
    }

    if (mode & DEBUGHUD_SHOW_MODE)
    {
        // TODO: Add more stats?
        const ImGuiStyle& style = ui::GetStyle();
        const ImGuiContext& g = *ui::GetCurrentContext();
        ui::SetCursorPos({style.WindowPadding.x, ui::GetWindowSize().y - ui::GetStyle().WindowPadding.y - g.Font->FontSize});
        ui::Text("API:%s | Tex:%s | Filter:%s",
            graphics->GetApiName().c_str(),
            qualityTexts[renderer->GetTextureQuality()],
            filterModeTexts[renderer->GetTextureFilterMode()]);
    }
}

void DebugHud::OnRenderDebugUI(StringHash, VariantMap&)
{
    const ImGuiContext& g = *ui::GetCurrentContext();
    if (!g.WithinFrameScope)
        return;

    ImGuiViewport* viewport = ui::GetMainViewport();
    ui::SetNextWindowPos(viewport->Pos);
    ui::SetNextWindowSize(viewport->Size);
    ui::SetNextWindowViewport(viewport->ID);
    ui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ui::PushStyleColor(ImGuiCol_Border, 0);
    unsigned flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar;
    if (ui::Begin("DebugHud", nullptr, flags))
        RenderUI(mode_);
    ui::End();
    ui::PopStyleColor(2);
}

}
