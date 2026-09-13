// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Math/BoundingBox.h"
#include "../Math/SphericalHarmonics.h"

namespace Urho3D
{

class TextureCube;

/// Reflection probe data. Reused by actual reflection probes and zones.
struct ReflectionProbeData
{
    /// Reflection map, should never be null.
    TextureCube* reflectionMap_{};
    /// Roughness to LOD factor. Should be equal to log2(NumLODs - 1).
    float roughnessToLODFactor_{};

    /// Position of cubemap center. W component indicates whether it is initialized.
    Vector4 cubemapCenter_{};
    /// World-space bounding box used for cubemap box projection.
    BoundingBox projectionBox_;
};

/// Reference to reflection probe affecting geometry.
struct ReflectionProbeReference
{
    const ReflectionProbeData* data_{};
    int priority_{};
    float volume_{};

    void Reset() { data_ = nullptr; }

    operator bool() const { return !!data_; }

    bool IsMoreImportantThan(const ReflectionProbeReference& other) const
    {
        return other.priority_ < priority_ || other.volume_ < volume_;
    }
};

}
