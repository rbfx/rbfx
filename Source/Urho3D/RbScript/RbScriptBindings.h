// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptVM.h"

#include <EASTL/functional.h>

namespace Urho3D
{

class Context;
class Node;
class Scene;

using RbScriptNativeCallback = ea::function<RbScriptValue(const ea::vector<RbScriptValue>&)>;

struct URHO3D_API RbScriptNativeFunction
{
    RbScriptFunctionSignature signature;
    RbScriptNativeCallback callback;
};

class URHO3D_API RbScriptBindings
{
public:
    explicit RbScriptBindings(Context* context = nullptr);

    unsigned Register(RbScriptTypeRegistry& registry);
    void SetOwner(Node* owner) { owner_ = owner; }
    void SetWorld(Scene* world) { world_ = world; }
    Node* GetOwner() const { return owner_; }
    Scene* GetWorld() const { return world_; }

    const RbScriptNativeFunction* Find(const ea::string& name) const;
    bool Invoke(const ea::string& name, const ea::vector<RbScriptValue>& arguments, RbScriptValue& result) const;

private:
    void RegisterFunction(RbScriptTypeRegistry& registry, const ea::string& name,
        const ea::string& returnType, const ea::vector<ea::string>& parameters, RbScriptNativeCallback callback);
    void RegisterBuiltins(RbScriptTypeRegistry& registry);

    Context* context_{};
    Node* owner_{};
    Scene* world_{};
    ea::unordered_map<ea::string, RbScriptNativeFunction> functions_;
};

} // namespace Urho3D
