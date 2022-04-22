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

#include "BufferSuballocator.h"

#include <vector>
#include <algorithm>
#include <thread>

#include "GPUTestingEnvironment.hpp"
#include "FastRand.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(BufferSuballocatorTest, Create)
{
    auto* const pEnv     = GPUTestingEnvironment::GetInstance();
    auto* const pDevice  = pEnv->GetDevice();
    auto* const pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    BufferSuballocatorCreateInfo CI;
    CI.Desc.Name      = "Buffer Suballocator Test";
    CI.Desc.BindFlags = BIND_VERTEX_BUFFER;
    CI.Desc.Size      = 1024;

    RefCntAutoPtr<IBufferSuballocator> pAllocator;
    CreateBufferSuballocator(pDevice, CI, &pAllocator);

    auto* pBuffer = pAllocator->GetBuffer(pDevice, pContext);
    EXPECT_NE(pBuffer, nullptr);

    RefCntAutoPtr<IBufferSuballocation> pAlloc;
    pAllocator->Allocate(256, 16, &pAlloc);
    // Release allocator first
    pAllocator.Release();
    pAlloc.Release();
}

TEST(BufferSuballocatorTest, Allocate)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    BufferSuballocatorCreateInfo CI;
    CI.Desc.Name      = "Buffer Suballocator Test";
    CI.Desc.BindFlags = BIND_VERTEX_BUFFER;
    CI.Desc.Size      = 1024;

    RefCntAutoPtr<IBufferSuballocator> pAllocator;
    CreateBufferSuballocator(pDevice, CI, &pAllocator);

#ifdef DILIGENT_DEBUG
    constexpr size_t NumIterations = 8;
#else
    constexpr size_t NumIterations = 32;
#endif
    for (size_t i = 0; i < NumIterations; ++i)
    {
        const size_t NumThreads = std::max(4u, std::thread::hardware_concurrency());

        const size_t NumAllocations = NumIterations * 8;

        std::vector<std::vector<RefCntAutoPtr<IBufferSuballocation>>> pSubAllocations(NumThreads);
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
                            Uint32 size = static_cast<Uint32>(rnd());
                            pAllocator->Allocate(size, 8, &Alloc);
                            ASSERT_TRUE(Alloc);
                            EXPECT_EQ(Alloc->GetSize(), size);
                        }
                    },
                    t //
                };
            }

            for (auto& Thread : Threads)
                Thread.join();
        }

        auto* pBuffer = pAllocator->GetBuffer(pDevice, pContext);
        EXPECT_NE(pBuffer, nullptr);

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

} // namespace
