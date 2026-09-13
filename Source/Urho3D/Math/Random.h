// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

namespace Urho3D
{

/// Set the random seed. The default seed is 1.
URHO3D_API void SetRandomSeed(unsigned seed);
/// Return the current random seed.
URHO3D_API unsigned GetRandomSeed();
/// Return a random number between 0-32767. Should operate similarly to MSVC rand().
/// @alias{RandomInt}
URHO3D_API int Rand();
/// Return a standard normal distributed number.
URHO3D_API float RandStandardNormal();

}
