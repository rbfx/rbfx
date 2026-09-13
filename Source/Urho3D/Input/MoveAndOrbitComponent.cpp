// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Input/MoveAndOrbitComponent.h>

namespace Urho3D
{

MoveAndOrbitComponent::MoveAndOrbitComponent(Context* context)
    : BaseClassName(context)
{

}

void MoveAndOrbitComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<MoveAndOrbitComponent>(Category_Logic);
}

void MoveAndOrbitComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    BaseClassName::OnNodeSet(previousNode, currentNode);

    if (currentNode)
    {
        const Quaternion& currentRotation = currentNode->GetRotation();
        SetYaw(currentRotation.YawAngle());
        SetPitch(currentRotation.PitchAngle());
    }
}

void MoveAndOrbitComponent::SetVelocity(const Vector3& velocity)
{
    velocity_ = velocity;
}

void MoveAndOrbitComponent::SetYaw(float yaw)
{
    yaw_ = yaw;
}

void MoveAndOrbitComponent::SetPitch(float pitch)
{
    pitch_ = Clamp(pitch, -90.0f, 90.0f);
}

void MoveAndOrbitComponent::SetDistanceLimits(float minDistance, float maxDistance)
{
    minDistance_ = minDistance;
    maxDistance_ = maxDistance;
}

} // namespace Urho3D
