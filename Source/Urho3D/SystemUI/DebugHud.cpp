//
// Copyright (c) 2008-2017 the Urho3D project.
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
    mode_(DEBUGHUD_SHOW_NONE)
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
    RecalculateWindowPositions();
}

void DebugHud::RecalculateWindowPositions()
{
    posMode_ = WithinExtents({ui::GetStyle().WindowPadding.x, -ui::GetStyle().WindowPadding.y - 10});
    posStats_ = WithinExtents({ui::GetStyle().WindowPadding.x, ui::GetStyle().WindowPadding.y});
}

void DebugHud::SetMode(unsigned mode)
{
    mode_ = mode;
}

void DebugHud::CycleMode()
{
    switch (mode_)
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

void DebugHud::Toggle(unsigned mode)
{
    SetMode(GetMode() ^ mode);
}

void DebugHud::ToggleAll()
{
    Toggle(DEBUGHUD_SHOW_ALL);
}

void DebugHud::SetAppStats(const String& label, const Variant& stats)
{
    SetAppStats(label, stats.ToString());
}

void DebugHud::SetAppStats(const String& label, const String& stats)
{
    bool newLabel = !appStats_.Contains(label);
    appStats_[label] = stats;
    if (newLabel)
        appStats_.Sort();
}

bool DebugHud::ResetAppStats(const String& label)
{
    return appStats_.Erase(label);
}

void DebugHud::ClearAppStats()
{
    appStats_.Clear();
}

Vector2 DebugHud::WithinExtents(Vector2 pos)
{
    if (pos.x_ < 0)
        pos.x_ += extents_.right_;
    else if (pos.x_ > 0)
        pos.x_ += extents_.left_;
    else
        pos.x_ = extents_.left_;

    if (pos.y_ < 0)
        pos.y_ += extents_.bottom_;
    else if (pos.y_ > 0)
        pos.y_ += extents_.top_;
    else
        pos.y_ = extents_.top_;

    return pos;
};

void DebugHud::RenderUi(VariantMap& eventData)
{
    Renderer* renderer = GetSubsystem<Renderer>();
    Graphics* graphics = GetSubsystem<Graphics>();

    ui::SetNextWindowPos({0, 0});
    ui::SetNextWindowSize({(float)extents_.Width(), (float)extents_.Height()});
    ui::PushStyleColor(ImGuiCol_WindowBg, 0);
    if (ui::Begin("DebugHud mode", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoTitleBar|
                                            ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoInputs))
    {
        if (mode_ & DEBUGHUD_SHOW_MODE)
        {
            ui::SetCursorPos({posMode_.x_, posMode_.y_});
            ui::Text("Tex:%s Mat:%s Spec:%s Shadows:%s Size:%i Quality:%s Occlusion:%s Instancing:%s API:%s",
                     qualityTexts[renderer->GetTextureQuality()],
                     qualityTexts[renderer->GetMaterialQuality()],
                     renderer->GetSpecularLighting() ? "On" : "Off",
                     renderer->GetDrawShadows() ? "On" : "Off",
                     renderer->GetShadowMapSize(),
                     shadowQualityTexts[renderer->GetShadowQuality()],
                     renderer->GetMaxOccluderTriangles() > 0 ? "On" : "Off",
                     renderer->GetDynamicInstancing() ? "On" : "Off",
                     graphics->GetApiName().CString());
        }

        if (mode_ & DEBUGHUD_SHOW_STATS)
        {

            String stats;
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

            ui::SetCursorPos({posStats_.x_, posStats_.y_});

			String updateFpsString;
			if (!GSS<Engine>()->GetUpdateIsLimited())
				updateFpsString = "Update FPS %f";
			else
				updateFpsString = "Update FPS (Limited) %f";

			String renderFpsString;
			if (!GSS<Engine>()->GetRenderIsLimited())
				renderFpsString = "Render FPS %f";
			else
				renderFpsString = "Render FPS (Limited) %f";


            ui::Text(updateFpsString.CString(), (1000.0f / GSS<Engine>()->GetAverageUpdateTimeMs()));
			ui::Text(renderFpsString.CString(), (1000.0f / GSS<Engine>()->GetAverageRenderTimeMs()));
            ui::Text("Triangles %u", primitives);
            ui::Text("Batches %u", batches);
            ui::Text("Views %u", renderer->GetNumViews());
            ui::Text("Lights %u", renderer->GetNumLights(true));
            ui::Text("Shadowmaps %u", renderer->GetNumShadowMaps(true));
            ui::Text("Occluders %u", renderer->GetNumOccluders(true));

            for (HashMap<String, String>::ConstIterator i = appStats_.Begin(); i != appStats_.End(); ++i)
                ui::Text("%s %s", i->first_.CString(), i->second_.CString());
        }
    }
    ui::End();
    ui::PopStyleColor();
}

}
