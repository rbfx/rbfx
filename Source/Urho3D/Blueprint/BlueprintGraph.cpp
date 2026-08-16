// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintGraph.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/JSONFile.h>

namespace Urho3D
{

namespace
{

ea::string LowerString(const ea::string& value)
{
    ea::string result(value);
    for (unsigned i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(ToLower(static_cast<unsigned char>(result[i])));
    return result;
}

void SetError(ea::string* error, const ea::string& message)
{
    if (error)
        *error = message;
}

JSONValue SerializeVariant(const Variant& value)
{
    JSONValue result(JSON_OBJECT);
    result.Set("type", value.GetTypeName());
    result.Set("value", value.GetType() == VAR_NONE ? ea::string() : value.ToString());
    return result;
}

Variant DeserializeVariant(const JSONValue& value)
{
    if (!value.IsObject())
        return Variant();

    const ea::string typeName = value.Contains("type") ? value["type"].GetString() : "none";
    const ea::string text = value.Contains("value") ? value["value"].GetString() : ea::string();
    Variant result;
    result.FromString(typeName, text);
    return result;
}

JSONValue SerializePin(const BlueprintPin& pin)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", pin.name);
    result.Set("displayName", pin.displayName);
    result.Set("kind", ToString(pin.kind));
    result.Set("dataType", ToString(pin.dataType));
    result.Set("default", SerializeVariant(pin.defaultValue));
    result.Set("required", pin.required);
    result.Set("allowMultipleConnections", pin.allowMultipleConnections);
    return result;
}

BlueprintPin DeserializePin(const JSONValue& value)
{
    BlueprintPin pin;
    if (!value.IsObject())
        return pin;

    pin.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    pin.displayName = value.Contains("displayName") ? value["displayName"].GetString() : pin.name;
    pin.kind = value.Contains("kind") ? ParseBlueprintPinKind(value["kind"].GetString()) : BlueprintPinKind::Input;
    pin.dataType = value.Contains("dataType") ? ParseBlueprintDataType(value["dataType"].GetString()) : BlueprintDataType::Variant;
    pin.defaultValue = value.Contains("default") ? DeserializeVariant(value["default"]) : Variant();
    pin.required = value.Contains("required") && value["required"].GetBool();
    pin.allowMultipleConnections = value.Contains("allowMultipleConnections") && value["allowMultipleConnections"].GetBool();
    return pin;
}

JSONValue SerializeStructField(const BlueprintStructField& field)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", field.name);
    result.Set("dataType", ToString(field.dataType));
    result.Set("typeName", field.typeName);
    result.Set("default", SerializeVariant(field.defaultValue));
    return result;
}

BlueprintStructField DeserializeStructField(const JSONValue& value)
{
    BlueprintStructField field;
    if (!value.IsObject())
        return field;
    field.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    field.dataType = value.Contains("dataType") ? ParseBlueprintDataType(value["dataType"].GetString()) : BlueprintDataType::Variant;
    field.typeName = value.Contains("typeName") ? value["typeName"].GetString() : ea::string();
    field.defaultValue = value.Contains("default") ? DeserializeVariant(value["default"]) : Variant();
    return field;
}

JSONValue SerializeStruct(const BlueprintStructDef& structure)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", structure.name);
    result.Set("description", structure.description);
    JSONValue fields(JSON_ARRAY);
    for (const BlueprintStructField& field : structure.fields)
        fields.Push(SerializeStructField(field));
    result.Set("fields", ea::move(fields));
    return result;
}

BlueprintStructDef DeserializeStruct(const JSONValue& value)
{
    BlueprintStructDef structure;
    if (!value.IsObject())
        return structure;
    structure.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    structure.description = value.Contains("description") ? value["description"].GetString() : ea::string();
    if (value.Contains("fields") && value["fields"].IsArray())
    {
        for (const JSONValue& field : value["fields"].GetArray())
            structure.fields.push_back(DeserializeStructField(field));
    }
    return structure;
}

JSONValue SerializeEnum(const BlueprintEnumDef& enumeration)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", enumeration.name);
    result.Set("description", enumeration.description);
    JSONValue values(JSON_ARRAY);
    for (const BlueprintEnumValue& value : enumeration.values)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("name", value.name);
        item.Set("value", value.value);
        values.Push(ea::move(item));
    }
    result.Set("values", ea::move(values));
    return result;
}

BlueprintEnumDef DeserializeEnum(const JSONValue& value)
{
    BlueprintEnumDef enumeration;
    if (!value.IsObject())
        return enumeration;
    enumeration.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    enumeration.description = value.Contains("description") ? value["description"].GetString() : ea::string();
    if (value.Contains("values") && value["values"].IsArray())
    {
        for (const JSONValue& item : value["values"].GetArray())
        {
            BlueprintEnumValue enumValue;
            enumValue.name = item.Contains("name") ? item["name"].GetString() : ea::string();
            enumValue.value = item.Contains("value") ? item["value"].GetInt() : 0;
            enumeration.values.push_back(ea::move(enumValue));
        }
    }
    return enumeration;
}

JSONValue SerializeDelegate(const BlueprintDelegate& delegate)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", delegate.name);
    result.Set("description", delegate.description);
    JSONValue parameters(JSON_ARRAY);
    for (const BlueprintPin& parameter : delegate.parameters)
        parameters.Push(SerializePin(parameter));
    result.Set("parameters", ea::move(parameters));
    return result;
}

