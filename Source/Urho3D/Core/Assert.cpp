// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Assert.h"

#include "Urho3D/IO/Log.h"

#include <cassert>
#include <stdexcept>

#ifdef URHO3D_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

enum class AssertResult
{
    Abort,
    DebugBreak,
    Ignore
};

AssertResult ShowAssertMessage(const ea::string& assertMessage, bool isFatal)
{
#ifdef URHO3D_PLATFORM_WINDOWS
    const int result = MessageBoxA(nullptr, assertMessage.c_str(), "Assertion Failed",
        MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_TOPMOST | MB_SETFOREGROUND);

    switch (result)
    {
    case IDABORT: return AssertResult::Abort;
    case IDRETRY: return AssertResult::DebugBreak;
    case IDIGNORE: return AssertResult::Ignore;
    default: return AssertResult::Abort;
    }
#else
    assert(0);

    // Fallback for release builds
    return isFatal ? AssertResult::Abort : AssertResult::Ignore;
#endif
}

void TriggerDebugBreak()
{
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
    __builtin_trap();
#endif
}

} // namespace

void AssertFailure(bool isFatal, ea::string_view expression, ea::string_view message, ea::string_view file,
    int line, ea::string_view function)
{
    const ea::string assertMessage =
        Format("Assertion failure!\nExpression:\t{}\nMessage:\t{}\nFile:\t{}\nLine:\t{}\nFunction:\t{}\n", expression,
            message, file, line, function);

    // Always log an error
    URHO3D_LOGERROR(assertMessage);

    // Show standard assert popup whenever possible
    const AssertResult result = ShowAssertMessage(assertMessage, isFatal);
    switch (result)
    {
    case AssertResult::Abort: std::terminate(); break;
    case AssertResult::DebugBreak: TriggerDebugBreak(); break;
    case AssertResult::Ignore: break;
    }
}

} // namespace Urho3D
