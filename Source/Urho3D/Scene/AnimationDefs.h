// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{

/// Animation wrap mode.
enum WrapMode
{
    /// Loop mode.
    WM_LOOP = 0,
    /// Play once, when animation finished it will be removed.
    WM_ONCE,
    /// Clamp mode.
    WM_CLAMP,
};

}

