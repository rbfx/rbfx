// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintRuntime.h"
#include "BlueprintReflection.h"

#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics2D/PhysicsWorld2D.h>
#include <Urho3D/Physics2D/RigidBody2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/WorldPartition.h>
#include <Urho3D/Replica/RpcDispatcher.h>
#include <Urho3D/Replica/RelevancyManager.h>
#include <Urho3D/Network/DedicatedServer.h>
#include <Urho3D/Profiler/ProductionProfiler.h>
#include <Urho3D/Animation/AnimationStateMachine.h>
#include <Urho3D/Animation/Sequencer.h>
#include <Urho3D/AI/Blackboard.h>
#include <Urho3D/AI/BehaviorTree.h>
#include <Urho3D/AI/EQS.h>
#include <Urho3D/AI/PerceptionSystem.h>
#include <Urho3D/AI/StateTree.h>

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

ea::vector<BlueprintPin> BuiltinPins(const char* typeName)
{
    const ea::string name(typeName);
    ea::vector<BlueprintPin> pins;
    const auto executionInput = [&]() { pins.push_back(RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard)); };
    const auto executionOutput = [&](const char* pin = "then") {
        pins.push_back(RuntimePin(pin, BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard));
    };
    const auto floatInput = [&](const char* pin) {
        pins.push_back(RuntimePin(pin, BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f)));
    };

    if (name == "Event.OnStart")
        executionOutput();
    else if (name == "Flow.Branch")
    {
        executionInput();
        pins.push_back(RuntimePin("true", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard));
        pins.push_back(RuntimePin("false", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard));
        pins.push_back(RuntimePin("condition", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(false)));
    }
    else if (name == "Flow.Print")
    {
        executionInput();
        executionOutput();
        pins.push_back(RuntimePin("message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("Message"))));
    }
    else if (name == "Function.Return")
        executionInput();
    else if (name == "Function.RbScript")
    {
        executionInput();
        executionOutput();
    }
    else if (name == "Math.AddFloat" || name == "Math.MultiplyFloat" || name == "Math.LessFloat"
        || name == "Math.SubtractFloat" || name == "Math.DivideFloat")
    {
        floatInput("a");
        floatInput("b");
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output,
            name == "Math.LessFloat" ? BlueprintDataType::Bool : BlueprintDataType::Float));
    }
    else if (name == "Math.ClampFloat")
    {
        floatInput("value");
        floatInput("min");
        floatInput("max");
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Float));
    }
    else if (name == "Math.LerpFloat")
    {
        floatInput("a");
        floatInput("b");
        floatInput("t");
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Float));
    }
    else if (name == "Math.SinFloat" || name == "Math.CosFloat")
    {
        floatInput("value");
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Float));
    }
    else if (name == "Math.AddVector3")
    {
        pins.push_back(RuntimePin("a", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)));
        pins.push_back(RuntimePin("b", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)));
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Vector3));
    }
    else if (name == "Math.ScaleVector3")
    {
        pins.push_back(RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)));
        floatInput("scale");
        pins.push_back(RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Vector3));
    }
    else if (name == "Variable.Get")
        pins.push_back(RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant));
    else if (name == "Variable.Set")
    {
        executionInput();
        executionOutput();
        pins.push_back(RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant));
    }
    return pins;
}

ea::string GetNodeString(BlueprintExecutionContext& context, const char* propertyName, const char* inputName)
{
    const auto property = context.GetNode().properties.find(propertyName);
    if (property != context.GetNode().properties.end() && property->second.GetType() == VAR_STRING)
        return property->second.GetString();
    return context.GetInput(inputName).GetString();
}

