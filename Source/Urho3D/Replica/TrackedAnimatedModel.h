// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/Transform.h"
#include "Urho3D/Replica/BehaviorNetworkObject.h"
#include "Urho3D/Replica/NetworkValue.h"

namespace Urho3D
{

class AnimatedModel;
class RayOctreeQuery;
struct RayQueryResult;

/// Behavior that tracks bone transforms of AnimatedModel on server. Not implemented on client.
class URHO3D_API TrackedAnimatedModel : public NetworkBehavior
{
    URHO3D_OBJECT(TrackedAnimatedModel, NetworkBehavior);

public:
    explicit TrackedAnimatedModel(Context* context);
    ~TrackedAnimatedModel() override;

    static void RegisterObject(Context* context);

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;
    /// @}

    /// Getters for network properties
    /// @{
    DoubleVector3 SampleTemporalBonePosition(const NetworkTime& time, unsigned index) const;
    Quaternion SampleTemporalBoneRotation(const NetworkTime& time, unsigned index) const;
    void ProcessTemporalRayQuery(const NetworkTime& time, const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) const;
    /// @}

private:
    void OnServerFrameEnd(NetworkFrame frame);

    /// Attributes independent on the client and the server.
    /// @{
    bool trackOnClient_{};
    /// @}

    WeakPtr<AnimatedModel> animatedModel_;

    NetworkValue<DoubleTransform> nodeTransformTrace_;
    NetworkValue<BoundingBox> boundingBoxTrace_;
    NetworkValueVector<DoubleTransform> boneTransformsTrace_;
};

};
