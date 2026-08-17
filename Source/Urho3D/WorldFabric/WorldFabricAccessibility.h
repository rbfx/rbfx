// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

enum class URHO3D_API AccessibilityFeature
{
    Subtitles,
    HighContrast,
    ColorBlindDeuteranopia,
    ColorBlindProtanopia,
    ColorBlindTritanopia,
    ReducedMotion,
    ScreenReader
};

struct URHO3D_API AccessibilitySettings
{
    bool subtitles{};
    bool highContrast{};
    bool reducedMotion{};
    bool screenReader{};
    unsigned colorBlindMode{};
    float textScale{1.0f};
};

/// Runtime accessibility preferences shared by UI, gameplay and presentation systems.
class URHO3D_API WorldFabricAccessibility
{
public:
    bool SetFeature(AccessibilityFeature feature, bool enabled);
    bool IsFeatureEnabled(AccessibilityFeature feature) const;
    bool SetTextScale(float scale);
    float GetTextScale() const { return settings_.textScale; }
    const AccessibilitySettings& GetSettings() const { return settings_; }
    bool ApplySettings(const AccessibilitySettings& settings, ea::string* error = nullptr);
    unsigned long long ComputeDigest() const;

private:
    AccessibilitySettings settings_;
};

} // namespace Urho3D
