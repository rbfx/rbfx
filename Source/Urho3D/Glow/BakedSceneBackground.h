// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Math/Color.h"
#include "../Math/Vector3.h"
#include "../Resource/ImageCube.h"

#include <EASTL/vector.h>
#include <EASTL/shared_ptr.h>

namespace Urho3D
{

/// Scene background description.
struct BakedSceneBackground
{
    float intensity_{};
    Color color_;
    SharedPtr<ImageCube> image_;

    Vector3 SampleLinear(const Vector3& direction) const
    {
        return intensity_ * SampleGammaInternal(direction).GammaToLinear().ToVector3();
    }

private:
    Color SampleGammaInternal(const Vector3& direction) const
    {
        return image_ ? image_->SampleNearest(direction) : color_;
    }
};

/// Immutable array of scene backgrounds.
using BakedSceneBackgroundArrayPtr = ea::shared_ptr<const ea::vector<BakedSceneBackground>>;

}
