// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

namespace Urho3D
{

/// Image mipmap generation algorithm.
enum class ImageMipMapAlgorithm
{
    /// Component-wise average. Color channels and alpha channel are treated in the same way.
    Average,
    /// Alpha-weighted linear composition of colors. Preserves perceived color for transparent images.
    AlphaWeightedLinear,
};

/// Image mipmap generation parameters.
struct ImageMipMapParams
{
    ImageMipMapAlgorithm algorithm_{};

    /// Alpha is scaled by this factor after color composition.
    /// Adjust this parameter to tweak perceived cutout texture transparency.
    float alphaScaleFactor_{1.0f};
};

} // namespace Urho3D
