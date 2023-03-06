/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "DynamicTextureAtlas.h"

#include <thread>

#include "GPUTestingEnvironment.hpp"
#include "gtest/gtest.h"
#include "FastRand.hpp"
#include "ThreadSignal.hpp"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(DynamicTextureAtlas, Create)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    auto* const pDevice = pEnv->GetDevice();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    DynamicTextureAtlasCreateInfo CI;
    CI.MinAlignment   = 16;
    CI.Desc.Format    = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name      = "Dynamic Texture Atlas Test";
    CI.Desc.Type      = RESOURCE_DIM_TEX_2D;
    CI.Desc.BindFlags = BIND_SHADER_RESOURCE;
    CI.Desc.Width     = 512;
    CI.Desc.Height    = 512;

    RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
    CreateDynamicTextureAtlas(nullptr, CI, &pAtlas);

    auto* pTexture = pAtlas->GetTexture(pDevice, nullptr);
    EXPECT_NE(pTexture, nullptr);

    RefCntAutoPtr<ITextureAtlasSuballocation> pSuballoc;
    pAtlas->Allocate(128, 128, &pSuballoc);
    EXPECT_TRUE(pSuballoc);

    DynamicTextureAtlasUsageStats Stats;
    pAtlas->GetUsageStats(Stats);
    EXPECT_EQ(Stats.AllocationCount, 1u);
    EXPECT_EQ(Stats.TotalArea, CI.Desc.Width * CI.Desc.Height);
    EXPECT_EQ(Stats.AllocatedArea, 128u * 128u);
    EXPECT_EQ(Stats.UsedArea, 128u * 128u);
    EXPECT_GE(Stats.Size, 0u);
}

TEST(DynamicTextureAtlas, CreateArray)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    DynamicTextureAtlasCreateInfo CI;
    CI.ExtraSliceCount = 2;
    CI.MinAlignment    = 16;
    CI.Desc.Format     = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name       = "Dynamic Texture Atlas Test";
    CI.Desc.Type       = RESOURCE_DIM_TEX_2D_ARRAY;
    CI.Desc.BindFlags  = BIND_SHADER_RESOURCE;
    CI.Desc.Width      = 512;
    CI.Desc.Height     = 512;
    CI.Desc.ArraySize  = 0;

    {
        RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
        CreateDynamicTextureAtlas(nullptr, CI, &pAtlas);

        auto* pTexture = pAtlas->GetTexture(nullptr, nullptr);
        EXPECT_EQ(pTexture, nullptr);

        RefCntAutoPtr<ITextureAtlasSuballocation> pSuballoc;
        pAtlas->Allocate(128, 128, &pSuballoc);
        EXPECT_TRUE(pSuballoc);

        pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);

        DynamicTextureAtlasUsageStats Stats;
        pAtlas->GetUsageStats(Stats);
        EXPECT_EQ(Stats.AllocationCount, 1u);
        EXPECT_EQ(Stats.TotalArea, CI.Desc.Width * CI.Desc.Height * 2u);
        EXPECT_EQ(Stats.AllocatedArea, 128u * 128u);
        EXPECT_EQ(Stats.UsedArea, 128u * 128u);
        EXPECT_GE(Stats.Size, 0u);
    }

    CI.Desc.ArraySize = 2;
    {
        RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
        CreateDynamicTextureAtlas(nullptr, CI, &pAtlas);

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);
    }

    {
        RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
        CreateDynamicTextureAtlas(pDevice, CI, &pAtlas);

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);

        RefCntAutoPtr<ITextureAtlasSuballocation> pSuballoc;
        pAtlas->Allocate(128, 128, &pSuballoc);
        EXPECT_TRUE(pSuballoc);

        // Release atlas first
        pAtlas.Release();
        pSuballoc.Release();
    }
}

