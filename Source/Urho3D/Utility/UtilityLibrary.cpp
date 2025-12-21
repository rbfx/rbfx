// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Utility/UtilityLibrary.h"

#include "Urho3D/Utility/AnimationVelocityExtractor.h"
#include "Urho3D/Utility/AssetPipeline.h"
#include "Urho3D/Utility/AssetTransformer.h"
#include "Urho3D/Utility/GenerateWorldSpaceTracksTransformer.h"
#include "Urho3D/Utility/RetargetAnimationsTransformer.h"
#include "Urho3D/Utility/SceneViewerApplication.h"

namespace Urho3D
{

void RegisterUtilityLibrary(Context* context)
{
    SceneViewerApplication::RegisterObject();

    context->AddFactoryReflection<AssetPipeline>();
    context->AddFactoryReflection<AssetTransformer>();

    AnimationVelocityExtractor::RegisterObject(context);
    RetargetAnimationsTransformer::RegisterObject(context);
    GenerateWorldSpaceTracksTransformer::RegisterObject(context);
}

} // namespace Urho3D
