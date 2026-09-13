// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "Emitter.h"

#include "ParticleGraphLayerInstance.h"
#include "Span.h"
#include "UpdateContext.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

namespace
{

const char* emitFromNames[]{"Base", "Volume", "Surface", "Edge", "Vertex", nullptr};

}

const char** GetEmitFromNames()
{
    return emitFromNames;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
