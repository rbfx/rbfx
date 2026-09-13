// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/ArchiveSerializationBasic.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/Core/Variant.h"

namespace Urho3D
{

/// Serialize type of the Variant.
inline void SerializeValue(Archive& archive, const char* name, VariantType& value)
{
    SerializeEnum(archive, name, value, Variant::GetTypeNameList());
}

/// Serialize value of the Variant.
URHO3D_API void SerializeVariantAsType(Archive& archive, const char* name, Variant& value, VariantType variantType);

/// Serialize Variant in existing block.
inline void SerializeVariantInBlock(Archive& archive, Variant& value)
{
    VariantType variantType = value.GetType();
    SerializeValue(archive, "type", variantType);
    SerializeVariantAsType(archive, "value", value, variantType);
}

/// Serialize Variant.
inline void SerializeValue(Archive& archive, const char* name, Variant& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    SerializeVariantInBlock(archive, value);
}

/// Serialize variant types.
/// @{
URHO3D_API void SerializeValue(Archive& archive, const char* name, StringVector& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, VariantVector& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, VariantMap& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, StringVariantMap& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, ResourceRef& value);
URHO3D_API void SerializeValue(Archive& archive, const char* name, ResourceRefList& value);
/// @}

}
