// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/ReplicatedPeer.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class NetworkConnection;
class Scene;

/// Server-side scene reloader.
class URHO3D_API ServerSceneLoader : public Object
{
    URHO3D_OBJECT(ServerSceneLoader, Object);

public:
    ServerSceneLoader(NetworkConnection* connection, const SharedPtr<ReplicatedPeer, RefCounted>& replicatedPeer);

    /// Set scene to be (re)loaded on the client side.
    void SetScene(Scene* scene, const StringVariantMap& params = Variant::emptyStringVariantMap);

private:
    void BeginSceneLoad();

    void OnClientConnected(NetworkConnection* connection);
    void OnClientDisconnected(NetworkConnection* connection);
    void OnMessageReceived(
        NetworkConnection* connection, NetworkMessageId messageId, ConstByteSpan message, bool& handled);

private:
    WeakPtr<NetworkConnection> connection_{};
    SharedPtr<ReplicatedPeer, RefCounted> replicatedPeer_{};
    SharedPtr<Scene> scene_;
    StringVariantMap params_;

    ea::optional<unsigned> pendingRequestMagic_;
};

/// Client-side scene reloader.
class URHO3D_API ClientSceneLoader : public Object
{
    URHO3D_OBJECT(ClientSceneLoader, Object);

public:
    ClientSceneLoader(NetworkConnection* connection, const SharedPtr<ReplicatedPeer, RefCounted>& replicatedPeer);

    /// Set scene that receives load requests.
    void SetScene(Scene* scene);

    using LoadRequestedCallback = ea::function<void(const StringVariantMap& params, ea::function<void()> onCompleted)>;
    void SetLoadRequestedCallback(const LoadRequestedCallback& onLoadRequested) { onLoadRequested_ = onLoadRequested; }

protected:
    virtual void OnLoadRequested(const StringVariantMap& params, ea::function<void()> onCompleted);

private:
    void OnMessageReceived(
        NetworkConnection* connection, NetworkMessageId messageId, ConstByteSpan message, bool& handled);
    void OnSceneLoaded(unsigned magic);

private:
    WeakPtr<NetworkConnection> connection_{};
    SharedPtr<ReplicatedPeer, RefCounted> replicatedPeer_{};
    SharedPtr<Scene> scene_;
    LoadRequestedCallback onLoadRequested_;
};

} // namespace Urho3D
