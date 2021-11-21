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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkComponent.h"
#include "../Network/NetworkManager.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

NetworkManager::NetworkManager(Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
{}

NetworkManager::~NetworkManager() = default;

void NetworkManager::MarkAsServer()
{
    if (client_)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from client to server mode");
        client_ = nullptr;
        assert(0);
    }

    if (!server_)
    {
        server_ = MakeShared<ServerNetworkManager>(scene_);
    }
}

void NetworkManager::MarkAsClient(AbstractConnection* connectionToServer)
{
    if (server_)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from client to server mode");
        server_ = nullptr;
        assert(0);
    }

    if (client_ && client_->GetConnection() != connectionToServer)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from one server to another without scene recreation");
        client_ = nullptr;
        assert(0);
    }

    if (!client_)
    {
        client_ = MakeShared<ClientNetworkManager>(scene_, connectionToServer);
    }
}

ServerNetworkManager& NetworkManager::AsServer()
{
    assert(server_);
    return *server_;
}

ClientNetworkManager& NetworkManager::AsClient()
{
    assert(client_);
    return *client_;
}

void NetworkManager::AddNetworkComponent(NetworkComponent* networkComponent)
{
    if (networkComponents_.contains(networkComponent->GetNetworkID()))
    {
        URHO3D_LOGERROR("NetworkComponent is already added");
        return;
    }

    const unsigned networkId = idAllocator_.Allocate();
    networkComponents_.emplace(networkId, networkComponent);
    networkComponent->SetNetworkID(networkId);
}

void NetworkManager::RemoveNetworkComponent(NetworkComponent* networkComponent)
{
    if (!networkComponents_.contains(networkComponent->GetNetworkID()))
    {
        URHO3D_LOGERROR("NetworkComponent is not added");
        return;
    }

    const unsigned networkId = networkComponent->GetNetworkID();
    networkComponent->SetNetworkID(M_MAX_UNSIGNED);
    idAllocator_.Release(networkId);
    networkComponents_.erase(networkId);
}

void NetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (!server_ && !client_)
    {
        URHO3D_LOGWARNING("Uninitialized NetworkManager cannot process incoming message");
        return;
    }

    if (server_)
        server_->ProcessMessage(connection, messageId, messageData);
    else
        client_->ProcessMessage(messageId, messageData);
}

ea::string NetworkManager::ToString() const
{
    if (server_)
        return server_->ToString();
    else if (client_)
        return client_->ToString();
    else
        return "";
}

}
