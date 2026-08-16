// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintGraph.h"
#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class BlueprintRuntime;
class Context;
class Serializable;

/// Runtime callback implemented by a built-in node or a user extension.
using BlueprintNodeExecutor = ea::function<void(class BlueprintExecutionContext&)>;

/// Description and executable callback for a registered Blueprint node type.
struct URHO3D_API BlueprintNodeDefinition
{
    ea::string typeName;
    ea::string category;
    ea::string description;
    BlueprintExecutionMode executionMode{BlueprintExecutionMode::Immediate};
    /// Pins exposed when this definition is instantiated in the editor.
    ea::vector<BlueprintPin> pins;
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
    /// Find a node definition by serialized type name.
    const BlueprintNodeDefinition* Find(const ea::string& typeName) const;
    /// Return all registered definitions for palette search and editor tooling.
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
    /// Return the Serializable object bound to the current execution, if any.
    Serializable* GetTargetObject() const;
    /// Add a runtime diagnostic associated with the current node.
    void ReportError(const ea::string& code, const ea::string& message);

    /// Read or write a Blueprint graph variable.
    Variant GetVariable(const ea::string& name) const;
    void SetVariable(const ea::string& name, const Variant& value);
    /// Execute a user-defined function/subgraph from the owning Blueprint graph.
    bool CallFunction(const ea::string& functionName, const StringVariantMap& inputs = {},
        StringVariantMap* outputs = nullptr);
    /// Pause execution for a duration and resume through BlueprintRuntime::Tick.
    void Delay(float seconds);

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
    /// Generate getter and setter nodes from rbfx ObjectReflection metadata.
    unsigned RegisterReflectedNodes(Context* context);
    /// Bind a scene node or component for reflected property nodes.
    void SetTargetObject(Serializable* object) { targetObject_ = object; }
    Serializable* GetTargetObject() const { return targetObject_; }

    /// Execute a graph from a node identifier.
    bool Execute(const BlueprintGraph& graph, BlueprintId entryNode,
        StringVariantMap variables = {}, unsigned maxSteps = 10000);
    /// Execute the first node with a matching type name.
    bool ExecuteEvent(const BlueprintGraph& graph, const ea::string& eventType,
        StringVariantMap variables = {}, unsigned maxSteps = 10000);

    /// Start a resumable debug session at an event node.
    bool BeginDebug(const BlueprintGraph& graph, const ea::string& eventType, StringVariantMap variables = {});
    /// Execute only the current debug node and advance to the next execution node.
    bool StepDebug();
    /// Stop the active debug session.
    void StopDebug();
    /// Return whether a debug session is active.
    bool IsDebugActive() const { return debugGraph_ != nullptr && debugCurrentNode_ != BLUEPRINT_INVALID_ID; }
    /// Advance a latent Blueprint execution by elapsed seconds.
    bool Tick(float deltaSeconds);
    /// Continue a debug session until completion or the next breakpoint.
    bool ContinueDebug(unsigned maxSteps = 10000);
    /// Enable or disable a breakpoint on a node.
    void SetBreakpoint(BlueprintId nodeId, bool enabled = true);
    /// Return whether a node has a breakpoint.
    bool HasBreakpoint(BlueprintId nodeId) const { return breakpoints_.find(nodeId) != breakpoints_.end(); }
    /// Return all active breakpoints.
    const ea::unordered_set<BlueprintId>& GetBreakpoints() const { return breakpoints_; }
    /// Return whether an execution is currently waiting on a latent node.
    bool IsLatent() const { return latentPending_; }
    /// Return the node currently paused in the debugger.
    BlueprintId GetDebugCurrentNode() const { return debugCurrentNode_; }

    /// Read runtime values after execution.
    Variant GetValue(BlueprintId nodeId, const ea::string& pinName) const;
    Variant GetVariable(const ea::string& name) const;
    /// Return node output values accumulated during execution for the Watch window.
    const StringVariantMap& GetWatchValues() const { return values_; }
    /// Return graph variables accumulated during execution for the Watch window.
    const StringVariantMap& GetWatchVariables() const { return variables_; }
    /// Return the active Blueprint function call stack.
    const ea::vector<ea::string>& GetCallStack() const { return functionCallStack_; }
    const ea::vector<BlueprintDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    bool HadRuntimeError() const { return hadRuntimeError_; }

private:
    friend class BlueprintExecutionContext;

    bool ExecuteNode(const BlueprintGraph& graph, BlueprintId nodeId, unsigned& steps);
    Variant EvaluateOutput(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName, unsigned depth);
    const BlueprintLink* FindIncomingLink(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName) const;
    const BlueprintLink* FindOutgoingExecutionLink(const BlueprintGraph& graph, BlueprintId nodeId, const ea::string& pinName) const;
    void AddError(BlueprintId nodeId, const ea::string& code, const ea::string& message);
    void ScheduleDelay(float seconds) { latentPending_ = true; latentRemaining_ = Max(0.0f, seconds); }
    bool ExecuteFunction(const BlueprintGraph& ownerGraph, const ea::string& functionName,
        const StringVariantMap& inputs = {}, StringVariantMap* outputs = nullptr, unsigned maxSteps = 10000);

    BlueprintNodeRegistry registry_;
    Serializable* targetObject_{};
    StringVariantMap values_;
    StringVariantMap variables_;
    ea::vector<BlueprintDiagnostic> diagnostics_;
    ea::vector<ea::string> functionCallStack_;
    bool hadRuntimeError_{false};
    const BlueprintGraph* debugGraph_{};
    BlueprintId debugCurrentNode_{BLUEPRINT_INVALID_ID};
    unsigned debugSteps_{};
    const BlueprintGraph* latentGraph_{};
    BlueprintId latentNextNode_{BLUEPRINT_INVALID_ID};
    float latentRemaining_{};
    bool latentPending_{};
    ea::unordered_set<BlueprintId> breakpoints_;
};

}
