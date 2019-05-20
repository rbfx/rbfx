//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <EASTL/sort.h>

#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Engine/Engine.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/GraphicsEvents.h"
#include "../IO/Log.h"
#include "../UI/UI.h"
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/DebugHud.h"

#include "../DebugNew.h"


namespace Urho3D
{

static const char* qualityTexts[] =
{
    "Low",
    "Med",
    "High"
};

static const char* shadowQualityTexts[] =
{
    "16bit Low",
    "24bit Low",
    "16bit High",
    "24bit High"
};

static const unsigned FPS_UPDATE_INTERVAL_MS = 500;

DebugHud::DebugHud(Context* context) :
    Object(context),
    profilerMaxDepth_(M_MAX_UNSIGNED),
    profilerInterval_(1000),
    useRendererStats_(true),
    mode_(DEBUGHUD_SHOW_NONE),
    fps_(0)
{
    SetExtents();
    SubscribeToEvent(E_UPDATE, std::bind(&DebugHud::RenderUi, this, std::placeholders::_2));
}

DebugHud::~DebugHud()
{
    UnsubscribeFromAllEvents();
}

void DebugHud::SetExtents(const IntVector2& position, IntVector2 size)
{
    if (size == IntVector2::ZERO)
    {
        size = {GetGraphics()->GetWidth(), GetGraphics()->GetHeight()};
        if (!HasSubscribedToEvent(E_SCREENMODE))
            SubscribeToEvent(E_SCREENMODE, std::bind(&DebugHud::SetExtents, this, IntVector2::ZERO, IntVector2::ZERO));
    }
    else
        UnsubscribeFromEvent(E_SCREENMODE);

    auto bottomRight = position + size;
    extents_ = IntRect(position.x_, position.y_, bottomRight.x_, bottomRight.y_);
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

void DebugHud::SetUseRendererStats(bool enable)
{
    useRendererStats_ = enable;
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

void DebugHud::RenderUi(VariantMap& eventData)
{
    Renderer* renderer = GetSubsystem<Renderer>();
    Graphics* graphics = GetSubsystem<Graphics>();


    ui::SetNextWindowPos(ToImGui(Vector2(extents_.Min())));
    ui::SetNextWindowSize(ToImGui(Vector2(extents_.Size())));
    ui::PushStyleColor(ImGuiCol_WindowBg, 0);
    if (ui::Begin("DebugHud", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|
                                       ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoScrollbar))
    {
        if (mode_ & DEBUGHUD_SHOW_STATS)
        {
            // Update stats regardless of them being shown.
            if (fpsTimer_.GetMSec(false) > FPS_UPDATE_INTERVAL_MS)
            {
                fps_ = static_cast<unsigned int>(Round(GetTime()->GetFramesPerSecond()));
                fpsTimer_.Reset();
            }

            ea::string stats;
            unsigned primitives, batches;
            if (!useRendererStats_)
            {
                primitives = graphics->GetNumPrimitives();
                batches = graphics->GetNumBatches();
            }
            else
            {
                primitives = GetRenderer()->GetNumPrimitives();
                batches = GetRenderer()->GetNumBatches();
            }

            ui::Text("FPS %d", fps_);
            ui::Text("Triangles %u", primitives);
            ui::Text("Batches %u", batches);
            ui::Text("Views %u", renderer->GetNumViews());
            ui::Text("Lights %u", renderer->GetNumLights(true));
            ui::Text("Shadowmaps %u", renderer->GetNumShadowMaps(true));
            ui::Text("Occluders %u", renderer->GetNumOccluders(true));

            for (auto i = appStats_.begin(); i !=
                appStats_.end(); ++i)
                ui::Text("%s %s", i->first.c_str(), i->second.c_str());
        }

        if (mode_ & DEBUGHUD_SHOW_MODE)
        {
            auto& style = ui::GetStyle();
            ui::SetCursorPos({style.WindowPadding.x, ui::GetWindowSize().y - ui::GetStyle().WindowPadding.y - 10});
            ui::Text("Tex:%s | Mat:%s | Spec:%s | Shadows:%s | Size:%i | Quality:%s | Occlusion:%s | Instancing:%s | API:%s",
                qualityTexts[renderer->GetTextureQuality()],
                qualityTexts[renderer->GetMaterialQuality()],
                renderer->GetSpecularLighting() ? "On" : "Off",
                renderer->GetDrawShadows() ? "On" : "Off",
                renderer->GetShadowMapSize(),
                shadowQualityTexts[renderer->GetShadowQuality()],
                renderer->GetMaxOccluderTriangles() > 0 ? "On" : "Off",
                renderer->GetDynamicInstancing() ? "On" : "Off",
                graphics->GetApiName().c_str());
        }
    }
    ui::End();
    ui::PopStyleColor();
}

}
