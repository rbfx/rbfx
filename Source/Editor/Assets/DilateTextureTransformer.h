// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/Project.h"

#include <Urho3D/Utility/AssetTransformer.h>

#include <regex>

namespace Urho3D
{

void Assets_DilateTextureTransformer(Context* context, Project* project);

/// Asset transformer that dilates opaque regions of texture. Only PNG textures are supported.
class DilateTextureTransformer : public AssetTransformer
{
    URHO3D_OBJECT(DilateTextureTransformer, AssetTransformer);

public:
    explicit DilateTextureTransformer(Context* context);

    static void RegisterObject(Context* context);

    bool IsApplicable(const AssetTransformerInput& input) override;
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;

private:
    void MarkRegexesDirty();
    void EnsureInitialized();

private:
    StringVector fileNamePatterns_;
    ea::vector<std::regex> fileNamePatternsRegex_;
    float alphaThreshold_{};
};

} // namespace Urho3D
