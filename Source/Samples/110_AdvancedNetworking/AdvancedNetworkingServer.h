// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "AdvancedNetworkingRaycast.h"

#include <Urho3D/Network/Transport/DataChannel/DataChannelServer.h>

#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{
class Scene;
class Node;
class MemoryBuffer;
}

class AdvancedNetworkingServer : public DataChannelServer
{
public:
    explicit AdvancedNetworkingServer(Scene* scene);

    bool Start(unsigned short port);
    void Stop();

    void ProcessRaycasts();
    unsigned GetClientCount() const;

private:
    Node* CreateControllableObject(SharedPtr<ReplicatedPeer, RefCounted> owner);
    void ProcessSingleRaycast(const ServerRaycastInfo& raycastInfo);
    void HandleServerConnected(NetworkConnection* connection);
    void HandleServerDisconnected(NetworkConnection* connection);
    void HandleNetworkMessage(Urho3D::NetworkConnection* connection, NetworkMessageId messageId,
        Urho3D::MemoryBuffer& message, bool& handled);

    WeakPtr<Scene> scene_;
    ea::unordered_map<NetworkConnection*, WeakPtr<Node>> serverObjects_;
    ea::vector<ServerRaycastInfo> serverRaycasts_;
};
