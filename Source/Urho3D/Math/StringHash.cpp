//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#ifdef URHO3D_HASH_DEBUG
#include "../Core/StringHashRegister.h"
#endif
#include "../Math/MathDefs.h"
#include "../Math/StringHash.h"
#include "../IO/Log.h"

#include <cstdio>

#include "../DebugNew.h"

namespace Urho3D
{

#ifdef URHO3D_HASH_DEBUG

// Expose map to let Visual Studio debugger access it if Urho3D is linked statically.
const StringMap* hashReverseMap = nullptr;

// Hide static global variables in functions to ensure initialization order.
static StringHashRegister& GetGlobalStringHashRegister()
{
    static StringHashRegister stringHashRegister(true /*thread safe*/ );
    hashReverseMap = &stringHashRegister.GetInternalMap();
    return stringHashRegister;
}

#endif

const StringHash StringHash::ZERO;

#ifdef URHO3D_HASH_DEBUG
StringHash::StringHash(const char* str) noexcept :      // NOLINT(google-explicit-constructor)
    value_(Calculate(str))
{
    GetGlobalStringHashRegister()->RegisterString(*this, str);
}
#endif
StringHash::StringHash(const ea::string& str) noexcept :
    value_(Calculate(str.c_str()))
{
#ifdef URHO3D_HASH_DEBUG
    Urho3D::GetGlobalStringHashRegister().RegisterString(*this, str.c_str());
#endif
}

StringHash::StringHash(const ea::string_view& str) noexcept :
    value_(Calculate(static_cast<const void*>(str.data()), str.length()))
{
#ifdef URHO3D_HASH_DEBUG
    Urho3D::GetGlobalStringHashRegister().RegisterString(*this, str);
#endif
}

#ifdef URHO3D_HASH_DEBUG
unsigned StringHash::Calculate(const char* str, unsigned hash)
{
    if (!str)
        return hash;

    while (*str)
        hash = SDBMHash(hash, (unsigned char)*str++);

    return hash;
}
#endif

unsigned StringHash::Calculate(const void* data, unsigned int length, unsigned int hash)
{
    if (!data)
        return hash;

    auto* bytes = static_cast<const unsigned char*>(data);
    auto* end = bytes + length;
    while (bytes < end)
        hash = SDBMHash(hash, (unsigned char)*bytes++);

    return hash;
}

StringHashRegister* StringHash::GetGlobalStringHashRegister()
{
#ifdef URHO3D_HASH_DEBUG
    return &Urho3D::GetGlobalStringHashRegister();
#else
    return nullptr;
#endif
}

ea::string StringHash::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%08X", value_);
    return ea::string(tempBuffer);
}

ea::string StringHash::ToDebugString() const
{
#ifdef URHO3D_HASH_DEBUG
    return Format("#{} '{}'", ToString(), Reverse());
#else
    return Format("#{}", ToString());
#endif
}

ea::string StringHash::Reverse() const
{
#ifdef URHO3D_HASH_DEBUG
    return Urho3D::GetGlobalStringHashRegister().GetStringCopy(*this);
#else
    return EMPTY_STRING;
#endif
}

}
