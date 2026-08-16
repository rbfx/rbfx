// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/functional.h>
#include <EASTL/vector.h>

#include <Urho3D/Network/AbstractConnection.h>
#include <Urho3D/Network/Protocol.h>

namespace Urho3D
{

class MemoryBuffer;

/// Message id reserved by rbfx-blueprint for production RPC payloads.
static constexpr NetworkMessageId MSG_RBFX_RPC = static_cast<NetworkMessageId>(MSG_USER + 1);

using RpcHandler = ea::function<bool(const StringVariantMap&, StringVariantMap&)>;

/// Declarative typed-RPC endpoint.
struct URHO3D_API RpcDefinition
{
    ea::string name;
    RpcHandler handler;
    bool reliable{true};
    bool serverOnly{};
};

/// Typed RPC registry and transport adapter for rbfx Network connections.
class URHO3D_API RpcDispatcher
{
public:
    /// Register or replace an endpoint.
    bool Register(const RpcDefinition& definition);
    /// Remove an endpoint.
    bool Unregister(const ea::string& name);
    /// Find an endpoint.
    const RpcDefinition* Find(const ea::string& name) const;
    /// Return all endpoint definitions for tooling and validation.
    const ea::vector<RpcDefinition>& GetDefinitions() const { return definitions_; }

    /// Dispatch an already decoded call locally.
    bool Dispatch(const ea::string& name, const StringVariantMap& arguments, bool isServer,
        StringVariantMap* outputs = nullptr, ea::string* error = nullptr) const;
    /// Encode and send a call over a rbfx connection.
    bool Send(AbstractConnection* connection, const ea::string& name, const StringVariantMap& arguments,
        bool reliable = true) const;
    /// Decode and dispatch a message received under MSG_RBFX_RPC.
    bool HandleMessage(AbstractConnection* connection, MemoryBuffer& message, bool isServer,
        StringVariantMap* outputs = nullptr, ea::string* error = nullptr) const;

    /// Return the maximum payload accepted by the transport adapter.
    static unsigned GetMaximumPayloadSize() { return MaxNetworkMessageSize; }

private:
    const RpcDefinition* FindInternal(const ea::string& name) const;
    ea::vector<RpcDefinition> definitions_;
};

} // namespace Urho3D
