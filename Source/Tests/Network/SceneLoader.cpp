//
// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.
//

#include "../CommonUtils.h"
#include "../NetworkUtils.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Network/ReplicatedPeer.h>
#include <Urho3D/Network/SceneLoader.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

namespace
{

SharedPtr<Scene> CreateSceneAndResource(Context* context, const ea::string& sceneName, const ea::string& markerName)
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

bool LoadSceneFromParams(Context* context, Scene* scene, const StringVariantMap& params)
{
    const ea::string sceneName = params.at("Scene").GetString();
    auto* resource = context->GetSubsystem<ResourceCache>()->GetResource<BinaryFile>(sceneName, false);
    if (!resource)
        return false;

    Deserializer& source = resource->AsDeserializer();
    source.Seek(0);
    return scene->Load(source);
}

} // namespace

TEST_CASE("Server-client scene loader sequence")
{
    auto context = Tests::CreateCompleteContext();

    auto sceneA = CreateSceneAndResource(context, "Scenes/TestSceneLoader_ReloadA.bin", "MarkerA");
    auto sceneB = CreateSceneAndResource(context, "Scenes/TestSceneLoader_ReloadB.bin", "MarkerB");
    auto sceneC = CreateSceneAndResource(context, "Scenes/TestSceneLoader_ReloadC.bin", "MarkerC");

    RandomEngine random{0};
    const auto [serverConnection, clientConnection] = Tests::ManualConnection::CreatePair(context, random);

    auto serverLoader = MakeShared<ServerSceneLoader>(serverConnection, serverConnection);
    auto clientLoader = MakeShared<ClientSceneLoader>(clientConnection, clientConnection);

    auto clientScene = MakeShared<Scene>(context);
    clientLoader->SetScene(clientScene);

    StringVariantMap pendingParams;
    ea::function<void()> pendingCompletion;
    unsigned requestCount{};
    clientLoader->SetLoadRequestedCallback([&](const StringVariantMap& params, ea::function<void()> onCompleted)
    {
        pendingParams = params;
        pendingCompletion = ea::move(onCompleted);
        ++requestCount;
    });

    // Load scene A on server
    serverLoader->SetScene(sceneA, {{"Scene", sceneA->GetFileName()}});
    serverConnection->IncrementTime(1);

    REQUIRE(requestCount == 1);
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == nullptr);

    // Send invalid confirmation
    VectorBuffer invalidResult;
    invalidResult.WriteUInt(0xDEADBEEF);
    clientConnection->SendMessage(MSG_SCENE_LOAD_RESULT, invalidResult.GetBuffer());
    clientConnection->IncrementTime(1);
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == nullptr);

    // Load scene A load on client
    REQUIRE(LoadSceneFromParams(context, clientScene, pendingParams));
    pendingCompletion();
    pendingCompletion = {};

    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == clientScene->GetComponent<ReplicationManager>());
    REQUIRE(clientScene->FindChild("MarkerA", true) != nullptr);
    REQUIRE(clientScene->FindChild("MarkerB", true) == nullptr);
    REQUIRE(clientScene->FindChild("MarkerC", true) == nullptr);

    // Confirm scene A load on server
    clientConnection->IncrementTime(1);
    REQUIRE(serverConnection->GetReplicationManager() == sceneA->GetComponent<ReplicationManager>());
    REQUIRE(clientConnection->GetReplicationManager() == clientScene->GetComponent<ReplicationManager>());

    // Load scene B on server
    serverLoader->SetScene(sceneB, {{"Scene", sceneB->GetFileName()}});
    REQUIRE(requestCount == 1);
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == clientScene->GetComponent<ReplicationManager>());

    // Start loading scene B on client
    serverConnection->IncrementTime(1);
    REQUIRE(requestCount == 2);
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == nullptr);

    // Load scene C on server
    serverLoader->SetScene(sceneC, {{"Scene", sceneC->GetFileName()}});
    serverConnection->IncrementTime(1);
    REQUIRE(requestCount == 3);
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == nullptr);

    // Load scene C load on client
    REQUIRE(LoadSceneFromParams(context, clientScene, pendingParams));
    pendingCompletion();
    pendingCompletion = {};
    REQUIRE(serverConnection->GetReplicationManager() == nullptr);
    REQUIRE(clientConnection->GetReplicationManager() == clientScene->GetComponent<ReplicationManager>());

    // Confirm scene A load on server
    clientConnection->IncrementTime(1);
    REQUIRE(serverConnection->GetReplicationManager() == sceneC->GetComponent<ReplicationManager>());
    REQUIRE(clientConnection->GetReplicationManager() == clientScene->GetComponent<ReplicationManager>());
    REQUIRE(clientScene->FindChild("MarkerA", true) == nullptr);
    REQUIRE(clientScene->FindChild("MarkerB", true) == nullptr);
    REQUIRE(clientScene->FindChild("MarkerC", true) != nullptr);
}
