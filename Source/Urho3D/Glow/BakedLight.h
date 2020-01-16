//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

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
        , color_(light->GetEffectiveColor())
        , radius_(lightType_ != LIGHT_DIRECTIONAL ? light->GetRadius() : 0.0f)
        , angle_(lightType_ == LIGHT_DIRECTIONAL ? light->GetRadius() : 0.0f)
        , position_(light->GetNode()->GetWorldPosition())
        , rotation_(light->GetNode()->GetWorldRotation())
        , direction_(light->GetNode()->GetWorldDirection())
    {
    }

    /// Light type.
    LightType lightType_{};
    /// Light mode.
    LightMode lightMode_{};
    /// Light color.
    Color color_{};
    /// Light radius (for spot and point lights).
    float radius_{};
    /// Light angle (for directional light).
    float angle_{};
    /// Position.
    Vector3 position_;
    /// Direction.
    Vector3 direction_;
    /// Rotation.
    Quaternion rotation_;
};

}
