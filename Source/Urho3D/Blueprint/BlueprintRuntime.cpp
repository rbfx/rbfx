// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintRuntime.h"
#include "BlueprintReflection.h"

#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Scene/Node.h>

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

BlueprintPin RuntimePin(const char* name, BlueprintPinKind kind, BlueprintDataType type, const Variant& defaultValue = Variant())
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

Node* GetTargetNode(BlueprintExecutionContext& context)
{
    Serializable* target = context.GetTargetObject();
    if (!target || !target->IsInstanceOf(Node::GetTypeStatic()))
    {
        context.ReportError("BP2D3D001", "This node requires a bound rbfx scene Node target.");
        return nullptr;
    }
    return static_cast<Node*>(target);
}

void RegisterDefinition(BlueprintNodeRegistry& registry, const char* typeName, const char* category,
    BlueprintExecutionMode mode, BlueprintNodeExecutor executor, const char* description,
    ea::vector<BlueprintPin> pins = {})
{
    BlueprintNodeDefinition definition;
    definition.typeName = typeName;
    definition.category = category;
    definition.executionMode = mode;
    definition.pins = ea::move(pins);
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

Serializable* BlueprintExecutionContext::GetTargetObject() const
{
    return runtime_.GetTargetObject();
}

void BlueprintExecutionContext::ReportError(const ea::string& code, const ea::string& message)
{
    runtime_.AddError(node_.id, code, message);
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

bool BlueprintExecutionContext::CallFunction(const ea::string& functionName)
{
    return runtime_.ExecuteFunction(graph_, functionName);
}

BlueprintRuntime::BlueprintRuntime()
{
    RegisterBuiltinNodes();
}

unsigned BlueprintRuntime::RegisterReflectedNodes(Context* context)
{
    return BlueprintReflectionRegistry::RegisterNodes(context, registry_);
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

    RegisterDefinition(registry_, "Function.Entry", "Functions", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            context.ContinueWith("then");
        }, "Entry point of a user-defined Blueprint function.",
        {RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard)});

    RegisterDefinition(registry_, "Function.Return", "Functions", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext&)
        {
        }, "Return from a user-defined Blueprint function.");

    RegisterDefinition(registry_, "Function.Call", "Functions", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const auto iter = context.GetNode().properties.find("functionName");
            if (iter == context.GetNode().properties.end() || iter->second.GetString().empty())
            {
                context.ReportError("BP200", "Function.Call requires a non-empty functionName property.");
                return;
            }
            if (context.CallFunction(iter->second.GetString()))
                context.ContinueWith("then");
        }, "Execute a user-defined Blueprint function/subgraph.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard)});

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

    RegisterDefinition(registry_, "Scene.GetPosition3D", "Scene/3D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("position", node->GetPosition());
        }, "Read the target Node position in 3D.",
        {RuntimePin("position", BlueprintPinKind::Output, BlueprintDataType::Vector3)});

    RegisterDefinition(registry_, "Scene.SetPosition3D", "Scene/3D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetPosition(context.GetInput("position").GetVector3());
                context.ContinueWith("then");
            }
        }, "Set the target Node position in 3D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("position", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO))});

    RegisterDefinition(registry_, "Scene.Translate3D", "Scene/3D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->Translate(context.GetInput("delta").GetVector3());
                context.ContinueWith("then");
            }
        }, "Translate the target Node in 3D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("delta", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO))});

    RegisterDefinition(registry_, "Scene.GetRotation3D", "Scene/3D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("rotation", node->GetRotation());
        }, "Read the target Node rotation in 3D.",
        {RuntimePin("rotation", BlueprintPinKind::Output, BlueprintDataType::Quaternion)});

    RegisterDefinition(registry_, "Scene.SetRotation3D", "Scene/3D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetRotation(context.GetInput("rotation").GetQuaternion());
                context.ContinueWith("then");
            }
        }, "Set the target Node rotation in 3D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("rotation", BlueprintPinKind::Input, BlueprintDataType::Quaternion, Variant(Quaternion::IDENTITY))});

    RegisterDefinition(registry_, "Scene.GetScale3D", "Scene/3D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("scale", node->GetScale());
        }, "Read the target Node scale in 3D.",
        {RuntimePin("scale", BlueprintPinKind::Output, BlueprintDataType::Vector3)});

    RegisterDefinition(registry_, "Scene.SetScale3D", "Scene/3D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetScale(context.GetInput("scale").GetVector3());
                context.ContinueWith("then");
            }
        }, "Set the target Node scale in 3D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("scale", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ONE))});

    RegisterDefinition(registry_, "Scene.GetRotation2D", "Scene/2D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("rotation", node->GetRotation2D());
        }, "Read the target Node rotation in 2D.",
        {RuntimePin("rotation", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "Scene.SetRotation2D", "Scene/2D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetRotation2D(context.GetInput("rotation").GetFloat());
                context.ContinueWith("then");
            }
        }, "Set the target Node rotation in 2D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("rotation", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f))});

    RegisterDefinition(registry_, "Scene.GetScale2D", "Scene/2D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("scale", node->GetScale2D());
        }, "Read the target Node scale in 2D.",
        {RuntimePin("scale", BlueprintPinKind::Output, BlueprintDataType::Vector2)});

    RegisterDefinition(registry_, "Scene.SetScale2D", "Scene/2D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetScale2D(context.GetInput("scale").GetVector2());
                context.ContinueWith("then");
            }
        }, "Set the target Node scale in 2D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("scale", BlueprintPinKind::Input, BlueprintDataType::Vector2, Variant(Vector2::ONE))});

    RegisterDefinition(registry_, "Scene.GetPosition2D", "Scene/2D", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
                context.SetOutput("position", node->GetPosition2D());
        }, "Read the target Node position in 2D.",
        {RuntimePin("position", BlueprintPinKind::Output, BlueprintDataType::Vector2)});

    RegisterDefinition(registry_, "Scene.SetPosition2D", "Scene/2D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->SetPosition2D(context.GetInput("position").GetVector2());
                context.ContinueWith("then");
            }
        }, "Set the target Node position in 2D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("position", BlueprintPinKind::Input, BlueprintDataType::Vector2, Variant(Vector2::ZERO))});

    RegisterDefinition(registry_, "Scene.Translate2D", "Scene/2D", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            if (Node* node = GetTargetNode(context))
            {
                node->Translate2D(context.GetInput("delta").GetVector2());
                context.ContinueWith("then");
            }
        }, "Translate the target Node in 2D.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("delta", BlueprintPinKind::Input, BlueprintDataType::Vector2, Variant(Vector2::ZERO))});
}

