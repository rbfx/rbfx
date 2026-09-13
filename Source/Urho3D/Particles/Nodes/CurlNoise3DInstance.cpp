// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "CurlNoise3DInstance.h"

#include "../../Engine/Engine.h"
#include "../ParticleGraphLayerInstance.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
CurlNoise3DInstance::CurlNoise3DInstance()
    : noise_(RandomEngine::GetDefaultEngine())
{
}

void CurlNoise3DInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);
    scrollPos_ = 0.0;

    //noise_.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    //noise_.SetFractalOctaves(1);
    //noise_.SetFrequency(0.99f);
}

Vector3 CurlNoise3DInstance::Generate(const Vector3& pos)
{
    constexpr float offset = 0.01f;

    constexpr float frequency = 2.0f;
    double x = pos.x_ * frequency;
    double y = pos.y_ * frequency;
    double z = pos.z_ * frequency + scrollPos_;
    double scale = 0.02;

    const auto center = noise_.GetDouble(x, y, z) * scale;

    const double sampleX = noise_.GetDouble(x + offset, y, z) * scale;
    const double sampleY = noise_.GetDouble(x, y + offset, z) * scale;
    const double sampleZ = noise_.GetDouble(x, y, z + offset) * scale;

    const double dx = (sampleX - center) / offset;
    const double dy = (sampleY - center) / offset;
    const double dz = (sampleZ - center) / offset;
    return Vector3(dz - dy, dx - dz, dy - dx);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
