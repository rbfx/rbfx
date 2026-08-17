// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "WorldFabricAccessibility.h"

#include <cmath>

namespace Urho3D
{

bool WorldFabricAccessibility::SetFeature(AccessibilityFeature feature, bool enabled)
{
    switch (feature)
    {
    case AccessibilityFeature::Subtitles: settings_.subtitles = enabled; return true;
    case AccessibilityFeature::HighContrast: settings_.highContrast = enabled; return true;
    case AccessibilityFeature::ReducedMotion: settings_.reducedMotion = enabled; return true;
    case AccessibilityFeature::ScreenReader: settings_.screenReader = enabled; return true;
    case AccessibilityFeature::ColorBlindDeuteranopia:
        if (enabled) settings_.colorBlindMode = 1;
        else if (settings_.colorBlindMode == 1) settings_.colorBlindMode = 0;
        return true;
    case AccessibilityFeature::ColorBlindProtanopia:
        if (enabled) settings_.colorBlindMode = 2;
        else if (settings_.colorBlindMode == 2) settings_.colorBlindMode = 0;
        return true;
    case AccessibilityFeature::ColorBlindTritanopia:
        if (enabled) settings_.colorBlindMode = 3;
        else if (settings_.colorBlindMode == 3) settings_.colorBlindMode = 0;
        return true;
    }
    return false;
}

bool WorldFabricAccessibility::IsFeatureEnabled(AccessibilityFeature feature) const
{
    switch (feature)
    {
    case AccessibilityFeature::Subtitles: return settings_.subtitles;
    case AccessibilityFeature::HighContrast: return settings_.highContrast;
    case AccessibilityFeature::ReducedMotion: return settings_.reducedMotion;
    case AccessibilityFeature::ScreenReader: return settings_.screenReader;
    case AccessibilityFeature::ColorBlindDeuteranopia: return settings_.colorBlindMode == 1;
    case AccessibilityFeature::ColorBlindProtanopia: return settings_.colorBlindMode == 2;
    case AccessibilityFeature::ColorBlindTritanopia: return settings_.colorBlindMode == 3;
    }
    return false;
}

bool WorldFabricAccessibility::SetTextScale(float scale)
{
    if (!std::isfinite(scale) || scale < 0.5f || scale > 3.0f)
        return false;
    settings_.textScale = scale;
    return true;
}

bool WorldFabricAccessibility::ApplySettings(const AccessibilitySettings& settings, ea::string* error)
{
    if (!std::isfinite(settings.textScale) || settings.textScale < 0.5f || settings.textScale > 3.0f)
    {
        if (error)
            *error = "Accessibility text scale must be finite and within [0.5, 3.0].";
        return false;
    }
    if (settings.colorBlindMode > 3)
    {
        if (error)
            *error = "Accessibility color blind mode must be 0, 1, 2 or 3.";
        return false;
    }
    settings_ = settings;
    return true;
}

unsigned long long WorldFabricAccessibility::ComputeDigest() const
{
    unsigned long long digest = 1469598103934665603ull;
    const unsigned values[] = {
        settings_.subtitles ? 1u : 0u,
        settings_.highContrast ? 1u : 0u,
        settings_.reducedMotion ? 1u : 0u,
        settings_.screenReader ? 1u : 0u,
        settings_.colorBlindMode,
        static_cast<unsigned>(settings_.textScale * 1000.0f)
    };
    for (unsigned value : values)
    {
        digest ^= value;
        digest *= 1099511628211ull;
    }
    return digest;
}

} // namespace Urho3D
