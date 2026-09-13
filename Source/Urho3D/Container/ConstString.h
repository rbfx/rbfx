// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Math/StringHash.h"

#include <EASTL/string.h>

namespace Urho3D
{

/// Immutable string with precomputed hash.
/// Inherit it from string to allow implicit casts to ea::string w/o allocation.
/// It could be done with cast operator too, but this code would cause crash if `operator const ea::string&()` is used:
/// `const ea::string& s2 = ConstString("text");`
class ConstString : public ea::string
{
public:
    /// Construct
    /// @{
    ConstString() = default;
    ConstString(ea::string_view str)
        : ea::string(str)
        , hash_(c_str())
    {
    }
    /// @}

    /// Return hash
    /// @{
    StringHash GetHash() const { return hash_; }
    operator StringHash() const { return hash_; }
    /// @}

private:
    const StringHash hash_;
};

/// Macro to define global constant.
/// VS 2017 has bug, so it uses less optimal version.
/// https://developercommunity.visualstudio.com/t/static-inline-class-variables-have-their-destructo/300686
#if defined(_MSC_VER) && _MSC_VER <= 1916
    #define URHO3D_GLOBAL_CONSTANT(definition) static const definition
#else
    #define URHO3D_GLOBAL_CONSTANT(definition) static inline const definition
#endif

}