BlueprintDelegate DeserializeDelegate(const JSONValue& value)
{
    BlueprintDelegate delegate;
    if (!value.IsObject())
        return delegate;
    delegate.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    delegate.description = value.Contains("description") ? value["description"].GetString() : ea::string();
    if (value.Contains("parameters") && value["parameters"].IsArray())
    {
        for (const JSONValue& item : value["parameters"].GetArray())
            delegate.parameters.push_back(DeserializePin(item));
    }
    return delegate;
}

JSONValue SerializeTimeline(const BlueprintTimeline& timeline)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", timeline.name);
    result.Set("description", timeline.description);
    result.Set("length", timeline.length);
    result.Set("looping", timeline.looping);
    JSONValue keyframes(JSON_ARRAY);
    for (const BlueprintTimelineKeyframe& keyframe : timeline.keyframes)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("time", keyframe.time);
        item.Set("value", SerializeVariant(keyframe.value));
        keyframes.Push(ea::move(item));
    }
    result.Set("keyframes", ea::move(keyframes));
    return result;
}

BlueprintTimeline DeserializeTimeline(const JSONValue& value)
{
    BlueprintTimeline timeline;
    if (!value.IsObject())
        return timeline;
    timeline.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    timeline.description = value.Contains("description") ? value["description"].GetString() : ea::string();
    timeline.length = value.Contains("length") ? value["length"].GetFloat() : 1.0f;
    timeline.looping = value.Contains("looping") && value["looping"].GetBool();
    if (value.Contains("keyframes") && value["keyframes"].IsArray())
    {
        for (const JSONValue& item : value["keyframes"].GetArray())
        {
            BlueprintTimelineKeyframe keyframe;
            keyframe.time = item.Contains("time") ? item["time"].GetFloat() : 0.0f;
            keyframe.value = item.Contains("value") ? DeserializeVariant(item["value"]) : Variant();
            timeline.keyframes.push_back(ea::move(keyframe));
        }
    }
    return timeline;
}

JSONValue SerializeMacro(const BlueprintMacro& macro)
{
    JSONValue result(JSON_OBJECT);
    result.Set("name", macro.name);
    result.Set("description", macro.description);
    result.Set("body", macro.body);
    JSONValue inputs(JSON_ARRAY);
    for (const BlueprintPin& pin : macro.inputs)
        inputs.Push(SerializePin(pin));
    result.Set("inputs", ea::move(inputs));
    JSONValue outputs(JSON_ARRAY);
    for (const BlueprintPin& pin : macro.outputs)
        outputs.Push(SerializePin(pin));
    result.Set("outputs", ea::move(outputs));
    return result;
}

BlueprintMacro DeserializeMacro(const JSONValue& value)
{
    BlueprintMacro macro;
    if (!value.IsObject())
        return macro;
    macro.name = value.Contains("name") ? value["name"].GetString() : ea::string();
    macro.description = value.Contains("description") ? value["description"].GetString() : ea::string();
    macro.body = value.Contains("body") ? value["body"].GetString() : ea::string();
    if (value.Contains("inputs") && value["inputs"].IsArray())
    {
        for (const JSONValue& item : value["inputs"].GetArray())
            macro.inputs.push_back(DeserializePin(item));
    }
    if (value.Contains("outputs") && value["outputs"].IsArray())
    {
        for (const JSONValue& item : value["outputs"].GetArray())
            macro.outputs.push_back(DeserializePin(item));
    }
    return macro;
}

bool IsExecutionPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::ExecutionInput || kind == BlueprintPinKind::ExecutionOutput;
}

bool IsInputPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::Input || kind == BlueprintPinKind::ExecutionInput;
}

bool IsOutputPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::Output || kind == BlueprintPinKind::ExecutionOutput;
}

bool IsNumeric(BlueprintDataType type)
{
    return type == BlueprintDataType::Int || type == BlueprintDataType::Int64 ||
        type == BlueprintDataType::Float || type == BlueprintDataType::Double;
}

}

void BlueprintGraph::Clear()
{
    name_.clear();
    nextNodeId_ = 1;
    nextLinkId_ = 1;
    nodes_.clear();
    links_.clear();
    variables_.clear();
    comments_.clear();
    functions_.clear();
    structs_.clear();
    enums_.clear();
    delegates_.clear();
    timelines_.clear();
    macros_.clear();
    nextCommentId_ = 1;
}

BlueprintId BlueprintGraph::AllocateNodeId()
{
    while (nextNodeId_ == BLUEPRINT_INVALID_ID || GetNode(nextNodeId_))
        ++nextNodeId_;
    return nextNodeId_++;
}

BlueprintId BlueprintGraph::AllocateCommentId()
{
    while (nextCommentId_ == BLUEPRINT_INVALID_ID || GetComment(nextCommentId_))
        ++nextCommentId_;
    return nextCommentId_++;
}

BlueprintId BlueprintGraph::AllocateLinkId()
{
    while (nextLinkId_ == BLUEPRINT_INVALID_ID)
        ++nextLinkId_;
    for (const BlueprintLink& link : links_)
    {
        if (link.id == nextLinkId_)
        {
            ++nextLinkId_;
            return AllocateLinkId();
        }
    }
    return nextLinkId_++;
}

