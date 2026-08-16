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
    if (source.dataType == target.dataType)
        return true;
    return IsNumeric(source.dataType) && IsNumeric(target.dataType);
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
    root.Set("format", 1);
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
    return root;
}

bool BlueprintGraph::FromJSON(const JSONValue& value, ea::string* error)
{
    if (!value.IsObject())
    {
        SetError(error, "Blueprint root must be a JSON object.");
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
            node.typeName = item.Contains("type") ? item["type"].GetString() : ea::string();
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
