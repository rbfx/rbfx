//
// Copyright (c) 2021-2022 the rbfx project.
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
