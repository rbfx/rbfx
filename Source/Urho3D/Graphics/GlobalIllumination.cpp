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

#include "../Graphics/GlobalIllumination.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/BinaryArchive.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

extern const char* SUBSYSTEM_CATEGORY;

GlobalIllumination::GlobalIllumination(Context* context) :
    Component(context)
{
}

GlobalIllumination::~GlobalIllumination() = default;

void GlobalIllumination::RegisterObject(Context* context)
{
    context->RegisterFactory<GlobalIllumination>(SUBSYSTEM_CATEGORY);

    URHO3D_ATTRIBUTE("Is Background Static", bool, backgroundStatic_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Background Brightness", float, backgroundBrightness_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Light Probes Data", GetLightProbesData, SetLightProbesData, ea::string, EMPTY_STRING, AM_DEFAULT | AM_NOEDIT);
}

void GlobalIllumination::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    static thread_local ea::vector<ea::pair<unsigned, unsigned>> edges;
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
}

SphericalHarmonicsDot9 GlobalIllumination::SampleAmbientSH(const Vector3& position, unsigned& hint) const
{
    return lightProbesMesh_.Sample(lightProbesBakedData_.sphericalHarmonics_, position, hint);
}

Vector3 GlobalIllumination::SampleAverageAmbient(const Vector3& position, unsigned& hint) const
{
    return lightProbesMesh_.Sample(lightProbesBakedData_.ambient_, position, hint);
}

void GlobalIllumination::SerializeLightProbesData(Archive& archive)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock("LightProbes"))
    {
        static const unsigned currentVersion = 2;
        const unsigned version = archive.SerializeVersion(currentVersion);
        if (version == currentVersion)
        {
            SerializeValue(archive, "Mesh", lightProbesMesh_);
            SerializeValue(archive, "Data", lightProbesBakedData_);
        }
    }
}

void GlobalIllumination::SetLightProbesData(const ea::string& data)
{
    VectorBuffer buffer(DecodeBase64(data));
    BinaryInputArchive archive(context_, buffer);
    SerializeLightProbesData(archive);
}

ea::string GlobalIllumination::GetLightProbesData() const
{
    VectorBuffer buffer;
    BinaryOutputArchive archive(context_, buffer);
    const_cast<GlobalIllumination*>(this)->SerializeLightProbesData(archive);
    return EncodeBase64(buffer.GetBuffer());
}

}
