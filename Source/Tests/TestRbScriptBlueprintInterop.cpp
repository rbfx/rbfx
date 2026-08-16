// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/RbScript/RbScriptBindings.h>
#include <Urho3D/RbScript/RbScriptBlueprintInterop.h>
#include <Urho3D/RbScript/RbScriptCompiler.h>
#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/RbScript/RbScriptParser.h>

using namespace Urho3D;

namespace
{

RbScriptChunk Compile(const ea::string& source, RbScriptTypeRegistry& registry)
{
    RbScriptLexer lexer(source, "InteropTest.rbscript");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());
    RbScriptParser parser(tokens, "InteropTest.rbscript");
    const RbScriptModule module = parser.ParseModule();
    REQUIRE(parser.GetDiagnostics().empty());
    REQUIRE(module.IsValid());
    RbScriptCompiler compiler(&registry);
    RbScriptChunk chunk = compiler.Compile(module);
    REQUIRE(!compiler.HadError());
    return chunk;
}

void AddPin(BlueprintGraph& graph, BlueprintId nodeId, const char* name, BlueprintPinKind kind,
    BlueprintDataType type, const Variant& defaultValue = Variant())
{
    BlueprintPin pin;
    pin.name = name;
    pin.displayName = name;
    pin.kind = kind;
    pin.dataType = type;
    pin.defaultValue = defaultValue;
    REQUIRE(graph.AddPin(nodeId, pin));
}

BlueprintGraph MakeEchoGraph()
{
    BlueprintGraph body("EchoBody");
    const BlueprintId entry = body.AddNode("Function.Entry", "Entry");
    const BlueprintId get = body.AddNode("Variable.Get", "Get value");
    const BlueprintId set = body.AddNode("Variable.Set", "Set result");
    const BlueprintId result = body.AddNode("Function.Return", "Return");
    AddPin(body, entry, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, get, "value", BlueprintPinKind::Output, BlueprintDataType::Int);
    AddPin(body, set, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(body, set, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, set, "value", BlueprintPinKind::Input, BlueprintDataType::Int);
    AddPin(body, result, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    body.GetNode(get)->properties["variableName"] = Variant(ea::string("value"));
    body.GetNode(set)->properties["variableName"] = Variant(ea::string("result"));
    REQUIRE(body.AddLink(entry, "then", set, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(get, "value", set, "value") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(set, "then", result, "execute") != BLUEPRINT_INVALID_ID);
    return body;
}

}

TEST_CASE("rbscript VM invokes registered native bindings", "[rbscript][interop]")
{
    RbScriptTypeRegistry registry;
    RbScriptBindings bindings;
    REQUIRE(bindings.Register(registry) == 8);
    const RbScriptChunk chunk = Compile(
        "module Game.Interop; script Caller : Node {"
        " fn main() -> bool { return input::is_pressed(\"Jump\"); } }", registry);

    RbScriptVM vm;
    vm.SetNativeCallHandler([&bindings](const ea::string& name, const ea::vector<RbScriptValue>& arguments,
        RbScriptValue& result) { return bindings.Invoke(name, arguments, result); });
    REQUIRE(vm.Execute(chunk));
    REQUIRE(vm.GetResult().kind == RbScriptValueKind::Boolean);
    REQUIRE_FALSE(vm.GetResult().booleanValue);
}

TEST_CASE("Blueprint invokes exported rbscript functions", "[blueprint][rbscript][interop]")
{
    RbScriptTypeRegistry registry;
    const RbScriptChunk chunk = Compile(
        "module Game.Interop; script Math : Node {"
        " [[blueprint_callable]] fn multiply(i32 left, i32 right) -> i32 { return left * right; } }", registry);
    REQUIRE(chunk.functions.size() == 1);
    REQUIRE(chunk.functions.front().blueprintCallable);
    REQUIRE(chunk.functions.front().parameterNames[0] == "left");

    BlueprintRuntime runtime;
    RbScriptVM vm;
    REQUIRE(RbScriptBlueprintInterop::RegisterFunctionNodes(runtime, chunk, vm) == 1);
    const ea::string typeName = "Function.RbScript.Math::multiply";
    const BlueprintNodeDefinition* definition = runtime.GetRegistry().Find(typeName);
    REQUIRE(definition != nullptr);
    REQUIRE(definition->pins.size() == 5);

    BlueprintGraph graph("CallRbScript");
    const BlueprintId event = graph.AddNode("Event.OnStart", "Start");
    const BlueprintId call = graph.AddNode(typeName, "Multiply");
    AddPin(graph, event, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    graph.GetNode(call)->pins = definition->pins;
    for (BlueprintPin& pin : graph.GetNode(call)->pins)
    {
        if (pin.name == "left" || pin.name == "right")
            pin.defaultValue = Variant(3);
    }
    REQUIRE(graph.AddLink(event, "then", call, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(runtime.ExecuteEvent(graph, "Event.OnStart"));
    if (!runtime.GetDiagnostics().empty())
    {
        WARN(runtime.GetDiagnostics().front().code.c_str());
        WARN(runtime.GetDiagnostics().front().message.c_str());
    }
    REQUIRE_FALSE(runtime.HadRuntimeError());
}

TEST_CASE("rbscript blueprint::call invokes a Blueprint function", "[blueprint][rbscript][interop]")
{
    BlueprintGraph graph("InteropGraph");
    BlueprintFunction function;
    function.name = "Echo";
    function.inputs.push_back({"value", "value", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(), true, false});
    function.outputs.push_back({"result", "result", BlueprintPinKind::Output, BlueprintDataType::Int, Variant(), false, false});
    function.body = MakeEchoGraph().ToString();
    REQUIRE(graph.AddFunction(function));

    BlueprintRuntime runtime;
    RbScriptTypeRegistry registry;
    RbScriptBindings bindings;
    REQUIRE(bindings.Register(registry) == 8);
    RbScriptBlueprintInterop::BindBlueprintCalls(bindings, runtime, graph);
    const RbScriptChunk chunk = Compile(
        "module Game.Interop; script Caller : Node {"
        " fn main() -> i32 { return blueprint::call(\"Echo\", 21); } }", registry);

    RbScriptVM vm;
    vm.SetNativeCallHandler([&bindings](const ea::string& name, const ea::vector<RbScriptValue>& arguments,
        RbScriptValue& result) { return bindings.Invoke(name, arguments, result); });
    REQUIRE(vm.Execute(chunk));
    REQUIRE(vm.GetResult().kind == RbScriptValueKind::Integer);
    REQUIRE(vm.GetResult().integerValue == 21);
}
