// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptBindings.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

RbScriptBindings::RbScriptBindings(Context* context)
    : context_(context)
{
}

unsigned RbScriptBindings::Register(RbScriptTypeRegistry& registry)
{
    unsigned count = registry.RegisterFromReflection(context_);
    const unsigned before = functions_.size();
    RegisterBuiltins(registry);
    count += functions_.size() - before;
    return count;
}

const RbScriptNativeFunction* RbScriptBindings::Find(const ea::string& name) const
{
    const auto it = functions_.find(name);
    return it != functions_.end() ? &it->second : nullptr;
}

bool RbScriptBindings::Invoke(const ea::string& name, const ea::vector<RbScriptValue>& arguments,
    RbScriptValue& result) const
{
    const RbScriptNativeFunction* function = Find(name);
    if (!function || !function->callback)
        return false;
    if (arguments.size() != function->signature.parameterTypes.size())
        return false;
    result = function->callback(arguments);
    return true;
}

void RbScriptBindings::RegisterFunction(RbScriptTypeRegistry& registry, const ea::string& name,
    const ea::string& returnType, const ea::vector<ea::string>& parameters, RbScriptNativeCallback callback)
{
    RbScriptNativeFunction native;
    native.signature.name = name;
    native.signature.returnType = registry.Resolve(returnType);
    for (const ea::string& parameter : parameters)
        native.signature.parameterTypes.push_back(registry.Resolve(parameter));
    native.callback = std::move(callback);
    functions_[name] = native;
    registry.RegisterFunction(native.signature);
}

void RbScriptBindings::RegisterBuiltins(RbScriptTypeRegistry& registry)
{
    RegisterFunction(registry, "owner", "Node", {}, [this](const ea::vector<RbScriptValue>&)
    {
        return RbScriptValue::FromPointer(owner_);
    });

    RegisterFunction(registry, "child", "Node", {"String"}, [this](const ea::vector<RbScriptValue>& arguments)
    {
        if (!owner_ || arguments.empty() || arguments[0].kind != RbScriptValueKind::String)
            return RbScriptValue::Null();
        return RbScriptValue::FromPointer(owner_->GetChild(StringHash(arguments[0].stringValue.c_str()), true));
    });

    RegisterFunction(registry, "component", "Component", {"String"}, [this](const ea::vector<RbScriptValue>& arguments)
    {
        if (!owner_ || arguments.empty() || arguments[0].kind != RbScriptValueKind::String)
            return RbScriptValue::Null();
        return RbScriptValue::FromPointer(owner_->GetComponent(StringHash(arguments[0].stringValue.c_str())));
    });

    RegisterFunction(registry, "world", "Node", {}, [this](const ea::vector<RbScriptValue>&)
    {
        Node* scene = world_ ? static_cast<Node*>(world_) : (owner_ ? static_cast<Node*>(owner_->GetScene()) : nullptr);
        return RbScriptValue::FromPointer(scene);
    });

    RegisterFunction(registry, "input::is_pressed", "bool", {"String"},
        [](const ea::vector<RbScriptValue>&)
        {
            // Input is supplied by the host runtime in the next execution layer.
            return RbScriptValue::FromBoolean(false);
        });

    RegisterFunction(registry, "timer::after", "void", {"f32"},
        [](const ea::vector<RbScriptValue>&)
        {
            // Timers are intentionally side-effect free at compile/runtime binding level.
            return RbScriptValue::Null();
        });

    RegisterFunction(registry, "scene::current", "Node", {}, [this](const ea::vector<RbScriptValue>&)
    {
        Node* scene = world_ ? static_cast<Node*>(world_) : (owner_ ? static_cast<Node*>(owner_->GetScene()) : nullptr);
        return RbScriptValue::FromPointer(scene);
    });
}

} // namespace Urho3D
