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

    URHO3D_ACCESSOR_ATTRIBUTE("Light Probes Data", GetLightProbesData, SetLightProbesData, VariantBuffer, Variant::emptyBuffer, AM_DEFAULT | AM_NOEDIT);
}

void GlobalIllumination::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (unsigned tetIndex = 0; tetIndex < lightProbesMesh_.tetrahedrons_.size(); ++tetIndex)
    {
        const Tetrahedron& tetrahedron = lightProbesMesh_.tetrahedrons_[tetIndex];
        if (tetIndex < lightProbesMesh_.numInnerTetrahedrons_)
        {
            for (unsigned i = 0; i < 4; ++i)
            {
                for (unsigned j = 0; j < 4; ++j)
                {
                    const unsigned startIndex = tetrahedron.indices_[i];
                    const unsigned endIndex = tetrahedron.indices_[j];
                    const Vector3& startPos = lightProbesMesh_.vertices_[startIndex];
                    const Vector3& endPos = lightProbesMesh_.vertices_[endIndex];
                    const Vector3 midPos = startPos.Lerp(endPos, 0.5f);
                    const Color startColor = lightProbesCollection_.bakedAmbient_[startIndex];
                    const Color endColor = lightProbesCollection_.bakedAmbient_[endIndex];
                    debug->AddLine(startPos, midPos, startColor);
                    debug->AddLine(midPos, endPos, endColor);
                }
            }
        }
        else
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                const unsigned index = tetrahedron.indices_[i];
                const Vector3& pos = lightProbesMesh_.vertices_[index];
                const Vector3& normal = lightProbesMesh_.hullNormals_[index];
                const Color color = lightProbesCollection_.bakedAmbient_[index];
                debug->AddLine(pos, pos + normal, color);
            }
        }
    }
}

void GlobalIllumination::ResetLightProbes()
{
    lightProbesCollection_.Clear();
    lightProbesMesh_ = {};
}

void GlobalIllumination::CompileLightProbes()
{
    ResetLightProbes();

    // Collect light probes
    LightProbeGroup::CollectLightProbes(GetScene(), lightProbesCollection_);
    if (lightProbesCollection_.Empty())
        return;

    // Add padding to avoid vertex collision
    lightProbesMesh_.Define(lightProbesCollection_.worldPositions_);
}

SphericalHarmonicsDot9 GlobalIllumination::SampleAmbientSH(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    return lightProbesMesh_.Sample(lightProbesCollection_.bakedSphericalHarmonics_, position, hint);
}

Color GlobalIllumination::SampleAverageAmbient(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    return lightProbesMesh_.Sample(lightProbesCollection_.bakedAmbient_, position, hint);
}

void GlobalIllumination::SerializeLightProbesData(Archive& archive)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock("LightProbes"))
    {
        SerializeValue(archive, "Data", lightProbesCollection_);
        SerializeValue(archive, "Mesh", lightProbesMesh_);
    }
}

void GlobalIllumination::SetLightProbesData(const VariantBuffer& data)
{
    VectorBuffer buffer(data);
    BinaryInputArchive archive(context_, buffer);
    SerializeLightProbesData(archive);
}

VariantBuffer GlobalIllumination::GetLightProbesData() const
{
    VectorBuffer buffer;
    BinaryOutputArchive archive(context_, buffer);
    const_cast<GlobalIllumination*>(this)->SerializeLightProbesData(archive);
    return buffer.GetBuffer();
}

}