StringVariantMap CollectCallableInputs(BlueprintExecutionContext& context, const char* excludedName)
{
    StringVariantMap inputs;
    for (const BlueprintPin& pin : context.GetNode().pins)
    {
        if (pin.kind == BlueprintPinKind::Input && pin.name != excludedName)
            inputs[pin.name] = context.GetInput(pin.name);
    }
    return inputs;
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

Material* GetTargetMaterial(BlueprintExecutionContext& context, unsigned index)
{
    Node* node = GetTargetNode(context);
    if (!node)
        return nullptr;
    StaticModel* model = node->GetComponent<StaticModel>();
    if (!model)
    {
        context.ReportError("BPMAT001", "This material node requires a StaticModel or AnimatedModel target.");
        return nullptr;
    }
    Material* material = model->GetMaterial(index);
    if (!material)
        context.ReportError("BPMAT002", "The requested material slot is not available on the target model.");
    return material;
}

template <class T> T* GetTargetResource(BlueprintExecutionContext& context, const ea::string& name, const char* errorCode,
    const char* errorMessage)
{
    Node* node = GetTargetNode(context);
    if (!node)
        return nullptr;
    ResourceCache* cache = node->GetContext()->GetSubsystem<ResourceCache>();
    T* resource = cache && !name.empty() ? cache->GetResource<T>(name) : nullptr;
    if (!resource)
        context.ReportError(errorCode, errorMessage);
    return resource;
}

void RegisterDefinition(BlueprintNodeRegistry& registry, const char* typeName, const char* category,
    BlueprintExecutionMode mode, BlueprintNodeExecutor executor, const char* description,
    ea::vector<BlueprintPin> pins = {})
{
    BlueprintNodeDefinition definition;
    definition.typeName = typeName;
    definition.category = category;
    definition.executionMode = mode;
    if (pins.empty())
        pins = BuiltinPins(typeName);
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

bool BlueprintExecutionContext::CallFunction(const ea::string& functionName, const StringVariantMap& inputs,
    StringVariantMap* outputs)
{
    return runtime_.ExecuteFunction(graph_, functionName, inputs, outputs);
}

void BlueprintExecutionContext::Delay(float seconds)
{
    runtime_.ScheduleDelay(seconds);
}

BlueprintRuntime::BlueprintRuntime()
{
    RegisterBuiltinNodes();
}

bool BlueprintRuntime::BindDelegate(const ea::string& delegateName, const ea::string& functionName)
{
    if (delegateName.empty() || functionName.empty())
        return false;
    delegateBindings_[delegateName] = functionName;
    return true;
}

bool BlueprintRuntime::UnbindDelegate(const ea::string& delegateName)
{
    return delegateBindings_.erase(delegateName) != 0;
}

bool BlueprintRuntime::IsDelegateBound(const ea::string& delegateName) const
{
    return delegateBindings_.find(delegateName) != delegateBindings_.end();
}

bool BlueprintRuntime::InvokeDelegate(const BlueprintGraph& graph, const ea::string& delegateName,
    const StringVariantMap& inputs, StringVariantMap* outputs)
{
    const auto binding = delegateBindings_.find(delegateName);
    if (binding == delegateBindings_.end())
        return false;
    return ExecuteFunction(graph, binding->second, inputs, outputs);
}

bool BlueprintRuntime::InvokeMacro(const BlueprintGraph& graph, const ea::string& macroName,
    const StringVariantMap& inputs, StringVariantMap* outputs)
{
    return ExecuteFunction(graph, macroName, inputs, outputs);
}

bool BlueprintRuntime::PlayTimeline(const BlueprintGraph& graph, const ea::string& timelineName)
{
    const BlueprintTimeline* timeline = graph.GetTimeline(timelineName);
    if (!timeline)
        return false;
    timelineGraph_ = &graph;
    timelinePositions_[timelineName] = 0.0f;
    activeTimelines_.insert(timelineName);
    return true;
}

bool BlueprintRuntime::PauseTimeline(const ea::string& timelineName)
{
    return activeTimelines_.erase(timelineName) != 0;
}

bool BlueprintRuntime::StopTimeline(const ea::string& timelineName)
{
    const bool existed = activeTimelines_.erase(timelineName) != 0 || timelinePositions_.find(timelineName) != timelinePositions_.end();
    timelinePositions_.erase(timelineName);
    if (activeTimelines_.empty())
        timelineGraph_ = nullptr;
    return existed;
}

bool BlueprintRuntime::IsTimelinePlaying(const ea::string& timelineName) const
{
    return activeTimelines_.find(timelineName) != activeTimelines_.end();
}

Variant BlueprintRuntime::GetTimelineValue(const BlueprintGraph& graph, const ea::string& timelineName) const
{
    const BlueprintTimeline* timeline = graph.GetTimeline(timelineName);
    if (!timeline || timeline->keyframes.empty())
        return Variant();
    const auto position = timelinePositions_.find(timelineName);
    const float time = position != timelinePositions_.end() ? position->second : 0.0f;
    if (time <= timeline->keyframes.front().time)
        return timeline->keyframes.front().value;
    if (time >= timeline->keyframes.back().time)
        return timeline->keyframes.back().value;
    for (unsigned i = 1; i < timeline->keyframes.size(); ++i)
    {
        const BlueprintTimelineKeyframe& right = timeline->keyframes[i];
        const BlueprintTimelineKeyframe& left = timeline->keyframes[i - 1];
        if (time <= right.time)
        {
            const float span = right.time - left.time;
            const float alpha = span > 0.0f ? (time - left.time) / span : 1.0f;
            if (left.value.GetType() == VAR_FLOAT && right.value.GetType() == VAR_FLOAT)
                return Lerp(left.value.GetFloat(), right.value.GetFloat(), alpha);
            return alpha < 0.5f ? left.value : right.value;
        }
    }
    return timeline->keyframes.back().value;
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

    RegisterDefinition(registry_, "Flow.Delay", "Flow Control", BlueprintExecutionMode::Latent,
        [](BlueprintExecutionContext& context)
        {
            context.Delay(context.GetInput("duration").GetFloat());
            context.ContinueWith("completed");
        }, "Pause execution and resume on a later Tick.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("completed", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("duration", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f))});

    RegisterDefinition(registry_, "Event.OnKeyPressed", "Events/Input", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("key", context.GetVariable("__event.key"));
            context.ContinueWith("then");
        }, "Input event carrying the pressed key in the __event.key variable.",
        {RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("key", BlueprintPinKind::Output, BlueprintDataType::Int)});

    RegisterDefinition(registry_, "Event.OnMouseClick", "Events/Input", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("button", context.GetVariable("__event.button"));
            context.SetOutput("x", context.GetVariable("__event.x"));
            context.SetOutput("y", context.GetVariable("__event.y"));
            context.ContinueWith("then");
        }, "Mouse event carrying button and coordinates in __event.button, __event.x and __event.y.",
        {RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("button", BlueprintPinKind::Output, BlueprintDataType::Int),
         RuntimePin("x", BlueprintPinKind::Output, BlueprintDataType::Int),
         RuntimePin("y", BlueprintPinKind::Output, BlueprintDataType::Int)});

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

            StringVariantMap inputs;
            const BlueprintFunction* function = context.GetGraph().GetFunction(iter->second.GetString());
            if (function)
            {
                for (const BlueprintPin& pin : function->inputs)
                {
                    if (pin.kind != BlueprintPinKind::Input)
                        continue;
                    inputs[pin.name] = context.GetInput(pin.name);
                }
            }
            StringVariantMap outputs;
            if (context.CallFunction(iter->second.GetString(), inputs, &outputs))
            {
                for (const auto& output : outputs)
                    context.SetOutput(output.first, output.second);
                context.ContinueWith("then");
            }
        }, "Execute a user-defined Blueprint function/subgraph with typed input and output pins.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard)});

    RegisterDefinition(registry_, "Function.RbScript", "Functions/RbScript", BlueprintExecutionMode::Immediate,
        [this](BlueprintExecutionContext& context)
        {
            const auto iter = context.GetNode().properties.find("functionName");
            if (iter == context.GetNode().properties.end() || iter->second.GetString().empty())
            {
                context.ReportError("BP210", "Function.RbScript requires a non-empty functionName property.");
                return;
            }
            if (!rbScriptInvoker_)
            {
                context.ReportError("BP211", "Function.RbScript has no rbscript invoker configured.");
                return;
            }

            StringVariantMap inputs;
            for (const BlueprintPin& pin : context.GetNode().pins)
            {
                if (pin.kind == BlueprintPinKind::Input)
                    inputs[pin.name] = context.GetInput(pin.name);
            }

            StringVariantMap outputs;
            if (rbScriptInvoker_(iter->second.GetString(), inputs, outputs))
            {
                for (const auto& output : outputs)
                    context.SetOutput(output.first, output.second);
                context.ContinueWith("then");
            }
            else
                context.ReportError("BP212", "The rbscript function invocation failed.");
        }, "Invoke a compiled rbscript function through the configured host bridge.",
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

    RegisterDefinition(registry_, "Math.SubtractFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("a").GetFloat() - context.GetInput("b").GetFloat());
        }, "Subtract two floating-point values.");

    RegisterDefinition(registry_, "Math.DivideFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const float divisor = context.GetInput("b").GetFloat();
            if (Equals(divisor, 0.0f))
            {
                context.ReportError("BP301", "Math.DivideFloat cannot divide by zero.");
                return;
            }
            context.SetOutput("result", context.GetInput("a").GetFloat() / divisor);
        }, "Divide two floating-point values with zero protection.");

    RegisterDefinition(registry_, "Math.ClampFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const float value = context.GetInput("value").GetFloat();
            const float minimum = context.GetInput("min").GetFloat();
            const float maximum = context.GetInput("max").GetFloat();
            context.SetOutput("result", Clamp(value, minimum, maximum));
        }, "Clamp a floating-point value to a range.");

    RegisterDefinition(registry_, "Math.LerpFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const float a = context.GetInput("a").GetFloat();
            const float b = context.GetInput("b").GetFloat();
            const float t = context.GetInput("t").GetFloat();
            context.SetOutput("result", Lerp(a, b, t));
        }, "Linearly interpolate two floating-point values.");

    RegisterDefinition(registry_, "Math.SinFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", Sin(context.GetInput("value").GetFloat()));
        }, "Compute the sine of a value in radians.");

    RegisterDefinition(registry_, "Math.CosFloat", "Math", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", Cos(context.GetInput("value").GetFloat()));
        }, "Compute the cosine of a value in radians.");

    RegisterDefinition(registry_, "Math.AddVector3", "Math/Vector", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("a").GetVector3() + context.GetInput("b").GetVector3());
        }, "Add two Vector3 values.");

    RegisterDefinition(registry_, "Math.ScaleVector3", "Math/Vector", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("result", context.GetInput("value").GetVector3() * context.GetInput("scale").GetFloat());
        }, "Scale a Vector3 by a scalar.");

    RegisterDefinition(registry_, "Array.Make", "Collections/Array", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const Variant input = context.GetInput("values");
            VariantVector result = input.GetVariantVector();
            for (const BlueprintPin& pin : context.GetNode().pins)
            {
                if (pin.kind == BlueprintPinKind::Input && pin.name != "values")
                    result.push_back(context.GetInput(pin.name));
            }
            context.SetOutput("array", Variant(result));
        }, "Create a Variant array from an optional values pin and additional input pins.",
        {RuntimePin("values", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("array", BlueprintPinKind::Output, BlueprintDataType::Array)});

    RegisterDefinition(registry_, "Array.Get", "Collections/Array", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const Variant arrayInput = context.GetInput("array");
            const VariantVector& array = arrayInput.GetVariantVector();
            const unsigned index = context.GetInput("index").GetUInt();
            const bool valid = index < array.size();
            context.SetOutput("valid", valid);
            if (valid)
                context.SetOutput("value", array[index]);
        }, "Read an array element with a bounds check.",
        {RuntimePin("array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("index", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0)),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant),
         RuntimePin("valid", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Array.Set", "Collections/Array", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const Variant arrayInput = context.GetInput("array");
            VariantVector result = arrayInput.GetVariantVector();
            const unsigned index = context.GetInput("index").GetUInt();
            if (index >= result.size())
            {
                context.ReportError("BP410", "Array.Set index is out of bounds.");
                return;
            }
            result[index] = context.GetInput("value");
            context.SetOutput("result", Variant(result));
            context.ContinueWith("then");
        }, "Assign an existing array element after checking its index.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("index", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0)),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Array)});

    RegisterDefinition(registry_, "Array.Length", "Collections/Array", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const Variant arrayInput = context.GetInput("array");
            context.SetOutput("length", static_cast<int>(arrayInput.GetVariantVector().size()));
        }, "Return the number of elements in an array.",
        {RuntimePin("array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("length", BlueprintPinKind::Output, BlueprintDataType::Int)});

    RegisterDefinition(registry_, "Array.Add", "Collections/Array", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const Variant arrayInput = context.GetInput("array");
            VariantVector result = arrayInput.GetVariantVector();
            result.push_back(context.GetInput("value"));
            context.SetOutput("result", Variant(result));
            context.ContinueWith("then");
        }, "Append one element to an array and return the new array.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Array)});

    RegisterDefinition(registry_, "Array.Remove", "Collections/Array", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const Variant arrayInput = context.GetInput("array");
            VariantVector result = arrayInput.GetVariantVector();
            const unsigned index = context.GetInput("index").GetUInt();
            if (index >= result.size())
            {
                context.ReportError("BP411", "Array.Remove index is out of bounds.");
                return;
            }
            result.erase_at(index);
            context.SetOutput("result", Variant(result));
            context.ContinueWith("then");
        }, "Remove an array element after checking its index.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(VariantVector{})),
         RuntimePin("index", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0)),
         RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Array)});

    RegisterDefinition(registry_, "Map.Make", "Collections/Map", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("map", Variant(context.GetInput("entries").GetStringVariantMap()));
        }, "Create a string-keyed Variant map from entries.",
        {RuntimePin("entries", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(StringVariantMap{})),
         RuntimePin("map", BlueprintPinKind::Output, BlueprintDataType::Map)});

    RegisterDefinition(registry_, "Map.Get", "Collections/Map", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const Variant mapInput = context.GetInput("map");
            const StringVariantMap& map = mapInput.GetStringVariantMap();
            const auto iter = map.find(context.GetInput("key").GetString());
            const bool found = iter != map.end();
            context.SetOutput("found", found);
            if (found)
                context.SetOutput("value", iter->second);
        }, "Read a string-keyed map entry with a found flag.",
        {RuntimePin("map", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(StringVariantMap{})),
         RuntimePin("key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant),
         RuntimePin("found", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Map.Set", "Collections/Map", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const Variant mapInput = context.GetInput("map");
            StringVariantMap result = mapInput.GetStringVariantMap();
            result[context.GetInput("key").GetString()] = context.GetInput("value");
            context.SetOutput("result", Variant(result));
            context.ContinueWith("then");
        }, "Assign a string-keyed map entry and return the new map.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("map", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(StringVariantMap{})),
         RuntimePin("key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Map)});

    RegisterDefinition(registry_, "Map.Contains", "Collections/Map", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const Variant mapInput = context.GetInput("map");
            const StringVariantMap& map = mapInput.GetStringVariantMap();
            context.SetOutput("contains", map.find(context.GetInput("key").GetString()) != map.end());
        }, "Check whether a string-keyed map contains a key.",
        {RuntimePin("map", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(StringVariantMap{})),
         RuntimePin("key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("contains", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Struct.Make", "Types/Struct", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const auto type = context.GetNode().properties.find("structName");
            if (type == context.GetNode().properties.end() || type->second.GetString().empty())
            {
                context.ReportError("BP420", "Struct.Make requires a structName property.");
                return;
            }
            const BlueprintStructDef* definition = context.GetGraph().GetStruct(type->second.GetString());
            if (!definition)
            {
                context.ReportError("BP421", "Struct.Make references an unknown Blueprint struct.");
                return;
            }
            VariantMap result;
            for (const BlueprintStructField& field : definition->fields)
            {
                const BlueprintPin* pin = nullptr;
                for (const BlueprintPin& candidate : context.GetNode().pins)
                {
                    if (candidate.name == field.name)
                    {
                        pin = &candidate;
                        break;
                    }
                }
                result[StringHash(field.name)] = pin ? context.GetInput(field.name) : field.defaultValue;
            }
            context.SetOutput("value", Variant(result));
        }, "Construct a user-defined Blueprint struct from its field pins.");

    RegisterDefinition(registry_, "Struct.Get", "Types/Struct", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const auto field = context.GetNode().properties.find("fieldName");
            if (field == context.GetNode().properties.end() || field->second.GetString().empty())
            {
                context.ReportError("BP422", "Struct.Get requires a fieldName property.");
                return;
            }
            const Variant structInput = context.GetInput("struct");
            const VariantMap& value = structInput.GetVariantMap();
            const auto iter = value.find(StringHash(field->second.GetString()));
            if (iter != value.end())
                context.SetOutput("value", iter->second);
        }, "Read a named field from a Blueprint struct value.",
        {RuntimePin("struct", BlueprintPinKind::Input, BlueprintDataType::Struct, Variant(VariantMap{})),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant)});

    RegisterDefinition(registry_, "Struct.Set", "Types/Struct", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const auto field = context.GetNode().properties.find("fieldName");
            if (field == context.GetNode().properties.end() || field->second.GetString().empty())
            {
                context.ReportError("BP423", "Struct.Set requires a fieldName property.");
                return;
            }
            const Variant structInput = context.GetInput("struct");
            VariantMap result = structInput.GetVariantMap();
            result[StringHash(field->second.GetString())] = context.GetInput("value");
            context.SetOutput("result", Variant(result));
            context.ContinueWith("then");
        }, "Assign a named field and return the updated Blueprint struct.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("struct", BlueprintPinKind::Input, BlueprintDataType::Struct, Variant(VariantMap{})),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("result", BlueprintPinKind::Output, BlueprintDataType::Struct)});

    RegisterDefinition(registry_, "Enum.Value", "Types/Enum", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const auto enumName = context.GetNode().properties.find("enumName");
            const auto valueName = context.GetNode().properties.find("valueName");
            if (enumName == context.GetNode().properties.end() || valueName == context.GetNode().properties.end())
            {
                context.ReportError("BP430", "Enum.Value requires enumName and valueName properties.");
                return;
            }
            const BlueprintEnumDef* definition = context.GetGraph().GetEnum(enumName->second.GetString());
            if (!definition)
            {
                context.ReportError("BP431", "Enum.Value references an unknown Blueprint enum.");
                return;
            }
            for (const BlueprintEnumValue& value : definition->values)
            {
                if (value.name == valueName->second.GetString())
                {
                    context.SetOutput("value", value.value);
                    return;
                }
            }
            context.ReportError("BP432", "Enum.Value references an unknown enum constant.");
        }, "Return a named constant from a user-defined Blueprint enum.",
        {RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Enum)});

    RegisterDefinition(registry_, "Delegate.Bind", "Events/Delegate", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string delegateName = GetNodeString(context, "delegateName", "delegate");
            const ea::string functionName = GetNodeString(context, "functionName", "function");
            const bool bound = context.GetRuntime().BindDelegate(delegateName, functionName);
            context.SetOutput("bound", bound);
            if (!bound)
                context.ReportError("BP440", "Delegate.Bind requires non-empty delegate and function names.");
            else
                context.ContinueWith("then");
        }, "Bind a delegate or signal to a Blueprint function.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("delegate", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("function", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("bound", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Delegate.Unbind", "Events/Delegate", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string delegateName = GetNodeString(context, "delegateName", "delegate");
            const bool unbound = context.GetRuntime().UnbindDelegate(delegateName);
            context.SetOutput("unbound", unbound);
            if (unbound)
                context.ContinueWith("then");
        }, "Remove a delegate or signal binding.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("delegate", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("unbound", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Delegate.Call", "Events/Delegate", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string delegateName = GetNodeString(context, "delegateName", "delegate");
            if (!context.GetGraph().GetDelegate(delegateName))
            {
                context.ReportError("BP441", "Delegate.Call references an unknown delegate.");
                return;
            }
            const StringVariantMap inputs = CollectCallableInputs(context, "delegate");
            const bool called = context.GetRuntime().InvokeDelegate(context.GetGraph(), delegateName, inputs);
            context.SetOutput("called", called);
            if (called)
                context.ContinueWith("then");
        }, "Invoke a bound delegate or signal function.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("delegate", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("called", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Delegate.IsBound", "Events/Delegate", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const ea::string delegateName = GetNodeString(context, "delegateName", "delegate");
            context.SetOutput("bound", context.GetRuntime().IsDelegateBound(delegateName));
        }, "Check whether a delegate or signal currently has a binding.",
        {RuntimePin("delegate", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("bound", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Signal.Connect", "Events/Signal", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string signalName = GetNodeString(context, "signalName", "signal");
            const ea::string functionName = GetNodeString(context, "functionName", "function");
            const bool connected = context.GetRuntime().BindDelegate(signalName, functionName);
            context.SetOutput("connected", connected);
            if (connected)
                context.ContinueWith("then");
        }, "Connect a Signal to a Blueprint function.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("signal", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("function", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("connected", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Signal.Disconnect", "Events/Signal", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string signalName = GetNodeString(context, "signalName", "signal");
            const bool disconnected = context.GetRuntime().UnbindDelegate(signalName);
            context.SetOutput("disconnected", disconnected);
            if (disconnected)
                context.ContinueWith("then");
        }, "Disconnect a Signal from its Blueprint function.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("signal", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("disconnected", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Signal.Emit", "Events/Signal", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string signalName = GetNodeString(context, "signalName", "signal");
            if (!context.GetGraph().GetDelegate(signalName))
            {
                context.ReportError("BP442", "Signal.Emit references an unknown signal.");
                return;
            }
            const StringVariantMap inputs = CollectCallableInputs(context, "signal");
            const bool emitted = context.GetRuntime().InvokeDelegate(context.GetGraph(), signalName, inputs);
            context.SetOutput("emitted", emitted);
            if (emitted)
                context.ContinueWith("then");
        }, "Emit a connected Signal.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("signal", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("emitted", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Signal.IsConnected", "Events/Signal", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const ea::string signalName = GetNodeString(context, "signalName", "signal");
            context.SetOutput("connected", context.GetRuntime().IsDelegateBound(signalName));
        }, "Check whether a Signal has a connected Blueprint function.",
        {RuntimePin("signal", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("connected", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Timeline.Play", "Animation/Timeline", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string timelineName = GetNodeString(context, "timelineName", "timeline");
            const bool playing = context.GetRuntime().PlayTimeline(context.GetGraph(), timelineName);
            context.SetOutput("playing", playing);
            if (playing)
                context.ContinueWith("then");
        }, "Start a Blueprint timeline from its first keyframe.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("timeline", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("playing", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Timeline.Pause", "Animation/Timeline", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string timelineName = GetNodeString(context, "timelineName", "timeline");
            const bool paused = context.GetRuntime().PauseTimeline(timelineName);
            context.SetOutput("paused", paused);
            if (paused)
                context.ContinueWith("then");
        }, "Pause a Blueprint timeline while preserving its current position.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("timeline", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("paused", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Timeline.Stop", "Animation/Timeline", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string timelineName = GetNodeString(context, "timelineName", "timeline");
            const bool stopped = context.GetRuntime().StopTimeline(timelineName);
            context.SetOutput("stopped", stopped);
            if (stopped)
                context.ContinueWith("then");
        }, "Stop a Blueprint timeline and reset its position.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("timeline", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("stopped", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Timeline.GetValue", "Animation/Timeline", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const ea::string timelineName = GetNodeString(context, "timelineName", "timeline");
            context.SetOutput("value", context.GetRuntime().GetTimelineValue(context.GetGraph(), timelineName));
        }, "Read the interpolated value of a Blueprint timeline.",
        {RuntimePin("timeline", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant)});

    RegisterDefinition(registry_, "Anim.PlayStateMachine", "Animation/State Machine", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            AnimationStateMachine* machine = context.GetRuntime().GetAnimationStateMachine();
            if (!machine)
            {
                context.ReportError("BP460", "Anim.PlayStateMachine requires an injected AnimationStateMachine.");
                return;
            }
            const ea::string state = context.GetInput("state").GetString();
            const bool played = machine->Start(state);
            context.SetOutput("played", played);
            if (played)
                context.ContinueWith("then");
        }, "Start an injected native animation state machine.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("state", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("played", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Anim.SetParameter", "Animation/State Machine", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            AnimationStateMachine* machine = context.GetRuntime().GetAnimationStateMachine();
            if (!machine)
            {
                context.ReportError("BP461", "Anim.SetParameter requires an injected AnimationStateMachine.");
                return;
            }
            const bool changed = machine->SetParameter(context.GetInput("parameter").GetString(), context.GetInput("value"));
            context.SetOutput("changed", changed);
            if (changed)
                context.ContinueWith("then");
        }, "Set a typed parameter consumed by animation transition conditions.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("parameter", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("changed", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Anim.GetCurrentState", "Animation/State Machine", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            AnimationStateMachine* machine = context.GetRuntime().GetAnimationStateMachine();
            context.SetOutput("state", machine ? Variant(machine->GetCurrentState()) : Variant(ea::string()));
        }, "Read the active state from the injected animation state machine.",
        {RuntimePin("state", BlueprintPinKind::Output, BlueprintDataType::String)});

    RegisterDefinition(registry_, "Seq.Play", "Animation/Sequencer", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Sequencer* sequencer = context.GetRuntime().GetSequencer();
            if (!sequencer)
            {
                context.ReportError("BP470", "Seq.Play requires an injected Sequencer.");
                return;
            }
            const bool played = sequencer->Play();
            context.SetOutput("played", played);
            if (played)
                context.ContinueWith("then");
        }, "Start the injected cinematic sequencer from time zero.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("played", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Seq.Pause", "Animation/Sequencer", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Sequencer* sequencer = context.GetRuntime().GetSequencer();
            if (!sequencer)
            {
                context.ReportError("BP471", "Seq.Pause requires an injected Sequencer.");
                return;
            }
            const bool paused = sequencer->Pause();
            context.SetOutput("paused", paused);
            if (paused)
                context.ContinueWith("then");
        }, "Pause the injected cinematic sequencer without losing its position.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("paused", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Seq.Seek", "Animation/Sequencer", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Sequencer* sequencer = context.GetRuntime().GetSequencer();
            if (!sequencer)
            {
                context.ReportError("BP472", "Seq.Seek requires an injected Sequencer.");
                return;
            }
            const bool moved = sequencer->Seek(context.GetInput("time").GetFloat());
            context.SetOutput("moved", moved);
            if (moved)
                context.ContinueWith("then");
        }, "Scrub the injected cinematic sequencer to a validated time.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("time", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f)),
         RuntimePin("moved", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Seq.GetPosition", "Animation/Sequencer", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Sequencer* sequencer = context.GetRuntime().GetSequencer();
            context.SetOutput("time", sequencer ? Variant(sequencer->GetPosition()) : Variant(0.0f));
        }, "Read the current cinematic sequencer position.",
        {RuntimePin("time", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "AI.RunBehaviorTree", "Gameplay/AI", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            BehaviorTree* tree = context.GetRuntime().GetBehaviorTree();
            if (!tree)
            {
                context.ReportError("BP480", "AI.RunBehaviorTree requires an injected BehaviorTree.");
                return;
            }
            BehaviorTreeTickContext tick;
            tick.blackboard = context.GetRuntime().GetBlackboard();
            tick.deltaSeconds = Max(context.GetInput("deltaSeconds").GetFloat(), 0.0f);
            const BehaviorStatus status = tree->Tick(tick);
            context.SetOutput("status", Variant(static_cast<int>(status)));
            context.SetOutput("running", status == BehaviorStatus::Running);
            context.SetOutput("succeeded", status == BehaviorStatus::Success);
            if (status != BehaviorStatus::Failure && status != BehaviorStatus::Invalid)
                context.ContinueWith("then");
        }, "Tick the injected gameplay behavior tree.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("deltaSeconds", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.016f)),
         RuntimePin("status", BlueprintPinKind::Output, BlueprintDataType::Int),
         RuntimePin("running", BlueprintPinKind::Output, BlueprintDataType::Bool),
         RuntimePin("succeeded", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "AI.SetBlackboardValue", "Gameplay/AI", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Blackboard* blackboard = context.GetRuntime().GetBlackboard();
            if (!blackboard)
            {
                context.ReportError("BP481", "AI.SetBlackboardValue requires an injected Blackboard.");
                return;
            }
            const bool changed = blackboard->Set(context.GetInput("key").GetString(), context.GetInput("value"));
            context.SetOutput("changed", changed);
            if (changed)
                context.ContinueWith("then");
        }, "Write a typed value into the injected AI Blackboard.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant),
         RuntimePin("changed", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "AI.GetBlackboardValue", "Gameplay/AI", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Blackboard* blackboard = context.GetRuntime().GetBlackboard();
            context.SetOutput("value", blackboard ? blackboard->Get(context.GetInput("key").GetString()) : Variant());
        }, "Read a value from the injected AI Blackboard.",
        {RuntimePin("key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant)});

    RegisterDefinition(registry_, "AI.QueryEQS", "Gameplay/AI", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            EQS* eqs = context.GetRuntime().GetEQS();
            if (!eqs)
            {
                context.ReportError("BP482", "AI.QueryEQS requires an injected EQS service.");
                return;
            }
            const EQSQueryResult result = eqs->Query(context.GetInput("origin").GetVector3(),
                Max(context.GetInput("radius").GetFloat(), 0.0f), context.GetRuntime().GetBlackboard());
            const EQSResultItem* best = result.GetBest();
            context.SetOutput("best", best ? Variant(best->item.position) : Variant(Vector3::ZERO));
            context.SetOutput("found", result.found);
            if (result.found)
                context.ContinueWith("then");
        }, "Run a deterministic spatial Environment Query and return its best item.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("origin", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)),
         RuntimePin("radius", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(100.0f)),
         RuntimePin("best", BlueprintPinKind::Output, BlueprintDataType::Vector3),
         RuntimePin("found", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Macro.Call", "Flow/Macro", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            const ea::string macroName = GetNodeString(context, "macroName", "macro");
            if (!context.GetGraph().GetMacro(macroName))
            {
                context.ReportError("BP450", "Macro.Call references an unknown Blueprint macro.");
                return;
            }
            const StringVariantMap inputs = CollectCallableInputs(context, "macro");
            const bool called = context.GetRuntime().InvokeMacro(context.GetGraph(), macroName, inputs);
            context.SetOutput("called", called);
            if (called)
                context.ContinueWith("then");
        }, "Expand and execute a user-defined Blueprint macro.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("macro", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("called", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Physics.ApplyForce", "Gameplay/Physics", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node)
                return;
            const Vector3 force = context.GetInput("force").GetVector3();
            if (RigidBody* body = node->GetComponent<RigidBody>())
                body->ApplyForce(force);
            else if (RigidBody2D* body2d = node->GetComponent<RigidBody2D>())
                body2d->ApplyForceToCenter(Vector2(force.x_, force.y_), true);
            else
            {
                context.ReportError("BPPHY001", "Physics.ApplyForce requires a RigidBody or RigidBody2D target.");
                return;
            }
            context.ContinueWith("then");
        }, "Apply a force to a 2D or 3D rigid body.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("force", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO))});

    RegisterDefinition(registry_, "Physics.ApplyImpulse", "Gameplay/Physics", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node)
                return;
            const Vector3 impulse = context.GetInput("impulse").GetVector3();
            if (RigidBody* body = node->GetComponent<RigidBody>())
                body->ApplyImpulse(impulse);
            else if (RigidBody2D* body2d = node->GetComponent<RigidBody2D>())
                body2d->ApplyLinearImpulseToCenter(Vector2(impulse.x_, impulse.y_), true);
            else
            {
                context.ReportError("BPPHY002", "Physics.ApplyImpulse requires a RigidBody or RigidBody2D target.");
                return;
            }
            context.ContinueWith("then");
        }, "Apply an impulse to a 2D or 3D rigid body.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("impulse", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO))});

    RegisterDefinition(registry_, "Physics.SetVelocity", "Gameplay/Physics", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node)
                return;
            const Vector3 velocity = context.GetInput("velocity").GetVector3();
            if (RigidBody* body = node->GetComponent<RigidBody>())
                body->SetLinearVelocity(velocity);
            else if (RigidBody2D* body2d = node->GetComponent<RigidBody2D>())
                body2d->SetLinearVelocity(Vector2(velocity.x_, velocity.y_));
            else
            {
                context.ReportError("BPPHY003", "Physics.SetVelocity requires a RigidBody or RigidBody2D target.");
                return;
            }
            context.ContinueWith("then");
        }, "Set linear velocity on a 2D or 3D rigid body.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("velocity", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO))});

    RegisterDefinition(registry_, "Physics.GetVelocity", "Gameplay/Physics", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node)
                return;
            if (RigidBody* body = node->GetComponent<RigidBody>())
                context.SetOutput("velocity", body->GetLinearVelocity());
            else if (RigidBody2D* body2d = node->GetComponent<RigidBody2D>())
            {
                const Vector2 velocity = body2d->GetLinearVelocity();
                context.SetOutput("velocity", Vector3(velocity.x_, velocity.y_, 0.0f));
            }
            else
                context.ReportError("BPPHY004", "Physics.GetVelocity requires a RigidBody or RigidBody2D target.");
        }, "Read linear velocity from a 2D or 3D rigid body.",
        {RuntimePin("velocity", BlueprintPinKind::Output, BlueprintDataType::Vector3)});

    RegisterDefinition(registry_, "Physics.RayCast", "Gameplay/Physics", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node || !node->GetScene())
                return;
            const Vector3 start = context.GetInput("start").GetVector3();
            const Vector3 end = context.GetInput("end").GetVector3();
            const unsigned mask = context.GetInput("collisionMask").GetUInt();
            bool hit = false;
            PhysicsRaycastResult result;
            if (PhysicsWorld* world = node->GetScene()->GetComponent<PhysicsWorld>())
            {
                world->RaycastSingle(result, Ray(start, end - start), (end - start).Length(), mask);
                hit = result.body_ != nullptr;
                context.SetOutput("position", result.position_);
                context.SetOutput("normal", result.normal_);
                context.SetOutput("distance", result.distance_);
            }
            else if (PhysicsWorld2D* world2d = node->GetScene()->GetComponent<PhysicsWorld2D>())
            {
                PhysicsRaycastResult2D result2d;
                world2d->RaycastSingle(result2d, Vector2(start.x_, start.y_), Vector2(end.x_, end.y_), mask);
                hit = result2d.body_ != nullptr;
                context.SetOutput("position", Vector3(result2d.position_.x_, result2d.position_.y_, 0.0f));
                context.SetOutput("normal", Vector3(result2d.normal_.x_, result2d.normal_.y_, 0.0f));
                context.SetOutput("distance", result2d.distance_);
            }
            else
            {
                context.ReportError("BPPHY005", "Physics.RayCast requires a PhysicsWorld or PhysicsWorld2D in the scene.");
                return;
            }
            context.SetOutput("hit", hit);
        }, "Raycast between two points in a 2D or 3D physics world.",
        {RuntimePin("start", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)),
         RuntimePin("end", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::FORWARD)),
         RuntimePin("collisionMask", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(M_MAX_UNSIGNED)),
         RuntimePin("hit", BlueprintPinKind::Output, BlueprintDataType::Bool),
         RuntimePin("position", BlueprintPinKind::Output, BlueprintDataType::Vector3),
         RuntimePin("normal", BlueprintPinKind::Output, BlueprintDataType::Vector3),
         RuntimePin("distance", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "Physics.SetGravity", "Gameplay/Physics", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            if (!node || !node->GetScene())
                return;
            const Vector3 gravity = context.GetInput("gravity").GetVector3();
            bool changed = false;
            if (PhysicsWorld* world = node->GetScene()->GetComponent<PhysicsWorld>())
            {
                world->SetGravity(gravity);
                changed = true;
            }
            if (PhysicsWorld2D* world2d = node->GetScene()->GetComponent<PhysicsWorld2D>())
            {
                world2d->SetGravity(Vector2(gravity.x_, gravity.y_));
                changed = true;
            }
            if (!changed)
            {
                context.ReportError("BPPHY006", "Physics.SetGravity requires a physics world in the scene.");
                return;
            }
            context.ContinueWith("then");
        }, "Set gravity for the active 2D and/or 3D physics world.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("gravity", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3(0.0f, -9.81f, 0.0f)))});

    RegisterDefinition(registry_, "Animation.Play", "Gameplay/Animation", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            AnimationController* controller = node ? node->GetComponent<AnimationController>() : nullptr;
            if (!controller)
            {
                context.ReportError("BPANI001", "Animation.Play requires an AnimationController target.");
                return;
            }
            const bool played = controller->Play(context.GetInput("animation").GetString(),
                static_cast<unsigned char>(context.GetInput("layer").GetUInt()), context.GetInput("looped").GetBool());
            context.SetOutput("played", played);
            if (played)
                context.ContinueWith("then");
        }, "Play a named animation on an AnimationController.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("animation", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("layer", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("looped", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(true)),
         RuntimePin("played", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Animation.Stop", "Gameplay/Animation", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            AnimationController* controller = node ? node->GetComponent<AnimationController>() : nullptr;
            if (!controller)
            {
                context.ReportError("BPANI002", "Animation.Stop requires an AnimationController target.");
                return;
            }
            const bool stopped = controller->Stop(context.GetInput("animation").GetString());
            context.SetOutput("stopped", stopped);
            if (stopped)
                context.ContinueWith("then");
        }, "Stop a named animation.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("animation", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("stopped", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Animation.SetSpeed", "Gameplay/Animation", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            AnimationController* controller = node ? node->GetComponent<AnimationController>() : nullptr;
            if (!controller)
            {
                context.ReportError("BPANI003", "Animation.SetSpeed requires an AnimationController target.");
                return;
            }
            const bool changed = controller->SetSpeed(context.GetInput("animation").GetString(), context.GetInput("speed").GetFloat());
            context.SetOutput("changed", changed);
            if (changed)
                context.ContinueWith("then");
        }, "Set playback speed for a named animation.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("animation", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("speed", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(1.0f)),
         RuntimePin("changed", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Animation.IsPlaying", "Gameplay/Animation", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            AnimationController* controller = node ? node->GetComponent<AnimationController>() : nullptr;
            if (!controller)
            {
                context.ReportError("BPANI004", "Animation.IsPlaying requires an AnimationController target.");
                return;
            }
            context.SetOutput("playing", controller->IsPlaying(context.GetInput("animation").GetString()));
        }, "Check whether a named animation is playing.",
        {RuntimePin("animation", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("playing", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Animation.GetTime", "Gameplay/Animation", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            AnimationController* controller = node ? node->GetComponent<AnimationController>() : nullptr;
            if (!controller)
            {
                context.ReportError("BPANI005", "Animation.GetTime requires an AnimationController target.");
                return;
            }
            context.SetOutput("time", controller->GetTime(context.GetInput("animation").GetString()));
        }, "Read the current time of a named animation.",
        {RuntimePin("animation", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("time", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "Audio.Play", "Gameplay/Audio", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            SoundSource* source = node ? node->GetComponent<SoundSource>() : nullptr;
            if (!source)
            {
                context.ReportError("BPAUD001", "Audio.Play requires a SoundSource target.");
                return;
            }
            Sound* sound = GetTargetResource<Sound>(context, context.GetInput("sound").GetString(), "BPAUD002", "Audio.Play could not load the requested Sound resource.");
            if (!sound)
                return;
            const float frequency = context.GetInput("frequency").GetFloat();
            const float volume = context.GetInput("volume").GetFloat();
            if (frequency > 0.0f)
                source->Play(sound, frequency, volume);
            else
                source->Play(sound);
            context.ContinueWith("then");
        }, "Play a Sound resource through a SoundSource.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("sound", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("frequency", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f)),
         RuntimePin("volume", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(1.0f))});

    RegisterDefinition(registry_, "Audio.Stop", "Gameplay/Audio", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            SoundSource* source = node ? node->GetComponent<SoundSource>() : nullptr;
            if (!source)
            {
                context.ReportError("BPAUD003", "Audio.Stop requires a SoundSource target.");
                return;
            }
            source->Stop();
            context.ContinueWith("then");
        }, "Stop SoundSource playback.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard)});

    RegisterDefinition(registry_, "Audio.SetVolume", "Gameplay/Audio", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            SoundSource* source = node ? node->GetComponent<SoundSource>() : nullptr;
            if (!source)
            {
                context.ReportError("BPAUD004", "Audio.SetVolume requires a SoundSource target.");
                return;
            }
            source->SetGain(context.GetInput("volume").GetFloat());
            context.ContinueWith("then");
        }, "Set SoundSource gain.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("volume", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(1.0f))});

    RegisterDefinition(registry_, "Audio.SetPitch", "Gameplay/Audio", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            SoundSource* source = node ? node->GetComponent<SoundSource>() : nullptr;
            if (!source)
            {
                context.ReportError("BPAUD005", "Audio.SetPitch requires a SoundSource target.");
                return;
            }
            source->SetFrequency(context.GetInput("frequency").GetFloat());
            context.ContinueWith("then");
        }, "Set SoundSource playback frequency.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("frequency", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(44100.0f))});

    RegisterDefinition(registry_, "Audio.IsPlaying", "Gameplay/Audio", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            SoundSource* source = node ? node->GetComponent<SoundSource>() : nullptr;
            if (!source)
            {
                context.ReportError("BPAUD006", "Audio.IsPlaying requires a SoundSource target.");
                return;
            }
            context.SetOutput("playing", source->IsPlaying());
        }, "Check whether a SoundSource is currently playing.",
        {RuntimePin("playing", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Camera.SetFOV", "Gameplay/Camera", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            Camera* camera = node ? node->GetComponent<Camera>() : nullptr;
            if (!camera)
            {
                context.ReportError("BPCAM001", "Camera.SetFOV requires a Camera target.");
                return;
            }
            camera->SetFov(context.GetInput("fov").GetFloat());
            context.ContinueWith("then");
        }, "Set camera vertical field of view in degrees.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("fov", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(45.0f))});

    RegisterDefinition(registry_, "Camera.GetFOV", "Gameplay/Camera", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            Camera* camera = node ? node->GetComponent<Camera>() : nullptr;
            if (!camera)
            {
                context.ReportError("BPCAM002", "Camera.GetFOV requires a Camera target.");
                return;
            }
            context.SetOutput("fov", camera->GetFov());
        }, "Read camera vertical field of view in degrees.",
        {RuntimePin("fov", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "Camera.SetOrtho", "Gameplay/Camera", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            Camera* camera = node ? node->GetComponent<Camera>() : nullptr;
            if (!camera)
            {
                context.ReportError("BPCAM003", "Camera.SetOrtho requires a Camera target.");
                return;
            }
            camera->SetOrthographic(context.GetInput("orthographic").GetBool());
            context.ContinueWith("then");
        }, "Enable or disable orthographic camera projection.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("orthographic", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(false))});

    RegisterDefinition(registry_, "Camera.ScreenToWorld", "Gameplay/Camera", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            Camera* camera = node ? node->GetComponent<Camera>() : nullptr;
            if (!camera)
            {
                context.ReportError("BPCAM004", "Camera.ScreenToWorld requires a Camera target.");
                return;
            }
            context.SetOutput("world", camera->ScreenToWorldPoint(context.GetInput("screen").GetVector3()));
        }, "Convert a screen-space point to world space.",
        {RuntimePin("screen", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)),
         RuntimePin("world", BlueprintPinKind::Output, BlueprintDataType::Vector3)});

    RegisterDefinition(registry_, "Camera.WorldToScreen", "Gameplay/Camera", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Node* node = GetTargetNode(context);
            Camera* camera = node ? node->GetComponent<Camera>() : nullptr;
            if (!camera)
            {
                context.ReportError("BPCAM005", "Camera.WorldToScreen requires a Camera target.");
                return;
            }
            context.SetOutput("screen", camera->WorldToScreenPoint(context.GetInput("world").GetVector3()));
        }, "Convert a world-space point to screen space.",
        {RuntimePin("world", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)),
         RuntimePin("screen", BlueprintPinKind::Output, BlueprintDataType::Vector3)});

    RegisterDefinition(registry_, "Material.SetParameter", "Gameplay/Materials", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Material* material = GetTargetMaterial(context, context.GetInput("materialIndex").GetUInt());
            if (!material)
                return;
            material->SetShaderParameter(context.GetInput("parameter").GetString(), context.GetInput("value"));
            context.ContinueWith("then");
        }, "Set a shader parameter on a model material.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("materialIndex", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("parameter", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Input, BlueprintDataType::Variant)});

    RegisterDefinition(registry_, "Material.GetParameter", "Gameplay/Materials", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            Material* material = GetTargetMaterial(context, context.GetInput("materialIndex").GetUInt());
            if (!material)
                return;
            context.SetOutput("value", material->GetShaderParameter(context.GetInput("parameter").GetString()));
        }, "Read a shader parameter from a model material.",
        {RuntimePin("materialIndex", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("parameter", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Variant)});

    RegisterDefinition(registry_, "Material.SetTexture", "Gameplay/Materials", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            Material* material = GetTargetMaterial(context, context.GetInput("materialIndex").GetUInt());
            if (!material)
                return;
            Texture* texture = GetTargetResource<Texture>(context, context.GetInput("texture").GetString(), "BPMAT003", "Material.SetTexture could not load the requested Texture resource.");
            if (!texture)
                return;
            material->SetTexture(context.GetInput("slot").GetString(), texture);
            context.ContinueWith("then");
        }, "Assign a texture resource to a material texture slot.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("materialIndex", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("slot", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("texture", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string()))});

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

    RegisterDefinition(registry_, "World.LoadCell", "World/Streaming", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            WorldPartition* partition = context.GetRuntime().GetWorldPartition();
            if (!partition)
            {
                context.ReportError("BPWORLD001", "World.LoadCell requires a WorldPartition bound to the BlueprintRuntime.");
                return;
            }
            const bool accepted = partition->RequestLoad(context.GetInput("cell").GetString());
            context.SetOutput("queued", accepted);
            if (accepted)
                context.ContinueWith("then");
            else
                context.ReportError("BPWORLD002", partition->GetLastError());
        }, "Request a named world-partition cell to load.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("cell", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("queued", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "World.UnloadCell", "World/Streaming", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            WorldPartition* partition = context.GetRuntime().GetWorldPartition();
            if (!partition)
            {
                context.ReportError("BPWORLD003", "World.UnloadCell requires a WorldPartition bound to the BlueprintRuntime.");
                return;
            }
            const bool accepted = partition->RequestUnload(context.GetInput("cell").GetString());
            context.SetOutput("queued", accepted);
            if (accepted)
                context.ContinueWith("then");
            else
                context.ReportError("BPWORLD004", partition->GetLastError());
        }, "Request a named world-partition cell to unload.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("cell", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("queued", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "World.SetStreamingRadius", "World/Streaming", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            WorldPartition* partition = context.GetRuntime().GetWorldPartition();
            if (!partition)
            {
                context.ReportError("BPWORLD005", "World.SetStreamingRadius requires a WorldPartition bound to the BlueprintRuntime.");
                return;
            }
            partition->SetStreamingRadius(context.GetInput("radius").GetFloat());
            context.SetOutput("value", partition->GetStreamingRadius());
            context.ContinueWith("then");
        }, "Set the active world-partition streaming radius.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("radius", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(100.0f)),
         RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Float)});

    RegisterDefinition(registry_, "Net.SendRPC", "Network/RPC", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            RpcDispatcher* dispatcher = context.GetRuntime().GetRpcDispatcher();
            if (!dispatcher)
            {
                context.ReportError("BPNET001", "Net.SendRPC requires an RpcDispatcher bound to the BlueprintRuntime.");
                return;
            }
            const ea::string name = context.GetInput("name").GetString();
            AbstractConnection* connection = context.GetRuntime().GetRpcConnection();
            if (!connection && context.GetRuntime().GetNetwork())
                connection = context.GetRuntime().GetNetwork()->GetServerConnection();
            const bool sent = dispatcher->Send(connection, name, context.GetInput("arguments").GetStringVariantMap(),
                context.GetInput("reliable").GetBool());
            context.SetOutput("sent", sent);
            if (sent)
                context.ContinueWith("then");
            else
                context.ReportError("BPNET002", "Net.SendRPC could not send the RPC on the bound connection.");
        }, "Send a typed RPC through the bound rbfx connection.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("name", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string())),
         RuntimePin("arguments", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(StringVariantMap{})),
         RuntimePin("reliable", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(true)),
         RuntimePin("sent", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Net.SetRelevancy", "Network/Interest", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            RelevancyManager* manager = context.GetRuntime().GetRelevancyManager();
            if (!manager)
            {
                context.ReportError("BPNET003", "Net.SetRelevancy requires a RelevancyManager bound to the BlueprintRuntime.");
                return;
            }
            const NetworkId objectId = ConstructComponentReference(context.GetInput("objectIndex").GetUInt(),
                context.GetInput("objectVersion").GetUInt());
            manager->SetObjectRule(objectId, context.GetInput("position").GetVector3(),
                context.GetInput("radius").GetFloat(), context.GetInput("alwaysRelevant").GetBool());
            context.SetOutput("applied", true);
            context.ContinueWith("then");
        }, "Set the spatial interest rule for a replicated object.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("objectIndex", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("objectVersion", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(0u)),
         RuntimePin("position", BlueprintPinKind::Input, BlueprintDataType::Vector3, Variant(Vector3::ZERO)),
         RuntimePin("radius", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(100.0f)),
         RuntimePin("alwaysRelevant", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(false)),
         RuntimePin("applied", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Net.GetPing", "Network/Diagnostics", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            unsigned ping = context.GetVariable("__net.ping").GetUInt();
            if (AbstractConnection* connection = context.GetRuntime().GetRpcConnection())
                ping = connection->GetPing();
            else if (context.GetRuntime().GetNetwork() && context.GetRuntime().GetNetwork()->GetServerConnection())
                ping = context.GetRuntime().GetNetwork()->GetServerConnection()->GetPing();
            context.SetOutput("ping", ping);
        }, "Return the latest measured round-trip latency in milliseconds.",
        {RuntimePin("ping", BlueprintPinKind::Output, BlueprintDataType::Int)});

    RegisterDefinition(registry_, "Net.IsServer", "Network/Role", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            const DedicatedServer* server = context.GetRuntime().GetDedicatedServer();
            context.SetOutput("value", server && server->IsRunning());
        }, "Return whether the bound dedicated server is running.",
        {RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Net.IsClient", "Network/Role", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            context.SetOutput("value", context.GetRuntime().GetNetwork()
                && context.GetRuntime().GetNetwork()->GetServerConnection());
        }, "Return whether the bound Network subsystem has a server connection.",
        {RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Bool)});

    RegisterDefinition(registry_, "Profiler.BeginScope", "Profiler/CPU", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            ProductionProfiler* profiler = context.GetRuntime().GetProductionProfiler();
            if (!profiler)
            {
                context.ReportError("BPPROF001", "Profiler.BeginScope requires a ProductionProfiler bound to the BlueprintRuntime.");
                return;
            }
            profiler->BeginScope(context.GetInput("name").GetString());
            context.ContinueWith("then");
        }, "Begin a named hierarchical CPU profiler scope.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("name", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string()))});

    RegisterDefinition(registry_, "Profiler.EndScope", "Profiler/CPU", BlueprintExecutionMode::Immediate,
        [](BlueprintExecutionContext& context)
        {
            ProductionProfiler* profiler = context.GetRuntime().GetProductionProfiler();
            if (!profiler)
            {
                context.ReportError("BPPROF002", "Profiler.EndScope requires a ProductionProfiler bound to the BlueprintRuntime.");
                return;
            }
            profiler->EndScope(context.GetInput("name").GetString());
            context.ContinueWith("then");
        }, "End the active CPU profiler scope.",
        {RuntimePin("execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard),
         RuntimePin("then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard),
         RuntimePin("name", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string()))});

    RegisterDefinition(registry_, "Profiler.GetFrameTime", "Profiler/CPU", BlueprintExecutionMode::Pure,
        [](BlueprintExecutionContext& context)
        {
            ProductionProfiler* profiler = context.GetRuntime().GetProductionProfiler();
            context.SetOutput("value", profiler ? static_cast<float>(profiler->GetLastFrameTimeMilliseconds()) : 0.0f);
        }, "Read the latest captured frame time in milliseconds.",
        {RuntimePin("value", BlueprintPinKind::Output, BlueprintDataType::Float)});
}

