// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/LightProbeGroup.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/BinaryArchive.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

void SerializeValue(Archive& archive, const char* name, LightProbe& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    SerializeValue(archive, "position", value.position_);
}

void SerializeValue(Archive& archive, const char* name, LightProbeCollectionBakedData& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    static const unsigned currentVersion = 1;
    const unsigned version = archive.SerializeVersion(currentVersion);
    if (version == currentVersion)
    {
        SerializeVector(archive, "sphericalHarmonics", value.sphericalHarmonics_);

        // Generate ambient if loading
        if (archive.IsInput())
        {
            const unsigned numLightProbes = value.Size();
            value.ambient_.resize(numLightProbes);
            for (unsigned i = 0; i < numLightProbes; ++i)
                value.ambient_[i] = value.sphericalHarmonics_[i].GetDebugColor().ToVector3();
        }
    }
}

LightProbeGroup::LightProbeGroup(Context* context) :
    Component(context)
{
}

LightProbeGroup::~LightProbeGroup() = default;

void LightProbeGroup::RegisterObject(Context* context)
{
    context->AddFactoryReflection<LightProbeGroup>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Light Mask", unsigned, lightMask_, DEFAULT_LIGHTMASK, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Zone Mask", unsigned, zoneMask_, DEFAULT_ZONEMASK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement", GetAutoPlacementEnabled, SetAutoPlacementEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Placement Step", GetAutoPlacementStep, SetAutoPlacementStep, float, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Local Bounding Box Min", Vector3, localBoundingBox_.min_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ATTRIBUTE("Local Bounding Box Max", Vector3, localBoundingBox_.max_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Serialized Light Probes", GetSerializedLightProbes, SetSerializedLightProbes, ea::string, EMPTY_STRING, AM_DEFAULT | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Baked Data File", GetBakedDataFileRef, SetBakedDataFileRef, ResourceRef, ResourceRef{ BinaryFile::GetTypeStatic() }, AM_DEFAULT | AM_NOEDIT);
}

void LightProbeGroup::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    UpdateBakedData();

    const BoundingBox boundingBox{ -Vector3::ONE * 0.5f, Vector3::ONE * 0.5f };
    debug->AddBoundingBox(boundingBox, node_->GetWorldTransform(), Color::GREEN);

    for (unsigned i = 0; i < lightProbes_.size(); ++i)
    {
        const LightProbe& probe = lightProbes_[i];
        const Vector3 worldPosition = node_->LocalToWorld(probe.position_);
        debug->AddSphere(Sphere(worldPosition, 0.1f), static_cast<Color>(bakedData_.ambient_[i]));
    }
}

BoundingBox LightProbeGroup::GetWorldBoundingBox() const
{
    return localBoundingBox_.Transformed(node_->GetWorldTransform());
}

void LightProbeGroup::CollectLightProbes(const ea::vector<LightProbeGroup*>& lightProbeGroups,
    LightProbeCollection& collection, LightProbeCollectionBakedData* bakedData, bool reload)
{
    // Initialize offset according to current state of collection
    unsigned offset = collection.GetNumProbes();

    for (LightProbeGroup* group : lightProbeGroups)
    {
        // Ensure that baked data is up to data
        if (bakedData)
        {
            if (reload)
                group->ReloadBakedData();
            group->UpdateBakedData();
        }

        // Store metadata
        Node* node = group->GetNode();
        const LightProbeVector& probes = group->GetLightProbes();

        collection.offsets_.push_back(offset);
        collection.counts_.push_back(probes.size());
        collection.names_.push_back(node->GetName());
        offset += probes.size();

        // Store light probes data
        for (const LightProbe& probe : probes)
        {
            const Vector3 worldPosition = node->LocalToWorld(probe.position_);
            collection.worldPositions_.push_back(worldPosition);
        }

        // Store baked data
        if (bakedData)
        {
            bakedData->sphericalHarmonics_.append(group->bakedData_.sphericalHarmonics_);
            bakedData->ambient_.append(group->bakedData_.ambient_);
        }
    }
}

void LightProbeGroup::CollectLightProbes(Scene* scene,
    LightProbeCollection& collection, LightProbeCollectionBakedData* bakedData, bool reload)
{
    ea::vector<LightProbeGroup*> lightProbeGroups;
    scene->FindComponents(lightProbeGroups);

    const auto isNotEnabled = [](const LightProbeGroup* lightProbeGroup) { return !lightProbeGroup->IsEnabledEffective(); };
    ea::erase_if(lightProbeGroups, isNotEnabled);

    CollectLightProbes(lightProbeGroups, collection, bakedData, reload);
}

