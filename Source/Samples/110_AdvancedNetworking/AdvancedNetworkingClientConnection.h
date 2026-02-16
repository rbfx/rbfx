// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "AdvancedNetworkingConnection.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Signal.h>
#include <Urho3D/Input/InputMap.h>
#include <Urho3D/Replica/NetworkTime.h>

#include <EASTL/functional.h>

namespace Urho3D
{
class MemoryBuffer;
class NetworkConnection;
class NetworkObject;
class ReplicationManager;
class Scene;
class Node;
}

class AdvancedNetworkingUI;

class AdvancedNetworkingClientConnection : public AdvancedNetworkingConnection
{
    URHO3D_OBJECT(AdvancedNetworkingClientConnection, AdvancedNetworkingConnection)
public:
    explicit AdvancedNetworkingClientConnection(Urho3D::Scene* scene, AdvancedNetworkingUI* ui);
    bool Connect(const ea::string& address, unsigned short port);

private:
    bool SendRaycastRequest(const DoubleVector3& origin, const DoubleVector3& target,
        const NetworkTime& replicaTime, const NetworkTime& inputTime);
    bool UpdateControl();
    void ProcessClientMovement(Urho3D::NetworkObject* clientObject);
    DoubleVector3 GetAimPosition(const DoubleVector3& playerPosition, const Ray& screenRay) const;
    void RequestClientRaycast(Urho3D::NetworkObject* clientObject, const Ray& screenRay);
    ea::optional<Vector3> RaycastImportantGeometries(const Ray& ray) const;
    void AddHitMarker(const DoubleVector3& position, bool isConfirmed);
    void HandleNetworkMessage(Urho3D::NetworkConnection* connection, NetworkMessageId messageId,
        Urho3D::MemoryBuffer& message, bool& handled);

    WeakPtr<Urho3D::Scene> scene_;
    WeakPtr<Urho3D::Node> cameraNode_;
    WeakPtr<Urho3D::Node> hitMarkersNode_;
    SharedPtr<Urho3D::InputMap> inputMap_;
    WeakPtr<AdvancedNetworkingUI> ui_;
    Timer autoMovementTimer_{};
    unsigned autoMovementPhase_{};
    Timer autoClickTimer_{};
    float yaw_{};
    float pitch_{};
};