TEST(DynamicTextureAtlas, Allocate)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    DynamicTextureAtlasCreateInfo CI;
    CI.ExtraSliceCount = 2;
    CI.MinAlignment    = 16;
    CI.Desc.Format     = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name       = "Dynamic Texture Atlas Test";
    CI.Desc.Type       = RESOURCE_DIM_TEX_2D_ARRAY;
    CI.Desc.BindFlags  = BIND_SHADER_RESOURCE;
    CI.Desc.Width      = 512;
    CI.Desc.Height     = 512;
    CI.Desc.ArraySize  = 1;

    RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
    CreateDynamicTextureAtlas(pDevice, CI, &pAtlas);

#ifdef DILIGENT_DEBUG
    constexpr size_t NumIterations = 8;
#else
    constexpr size_t NumIterations = 32;
#endif
    for (size_t i = 0; i < NumIterations; ++i)
    {
        const size_t NumThreads = std::max(4u, std::thread::hardware_concurrency());

        const size_t NumAllocations = i * 8;

        std::vector<std::vector<RefCntAutoPtr<ITextureAtlasSuballocation>>> pSubAllocations(NumThreads);
        for (auto& Allocs : pSubAllocations)
            Allocs.resize(NumAllocations);

        {
            std::vector<std::thread> Threads(NumThreads);
            for (size_t t = 0; t < Threads.size(); ++t)
            {
                Threads[t] = std::thread{
                    [&](size_t thread_id) //
                    {
                        FastRandInt rnd{static_cast<unsigned int>(thread_id), 4, 64};

                        auto& Allocs = pSubAllocations[thread_id];
                        for (auto& Alloc : Allocs)
                        {
                            Uint32 Width  = static_cast<Uint32>(rnd());
                            Uint32 Height = static_cast<Uint32>(rnd());
                            pAtlas->Allocate(Width, Height, &Alloc);
                            ASSERT_TRUE(Alloc);
                            EXPECT_EQ(Alloc->GetSize().x, Width);
                            EXPECT_EQ(Alloc->GetSize().y, Height);
                        }
                    },
                    t //
                };
            }

            for (auto& Thread : Threads)
                Thread.join();
        }

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);

        {
            std::vector<std::thread> Threads(NumThreads);
            for (size_t t = 0; t < Threads.size(); ++t)
            {
                Threads[t] = std::thread{
                    [&](size_t thread_id) //
                    {
                        auto& Allocs = pSubAllocations[thread_id];
                        for (auto& Alloc : Allocs)
                            Alloc.Release();
                    },
                    t //
                };
            }

            for (auto& Thread : Threads)
                Thread.join();
        }
    }
}


// Allocate more regions than the atlas can hold
TEST(DynamicTextureAtlas, Overflow)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    constexpr Uint32 AtlasDim            = 512;
    constexpr Uint32 AllocDim            = 128;
    constexpr Uint32 AllocationsPerSlice = (AtlasDim / AllocDim) * (AtlasDim / AllocDim);
    constexpr Uint32 MaxSliceCount       = 2;

    const Uint32 NumThreads = std::max(4u, std::thread::hardware_concurrency() * 4);

    DynamicTextureAtlasCreateInfo CI;
    CI.ExtraSliceCount = 2;
    CI.MaxSliceCount   = MaxSliceCount;
    CI.MinAlignment    = 16;
    CI.Silent          = true;
    CI.Desc.Format     = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name       = "Dynamic Texture Atlas Overflow Test";
    CI.Desc.Type       = RESOURCE_DIM_TEX_2D_ARRAY;
    CI.Desc.BindFlags  = BIND_SHADER_RESOURCE;
    CI.Desc.Width      = AtlasDim;
    CI.Desc.Height     = AtlasDim;
    CI.Desc.ArraySize  = MaxSliceCount;

    RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
    CreateDynamicTextureAtlas(pDevice, CI, &pAtlas);

#ifdef DILIGENT_DEBUG
    constexpr size_t NumIterations = 8;
#else
    constexpr size_t NumIterations = 32;
