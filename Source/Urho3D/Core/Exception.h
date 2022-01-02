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
