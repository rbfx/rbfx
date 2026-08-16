// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RbScript/RbScriptCompiler.h>
#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/RbScript/RbScriptParser.h>
#include <Urho3D/RbScript/RbScriptVM.h>

using namespace Urho3D;

namespace
{

RbScriptChunk Compile(const ea::string& source, RbScriptCompiler& compiler)
{
    RbScriptLexer lexer(source, "VMTest.rbscript");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());
    RbScriptParser parser(tokens, "VMTest.rbscript");
    const RbScriptModule module = parser.ParseModule();
    REQUIRE(parser.GetDiagnostics().empty());
    REQUIRE(module.IsValid());
    RbScriptChunk chunk = compiler.Compile(module);
    REQUIRE(!compiler.HadError());
    return chunk;
}

}

TEST_CASE("rbscript VM evaluates arithmetic and locals", "[rbscript][vm]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.VM;\n"
        "script Arithmetic : Node {\n"
        "  fn main() -> i32 {\n"
        "    var value: i32 = 2 + 3 * 4;\n"
        "    return value;\n"
        "  }\n"
        "}\n", compiler);

    RbScriptVM vm;
    REQUIRE(vm.Execute(chunk));
    REQUIRE(!vm.HadError());
    REQUIRE(vm.GetResult().kind == RbScriptValueKind::Integer);
    REQUIRE(vm.GetResult().integerValue == 14);
    REQUIRE(vm.GetExecutedSteps() > 0);
}

TEST_CASE("rbscript VM executes function calls and control flow", "[rbscript][vm]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.VM;\n"
        "script Flow : Node {\n"
        "  fn main() -> i32 { return add(2, 3); }\n"
        "  fn add(i32 left, i32 right) -> i32 { return left + right; }\n"
        "}\n", compiler);

    RbScriptVM vm;
    REQUIRE(vm.Execute(chunk));
    REQUIRE(vm.GetResult().integerValue == 5);

    RbScriptCompiler loopCompiler;
    const RbScriptChunk loopChunk = Compile(
        "module Game.Loop;\n"
        "script Flow : Node {\n"
        "  fn main() -> i32 {\n"
        "    var value: i32 = 0;\n"
        "    while (value < 3) { value = value + 1; }\n"
        "    return value;\n"
        "  }\n"
        "}\n", loopCompiler);
    RbScriptVM loopVm;
    REQUIRE(loopVm.Execute(loopChunk));
    REQUIRE(loopVm.GetResult().integerValue == 3);
}

TEST_CASE("rbscript VM emits events and recovers with diagnostics", "[rbscript][vm][diagnostics]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.Events;\n"
        "script Emitter : Node {\n"
        "  fn main() -> i32 { emit Started; return 7; }\n"
        "}\n", compiler);

    RbScriptVM vm;
    REQUIRE(vm.Execute(chunk));
    REQUIRE(vm.GetEmittedEvents().size() == 1);
    REQUIRE(vm.GetEmittedEvents()[0].kind == RbScriptValueKind::String);
    REQUIRE(vm.GetEmittedEvents()[0].stringValue == "Started");
    REQUIRE(vm.GetResult().integerValue == 7);

    RbScriptCompiler errorCompiler;
    const RbScriptChunk errorChunk = Compile(
        "module Game.Errors;\n"
        "script Errors : Node {\n"
        "  fn main() -> i32 { return 4 / 0; }\n"
        "}\n", errorCompiler);
    RbScriptVM errorVm;
    REQUIRE(!errorVm.Execute(errorChunk));
    REQUIRE(errorVm.HadError());
    REQUIRE(errorVm.GetDiagnostics()[0].code == "V3015");
}

TEST_CASE("rbscript VM executes Array and Map collections", "[rbscript][vm][collections]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.Collections;\n"
        "script Collections : Node {\n"
        "  fn main() -> i32 {\n"
        "    var values: Array<i32> = [1, 2, 3];\n"
        "    values[1] = 9;\n"
        "    values.push(4);\n"
        "    var scores: Map<String,i32> = {\"score\": values[1]};\n"
        "    scores[\"bonus\"] = 2;\n"
        "    if (scores.contains(\"bonus\")) { return values.length() + scores[\"score\"] + scores[\"bonus\"]; }\n"
        "    return 0;\n"
        "  }\n"
        "}\n", compiler);

    RbScriptVM vm;
    REQUIRE(vm.Execute(chunk));
    REQUIRE(!vm.HadError());
    REQUIRE(vm.GetResult().kind == RbScriptValueKind::Integer);
    REQUIRE(vm.GetResult().integerValue == 15);
}

TEST_CASE("rbscript VM supports source debugging", "[rbscript][vm][debugger]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.Debug;\n"
        "script Debugger : Node {\n"
        "  fn main() -> i32 {\n"
        "    var value: i32 = 1;\n"
        "    value = value + 2;\n"
        "    return value;\n"
        "  }\n"
        "}\n", compiler);

    RbScriptVM vm;
    vm.SetBreakpoint(5);
    REQUIRE(vm.BeginDebug(chunk));
    REQUIRE(vm.IsDebugging());
    REQUIRE(vm.IsDebugPaused());
    REQUIRE(vm.GetCurrentLine() == 4);
    REQUIRE(vm.GetCallStack().size() == 1);
    REQUIRE(vm.GetCallStack()[0] == "Debugger::main");
    REQUIRE(vm.StepDebug());
    REQUIRE(vm.IsDebugPaused());
    REQUIRE(vm.ContinueDebug());
    REQUIRE(vm.IsDebugPaused());
    REQUIRE(vm.GetCurrentLine() == 5);
    REQUIRE(vm.GetLocals().at("value").integerValue == 1);

    vm.RemoveBreakpoint(5);
    REQUIRE(vm.ContinueDebug());
    REQUIRE_FALSE(vm.IsDebugging());
    REQUIRE(vm.GetResult().integerValue == 3);
    vm.StopDebug();
    REQUIRE_FALSE(vm.IsDebugPaused());
    REQUIRE(vm.GetCallStack().empty());
}

TEST_CASE("rbscript VM enforces execution limits", "[rbscript][vm][diagnostics]")
{
    RbScriptCompiler compiler;
    const RbScriptChunk chunk = Compile(
        "module Game.Limits;\n"
        "script Infinite : Node {\n"
        "  fn main() -> void { while (true) { } }\n"
        "}\n", compiler);

    RbScriptVM vm;
    vm.SetStepLimit(32);
    REQUIRE(!vm.Execute(chunk));
    REQUIRE(vm.HadError());
    REQUIRE(vm.GetDiagnostics()[0].code == "V3003");
    REQUIRE(vm.GetExecutedSteps() == 33);
}
