//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022 the rbfx project.
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

#pragma once

#include "Urho3D/Graphics/AnimatedModel.h"

#include <Urho3D/Replica/ReplicatedAnimation.h>

namespace Urho3D
{
class KinematicCharacterController;
class PredictedKinematicController;
class ReplicatedTransform;
}

static constexpr unsigned IMPORTANT_VIEW_MASK = 0x1;
static constexpr unsigned UNIMPORTANT_VIEW_MASK = 0x2;
static constexpr float MAX_ROTATION_SPEED = 360.0f;

/// Custom networking component that handles all sample-specific behaviors:
/// - Animation synchronization;
/// - Player rotation synchronization;
/// - View mask assignment for easy raycasting.
class AdvancedNetworkingPlayer : public Urho3D::ReplicatedAnimation
{
    URHO3D_OBJECT(AdvancedNetworkingPlayer, ReplicatedAnimation);

public:
    static constexpr unsigned RingBufferSize = 8;
    static constexpr Urho3D::NetworkCallbackFlags CallbackMask =
        Urho3D::NetworkCallbackMask::UnreliableDelta | Urho3D::NetworkCallbackMask::Update | Urho3D::NetworkCallbackMask::InterpolateState;

    explicit AdvancedNetworkingPlayer(Urho3D::Context* context);

    /// Initialize component on the server.
    void InitializeOnServer() override;

    /// Initialize component on the client.
    void InitializeFromSnapshot(Urho3D::NetworkFrame frame, Urho3D::Deserializer& src, bool isOwned) override;

    /// Always send animation updates for simplicity.
    bool PrepareUnreliableDelta(Urho3D::NetworkFrame frame) override;

    /// Write current animation state and rotation on server.
    void WriteUnreliableDelta(Urho3D::NetworkFrame frame, Urho3D::Serializer& dest) override;

    /// Read current animation state on replicating client.
    void ReadUnreliableDelta(Urho3D::NetworkFrame frame, Urho3D::Deserializer& src) override;

    /// Perform interpolation of received data on replicated client.
    void InterpolateState(float timeStep, const Urho3D::NetworkTime& replicaTime, const Urho3D::NetworkTime& inputTime);

    void Update(float replicaTimeStep, float inputTimeStep) override;

private:
    void InitializeCommon();

    void UpdateAnimations(float timeStep);

    /// Enum that represents current animation state and corresponding animations.
    enum
    {
        ANIM_IDLE,
        ANIM_WALK,
        ANIM_JUMP,
    };
    ea::vector<Urho3D::Animation*> animations_;

    /// Animation parameters.
    const float moveThreshold_{0.1f};
    const float jumpThreshold_{0.2f};
    const float fadeTime_{0.1f};

    /// Dependencies of this behaviors.
    Urho3D::WeakPtr<Urho3D::ReplicatedTransform> replicatedTransform_;
    Urho3D::WeakPtr<Urho3D::PredictedKinematicController> networkController_;
    Urho3D::WeakPtr<Urho3D::KinematicCharacterController> kinematicController_;

    /// Index of current animation tracked on server for simplicity.
    unsigned currentAnimation_{};

    /// Client-side containers that keep aggregated data from the server for the future sampling.
    float networkFrameDuration_{};
    Urho3D::NetworkValue<ea::pair<unsigned, float>> animationTrace_;
    Urho3D::NetworkValue<Urho3D::Quaternion> rotationTrace_;
    Urho3D::NetworkValueSampler<Urho3D::Quaternion> rotationSampler_;
};
