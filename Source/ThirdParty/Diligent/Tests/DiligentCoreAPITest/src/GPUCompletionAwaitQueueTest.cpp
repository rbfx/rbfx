/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "GPUCompletionAwaitQueue.hpp"
#include "GPUTestingEnvironment.hpp"
#include "MapHelper.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(GPUCompletionAwaitQueueTest, ReadBack)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    constexpr Uint32 NumTestBuffs = 3;
    constexpr float  TestData[NumTestBuffs][16] =
        {
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
            {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3},
            {9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8} //
        };
    constexpr auto BuffSize = sizeof(TestData[0]);

    RefCntAutoPtr<IBuffer> pBuffer[NumTestBuffs];
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "GPU Completion Await Queue Test";
        BuffDesc.Size      = BuffSize;
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage     = USAGE_DEFAULT;

        for (Uint32 buff = 0; buff < NumTestBuffs; ++buff)
        {
            BufferData InitData{TestData[buff], BuffSize};
            pDevice->CreateBuffer(BuffDesc, &InitData, &pBuffer[buff]);
            ASSERT_NE(pBuffer, nullptr);
        }
    }

    GPUCompletionAwaitQueue<RefCntAutoPtr<IBuffer>> ReadBackQueue{pDevice};

    for (Uint32 pass = 0; pass < 3; ++pass)
    {
        for (Uint32 buff = 0; buff < NumTestBuffs; ++buff)
        {
            auto pStagingBuffer = ReadBackQueue.GetRecycled();
            EXPECT_TRUE(pStagingBuffer || pass == 0);

            if (!pStagingBuffer)
            {
                BufferDesc BuffDesc;
                BuffDesc.Name           = "GPU Completion Await Queue Test - Staging Buffer";
                BuffDesc.Size           = BuffSize;
                BuffDesc.BindFlags      = BIND_NONE;
                BuffDesc.Usage          = USAGE_STAGING;
                BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
                pDevice->CreateBuffer(BuffDesc, nullptr, &pStagingBuffer);
                ASSERT_NE(pStagingBuffer, nullptr);
            }

            pContext->CopyBuffer(pBuffer[(buff + pass) % NumTestBuffs], 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                 pStagingBuffer, 0, BuffSize, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            ReadBackQueue.Enqueue(pContext, std::move(pStagingBuffer));
        }

        {
            auto pStagingBuffer = ReadBackQueue.GetRecycled();
            EXPECT_EQ(pStagingBuffer, nullptr);
        }

        pContext->WaitForIdle();

        for (Uint32 buff = 0; buff < NumTestBuffs; ++buff)
        {
            auto pStagingBuffer = ReadBackQueue.GetFirstCompleted();
            ASSERT_NE(pStagingBuffer, nullptr);

            {
                MapHelper<float> ReadBackData{pContext, pStagingBuffer, MAP_READ, MAP_FLAG_DO_NOT_WAIT};
                EXPECT_EQ(memcmp(ReadBackData, TestData[(buff + pass) % NumTestBuffs], BuffSize), 0);
            }

            ReadBackQueue.Recycle(std::move(pStagingBuffer));
        }

        {
            auto pStagingBuffer = ReadBackQueue.GetFirstCompleted();
            EXPECT_EQ(pStagingBuffer, nullptr);
        }
    }
}

} // namespace