#endif

    for (size_t i = 0; i < NumIterations; ++i)
    {
        {
            std::vector<std::thread> Threads(NumThreads);
            for (size_t t = 0; t < Threads.size(); ++t)
            {
                Threads[t] = std::thread{
                    [&]() //
                    {
                        std::vector<RefCntAutoPtr<ITextureAtlasSuballocation>> pSubAllocations(AllocationsPerSlice);
                        for (auto& pSubAlloc : pSubAllocations)
                            pAtlas->Allocate(AllocDim, AllocDim, &pSubAlloc);
                    } //
                };
            }

            for (auto& Thread : Threads)
                Thread.join();
        }

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);
    }
}

// Test allocation race
TEST(DynamicTextureAtlas, AllocRace)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    const Uint32 NumThreads = std::max(4u, std::thread::hardware_concurrency() * 4);

    constexpr Uint32 AtlasDim            = 512;
    constexpr Uint32 AllocDim            = 256;
    constexpr Uint32 AllocationsPerSlice = (AtlasDim / AllocDim) * (AtlasDim / AllocDim);

    DynamicTextureAtlasCreateInfo CI;
    CI.ExtraSliceCount = 2;
    CI.MaxSliceCount   = NumThreads;
    CI.Silent          = true;
    CI.MinAlignment    = 16;
    CI.Desc.Format     = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name       = "Dynamic Texture Atlas Alloc Race Test";
    CI.Desc.Type       = RESOURCE_DIM_TEX_2D_ARRAY;
    CI.Desc.BindFlags  = BIND_SHADER_RESOURCE;
    CI.Desc.Width      = AtlasDim;
    CI.Desc.Height     = AtlasDim;
    CI.Desc.ArraySize  = 2;

    RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
    CreateDynamicTextureAtlas(pDevice, CI, &pAtlas);

    Threading::Signal   AllocSignal;
    Threading::Signal   ReleaseSignal;
    Threading::Signal   AllocCompleteSignal;
    Threading::Signal   ReleaseCompleteSignal;
    std::atomic<Uint32> NumThreadsReady{0};

    std::vector<std::thread> Threads(NumThreads);
    for (size_t t = 0; t < Threads.size(); ++t)
    {
        Threads[t] = std::thread{
            [&]() //
            {
                while (true)
                {
                    auto Ret = AllocSignal.Wait(true, NumThreads);
                    if (Ret < 0)
                        break;

                    std::vector<RefCntAutoPtr<ITextureAtlasSuballocation>> pSubAllocations(AllocationsPerSlice);
                    for (auto& pSubAlloc : pSubAllocations)
                    {
                        pAtlas->Allocate(AllocDim, AllocDim, &pSubAlloc);
                        // Note: some allocations may fail even if there is enough space
                    }
                    if (NumThreadsReady.fetch_add(1) + 1 == NumThreads)
                    {
                        AllocCompleteSignal.Trigger();
                    }

                    ReleaseSignal.Wait(true, NumThreads);
                    pSubAllocations.clear();
                    if (NumThreadsReady.fetch_add(1) + 1 == NumThreads)
                    {
                        ReleaseCompleteSignal.Trigger();
                    }
                }
            } //
        };
    }

#ifdef DILIGENT_DEBUG
    constexpr size_t NumIterations = 64;
#else
    constexpr size_t NumIterations = 512;
#endif
    for (size_t i = 0; i < NumIterations; ++i)
    {
        NumThreadsReady.store(0);
        AllocSignal.Trigger(true);

        AllocCompleteSignal.Wait(true, 1);

        NumThreadsReady.store(0);
        ReleaseSignal.Trigger(true);

        ReleaseCompleteSignal.Wait(true, 1);

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);
    }

    AllocSignal.Trigger(true, -1);

    {
        for (auto& Thread : Threads)
            Thread.join();
    }
}


