// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptReflection.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ObjectReflection.h>

namespace Urho3D
{

namespace
{

bool IsSupported(VariantType type)
{
    switch (type)
    {
    case VAR_BOOL:
    case VAR_INT:
    case VAR_INT64:
    case VAR_FLOAT:
    case VAR_DOUBLE:
    case VAR_STRING:
    case VAR_VECTOR2:
    case VAR_VECTOR3:
    case VAR_VECTOR4:
    case VAR_COLOR:
    case VAR_QUATERNION:
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST:
    case VAR_PTR:
    case VAR_VOIDPTR:
        return true;
    default:
        return false;
    }
}

ea::string PropertyFunctionName(const ea::string& typeName, const ea::string& property, bool setter)
{
    return typeName + (setter ? "::set_" : "::get_") + property;
}

} // namespace

RbScriptType RbScriptReflection::MapVariantType(VariantType type)
{
    switch (type)
    {
    case VAR_BOOL: return {RbScriptTypeKind::Bool, "bool"};
    case VAR_INT: return {RbScriptTypeKind::Int, "i32"};
    case VAR_INT64: return {RbScriptTypeKind::Int, "i64"};
    case VAR_FLOAT: return {RbScriptTypeKind::Float, "f32"};
    case VAR_DOUBLE: return {RbScriptTypeKind::Double, "f64"};
    case VAR_STRING: return {RbScriptTypeKind::String, "String"};
    case VAR_VECTOR2: return {RbScriptTypeKind::Vector2, "Vector2"};
    case VAR_VECTOR3: return {RbScriptTypeKind::Vector3, "Vector3"};
    case VAR_VECTOR4: return {RbScriptTypeKind::Variant, "Vector4"};
    case VAR_COLOR: return {RbScriptTypeKind::Color, "Color"};
    case VAR_QUATERNION: return {RbScriptTypeKind::Quaternion, "Quaternion"};
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST: return {RbScriptTypeKind::Resource, "Resource"};
    case VAR_PTR:
    case VAR_VOIDPTR: return {RbScriptTypeKind::Variant, "Variant"};
    default: return {RbScriptTypeKind::Variant, "Variant"};
    }
}

unsigned RbScriptReflection::RegisterObjectReflection(Context* context, RbScriptTypeRegistry& registry)
{
    if (!context)
        return 0;

    unsigned count = 0;
    for (const auto& entry : context->GetObjectReflections())
    {
        const ObjectReflection* reflection = entry.second;
        if (!reflection)
            continue;

        const ea::string typeName = reflection->GetTypeName();
        if (typeName.empty())
            continue;

        RbScriptType objectType;
        objectType.kind = RbScriptTypeKind::User;
        objectType.name = typeName;
        registry.RegisterType(objectType);

        for (const AttributeInfo& attribute : reflection->GetAttributes())
        {
            if (!IsSupported(attribute.type_) || (attribute.mode_ & AM_NOEDIT))
                continue;

            const RbScriptType propertyType = MapVariantType(attribute.type_);
            const ea::string getterName = PropertyFunctionName(typeName, attribute.name_, false);
            RbScriptFunctionSignature getter;
            getter.name = getterName;
            getter.returnType = propertyType;
            registry.RegisterFunction(getter);
            ++count;

            RbScriptFunctionSignature setter;
            setter.name = PropertyFunctionName(typeName, attribute.name_, true);
            setter.returnType = registry.Resolve("void");
            setter.parameterTypes.push_back(propertyType);
            registry.RegisterFunction(setter);
            ++count;
        }
    }
    return count;
}

} // namespace Urho3D
