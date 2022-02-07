//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Container/Ptr.h>

#include <EASTL/optional.h>

#include <catch2/catch_amalgamated.hpp>
#include <ostream>

using namespace Urho3D;

namespace Tests
{

/// Callback used to create context.
using CreateContextCallback = SharedPtr<Context>(*)();

/// Get or create test context.
SharedPtr<Context> GetOrCreateContext(CreateContextCallback callback);

/// Reset test context created by GetOrCreateContext.
void ResetContext();

/// Create test context with all subsystems ready.
SharedPtr<Context> CreateCompleteContext();

/// Run frame with given time step.
void RunFrame(Context* context, float timeStep, float maxTimeStep = M_LARGE_VALUE);

}

/// Convert common types to strings
/// @{
namespace Catch
{

#define DEFINE_STRING_MAKER(type, expr) \
    template<> struct StringMaker<type> \
    { \
        static std::string convert(const type& value) \
        { \
            return expr; \
        } \
    }

DEFINE_STRING_MAKER(Variant, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector2, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector3, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector4, value.ToString().c_str());

#undef DEFINE_STRING_MAKER

template<class T> struct StringMaker<ea::optional<T>>
{
    static std::string convert(const ea::optional<T>& value)
    {
        return value ? StringMaker<T>::convert(*value) : "(nullopt)";
    }
};

}
/// @}
