// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Math/StringHash.h>

#include <benchmark/benchmark.h>

using namespace Urho3D;

static void SampleBenchmark(benchmark::State& state)
{
    StringHash value;
    for (auto _ : state)
        value = StringHash{"foo"};
}
BENCHMARK(SampleBenchmark);

BENCHMARK_MAIN();
