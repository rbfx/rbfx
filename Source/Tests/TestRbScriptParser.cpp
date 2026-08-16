// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/RbScript/RbScriptParser.h>

using namespace Urho3D;

namespace
{

RbScriptModule Parse(const ea::string& source)
{
    RbScriptLexer lexer(source, "ParserTest.rbscript");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());
    RbScriptParser parser(tokens, "ParserTest.rbscript");
    RbScriptModule module = parser.ParseModule();
    REQUIRE(parser.GetDiagnostics().empty());
    return module;
}

}

TEST_CASE("rbscript parser builds typed module AST", "[rbscript][parser]")
{
    const RbScriptModule module = Parse(
        "module Game.Player;\n"
        "use rbfx::input;\n"
        "script PlayerController : NodeBehavior {\n"
        "  [[export]] f32 speed = 6.0f;\n"
        "  fn move(Vector3 direction) -> void {\n"
        "    owner().translate(direction * speed);\n"
        "  }\n"
        "}\n");

    REQUIRE(module.IsValid());
    REQUIRE(module.name == "Game::Player");
    REQUIRE(module.imports.size() == 1);
    REQUIRE(module.imports[0] == "rbfx::input");
    REQUIRE(module.scripts.size() == 1);
    REQUIRE(module.scripts[0].name == "PlayerController");
    REQUIRE(module.scripts[0].baseType == "NodeBehavior");
    REQUIRE(module.scripts[0].fields.size() == 1);
    REQUIRE(module.scripts[0].fields[0].name == "speed");
    REQUIRE(module.scripts[0].fields[0].typeName == "f32");
    REQUIRE(module.scripts[0].fields[0].attributes[0] == "export");
    REQUIRE(module.scripts[0].functions.size() == 1);
    REQUIRE(module.scripts[0].functions[0].name == "move");
    REQUIRE(module.scripts[0].functions[0].parameters[0].typeName == "Vector3");
    REQUIRE(module.scripts[0].functions[0].parameters[0].name == "direction");
    REQUIRE(module.scripts[0].functions[0].body.size() == 1);
    REQUIRE(module.scripts[0].functions[0].body[0]->kind == RbScriptStatementKind::Expression);
    REQUIRE(module.scripts[0].functions[0].body[0]->expression->kind == RbScriptExpressionKind::Call);
}

TEST_CASE("rbscript parser handles events, conditionals and returns", "[rbscript][parser]")
{
    const RbScriptModule module = Parse(
        "module Game.Enemy;\n"
        "script Enemy : NodeBehavior {\n"
        "  fn damage(i32 amount) -> bool {\n"
        "    if (amount > 0) { return true; } else { return false; }\n"
        "  }\n"
        "  on Input.KeyPressed(Key key) { emit Jumped; }\n"
        "}\n");

    REQUIRE(module.scripts[0].functions.size() == 2);
    REQUIRE(module.scripts[0].functions[0].name == "damage");
    REQUIRE(module.scripts[0].functions[0].returnType == "bool");
    REQUIRE(module.scripts[0].functions[0].body[0]->kind == RbScriptStatementKind::If);
    REQUIRE(module.scripts[0].functions[1].eventHandler);
    REQUIRE(module.scripts[0].functions[1].name == "Input::KeyPressed");
    REQUIRE(module.scripts[0].functions[1].parameters[0].name == "key");
    REQUIRE(module.scripts[0].functions[1].body[0]->kind == RbScriptStatementKind::Emit);
}

TEST_CASE("rbscript parser reports malformed declarations", "[rbscript][parser][diagnostics]")
{
    RbScriptLexer lexer("module Broken; script MissingBase { fn broken( -> void { } }");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());

    RbScriptParser parser(tokens, "Broken.rbscript");
    const RbScriptModule module = parser.ParseModule();
    REQUIRE(!parser.GetDiagnostics().empty());
    REQUIRE(!module.IsValid());
    REQUIRE(parser.GetDiagnostics()[0].code.size() > 0);
}
