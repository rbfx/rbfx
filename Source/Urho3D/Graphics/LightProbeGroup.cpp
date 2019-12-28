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

#include "../Precompiled.h"

#include "../Graphics/LightProbeGroup.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/BinaryArchive.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"

namespace Urho3D
{

extern const char* SUBSYSTEM_CATEGORY;

LightProbeGroup::LightProbeGroup(Context* context) :
    Component(context)
{
}

LightProbeGroup::~LightProbeGroup() = default;

void LightProbeGroup::RegisterObject(Context* context)
{
    context->RegisterFactory<LightProbeGroup>(SUBSYSTEM_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement", GetAutoPlacementEnabled, SetAutoPlacementEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement Step", GetAutoPlacementStep, SetAutoPlacementStep, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Data", GetLightProbesData, SetLightProbesData, VariantBuffer, Variant::emptyBuffer, AM_DEFAULT | AM_NOEDIT);
}

void LightProbeGroup::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const BoundingBox boundingBox{ -Vector3::ONE * 0.5f, Vector3::ONE * 0.5f };
    debug->AddBoundingBox(boundingBox, node_->GetWorldTransform(), Color::GREEN);

    for (const LightProbe& probe : lightProbes_)
    {
        const Vector3 worldPosition = node_->LocalToWorld(probe.position_);
        const Color color = static_cast<Color>(probe.bakedLight_.EvaluateAverage());
        debug->AddSphere(Sphere(worldPosition, 0.1f), color);
    }
}

void LightProbeGroup::ArrangeLightProbes()
{
    lightProbes_.clear();
    if (autoPlacementStep_ <= M_LARGE_EPSILON)
        return;

    const Vector3 volumeSize = VectorAbs(node_->GetScale());
    const IntVector3 gridSize = VectorMax(IntVector3::ONE * 2,
        IntVector3::ONE + VectorRoundToInt(volumeSize / autoPlacementStep_));
    const int maxGridSize = ea::max({ gridSize.x_, gridSize.y_, gridSize.z_ });

    // Integer multiplication overflow is undefined behaviour and should be treated carefully.
    if (maxGridSize >= static_cast<int>(MaxAutoGridSize)
        || gridSize.x_ * gridSize.y_ * gridSize.z_ >= static_cast<int>(MaxAutoProbes))
    {
        URHO3D_LOGERROR("Automatic Light Probe Grid is too big");
        return;
    }

    // Fill volume with probes
    const Vector3 gridStep = Vector3::ONE / static_cast<Vector3>(gridSize - IntVector3::ONE);
    IntVector3 index;
    for (index.z_ = 0; index.z_ < gridSize.z_; ++index.z_)
    {
        for (index.y_ = 0; index.y_ < gridSize.y_; ++index.y_)
        {
            for (index.x_ = 0; index.x_ < gridSize.x_; ++index.x_)
            {
                const Vector3 localPosition = -Vector3::ONE / 2 + static_cast<Vector3>(index) * gridStep;
                // TODO(glow): Initialize with zeros
                const Color color{ Random(1.0f), Random(1.0f), Random(1.0f) };
                const SphericalHarmonicsDot9 sh{ SphericalHarmonicsColor9{ color } };
                lightProbes_.push_back(LightProbe{ localPosition, sh });
            }
        }
    }
}

void LightProbeGroup::SetAutoPlacementEnabled(bool enabled)
{
    autoPlacementEnabled_ = enabled;
    if (autoPlacementEnabled_)
        ArrangeLightProbes();
}

void LightProbeGroup::SetAutoPlacementStep(float step)
{
    autoPlacementStep_ = step;
    if (autoPlacementEnabled_)
        ArrangeLightProbes();
}

void LightProbeGroup::SetLightProbesData(const VariantBuffer& data)
{
    VectorBuffer buffer(data);
    BinaryInputArchive archive(context_, buffer);
    SerializeVectorAsBytes(archive, "LightProbes", "LightProbe", lightProbes_);
}

VariantBuffer LightProbeGroup::GetLightProbesData() const
{
    VectorBuffer buffer;
    BinaryOutputArchive archive(context_, buffer);
    SerializeVectorAsBytes(archive, "LightProbes", "LightProbe", const_cast<ea::vector<LightProbe>&>(lightProbes_));
    return buffer.GetBuffer();
}

void LightProbeGroup::OnNodeSet(Node* node)
{
    if (node)
        node->AddListener(this);
}

void LightProbeGroup::OnMarkedDirty(Node* node)
{
    if (autoPlacementEnabled_ && lastNodeScale_ != node->GetScale())
    {
        lastNodeScale_ = node->GetScale();
        ArrangeLightProbes();
    }
}

}
