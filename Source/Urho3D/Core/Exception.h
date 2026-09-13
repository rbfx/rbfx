// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Format.h"

#include "Urho3D/Urho3D.h"

#include <EASTL/string.h>

#include <exception>

namespace Urho3D
{

/// Generic runtime exception adapted for usage in Urho.
/// Note that this exception shouldn't leak into main loop of the engine and should only be used internally.
class URHO3D_API RuntimeException : public std::exception
{
public:
    /// Construct exception with static message.
    explicit RuntimeException(ea::string_view message) : message_(message) {}
    /// Construct exception with formatted message.
    template <class T, class ... Ts>
    RuntimeException(ea::string_view format, const T& firstArg, const Ts& ... otherArgs)
    {
        try
        {
            message_ = Format(format, firstArg, otherArgs...);
        }
        catch(const std::exception& e)
        {
            message_ = "Failed to format RuntimeException: ";
            message_ += e.what();
        }
    }
    /// Return message.
    const ea::string& GetMessage() const { return message_; }

    const char* what() const noexcept override { return message_.c_str(); }

private:
    ea::string message_;
};

/// Exception thrown on I/O error on Archive serialization/deserialization.
/// Try to catch this exception outside of serialization code and don't leak it to user code.
/// Archive is generally not safe to use if ArchiveException has been thrown.
class URHO3D_API ArchiveException : public RuntimeException
{
public:
    using RuntimeException::RuntimeException;
};

}
