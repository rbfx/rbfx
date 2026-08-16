// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RbScript/RbScriptCompiler.h>
#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/RbScript/RbScriptParser.h>

using namespace Urho3D;

namespace
{

RbScriptModule Parse(const ea::string& source)
{
    RbScriptLexer lexer(source, "CompilerTest.rbscript");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());
    RbScriptParser parser(tokens, "CompilerTest.rbscript");
    RbScriptModule module = parser.ParseModule();
    REQUIRE(parser.GetDiagnostics().empty());
    REQUIRE(module.IsValid());
    return module;
}

bool ContainsOpcode(const RbScriptChunk& chunk, RbScriptOpcode opcode)
{
    for (const RbScriptInstruction& instruction : chunk.instructions)
    {
        if (instruction.opcode == opcode)
            return true;
    }
    return false;
}

}

TEST_CASE("rbscript compiler emits arithmetic and locals", "[rbscript][compiler]")
{
    const RbScriptModule module = Parse(
        "module Game.Math;\n"
        "script Calculator : Node {\n"
        "  fn sum(i32 value) -> i32 {\n"
        "    var result: i32 = value + 2;\n"
        "    return result;\n"
        "  }\n"
        "}\n");

    RbScriptCompiler compiler;
    const RbScriptChunk chunk = compiler.Compile(module);
    REQUIRE(!compiler.HadError());
    REQUIRE(chunk.functions.size() == 1);
    REQUIRE(chunk.functions[0].name == "sum");
    REQUIRE(chunk.functions[0].scriptName == "Calculator");
    REQUIRE(chunk.functions[0].parameterCount == 1);
    REQUIRE(chunk.functions[0].localCount == 2);
    REQUIRE(chunk.constants.size() == 1);
    REQUIRE(chunk.constants[0].kind == RbScriptConstantKind::Integer);
    REQUIRE(chunk.constants[0].integerValue == 2);
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::LoadLocal));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Add));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::StoreLocal));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Return));
}

TEST_CASE("rbscript compiler emits control flow and calls", "[rbscript][compiler]")
{
    const RbScriptModule module = Parse(
        "module Game.Flow;\n"
        "script Controller : Node {\n"
        "  fn check(i32 value) -> bool {\n"
        "    while (value > 0) {\n"
        "      value = value - 1;\n"
        "      if (value == 2) { break; }\n"
        "    }\n"
        "    return value == 0;\n"
        "  }\n"
        "  fn start() -> void { check(3); emit Started; }\n"
        "}\n");

    RbScriptCompiler compiler;
    const RbScriptChunk chunk = compiler.Compile(module);
    REQUIRE(!compiler.HadError());
    REQUIRE(chunk.functions.size() == 2);
    REQUIRE(chunk.functions[0].name == "check");
    REQUIRE(chunk.functions[1].name == "start");
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::JumpIfFalse));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Jump));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Call));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Emit));
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Equal));
}

TEST_CASE("rbscript compiler adds implicit returns and reports unresolved symbols", "[rbscript][compiler][diagnostics]")
{
    const RbScriptModule module = Parse(
        "module Game.Diagnostics;\n"
        "script Broken : Node {\n"
        "  fn empty() -> void { }\n"
        "  fn invalid() -> void { missing(); }\n"
        "}\n");

    RbScriptCompiler compiler;
    const RbScriptChunk chunk = compiler.Compile(module);
    REQUIRE(compiler.HadError());
    REQUIRE(compiler.GetDiagnostics().size() == 1);
    REQUIRE(compiler.GetDiagnostics()[0].code == "C3008");
    REQUIRE(chunk.functions.size() == 2);
    REQUIRE(chunk.functions[0].entryPoint < chunk.functions[1].entryPoint);
    REQUIRE(chunk.instructions[chunk.functions[0].entryPoint].opcode == RbScriptOpcode::LoadConstant);
    REQUIRE(ContainsOpcode(chunk, RbScriptOpcode::Return));
}