bool LightProbeGroup::SaveLightProbesBakedData(Context* context, const FileIdentifier& fileName,
    const LightProbeCollection& collection, const LightProbeCollectionBakedData& bakedData, unsigned index)
{
    if (index >= collection.GetNumGroups())
        return false;

    const unsigned offset = collection.offsets_[index];
    const unsigned count = collection.counts_[index];
    if (offset + count > bakedData.Size())
        return false;

    LightProbeCollectionBakedData copy;

    auto sphericalHarmonicsBegin = bakedData.sphericalHarmonics_.begin() + offset;
    auto ambientBegin = bakedData.ambient_.begin() + offset;
    copy.sphericalHarmonics_.assign(sphericalHarmonicsBegin, sphericalHarmonicsBegin + count);
    copy.ambient_.assign(ambientBegin, ambientBegin + count);

    BinaryFile bakedDataFile(context);

    if (!bakedDataFile.SaveObject("LightProbesBakedData", copy))
        return false;

    if (!bakedDataFile.SaveFile(fileName))
        return false;

    return true;
}

void LightProbeGroup::ArrangeLightProbesInVolume()
{
    bakedDataDirty_ = true; // Reset baked data every time light probes change
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
    const Vector3 gridStep = Vector3::ONE / (gridSize - IntVector3::ONE).ToVector3();
    IntVector3 index;
    for (index.z_ = 0; index.z_ < gridSize.z_; ++index.z_)
    {
        for (index.y_ = 0; index.y_ < gridSize.y_; ++index.y_)
        {
            for (index.x_ = 0; index.x_ < gridSize.x_; ++index.x_)
            {
                const Vector3 localPosition = -Vector3::ONE / 2 + index.ToVector3() * gridStep;
                lightProbes_.push_back(LightProbe{ localPosition });
            }
        }
    }

    localBoundingBox_ = { -Vector3::ONE / 2, Vector3::ONE / 2 };
}

void LightProbeGroup::ReloadBakedData()
{
    bakedDataDirty_ = true;
    UpdateBakedData();
}

void LightProbeGroup::SetAutoPlacementEnabled(bool enabled)
{
    autoPlacementEnabled_ = enabled;
    if (autoPlacementEnabled_)
        ArrangeLightProbesInVolume();
}

void LightProbeGroup::SetAutoPlacementStep(float step)
{
    autoPlacementStep_ = step;
    if (autoPlacementEnabled_)
        ArrangeLightProbesInVolume();
}

void LightProbeGroup::SetLightProbes(const LightProbeVector& lightProbes)
{
    lightProbes_ = lightProbes;
    bakedDataDirty_ = true; // Reset baked data every time light probes change
    UpdateLocalBoundingBox();
}

void LightProbeGroup::SerializeLightProbes(Archive& archive)
{
    ArchiveBlock block = archive.OpenUnorderedBlock("LightProbesData");
    static const unsigned currentVersion = 2;
    const unsigned version = archive.SerializeVersion(currentVersion);
    if (version == currentVersion)
    {
        SerializeVector(archive, "lightProbes", lightProbes_);
    }
}

void LightProbeGroup::SetSerializedLightProbes(const ea::string& data)
{
    VectorBuffer buffer(DecodeBase64(data));
    BinaryInputArchive archive(context_, buffer);
    SerializeLightProbes(archive);
    bakedDataDirty_ = true; // Reset baked data every time light probes change
}

ea::string LightProbeGroup::GetSerializedLightProbes() const
{
    VectorBuffer buffer;
    BinaryOutputArchive archive(context_, buffer);
    const_cast<LightProbeGroup*>(this)->SerializeLightProbes(archive);
    return EncodeBase64(buffer.GetBuffer());
}

void LightProbeGroup::SetBakedDataFileRef(const ResourceRef& fileRef)
{
    if (bakedDataRef_ != fileRef)
    {
        bakedDataDirty_ = true;
        bakedDataRef_ = fileRef;
    }
}

ResourceRef LightProbeGroup::GetBakedDataFileRef() const
{
    return bakedDataRef_;
}

void LightProbeGroup::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
        node_->AddListener(this);
}

void LightProbeGroup::OnMarkedDirty(Node* node)
{
    if (autoPlacementEnabled_ && lastNodeScale_ != node->GetScale())
    {
        lastNodeScale_ = node->GetScale();
        ArrangeLightProbesInVolume();
    }
}

void LightProbeGroup::UpdateLocalBoundingBox()
{
    localBoundingBox_.Clear();
    for (const LightProbe& probe : lightProbes_)
        localBoundingBox_.Merge(probe.position_);
}

void LightProbeGroup::UpdateBakedData()
{
    if (!bakedDataDirty_)
        return;

    bakedDataDirty_ = false;
    auto cache = context_->GetSubsystem<ResourceCache>();
    auto bakedDataFile = cache->GetTempResource<BinaryFile>(bakedDataRef_.name_);

    // Try to load from file
    bool success = false;

    if (bakedDataFile)
    {
        if (bakedDataFile->LoadObject("LightProbesBakedData", bakedData_))
        {
            if (bakedData_.sphericalHarmonics_.size() == lightProbes_.size())
                success = true;
        }
    }

    // Reset to default if failed
    if (!success)
    {
        bakedData_.Resize(lightProbes_.size());
        for (unsigned i = 0; i < lightProbes_.size(); ++i)
        {
            bakedData_.sphericalHarmonics_[i] = SphericalHarmonicsDot9::ZERO;
            bakedData_.ambient_[i] = Vector3::ZERO;
        }
    }
}

}
