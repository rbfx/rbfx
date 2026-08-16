// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintRuntime.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/Log.h>

namespace Urho3D
{

namespace
{

ea::string MakeValueKey(BlueprintId nodeId, const ea::string& pinName)
{
    return Format("{}:{}", nodeId, pinName);
}

bool IsPureNode(const BlueprintNode& node, const BlueprintNodeDefinition& definition)
{
    return node.executionMode == BlueprintExecutionMode::Pure || definition.executionMode == BlueprintExecutionMode::Pure;
}

void RegisterDefinition(BlueprintNodeRegistry& registry, const char* typeName, const char* category,
    BlueprintExecutionMode mode, BlueprintNodeExecutor executor, const char* description)
{
    BlueprintNodeDefinition definition;
    definition.typeName = typeName;
    definition.category = category;
    definition.executionMode = mode;
    definition.execute = ea::move(executor);
    definition.description = description;
    registry.Register(definition);
}

}

bool BlueprintNodeRegistry::Register(const BlueprintNodeDefinition& definition)
{
    if (definition.typeName.empty() || !definition.execute)
        return false;

    for (BlueprintNodeDefinition& existing : definitions_)
    {
        if (existing.typeName == definition.typeName)
        {
            existing = definition;
            return true;
        }
    }
    definitions_.push_back(definition);
    return true;
}

bool BlueprintNodeRegistry::Unregister(const ea::string& typeName)
{
    for (unsigned i = 0; i < definitions_.size(); ++i)
    {
        if (definitions_[i].typeName == typeName)
        {
            definitions_.erase_at(i);
            return true;
        }
    }
    return false;
}

const BlueprintNodeDefinition* BlueprintNodeRegistry::Find(const ea::string& typeName) const
{
    for (const BlueprintNodeDefinition& definition : definitions_)
    {
        if (definition.typeName == typeName)
            return &definition;
    }
    return nullptr;
}

BlueprintExecutionContext::BlueprintExecutionContext(BlueprintRuntime& runtime, const BlueprintGraph& graph,
    const BlueprintNode& node, StringVariantMap& values, StringVariantMap& variables)
    : runtime_(runtime)
    , graph_(graph)
    , node_(node)
    , values_(values)
    , variables_(variables)
{
}

Variant BlueprintExecutionContext::GetInput(const ea::string& pinName) const
{
    const BlueprintLink* link = runtime_.FindIncomingLink(graph_, node_.id, pinName);
    if (link)
        return runtime_.EvaluateOutput(graph_, link->fromNode, link->fromPin, 0);

    for (const BlueprintPin& pin : node_.pins)
    {
        if (pin.name == pinName)
            return pin.defaultValue;
    }
    return Variant();
}

Variant BlueprintExecutionContext::GetOutput(BlueprintId nodeId, const ea::string& pinName) const
{
    return runtime_.GetValue(nodeId, pinName);
}

void BlueprintExecutionContext::SetOutput(const ea::string& pinName, const Variant& value)
{
    values_[MakeValueKey(node_.id, pinName)] = value;
}

void BlueprintExecutionContext::ContinueWith(const ea::string& executionPin)
{
    continuationPin_ = executionPin;
}

Variant BlueprintExecutionContext::GetVariable(const ea::string& name) const
{
    const auto iter = variables_.find(name);
    return iter != variables_.end() ? iter->second : Variant();
}

void BlueprintExecutionContext::SetVariable(const ea::string& name, const Variant& value)
{
    variables_[name] = value;
}

BlueprintRuntime::BlueprintRuntime()
{
    RegisterBuiltinNodes();
}

void BlueprintRuntime::RegisterBuiltinNodes()
{
    RegisterDefinition(registry_, "Event.OnStart", "Events", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            context.ContinueWith("then");
        }, "Entry point fired when a Blueprint instance starts.");

    RegisterDefinition(registry_, "Flow.Branch", "Flow Control", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            context.ContinueWith(context.GetInput("condition").GetBool() ? "true" : "false");
        }, "Route execution to one of two outputs based on a boolean condition.");

    RegisterDefinition(registry_, "Flow.Print", "Flow Control", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            URHO3D_LOGINFO("Blueprint: {}", context.GetInput("message").GetString());
            context.ContinueWith("then");
        }, "Write a message to the engine log.");

    RegisterDefinition(registry_, "Math.AddFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("a").GetFloat() + context.GetInput("b").GetFloat());
        }, "Add two floating-point values.");

    RegisterDefinition(registry_, "Math.MultiplyFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("a").GetFloat() * context.GetInput("b").GetFloat());
        }, "Multiply two floating-point values.");

    RegisterDefinition(registry_, "Math.LessFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("a").GetFloat() < context.GetInput("b").GetFloat());
        }, "Compare two floating-point values.");

    RegisterDefinition(registry_, "Variable.Get", "Variables", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const auto iter = context.GetNode().properties.find("variableName");
            if (iter != context.GetNode().properties.end())
                context.SetOutput("value", context.GetVariable(iter->second.GetString()));
        }, "Read a graph variable.");

    RegisterDefinition(registry_, "Variable.Set", "Variables", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const auto iter = context.GetNode().properties.find("variableName");
            if (iter != context.GetNode().properties.end())
                context.SetVariable(iter->second.GetString(), context.GetInput("value"));
            context.ContinueWith("then");
        }, "Write a graph variable and continue execution.");
}

