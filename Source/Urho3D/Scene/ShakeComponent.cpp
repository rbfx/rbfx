// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Scene/ShakeComponent.h"

#include "Urho3D/Scene/Node.h"
#include "Urho3D/Core/Context.h"

namespace Urho3D
{

ShakeComponent::ShakeComponent(Context* context)
    : BaseClassName(context)
    , perlinNoise_(RandomEngine::GetDefaultEngine())
{
    SetUpdateEventMask(USE_NO_EVENT);
}

ShakeComponent::~ShakeComponent()
{
}

void ShakeComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ShakeComponent>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Trauma", GetTrauma, SetTrauma, float, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Trauma Power", float, traumaPower_, 2.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Trauma Falloff", float, traumaFalloff_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Time Scale", float, timeScale_, DEFAULT_TIMESCALE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Shift Range", Vector3, shiftRange_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Range", Vector3, rotationRange_, Vector3::ZERO, AM_DEFAULT);
}

void ShakeComponent::SetTimeScale(float value)
{
    timeScale_ = value;
}

void ShakeComponent::AddTrauma(float value)
{
    SetTrauma(trauma_ + value);
}

void ShakeComponent::SetTrauma(float value)
{
    trauma_ = Max(value, 0.0f);

    if (trauma_ > 0.0f)
    {
        if (static_cast<UpdateEventFlags>(0) == (GetUpdateEventMask() & USE_UPDATE))
        {
            SetUpdateEventMask(USE_UPDATE);
        }
    }
}

void ShakeComponent::SetTraumaPower(float value)
{
    traumaPower_ = value;
}

void ShakeComponent::SetTraumaFalloff(float value)
{
    traumaFalloff_ = value;
}

void ShakeComponent::SetShiftRange(const Vector3& value)
{
    shiftRange_ = value;
}

void ShakeComponent::SetRotationRange(const Vector3& value)
{
    rotationRange_ = value;
}

void ShakeComponent::Update(float timeStep)
{
    if (node_ == nullptr)
    {
        return;
    }

    const auto currentPosition = node_->GetPosition();
    const auto currentRotation = node_->GetRotation();
    if (!hasOriginalPosition_)
    {
        lastKnownPosition_ = originalPosition_ = currentPosition;
        lastKnownRotation_ = originalRotation_ = currentRotation;
        hasOriginalPosition_ = true;
    }
    else
    {
        if (lastKnownPosition_ != currentPosition)
        {
            const auto diff = currentPosition - lastKnownPosition_;
            originalPosition_ = originalPosition_ + diff;
        }
        if (lastKnownRotation_ != currentRotation)
        {
            const auto diff = lastKnownRotation_.Inverse() * currentRotation;
            originalRotation_ = originalRotation_ * diff;
        }
    }

    time_ += timeStep * timeScale_;
    trauma_ = Max(0.0f, trauma_ - traumaFalloff_ * timeStep);
    if (trauma_ == 0.0f)
    {
        node_->SetPosition(originalPosition_);
        node_->SetRotation(originalRotation_);
        SetUpdateEventMask(USE_NO_EVENT);
        hasOriginalPosition_ = false;
        return;
    }

    float scale = Pow(trauma_, traumaPower_);
    Vector3 offset = Vector3::ZERO;
    if (shiftRange_ != Vector3::ZERO)
    {
        offset = (Vector3(perlinNoise_.Get(time_), perlinNoise_.Get(time_ + 1), perlinNoise_.Get(time_ + 2)) * 2.0f
                     - Vector3::ONE)
            * shiftRange_ * scale;
    }
    node_->SetPosition(lastKnownPosition_ = originalPosition_ + offset);

    Vector3 rotation = Vector3::ZERO;
    if (rotationRange_ != Vector3::ZERO)
    {
        rotation =
            (Vector3(perlinNoise_.Get(time_ + 3), perlinNoise_.Get(time_ + 4), perlinNoise_.Get(time_ + 5)) * 2.0f
                - Vector3::ONE)
            * rotationRange_ * scale;
    }
    node_->SetRotation(lastKnownRotation_ = originalRotation_ * Quaternion(rotation));
}
}