BlueprintId BlueprintGraph::AddNode(const ea::string& typeName, const ea::string& title,
    const Vector2& position, BlueprintExecutionMode executionMode)
{
    BlueprintNode node;
    node.id = AllocateNodeId();
    node.typeName = typeName;
    node.title = title;
    node.position = position;
    node.executionMode = executionMode;
    nodes_.push_back(node);
    return node.id;
}

BlueprintId BlueprintGraph::AddNode(const BlueprintNode& node)
{
    BlueprintNode copy = node;
    if (copy.id == BLUEPRINT_INVALID_ID || GetNode(copy.id))
        copy.id = AllocateNodeId();
    else if (copy.id >= nextNodeId_)
        nextNodeId_ = copy.id + 1;

    nodes_.push_back(ea::move(copy));
    return nodes_.back().id;
}

bool BlueprintGraph::RemoveNode(BlueprintId nodeId)
{
    for (unsigned i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].id == nodeId)
        {
            RemoveLinksForNode(nodeId);
            nodes_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintNode* BlueprintGraph::GetNode(BlueprintId nodeId)
{
    for (BlueprintNode& node : nodes_)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

const BlueprintNode* BlueprintGraph::GetNode(BlueprintId nodeId) const
{
    for (const BlueprintNode& node : nodes_)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

bool BlueprintGraph::AddPin(BlueprintId nodeId, const BlueprintPin& pin)
{
    BlueprintNode* node = GetNode(nodeId);
    if (!node || pin.name.empty() || GetPin(nodeId, pin.name))
        return false;

    node->pins.push_back(pin);
    return true;
}

bool BlueprintGraph::RemovePin(BlueprintId nodeId, const ea::string& pinName)
{
    BlueprintNode* node = GetNode(nodeId);
    if (!node)
        return false;

    for (unsigned i = 0; i < node->pins.size(); ++i)
    {
        if (node->pins[i].name == pinName)
        {
            RemoveLinksForPin(nodeId, pinName);
            node->pins.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintPin* BlueprintGraph::GetPin(BlueprintId nodeId, const ea::string& pinName)
{
    BlueprintNode* node = GetNode(nodeId);
    if (!node)
        return nullptr;

    for (BlueprintPin& pin : node->pins)
    {
        if (pin.name == pinName)
            return &pin;
    }
    return nullptr;
}

const BlueprintPin* BlueprintGraph::GetPin(BlueprintId nodeId, const ea::string& pinName) const
{
    const BlueprintNode* node = GetNode(nodeId);
    if (!node)
        return nullptr;

    for (const BlueprintPin& pin : node->pins)
    {
        if (pin.name == pinName)
            return &pin;
    }
    return nullptr;
}

bool BlueprintGraph::IsPinCompatible(const BlueprintPin& source, const BlueprintPin& target) const
{
    if (!IsOutputPin(source.kind) || !IsInputPin(target.kind))
        return false;
    if (IsExecutionPin(source.kind) || IsExecutionPin(target.kind))
        return source.kind == BlueprintPinKind::ExecutionOutput && target.kind == BlueprintPinKind::ExecutionInput;
    if (source.dataType == BlueprintDataType::Wildcard || target.dataType == BlueprintDataType::Wildcard)
        return true;
    if (source.dataType != target.dataType)
        return IsNumeric(source.dataType) && IsNumeric(target.dataType);
    return true;
}

BlueprintId BlueprintGraph::AddLink(BlueprintId fromNode, const ea::string& fromPin,
    BlueprintId toNode, const ea::string& toPin)
{
    const BlueprintPin* source = GetPin(fromNode, fromPin);
    const BlueprintPin* target = GetPin(toNode, toPin);
    if (!source || !target || !IsPinCompatible(*source, *target))
        return BLUEPRINT_INVALID_ID;

    if (!target->allowMultipleConnections)
    {
        for (const BlueprintLink& link : links_)
        {
            if (link.toNode == toNode && link.toPin == toPin)
                return BLUEPRINT_INVALID_ID;
        }
    }

    BlueprintLink link;
    link.id = AllocateLinkId();
    link.fromNode = fromNode;
    link.fromPin = fromPin;
    link.toNode = toNode;
    link.toPin = toPin;
    links_.push_back(link);
    return link.id;
}

BlueprintId BlueprintGraph::AddLink(const BlueprintLink& link)
{
    return AddLink(link.fromNode, link.fromPin, link.toNode, link.toPin);
}

bool BlueprintGraph::RemoveLink(BlueprintId linkId)
{
    for (unsigned i = 0; i < links_.size(); ++i)
    {
        if (links_[i].id == linkId)
        {
            links_.erase_at(i);
            return true;
        }
    }
    return false;
}

bool BlueprintGraph::AddVariable(const BlueprintVariable& variable)
{
    if (variable.name.empty())
        return false;

    if (BlueprintVariable* existing = GetVariable(variable.name))
    {
        *existing = variable;
        return true;
    }
    variables_.push_back(variable);
    return true;
}

bool BlueprintGraph::RemoveVariable(const ea::string& name)
{
    for (unsigned i = 0; i < variables_.size(); ++i)
    {
        if (variables_[i].name == name)
        {
            variables_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintVariable* BlueprintGraph::GetVariable(const ea::string& name)
{
    for (BlueprintVariable& variable : variables_)
    {
        if (variable.name == name)
            return &variable;
    }
    return nullptr;
}

const BlueprintVariable* BlueprintGraph::GetVariable(const ea::string& name) const
{
    for (const BlueprintVariable& variable : variables_)
    {
        if (variable.name == name)
            return &variable;
    }
    return nullptr;
}

bool BlueprintGraph::AddComment(const BlueprintComment& comment)
{
    if (comment.text.empty())
        return false;
    BlueprintComment copy = comment;
    if (copy.id == BLUEPRINT_INVALID_ID)
        copy.id = AllocateCommentId();
    else if (copy.id >= nextCommentId_)
        nextCommentId_ = copy.id + 1;
    if (BlueprintComment* existing = GetComment(copy.id))
    {
        *existing = copy;
        return true;
    }
    comments_.push_back(copy);
    return true;
}

bool BlueprintGraph::RemoveComment(BlueprintId commentId)
{
    for (unsigned i = 0; i < comments_.size(); ++i)
    {
        if (comments_[i].id == commentId)
        {
            comments_.erase_at(i);
            for (BlueprintNode& node : nodes_)
            {
                if (node.commentId == commentId)
                    node.commentId = BLUEPRINT_INVALID_ID;
            }
            return true;
        }
    }
    return false;
}

BlueprintComment* BlueprintGraph::GetComment(BlueprintId commentId)
{
    for (BlueprintComment& comment : comments_)
    {
        if (comment.id == commentId)
            return &comment;
    }
    return nullptr;
}

const BlueprintComment* BlueprintGraph::GetComment(BlueprintId commentId) const
{
    for (const BlueprintComment& comment : comments_)
    {
        if (comment.id == commentId)
            return &comment;
    }
    return nullptr;
}

bool BlueprintGraph::AddFunction(const BlueprintFunction& function)
{
    if (function.name.empty())
        return false;
    if (BlueprintFunction* existing = GetFunction(function.name))
    {
        *existing = function;
        return true;
    }
    functions_.push_back(function);
    return true;
}

BlueprintFunction* BlueprintGraph::GetFunction(const ea::string& name)
{
    for (BlueprintFunction& function : functions_)
    {
        if (function.name == name)
            return &function;
    }
    return nullptr;
}

const BlueprintFunction* BlueprintGraph::GetFunction(const ea::string& name) const
{
    for (const BlueprintFunction& function : functions_)
    {
        if (function.name == name)
            return &function;
    }
    return nullptr;
}

bool BlueprintGraph::AddStruct(const BlueprintStructDef& structure)
{
    if (structure.name.empty())
        return false;
    for (unsigned i = 0; i < structure.fields.size(); ++i)
    {
        if (structure.fields[i].name.empty())
            return false;
        for (unsigned j = 0; j < i; ++j)
        {
            if (structure.fields[j].name == structure.fields[i].name)
                return false;
        }
    }
    if (BlueprintStructDef* existing = GetStruct(structure.name))
    {
        *existing = structure;
        return true;
    }
    structs_.push_back(structure);
    return true;
}

bool BlueprintGraph::RemoveStruct(const ea::string& name)
{
    for (unsigned i = 0; i < structs_.size(); ++i)
    {
        if (structs_[i].name == name)
        {
            structs_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintStructDef* BlueprintGraph::GetStruct(const ea::string& name)
{
    for (BlueprintStructDef& structure : structs_)
    {
        if (structure.name == name)
            return &structure;
    }
    return nullptr;
}

const BlueprintStructDef* BlueprintGraph::GetStruct(const ea::string& name) const
{
    for (const BlueprintStructDef& structure : structs_)
    {
        if (structure.name == name)
            return &structure;
    }
    return nullptr;
}

bool BlueprintGraph::AddEnum(const BlueprintEnumDef& enumeration)
{
    if (enumeration.name.empty())
        return false;
    for (unsigned i = 0; i < enumeration.values.size(); ++i)
    {
        if (enumeration.values[i].name.empty())
            return false;
        for (unsigned j = 0; j < i; ++j)
        {
            if (enumeration.values[j].name == enumeration.values[i].name)
                return false;
        }
    }
    if (BlueprintEnumDef* existing = GetEnum(enumeration.name))
    {
        *existing = enumeration;
        return true;
    }
    enums_.push_back(enumeration);
    return true;
}

bool BlueprintGraph::RemoveEnum(const ea::string& name)
{
    for (unsigned i = 0; i < enums_.size(); ++i)
    {
        if (enums_[i].name == name)
        {
            enums_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintEnumDef* BlueprintGraph::GetEnum(const ea::string& name)
{
    for (BlueprintEnumDef& enumeration : enums_)
    {
        if (enumeration.name == name)
            return &enumeration;
    }
    return nullptr;
}

const BlueprintEnumDef* BlueprintGraph::GetEnum(const ea::string& name) const
{
    for (const BlueprintEnumDef& enumeration : enums_)
    {
        if (enumeration.name == name)
            return &enumeration;
    }
    return nullptr;
}

bool BlueprintGraph::AddDelegate(const BlueprintDelegate& delegate)
{
    if (delegate.name.empty())
        return false;
    for (unsigned i = 0; i < delegate.parameters.size(); ++i)
    {
        const BlueprintPin& parameter = delegate.parameters[i];
        if (parameter.name.empty() || parameter.kind != BlueprintPinKind::Input)
            return false;
        for (unsigned j = 0; j < i; ++j)
        {
            if (delegate.parameters[j].name == parameter.name)
                return false;
        }
    }
    if (BlueprintDelegate* existing = GetDelegate(delegate.name))
    {
        *existing = delegate;
        return true;
    }
    delegates_.push_back(delegate);
    return true;
}

bool BlueprintGraph::RemoveDelegate(const ea::string& name)
{
    for (unsigned i = 0; i < delegates_.size(); ++i)
    {
        if (delegates_[i].name == name)
        {
            delegates_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintDelegate* BlueprintGraph::GetDelegate(const ea::string& name)
{
    for (BlueprintDelegate& delegate : delegates_)
    {
        if (delegate.name == name)
            return &delegate;
    }
    return nullptr;
}

const BlueprintDelegate* BlueprintGraph::GetDelegate(const ea::string& name) const
{
    for (const BlueprintDelegate& delegate : delegates_)
    {
        if (delegate.name == name)
            return &delegate;
    }
    return nullptr;
}

bool BlueprintGraph::AddTimeline(const BlueprintTimeline& timeline)
{
    if (timeline.name.empty() || timeline.length <= 0.0f)
        return false;
    float previousTime = -1.0f;
    for (const BlueprintTimelineKeyframe& keyframe : timeline.keyframes)
    {
        if (keyframe.time < 0.0f || keyframe.time > timeline.length || keyframe.time < previousTime)
            return false;
        previousTime = keyframe.time;
    }
    if (BlueprintTimeline* existing = GetTimeline(timeline.name))
    {
        *existing = timeline;
        return true;
    }
    timelines_.push_back(timeline);
    return true;
}

bool BlueprintGraph::RemoveTimeline(const ea::string& name)
{
    for (unsigned i = 0; i < timelines_.size(); ++i)
    {
        if (timelines_[i].name == name)
        {
            timelines_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintTimeline* BlueprintGraph::GetTimeline(const ea::string& name)
{
    for (BlueprintTimeline& timeline : timelines_)
    {
        if (timeline.name == name)
            return &timeline;
    }
    return nullptr;
}

const BlueprintTimeline* BlueprintGraph::GetTimeline(const ea::string& name) const
{
    for (const BlueprintTimeline& timeline : timelines_)
    {
        if (timeline.name == name)
            return &timeline;
    }
    return nullptr;
}

bool BlueprintGraph::AddMacro(const BlueprintMacro& macro)
{
    if (macro.name.empty())
        return false;
    auto validatePins = [](const ea::vector<BlueprintPin>& pins)
    {
        for (unsigned i = 0; i < pins.size(); ++i)
        {
            if (pins[i].name.empty() || pins[i].kind != BlueprintPinKind::Input && pins[i].kind != BlueprintPinKind::Output)
                return false;
            for (unsigned j = 0; j < i; ++j)
            {
                if (pins[j].name == pins[i].name)
                    return false;
            }
        }
        return true;
    };
    if (!validatePins(macro.inputs) || !validatePins(macro.outputs))
        return false;
    if (BlueprintMacro* existing = GetMacro(macro.name))
    {
        *existing = macro;
        return true;
    }
    macros_.push_back(macro);
    return true;
}

bool BlueprintGraph::RemoveMacro(const ea::string& name)
{
    for (unsigned i = 0; i < macros_.size(); ++i)
    {
        if (macros_[i].name == name)
        {
            macros_.erase_at(i);
            return true;
        }
    }
    return false;
}

BlueprintMacro* BlueprintGraph::GetMacro(const ea::string& name)
{
    for (BlueprintMacro& macro : macros_)
    {
        if (macro.name == name)
            return &macro;
    }
    return nullptr;
}

const BlueprintMacro* BlueprintGraph::GetMacro(const ea::string& name) const
{
    for (const BlueprintMacro& macro : macros_)
    {
        if (macro.name == name)
            return &macro;
    }
    return nullptr;
}

ea::vector<BlueprintId> BlueprintGraph::SearchNodes(const ea::string& query) const
{
    ea::vector<BlueprintId> result;
    if (query.empty())
        return result;
    const ea::string needle = LowerString(query);
    for (const BlueprintNode& node : nodes_)
    {
        if (LowerString(node.typeName).find(needle) != ea::string::npos ||
            LowerString(node.title).find(needle) != ea::string::npos ||
            LowerString(node.category).find(needle) != ea::string::npos)
            result.push_back(node.id);
    }
    return result;
}

void BlueprintGraph::AutoLayout(float horizontalSpacing, float verticalSpacing)
{
    ea::unordered_map<BlueprintId, unsigned> levels;
    for (const BlueprintNode& node : nodes_)
        levels[node.id] = 0;
    for (unsigned pass = 0; pass < nodes_.size(); ++pass)
    {
        bool changed = false;
        for (const BlueprintLink& link : links_)
        {
            const unsigned nextLevel = levels[link.fromNode] + 1;
            if (nextLevel > levels[link.toNode])
            {
                levels[link.toNode] = nextLevel;
                changed = true;
            }
        }
        if (!changed)
            break;
    }
    ea::unordered_map<unsigned, unsigned> rows;
    for (BlueprintNode& node : nodes_)
    {
        const unsigned level = levels[node.id];
        const unsigned row = rows[level]++;
        node.position = Vector2(level * horizontalSpacing, row * verticalSpacing);
    }
}

void BlueprintGraph::RemoveLinksForNode
(BlueprintId nodeId)
{
    for (unsigned i = 0; i < links_.size();)
    {
        if (links_[i].fromNode == nodeId || links_[i].toNode == nodeId)
            links_.erase_at(i);
        else
            ++i;
    }
}

void BlueprintGraph::RemoveLinksForPin(BlueprintId nodeId, const ea::string& pinName)
{
    for (unsigned i = 0; i < links_.size();)
    {
        const BlueprintLink& link = links_[i];
        if ((link.fromNode == nodeId && link.fromPin == pinName) || (link.toNode == nodeId && link.toPin == pinName))
            links_.erase_at(i);
        else
            ++i;
    }
}

BlueprintValidationResult BlueprintGraph::Validate() const
{
    BlueprintValidationResult result;

    for (unsigned i = 0; i < nodes_.size(); ++i)
    {
        const BlueprintNode& node = nodes_[i];
        if (node.id == BLUEPRINT_INVALID_ID)
            result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, node.id, "BP001", "Node has an invalid identifier."});
        if (node.typeName.empty())
            result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, node.id, "BP002", "Node type name is empty."});

        for (unsigned j = 0; j < node.pins.size(); ++j)
        {
            const BlueprintPin& pin = node.pins[j];
            if (pin.name.empty())
                result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, node.id, "BP003", "Pin name is empty."});
            for (unsigned k = 0; k < j; ++k)
            {
                if (node.pins[k].name == pin.name)
                    result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, node.id, "BP004", Format("Duplicate pin '{}'.", pin.name)});
            }
        }

        for (unsigned j = 0; j < i; ++j)
        {
            if (nodes_[j].id == node.id)
                result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, node.id, "BP005", "Duplicate node identifier."});
        }
    }

    for (const BlueprintLink& link : links_)
    {
        const BlueprintPin* source = GetPin(link.fromNode, link.fromPin);
        const BlueprintPin* target = GetPin(link.toNode, link.toPin);
        if (!source || !target)
        {
            result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, BLUEPRINT_INVALID_ID, "BP010", "Link references a missing node or pin."});
            continue;
        }
        if (!IsPinCompatible(*source, *target))
            result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, link.toNode, "BP011", "Link connects incompatible pins."});

        for (const BlueprintLink& other : links_)
        {
            if (other.id != link.id && other.toNode == link.toNode && other.toPin == link.toPin && !target->allowMultipleConnections)
            {
                result.diagnostics.push_back({BlueprintDiagnosticSeverity::Error, link.toNode, "BP012", "Input pin has more than one connection."});
                break;
            }
        }
    }

    return result;
}

JSONValue BlueprintGraph::ToJSON() const
{
    JSONValue root(JSON_OBJECT);
    // Schema 5 adds timelines and macros while remaining compatible
    // with schema-1 through schema-4 graph files.
    root.Set("format", 5);
    root.Set("schemaVersion", 5);
    root.Set("name", name_);

    JSONValue nodes(JSON_ARRAY);
    for (const BlueprintNode& node : nodes_)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("id", node.id);
        item.Set("type", node.typeName);
        item.Set("title", node.title);
        item.Set("category", node.category);
        item.Set("x", node.position.x_);
        item.Set("y", node.position.y_);
        item.Set("executionMode", Urho3D::ToString(node.executionMode));
        item.Set("enabled", node.enabled);
        item.Set("commentId", node.commentId);

        JSONValue pins(JSON_ARRAY);
        for (const BlueprintPin& pin : node.pins)
            pins.Push(SerializePin(pin));
        item.Set("pins", ea::move(pins));

        JSONValue properties(JSON_OBJECT);
        for (const auto& property : node.properties)
            properties.Set(property.first, SerializeVariant(property.second));
        item.Set("properties", ea::move(properties));
        nodes.Push(ea::move(item));
    }
    root.Set("nodes", ea::move(nodes));

    JSONValue links(JSON_ARRAY);
    for (const BlueprintLink& link : links_)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("id", link.id);
        item.Set("fromNode", link.fromNode);
        item.Set("fromPin", link.fromPin);
        item.Set("toNode", link.toNode);
        item.Set("toPin", link.toPin);
        links.Push(ea::move(item));
    }
    root.Set("links", ea::move(links));

    JSONValue variables(JSON_ARRAY);
    for (const BlueprintVariable& variable : variables_)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("name", variable.name);
        item.Set("dataType", Urho3D::ToString(variable.dataType));
        item.Set("default", SerializeVariant(variable.defaultValue));
        item.Set("exposeOnInstance", variable.exposeOnInstance);
        variables.Push(ea::move(item));
    }
    root.Set("variables", ea::move(variables));

    JSONValue comments(JSON_ARRAY);
    for (const BlueprintComment& comment : comments_)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("id", comment.id);
        item.Set("text", comment.text);
        item.Set("x", comment.position.x_);
        item.Set("y", comment.position.y_);
        item.Set("width", comment.size.x_);
        item.Set("height", comment.size.y_);
        item.Set("color", comment.color);
        comments.Push(ea::move(item));
    }
    root.Set("comments", ea::move(comments));

    JSONValue functions(JSON_ARRAY);
    for (const BlueprintFunction& function : functions_)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("name", function.name);
        item.Set("description", function.description);
        item.Set("body", function.body);
        JSONValue inputs(JSON_ARRAY);
        for (const BlueprintPin& pin : function.inputs)
            inputs.Push(SerializePin(pin));
        item.Set("inputs", ea::move(inputs));
        JSONValue outputs(JSON_ARRAY);
        for (const BlueprintPin& pin : function.outputs)
            outputs.Push(SerializePin(pin));
        item.Set("outputs", ea::move(outputs));
        functions.Push(ea::move(item));
    }
    root.Set("functions", ea::move(functions));

    JSONValue structs(JSON_ARRAY);
    for (const BlueprintStructDef& structure : structs_)
        structs.Push(SerializeStruct(structure));
    root.Set("structs", ea::move(structs));

    JSONValue enums(JSON_ARRAY);
    for (const BlueprintEnumDef& enumeration : enums_)
        enums.Push(SerializeEnum(enumeration));
    root.Set("enums", ea::move(enums));

    JSONValue delegates(JSON_ARRAY);
    for (const BlueprintDelegate& delegate : delegates_)
        delegates.Push(SerializeDelegate(delegate));
    root.Set("delegates", ea::move(delegates));

    JSONValue timelines(JSON_ARRAY);
    for (const BlueprintTimeline& timeline : timelines_)
        timelines.Push(SerializeTimeline(timeline));
    root.Set("timelines", ea::move(timelines));

    JSONValue macros(JSON_ARRAY);
    for (const BlueprintMacro& macro : macros_)
        macros.Push(SerializeMacro(macro));
    root.Set("macros", ea::move(macros));
    return root;
}

bool BlueprintGraph::FromJSON(const JSONValue& value, ea::string* error)
{
    if (!value.IsObject())
    {
        SetError(error, "Blueprint root must be a JSON object.");
        return false;
    }

    const unsigned schemaVersion = value.Contains("schemaVersion")
        ? value["schemaVersion"].GetUInt()
        : (value.Contains("format") ? value["format"].GetUInt() : 1u);
    if (schemaVersion == 0 || schemaVersion > 5)
    {
        SetError(error, Format("Unsupported Blueprint schema version {}.", schemaVersion));
        return false;
    }

    BlueprintGraph parsed;
    parsed.name_ = value.Contains("name") ? value["name"].GetString() : ea::string();

    if (value.Contains("nodes") && value["nodes"].IsArray())
    {
        for (const JSONValue& item : value["nodes"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint node entry must be an object.");
                return false;
            }

            BlueprintNode node;
            node.id = item.Contains("id") ? item["id"].GetUInt() : BLUEPRINT_INVALID_ID;
            // Format 1 used `type`; accept the legacy `typeName` spelling as well.
            node.typeName = item.Contains("type") ? item["type"].GetString()
                : (item.Contains("typeName") ? item["typeName"].GetString() : ea::string());
            node.title = item.Contains("title") ? item["title"].GetString() : node.typeName;
            node.category = item.Contains("category") ? item["category"].GetString() : ea::string();
            node.position = Vector2(item.Contains("x") ? item["x"].GetFloat() : 0.0f, item.Contains("y") ? item["y"].GetFloat() : 0.0f);
            node.executionMode = item.Contains("executionMode") ? ParseBlueprintExecutionMode(item["executionMode"].GetString()) : BlueprintExecutionMode::Immediate;
            node.enabled = !item.Contains("enabled") || item["enabled"].GetBool(true);
            node.commentId = item.Contains("commentId") ? item["commentId"].GetUInt() : BLUEPRINT_INVALID_ID;

            if (item.Contains("pins") && item["pins"].IsArray())
            {
                for (const JSONValue& pinValue : item["pins"].GetArray())
                    node.pins.push_back(DeserializePin(pinValue));
            }
            if (item.Contains("properties") && item["properties"].IsObject())
            {
                for (const auto& property : item["properties"].GetObject())
                    node.properties[property.first] = DeserializeVariant(property.second);
            }
            parsed.AddNode(node);
        }
    }

    if (value.Contains("links") && value["links"].IsArray())
    {
        for (const JSONValue& item : value["links"].GetArray())
        {
            BlueprintLink link;
            link.id = item.Contains("id") ? item["id"].GetUInt() : BLUEPRINT_INVALID_ID;
            link.fromNode = item.Contains("fromNode") ? item["fromNode"].GetUInt() : BLUEPRINT_INVALID_ID;
            link.fromPin = item.Contains("fromPin") ? item["fromPin"].GetString() : ea::string();
            link.toNode = item.Contains("toNode") ? item["toNode"].GetUInt() : BLUEPRINT_INVALID_ID;
            link.toPin = item.Contains("toPin") ? item["toPin"].GetString() : ea::string();
            const BlueprintId newId = parsed.AddLink(link.fromNode, link.fromPin, link.toNode, link.toPin);
            if (newId == BLUEPRINT_INVALID_ID)
            {
                SetError(error, Format("Invalid Blueprint link from node {}:{} to node {}:{}.", link.fromNode, link.fromPin, link.toNode, link.toPin));
                return false;
            }
            parsed.links_.back().id = link.id == BLUEPRINT_INVALID_ID ? newId : link.id;
            if (parsed.links_.back().id >= parsed.nextLinkId_)
                parsed.nextLinkId_ = parsed.links_.back().id + 1;
        }
    }

    if (value.Contains("comments") && value["comments"].IsArray())
    {
        for (const JSONValue& item : value["comments"].GetArray())
        {
            BlueprintComment comment;
            comment.id = item.Contains("id") ? item["id"].GetUInt() : BLUEPRINT_INVALID_ID;
            comment.text = item.Contains("text") ? item["text"].GetString() : ea::string();
            comment.position = Vector2(item.Contains("x") ? item["x"].GetFloat() : 0.0f, item.Contains("y") ? item["y"].GetFloat() : 0.0f);
            comment.size = Vector2(item.Contains("width") ? item["width"].GetFloat() : 260.0f, item.Contains("height") ? item["height"].GetFloat() : 120.0f);
            comment.color = item.Contains("color") ? item["color"].GetUInt() : 0x664A78A8;
            if (!parsed.AddComment(comment))
            {
                SetError(error, "Blueprint comment has an empty text.");
                return false;
            }
        }
    }

    if (value.Contains("functions") && value["functions"].IsArray())
    {
        for (const JSONValue& item : value["functions"].GetArray())
        {
            BlueprintFunction function;
            function.name = item.Contains("name") ? item["name"].GetString() : ea::string();
            function.description = item.Contains("description") ? item["description"].GetString() : ea::string();
            function.body = item.Contains("body") ? item["body"].GetString() : ea::string();
            if (item.Contains("inputs") && item["inputs"].IsArray())
            {
                for (const JSONValue& pin : item["inputs"].GetArray())
                    function.inputs.push_back(DeserializePin(pin));
            }
            if (item.Contains("outputs") && item["outputs"].IsArray())
            {
                for (const JSONValue& pin : item["outputs"].GetArray())
                    function.outputs.push_back(DeserializePin(pin));
            }
            if (!parsed.AddFunction(function))
            {
                SetError(error, "Blueprint function has an empty name.");
                return false;
            }
        }
    }

    if (value.Contains("structs") && value["structs"].IsArray())
    {
        for (const JSONValue& item : value["structs"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint struct entry must be an object.");
                return false;
            }
            const BlueprintStructDef structure = DeserializeStruct(item);
            if (!parsed.AddStruct(structure))
            {
                SetError(error, "Blueprint struct has an empty or duplicate field name.");
                return false;
            }
        }
    }

    if (value.Contains("enums") && value["enums"].IsArray())
    {
        for (const JSONValue& item : value["enums"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint enum entry must be an object.");
                return false;
            }
            const BlueprintEnumDef enumeration = DeserializeEnum(item);
            if (!parsed.AddEnum(enumeration))
            {
                SetError(error, "Blueprint enum has an empty or duplicate value name.");
                return false;
            }
        }
    }

    if (value.Contains("delegates") && value["delegates"].IsArray())
    {
        for (const JSONValue& item : value["delegates"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint delegate entry must be an object.");
                return false;
            }
            const BlueprintDelegate delegate = DeserializeDelegate(item);
            if (!parsed.AddDelegate(delegate))
            {
                SetError(error, "Blueprint delegate has an empty name, invalid parameter or duplicate parameter name.");
                return false;
            }
        }
    }

    if (value.Contains("timelines") && value["timelines"].IsArray())
    {
        for (const JSONValue& item : value["timelines"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint timeline entry must be an object.");
                return false;
            }
            const BlueprintTimeline timeline = DeserializeTimeline(item);
            if (!parsed.AddTimeline(timeline))
            {
                SetError(error, "Blueprint timeline has an invalid name, length or keyframe order.");
                return false;
            }
        }
    }

    if (value.Contains("macros") && value["macros"].IsArray())
    {
        for (const JSONValue& item : value["macros"].GetArray())
        {
            if (!item.IsObject())
            {
                SetError(error, "Blueprint macro entry must be an object.");
                return false;
            }
            const BlueprintMacro macro = DeserializeMacro(item);
            if (!parsed.AddMacro(macro))
            {
                SetError(error, "Blueprint macro has an invalid name or pin signature.");
                return false;
            }
        }
    }

    if (value.Contains("variables") && value["variables"].IsArray())
    {
        for (const JSONValue& item : value["variables"].GetArray())
        {
            BlueprintVariable variable;
            variable.name = item.Contains("name") ? item["name"].GetString() : ea::string();
            variable.dataType = item.Contains("dataType") ? ParseBlueprintDataType(item["dataType"].GetString()) : BlueprintDataType::Variant;
            variable.defaultValue = item.Contains("default") ? DeserializeVariant(item["default"]) : Variant();
            variable.exposeOnInstance = item.Contains("exposeOnInstance") && item["exposeOnInstance"].GetBool();
            if (!parsed.AddVariable(variable))
            {
                SetError(error, "Blueprint variable has an empty or duplicate name.");
                return false;
            }
        }
    }

    const BlueprintValidationResult validation = parsed.Validate();
    if (validation.HasErrors())
    {
        SetError(error, validation.diagnostics[0].message);
        return false;
    }

    *this = ea::move(parsed);
    return true;
}

ea::string BlueprintGraph::ToString(const ea::string& indentation) const
{
    const auto serialize = [&](Context* context)
    {
        JSONFile file(context);
        file.GetRoot() = ToJSON();
        return file.ToString(indentation);
    };

    if (Context* context = Context::GetInstance())
        return serialize(context);

    // JSONFile derives from Object and requires a valid Context. Keep the
    // context-free graph API usable in command-line tools and unit tests while
    // avoiding a second global Context when the engine is already running.
    Context localContext;
    return serialize(&localContext);
}

bool BlueprintGraph::FromString(const ea::string& source, ea::string* error)
{
    JSONValue root;
    if (!JSONFile::ParseJSON(source, root, false))
    {
        SetError(error, "Unable to parse Blueprint JSON.");
        return false;
    }
    return FromJSON(root, error);
}

}
