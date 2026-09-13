// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/StaticModel.h"

namespace Urho3D
{

class ImageCube;

/// Static model component with fixed position in relation to the camera.
class URHO3D_API Skybox : public StaticModel
{
    URHO3D_OBJECT(Skybox, StaticModel);

public:
    /// Construct.
    explicit Skybox(Context* context);
    /// Destruct.
    ~Skybox() override;
    /// Register object factory. StaticModel must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Return skybox image.
    ImageCube* GetImage() const;

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;

    /// Custom world transform per camera.
    ea::unordered_map<Camera*, Matrix3x4> customWorldTransforms_;
    /// Last frame counter for knowing when to erase the custom world transforms of previous frame.
    unsigned lastFrame_;
};

}