bool BlueprintRuntime::Execute(const BlueprintGraph& graph, BlueprintId entryNode,
    StringVariantMap variables, unsigned maxSteps)
{
    values_.clear();
    variables_ = ea::move(variables);
    diagnostics_.clear();
    hadRuntimeError_ = false;

    const BlueprintValidationResult validation = graph.Validate();
    if (validation.HasErrors())
    {
        diagnostics_ = validation.diagnostics;
        hadRuntimeError_ = true;
        return false;
    }
    if (!graph.GetNode(entryNode))
    {
        AddError(entryNode, "BP100", "Entry node does not exist.");
        return false;
    }

    unsigned steps = 0;
    return ExecuteNode(graph, entryNode, steps) && steps <= maxSteps;
}

bool BlueprintRuntime::ExecuteEvent(const BlueprintGraph& graph, const ea::string& eventType,
    StringVariantMap variables, unsigned maxSteps)
{
    for (const BlueprintNode& node : graph.GetNodes())
    {
        if (node.typeName == eventType)
            return Execute(graph, node.id, ea::move(variables), maxSteps);
    }

    diagnostics_.clear();
    hadRuntimeError_ = true;
    AddError(BLUEPRINT_INVALID_ID, "BP101", Format("Event node '{}' was not found.", eventType));
    return false;
}

bool BlueprintRuntime::ExecuteNode(const BlueprintGraph& graph, BlueprintId nodeId, unsigned& steps)
{
    if (++steps > 10000)
    {
        AddError(nodeId, "BP102", "Blueprint execution exceeded the safety step limit.");
        return false;
    }

    const BlueprintNode* node = graph.GetNode(nodeId);
    if (!node)
    {
        AddError(nodeId, "BP103", "Execution reached a missing node.");
        return false;
    }
    if (!node->enabled)
        return true;

    const BlueprintNodeDefinition* definition = registry_.Find(node->typeName);
    if (!definition)
    {
        AddError(nodeId, "BP104", Format("No runtime implementation is registered for '{}'.", node->typeName));
        return false;
    }

    BlueprintExecutionContext context(*this, graph, *node, values_, variables_);
    definition->execute(context);
    if (IsPureNode(*node, *definition))
        return true;
    if (context.GetContinuationPin().empty())
        return true;

    const BlueprintLink* next = FindOutgoingExecutionLink(graph, nodeId, context.GetContinuationPin());
    return next ? ExecuteNode(graph, next->toNode, steps) : true;
}

Variant BlueprintRuntime::EvaluateOutput(const BlueprintGraph& graph, BlueprintId nodeId,
    const ea::string& pinName, unsigned depth)
{
    if (depth > 128)
    {
        AddError(nodeId, "BP105", "Pure Blueprint evaluation exceeded the recursion limit.");
        return Variant();
    }

    const ea::string key = MakeValueKey(nodeId, pinName);
    const auto cached = values_.find(key);
    if (cached != values_.end())
        return cached->second;

    const BlueprintNode* node = graph.GetNode(nodeId);
    const BlueprintNodeDefinition* definition = node ? registry_.Find(node->typeName) : nullptr;
    if (!node || !definition)
    {
        AddError(nodeId, "BP106", "Pure evaluation reached an unknown node.");
        return Variant();
    }

    if (!IsPureNode(*node, *definition))
    {
        AddError(nodeId, "BP107", "A data pin references a non-pure node.");
        return Variant();
    }

    BlueprintExecutionContext context(*this, graph, *node, values_, variables_);
    definition->execute(context);
    const auto result = values_.find(key);
    return result != values_.end() ? result->second : Variant();
}

const BlueprintLink* BlueprintRuntime::FindIncomingLink(const BlueprintGraph& graph,
    BlueprintId nodeId, const ea::string& pinName) const
{
    for (const BlueprintLink& link : graph.GetLinks())
    {
        if (link.toNode == nodeId && link.toPin == pinName)
            return &link;
    }
    return nullptr;
}

const BlueprintLink* BlueprintRuntime::FindOutgoingExecutionLink(const BlueprintGraph& graph,
    BlueprintId nodeId, const ea::string& pinName) const
{
    for (const BlueprintLink& link : graph.GetLinks())
    {
        if (link.fromNode == nodeId && link.fromPin == pinName)
            return &link;
    }
    return nullptr;
}

Variant BlueprintRuntime::GetValue(BlueprintId nodeId, const ea::string& pinName) const
{
    const auto iter = values_.find(MakeValueKey(nodeId, pinName));
    return iter != values_.end() ? iter->second : Variant();
}

Variant BlueprintRuntime::GetVariable(const ea::string& name) const
{
    const auto iter = variables_.find(name);
    return iter != variables_.end() ? iter->second : Variant();
}

void BlueprintRuntime::AddError(BlueprintId nodeId, const ea::string& code, const ea::string& message)
{
    hadRuntimeError_ = true;
    diagnostics_.push_back({BlueprintDiagnosticSeverity::Error, nodeId, code, message});
}

}
