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

#include "DynamicBuffer.hpp"
#include "GPUTestingEnvironment.hpp"
#include "GraphicsAccessories.hpp"
#include "FastRand.hpp"
#include "MapHelper.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

BufferDesc GetSparseBuffDesc(const char* Name, USAGE Usage, Uint64 Size)
{
    BufferDesc BuffDesc;
    BuffDesc.Name              = Name;
    BuffDesc.Usage             = Usage;
    BuffDesc.BindFlags         = BIND_SHADER_RESOURCE;
    BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = 16;
    BuffDesc.Size              = Size;
    return BuffDesc;
}

class DynamicBufferCreateTest : public testing::TestWithParam<USAGE>
{
};

TEST_P(DynamicBufferCreateTest, Run)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    const auto  Usage      = GetParam();

    if (Usage == USAGE_SPARSE)
    {
        if (!DeviceInfo.Features.SparseResources)
            GTEST_SKIP() << "Sparse resources are not enabled on this device";

        const auto& AdapterInfo = pDevice->GetAdapterInfo();
        if ((AdapterInfo.SparseResources.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
            GTEST_SKIP() << "This device does not support sparse buffers";
    }


    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer create test 0", Usage, 0);
        DynamicBuffer DynBuff{pDevice, CI};
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_STREQ(DynBuff.GetDesc().Name, BuffDesc.Name);
        EXPECT_FALSE(DynBuff.PendingUpdate());

        auto* pBuffer = DynBuff.GetBuffer(nullptr, nullptr);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        if (Usage == USAGE_SPARSE)
            EXPECT_NE(pBuffer, nullptr);
        else
            EXPECT_EQ(pBuffer, nullptr);
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer create test 1", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};
        if (BuffDesc.Usage == USAGE_SPARSE)
            BuffDesc.Size = 0;
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_EQ(DynBuff.PendingUpdate(), BuffDesc.Usage == USAGE_SPARSE);

        auto* pBuffer = DynBuff.GetBuffer(nullptr, BuffDesc.Usage == USAGE_SPARSE ? pContext : nullptr);
        ASSERT_NE(pBuffer, nullptr);
        BuffDesc.Size = 256 << 10;
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        if (BuffDesc.Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer create test 2", Usage, 256 << 10);
        DynamicBuffer DynBuff{nullptr, CI};
        BuffDesc.Size = 0;
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_TRUE(DynBuff.PendingUpdate());

        auto* pBuffer = DynBuff.GetBuffer(pDevice, BuffDesc.Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        ASSERT_NE(pBuffer, nullptr);
        BuffDesc.Size = 256 << 10;
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        if (BuffDesc.Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
    }
}

std::string GetTestName(const testing::TestParamInfo<USAGE>& info)
{
    return GetUsageString(info.param);
}

INSTANTIATE_TEST_SUITE_P(DynamicBuffer,
                         DynamicBufferCreateTest,
                         testing::Values<USAGE>(USAGE_DEFAULT, USAGE_SPARSE),
                         GetTestName); //


class DynamicBufferResizeTest : public testing::TestWithParam<USAGE>
{
};

TEST_P(DynamicBufferResizeTest, Run)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    const auto  Usage      = GetParam();

    if (Usage == USAGE_SPARSE)
    {
        if (!DeviceInfo.Features.SparseResources)
            GTEST_SKIP() << "Sparse resources are not enabled on this device";

        const auto& AdapterInfo = pDevice->GetAdapterInfo();
        if ((AdapterInfo.SparseResources.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
            GTEST_SKIP() << "This device does not support sparse buffers";
    }

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    FastRandInt rnd{0, 0, 255};

    constexpr Uint32   MaxSize = 1024 << 10;
    std::vector<Uint8> RefData(MaxSize);
    for (auto& Val : RefData)
        Val = rnd() & 0xFF;

    RefCntAutoPtr<IBuffer> pStagingBuff;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Staging buffer for dynamic buffer test";
        BuffDesc.Usage          = USAGE_STAGING;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
        BuffDesc.BindFlags      = BIND_NONE;
        BuffDesc.Size           = MaxSize;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pStagingBuff);
        ASSERT_NE(pStagingBuff, nullptr);
    }

    auto UpdateBuffer = [&](IBuffer* pBuffer, Uint64 Offset, Uint64 Size) //
    {
        VERIFY_EXPR(Offset + Size <= RefData.size());
        pContext->UpdateBuffer(pBuffer, Offset, Size, &RefData[static_cast<size_t>(Offset)], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    };

    auto VerifyBuffer = [&](IBuffer* pBuffer, Uint64 Offset, Uint64 Size) //
    {
        VERIFY_EXPR(Size > 0);
        VERIFY_EXPR(Offset + Size <= RefData.size());
        pContext->CopyBuffer(pBuffer, Offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pStagingBuff, Offset, Size, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->WaitForIdle();
        void* pData = nullptr;
        pContext->MapBuffer(pStagingBuff, MAP_READ, MAP_FLAG_DO_NOT_WAIT, pData);
        bool IsEqual = (memcmp(reinterpret_cast<const Uint8*>(pData) + Offset, &RefData[static_cast<size_t>(Offset)], static_cast<size_t>(Size)) == 0);
        pContext->UnmapBuffer(pStagingBuff, MAP_READ);
        return IsEqual;
    };

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 0", Usage, 256 << 10);
        DynamicBuffer DynBuff{nullptr, CI};
        EXPECT_TRUE(DynBuff.PendingUpdate());

        BuffDesc.Size = 512 << 10;
        DynBuff.Resize(nullptr, nullptr, BuffDesc.Size);
        EXPECT_TRUE(DynBuff.PendingUpdate());

        auto* pBuffer = DynBuff.GetBuffer(pDevice, Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_EQ(DynBuff.GetVersion(), Uint32{1});
        EXPECT_FALSE(DynBuff.PendingUpdate());
        ASSERT_NE(pBuffer, nullptr);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);

        UpdateBuffer(pBuffer, 0, BuffDesc.Size);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 1", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};
        EXPECT_EQ(DynBuff.PendingUpdate(), Usage == USAGE_SPARSE);

        DynBuff.Resize(nullptr, nullptr, 1024 << 10);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_TRUE(DynBuff.PendingUpdate());

        DynBuff.Resize(pDevice, nullptr, 512 << 10);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_TRUE(DynBuff.PendingUpdate());

        auto* pBuffer = DynBuff.GetBuffer(nullptr, pContext);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        BuffDesc.Size = 512 << 10;
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        ASSERT_NE(pBuffer, nullptr);

        UpdateBuffer(pBuffer, 0, BuffDesc.Size);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 2", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};

        auto* pBuffer = DynBuff.GetBuffer(nullptr, Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_NE(pBuffer, nullptr);
        UpdateBuffer(pBuffer, 0, BuffDesc.Size);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));

        BuffDesc.Size = 512 << 10;
        DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        EXPECT_EQ(DynBuff.GetVersion(), Usage == USAGE_SPARSE ? 1u : 2u);
        pBuffer = DynBuff.GetBuffer(nullptr, Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_NE(pBuffer, nullptr);
        UpdateBuffer(pBuffer, 256 << 10, 256 << 10);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 256 << 10, 256 << 10));

        pBuffer = DynBuff.GetBuffer(nullptr, nullptr);
        ASSERT_NE(pBuffer, nullptr);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);

        BuffDesc.Size = 128 << 10;
        DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        EXPECT_EQ(DynBuff.GetVersion(), Usage == USAGE_SPARSE ? 1u : 3u);

        pBuffer = DynBuff.GetBuffer(nullptr, Usage == USAGE_SPARSE ? pContext : nullptr);
        ASSERT_NE(pBuffer, nullptr);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer->GetDesc(), BuffDesc);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);

        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, 128 << 10));
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 3", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};
        EXPECT_EQ(DynBuff.PendingUpdate(), Usage == USAGE_SPARSE);

        DynBuff.Resize(pDevice, nullptr, 1024 << 10);
        EXPECT_TRUE(DynBuff.PendingUpdate());
        EXPECT_EQ(DynBuff.GetVersion(), Usage == USAGE_SPARSE ? 1u : 2u);

        DynBuff.Resize(nullptr, pContext, BuffDesc.Size);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        EXPECT_EQ(DynBuff.GetVersion(), Usage == USAGE_SPARSE ? 1u : 2u);

        auto* pBuffer = DynBuff.GetBuffer(nullptr, Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_FALSE(DynBuff.PendingUpdate());
        ASSERT_NE(pBuffer, nullptr);
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 4", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};
        EXPECT_NE(DynBuff.GetBuffer(nullptr, Usage == USAGE_SPARSE ? pContext : nullptr), nullptr);

        DynBuff.Resize(nullptr, nullptr, 1024 << 10);

        BuffDesc.Size = 0;
        DynBuff.Resize(nullptr, nullptr, BuffDesc.Size);
        EXPECT_EQ(DynBuff.PendingUpdate(), Usage == USAGE_SPARSE);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);

        auto* pBuffer = DynBuff.GetBuffer(pDevice, Usage == USAGE_SPARSE ? pContext : nullptr);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer, nullptr);

        DynBuff.Resize(pDevice, pContext, 512 << 10);
        EXPECT_FALSE(DynBuff.PendingUpdate());

        DynBuff.Resize(pDevice, nullptr, 1024 << 10);
        DynBuff.Resize(nullptr, nullptr, 0);
        EXPECT_EQ(DynBuff.PendingUpdate(), Usage == USAGE_SPARSE);
        pBuffer = DynBuff.GetBuffer(pDevice, Usage == USAGE_SPARSE ? pContext : nullptr);
        if (Usage != USAGE_SPARSE)
            EXPECT_EQ(pBuffer, nullptr);
    }

    {
        DynamicBufferCreateInfo CI;

        auto& BuffDesc{CI.Desc};
        BuffDesc = GetSparseBuffDesc("Dynamic buffer resize test 5", Usage, 256 << 10);
        DynamicBuffer DynBuff{pDevice, CI};

        auto* pBuffer = DynBuff.GetBuffer(pDevice, pContext);
        ASSERT_NE(pBuffer, nullptr);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        UpdateBuffer(pBuffer, 0, BuffDesc.Size);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));

        BuffDesc.Size = 512 << 10;
        pBuffer       = DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        ASSERT_NE(pBuffer, nullptr);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        UpdateBuffer(pBuffer, 256 << 10, 256 << 10);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));

        BuffDesc.Size = 1024 << 10;
        pBuffer       = DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        ASSERT_NE(pBuffer, nullptr);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        UpdateBuffer(pBuffer, 512 << 10, 512 << 10);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));

        BuffDesc.Size = 4096 << 10;
        pBuffer       = DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        ASSERT_NE(pBuffer, nullptr);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        BuffDesc.Size = 1024 << 10;
        pBuffer       = DynBuff.Resize(pDevice, pContext, BuffDesc.Size);
        ASSERT_NE(pBuffer, nullptr);
        EXPECT_EQ(DynBuff.GetDesc(), BuffDesc);
        EXPECT_TRUE(VerifyBuffer(pBuffer, 0, BuffDesc.Size));
    }
}

INSTANTIATE_TEST_SUITE_P(DynamicBuffer,
                         DynamicBufferResizeTest,
                         testing::Values<USAGE>(USAGE_DEFAULT, USAGE_SPARSE),
                         GetTestName); //

} // namespace
