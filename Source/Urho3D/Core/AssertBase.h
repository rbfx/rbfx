// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

#include <EASTL/string.h>

namespace Urho3D
{

namespace Detail
{

inline ea::string GetAssertMessage(int)
{
    return "";
}

inline ea::string GetAssertMessage(int, ea::string_view message)
{
    return ea::string{message};
}

} // namespace Detail

/// Process assertion failure.
URHO3D_API void AssertFailure(bool isFatal, ea::string_view expression, ea::string_view message,
    ea::string_view file, int line, ea::string_view function);

} // namespace Urho3D

#define URHO3D_ASSERT_IMPL(isFatal, expression, message) \
    (Urho3D::AssertFailure(isFatal, expression, message, __FILE__, __LINE__, __FUNCTION__), false)

#ifdef URHO3D_DEBUG_ASSERT
    /// If expression is false, log error and throw exception. Assert popup is displayed in Debug mode.
    #define URHO3D_ASSERT(expression, ...) \
        ((void)(!(expression) \
            && URHO3D_ASSERT_IMPL(true, #expression, Urho3D::Detail::GetAssertMessage(0, ##__VA_ARGS__))))
    /// If expression is false, log error and continute execution. Assert popup is displayed in Debug mode.
    #define URHO3D_ASSERTLOG(expression, ...) \
        ((void)(!(expression) \
            && URHO3D_ASSERT_IMPL(false, #expression, Urho3D::Detail::GetAssertMessage(0, ##__VA_ARGS__))))
#else
    #define URHO3D_ASSERT(expression, ...) ((void)0)
    #define URHO3D_ASSERTLOG(expression, ...) ((void)0)
#endif
