//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Container/IndexAllocator.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/ClientNetworkManager.h"
#include "../Network/ProtocolMessages.h"
#include "../Network/ServerNetworkManager.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class Connection;
class Network;
class NetworkComponent;

/// Subsystem that keeps trace of all NetworkComponent in the Scene.
/// Built-in in Scene instead of being independent component for quick access and easier management.
class URHO3D_API NetworkManager : public Object
{
    URHO3D_OBJECT(NetworkManager, Object);

public:
    using NetworkComponentById = ea::unordered_map<unsigned, NetworkComponent*>;

    explicit NetworkManager(Scene* scene);
    ~NetworkManager() override;

    /// Switch network manager to server mode. It's not supposed to be called on NetworkManager in client mode.
    void MarkAsServer();
    /// Switch network manager to client mode. It's not supposed to be called on NetworkManager in server mode.
    void MarkAsClient(AbstractConnection* connectionToServer);
    /// Return expected client or server network manager.
    ServerNetworkManager& AsServer();
    ClientNetworkManager& AsClient();

    /// Add new NetworkComponent and allocate ID for it.
    void AddNetworkComponent(NetworkComponent* networkComponent);
    /// Remove existing NetworkComponent and deallocate its ID.
    void RemoveNetworkComponent(NetworkComponent* networkComponent);

    /// Process network message either as client or as server.
    void ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData);

    /// Getters
    /// @{
    const NetworkComponentById& GetNetworkComponents() const { return networkComponents_; }
    Scene* GetScene() const { return scene_; }
    ea::string ToString() const;
    /// @}

private:
    Network* network_{};
    Scene* scene_{};

    SharedPtr<ServerNetworkManager> server_;
    SharedPtr<ClientNetworkManager> client_;

    IndexAllocator idAllocator_;
    NetworkComponentById networkComponents_;
};

}
