// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/StringHash.h"

#include <EASTL/string.h>

namespace Urho3D
{

/// Type info.
/// @nobind
class URHO3D_API TypeInfo
{
public:
    /// Construct.
    TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo);
    /// Destruct.
    ~TypeInfo();

    /// Check current type is type of specified type.
    bool IsTypeOf(StringHash type) const;
    /// Check current type is type of specified type.
    bool IsTypeOf(const TypeInfo* typeInfo) const;
    /// Check current type is type of specified class type.
    template <typename T> bool IsTypeOf() const { return IsTypeOf(T::GetTypeInfoStatic()); }

    /// Return type.
    StringHash GetType() const { return type_; }
    /// Return type name.
    const ea::string& GetTypeName() const { return typeName_; }
    /// Return base type info.
    const TypeInfo* GetBaseTypeInfo() const { return baseTypeInfo_; }

private:
    /// Type.
    StringHash type_;
    /// Type name.
    ea::string typeName_;
    /// Base class type info.
    const TypeInfo* baseTypeInfo_;
};

} // namespace Urho3D