// Make half of the threads release allocations, while another half create them
TEST(DynamicTextureAtlas, AllocFreeRace)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    const Uint32 NumThreads = std::max(4u, std::thread::hardware_concurrency() * 4);

    constexpr Uint32 AtlasDim            = 512;
    constexpr Uint32 AllocDim            = 256;
    constexpr Uint32 AllocationsPerSlice = (AtlasDim / AllocDim) * (AtlasDim / AllocDim);

    DynamicTextureAtlasCreateInfo CI;
    CI.ExtraSliceCount = 2;
    CI.MaxSliceCount   = NumThreads;
    CI.Silent          = true;
    CI.MinAlignment    = 16;
    CI.Desc.Format     = TEX_FORMAT_RGBA8_UNORM;
    CI.Desc.Name       = "Dynamic Texture Atlas Alloc-Free Race Test";
    CI.Desc.Type       = RESOURCE_DIM_TEX_2D_ARRAY;
    CI.Desc.BindFlags  = BIND_SHADER_RESOURCE;
    CI.Desc.Width      = AtlasDim;
    CI.Desc.Height     = AtlasDim;
    CI.Desc.ArraySize  = 2;

    RefCntAutoPtr<IDynamicTextureAtlas> pAtlas;
    CreateDynamicTextureAtlas(pDevice, CI, &pAtlas);

    Threading::Signal   AllocSignal;
    Threading::Signal   ReleaseSignal;
    Threading::Signal   AllocCompleteSignal;
    Threading::Signal   ReleaseCompleteSignal;
    std::atomic<Uint32> NumThreadsReady{0};

    // Pre-populate half of the atlas
    const auto                                             PrePopulatedSliceCount = NumThreads / 2;
    std::vector<RefCntAutoPtr<ITextureAtlasSuballocation>> pAllocations(AllocationsPerSlice * PrePopulatedSliceCount);

    std::vector<std::thread> Threads(NumThreads);
    for (size_t t = 0; t < Threads.size(); ++t)
    {
        Threads[t] = std::thread{
            [&](size_t t) //
            {
                while (true)
                {
                    auto Ret = AllocSignal.Wait(true, NumThreads);
                    if (Ret < 0)
                        break;

                    std::vector<RefCntAutoPtr<ITextureAtlasSuballocation>> pThreadAllocations(AllocationsPerSlice);
                    if (t < PrePopulatedSliceCount)
                    {
                        // First half of the threads - release allocations
                        for (size_t i = 0; i < AllocationsPerSlice; ++i)
                            pAllocations[t * AllocationsPerSlice + i].Release();
                    }
                    else
                    {
                        // Second half of the threads - create allocations
                        for (auto& pSubAlloc : pThreadAllocations)
                            pAtlas->Allocate(AllocDim, AllocDim, &pSubAlloc);
                    }

                    if (NumThreadsReady.fetch_add(1) + 1 == NumThreads)
                    {
                        AllocCompleteSignal.Trigger();
                    }

                    ReleaseSignal.Wait(true, NumThreads);
                    pThreadAllocations.clear();
                    if (NumThreadsReady.fetch_add(1) + 1 == NumThreads)
                    {
                        ReleaseCompleteSignal.Trigger();
                    }
                }
            },
            t //
        };
    }

#ifdef DILIGENT_DEBUG
    constexpr size_t NumIterations = 64;
#else
    constexpr size_t NumIterations = 512;
#endif
    for (size_t i = 0; i < NumIterations; ++i)
    {
        DynamicTextureAtlasUsageStats UsageStats;
        pAtlas->GetUsageStats(UsageStats);
        EXPECT_EQ(UsageStats.AllocationCount, 0u) << "iteration: " << i;

        // Use half of the atlas
        for (size_t j = 0; j < pAllocations.size(); ++j)
        {
            auto& pAlloc = pAllocations[j];
            pAtlas->Allocate(AllocDim, AllocDim, &pAlloc);
            EXPECT_TRUE(pAlloc) << "alloc idx: " << j << "; iteration: " << i;
        }

        NumThreadsReady.store(0);
        AllocSignal.Trigger(true);

        AllocCompleteSignal.Wait(true, 1);

        NumThreadsReady.store(0);
        ReleaseSignal.Trigger(true);

        ReleaseCompleteSignal.Wait(true, 1);

        auto* pTexture = pAtlas->GetTexture(pDevice, pContext);
        EXPECT_NE(pTexture, nullptr);
    }

    AllocSignal.Trigger(true, -1);

    {
        for (auto& Thread : Threads)
            Thread.join();
    }
}

} // namespace
