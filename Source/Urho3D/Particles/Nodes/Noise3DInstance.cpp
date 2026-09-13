// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Noise3DInstance.h"

#include "../ParticleGraphLayerInstance.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
Noise3DInstance::Noise3DInstance()
    : noise_(RandomEngine::GetDefaultEngine())
{
}

void Noise3DInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);
}

float Noise3DInstance::Generate(const Vector3& pos) const
{
    return noise_.GetDouble(pos.x_, pos.y_, pos.z_);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
