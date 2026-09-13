// Copyright (c) 2008-2017 the Urho3D project.
// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <EASTL/map.h>

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
    /// Toggle elements.
    /// \param mode is a combination of DEBUGHUD_SHOW_* flags.
    void Toggle(DebugHudModeFlags mode);
    /// Toggle all elements.
    void ToggleAll();
    /// Return currently shown elements.
    /// \return a combination of DEBUGHUD_SHOW_* flags.
    DebugHudModeFlags GetMode() const { return mode_; }
    /// Set application-specific stats.
    /// \param label a title of stat to be displayed.
    /// \param stats a variant value to be displayed next to the specified label.
    void SetAppStats(const ea::string& label, const Variant& stats);
    /// Set application-specific stats.
    /// \param label a title of stat to be displayed.
    /// \param stats a string value to be displayed next to the specified label.
    void SetAppStats(const ea::string& label, const ea::string& stats);
    /// Reset application-specific stats. Return true if it was erased successfully.
    /// \param label a title of stat to be reset.
    bool ResetAppStats(const ea::string& label);
    /// Clear all application-specific stats.
    void ClearAppStats();
    /// Render system ui.
    void RenderUI(DebugHudModeFlags mode);

private:
    /// Render debug hud on to entire viewport.
    void OnRenderDebugUI(StringHash, VariantMap&);

    /// Hashmap containing application specific stats.
    ea::map<ea::string, ea::string> appStats_{};
    /// Current shown-element mode.
    DebugHudModeFlags mode_{DEBUGHUD_SHOW_NONE};
    /// FPS timer.
    Timer fpsTimer_{};
    /// Calculated fps
    unsigned fps_ = 0;
    unsigned numChangedAnimations_[2]{};
};

}
