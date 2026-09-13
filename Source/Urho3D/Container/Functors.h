// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

namespace Urho3D
{

/// Unary operator that performs static cast on the argument.
template <class T>
struct StaticCaster
{
    template <class U>
    T operator() (U x) const { return static_cast<T>(x); }
};

}
