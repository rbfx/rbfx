// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Math/Vector4.h"

#include "Urho3D/Container/Str.h"

#include <cstdio>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

const Vector4 Vector4::ZERO;
const Vector4 Vector4::ONE(1.0f, 1.0f, 1.0f, 1.0f);

ea::string Vector4::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    snprintf(tempBuffer, CONVERSION_BUFFER_LENGTH, "%g %g %g %g", x_, y_, z_, w_);
    return ea::string(tempBuffer);
}

}
