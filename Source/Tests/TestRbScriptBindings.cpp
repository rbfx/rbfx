// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RbScript/RbScriptBindings.h>

using namespace Urho3D;

TEST_CASE("rbscript bindings register gameplay functions", "[rbscript][bindings]")
{
    RbScriptTypeRegistry registry;
    RbScriptBindings bindings;
    REQUIRE(bindings.Register(registry) == 8);

    const RbScriptFunctionSignature* ownerSignature = registry.FindFunction("owner");
    REQUIRE(ownerSignature != nullptr);
    REQUIRE(ownerSignature->returnType.name == "Node");
    REQUIRE(ownerSignature->parameterTypes.empty());

    const RbScriptFunctionSignature* childSignature = registry.FindFunction("child");
    REQUIRE(childSignature != nullptr);
    REQUIRE(childSignature->returnType.name == "Node");
    REQUIRE(childSignature->parameterTypes.size() == 1);
    REQUIRE(childSignature->parameterTypes[0].name == "String");

    RbScriptValue result;
    REQUIRE(bindings.Invoke("owner", {}, result));
    REQUIRE(result.kind == RbScriptValueKind::Pointer);
    REQUIRE(result.pointerValue == nullptr);

    REQUIRE(bindings.Invoke("input::is_pressed", {RbScriptValue::FromString("Jump")}, result));
    REQUIRE(result.kind == RbScriptValueKind::Boolean);
    REQUIRE(!result.booleanValue);
    REQUIRE(!bindings.Invoke("missing", {}, result));
}

TEST_CASE("rbscript bindings expose scene access signatures", "[rbscript][bindings]")
{
    RbScriptTypeRegistry registry;
    RbScriptBindings bindings;
    bindings.Register(registry);

    for (const ea::string& name : {ea::string("world"), ea::string("scene::current")})
    {
        const RbScriptFunctionSignature* signature = registry.FindFunction(name);
        REQUIRE(signature != nullptr);
        REQUIRE(signature->returnType.name == "Node");
        REQUIRE(signature->parameterTypes.empty());
    }

    RbScriptValue result;
    REQUIRE(bindings.Invoke("timer::after", {RbScriptValue::FromFloat(0.5)}, result));
    REQUIRE(result.kind == RbScriptValueKind::Null);
}
