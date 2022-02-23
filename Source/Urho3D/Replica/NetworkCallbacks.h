//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/Assert.h"
#include "../Container/FlagSet.h"
#include "../Replica/NetworkId.h"
#include "../Replica/NetworkTime.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class AbstractConnection;
class Deserializer;
class Serializer;

/// Mask used to enable and disable network callbacks.
/// Note that some callbacks are called unconditionally.
enum class NetworkCallbackMask
{
    None = 0,

    /// Server callbacks
    /// @{
    GetRelevanceForClient   = 1 << 0,
    UpdateTransformOnServer = 1 << 1,
    /// @}

    /// Client callbacks
    /// @{
    PrepareToRemove         = 1 << 2,
    InterpolateState        = 1 << 3,
    /// @}

    /// Common callbacks
    /// @{
    ReliableDelta           = 1 << 4,
    UnreliableDelta         = 1 << 5,
    UnreliableFeedback      = 1 << 6,

    Update                  = 1 << 7,
    /// @}
};
URHO3D_FLAGSET(NetworkCallbackMask, NetworkCallbackFlags);

/// Server-side callbacks for NetworkObject and NetworkBehavior.
/// ServerReplicator is guaranteed to be present.
class ServerNetworkCallback
{
public:
    /// Perform initialization. Called once.
    virtual void InitializeOnServer() {}

    /// Return whether the component should be replicated for specified client connection, and how frequently.
    /// The first reported valid relevance is used.
    virtual ea::optional<NetworkObjectRelevance> GetRelevanceForClient(AbstractConnection* connection) { return ea::nullopt; }
    /// Called when world transform or parent of the object is updated in Server mode.
    virtual void UpdateTransformOnServer() {}

    /// Write full snapshot.
    virtual void WriteSnapshot(NetworkFrame frame, Serializer& dest) {}

    /// Prepare for reliable delta update and return update mask. If mask is zero, reliable delta update is skipped.
    virtual bool PrepareReliableDelta(NetworkFrame frame) { return false; }
    /// Write reliable delta update. Delta is applied to previous delta or snapshot.
    virtual void WriteReliableDelta(NetworkFrame frame, Serializer& dest) {}
    /// Prepare for unreliable delta update and return update mask. If mask is zero, unreliable delta update is skipped.
    virtual bool PrepareUnreliableDelta(NetworkFrame frame) { return false; }
    /// Write unreliable delta update.
    virtual void WriteUnreliableDelta(NetworkFrame frame, Serializer& dest) {}

    /// Read unreliable feedback from client.
    virtual void ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src) {}
};

/// Client-side callbacks for NetworkObject and NetworkBehavior.
/// ClientReplica is guaranteed to be present.
class ClientNetworkCallback
{
public:
    /// Read full snapshot and initialize object. Called once.
    virtual void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) {}
    /// This component is about to be removed by the authority of the server.
    virtual void PrepareToRemove() {}

    /// Interpolate replicated state.
    virtual void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) {}

    /// Read reliable delta update. Delta is applied to previous reliable delta or snapshot message.
    virtual void ReadReliableDelta(NetworkFrame frame, Deserializer& src) {}
    /// Read unreliable delta update.
    virtual void ReadUnreliableDelta(NetworkFrame frame, Deserializer& src) {}

    /// Prepare for unreliable feedback and return feedback mask. If mask is zero, unreliable feedback is skipped.
    virtual bool PrepareUnreliableFeedback(NetworkFrame frame) { return false; }
    /// Write unreliable feedback from client.
    virtual void WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest) {}
};

/// Aggregate network-related callbacks used by NetworkObject and NetworkBehavior.
class NetworkCallback : public ServerNetworkCallback, public ClientNetworkCallback
{
public:
    /// Initialize object in standalone mode when neither server nor client is running.
    virtual void InitializeStandalone() {};
    /// Process generic network update.
    virtual void Update(float replicaTimeStep, float inputTimeStep) {}
};

}
