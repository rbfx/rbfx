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

#include "../Math/PerlinNoise.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

PerlinNoise::PerlinNoise(RandomEngine& engine)
{
    ea::iota(p_.begin(), p_.begin() + NumPer, 0);
    engine.Shuffle(p_.begin(), p_.begin() + NumPer);
    ea::copy(p_.begin(), p_.begin() + NumPer, p_.begin() + NumPer);
}

double PerlinNoise::GetDouble(double x, double y, double z, int repeat) const
{
    repeat = Clamp(repeat, 0, static_cast<int>(NumPer));

    const int hX = AbsMod(static_cast<int>(floor(x)), repeat);
    const int hY = AbsMod(static_cast<int>(floor(y)), repeat);
    const int hZ = AbsMod(static_cast<int>(floor(z)), repeat);

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    const double u = Fade(x);
    const double v = Fade(y);
    const double w = Fade(z);

    const double aaa = Grad(p_[p_[p_[hX] + hY] + hZ], x, y, z);
    const double aab = Grad(p_[p_[p_[Inc(hX, repeat)] + hY] + hZ], x - 1, y, z);
    const double aba = Grad(p_[p_[p_[hX] + Inc(hY, repeat)] + hZ], x, y - 1, z);
    const double abb = Grad(p_[p_[p_[Inc(hX, repeat)] + Inc(hY, repeat)] + hZ], x - 1, y - 1, z);
    const double baa = Grad(p_[p_[p_[hX] + hY] + Inc(hZ, repeat)], x, y, z - 1);
    const double bab = Grad(p_[p_[p_[Inc(hX, repeat)] + hY] + Inc(hZ, repeat)], x - 1, y, z - 1);
    const double bba = Grad(p_[p_[p_[hX] + Inc(hY, repeat)] + Inc(hZ, repeat)], x, y - 1, z - 1);
    const double bbb = Grad(p_[p_[p_[Inc(hX, repeat)] + Inc(hY, repeat)] + Inc(hZ, repeat)], x - 1, y - 1, z - 1);

    const double aa = Lerp(aaa, aab, u);
    const double ab = Lerp(aba, abb, u);
    const double ba = Lerp(baa, bab, u);
    const double bb = Lerp(bba, bbb, u);

    const double a = Lerp(aa, ab, v);
    const double b = Lerp(ba, bb, v);

    const double res = Lerp(a, b, w);
    return (res + 1.0) / 2.0;
}

double PerlinNoise::Fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::Grad(int hash, double x, double y, double z) const
{
    // Convert lower 4 bits of hash into 12 gradient directions
    const int h = hash & 15;
    const double u = h < 8 ? x : y;
    const double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

}
