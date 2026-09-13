// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    /// Return noise value as double.
    double GetDouble(double x, int repeat = NumPer) const;
    /// Return noise value as float.
    float Get(float x, float y, float z, int repeat = NumPer) const { return static_cast<float>(GetDouble(x, y, z, repeat)); }
    /// Return noise value as float.
    float Get(float x, int repeat = NumPer) const { return static_cast<float>(GetDouble(x, repeat)); }
private:
    /// Apply 5-th order smoothstep.
    static double Fade(double t);
    /// Increment coordinate.
    static int Inc(int coord, int repeat) { return AbsMod(coord + 1, repeat); }
    /// Return random gradient.
    double Grad(int hash, double x, double y, double z) const;
    /// Return random gradient.
    double Grad(int hash, double x) const;

    /// Permutations.
    ea::array<int, NumPer * 2> p_{};
};

}
