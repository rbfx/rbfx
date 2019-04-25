//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#pragma once

#include "../Container/FlagSet.h"
#include "../Core/Object.h"
#include "../Core/Timer.h"

namespace Urho3D
{

enum DebugHudMode : unsigned
{
    DEBUGHUD_SHOW_NONE = 0x0,
    DEBUGHUD_SHOW_STATS = 0x1,
    DEBUGHUD_SHOW_MODE = 0x2,
    DEBUGHUD_SHOW_ALL = 0x7,
};
URHO3D_FLAGSET(DebugHudMode, DebugHudModeFlags);

/// Displays rendering stats and profiling information.
class URHO3D_API DebugHud : public Object
{
    URHO3D_OBJECT(DebugHud, Object)

public:
    /// Construct.
    explicit DebugHud(Context* context);
    /// Destruct.
    ~DebugHud() override;

    /// Set elements to show.
    /// \param mode is a combination of DEBUGHUD_SHOW_* flags.
    void SetMode(DebugHudModeFlags mode);
    /// Cycle through elements
    void CycleMode();
    /// Set whether to show 3D geometry primitive/batch count only. Default false.
    void SetUseRendererStats(bool enable);
    /// Toggle elements.
    /// \param mode is a combination of DEBUGHUD_SHOW_* flags.
    void Toggle(DebugHudModeFlags mode);
    /// Toggle all elements.
    void ToggleAll();
    /// Return currently shown elements.
    /// \return a combination of DEBUGHUD_SHOW_* flags.
    DebugHudModeFlags GetMode() const { return mode_; }
    /// Return whether showing 3D geometry primitive/batch count only.
    bool GetUseRendererStats() const { return useRendererStats_; }
    /// Set application-specific stats.
    /// \param label a title of stat to be displayed.
    /// \param stats a variant value to be displayed next to the specified label.
    void SetAppStats(const stl::string& label, const Variant& stats);
    /// Set application-specific stats.
    /// \param label a title of stat to be displayed.
    /// \param stats a string value to be displayed next to the specified label.
    void SetAppStats(const stl::string& label, const stl::string& stats);
    /// Reset application-specific stats. Return true if it was erased successfully.
    /// \param label a title of stat to be reset.
    bool ResetAppStats(const stl::string& label);
    /// Clear all application-specific stats.
    void ClearAppStats();
    /// Limit rendering area of debug hud.
    /// \param position of debug hud from top-left corner of the screen.
    /// \param size specifies size of debug hud rect. Pass zero vector to occupy entire screen and automatically resize
    /// debug hud to the size of the screen later. Calling this method with non-zero size parameter requires user to manually resize debug hud on screen size changes later.
    void SetExtents(const IntVector2& position = IntVector2::ZERO, IntVector2 size = IntVector2::ZERO);

private:
    /// Render system ui.
    void RenderUi(VariantMap& eventData);

    /// Hashmap containing application specific stats.
    HashMap<stl::string, stl::string> appStats_;
    /// Profiler max block depth.
    unsigned profilerMaxDepth_;
    /// Profiler accumulation interval.
    unsigned profilerInterval_;
    /// Show 3D geometry primitive/batch count flag.
    bool useRendererStats_;
    /// Current shown-element mode.
    DebugHudModeFlags mode_;
    /// FPS timer.
    Timer fpsTimer_;
    /// Calculated fps
    unsigned fps_;
    /// DebugHud extents that data will be rendered in.
    IntRect extents_;
};

}
