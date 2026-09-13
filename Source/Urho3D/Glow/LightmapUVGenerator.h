// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/ModelView.h"

#include <EASTL/vector.h>

namespace Urho3D
{

/// Lightmap UV generation settings.
struct URHO3D_API LightmapUVGenerationSettings
{
    /// Metadata key for lightmap size.
    static const ea::string LightmapSizeKey;
    /// Metadata key for lightmap density.
    static const ea::string LightmapDensityKey;
    /// Metadata key for shared lightmap UV flag.
    static const ea::string LightmapSharedUV;

    /// Texels per unit.
    float texelPerUnit_{ 10 };
    /// UV channel to write. 2nd channel by default.
    unsigned uvChannel_{ 1 };
};

/// Generate lightmap UVs for the model.
bool URHO3D_API GenerateLightmapUV(ModelView& model, const LightmapUVGenerationSettings& settings);

}
