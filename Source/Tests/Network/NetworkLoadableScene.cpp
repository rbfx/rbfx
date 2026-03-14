//
// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.
//

#include "../CommonUtils.h"
#include "../NetworkUtils.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Network/NetworkLoadableScene.h>
#include <Urho3D/Network/ReplicatedPeer.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

namespace
{
SharedPtr<Scene> CreateBinarySceneResource(Context* context, const ea::string& sceneName, const ea::string& markerName)
{
    auto scene = MakeShared<Scene>(context);
    scene->CreateChild(markerName)->SetPosition(Vector3(1.0f, 2.0f, 3.0f));

    auto resource = MakeShared<BinaryFile>(context);
    REQUIRE(scene->Save(resource->AsSerializer()));

    scene->SetFileName(sceneName);
    resource->SetName(sceneName);
    context->GetSubsystem<ResourceCache>()->AddManualResource(resource);
    return scene;
}

SharedPtr<Scene> CreateBinarySceneResourceWithReplicationManager(
    Context* context, const ea::string& sceneName, const ea::string& markerName)
{
    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<ReplicationManager>();
    scene->CreateChild(markerName)->SetPosition(Vector3(1.0f, 2.0f, 3.0f));

    auto resource = MakeShared<BinaryFile>(context);
    REQUIRE(scene->Save(resource->AsSerializer()));

    scene->SetFileName(sceneName);
    resource->SetName(sceneName);
    context->GetSubsystem<ResourceCache>()->AddManualResource(resource);
    return scene;
}

class ReplicatedInMemoryConnection : public Tests::InMemoryConnection
{
    URHO3D_OBJECT(ReplicatedInMemoryConnection, Tests::InMemoryConnection);

public:
    explicit ReplicatedInMemoryConnection(Context* context)
        : Tests::InMemoryConnection(context)
        , replicatedPeer_(this)
    {
    }

    SharedPtr<AbstractConnection, RefCounted> GetReplicatedPeer()
    {
        return SharedPtr<AbstractConnection, RefCounted>(&replicatedPeer_, this);
    }

private:
    AbstractConnection replicatedPeer_;
};

} // namespace

TEST_CASE("NetworkLoadableScene sends pending load request once connection is established")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<Tests::InMemoryConnection>(context);
    auto clientConnection = MakeShared<Tests::InMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection);
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection);

    ea::string clientLoadedSceneName;
    bool clientLoadedSuccessfully{};
    clientLoadableScene->onLoadSceneFinished_.Subscribe(clientLoadableScene.Get(),
        [&](const ea::string& sceneFileName, bool& success, VectorBuffer&)
        {
            clientLoadedSceneName = sceneFileName;
            clientLoadedSuccessfully = success;
        });

    ea::string serverFinishedSceneName;
    bool serverLoadedSuccessfully{};
    unsigned serverFinishedCount{};
    serverLoadableScene->onRemoteLoadSceneFinished_.Subscribe(serverLoadableScene.Get(),
        [&](const ea::string& sceneFileName, bool success, MemoryBuffer&)
        {
            serverFinishedSceneName = sceneFileName;
            serverLoadedSuccessfully = success;
            ++serverFinishedCount;
        });

    const ea::string sceneName = "Scenes/TestNetworkLoadableScene_A.bin";
    SharedPtr<Scene> serverScene = CreateBinarySceneResource(context, sceneName, "MarkerA");

    serverLoadableScene->SetScene(serverScene);
    REQUIRE(serverConnection->GetNumPendingPackets() == 0);
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    REQUIRE(serverConnection->GetNumPendingPackets() == 1);
    REQUIRE(serverLoadableScene->IsSwitchInProgress());

    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientLoadedSuccessfully);
    REQUIRE(clientLoadedSceneName == sceneName);
    REQUIRE(clientConnection->GetNumPendingPackets() == 1);
    REQUIRE(clientLoadableScene->GetScene() != nullptr);
    REQUIRE(clientLoadableScene->GetScene()->FindChild("MarkerA", true) != nullptr);

    REQUIRE(clientConnection->FlushOutgoing() == 1);
    REQUIRE(serverFinishedCount == 1);
    REQUIRE(serverLoadedSuccessfully);
    REQUIRE(serverFinishedSceneName == sceneName);
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());
}

