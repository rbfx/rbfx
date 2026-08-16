// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptBlueprintInterop.h"

namespace Urho3D
{

namespace
{

BlueprintPin MakePin(const ea::string& name, BlueprintPinKind kind, BlueprintDataType type)
{
    BlueprintPin pin;
    pin.name = name;
    pin.displayName = name;
    pin.kind = kind;
    pin.dataType = type;
    pin.required = kind == BlueprintPinKind::Input || kind == BlueprintPinKind::ExecutionInput;
    return pin;
}

const RbScriptCompiledFunction* FindFunction(const RbScriptChunk& chunk, const ea::string& name)
{
    for (const RbScriptCompiledFunction& function : chunk.functions)
    {
        if (function.name == name || function.scriptName + "::" + function.name == name)
            return &function;
    }
    return nullptr;
}

} // namespace

BlueprintDataType RbScriptBlueprintInterop::ToBlueprintDataType(const RbScriptType& type)
{
    switch (type.kind)
    {
    case RbScriptTypeKind::Bool: return BlueprintDataType::Bool;
    case RbScriptTypeKind::Int:
    case RbScriptTypeKind::UInt: return BlueprintDataType::Int;
    case RbScriptTypeKind::Float: return BlueprintDataType::Float;
    case RbScriptTypeKind::Double: return BlueprintDataType::Double;
    case RbScriptTypeKind::String: return BlueprintDataType::String;
    case RbScriptTypeKind::Vector2: return BlueprintDataType::Vector2;
    case RbScriptTypeKind::Vector3: return BlueprintDataType::Vector3;
    case RbScriptTypeKind::Quaternion: return BlueprintDataType::Quaternion;
    case RbScriptTypeKind::Color: return BlueprintDataType::Color;
    case RbScriptTypeKind::Array: return BlueprintDataType::Array;
    case RbScriptTypeKind::Map: return BlueprintDataType::Map;
    case RbScriptTypeKind::Node:
    case RbScriptTypeKind::Component:
    case RbScriptTypeKind::Resource:
    case RbScriptTypeKind::User: return BlueprintDataType::Object;
    default: return BlueprintDataType::Variant;
    }
}

RbScriptValue RbScriptBlueprintInterop::FromVariant(const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_BOOL: return RbScriptValue::FromBoolean(value.GetBool());
    case VAR_INT: return RbScriptValue::FromInteger(value.GetInt());
    case VAR_INT64: return RbScriptValue::FromInteger(value.GetInt64());
    case VAR_FLOAT: return RbScriptValue::FromFloat(value.GetFloat());
    case VAR_DOUBLE: return RbScriptValue::FromFloat(value.GetDouble());
    case VAR_STRING: return RbScriptValue::FromString(value.GetString());
    case VAR_VECTOR2: return RbScriptValue::FromVector2(value.GetVector2());
    case VAR_VECTOR3: return RbScriptValue::FromVector3(value.GetVector3());
    case VAR_QUATERNION: return RbScriptValue::FromQuaternion(value.GetQuaternion());
    case VAR_COLOR: return RbScriptValue::FromColor(value.GetColor());
    case VAR_PTR:
    case VAR_VOIDPTR: return RbScriptValue::FromPointer(value.GetPtr());
    case VAR_VARIANTVECTOR:
    {
        ea::vector<RbScriptValue> values;
        values.reserve(value.GetVariantVector().size());
        for (const Variant& item : value.GetVariantVector())
            values.push_back(FromVariant(item));
        return RbScriptValue::FromArray(values);
    }
    case VAR_STRINGVARIANTMAP:
    {
        ea::unordered_map<ea::string, RbScriptValue> values;
        for (const auto& item : value.GetStringVariantMap())
            values[item.first] = FromVariant(item.second);
        return RbScriptValue::FromMap(values);
    }
    default: return RbScriptValue::Null();
    }
}

Variant RbScriptBlueprintInterop::ToVariant(const RbScriptValue& value)
{
    switch (value.kind)
    {
    case RbScriptValueKind::Boolean: return Variant(value.booleanValue);
    case RbScriptValueKind::Integer: return Variant(static_cast<int>(value.integerValue));
    case RbScriptValueKind::Float: return Variant(static_cast<float>(value.floatValue));
    case RbScriptValueKind::String: return Variant(value.stringValue);
    case RbScriptValueKind::Vector2: return Variant(value.vector2Value);
    case RbScriptValueKind::Vector3: return Variant(value.vector3Value);
    case RbScriptValueKind::Quaternion: return Variant(value.quaternionValue);
    case RbScriptValueKind::Color: return Variant(value.colorValue);
    case RbScriptValueKind::Array:
    {
        VariantVector values;
        if (value.arrayValue)
        {
            values.reserve(value.arrayValue->values.size());
            for (const RbScriptValue& item : value.arrayValue->values)
                values.push_back(ToVariant(item));
        }
        return Variant(values);
    }
    case RbScriptValueKind::Map:
    {
        StringVariantMap values;
        if (value.mapValue)
        {
            for (const auto& item : value.mapValue->values)
                values[item.first] = ToVariant(item.second);
        }
        return Variant(values);
    }
    default: return Variant();
    }
}

