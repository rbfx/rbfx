// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#ifdef URHO3D_HASH_DEBUG
    #include "Urho3D/Core/StringHashRegister.h"
#endif
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/MathDefs.h"
#include "Urho3D/Math/StringHash.h"

#include <cstdio>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

#ifdef URHO3D_HASH_DEBUG

// Expose map to let Visual Studio debugger access it if Urho3D is linked statically.
const StringMap* hashReverseMap = nullptr;

// Hide static global variables in functions to ensure initialization order.
static StringHashRegister& GetGlobalStringHashRegister()
{
    static StringHashRegister stringHashRegister(true /*thread safe*/);
    hashReverseMap = &stringHashRegister.GetInternalMap();
    return stringHashRegister;
}

#endif

const StringHash StringHash::Empty{""};

StringHash::StringHash(const ea::string_view& str) noexcept
    : value_(Calculate(str.data(), str.length()))
{
#ifdef URHO3D_HASH_DEBUG
    Urho3D::GetGlobalStringHashRegister().RegisterString(*this, str);
#endif
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

} // namespace Urho3D
