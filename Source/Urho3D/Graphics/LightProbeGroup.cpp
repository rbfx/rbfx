//
// Copyright (c) 2019-2020 the rbfx project.
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
#include "../IO/ArchiveSerialization.h"
#include "../IO/BinaryArchive.h"
#include "../IO/Log.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

extern const char* SCENE_CATEGORY;

bool SerializeValue(Archive& archive, const char* name, LightProbe& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "Position", value.position_);
        SerializeValue(archive, "SH9", value.sphericalHarmonics_);
        return true;
    }
    return false;
}

bool SerializeValue(Archive& archive, const char* name, LightProbeCollection& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeVector(archive, "BakedSH", "SH9", value.bakedSphericalHarmonics_);
        SerializeVector(archive, "BakedAmbient", "Color", value.bakedAmbient_);
        SerializeVector(archive, "WorldPositions", "Position", value.worldPositions_);
        SerializeVector(archive, "SubsetOffsets", "Offset", value.offsets_);
        SerializeVector(archive, "SubsetCounts", "Count", value.counts_);
        return true;
    }
    return false;
}

LightProbeGroup::LightProbeGroup(Context* context) :
    Component(context)
{
}

LightProbeGroup::~LightProbeGroup() = default;

void LightProbeGroup::RegisterObject(Context* context)
{
    context->RegisterFactory<LightProbeGroup>(SCENE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement", GetAutoPlacementEnabled, SetAutoPlacementEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement Step", GetAutoPlacementStep, SetAutoPlacementStep, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Light Probes Data", GetLightProbesData, SetLightProbesData, ea::string, EMPTY_STRING, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ATTRIBUTE("Local Bounding Box Min", Vector3, localBoundingBox_.min_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ATTRIBUTE("Local Bounding Box Max", Vector3, localBoundingBox_.max_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
}

void LightProbeGroup::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const BoundingBox boundingBox{ -Vector3::ONE * 0.5f, Vector3::ONE * 0.5f };
    debug->AddBoundingBox(boundingBox, node_->GetWorldTransform(), Color::GREEN);

    for (const LightProbe& probe : lightProbes_)
    {
        const Vector3 worldPosition = node_->LocalToWorld(probe.position_);
        debug->AddSphere(Sphere(worldPosition, 0.1f), probe.sphericalHarmonics_.GetDebugColor());
    }
}

BoundingBox LightProbeGroup::GetWorldBoundingBox() const
{
    return localBoundingBox_.Transformed(node_->GetWorldTransform());
}

void LightProbeGroup::CollectLightProbes(const ea::vector<LightProbeGroup*>& lightProbeGroups, LightProbeCollection& collection)
{
    // Initialize offset according to current state of collection
    unsigned offset = collection.Size();

    for (LightProbeGroup* group : lightProbeGroups)
    {
        Node* node = group->GetNode();
        const LightProbeVector& probes = group->GetLightProbes();

        collection.offsets_.push_back(offset);
        collection.counts_.push_back(probes.size());
        offset += probes.size();

        for (const LightProbe& probe : probes)
        {
            const Vector3 worldPosition = node->LocalToWorld(probe.position_);
            collection.bakedSphericalHarmonics_.push_back(probe.sphericalHarmonics_);
            collection.bakedAmbient_.push_back(probe.sphericalHarmonics_.GetDebugColor());
            collection.worldPositions_.push_back(worldPosition);
        }
    }
}

void LightProbeGroup::CollectLightProbes(Scene* scene, LightProbeCollection& collection)
{
    ea::vector<LightProbeGroup*> lightProbeGroups;
    scene->GetComponents(lightProbeGroups, true);

    const auto isNotEnabled = [](const LightProbeGroup* lightProbeGroup) { return !lightProbeGroup->IsEnabledEffective(); };
    lightProbeGroups.erase(ea::remove_if(lightProbeGroups.begin(), lightProbeGroups.end(), isNotEnabled), lightProbeGroups.end());

    CollectLightProbes(lightProbeGroups, collection);
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
                lightProbes_.push_back(LightProbe{ localPosition, SphericalHarmonicsDot9{} });
            }
        }
    }

    localBoundingBox_ = { -Vector3::ONE / 2, Vector3::ONE / 2 };
}

bool LightProbeGroup::CommitLightProbes(const LightProbeCollection& collection, unsigned index)
{
    if (index >= collection.Size())
    {
        URHO3D_LOGERROR("Cannot commit light probes: index is out of range");
        return false;
    }

    if (collection.counts_[index] != lightProbes_.size())
    {
        URHO3D_LOGERROR("Cannot commit light probes: number of light probes doesn't match");
        return false;
    }

    const unsigned offset = collection.offsets_[index];
    for (unsigned probeIndex = 0; probeIndex < lightProbes_.size(); ++probeIndex)
    {
        lightProbes_[probeIndex].sphericalHarmonics_ = collection.bakedSphericalHarmonics_[offset + probeIndex];
    }

    return true;
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

void LightProbeGroup::SetLightProbes(const LightProbeVector& lightProbes)
{
    lightProbes_ = lightProbes;
    UpdateLocalBoundingBox();
}

void LightProbeGroup::SerializeLightProbesData(Archive& archive)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock("LightProbesData"))
    {
        static const unsigned currentVersion = 1;
        const unsigned version = archive.SerializeVersion(currentVersion);
        if (version == currentVersion)
        {
            SerializeVector(archive, "LightProbes", "LightProbe", lightProbes_);
        }
    }
}

void LightProbeGroup::SetLightProbesData(const ea::string& data)
{
    VectorBuffer buffer(DecodeBase64(data));
    BinaryInputArchive archive(context_, buffer);
    SerializeLightProbesData(archive);
}

ea::string LightProbeGroup::GetLightProbesData() const
{
    VectorBuffer buffer;
    BinaryOutputArchive archive(context_, buffer);
    const_cast<LightProbeGroup*>(this)->SerializeLightProbesData(archive);
    return EncodeBase64(buffer.GetBuffer());
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

void LightProbeGroup::UpdateLocalBoundingBox()
{
    localBoundingBox_.Clear();
    for (const LightProbe& probe : lightProbes_)
        localBoundingBox_.Merge(probe.position_);
}

}
