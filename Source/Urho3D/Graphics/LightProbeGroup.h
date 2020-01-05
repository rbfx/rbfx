//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Math/BoundingBox.h"
#include "../Math/SphericalHarmonics.h"
#include "../Scene/Component.h"

namespace Urho3D
{

class LightProbeGroup;

/// Light probe description.
struct LightProbe
{
    /// Position in local space of light probe group.
    Vector3 position_;
    /// Incoming light baked into spherical harmonics.
    SphericalHarmonicsDot9 sphericalHarmonics_;
};

/// Serialize light probe.
bool SerializeValue(Archive& archive, const char* name, LightProbe& value);

/// Vector of light probes.
using LightProbeVector = ea::vector<LightProbe>;

/// Light probes from multiple light probe groups.
struct LightProbeCollection
{
    /// Baked light as spherical harmonics.
    ea::vector<SphericalHarmonicsDot9> bakedSphericalHarmonics_;
    /// Baked light as color in gamma space.
    ea::vector<Color> bakedAmbient_;
    /// World-space positions of light probes.
    ea::vector<Vector3> worldPositions_;

    /// First light probe owned by corresponding group.
    ea::vector<unsigned> offsets_;
    /// Number of light probes owned by corresponding group.
    ea::vector<unsigned> counts_;

    /// Return whether the collection is empty.
    bool Empty() const { return bakedSphericalHarmonics_.empty(); }

    /// Return total size.
    unsigned Size() const { return bakedSphericalHarmonics_.size(); }

    /// Calculate padded bounding box.
    BoundingBox CalculateBoundingBox(const Vector3& padding = Vector3::ZERO)
    {
        BoundingBox boundingBox(worldPositions_.data(), worldPositions_.size());
        boundingBox.min_ -= padding;
        boundingBox.max_ += padding;
        return boundingBox;
    }

    /// Reset baked data in all probes.
    void ResetBakedData()
    {
        for (unsigned i = 0; i < Size(); ++i)
        {
            bakedSphericalHarmonics_[i] = SphericalHarmonicsDot9{};
            bakedAmbient_[i] = Color::BLACK;
        }
    }

    /// Clear collection.
    void Clear()
    {
        bakedSphericalHarmonics_.clear();
        bakedAmbient_.clear();
        worldPositions_.clear();
        offsets_.clear();
        counts_.clear();
    }
};

/// Serialize light probe collection.
bool SerializeValue(Archive& archive, const char* name, LightProbeCollection& value);

/// Light probe group.
class URHO3D_API LightProbeGroup : public Component
{
    URHO3D_OBJECT(LightProbeGroup, Component);

public:
    /// Auto placement limit: max grid size in one dimension.
    static const unsigned MaxAutoGridSize = 1024;
    /// Auto placement limit: max total number of probes generated.
    static const unsigned MaxAutoProbes = 65536;

    /// Construct.
    explicit LightProbeGroup(Context* context);
    /// Destruct.
    ~LightProbeGroup() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Collect all light probes from specified groups.
    static void CollectLightProbes(const ea::vector<LightProbeGroup*>& lightProbeGroups, LightProbeCollection& collection);
    /// Collect all light probes from all enabled groups in the scene.
    static void CollectLightProbes(Scene* scene, LightProbeCollection& collection);

    /// Return bounding box in local space.
    BoundingBox GetLocalBoundingBox() const { return localBoundingBox_; }
    /// Return bounding box in world space.
    BoundingBox GetWorldBoundingBox() const;

    /// Arrange light probes in scale.x*scale.y*scale.z volume around the node.
    void ArrangeLightProbes();
    /// Update light probes data from specified part of collection.
    bool CommitLightProbes(const LightProbeCollection& collection, unsigned index);

    /// Set whether the auto placement enabled.
    void SetAutoPlacementEnabled(bool enabled);
    /// Return auto placement step.
    bool GetAutoPlacementEnabled() const { return autoPlacementEnabled_; }
    /// Set auto placement step.
    void SetAutoPlacementStep(float step);
    /// Return auto placement step.
    float GetAutoPlacementStep() const { return autoPlacementStep_; }

    /// Set light probes.
    void SetLightProbes(const LightProbeVector& lightProbes);
    /// Return light probes.
    const LightProbeVector& GetLightProbes() const { return lightProbes_; }

    /// Serialize light probes data.
    void SerializeLightProbesData(Archive& archive);
    /// Set serialized light probes data.
    void SetLightProbesData(const VariantBuffer& data);
    /// Return serialized light probes data.
    VariantBuffer GetLightProbesData() const;

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* node) override;
    /// Handle scene node transform dirtied.
    void OnMarkedDirty(Node* node) override;
    /// Update local bounding box.
    void UpdateLocalBoundingBox();

    /// Light probes.
    LightProbeVector lightProbes_;
    /// Bounding box in local space.
    BoundingBox localBoundingBox_;

    /// Whether the auto placement is enabled.
    bool autoPlacementEnabled_{ true };
    /// Automatic placement step.
    float autoPlacementStep_{ 1.0f };
    /// Last node scale used during auto placement.
    Vector3 lastNodeScale_;
};

}
