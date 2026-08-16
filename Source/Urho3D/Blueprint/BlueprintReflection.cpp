// SPDX-License-Identifier: MIT

#include "BlueprintReflection.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ObjectReflection.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Scene/Serializable.h>

namespace Urho3D
{

namespace
{

BlueprintPin MakePin(const ea::string& name, BlueprintPinKind kind, BlueprintDataType type,
    const Variant& defaultValue = Variant())
{
    BlueprintPin pin;
    pin.name = name;
    pin.displayName = name;
    pin.kind = kind;
    pin.dataType = type;
    pin.defaultValue = defaultValue;
    pin.required = kind == BlueprintPinKind::Input || kind == BlueprintPinKind::ExecutionInput;
    return pin;
}

ea::string MakeNodeType(const char* operation, const ea::string& typeName, const ea::string& attributeName)
{
    return Format("Reflection.{}.{}.{}", operation, typeName, attributeName);
}

bool IsSupportedAttribute(const AttributeInfo& attribute)
{
    switch (attribute.type_)
    {
    case VAR_INT:
    case VAR_BOOL:
    case VAR_FLOAT:
    case VAR_DOUBLE:
    case VAR_VECTOR2:
    case VAR_VECTOR3:
    case VAR_VECTOR4:
    case VAR_QUATERNION:
    case VAR_COLOR:
    case VAR_STRING:
    case VAR_INT64:
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST:
    case VAR_VARIANTVECTOR:
    case VAR_VARIANTMAP:
    case VAR_STRINGVARIANTMAP:
    case VAR_PTR:
    case VAR_VOIDPTR:
        return true;
    default:
        return false;
    }
}

void RegisterPropertyNode(Context* context, BlueprintNodeRegistry& registry, const ObjectReflection* reflection,
    const AttributeInfo& attribute, bool setter)
{
    const ea::string typeName = reflection->GetTypeName();
    const ea::string nodeType = MakeNodeType(setter ? "SetProperty" : "GetProperty", typeName, attribute.name_);
    BlueprintNodeDefinition definition;
    definition.typeName = nodeType;
    definition.category = Format("Reflection/{}", reflection->GetCategory());
    definition.description = Format("{} the reflected property '{}' on a {}.", setter ? "Set" : "Read", attribute.name_, typeName);
    definition.executionMode = setter ? BlueprintExecutionMode::Immediate : BlueprintExecutionMode::Pure;

    if (setter)
    {
        definition.pins.push_back(MakePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard));
        definition.pins.push_back(MakePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard));
        definition.pins.push_back(MakePin("value", BlueprintPinKind::Input,
            BlueprintReflectionRegistry::MapVariantType(attribute.type_), attribute.defaultValue_));
    }
    else
    {
        definition.pins.push_back(MakePin("value", BlueprintPinKind::Output,
            BlueprintReflectionRegistry::MapVariantType(attribute.type_)));
    }

    const StringHash typeHash = reflection->GetTypeNameHash();
    const ea::string attributeName = attribute.name_;
    definition.execute = [context, typeHash, attributeName, setter](BlueprintExecutionContext& execution)
    {
        Serializable* target = execution.GetTargetObject();
        if (!target || !target->IsInstanceOf(typeHash))
        {
            execution.ReportError("BPREF001", Format("Reflection node requires a target object of type '{}'.", context->GetTypeName(typeHash)));
            return;
        }

        const ea::vector<AttributeInfo>* attributes = target->GetAttributes();
        if (!attributes)
        {
            execution.ReportError("BPREF002", Format("Target object '{}' has no reflected attributes.", target->GetTypeName()));
            return;
        }

        for (const AttributeInfo& reflectedAttribute : *attributes)
        {
            if (reflectedAttribute.name_ != attributeName)
                continue;

            if (setter)
            {
                target->OnSetAttribute(reflectedAttribute, execution.GetInput("value"));
                target->ApplyAttributes();
                execution.ContinueWith("then");
            }
            else
            {
                Variant value;
                target->OnGetAttribute(reflectedAttribute, value);
                execution.SetOutput("value", value);
            }
            return;
        }

        execution.ReportError("BPREF003", Format("Reflected attribute '{}' was not found on '{}'.", attributeName, target->GetTypeName()));
    };

    registry.Register(definition);
}

}

BlueprintDataType BlueprintReflectionRegistry::MapVariantType(VariantType type)
{
    switch (type)
    {
    case VAR_BOOL: return BlueprintDataType::Bool;
    case VAR_INT: return BlueprintDataType::Int;
    case VAR_INT64: return BlueprintDataType::Int64;
    case VAR_FLOAT: return BlueprintDataType::Float;
    case VAR_DOUBLE: return BlueprintDataType::Double;
    case VAR_STRING: return BlueprintDataType::String;
    case VAR_VECTOR2: return BlueprintDataType::Vector2;
    case VAR_VECTOR3: return BlueprintDataType::Vector3;
    case VAR_VECTOR4: return BlueprintDataType::Vector4;
    case VAR_COLOR: return BlueprintDataType::Color;
    case VAR_QUATERNION: return BlueprintDataType::Quaternion;
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST:
    case VAR_PTR:
    case VAR_VOIDPTR: return BlueprintDataType::Object;
    default: return BlueprintDataType::Variant;
    }
}

unsigned BlueprintReflectionRegistry::RegisterNodes(Context* context, BlueprintNodeRegistry& registry)
{
    if (!context)
        return 0;

    unsigned count = 0;
    for (const auto& entry : context->GetObjectReflections())
    {
        const ObjectReflection* reflection = entry.second;
        if (!reflection || reflection->GetCategory().empty())
            continue;

        for (const AttributeInfo& attribute : reflection->GetAttributes())
        {
            if (!IsSupportedAttribute(attribute) || (attribute.mode_ & AM_NOEDIT))
                continue;

            RegisterPropertyNode(context, registry, reflection, attribute, false);
            RegisterPropertyNode(context, registry, reflection, attribute, true);
            count += 2;
        }
    }
    return count;
}

}
