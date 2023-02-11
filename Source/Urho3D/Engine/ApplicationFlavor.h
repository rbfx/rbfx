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

#pragma once

#include <Urho3D/Urho3D.h>

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
