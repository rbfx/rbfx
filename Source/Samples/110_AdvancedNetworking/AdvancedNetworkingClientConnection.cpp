// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "AdvancedNetworkingClientConnection.h"

#include "AdvancedNetworkingPlayer.h"
#include "AdvancedNetworkingRaycast.h"
#include "AdvancedNetworkingUI.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/MoveAndOrbitController.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Replica/PredictedKinematicController.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace
{
static constexpr float CAMERA_DISTANCE = 5.0f;
static constexpr float CAMERA_OFFSET = 2.0f;
static constexpr float WALK_VELOCITY = 3.35f;
static constexpr float HIT_DISTANCE = 100.0f;
}

AdvancedNetworkingClientConnection::AdvancedNetworkingClientConnection(Scene* scene, AdvancedNetworkingUI* ui)
    : AdvancedNetworkingConnection(scene->GetContext())
    , scene_(scene)
    , ui_(ui)
{
    onMessage_.SubscribeWithSender(this, &AdvancedNetworkingClientConnection::HandleNetworkMessage);

    cameraNode_ = scene->GetChild("Camera");
    hitMarkersNode_ = scene->GetChild("Hit Markers");
    inputMap_ = InputMap::Load(context_, "Input/MoveAndOrbit.inputmap");

    // Subscribe to PostUpdate instead of the usual Update so that physics simulation has
    // already proceeded for the frame, and can accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE, &AdvancedNetworkingClientConnection::UpdateControl);
}

bool AdvancedNetworkingClientConnection::Connect(const ea::string& address, unsigned short port)
{
    Disconnect();

    ea::string normalizedAddress = address;
    normalizedAddress.trim();
    if (normalizedAddress.empty())
        normalizedAddress = "localhost";

    GetReplicatedPeer()->SetReplicationManager(scene_->GetComponent<ReplicationManager>());

    const URL connectUrl{Format("ws://{}:{}", normalizedAddress, port)};
    if (!BaseClassName::Connect(connectUrl))
    {
        URHO3D_LOGERROR("AdvancedNetworking: failed to start connection to {}", connectUrl.ToString());
        return false;
    }

    URHO3D_LOGINFO("AdvancedNetworking: connecting to {}", connectUrl.ToString());
    return true;
}

bool AdvancedNetworkingClientConnection::UpdateControl()
{
    if (!scene_ || !cameraNode_ || !ui_)
        return false;

    auto* input = context_->GetSubsystem<Input>();

    const bool isCameraMoving = input->GetMouseButtonDown(MOUSEB_RIGHT);
    input->SetMouseMode(isCameraMoving ? MM_RELATIVE : MM_FREE);
    input->SetMouseVisible(!isCameraMoving);

    const float mouseSensitivity = 0.1f;
    if (isCameraMoving)
    {
        const IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += mouseSensitivity * mouseMove.x_;
        pitch_ += mouseSensitivity * mouseMove.y_;
        pitch_ = Clamp(pitch_, 1.0f, 90.0f);
    }
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    auto* clientReplica = replicationManager ? replicationManager->GetClientReplica() : nullptr;
    if (!clientReplica || !clientReplica->HasOwnedNetworkObjects())
        return false;

    auto* clientObject = clientReplica->GetOwnedNetworkObject();
    ProcessClientMovement(clientObject);

    bool autoClick = false;
    if (ui_->GetCheatAutoClick())
    {
        autoClick = autoClickTimer_.GetMSec(false) >= 250;
        if (autoClick)
            autoClickTimer_.Reset();
    }

    if (input->GetMouseButtonPress(MOUSEB_LEFT) || autoClick)
    {
        auto* renderer = context_->GetSubsystem<Renderer>();
        auto* viewport = renderer->GetViewport(0);
        const IntVector2 mousePos = input->GetMousePosition();
        const Ray screenRay = viewport->GetScreenRay(mousePos.x_, mousePos.y_);
        RequestClientRaycast(clientObject, screenRay);
    }

    return true;
}

bool AdvancedNetworkingClientConnection::SendRaycastRequest(const DoubleVector3& origin, const DoubleVector3& target,
    const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!IsConnected())
        return false;

    VectorBuffer msg;
    WriteRaycastRequest(msg, origin, target, replicaTime, inputTime);
    SendMessage(MSG_ADVANCEDNETWORKING_RAYCAST, msg, PacketType::UnreliableUnordered);
    return true;
}

void AdvancedNetworkingClientConnection::ProcessClientMovement(Urho3D::NetworkObject* clientObject)
{
    auto* input = context_->GetSubsystem<Input>();

    Node* clientNode = clientObject->GetNode();
    auto clientController = clientNode->GetComponent<PredictedKinematicController>();

    const bool autoMovement = ui_->GetCheatAutoMovementCircle();
    if (autoMovement && autoMovementTimer_.GetMSec(false) >= 1000)
    {
        autoMovementTimer_.Reset();
        autoMovementPhase_ = (autoMovementPhase_ + 1) % 4;
    }

    const Quaternion rotation(0.0f, yaw_, 0.0f);
    Vector3 direction;
    if (inputMap_)
    {
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_FORWARD) > 0.5f
            || (autoMovement && autoMovementPhase_ == 3))
            direction += rotation * Vector3::FORWARD;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_BACK) > 0.5f
            || (autoMovement && autoMovementPhase_ == 1))
            direction += rotation * Vector3::BACK;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_LEFT) > 0.5f
            || (autoMovement && autoMovementPhase_ == 2))
            direction += rotation * Vector3::LEFT;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_RIGHT) > 0.5f
            || (autoMovement && autoMovementPhase_ == 0))
            direction += rotation * Vector3::RIGHT;
    }
    direction = direction.NormalizedOrDefault();

    const bool needJump = input->GetKeyDown(KEY_SPACE);
    clientController->SetWalkVelocity(direction * WALK_VELOCITY);
    if (needJump)
        clientController->SetJump();

    cameraNode_->SetPosition(clientNode->GetPosition()
        + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE + Vector3::UP * CAMERA_OFFSET);
}

