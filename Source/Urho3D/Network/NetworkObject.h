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

#include "../Container/FlagSet.h"
#include "../Scene/Component.h"
#include "../Network/NetworkManager.h"

namespace Urho3D
{

class AbstractConnection;

/// Helper base class for user-defined network replication logic.
class URHO3D_API NetworkObject : public Component
{
    URHO3D_OBJECT(NetworkObject, Component);

public:
    explicit NetworkObject(Context* context);
    ~NetworkObject() override;

    static void RegisterObject(Context* context);

    /// Assign network ID. On the Server, it's better to let the Server assign ID, to avoid unwanted side effects.
    void SetNetworkId(NetworkId networkId) { networkId_ = networkId; }
    /// Return current or last network ID. Return InvalidNetworkId if not registered.
    NetworkId GetNetworkId() const { return networkId_; }
    /// Return current or last network index. Return M_MAX_UNSIGNED if not registered.
    unsigned GetNetworkIndex() const { return networkId_ != InvalidNetworkId ? NetworkManager::DecomposeNetworkId(networkId_).first : M_MAX_UNSIGNED; }

    /// Return whether the component should be replicated for specified client connection.
    virtual bool IsRelevantForClient(AbstractConnection* connection);
    /// Callen when component is removed by server.
    virtual void OnRemovedOnClient();
    /// Write full snapshot on server.
    virtual void WriteSnapshot(VectorBuffer& dest);
    /// Write delta update on server. Delta is applied to previous delta or snapshot message.
    virtual bool WriteReliableDelta(VectorBuffer& dest);
    /// Read full snapshot on client.
    virtual void ReadSnapshot(VectorBuffer& src);
    /// Read delta update on client. Delta is applied to previous delta or snapshot message.
    virtual void ReadReliableDelta(VectorBuffer& src);

protected:
    /// Component implementation
    /// @{
    void OnSceneSet(Scene* scene) override;
    /// @}

private:
    /// NetworkManager corresponding to the NetworkObject.
    WeakPtr<NetworkManager> networkManager_;
    /// Network ID, unique within Scene. May contain outdated value if NetworkObject is not registered in any NetworkManager.
    NetworkId networkId_{ InvalidNetworkId };
};

}
