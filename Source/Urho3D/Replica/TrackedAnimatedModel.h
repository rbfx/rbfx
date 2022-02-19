//
// Copyright (c) 2017-2020 the rbfx project.
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

/// \file

#pragma once

#include "../Replica/BehaviorNetworkObject.h"
#include "../Replica/NetworkValue.h"

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
    Vector3 SampleTemporalBonePosition(const NetworkTime& time, unsigned index) const;
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

    NetworkValue<Matrix3x4> transformTrace_;
    NetworkValue<BoundingBox> boundingBoxTrace_;

    NetworkValueVector<Vector3> bonePositionsTrace_;
    NetworkValueVector<Quaternion> boneRotationsTrace_;
};

};
