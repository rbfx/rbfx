// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Glow/LightmapGeometryBuffer.h"
#include "Urho3D/Graphics/Texture2D.h"

namespace Urho3D
{

class Model;

/// Stiching context.
struct LightmapStitchingContext
{
    /// Context.
    Context* context_{};
    /// Lightmap size.
    unsigned lightmapSize_{};
    /// Number of texture channels.
    unsigned numChannels_{};
    /// First texture for ping-pong.
    SharedPtr<Texture2D> pingTexture_;
    /// Second texture for ping-pong.
    SharedPtr<Texture2D> pongTexture_;
};

/// Initialize lightmap stitching context.
URHO3D_API LightmapStitchingContext InitializeStitchingContext(
    Context* context, unsigned lightmapSize, unsigned numChannels);

/// Create model for lightmap seams.
URHO3D_API SharedPtr<Model> CreateSeamsModel(Context* context, const LightmapSeamVector& seams);

/// Stitch seams in the image and store result in context.
URHO3D_API void StitchLightmapSeams(LightmapStitchingContext& stitchingContext,
    const ea::vector<Vector3>& inputBuffer, ea::vector<Vector4>& outputBuffer,
    const LightmapStitchingSettings& settings, Model* seamsModel);

}
