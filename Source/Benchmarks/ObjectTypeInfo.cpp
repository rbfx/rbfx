// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Scene/PrefabReference.h>

#include <benchmark/benchmark.h>

using namespace Urho3D;

namespace
{

SharedPtr<Object> CreateRandomObject(unsigned type)
{
    static SharedPtr<Context> context = MakeShared<Context>();
    switch (type % 6)
    {
    case 0: return MakeShared<SoundSource>(context);
    case 1: return MakeShared<Time>(context);
    case 2: return MakeShared<AnimatedModel>(context);
    case 3: return MakeShared<Model>(context);
    case 4: return MakeShared<StaticModel>(context);
    case 5: return MakeShared<PrefabReference>(context);
    default: return nullptr;
    }
}

using ObjectCollection = ea::vector<SharedPtr<Object>>;

ObjectCollection CreateRandomObjects(int n)
{
    auto& rnd = RandomEngine::GetDefaultEngine();
    ObjectCollection result;
    for (int i = 0; i < n; ++i)
        result.push_back(CreateRandomObject(rnd.GetUInt()));
    return result;
}

unsigned CountNothing(const ObjectCollection& objects)
{
    unsigned garbage = 0;
    for (const auto& object : objects)
        garbage += static_cast<unsigned>(reinterpret_cast<size_t>(object.Get()));
    return garbage;
}

unsigned CountExactType(const ObjectCollection& objects, StringHash type)
{
    unsigned num = 0;
    for (const auto& object : objects)
    {
        if (object->GetType() == type)
            ++num;
    }
    return num;
}

unsigned CountHierarchyType(const ObjectCollection& objects, StringHash type)
{
    unsigned num = 0;
    for (const auto& object : objects)
    {
        if (object->GetTypeInfo()->IsTypeOf(type))
            ++num;
    }
    return num;
}

unsigned CountHierarchyTypeFast(const ObjectCollection& objects, StringHash type)
{
    unsigned num = 0;
    for (const auto& object : objects)
    {
        if (object->IsInstanceOf(type))
            ++num;
    }
    return num;
}

template <class T>
unsigned CountDynamicCast(const ObjectCollection& objects)
{
    unsigned num = 0;
    for (const auto& object : objects)
    {
        if (dynamic_cast<T*>(object.Get()) != nullptr)
            ++num;
    }
    return num;
}

const std::vector<int64_t> testedTypes = { //
    AnimatedModel::TypeId.Value(), StaticModel::TypeId.Value(), Serializable::TypeId.Value(), Model::TypeId.Value(),
    Material::TypeId.Value()};

const int iterationCount = 10;

} // namespace

static void Benchmark_CountNothing(benchmark::State& state)
{
    const auto objects = CreateRandomObjects(state.range(0));
    for (auto _ : state)
    {
        const unsigned count = CountNothing(objects);
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(Benchmark_CountNothing)
    ->ArgsProduct({{iterationCount}, testedTypes});

static void Benchmark_CountExact(benchmark::State& state)
{
    const auto objects = CreateRandomObjects(state.range(0));
    for (auto _ : state)
    {
        const unsigned count = CountExactType(objects, StringHash{(unsigned)state.range(1)});
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(Benchmark_CountExact)
    ->ArgsProduct({{iterationCount}, testedTypes});

static void Benchmark_CountHierarchy(benchmark::State& state)
{
    const auto objects = CreateRandomObjects(state.range(0));
    for (auto _ : state)
    {
        const unsigned count = CountHierarchyType(objects, StringHash{(unsigned)state.range(1)});
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(Benchmark_CountHierarchy)
    ->ArgsProduct({{iterationCount}, testedTypes});

template <class T>
static void Benchmark_CountDynamicCast(benchmark::State& state)
{
    const auto objects = CreateRandomObjects(state.range(0));
    for (auto _ : state)
    {
        const unsigned count = CountDynamicCast<T>(objects);
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK_TEMPLATE(Benchmark_CountDynamicCast, AnimatedModel)->Arg(iterationCount);
BENCHMARK_TEMPLATE(Benchmark_CountDynamicCast, StaticModel)->Arg(iterationCount);
BENCHMARK_TEMPLATE(Benchmark_CountDynamicCast, Serializable)->Arg(iterationCount);
BENCHMARK_TEMPLATE(Benchmark_CountDynamicCast, Model)->Arg(iterationCount);
BENCHMARK_TEMPLATE(Benchmark_CountDynamicCast, Material)->Arg(iterationCount);

static void Benchmark_CountHierarchyFast(benchmark::State& state)
{
    const auto objects = CreateRandomObjects(state.range(0));
    for (auto _ : state)
    {
        const unsigned count = CountHierarchyTypeFast(objects, StringHash{(unsigned)state.range(1)});
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(Benchmark_CountHierarchyFast)
    ->ArgsProduct({{iterationCount}, testedTypes});
