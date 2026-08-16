// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include "CommonUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/RbScript/RbScriptResource.h>

using namespace Urho3D;

namespace
{

const ea::string ValidSource =
    "module Game.Resource;\n"
    "script Actor : Node {\n"
    "  fn health() -> i32 { return 100; }\n"
    "}\n";

}

TEST_CASE("rbscript resource compiles and caches bytecode", "[rbscript][resource]")
{
    const auto context = Tests::GetOrCreateContext(&Tests::CreateCompleteContext);
    RbScriptResource resource(context.Get());

    REQUIRE(resource.CompileSource(ValidSource, "Memory.rbscript"));
    REQUIRE(resource.IsCompiled());
    REQUIRE(resource.GetSource() == ValidSource);
    REQUIRE(resource.GetChunk().functions.size() == 1);
    REQUIRE(resource.GetChunk().functions[0].name == "health");
    REQUIRE(resource.GetDiagnostics().empty());
}

TEST_CASE("rbscript resource saves and loads source text", "[rbscript][resource][serialization]")
{
    const auto context = Tests::GetOrCreateContext(&Tests::CreateCompleteContext);
    RbScriptResource resource(context.Get());
    REQUIRE(resource.CompileSource(ValidSource, "Memory.rbscript"));

    VectorBuffer buffer;
    buffer.SetName("Memory.rbscript");
    REQUIRE(resource.Save(buffer));
    REQUIRE(buffer.GetSize() == ValidSource.size());

    buffer.Seek(0);
    RbScriptResource restored(context.Get());
    REQUIRE(restored.Load(buffer));
    REQUIRE(restored.IsCompiled());
    REQUIRE(restored.GetSource() == ValidSource);
    REQUIRE(restored.GetChunk().functions.size() == 1);
}

TEST_CASE("rbscript resource keeps the last valid chunk after a failed compile", "[rbscript][resource][diagnostics]")
{
    const auto context = Tests::GetOrCreateContext(&Tests::CreateCompleteContext);
    RbScriptResource resource(context.Get());
    REQUIRE(resource.CompileSource(ValidSource, "Memory.rbscript"));
    const unsigned functionCount = resource.GetChunk().functions.size();

    REQUIRE(!resource.CompileSource(
        "module Broken; script Actor : Node { fn health() -> Missing { return 1; } }\n",
        "Broken.rbscript"));
    REQUIRE(resource.IsCompiled());
    REQUIRE(resource.GetChunk().functions.size() == functionCount);
    REQUIRE(!resource.GetDiagnostics().empty());
}
