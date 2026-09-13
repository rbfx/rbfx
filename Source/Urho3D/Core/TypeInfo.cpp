// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Precompiled.h>

#include <Urho3D/Core/TypeInfo.h>

#include <Urho3D/DebugNew.h>

namespace Urho3D
{

TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo)
    : type_(typeName)
    , typeName_(typeName)
    , baseTypeInfo_(baseTypeInfo)
{
}

TypeInfo::~TypeInfo() = default;

bool TypeInfo::IsTypeOf(StringHash type) const
{
    const TypeInfo* current = this;
    while (current)
    {
        if (current->GetType() == type)
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

bool TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
{
    if (typeInfo == nullptr)
        return false;

    const TypeInfo* current = this;
    while (current)
    {
        if (current == typeInfo || current->GetType() == typeInfo->GetType())
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

} // namespace Urho3D
