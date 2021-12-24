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

#include "../Core/Format.h"

#include <Urho3D/Urho3D.h>

namespace Urho3D
{

namespace Detail
{

template <class... T> inline ea::string GetAssertMessage(int, ea::string_view format, const T&... args)
{
    return Urho3D::Format(format, args...);
}

inline ea::string GetAssertMessage(int) { return ""; }

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
