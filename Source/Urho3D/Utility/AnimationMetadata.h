// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/ConstString.h"

namespace Urho3D
{

namespace AnimationMetadata
{

/// Resource name of the Model that is meant to be used with this Animation.
URHO3D_GLOBAL_CONSTANT(ConstString Model{"Model"});
/// Whether the Animation is looped.
URHO3D_GLOBAL_CONSTANT(ConstString Looped{"Looped"});
/// Frame step of fixed-rate animation, may contain unspecified value for variable-rate animations.
URHO3D_GLOBAL_CONSTANT(ConstString FrameStep{"FrameStep"});
/// Frame rate of fixed-rate animation, may contain unspecified value for variable-rate animations.
URHO3D_GLOBAL_CONSTANT(ConstString FrameRate{"FrameRate"});
/// Map of parent animation tracks for each animation track. Variant tracks are ignored.
URHO3D_GLOBAL_CONSTANT(ConstString ParentTracks{"ParentTracks"});

/// Linear velocity of root motion that was undone.
URHO3D_GLOBAL_CONSTANT(ConstString RootLinearVelocity{"RootLinearVelocity"});
/// Angular velocity of root motion that was undone.
URHO3D_GLOBAL_CONSTANT(ConstString RootAngularVelocity{"RootAngularVelocity"});
/// "Scale velocity" of root motion that was undone.
URHO3D_GLOBAL_CONSTANT(ConstString RootScaleVelocity{"RootScaleVelocity"});

} // namespace AnimationMetadata

} // namespace Urho3D
