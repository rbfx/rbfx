// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Utility/AssetTransformer.h>

namespace Urho3D
{

/// A custom component provided by the plugin.
class PixelArtGenerator
    : public AssetTransformer
{
    URHO3D_OBJECT(PixelArtGenerator, AssetTransformer);

public:
    PixelArtGenerator(Context* context);

    static void RegisterObject(Context* context);

    bool IsApplicable(const AssetTransformerInput& input) override;
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers) override;

private:
    unsigned maxSize_{32};
};

}
