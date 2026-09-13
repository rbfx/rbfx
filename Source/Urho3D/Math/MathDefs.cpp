// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Math/MathDefs.h"

#include "../DebugNew.h"

namespace Urho3D
{

void SinCos(float angle, float& sin, float& cos)
{
    float angleRadians = angle * M_DEGTORAD;
#if defined(HAVE_SINCOSF)
    sincosf(angleRadians, &sin, &cos);
#elif defined(HAVE___SINCOSF)
    __sincosf(angleRadians, &sin, &cos);
#else
    sin = sinf(angleRadians);
    cos = cosf(angleRadians);
#endif
}

}
