// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RpcDispatcher.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>

namespace Urho3D
{

bool RpcDispatcher::Register(const RpcDefinition& definition)
{
    if (definition.name.empty() || !definition.handler)
        return false;

    for (RpcDefinition& current : definitions_)
    {
        if (current.name == definition.name)
        {
            current = definition;
            return true;
        }
    }

    definitions_.push_back(definition);
    return true;
}

bool RpcDispatcher::Unregister(const ea::string& name)
{
    for (auto iter = definitions_.begin(); iter != definitions_.end(); ++iter)
    {
        if (iter->name == name)
        {
            definitions_.erase(iter);
            return true;
        }
    }
    return false;
}

const RpcDefinition* RpcDispatcher::FindInternal(const ea::string& name) const
{
    for (const RpcDefinition& definition : definitions_)
    {
        if (definition.name == name)
            return &definition;
    }
    return nullptr;
}

const RpcDefinition* RpcDispatcher::Find(const ea::string& name) const
{
    return FindInternal(name);
}

bool RpcDispatcher::Dispatch(const ea::string& name, const StringVariantMap& arguments, bool isServer,
    StringVariantMap* outputs, ea::string* error) const
{
    const RpcDefinition* definition = FindInternal(name);
    if (!definition)
    {
        if (error)
            *error = "Unknown RPC endpoint: " + name;
        return false;
    }

    if (definition->serverOnly && !isServer)
    {
        if (error)
            *error = "RPC endpoint is server-only: " + name;
        return false;
    }

    StringVariantMap localOutputs;
    if (!definition->handler(arguments, localOutputs))
    {
        if (error)
            *error = "RPC handler rejected call: " + name;
        return false;
    }

    if (outputs)
        *outputs = ea::move(localOutputs);
    return true;
}

bool RpcDispatcher::Send(AbstractConnection* connection, const ea::string& name,
    const StringVariantMap& arguments, bool reliable) const
{
    if (!connection || name.empty())
        return false;

    VectorBuffer payload;
    if (!payload.WriteString(name) || !payload.WriteStringVariantMap(arguments))
        return false;

    if (payload.GetSize() > MaxNetworkMessageSize)
        return false;

    const PacketTypeFlags packetType = reliable ? PacketType::ReliableOrdered : PacketType::UnreliableUnordered;
    connection->SendMessage(MSG_RBFX_RPC, payload, packetType);
    return true;
}

bool RpcDispatcher::HandleMessage(AbstractConnection* /*connection*/, MemoryBuffer& message, bool isServer,
    StringVariantMap* outputs, ea::string* error) const
{
    if (message.IsEof())
    {
        if (error)
            *error = "RPC message is empty.";
        return false;
    }

    const ea::string name = message.ReadString();
    if (message.IsEof() || name.empty())
    {
        if (error)
            *error = "RPC message has no valid endpoint name.";
        return false;
    }

    const StringVariantMap arguments = message.ReadStringVariantMap();
    if (message.IsEof())
    {
        if (error)
            *error = "RPC message payload is truncated.";
        return false;
    }

    return Dispatch(name, arguments, isServer, outputs, error);
}

} // namespace Urho3D
