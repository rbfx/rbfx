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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Replica/ReplicatedAnimation.h>

namespace Urho3D
{

class KinematicCharacterController;
class PredictedKinematicController;
class ReplicatedTransform;

}

using namespace Urho3D;

static constexpr unsigned IMPORTANT_VIEW_MASK = 0x1;
static constexpr unsigned UNIMPORTANT_VIEW_MASK = 0x2;
static constexpr float MAX_ROTATION_SPEED = 360.0f;

/// Custom networking component that handles all sample-specific behaviors:
/// - Animation synchronization;
/// - Player rotation synchronization;
/// - View mask assignment for easy raycasting.
class AdvancedNetworkingPlayer : public NetworkBehavior
{
    URHO3D_OBJECT(AdvancedNetworkingPlayer, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::Update;

    explicit AdvancedNetworkingPlayer(Context* context);

    /// Initialize component on the server.
    void InitializeOnServer() override;

    /// Initialize component on the client.
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

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
    ea::vector<Animation*> animations_;

    /// Animation parameters.
    const float moveThreshold_{0.1f};
    const float jumpThreshold_{5.0f};
    const float fadeTime_{0.1f};

    /// Dependencies of this behaviors.
    WeakPtr<AnimationController> animationController_;
    WeakPtr<ReplicatedTransform> replicatedTransform_;
    WeakPtr<PredictedKinematicController> networkController_;
    WeakPtr<KinematicCharacterController> kinematicController_;

    /// Index of current animation tracked on server for simplicity.
    unsigned currentAnimation_{};

    /// Client-side containers that keep aggregated data from the server for the future sampling.
    //float networkFrameDuration_{};
    //NetworkValue<ea::pair<unsigned, float>> animationTrace_;
    //NetworkValue<Quaternion> rotationTrace_;
    //NetworkValueSampler<Quaternion> rotationSampler_;
};
