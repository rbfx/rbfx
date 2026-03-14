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

namespace Urho3D
{

class NetworkConnection;
class Scene;

/// Helper class for network-driven scene loading.
///
/// This helper wraps a small request/response protocol around `MSG_LOAD_SCENE`
/// and `MSG_SCENE_LOAD_RESULT`. It is designed for cases where server and
/// client must switch the active `Scene` after a connection is already created,
/// while keeping the scene-loading handshake explicit and easy to customize.
///
/// High-level behavior:
/// - On the server, assign a scene with `SetScene()`. The helper sends a load request to the remote client.
/// - On the client, the helper receives the request, loads the requested scene resource into its owned `Scene`.
/// - After loading finishes, the client sends success/failure back to the server.
/// - The server receives the result and may finalize higher-level logic such as switching replication managers,
///   spawning player objects, or handling failure.
/// - If an optional replication-oriented `AbstractConnection` handle is provided to the constructor or `Attach()`,
///   the helper automatically assigns the scene's `ReplicationManager` when the client scene is ready and when the
///   server confirms a remote scene switch.
///
/// Ownership model:
/// - The helper stores the active scene in `SharedPtr<Scene>` and therefore owns it.
/// - `SetScene(Scene*)` transfers ownership into the helper.
/// - `SetScene(const SharedPtr<Scene>&)` shares ownership with the caller.
/// - `GetScene()` returns the currently owned scene.
///
/// Server-side usage:
/// 1. Create one `NetworkLoadableScene` for each `NetworkConnection`.
///    This helper stores per-connection protocol state and must not be shared between connections.
/// 2. It is valid and expected for multiple helpers to reference the same server-side `Scene`.
///    In that setup, several clients may load the same scene and therefore interact with each other through
///    the same scene content and the same server-side `ReplicationManager`.
/// 3. Call `Attach(connection, replicationConnection)` once the helper is associated with a `NetworkConnection`.
///    Pass the replication handle explicitly if replication-manager rebinding should be automatic.
/// 4. Call `SetScene(serverScene)` whenever that connection should instruct the remote client to load a new scene.
/// 5. Observe completion either by:
///    - overriding `OnRemoteLoadSceneFinished()`, or
///    - subscribing to `onRemoteLoadSceneFinished_`.
/// 6. In that completion handler, apply any server-specific follow-up. Typical examples:
///    - switch the connection to another `ReplicationManager`,
///    - spawn or migrate the controlled object,
///    - clear pending scene-switch state,
///    - abort or retry on failure.
///
/// Client-side usage:
/// 1. Create one `NetworkLoadableScene` attached to the client connection.
///    Pass the replication handle explicitly if the helper should rebind the client's `ReplicationManager`.
/// 2. Optionally assign a pre-created local scene with `SetScene()` before any remote request arrives.
///    If no scene is assigned, the helper creates one automatically on first load.
/// 3. When the server requests a scene, the helper loads the resource named in the request into its owned scene.
/// 4. Observe lifecycle either by:
///    - overriding `OnLoadSceneStarted()` / `OnLoadSceneFinished()`, or
///    - subscribing to `onLoadSceneStarted_` / `onLoadSceneFinished_`.
/// 5. In the load-finished handler, bind client-side systems to the newly loaded scene. Typical examples:
///    - recreate or revalidate the camera node,
///    - refresh UI or cached node references.
///    Replication manager rebinding is automatic only when an explicit replication handle was provided.
///
/// Customization points:
/// - Override `OnPrepareLoadSceneRequest()` to append custom request payload on the server.
/// - Override `OnLoadSceneRequestReceived()` to validate or reject the request on the client.
/// - Override `OnLoadSceneFinished()` to alter reported success or append response payload.
/// - Override `OnRemoteLoadSceneFinished()` to consume the client's response on the server.
/// - Use the corresponding signals when composition is preferable to subclassing:
///   `onPrepareLoadSceneRequest_`, `onLoadSceneRequestReceived_`, `onLoadSceneStarted_`,
///   `onLoadSceneFinished_`, and `onRemoteLoadSceneFinished_`.
///
/// Notes:
/// - The helper itself does not decide what to do after a successful scene change. It only guarantees the load handshake.
/// - Policy such as choosing the next scene, moving a player between scenes, or reconfiguring replication remains the
///   responsibility of the caller.
/// - `SetScene()` must not be called while a previous load request is still active.
/// - Re-attaching or disconnecting resets the internal transient loading state.
/// - Calling `Attach()` again with the same `NetworkConnection` updates only the optional replication handle.
class URHO3D_API NetworkLoadableScene : public Object
{
    URHO3D_OBJECT(NetworkLoadableScene, Object);

public:
    explicit NetworkLoadableScene(Context* context);
    /// Construct helper and attach it to a network connection.
    /// Optional `replicationConnection` enables automatic `ReplicationManager` rebinding.
    explicit NetworkLoadableScene(
        NetworkConnection* connection, SharedPtr<AbstractConnection, RefCounted> replicationConnection = {});