TEST_CASE("NetworkLoadableScene switches between in-memory binary scenes")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<ReplicatedInMemoryConnection>(context);
    auto clientConnection = MakeShared<ReplicatedInMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection, serverConnection->GetReplicatedPeer());
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection, clientConnection->GetReplicatedPeer());

    unsigned clientFinishedCount{};
    clientLoadableScene->onLoadSceneFinished_.Subscribe(clientLoadableScene.Get(),
        [&](const ea::string&, bool& success, VectorBuffer&)
        {
            REQUIRE(success);
            ++clientFinishedCount;
        });

    unsigned serverFinishedCount{};
    serverLoadableScene->onRemoteLoadSceneFinished_.Subscribe(serverLoadableScene.Get(),
        [&](const ea::string&, bool success, MemoryBuffer&)
        {
            REQUIRE(success);
            ++serverFinishedCount;
        });

    const SharedPtr<Scene> firstScene = CreateBinarySceneResource(context, "Scenes/TestNetworkLoadableScene_B1.bin", "MarkerB1");
    const SharedPtr<Scene> secondScene = CreateBinarySceneResource(context, "Scenes/TestNetworkLoadableScene_B2.bin", "MarkerB2");

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    serverLoadableScene->SetScene(firstScene);
    REQUIRE(serverLoadableScene->IsSwitchInProgress());
    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientConnection->FlushOutgoing() == 1);
    REQUIRE(serverFinishedCount == 1);
    REQUIRE(clientFinishedCount == 1);
    REQUIRE(clientLoadableScene->GetScene()->FindChild("MarkerB1", true) != nullptr);
    REQUIRE(clientLoadableScene->GetScene()->FindChild("MarkerB2", true) == nullptr);

    serverLoadableScene->SetScene(secondScene);
    REQUIRE(serverLoadableScene->IsSwitchInProgress());
    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientLoadableScene->GetScene()->FindChild("MarkerB2", true) != nullptr);
    REQUIRE(clientLoadableScene->GetScene()->FindChild("MarkerB1", true) == nullptr);
    REQUIRE(clientConnection->FlushOutgoing() == 1);

    REQUIRE(serverFinishedCount == 2);
    REQUIRE(clientFinishedCount == 2);
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());
}

TEST_CASE("NetworkLoadableScene may reject client load request")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<Tests::InMemoryConnection>(context);
    auto clientConnection = MakeShared<Tests::InMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection);
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection);

    clientLoadableScene->onLoadSceneRequestReceived_.Subscribe(clientLoadableScene.Get(),
        [](const ea::string&, MemoryBuffer&, bool& accept)
        {
            accept = false;
        });

    bool clientFinished{};
    bool clientSuccess{true};
    clientLoadableScene->onLoadSceneFinished_.Subscribe(clientLoadableScene.Get(),
        [&](const ea::string&, bool& success, VectorBuffer&)
        {
            clientFinished = true;
            clientSuccess = success;
        });

    bool serverFinished{};
    bool serverSuccess{true};
    serverLoadableScene->onRemoteLoadSceneFinished_.Subscribe(serverLoadableScene.Get(),
        [&](const ea::string&, bool success, MemoryBuffer&)
        {
            serverFinished = true;
            serverSuccess = success;
        });

    const SharedPtr<Scene> serverScene =
        CreateBinarySceneResource(context, "Scenes/TestNetworkLoadableScene_Reject.bin", "MarkerReject");

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    serverLoadableScene->SetScene(serverScene);
    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientFinished);
    REQUIRE_FALSE(clientSuccess);
    REQUIRE(clientLoadableScene->GetScene() == nullptr);
    REQUIRE(clientConnection->GetNumPendingPackets() == 1);

    REQUIRE(clientConnection->FlushOutgoing() == 1);
    REQUIRE(serverFinished);
    REQUIRE_FALSE(serverSuccess);
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());
}

