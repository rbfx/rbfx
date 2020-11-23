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

#pragma once

#include "../Math/RandomEngine.h"

namespace Urho3D
{

/// Perlin noise generator.
class URHO3D_API PerlinNoise
{
public:
    /// Number of permutations.
    static const unsigned NumPer = 256;
    /// Construct and initialize from random generator.
    explicit PerlinNoise(RandomEngine& engine);
    /// Return noise value as double.
    double GetDouble(double x, double y, double z, int repeat = NumPer) const;
    /// Return noise value as float.
    float Get(float x, float y, float z, int repeat = NumPer) const { return static_cast<float>(GetDouble(x, y, z, repeat)); }

private:
    /// Apply 5-th order smoothstep.
    static double Fade(double t);
    /// Increment coordinate.
    static int Inc(int coord, int repeat) { return AbsMod(coord + 1, repeat); }
    /// Return random gradient.
    double Grad(int hash, double x, double y, double z) const;

    /// Permutations.
    ea::array<int, NumPer * 2> p_{};
};

}
