// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Context.h>

#include <EASTL/string.h>

namespace Urho3D
{

/// Scene creation parameters.
struct DefaultSceneParameters
{
    bool highQuality_{};
    bool createObjects_{};
    bool isPrefab_{};
};

/// Create default scene.
void CreateDefaultScene(Context* context, const ea::string& fileName, const DefaultSceneParameters& params);

}
