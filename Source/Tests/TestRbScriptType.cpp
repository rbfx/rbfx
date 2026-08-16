// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/RbScript/RbScriptParser.h>
#include <Urho3D/RbScript/RbScriptType.h>

using namespace Urho3D;

namespace
{

RbScriptModule ParseModule(const ea::string& source)
{
    RbScriptLexer lexer(source, "TypeTest.rbscript");
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    REQUIRE(lexer.GetDiagnostics().empty());
    RbScriptParser parser(tokens, "TypeTest.rbscript");
    RbScriptModule module = parser.ParseModule();
    REQUIRE(parser.GetDiagnostics().empty());
    return module;
}

}

TEST_CASE("rbscript registry resolves builtin aliases and generic types", "[rbscript][types]")
{
    RbScriptTypeRegistry registry;

    REQUIRE(registry.Resolve("Float") == registry.Resolve("f32"));
    REQUIRE(registry.Resolve("NodeBehavior") == registry.Resolve("Node"));
    REQUIRE(registry.Resolve("Array<String>").kind == RbScriptTypeKind::Array);
    REQUIRE(registry.Resolve("Array<String>").elementType == "String");
    REQUIRE(registry.Resolve("Map<String, i32>").kind == RbScriptTypeKind::Map);
    REQUIRE(registry.Resolve("Map<String, i32>").keyType == "String");
    REQUIRE(registry.Resolve("Map<String, i32>").elementType == "i32");
}

TEST_CASE("rbscript type checker accepts valid typed gameplay code", "[rbscript][types]")
{
    RbScriptModule module = ParseModule(
        "module Game.Math;\n"
        "script Calculator : NodeBehavior {\n"
        "  [[export]] f32 scale = 2.0f;\n"
        "  fn add(i32 a, i32 b) -> i32 { return a + b; }\n"
        "  fn is_positive(i32 value) -> bool { return value > 0; }\n"
        "}\n");

    RbScriptTypeRegistry registry;
    RbScriptTypeChecker checker(registry);
    REQUIRE(checker.Check(module));
    REQUIRE(checker.GetDiagnostics().empty());
}

TEST_CASE("rbscript type checker rejects invalid return and initializer types", "[rbscript][types][diagnostics]")
{
    RbScriptModule module = ParseModule(
        "module Game.Invalid;\n"
        "script Broken : NodeBehavior {\n"
        "  f32 scale = true;\n"
        "  fn invalid() -> i32 { return false; }\n"
        "}\n");

    RbScriptTypeRegistry registry;
    RbScriptTypeChecker checker(registry);
    REQUIRE(!checker.Check(module));
    REQUIRE(checker.GetDiagnostics().size() >= 2);
    REQUIRE(checker.GetDiagnostics()[0].severity == RbScriptDiagnosticSeverity::Error);
}
