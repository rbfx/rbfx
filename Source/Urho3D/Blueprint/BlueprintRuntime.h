// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintGraph.h"
#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

class BlueprintRuntime;

/// Runtime callback implemented by a built-in node or a user extension.
using BlueprintNodeExecutor = ea::function<void(class BlueprintExecutionContext&)>;

/// Description and executable callback for a registered Blueprint node type.
struct URHO3D_API BlueprintNodeDefinition
{
    ea::string typeName;
    ea::string category;
    ea::string description;
    BlueprintExecutionMode executionMode{BlueprintExecutionMode::Immediate};
    BlueprintNodeExecutor execute;
};

/// Registry that maps serialized node type names to runtime implementations.
class URHO3D_API BlueprintNodeRegistry
{
public:
    /// Register or replace a node definition.
    bool Register(const BlueprintNodeDefinition& definition);
    /// Remove a node definition.
    bool Unregister(const ea::string& typeName);
    /// Find a node definition.
    const BlueprintNodeDefinition* Find(const ea::string& typeName) const;
    /// Return all registered definitions.
    const ea::vector<BlueprintNodeDefinition>& GetDefinitions() const { return definitions_; }

private:
    ea::vector<BlueprintNodeDefinition> definitions_;
};

/// Per-node execution context used by custom callbacks.
class URHO3D_API BlueprintExecutionContext
{
public:
    BlueprintExecutionContext(BlueprintRuntime& runtime, const BlueprintGraph& graph, const BlueprintNode& node,
        StringVariantMap& values, StringVariantMap& variables);

    /// Return the current graph and node.
    const BlueprintGraph& GetGraph() const { return graph_; }
    const BlueprintNode& GetNode() const { return node_; }

    /// Read a connected input, falling back to the pin's default value.
    Variant GetInput(const ea::string& pinName) const;
    /// Read a named output previously produced by a node.
    Variant GetOutput(BlueprintId nodeId, const ea::string& pinName) const;
    /// Write an output value for the current node.
    void SetOutput(const ea::string& pinName, const Variant& value);
    /// Select the execution output that should be followed next.
    void ContinueWith(const ea::string& executionPin);
    /// Return the selected execution output.
    const ea::string& GetContinuationPin() const { return continuationPin_; }

    /// Read or write a Blueprint graph variable.
    Variant GetVariable(const ea::string& name) const;
    void SetVariable(const ea::string& name, const Variant& value);

private:
    BlueprintRuntime& runtime_;
    const BlueprintGraph& graph_;
    const BlueprintNode& node_;
    StringVariantMap& values_;
    StringVariantMap& variables_;
    ea::string continuationPin_;
};

/// Deterministic runtime that executes a validated graph through registered node callbacks.
class URHO3D_API BlueprintRuntime
{
public:
    BlueprintRuntime();

    /// Access the extension registry.
    BlueprintNodeRegistry& GetRegistry() { return registry_; }
    const BlueprintNodeRegistry& GetRegistry() const { return registry_; }

    /// Register the standard logic and math nodes.
    void RegisterBuiltinNodes();

    /// Execute a graph from a node identifier.
    bool Execute(const BlueprintGraph& graph, BlueprintId entryNode,
        StringVariantMap variables = {}, unsigned maxSteps = 10000);
    /// Execute the first node with a matching type name.
    bool ExecuteEvent(const BlueprintGraph& graph, const ea::string& eventType,
        StringVariantMap variables = {}, unsigned maxSteps = 10000);

    /// Read runtime values after execution.
    Variant GetValue(BlueprintId nodeId, const ea::string& pinName) const;
    Variant GetVariable(const ea::string& name) const;
    const ea::vector<BlueprintDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    bool HadRuntimeError() const { return hadRuntimeError_; }

private:
    friend class BlueprintExecutionContext;

    bool ExecuteNode(const BlueprintGraph& graph, BlueprintId nodeId, unsigned& steps);
    Variant EvaluateOutput(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName, unsigned depth);
    const BlueprintLink* FindIncomingLink(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName) const;
    const BlueprintLink* FindOutgoingExecutionLink(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName) const;
    void AddError(BlueprintId nodeId, const ea::string& code, const ea::string& message);

    BlueprintNodeRegistry registry_;
    StringVariantMap values_;
    StringVariantMap variables_;
    ea::vector<BlueprintDiagnostic> diagnostics_;
    bool hadRuntimeError_{false};
};

}
