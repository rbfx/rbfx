// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
