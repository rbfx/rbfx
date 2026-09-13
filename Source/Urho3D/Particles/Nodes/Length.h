// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../PatternMatchingNode.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class URHO3D_API Length : public PatternMatchingNode
{
    URHO3D_OBJECT(Length, ParticleGraphNode)
public:
    /// Construct Length.
    explicit Length(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

protected:
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