bool RbScriptBlueprintInterop::Invoke(const RbScriptChunk& chunk, RbScriptVM& vm, const ea::string& functionName,
    const StringVariantMap& inputs, StringVariantMap& outputs)
{
    const RbScriptCompiledFunction* function = FindFunction(chunk, functionName);
    if (!function || inputs.size() != function->parameterCount || function->parameterNames.size() != function->parameterCount)
        return false;

    ea::vector<RbScriptValue> arguments;
    arguments.reserve(function->parameterCount);
    for (const ea::string& parameterName : function->parameterNames)
    {
        const auto input = inputs.find(parameterName);
        if (input == inputs.end())
            return false;
        arguments.push_back(FromVariant(input->second));
    }

    if (!vm.ExecuteFunction(chunk, functionName, arguments))
        return false;

    outputs.clear();
    if (function->returnType != "void")
        outputs["result"] = ToVariant(vm.GetResult());
    return true;
}

unsigned RbScriptBlueprintInterop::RegisterFunctionNodes(BlueprintRuntime& runtime, const RbScriptChunk& chunk, RbScriptVM& vm)
{
    unsigned count = 0;
    for (const RbScriptCompiledFunction& function : chunk.functions)
    {
        if (!function.blueprintCallable)
            continue;

        BlueprintNodeDefinition definition;
        definition.typeName = "Function.RbScript." + function.scriptName + "::" + function.name;
        definition.category = "Functions/RbScript";
        definition.description = "Call the rbscript function " + function.scriptName + "::" + function.name + ".";
        definition.executionMode = BlueprintExecutionMode::Immediate;
        definition.pins.push_back(MakePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard));
        definition.pins.push_back(MakePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard));
        for (unsigned i = 0; i < function.parameterNames.size(); ++i)
        {
            const RbScriptType type = i < function.parameterTypes.size() ? function.parameterTypes[i] : RbScriptType{};
            definition.pins.push_back(MakePin(function.parameterNames[i], BlueprintPinKind::Input,
                ToBlueprintDataType(type)));
        }
        if (function.returnType != "void")
        {
            RbScriptType returnType;
            returnType.name = function.returnType;
            if (function.returnType == "bool") returnType.kind = RbScriptTypeKind::Bool;
            else if (function.returnType == "i32" || function.returnType == "u32") returnType.kind = RbScriptTypeKind::Int;
            else if (function.returnType == "f32") returnType.kind = RbScriptTypeKind::Float;
            else if (function.returnType == "f64") returnType.kind = RbScriptTypeKind::Double;
            else if (function.returnType == "String") returnType.kind = RbScriptTypeKind::String;
            else if (function.returnType == "Vector2") returnType.kind = RbScriptTypeKind::Vector2;
            else if (function.returnType == "Vector3") returnType.kind = RbScriptTypeKind::Vector3;
            else if (function.returnType == "Quaternion") returnType.kind = RbScriptTypeKind::Quaternion;
            else if (function.returnType == "Color") returnType.kind = RbScriptTypeKind::Color;
            else returnType.kind = RbScriptTypeKind::Variant;
            definition.pins.push_back(MakePin("result", BlueprintPinKind::Output, ToBlueprintDataType(returnType)));
        }

        const ea::string functionName = function.scriptName + "::" + function.name;
        definition.execute = [&chunk, &vm, functionName](BlueprintExecutionContext& context)
        {
            StringVariantMap inputs;
            for (const BlueprintPin& pin : context.GetNode().pins)
            {
                if (pin.kind == BlueprintPinKind::Input)
                    inputs[pin.name] = context.GetInput(pin.name);
            }
            StringVariantMap outputs;
            if (RbScriptBlueprintInterop::Invoke(chunk, vm, functionName, inputs, outputs))
            {
                for (const auto& output : outputs)
                    context.SetOutput(output.first, output.second);
                context.ContinueWith("then");
            }
            else
                context.ReportError("BP213", "The exported rbscript function invocation failed.");
        };
        if (runtime.GetRegistry().Register(definition))
            ++count;
    }
    return count;
}

void RbScriptBlueprintInterop::BindRuntime(BlueprintRuntime& runtime, const RbScriptChunk& chunk, RbScriptVM& vm)
{
    runtime.SetRbScriptInvoker([&chunk, &vm](const ea::string& functionName, const StringVariantMap& inputs,
        StringVariantMap& outputs)
    {
        return RbScriptBlueprintInterop::Invoke(chunk, vm, functionName, inputs, outputs);
    });
}

void RbScriptBlueprintInterop::BindBlueprintCalls(RbScriptBindings& bindings, BlueprintRuntime& runtime,
    const BlueprintGraph& graph)
{
    bindings.SetBlueprintCallHandler([&runtime, &graph](const ea::string& functionName,
        const ea::vector<RbScriptValue>& arguments, RbScriptValue& result)
    {
        const BlueprintFunction* function = graph.GetFunction(functionName);
        if (!function || arguments.size() != function->inputs.size())
            return false;

        StringVariantMap inputs;
        for (unsigned i = 0; i < function->inputs.size(); ++i)
            inputs[function->inputs[i].name] = RbScriptBlueprintInterop::ToVariant(arguments[i]);

        StringVariantMap outputs;
        if (!runtime.CallFunction(graph, functionName, inputs, &outputs))
            return false;
        if (!function->outputs.empty())
        {
            const auto output = outputs.find(function->outputs.front().name);
            if (output != outputs.end())
                result = RbScriptBlueprintInterop::FromVariant(output->second);
            else
                result = RbScriptValue::Null();
        }
        else
            result = RbScriptValue::Null();
        return true;
    });
}

} // namespace Urho3D
