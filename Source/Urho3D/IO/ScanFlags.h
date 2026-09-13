// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"

namespace Urho3D
{

enum ScanFlag : unsigned char
{
    SCAN_FILES = 0x1,
    SCAN_DIRS = 0x2,
    SCAN_HIDDEN = 0x4,
    SCAN_APPEND = 0x8,
    SCAN_RECURSIVE = 0x10,
};
URHO3D_FLAGSET(ScanFlag, ScanFlags);

/// Alias for type used for file times.
/// TODO(editor): Make 64 bit?
using FileTime = unsigned;

}