bool BlueprintRuntime::Execute(const BlueprintGraph& graph, BlueprintId entryNode,
    StringVariantMap variables, unsigned maxSteps)
{
    values_.clear();
    variables_ = ea::move(variables);
    diagnostics_.clear();
    functionCallStack_.clear();
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

bool BlueprintRuntime::ExecuteFunction(const BlueprintGraph& ownerGraph, const ea::string& functionName, unsigned maxSteps)
{
    const BlueprintFunction* function = ownerGraph.GetFunction(functionName);
    if (!function)
    {
        AddError(BLUEPRINT_INVALID_ID, "BP201", Format("Blueprint function '{}' was not found.", functionName));
        return false;
    }
    if (functionCallStack_.size() >= 32)
    {
        AddError(BLUEPRINT_INVALID_ID, "BP203", "Blueprint function call depth exceeded 32.");
        return false;
    }
    for (const ea::string& activeFunction : functionCallStack_)
    {
        if (activeFunction == functionName)
        {
            AddError(BLUEPRINT_INVALID_ID, "BP204", Format("Recursive Blueprint function '{}' is not allowed.", functionName));
            return false;
        }
    }

    BlueprintGraph body;
    ea::string error;
    if (function->body.empty() || !body.FromString(function->body, &error))
    {
        AddError(BLUEPRINT_INVALID_ID, "BP202", Format("Function '{}' has an invalid body: {}", functionName, error));
        return false;
    }
    const BlueprintValidationResult validation = body.Validate();
    if (validation.HasErrors())
    {
        diagnostics_ = validation.diagnostics;
        hadRuntimeError_ = true;
        return false;
    }

    BlueprintId entryNode = BLUEPRINT_INVALID_ID;
    for (const BlueprintNode& node : body.GetNodes())
    {
        if (node.typeName == "Function.Entry")
        {
            entryNode = node.id;
            break;
        }
    }
    if (entryNode == BLUEPRINT_INVALID_ID)
    {
        AddError(BLUEPRINT_INVALID_ID, "BP205", Format("Function '{}' has no Function.Entry node.", functionName));
        return false;
    }

    functionCallStack_.push_back(functionName);
    unsigned steps = 0;
    const bool result = ExecuteNode(body, entryNode, steps) && steps <= maxSteps;
    functionCallStack_.pop_back();
    return result && !hadRuntimeError_;
}

bool BlueprintRuntime::BeginDebug(const BlueprintGraph& graph, const ea::string& eventType, StringVariantMap variables)
{
    StopDebug();
    const BlueprintValidationResult validation = graph.Validate();
    if (validation.HasErrors())
    {
        diagnostics_ = validation.diagnostics;
        hadRuntimeError_ = true;
        return false;
    }

    for (const BlueprintNode& node : graph.GetNodes())
    {
        if (node.typeName == eventType)
        {
            debugGraph_ = &graph;
            debugCurrentNode_ = node.id;
            debugSteps_ = 0;
            values_.clear();
            variables_ = ea::move(variables);
            functionCallStack_.clear();
            diagnostics_.clear();
            hadRuntimeError_ = false;
            return true;
        }
    }

    AddError(BLUEPRINT_INVALID_ID, "BP101", Format("Debug event '{}' was not found.", eventType));
    return false;
}

bool BlueprintRuntime::StepDebug()
{
    if (!IsDebugActive())
        return false;
    if (++debugSteps_ > 10000)
    {
        AddError(debugCurrentNode_, "BP102", "Blueprint debug execution exceeded the safety step limit.");
        StopDebug();
        return false;
    }

    const BlueprintNode* node = debugGraph_->GetNode(debugCurrentNode_);
    const BlueprintNodeDefinition* definition = node ? registry_.Find(node->typeName) : nullptr;
    if (!node || !definition)
    {
        AddError(debugCurrentNode_, "BP104", "Debugger reached an unknown node.");
        StopDebug();
        return false;
    }

    BlueprintExecutionContext context(*this, *debugGraph_, *node, values_, variables_);
    definition->execute(context);
    if (hadRuntimeError_)
    {
        StopDebug();
        return false;
    }

    if (IsPureNode(*node, *definition) || context.GetContinuationPin().empty())
    {
        debugCurrentNode_ = BLUEPRINT_INVALID_ID;
        return true;
    }

    const BlueprintLink* next = FindOutgoingExecutionLink(*debugGraph_, node->id, context.GetContinuationPin());
    debugCurrentNode_ = next ? next->toNode : BLUEPRINT_INVALID_ID;
    return true;
}

void BlueprintRuntime::StopDebug()
{
    debugGraph_ = nullptr;
    debugCurrentNode_ = BLUEPRINT_INVALID_ID;
    debugSteps_ = 0;
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
