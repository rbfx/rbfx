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

#include <vector>

#include "DynamicTextureArray.hpp"
#include "GPUTestingEnvironment.hpp"
#include "GraphicsAccessories.hpp"
#include "FastRand.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class DynamicTextureArrayCreateTest : public testing::TestWithParam<std::tuple<USAGE, TEXTURE_FORMAT>>
{
};

TEST_P(DynamicTextureArrayCreateTest, Run)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    if (pDevice->GetDeviceInfo().IsMetalDevice())
        GTEST_SKIP() << "This test is currently disabled on Metal";

    const auto& TestInfo = GetParam();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    DynamicTextureArrayCreateInfo DynTexArrCI;
    DynTexArrCI.NumSlicesInMemoryPage = 2;

    auto& Desc{DynTexArrCI.Desc};
    Desc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    Desc.BindFlags = BIND_SHADER_RESOURCE;
    Desc.Width     = 1024;
    Desc.Height    = 1024;
    Desc.MipLevels = 0;
    Desc.Usage     = std::get<0>(TestInfo);
    Desc.Format    = std::get<1>(TestInfo);
    Desc.ArraySize = 0;

    if (Desc.Usage == USAGE_SPARSE)
    {
        const auto& DeviceInfo = pDevice->GetDeviceInfo();
        if (!DeviceInfo.Features.SparseResources)
            GTEST_SKIP() << "Sparse resources are not enabled on this device";

        const auto& AdapterInfo = pDevice->GetAdapterInfo();
        if ((AdapterInfo.SparseResources.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
            GTEST_SKIP() << "This device does not support sparse texture 2D arrays with mip tails";
    }

    Desc.Name = "Dynamic texture array create test 1";
    {
        auto pDynTexArray = std::make_unique<DynamicTextureArray>(nullptr, DynTexArrCI);
        ASSERT_NE(pDynTexArray, nullptr);

        EXPECT_FALSE(pDynTexArray->PendingUpdate());
        auto* pTexture = pDynTexArray->GetTexture(nullptr, nullptr);
        EXPECT_EQ(pTexture, nullptr);
    }

    Desc.Name      = "Dynamic texture array create test 2";
    Desc.ArraySize = 1;
    {
        auto pDynTexArray = std::make_unique<DynamicTextureArray>(nullptr, DynTexArrCI);
        ASSERT_NE(pDynTexArray, nullptr);

        EXPECT_TRUE(pDynTexArray->PendingUpdate());
        auto* pTexture = pDynTexArray->GetTexture(pDevice, Desc.Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_NE(pTexture, nullptr);
    }

    Desc.Name = "Dynamic texture array create test 3";
    {
        auto pDynTexArray = std::make_unique<DynamicTextureArray>(pDevice, DynTexArrCI);
        ASSERT_NE(pDynTexArray, nullptr);

        EXPECT_EQ(pDynTexArray->PendingUpdate(), Desc.Usage == USAGE_SPARSE);
        auto* pTexture = pDynTexArray->GetTexture(nullptr, Desc.Usage == USAGE_SPARSE ? pContext : nullptr);
        EXPECT_NE(pTexture, nullptr);
    }
}

std::string GetTestName(const testing::TestParamInfo<std::tuple<USAGE, TEXTURE_FORMAT>>& info)
{
    return std::string{GetUsageString(std::get<0>(info.param))} + "__" + GetTextureFormatAttribs(std::get<1>(info.param)).Name;
}

INSTANTIATE_TEST_SUITE_P(DynamicTextureArray,
                         DynamicTextureArrayCreateTest,
                         testing::Combine(
                             testing::Values<USAGE>(USAGE_DEFAULT, USAGE_SPARSE),
                             testing::Values<TEXTURE_FORMAT>(TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_BC1_UNORM_SRGB)),
                         GetTestName); //




class DynamicTextureArrayResizeTest : public testing::TestWithParam<std::tuple<USAGE, TEXTURE_FORMAT>>
{
};

TEST_P(DynamicTextureArrayResizeTest, Run)
{
    auto* pEnv     = GPUTestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    const auto& TestInfo   = GetParam();
    const auto  Usage      = std::get<0>(TestInfo);
    const auto  Format     = std::get<1>(TestInfo);

    if (Usage == USAGE_SPARSE)
    {
        if (!DeviceInfo.Features.SparseResources)
            GTEST_SKIP() << "Sparse resources are not enabled on this device";

        const auto& AdapterInfo = pDevice->GetAdapterInfo();
        if ((AdapterInfo.SparseResources.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL) == 0)
            GTEST_SKIP() << "This device does not support sparse texture 2D arrays with mip tails";
    }

    if (DeviceInfo.IsMetalDevice())
        GTEST_SKIP() << "This test is currently disabled on Metal";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    DynamicTextureArrayCreateInfo DynTexArrCI;
    DynTexArrCI.NumSlicesInMemoryPage = 2;

    auto& Desc{DynTexArrCI.Desc};
    Desc.Name      = "Dynamic texture array resize test";
    Desc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    Desc.BindFlags = BIND_SHADER_RESOURCE;
    Desc.Width     = 1024;
    Desc.Height    = 1024;
    Desc.MipLevels = 11;
    Desc.Usage     = Usage;
    Desc.Format    = Format;
    Desc.ArraySize = 0;

    constexpr Uint32 NumTestSlices = 6;

    RefCntAutoPtr<ITexture> pStagingTex;
    {
        auto StagingTexDesc           = Desc;
        StagingTexDesc.Name           = "Dynamice texture array staging texture";
        StagingTexDesc.BindFlags      = BIND_NONE;
        StagingTexDesc.Usage          = USAGE_STAGING;
        StagingTexDesc.CPUAccessFlags = CPU_ACCESS_READ;
        StagingTexDesc.ArraySize      = NumTestSlices;
        pDevice->CreateTexture(StagingTexDesc, nullptr, &pStagingTex);
        ASSERT_NE(pStagingTex, nullptr);
    }

    FastRandInt rnd{0, 0, 255};

    std::vector<std::vector<Uint8>> RefData(NumTestSlices * Desc.MipLevels);
    for (Uint32 slice = 0; slice < NumTestSlices; ++slice)
    {
        for (Uint32 mip = 0; mip < Desc.MipLevels; ++mip)
        {
            auto&      MipData    = RefData[slice * Desc.MipLevels + mip];
            const auto MipAttribs = GetMipLevelProperties(Desc, mip);
            MipData.resize(static_cast<size_t>(MipAttribs.MipSize));
            for (auto& texel : MipData)
                texel = rnd() & 0xFF;
        }
    }

    auto UpdateSlice = [&RefData, &Desc](IDeviceContext* pCtx, ITexture* pTex, Uint32 Slice) //
    {
        for (Uint32 mip = 0; mip < Desc.MipLevels; ++mip)
        {
            const auto& MipData    = RefData[Slice * Desc.MipLevels + mip];
            const auto  MipAttribs = GetMipLevelProperties(Desc, mip);

            TextureSubResData SubResData{MipData.data(), MipAttribs.RowSize};
            pCtx->UpdateTexture(pTex, mip, Slice, Box{0, MipAttribs.LogicalWidth, 0, MipAttribs.LogicalHeight, 0, 1}, SubResData,
                                RESOURCE_STATE_TRANSITION_MODE_NONE, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
    };

    auto VerifySlices = [&RefData, &Desc, &pStagingTex, IsGL = DeviceInfo.IsGLDevice()](IDeviceContext* pCtx, ITexture* pSrcTex, Uint32 FirstSlice, Uint32 NumSlices) //
    {
        const auto FmtAttribs = GetTextureFormatAttribs(Desc.Format);
        if (IsGL && FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            // Copying to compressed staging textures is not supported in GL
            return;
        }

        for (Uint32 slice = FirstSlice; slice < FirstSlice + NumSlices; ++slice)
        {
            for (Uint32 mip = 0; mip < Desc.MipLevels; ++mip)
            {
                CopyTextureAttribs CopyAttribs{pSrcTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pStagingTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION};
                CopyAttribs.SrcSlice    = slice;
                CopyAttribs.SrcMipLevel = mip;
                CopyAttribs.DstSlice    = slice;
                CopyAttribs.DstMipLevel = mip;
                pCtx->CopyTexture(CopyAttribs);
            }
        }

        pCtx->WaitForIdle();

        for (Uint32 slice = FirstSlice; slice < FirstSlice + NumSlices; ++slice)
        {
            for (Uint32 mip = 0; mip < Desc.MipLevels; ++mip)
            {
                const auto& RefMipData = RefData[slice * Desc.MipLevels + mip];

                MappedTextureSubresource MappedSubres;
                pCtx->MapTextureSubresource(pStagingTex, mip, slice, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, MappedSubres);

                bool       DataOK     = true;
                const auto MipAttribs = GetMipLevelProperties(Desc, mip);
                for (Uint32 row = 0; row < MipAttribs.StorageHeight / FmtAttribs.BlockHeight; ++row)
                {
                    const auto* pSrcRow = reinterpret_cast<const Uint8*>(MappedSubres.pData) + row * MappedSubres.Stride;
                    const auto* pRefRow = &RefMipData[static_cast<size_t>(row * MipAttribs.RowSize)];

                    if (memcmp(pSrcRow, pRefRow, static_cast<size_t>(MipAttribs.RowSize)) != 0)
                        DataOK = false;
                }
                EXPECT_TRUE(DataOK) << "Slice: " << slice << ", Mip: " << mip;

                pCtx->UnmapTextureSubresource(pStagingTex, mip, slice);
            }
        }
    };

    auto pDynTexArray = std::make_unique<DynamicTextureArray>(pDevice, DynTexArrCI);
    ASSERT_NE(pDynTexArray, nullptr);

    pDynTexArray->Resize(pDevice, nullptr, 1);
    EXPECT_EQ(pDynTexArray->PendingUpdate(), Desc.Usage == USAGE_SPARSE);
    auto* pTexture = pDynTexArray->GetTexture(pDevice, pContext);
    EXPECT_NE(pTexture, nullptr);
    UpdateSlice(pContext, pTexture, 0);
    VerifySlices(pContext, pTexture, 0, 1);

    pDynTexArray->Resize(pDevice, pContext, 2);
    pTexture = pDynTexArray->GetTexture(nullptr, nullptr);
    UpdateSlice(pContext, pTexture, 1);
    VerifySlices(pContext, pTexture, 1, 1);


    pDynTexArray->Resize(pDevice, nullptr, 16);
    pTexture = pDynTexArray->GetTexture(pDevice, pContext);
    UpdateSlice(pContext, pTexture, 2);
    VerifySlices(pContext, pTexture, 2, 1);

    pDynTexArray->Resize(pDevice, nullptr, 9);
    pTexture = pDynTexArray->GetTexture(pDevice, pContext);
    UpdateSlice(pContext, pTexture, 3);
    UpdateSlice(pContext, pTexture, 4);
    UpdateSlice(pContext, pTexture, 5);

    VerifySlices(pContext, pTexture, 0, NumTestSlices);

    pDynTexArray->Resize(nullptr, nullptr, 0);
    pTexture = pDynTexArray->GetTexture(nullptr, pContext);
    if (Desc.Usage != USAGE_SPARSE)
        EXPECT_EQ(pTexture, nullptr);
}


INSTANTIATE_TEST_SUITE_P(DynamicTextureArray,
                         DynamicTextureArrayResizeTest,
                         testing::Combine(
                             testing::Values<USAGE>(USAGE_DEFAULT, USAGE_SPARSE),
                             testing::Values<TEXTURE_FORMAT>(TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_BC1_UNORM_SRGB)),
                         GetTestName); //

} // namespace