TEST_CASE("NetworkLoadableScene reports failure when scene resource is missing")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<Tests::InMemoryConnection>(context);
    auto clientConnection = MakeShared<Tests::InMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection);
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection);

    bool clientFinished{};
    bool clientSuccess{true};
    clientLoadableScene->onLoadSceneFinished_.Subscribe(clientLoadableScene.Get(),
        [&](const ea::string&, bool& success, VectorBuffer&)
        {
            clientFinished = true;
            clientSuccess = success;
        });

    bool serverFinished{};
    bool serverSuccess{true};
    serverLoadableScene->onRemoteLoadSceneFinished_.Subscribe(serverLoadableScene.Get(),
        [&](const ea::string&, bool success, MemoryBuffer&)
        {
            serverFinished = true;
            serverSuccess = success;
        });

    const auto missingScene = MakeShared<Scene>(context);
    missingScene->SetFileName("Scenes/TestNetworkLoadableScene_Missing.bin");

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    serverLoadableScene->SetScene(missingScene);
    REQUIRE(serverLoadableScene->IsSwitchInProgress());
    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientFinished);
    REQUIRE_FALSE(clientSuccess);
    REQUIRE(clientLoadableScene->GetScene() != nullptr);
    REQUIRE(clientConnection->GetNumPendingPackets() == 1);

    REQUIRE(clientConnection->FlushOutgoing() == 1);
    REQUIRE(serverFinished);
    REQUIRE_FALSE(serverSuccess);
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());
}

TEST_CASE("NetworkLoadableScene resets pending server switch on disconnect")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<Tests::InMemoryConnection>(context);
    auto clientConnection = MakeShared<Tests::InMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection);
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection);

    const SharedPtr<Scene> serverScene =
        CreateBinarySceneResource(context, "Scenes/TestNetworkLoadableScene_Disconnect.bin", "MarkerDisconnect");

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    serverLoadableScene->SetScene(serverScene);
    REQUIRE(serverLoadableScene->IsSwitchInProgress());
    REQUIRE(serverConnection->FlushOutgoing() == 1);
    REQUIRE(clientLoadableScene->GetScene() != nullptr);
    REQUIRE(clientConnection->GetNumPendingPackets() == 1);

    serverConnection->Disconnect();
    REQUIRE_FALSE(serverLoadableScene->IsSwitchInProgress());
}

TEST_CASE("NetworkLoadableScene wires replication managers automatically")
{
    auto context = Tests::CreateCompleteContext();

    auto server = MakeShared<Tests::TestNetworkServer>(context);
    auto serverConnection = MakeShared<ReplicatedInMemoryConnection>(context);
    auto clientConnection = MakeShared<ReplicatedInMemoryConnection>(context);
    serverConnection->SetPeer(clientConnection.Get());
    clientConnection->SetPeer(serverConnection.Get());
    serverConnection->SetServerSide(server.Get());

    auto serverLoadableScene = MakeShared<NetworkLoadableScene>(serverConnection, serverConnection->GetReplicatedPeer());
    auto clientLoadableScene = MakeShared<NetworkLoadableScene>(clientConnection, clientConnection->GetReplicatedPeer());

    const SharedPtr<Scene> serverScene = CreateBinarySceneResourceWithReplicationManager(
        context, "Scenes/TestNetworkLoadableScene_Replication.bin", "MarkerReplication");
    auto* serverReplicationManager = serverScene->GetComponent<ReplicationManager>();
    serverReplicationManager->StartServer();

    clientConnection->DispatchConnected();
    serverConnection->DispatchConnected();

    serverLoadableScene->SetScene(serverScene);
    REQUIRE(serverConnection->FlushOutgoing() == 1);

    Scene* clientScene = clientLoadableScene->GetScene();
    REQUIRE(clientScene != nullptr);
    auto* clientReplicationManager = clientScene->GetComponent<ReplicationManager>();
    REQUIRE(clientReplicationManager != nullptr);
    REQUIRE(clientConnection->GetReplicatedPeer()->GetReplicationManager() == clientReplicationManager);

    REQUIRE(clientConnection->FlushOutgoing() == 1);
    REQUIRE(serverConnection->GetReplicatedPeer()->GetReplicationManager() == serverReplicationManager);
}