bool BlueprintRuntime::Execute(const BlueprintGraph& graph, BlueprintId entryNode,
    StringVariantMap variables, unsigned maxSteps)
{
    values_.clear();
    variables_ = ea::move(variables);
    diagnostics_.clear();
    functionCallStack_.clear();
    latentGraph_ = nullptr;
    latentNextNode_ = BLUEPRINT_INVALID_ID;
    latentRemaining_ = 0.0f;
    latentPending_ = false;
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

bool BlueprintRuntime::ExecuteFunction(const BlueprintGraph& ownerGraph, const ea::string& functionName,
    const StringVariantMap& inputs, StringVariantMap* outputs, unsigned maxSteps)
{
    BlueprintFunction macroAsFunction;
    const BlueprintFunction* function = ownerGraph.GetFunction(functionName);
    if (!function)
    {
        const BlueprintMacro* macro = ownerGraph.GetMacro(functionName);
        if (!macro)
        {
            AddError(BLUEPRINT_INVALID_ID, "BP201", Format("Blueprint function or macro '{}' was not found.", functionName));
            return false;
        }
        macroAsFunction.name = macro->name;
        macroAsFunction.description = macro->description;
        macroAsFunction.inputs = macro->inputs;
        macroAsFunction.outputs = macro->outputs;
        macroAsFunction.body = macro->body;
        function = &macroAsFunction;
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

    for (const auto& input : inputs)
        variables_[input.first] = input.second;

    functionCallStack_.push_back(functionName);
    unsigned steps = 0;
    const bool result = ExecuteNode(body, entryNode, steps) && steps <= maxSteps;
    functionCallStack_.pop_back();

    if (outputs)
    {
        outputs->clear();
        for (const BlueprintPin& pin : function->outputs)
        {
            const auto value = variables_.find(pin.name);
            if (value != variables_.end())
                (*outputs)[pin.name] = value->second;
        }
    }
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

bool BlueprintRuntime::ContinueDebug(unsigned maxSteps)
{
    if (!IsDebugActive())
        return false;

    unsigned steps = 0;
    while (IsDebugActive() && steps++ < maxSteps)
    {
        if (!StepDebug())
            return false;
        if (IsDebugActive() && HasBreakpoint(debugCurrentNode_))
            return true;
    }

    if (IsDebugActive())
    {
        AddError(debugCurrentNode_, "BP105", "Blueprint debug continue exceeded the safety step limit.");
        StopDebug();
        return false;
    }
    return true;
}

void BlueprintRuntime::SetBreakpoint(BlueprintId nodeId, bool enabled)
{
    if (nodeId == BLUEPRINT_INVALID_ID)
        return;
    if (enabled)
        breakpoints_.insert(nodeId);
    else
        breakpoints_.erase(nodeId);
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
    if (definition->executionMode == BlueprintExecutionMode::Latent && latentPending_)
    {
        latentGraph_ = &graph;
        latentNextNode_ = next ? next->toNode : BLUEPRINT_INVALID_ID;
        return true;
    }
    return next ? ExecuteNode(graph, next->toNode, steps) : true;
}

bool BlueprintRuntime::Tick(float deltaSeconds)
{
    const float delta = Max(0.0f, deltaSeconds);
    bool progressed = false;
    if (timelineGraph_)
    {
        for (auto iter = activeTimelines_.begin(); iter != activeTimelines_.end();)
        {
            const ea::string timelineName = *iter;
            const BlueprintTimeline* timeline = timelineGraph_->GetTimeline(timelineName);
            if (!timeline)
            {
                timelinePositions_.erase(timelineName);
                iter = activeTimelines_.erase(iter);
                continue;
            }
            float& position = timelinePositions_[timelineName];
            position += delta;
            progressed = true;
            if (position >= timeline->length)
            {
                if (timeline->looping)
                {
                    while (position >= timeline->length)
                        position -= timeline->length;
                    ++iter;
                }
                else
                {
                    position = timeline->length;
                    iter = activeTimelines_.erase(iter);
                }
            }
            else
                ++iter;
        }
        if (activeTimelines_.empty())
            timelineGraph_ = nullptr;
    }

    if (!latentPending_ || !latentGraph_)
        return progressed;

    latentRemaining_ -= delta;
    if (latentRemaining_ > 0.0f)
        return true;

    const BlueprintGraph* graph = latentGraph_;
    const BlueprintId nextNode = latentNextNode_;
    latentGraph_ = nullptr;
    latentNextNode_ = BLUEPRINT_INVALID_ID;
    latentRemaining_ = 0.0f;
    latentPending_ = false;

    if (nextNode == BLUEPRINT_INVALID_ID)
        return true;
    unsigned steps = 0;
    return ExecuteNode(*graph, nextNode, steps);
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
