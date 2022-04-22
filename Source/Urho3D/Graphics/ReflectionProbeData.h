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

#include "../Math/SphericalHarmonics.h"

namespace Urho3D
{

class TextureCube;

/// Reflection probe data. Reused by actual reflection probes and zones.
struct ReflectionProbeData
{
    /// Reflection map, should never be null.
    TextureCube* reflectionMap_{};
    /// Roughness to LOD factor. Should be equal to log2(NumLODs - 1).
    float roughnessToLODFactor_{};
    /// Smallest LOD of reflection represented as SH. Used by GL ES to approximate textureCubeLod.
    SphericalHarmonicsDot9 reflectionMapSH_;
};

/// Reference to reflection probe affecting geometry.
struct ReflectionProbeReference
{
    const ReflectionProbeData* data_{};
    int priority_{};
    float volume_{};

    void Reset() { data_ = nullptr; }
};

}
