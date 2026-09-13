// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphNode.h"
#include "Helpers.h"

namespace Urho3D
{

class ParticleGraphSystem;

namespace ParticleGraphNodes
{

enum class EmitFrom
{
    Base,
    Volume,
    Surface,
    Edge,
    Vertex,
};

const char** GetEmitFromNames();

} // namespace ParticleGraphNodes

} // namespace Urho3D
