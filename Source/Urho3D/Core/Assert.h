// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/AssertBase.h"
#include "Urho3D/Core/Format.h"
#include "Urho3D/Urho3D.h"

namespace Urho3D
{

namespace Detail
{

template <class T1, class... T>
inline ea::string GetAssertMessage(int, ea::string_view format, const T1& arg1, const T&... args)
{
    return Urho3D::Format(format, arg1, args...);
}

} // namespace Detail

} // namespace Urho3D
