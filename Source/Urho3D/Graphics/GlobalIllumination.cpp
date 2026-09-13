// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/GlobalIllumination.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/BinaryArchive.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

namespace Urho3D
{

GlobalIllumination::GlobalIllumination(Context* context) :
    Component(context)
{
}

GlobalIllumination::~GlobalIllumination() = default;

void GlobalIllumination::OnSceneSet(Scene* previousScene, Scene* scene)
{
    BaseClassName::OnSceneSet(previousScene, scene);

    if (previousScene)
        UnsubscribeFromEvent(E_WORLDORIGINPOSTUPDATE);

    if (scene)
        SubscribeToEvent(scene, E_WORLDORIGINPOSTUPDATE, &GlobalIllumination::HandleWorldOriginPostUpdate);
}

void GlobalIllumination::HandleWorldOriginPostUpdate(VariantMap& eventData)
{
    using namespace WorldOriginPostUpdate;
    const Vector3 delta = eventData[P_DELTA].GetIntVector3().ToVector3();

    offset_ -= delta;
    for (Vector3& position : lightProbesMesh_.vertices_)
        position -= delta;
}

void GlobalIllumination::RegisterObject(Context* context)
{
    context->AddFactoryReflection<GlobalIllumination>(Category_Subsystem);

    URHO3D_ACTION_STATIC_LABEL("Reload!", ReloadData, "Reload data from the file");
    URHO3D_ATTRIBUTE("Emission Brightness", float, emissionBrightness_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Offset", Vector3, offset_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Data File", GetFileRef, SetFileRef, ResourceRef, ResourceRef{ BinaryFile::GetTypeStatic() }, AM_DEFAULT | AM_NOEDIT);
}

void GlobalIllumination::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    thread_local ea::vector<ea::pair<unsigned, unsigned>> edges;
    lightProbesMesh_.CollectEdges(edges);

    for (const auto& edge : edges)
    {
        const Vector3& startPos = lightProbesMesh_.vertices_[edge.first];
        const Vector3& endPos = lightProbesMesh_.vertices_[edge.second];
        debug->AddLine(startPos, endPos, Color::YELLOW);
    }

    for (unsigned ignoredVertex : lightProbesMesh_.ignoredVertices_)
    {
        const Vector3& position = lightProbesMesh_.vertices_[ignoredVertex];
        const BoundingBox boundingBox{ position - Vector3::ONE * 0.1f, position + Vector3::ONE * 0.1f };
        debug->AddBoundingBox(boundingBox, Color::RED);
    }

    for (const auto& highlightEdge : lightProbesMesh_.debugHighlightEdges_)
    {
        const Vector3& startPos = lightProbesMesh_.vertices_[highlightEdge.first];
        const Vector3& endPos = lightProbesMesh_.vertices_[highlightEdge.second];
        debug->AddLine(startPos, endPos, Color::RED);
    }
}

void GlobalIllumination::ResetLightProbes()
{
    lightProbesBakedData_.Clear();
    lightProbesMesh_ = {};
}

void GlobalIllumination::CompileLightProbes()
{
    ResetLightProbes();

    // Collect light probes
    LightProbeCollection collection;
    LightProbeGroup::CollectLightProbes(GetScene(), collection, &lightProbesBakedData_, true /*reload*/);
    if (collection.Empty())
        return;

    // Add padding to avoid vertex collision
    lightProbesMesh_.Define(collection.worldPositions_);

    // Store in file
    auto cache = context_->GetSubsystem<ResourceCache>();
    auto file = cache->GetTempResource<BinaryFile>(fileRef_.name_);
    if (!file)
    {
        URHO3D_LOGERROR("Cannot save Global Illumination data: output file doesn't exist");
        return;
    }

    file->Clear();

    if (!file->SaveObjectCallback([&](Archive& archive) { SerializeData(archive); }))
    {
        URHO3D_LOGERROR("Cannot save Global Illumination data: serialization failed");
        return;
    }

    if (!file->SaveFile(file->GetAbsoluteFileName()))
    {
        URHO3D_LOGERROR("Cannot save Global Illumination data: cannot save file");
        return;
    }
}

SphericalHarmonicsDot9 GlobalIllumination::SampleAmbientSH(const Vector3& position, unsigned& hint) const
{
    return lightProbesMesh_.Sample(lightProbesBakedData_.sphericalHarmonics_, position, hint);
}

Vector3 GlobalIllumination::SampleAverageAmbient(const Vector3& position, unsigned& hint) const
{
    return lightProbesMesh_.Sample(lightProbesBakedData_.ambient_, position, hint);
}

void GlobalIllumination::SetFileRef(const ResourceRef& fileRef)
{
    if (fileRef_ != fileRef)
    {
        fileRef_ = fileRef;
        ReloadData();
    }
}

ResourceRef GlobalIllumination::GetFileRef() const
{
    return fileRef_;
}

void GlobalIllumination::SerializeData(Archive& archive)
{
    ArchiveBlock block = archive.OpenUnorderedBlock("lightProbes");
    static const unsigned currentVersion = 2;
    const unsigned version = archive.SerializeVersion(currentVersion);
    if (version == currentVersion)
    {
        SerializeValue(archive, "mesh", lightProbesMesh_);
        SerializeValue(archive, "data", lightProbesBakedData_);
    }
}

void GlobalIllumination::ReloadData()
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    auto file = cache->GetTempResource<BinaryFile>(fileRef_.name_);

    if (!file || !file->LoadObjectCallback([&](Archive& archive) { SerializeData(archive); }))
    {
        lightProbesMesh_ = {};
        lightProbesBakedData_.Clear();
    }
    else
    {
        for (Vector3& position : lightProbesMesh_.vertices_)
            position += offset_;
    }
}

}
