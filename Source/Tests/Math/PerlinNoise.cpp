//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"

#include <Urho3D/Math/PerlinNoise.h>

using namespace Urho3D;

TEST_CASE("Perlin noise 1D")
{
    double max = ea::numeric_limits<double>::min();
    double min = ea::numeric_limits<double>::max();

    RandomEngine randomEngine{0};
    const PerlinNoise noise{randomEngine};

    for (unsigned i = 0; i < 1024; ++i)
    {
        const double val = noise.GetDouble(i * 1.1);
        min = Min(min, val);
        max = Max(max, val);
    }

    CHECK(min >= 0.0);
    CHECK(max <= 1.0);
}

TEST_CASE("Perlin noise 3D")
{
    double max = ea::numeric_limits<double>::min();
    double min = ea::numeric_limits<double>::max();

    RandomEngine randomEngine{0};
    const PerlinNoise noise{randomEngine};

    for (unsigned x = 0; x < 100; ++x)
    {
        for (unsigned y = 0; y < 100; ++y)
        {
            for (unsigned z = 0; z < 100; ++z)
            {
                const double val = noise.GetDouble(x * 1.1, y * 1.1, z * 1.1);
                min = Min(min, val);
                max = Max(max, val);
            }
        }
    }

    CHECK(min >= 0.0);
    CHECK(max <= 1.0);
}
