// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/Light.h"
#include "../Math/Vector3.h"
#include "../Scene/Node.h"

namespace Urho3D
{

/// Baked light description.
struct BakedLight
{
    /// Construct default.
    BakedLight() = default;

    /// Construct valid.
    explicit BakedLight(Light* light)
        : lightType_(light->GetLightType())
        , lightMode_(light->GetLightMode())
        , lightMask_(light->GetLightMask())
        , color_(light->GetEffectiveColor().GammaToLinear())
        , indirectBrightness_(light->GetIndirectBrightness())
        , distance_(light->GetRange())
        , fov_(light->GetFov())
        , cutoff_(Cos(fov_ * 0.5f))
        , radius_(lightType_ != LIGHT_DIRECTIONAL ? light->GetRadius() : 0.0f)
        , angle_(lightType_ == LIGHT_DIRECTIONAL ? light->GetRadius() : 0.0f)
        , halfAngleTan_(Tan(angle_ / 2.0f))
        , position_(light->GetNode()->GetWorldPosition())
        , rotation_(light->GetNode()->GetWorldRotation())
        , direction_(light->GetNode()->GetWorldDirection())
    {
    }

    /// Light type.
    LightType lightType_{};
    /// Light mode.
    LightMode lightMode_{};
    unsigned lightMask_{};
    /// Light color.
    Color color_{};
    /// Indirect brightness.
    float indirectBrightness_{};
    /// FOV angle (for spot lights).
    float fov_{};
    /// Cutoff aka Cos(FOV * 0.5) (for spot lights).
    float cutoff_{};
    /// Light distance (for spot and point lights).
    float distance_{};
    /// Light radius (for spot and point lights).
    float radius_{};
    /// Light angle (for directional light).
    float angle_{};
    /// Tangent of half light angle.
    float halfAngleTan_{};
    /// Position.
    Vector3 position_;
    /// Direction.
    Vector3 direction_;
    /// Rotation.
    Quaternion rotation_;
};

}