    /// Subscribe to connection signals for automatic load-scene handling.
    /// Optional `replicationConnection` enables automatic `ReplicationManager` rebinding.
    void Attach(NetworkConnection* connection, SharedPtr<AbstractConnection, RefCounted> replicationConnection = {});

    /// Set scene. Helper stores scene in SharedPtr and assumes ownership.
    /// On server-side connection this sends load-scene request to client.
    void SetScene(Scene* scene);
    /// Set scene. Helper stores scene in SharedPtr and assumes ownership.
    /// On server-side connection this sends load-scene request to client.
    void SetScene(const SharedPtr<Scene>& scene);

    /// Get attached network connection.
    NetworkConnection* GetConnection() const { return connection_; }
    /// Get currently owned scene.
    Scene* GetScene() const { return scene_; }
    /// Return true when local client-side scene loading is in progress.
    bool IsLoadingScene() const { return !loadingSceneName_.empty(); }
    /// Return currently loading scene file name, if any.
    const ea::string& GetLoadingSceneName() const { return loadingSceneName_; }
    /// Return true when a scene switch is in progress (either waiting for remote load result or loading locally).
    bool IsSwitchInProgress() const { return loadState_ != LoadState::Idle; }

    /// Sent before sending load-scene request on the server. Handlers may append custom payload.
    Signal<void(const ea::string& sceneFileName, VectorBuffer& request), NetworkLoadableScene> onPrepareLoadSceneRequest_;
    /// Sent when load-scene request is received on the client. Handlers may inspect payload and reject the request.
    Signal<void(const ea::string& sceneFileName, MemoryBuffer& request, bool& accept), NetworkLoadableScene>
        onLoadSceneRequestReceived_;
    /// Sent when local scene loading starts on the client.
    Signal<void(const ea::string& sceneFileName), NetworkLoadableScene> onLoadSceneStarted_;
    /// Sent when local scene loading finishes on the client.
    Signal<void(const ea::string& sceneFileName, bool& success, VectorBuffer& response), NetworkLoadableScene>
        onLoadSceneFinished_;
    /// Sent on the server when remote scene loading finishes.
    Signal<void(const ea::string& sceneFileName, bool success, MemoryBuffer& response), NetworkLoadableScene>
        onRemoteLoadSceneFinished_;

protected:
    /// Called before sending load-scene request (server side). You may append custom payload.
    virtual void OnPrepareLoadSceneRequest(const ea::string& sceneFileName, VectorBuffer& request);
    /// Called on client when load-scene request is received. Set accept=false to reject.
    virtual void OnLoadSceneRequestReceived(const ea::string& sceneFileName, MemoryBuffer& request, bool& accept);
    /// Called when local loading starts on client.
    virtual void OnLoadSceneStarted(const ea::string& sceneFileName);
    /// Called when local loading finishes on client. You may modify success and append response payload.
    virtual void OnLoadSceneFinished(const ea::string& sceneFileName, bool& success, VectorBuffer& response);
    /// Called on server when client reports scene loading result.
    virtual void OnRemoteLoadSceneFinished(const ea::string& sceneFileName, bool success, MemoryBuffer& response);

private:
    enum class LoadState
    {
        Idle,
        AwaitingRemoteLoadResult,
        LoadingLocalScene,
    };

    bool IsServerSideConnection() const;
    void HandleProtocolError(ea::string_view reason);
    void RequestRemoteLoadScene();
    void ProcessNetworkMessage(NetworkConnection* connection, NetworkMessageId messageId, MemoryBuffer& message, bool& handled);
    void ProcessLoadSceneRequest(MemoryBuffer& message);
    void ProcessLoadSceneResult(MemoryBuffer& message);

    bool StartLocalLoad(const ea::string& sceneFileName);
    void FinishLocalLoad(bool success);
    void SendLocalLoadResult(const ea::string& sceneFileName, bool success);

    void HandleConnectionConnected(NetworkConnection* connection);
    void HandleConnectionDisconnected(NetworkConnection* connection);
    void HandleAsyncLoadFinished(StringHash eventType, VariantMap& eventData);

    WeakPtr<NetworkConnection> connection_{};
    SharedPtr<AbstractConnection, RefCounted> replicationConnection_{};
    SharedPtr<Scene> scene_{};
    LoadState loadState_{LoadState::Idle};

    ea::string expectedSceneName_{};
    ea::string loadingSceneName_{};
};

} // namespace Urho3D
