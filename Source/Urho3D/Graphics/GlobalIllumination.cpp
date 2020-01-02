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
}

void GlobalIllumination::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (const Tetrahedron& tetrahedrons : lightProbesMesh_.tetrahedrons_)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned startIndex = tetrahedrons.indices_[i];
                const unsigned endIndex = tetrahedrons.indices_[j];
                const Vector3& startPos = lightProbesMesh_.vertices_[startIndex];
                const Vector3& endPos = lightProbesMesh_.vertices_[endIndex];
                const Vector3 midPos = startPos.Lerp(endPos, 0.5f);
                const Color startColor = lightProbesCollection_.lightProbes_[startIndex].GetDebugColor();
                const Color endColor = lightProbesCollection_.lightProbes_[startIndex].GetDebugColor();
                debug->AddLine(startPos, midPos, startColor);
                debug->AddLine(midPos, endPos, endColor);
            }
        }
    }

    for (unsigned i = 0; i < lightProbesMesh_.vertices_.size(); ++i)
    {
        const Vector3& pos = lightProbesMesh_.vertices_[i];
        const Vector3& normal = lightProbesMesh_.hullNormals_[i];
        if (normal != Vector3::ZERO)
            debug->AddLine(pos, pos + normal, Color::YELLOW);
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

Vector4 GlobalIllumination::SampleLightProbeMesh(const Vector3& position, unsigned& hint) const
{
    const unsigned maxIters = lightProbesCollection_.Size();
    if (hint >= maxIters)
        hint = 0;

    Vector4 weights;
    for (unsigned i = 0; i < maxIters; ++i)
    {
        weights = lightProbesMesh_.GetInnerBarycentricCoords(hint, position);
        if (weights.x_ >= 0.0f && weights.y_ >= 0.0f && weights.z_ >= 0.0f && weights.w_ >= 0.0f)
            return weights;

        if (weights.x_ < weights.y_ && weights.x_ < weights.z_ && weights.x_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[0];
        else if (weights.y_ < weights.z_ && weights.y_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[1];
        else if (weights.z_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[2];
        else
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[3];

        // TODO(glow): Need to handle it more gracefully
        if (hint == M_MAX_UNSIGNED)
            break;
    }
    return weights;
}

SphericalHarmonicsDot9 GlobalIllumination::SampleAmbientSH(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    const Vector4& weights = SampleLightProbeMesh(position, hint);
    if (hint >= lightProbesMesh_.tetrahedrons_.size())
        return SphericalHarmonicsDot9{};

    const Tetrahedron& tetrahedron = lightProbesMesh_.tetrahedrons_[hint];

    SphericalHarmonicsDot9 sh;
    for (unsigned i = 0; i < 4; ++i)
        sh += lightProbesCollection_.lightProbes_[tetrahedron.indices_[i]].bakedLight_ * weights[i];
    return sh;
}

Vector3 GlobalIllumination::SampleAverageAmbient(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    const Vector4& weights = SampleLightProbeMesh(position, hint);
    if (hint >= lightProbesMesh_.tetrahedrons_.size())
        return Vector3::ZERO;

    const Tetrahedron& tetrahedron = lightProbesMesh_.tetrahedrons_[hint];

    Vector3 ambient;
    for (unsigned i = 0; i < 4; ++i)
        ambient += lightProbesCollection_.lightProbes_[tetrahedron.indices_[i]].bakedLight_.EvaluateAverage() * weights[i];
    ambient.x_ = Pow(ambient.x_, 1 / 2.2f);
    ambient.y_ = Pow(ambient.y_, 1 / 2.2f);
    ambient.z_ = Pow(ambient.z_, 1 / 2.2f);
    return ambient;
}

}
