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

#include "../Math/SphericalHarmonics.h"
#include "../Scene/Component.h"

namespace Urho3D
{

/// Light probe description.
struct LightProbe
{
    /// Position in local space of light probe group.
    Vector3 position_;
    /// Incoming light baked into spherical harmonics.
    SphericalHarmonicsDot9 bakedLight_;
};

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

    /// Arrange light probes in scale.x*scale.y*scale.z volume around the node.
    void ArrangeLightProbes();

    /// Set whether the auto placement enabled.
    void SetAutoPlacementEnabled(bool enabled);
    /// Return auto placement step.
    bool GetAutoPlacementEnabled() const { return autoPlacementEnabled_; }
    /// Set auto placement step.
    void SetAutoPlacementStep(float step);
    /// Return auto placement step.
    float GetAutoPlacementStep() const { return autoPlacementStep_; }

    /// Set light probes.
    void SetLightProbes(const ea::vector<LightProbe>& lightProbes) { lightProbes_ = lightProbes; }
    /// Return light probes.
    const ea::vector<LightProbe>& GetLightProbes() const { return lightProbes_; }

    /// Set serialized light probes data.
    void SetLightProbesData(const VariantBuffer& data);
    /// Return serialized light probes data.
    VariantBuffer GetLightProbesData() const;

private:
    /// Light probes.
    ea::vector<LightProbe> lightProbes_;
    /// Whether the auto placement is enabled.
    bool autoPlacementEnabled_{ true };
    /// Automatic placement step.
    float autoPlacementStep_{ 1.0f };
};

}
