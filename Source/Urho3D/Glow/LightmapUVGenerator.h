//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Graphics/ModelView.h"

#include <EASTL/vector.h>

namespace Urho3D
{

/// Lightmap UV generation settings.
struct URHO3D_API LightmapUVGenerationSettings
{
    /// Metadata key for lightmap size.
    static const ea::string LightmapSizeKey;
    /// Metadata key for lightmap density.
    static const ea::string LightmapDensityKey;
    /// Metadata key for shared lightmap UV flag.
    static const ea::string LightmapSharedUV;

    /// Texels per unit.
    float texelPerUnit_{ 10 };
    /// UV channel to write. 2nd channel by default.
    unsigned uvChannel_{ 1 };
};

/// Generate lightmap UVs for the model.
bool URHO3D_API GenerateLightmapUV(ModelView& model, const LightmapUVGenerationSettings& settings);

}