DoubleVector3 AdvancedNetworkingClientConnection::GetAimPosition(const DoubleVector3& playerPosition, const Ray& screenRay) const
{
    if (!scene_)
        return (screenRay.origin_ + screenRay.direction_ * HIT_DISTANCE).Cast<DoubleVector3>();

    if (ui_->GetCheatAutoAimHand())
    {
        ea::vector<AnimatedModel*> models;
        scene_->FindComponents(models);

        AnimatedModel* closestModel{};
        float closestDistance{};
        for (AnimatedModel* model : models)
        {
            if (model->GetViewMask() == UNIMPORTANT_VIEW_MASK)
                continue;

            const float distance =
                model->GetNode()->GetWorldPosition().DistanceToPoint(scene_->ToRelativeWorldPosition(playerPosition));
            if (!closestModel || distance < closestDistance)
            {
                closestModel = model;
                closestDistance = distance;
            }
        }

        if (closestModel)
        {
            Node* aimNode = closestModel->GetSkeleton().GetBone("Mutant:RightHandIndex2")->node_;
            return scene_->ToAbsoluteWorldPosition(aimNode->GetWorldPosition());
        }
    }

    const Vector3 defaultAimPosition = screenRay.origin_ + screenRay.direction_ * HIT_DISTANCE;
    return scene_->ToAbsoluteWorldPosition(RaycastImportantGeometries(screenRay).value_or(defaultAimPosition));
}

void AdvancedNetworkingClientConnection::RequestClientRaycast(Urho3D::NetworkObject* clientObject, const Ray& screenRay)
{
    if (!scene_ || !IsConnected())
        return;

    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    auto* clientReplica = replicationManager ? replicationManager->GetClientReplica() : nullptr;
    if (!clientReplica)
        return;

    const NetworkTime replicaTime = clientReplica->GetReplicaTime();
    const NetworkTime inputTime = clientReplica->GetInputTime();

    const DoubleVector3 playerPosition = scene_->ToAbsoluteWorldPosition(clientObject->GetNode()->GetWorldPosition());
    const DoubleVector3 aimPosition = GetAimPosition(playerPosition, screenRay);
    const DoubleVector3 origin = playerPosition + DoubleVector3::UP * CAMERA_OFFSET;
    const Ray castRay{scene_->ToRelativeWorldPosition(origin), (aimPosition - origin).Cast<Vector3>()};

    if (auto hitPosition = RaycastImportantGeometries(castRay))
        AddHitMarker(scene_->ToAbsoluteWorldPosition(*hitPosition), false);

    SendRaycastRequest(origin, aimPosition, replicaTime, inputTime);
}

ea::optional<Vector3> AdvancedNetworkingClientConnection::RaycastImportantGeometries(const Ray& ray) const
{
    if (!scene_)
        return ea::nullopt;

    auto* octree = scene_->GetComponent<Octree>();
    if (!octree)
        return ea::nullopt;

    RayOctreeQuery query(ray, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY, IMPORTANT_VIEW_MASK);
    octree->RaycastSingle(query);

    if (!query.result_.empty())
        return query.result_[0].position_;
    return ea::nullopt;
}

void AdvancedNetworkingClientConnection::AddHitMarker(const DoubleVector3& position, bool isConfirmed)
{
    if (!hitMarkersNode_)
        return;

    auto* cache = context_->GetSubsystem<ResourceCache>();

    if (hitMarkersNode_->GetNumChildren() >= 200)
        hitMarkersNode_->GetChild(0u)->Remove();

    Node* markerNode = hitMarkersNode_->CreateChild("Client Hit");
    markerNode->SetPosition(position.Cast<Vector3>());

    auto* markerModel = markerNode->CreateComponent<StaticModel>();
    markerModel->SetViewMask(UNIMPORTANT_VIEW_MASK);
    if (isConfirmed)
    {
        markerNode->SetScale(0.15f);
        markerModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        markerModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/GlowingGreen.xml"));
    }
    else
    {
        markerNode->SetScale(0.2f);
        markerModel->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
        markerModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/GlowingRed.xml"));
    }
}

void AdvancedNetworkingClientConnection::HandleNetworkMessage(Urho3D::NetworkConnection* connection,
    NetworkMessageId messageId, MemoryBuffer& message, bool& handled)
{
    if (messageId != MSG_ADVANCEDNETWORKING_RAYHIT)
        return;

    const ea::optional<DoubleVector3> hitPosition = ReadRaycastResult(message);
    if (hitPosition)
        AddHitMarker(*hitPosition, true);
    handled = true;
}
