// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

using ApplicationFlavorComponent = ea::unordered_set<ea::string>;
using ApplicationFlavorMap = ea::unordered_map<ea::string, ApplicationFlavorComponent>;

/// Class that represents pattern of flavor components.
struct URHO3D_API ApplicationFlavorPattern
{
    ApplicationFlavorPattern() = default;
    explicit ApplicationFlavorPattern(const ea::string& str);
    ApplicationFlavorPattern(std::initializer_list<ApplicationFlavorMap::value_type> components);

    ApplicationFlavorMap components_;
};

/// Class that represents specific set of flavor components.
struct URHO3D_API ApplicationFlavor
{
    /// Universal flavor matches any pattern.
    static const ApplicationFlavor Universal;
    /// Empty flavor matches only empty patterns.
    static const ApplicationFlavor Empty;
    /// Flavor of current platform.
    /// There's one component "platform" which consists of:
    /// - Platform name (if known): windows|uwp|linux|android|rpi|macos|ios|tvos|web
    /// - Platform type (if known and not web): desktop|mobile|console
    static const ApplicationFlavor Platform;

    ApplicationFlavor() = default;
    explicit ApplicationFlavor(const ea::string& str);
    ApplicationFlavor(std::initializer_list<ApplicationFlavorMap::value_type> components);

    /// Returns distance (smaller is better) if flavor matches the pattern. Returns none if doesn't match.
    ea::optional<unsigned> Matches(const ApplicationFlavorPattern& pattern) const;
    /// Returns string representation of flavor.
    ea::string ToString() const;

    ApplicationFlavorMap components_;
};

}
