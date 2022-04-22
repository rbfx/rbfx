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

#include <array>
#include <unordered_set>

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "GraphicsAccessories.hpp"
#include "ArchiveMemoryImpl.hpp"
#include "Dearchiver.h"
#include "SerializedPipelineState.h"
#include "ShaderMacroHelper.hpp"
#include "DataBlobImpl.hpp"
#include "MemoryFileStream.hpp"

#include "ResourceLayoutTestCommon.hpp"
#include "gtest/gtest.h"

#include "InlineShaders/RayTracingTestHLSL.h"
#include "RayTracingTestConstants.hpp"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

constexpr ARCHIVE_DEVICE_DATA_FLAGS GetDeviceBits()
{
    ARCHIVE_DEVICE_DATA_FLAGS DeviceBits = ARCHIVE_DEVICE_DATA_FLAG_NONE;
#if D3D11_SUPPORTED
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_D3D11;
#endif
#if D3D12_SUPPORTED
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_D3D12;
#endif
#if GL_SUPPORTED
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_GL;
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_GLES;
#endif
#if VULKAN_SUPPORTED
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_VULKAN;
#endif
#if METAL_SUPPORTED
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS;
    DeviceBits = DeviceBits | ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS;
#endif
    return DeviceBits;
}

void ArchivePRS(RefCntAutoPtr<IArchive>&                   pSource,
                const char*                                PRS1Name,
                const char*                                PRS2Name,
                RefCntAutoPtr<IPipelineResourceSignature>& pRefPRS_1,
                RefCntAutoPtr<IPipelineResourceSignature>& pRefPRS_2,
                ARCHIVE_DEVICE_DATA_FLAGS                  DeviceBits)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    SerializationDeviceCreateInfo       DeviceCI;
    pArchiverFactory->CreateSerializationDevice(DeviceCI, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    RefCntAutoPtr<IArchiver> pArchiver;
    pArchiverFactory->CreateArchiver(pSerializationDevice, &pArchiver);
    ASSERT_NE(pArchiver, nullptr);

    // PRS 1
    {
        constexpr auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        constexpr PipelineResourceDesc Resources[] = //
            {
                {SHADER_TYPE_ALL_GRAPHICS, "g_Tex2D_1", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "g_Tex2D_2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "ConstBuff_1", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "ConstBuff_2", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType}, //
            };

        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name         = PRS1Name;
        PRSDesc.BindingIndex = 0;
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);

        constexpr ImmutableSamplerDesc ImmutableSamplers[] = //
            {
                {SHADER_TYPE_ALL_GRAPHICS, "g_Tex2D_1_sampler", SamplerDesc{}},
                {SHADER_TYPE_ALL_GRAPHICS, "g_Sampler", SamplerDesc{}} //
            };
        PRSDesc.ImmutableSamplers    = ImmutableSamplers;
        PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

        for (Uint32 i = 0; i < 3; ++i)
        {
            ResourceSignatureArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = DeviceBits;
            RefCntAutoPtr<IPipelineResourceSignature> pSerializedPRS;
            pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ArchiveInfo, &pSerializedPRS);
            ASSERT_NE(pSerializedPRS, nullptr);
            ASSERT_TRUE(pArchiver->AddPipelineResourceSignature(pSerializedPRS));
        }

        pDevice->CreatePipelineResourceSignature(PRSDesc, &pRefPRS_1);
        ASSERT_NE(pRefPRS_1, nullptr);
    }

    // PRS 2
    {
        constexpr auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

        constexpr PipelineResourceDesc Resources[] = //
            {
                {SHADER_TYPE_COMPUTE, "g_RWTex2D", 2, SHADER_RESOURCE_TYPE_TEXTURE_UAV, VarType},
                {SHADER_TYPE_COMPUTE, "ConstBuff", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType}, //
            };

        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name         = PRS2Name;
        PRSDesc.BindingIndex = 2;
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);

        for (Uint32 i = 0; i < 3; ++i)
        {
            ResourceSignatureArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = DeviceBits;
            RefCntAutoPtr<IPipelineResourceSignature> pSerializedPRS;
            pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ArchiveInfo, &pSerializedPRS);
            ASSERT_NE(pSerializedPRS, nullptr);
            ASSERT_TRUE(pArchiver->AddPipelineResourceSignature(pSerializedPRS));
        }

        pDevice->CreatePipelineResourceSignature(PRSDesc, &pRefPRS_2);
        ASSERT_NE(pRefPRS_2, nullptr);
    }

    RefCntAutoPtr<IDataBlob> pBlob;
    pArchiver->SerializeToBlob(&pBlob);
    ASSERT_NE(pBlob, nullptr);

    pSource = RefCntAutoPtr<IArchive>{MakeNewRCObj<ArchiveMemoryImpl>{}(pBlob)};
}

void UnpackPRS(IArchive*                   pSource,
               const char*                 PRS1Name,
               const char*                 PRS2Name,
               IPipelineResourceSignature* pRefPRS_1,
               IPipelineResourceSignature* pRefPRS_2)
{
    auto* pEnv        = GPUTestingEnvironment::GetInstance();
    auto* pDevice     = pEnv->GetDevice();
    auto* pDearchiver = pDevice->GetEngineFactory()->GetDearchiver();

    RefCntAutoPtr<IDeviceObjectArchive> pArchive;
    pDearchiver->CreateDeviceObjectArchive(pSource, &pArchive);
    ASSERT_NE(pArchive, nullptr);

    // Unpack PRS 1
    {
        ResourceSignatureUnpackInfo UnpackInfo;
        UnpackInfo.Name                     = PRS1Name;
        UnpackInfo.pArchive                 = pArchive;
        UnpackInfo.pDevice                  = pDevice;
        UnpackInfo.SRBAllocationGranularity = 10;

        if (pRefPRS_1 == nullptr)
            GPUTestingEnvironment::SetErrorAllowance(1);

        RefCntAutoPtr<IPipelineResourceSignature> pUnpackedPRS;
        pDearchiver->UnpackResourceSignature(UnpackInfo, &pUnpackedPRS);

        if (pRefPRS_1 != nullptr)
        {
            ASSERT_NE(pUnpackedPRS, nullptr);
            EXPECT_TRUE(pUnpackedPRS->IsCompatibleWith(pRefPRS_1));
        }
        else
        {
            EXPECT_EQ(pUnpackedPRS, nullptr);
        }
    }

    // Unpack PRS 2
    {
        ResourceSignatureUnpackInfo UnpackInfo;
        UnpackInfo.Name                     = PRS2Name;
        UnpackInfo.pArchive                 = pArchive;
        UnpackInfo.pDevice                  = pDevice;
        UnpackInfo.SRBAllocationGranularity = 10;

        if (pRefPRS_2 == nullptr)
            TestingEnvironment::SetErrorAllowance(1);

        RefCntAutoPtr<IPipelineResourceSignature> pUnpackedPRS;
        pDearchiver->UnpackResourceSignature(UnpackInfo, &pUnpackedPRS);

        if (pRefPRS_2 != nullptr)
        {
            ASSERT_NE(pUnpackedPRS, nullptr);
            EXPECT_TRUE(pUnpackedPRS->IsCompatibleWith(pRefPRS_2));
        }
        else
        {
            EXPECT_EQ(pUnpackedPRS, nullptr);
        }
    }
}


TEST(ArchiveTest, ResourceSignature)
{
    constexpr char PRS1Name[] = "ArchiveTest.ResourceSignature - PRS 1";
    constexpr char PRS2Name[] = "ArchiveTest.ResourceSignature - PRS 2";

    RefCntAutoPtr<IArchive>                   pArchive;
    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_1;
    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_2;
    ArchivePRS(pArchive, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2, GetDeviceBits());
    UnpackPRS(pArchive, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2);
}


TEST(ArchiveTest, RemoveDeviceData)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    const auto CurrentDeviceFlag = static_cast<ARCHIVE_DEVICE_DATA_FLAGS>(1u << pDevice->GetDeviceInfo().Type);
    const auto AllDeviceFlags    = GetDeviceBits();

    if ((AllDeviceFlags & ~CurrentDeviceFlag) == 0)
        GTEST_SKIP() << "Test requires support for at least 2 backends";

    constexpr char PRS1Name[] = "ArchiveTest.RemoveDeviceData - PRS 1";
    constexpr char PRS2Name[] = "ArchiveTest.RemoveDeviceData - PRS 2";

    RefCntAutoPtr<IArchive> pArchive1;
    {
        RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_1;
        RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_2;
        ArchivePRS(pArchive1, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2, AllDeviceFlags);
        UnpackPRS(pArchive1, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2);
    }

    {
        auto pDataBlob  = DataBlobImpl::Create(0);
        auto pMemStream = MemoryFileStream::Create(pDataBlob);

        ASSERT_TRUE(pArchiverFactory->RemoveDeviceData(pArchive1, CurrentDeviceFlag, pMemStream));

        RefCntAutoPtr<IArchive> pArchive2{MakeNewRCObj<ArchiveMemoryImpl>{}(pDataBlob)};

        // PRS creation must fail
        UnpackPRS(pArchive2, PRS1Name, PRS2Name, nullptr, nullptr);
    }
}


TEST(ArchiveTest, AppendDeviceData)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    const auto CurrentDeviceFlag = static_cast<ARCHIVE_DEVICE_DATA_FLAGS>(1u << pDevice->GetDeviceInfo().Type);
    auto       AllDeviceFlags    = GetDeviceBits() & ~CurrentDeviceFlag;
    // OpenGL and GLES use the same device-specific data.
    // When one is removed, the other is removed too.
    if (CurrentDeviceFlag == ARCHIVE_DEVICE_DATA_FLAG_GLES)
        AllDeviceFlags &= ~ARCHIVE_DEVICE_DATA_FLAG_GL;
    else if (CurrentDeviceFlag == ARCHIVE_DEVICE_DATA_FLAG_GL)
        AllDeviceFlags &= ~ARCHIVE_DEVICE_DATA_FLAG_GLES;

    if (AllDeviceFlags == 0)
        GTEST_SKIP() << "Test requires support for at least 2 backends";

    constexpr char PRS1Name[] = "ArchiveTest.AppendDeviceData - PRS 1";
    constexpr char PRS2Name[] = "ArchiveTest.AppendDeviceData - PRS 2";

    RefCntAutoPtr<IArchive> pArchive;
    for (; AllDeviceFlags != 0;)
    {
        const auto DeviceFlag = ExtractLSB(AllDeviceFlags);

        RefCntAutoPtr<IArchive>                   pArchive2;
        RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_1;
        RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_2;
        ArchivePRS(pArchive2, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2, DeviceFlag);
        // PRS creation must fail
        UnpackPRS(pArchive2, PRS1Name, PRS2Name, nullptr, nullptr);

        if (pArchive != nullptr)
        {
            auto pDataBlob  = DataBlobImpl::Create(0);
            auto pMemStream = MemoryFileStream::Create(pDataBlob);

            // pArchive  - without DeviceFlag
            // pArchive2 - with DeviceFlag
            ASSERT_TRUE(pArchiverFactory->AppendDeviceData(pArchive, DeviceFlag, pArchive2, pMemStream));

            pArchive = RefCntAutoPtr<IArchive>{MakeNewRCObj<ArchiveMemoryImpl>{}(pDataBlob)};
        }
        else
        {
            pArchive = pArchive2;
        }
    }

    RefCntAutoPtr<IArchive>                   pArchive3;
    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_1;
    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS_2;
    ArchivePRS(pArchive3, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2, CurrentDeviceFlag);

    // Append device data
    {
        auto pDataBlob  = DataBlobImpl::Create(0);
        auto pMemStream = MemoryFileStream::Create(pDataBlob);

        // pArchive  - without CurrentDeviceFlag
        // pArchive3 - with CurrentDeviceFlag
        ASSERT_TRUE(pArchiverFactory->AppendDeviceData(pArchive, CurrentDeviceFlag, pArchive3, pMemStream));

        pArchive = RefCntAutoPtr<IArchive>{MakeNewRCObj<ArchiveMemoryImpl>{}(pDataBlob)};
        UnpackPRS(pArchive, PRS1Name, PRS2Name, pRefPRS_1, pRefPRS_2);
    }
}


class TestBrokenShader : public testing::TestWithParam<ARCHIVE_DEVICE_DATA_FLAGS>
{};

TEST_P(TestBrokenShader, CompileFailure)
{
    auto DataFlag    = GetParam();
    auto AllowedBits = GetDeviceBits();
    if ((DataFlag & AllowedBits) == 0)
        GTEST_SKIP() << GetArchiveDeviceDataFlagString(DataFlag) << " is not supported by archiver";

    if ((DataFlag & (ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS | ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS)) != 0)
        GTEST_SKIP() << "In Metal shaders are compiled when PSO is created";

    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCreateInfo{}, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pArchiverFactory->CreateDefaultShaderSourceStreamFactory("shaders/Archiver", &pShaderSourceFactory);

    ShaderCreateInfo ShaderCI;
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.Desc.ShaderType            = SHADER_TYPE_VERTEX;
    ShaderCI.EntryPoint                 = "main";
    ShaderCI.Desc.Name                  = "Archive test broken shader";
    ShaderCI.Source                     = "Not even a shader source";

    RefCntAutoPtr<IDataBlob> pCompilerOutput;
    ShaderCI.ppCompilerOutput = pCompilerOutput.RawDblPtr();

    const auto IsD3D = DataFlag == ARCHIVE_DEVICE_DATA_FLAG_D3D11 || DataFlag == ARCHIVE_DEVICE_DATA_FLAG_D3D12;
    pEnv->SetErrorAllowance(IsD3D ? 2 : 3, "No worries, errors are expected: testing broken shader\n");
    pEnv->PushExpectedErrorSubstring("Failed to create Shader object 'Archive test broken shader'");
    pEnv->PushExpectedErrorSubstring("Failed to compile shader 'Archive test broken shader'", false);
    if (!IsD3D)
        pEnv->PushExpectedErrorSubstring("Failed to parse shader source", false);

    RefCntAutoPtr<IShader> pSerializedShader;
    pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{DataFlag}, &pSerializedShader);
    EXPECT_EQ(pSerializedShader, nullptr);
    EXPECT_NE(pCompilerOutput, nullptr);
}

TEST_P(TestBrokenShader, MissingSourceFile)
{
    auto DataFlag    = GetParam();
    auto AllowedBits = GetDeviceBits();
    if ((DataFlag & AllowedBits) == 0)
        GTEST_SKIP() << GetArchiveDeviceDataFlagString(DataFlag) << " is not supported by archiver";

    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCreateInfo{}, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pArchiverFactory->CreateDefaultShaderSourceStreamFactory("shaders/Archiver", &pShaderSourceFactory);

    ShaderCreateInfo ShaderCI;
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.Desc.ShaderType            = SHADER_TYPE_VERTEX;
    ShaderCI.EntryPoint                 = "main";
    ShaderCI.Desc.Name                  = "Archive test broken shader";
    ShaderCI.FilePath                   = "non_existing.shader";
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    pEnv->SetErrorAllowance(3, "No worries, errors are expected: testing broken shader\n");
    pEnv->PushExpectedErrorSubstring("Failed to create Shader object 'Archive test broken shader'");
    pEnv->PushExpectedErrorSubstring("Failed to load shader source file 'non_existing.shader'", false);
    pEnv->PushExpectedErrorSubstring("Failed to create input stream for source file non_existing.shader", false);

    RefCntAutoPtr<IShader> pSerializedShader;
    pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{DataFlag}, &pSerializedShader);
    EXPECT_EQ(pSerializedShader, nullptr);
}

static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == 128, "Please add new device flag to the map");
INSTANTIATE_TEST_SUITE_P(ArchiveTest,
                         TestBrokenShader,
                         testing::Values<ARCHIVE_DEVICE_DATA_FLAGS>(
                             ARCHIVE_DEVICE_DATA_FLAG_D3D11,
                             ARCHIVE_DEVICE_DATA_FLAG_D3D12,
                             ARCHIVE_DEVICE_DATA_FLAG_GL,
                             ARCHIVE_DEVICE_DATA_FLAG_GLES,
                             ARCHIVE_DEVICE_DATA_FLAG_VULKAN,
                             ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS,
                             ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS),
                         [](const testing::TestParamInfo<ARCHIVE_DEVICE_DATA_FLAGS>& info) //
                         {
                             return GetArchiveDeviceDataFlagString(info.param);
                         });

void CreateTestRenderPass1(IRenderDevice*        pDevice,
                           ISerializationDevice* pSerializationDevice,
                           ISwapChain*           pSwapChain,
                           const char*           RPName,
                           IRenderPass**         ppRenderPass,
                           IRenderPass**         ppSerializedRP)
{
    auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
    ASSERT_NE(pRTV, nullptr);
    const auto& RTVDesc = pRTV->GetTexture()->GetDesc();

    RenderPassAttachmentDesc Attachments[1];
    Attachments[0].Format       = RTVDesc.Format;
    Attachments[0].SampleCount  = static_cast<Uint8>(RTVDesc.SampleCount);
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1] = {};

    Subpasses[0].RenderTargetAttachmentCount = 1;
    AttachmentReference RTAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
    Subpasses[0].pRenderTargetAttachments = &RTAttachmentRef;

    RenderPassDesc RPDesc;
    RPDesc.Name            = RPName;
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    if (ppRenderPass != nullptr)
    {
        pDevice->CreateRenderPass(RPDesc, ppRenderPass);
        ASSERT_NE(*ppRenderPass, nullptr);
    }

    if (ppSerializedRP != nullptr)
    {
        pSerializationDevice->CreateRenderPass(RPDesc, ppSerializedRP);
        ASSERT_NE(*ppSerializedRP, nullptr);
    }
}

void CreateTestRenderPass2(IRenderDevice*        pDevice,
                           ISerializationDevice* pSerializationDevice,
                           ISwapChain*           pSwapChain,
                           const char*           RPName,
                           IRenderPass**         ppRenderPass,
                           IRenderPass**         ppSerializedRP)
{
    auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = pSwapChain->GetDepthBufferDSV();
    ASSERT_NE(pRTV, nullptr);
    ASSERT_NE(pDSV, nullptr);
    const auto& RTVDesc = pRTV->GetTexture()->GetDesc();
    const auto& DSVDesc = pDSV->GetTexture()->GetDesc();

    RenderPassAttachmentDesc Attachments[2];
    Attachments[0].Format       = RTVDesc.Format;
    Attachments[0].SampleCount  = static_cast<Uint8>(RTVDesc.SampleCount);
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[1].Format       = DSVDesc.Format;
    Attachments[1].SampleCount  = static_cast<Uint8>(DSVDesc.SampleCount);
    Attachments[1].InitialState = RESOURCE_STATE_DEPTH_WRITE;
    Attachments[1].FinalState   = RESOURCE_STATE_DEPTH_WRITE;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1] = {};

    Subpasses[0].RenderTargetAttachmentCount = 1;
    AttachmentReference RTAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
    Subpasses[0].pRenderTargetAttachments = &RTAttachmentRef;

    AttachmentReference DSAttachmentRef{1, RESOURCE_STATE_DEPTH_WRITE};
    Subpasses[0].pDepthStencilAttachment = &DSAttachmentRef;

    RenderPassDesc RPDesc;
    RPDesc.Name            = RPName;
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    pDevice->CreateRenderPass(RPDesc, ppRenderPass);
    ASSERT_NE(*ppRenderPass, nullptr);

    pSerializationDevice->CreateRenderPass(RPDesc, ppSerializedRP);
    ASSERT_NE(*ppSerializedRP, nullptr);
}

RefCntAutoPtr<IBuffer> CreateTestVertexBuffer(IRenderDevice* pDevice, IDeviceContext* pContext)
{
    struct Vertex
    {
        float4 Pos;
        float3 Color;
        float2 UV;
    };
    constexpr Vertex Vert[] =
        {
            {float4{-1.0f, -0.5f, 0.f, 1.f}, float3{1.f, 0.f, 0.f}, float2{0.0f, 0.0f}},
            {float4{-0.5f, +0.5f, 0.f, 1.f}, float3{0.f, 1.f, 0.f}, float2{0.5f, 1.0f}},
            {float4{+0.0f, -0.5f, 0.f, 1.f}, float3{0.f, 0.f, 1.f}, float2{1.0f, 0.0f}},

            {float4{+0.0f, -0.5f, 0.f, 1.f}, float3{1.f, 0.f, 0.f}, float2{0.0f, 0.0f}},
            {float4{+0.5f, +0.5f, 0.f, 1.f}, float3{0.f, 1.f, 0.f}, float2{0.5f, 1.0f}},
            {float4{+1.0f, -0.5f, 0.f, 1.f}, float3{0.f, 0.f, 1.f}, float2{1.0f, 0.0f}} //
        };
    constexpr Vertex Triangles[] =
        {
            Vert[0], Vert[1], Vert[2],
            Vert[3], Vert[4], Vert[5] //
        };

    RefCntAutoPtr<IBuffer> pVB;
    BufferDesc             BuffDesc;
    BuffDesc.Name      = "Vertex buffer";
    BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    BuffDesc.Usage     = USAGE_IMMUTABLE;
    BuffDesc.Size      = sizeof(Triangles);

    BufferData InitialData{Triangles, sizeof(Triangles)};
    pDevice->CreateBuffer(BuffDesc, &InitialData, &pVB);
    if (!pVB)
        return pVB;

    StateTransitionDesc Barrier{pVB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE};
    pContext->TransitionResourceStates(1, &Barrier);

    return pVB;
}

std::array<RefCntAutoPtr<ITexture>, 3> CreateTestGBuffer(GPUTestingEnvironment* pEnv, IDeviceContext* pContext)
{
    constexpr Uint32 Width  = 64;
    constexpr Uint32 Height = 64;

    std::array<RefCntAutoPtr<ITexture>, 3> GBuffer;
    {
        Uint32 InitData[Width * Height] = {};

        for (Uint32 y = 0; y < Height; ++y)
            for (Uint32 x = 0; x < Width; ++x)
                InitData[x + y * Width] = (x & 1 ? 0xFF000000 : 0) | (y & 1 ? 0x00FF0000 : 0) | 0x000000FF;

        for (size_t i = 0; i < GBuffer.size(); ++i)
        {
            GBuffer[i] = pEnv->CreateTexture("", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, Width, Height, InitData);

            StateTransitionDesc Barrier{GBuffer[i], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
            pContext->TransitionResourceStates(1, &Barrier);
        }
    }

    return GBuffer;
}


void RenderGraphicsPSOTestImage(IDeviceContext*         pContext,
                                IPipelineState*         pPSO,
                                IRenderPass*            pRenderPass,
                                IShaderResourceBinding* pSRB,
                                IFramebuffer*           pFramebuffer,
                                IBuffer*                pVB)
{
    OptimizedClearValue ClearColor;
    ClearColor.SetColor(TEX_FORMAT_RGBA8_UNORM, 0.25f, 0.5f, 0.75f, 1.0f);

    BeginRenderPassAttribs BeginRPInfo;
    BeginRPInfo.pRenderPass         = pRenderPass;
    BeginRPInfo.pFramebuffer        = pFramebuffer;
    BeginRPInfo.ClearValueCount     = 1;
    BeginRPInfo.pClearValues        = &ClearColor;
    BeginRPInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(BeginRPInfo);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    pContext->SetVertexBuffers(0, 1, &pVB, nullptr, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->Draw(DrawAttribs{6, DRAW_FLAG_VERIFY_ALL});

    pContext->EndRenderPass();
}

void TestGraphicsPipeline(PSO_ARCHIVE_FLAGS ArchiveFlags)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();
    auto* pSwapChain       = pEnv->GetSwapChain();

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    if (pDevice->GetDeviceInfo().Features.SeparablePrograms != DEVICE_FEATURE_STATE_ENABLED)
        GTEST_SKIP() << "Non separable programs are not supported";

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    RefCntAutoPtr<IPipelineStateCache> pPSOCache;
    {
        PipelineStateCacheCreateInfo PSOCacheCI;
        PSOCacheCI.Desc.Mode = /*PSO_CACHE_MODE_LOAD |*/ PSO_CACHE_MODE_STORE;
        pDevice->CreatePipelineStateCache(PSOCacheCI, &pPSOCache);
    }

    constexpr char PSOWithResLayoutName[]  = "ArchiveTest.GraphicsPipeline - PSO with Layout";
    constexpr char PSOWithResLayoutName2[] = "ArchiveTest.GraphicsPipeline - PSO with Layout and Render Pass";
    constexpr char PSOWithSignName[]       = "ArchiveTest.GraphicsPipeline - PSO with Signatures";
    constexpr char PRSName[]               = "ArchiveTest.GraphicsPipeline - PRS";
    constexpr char RPName[]                = "ArchiveTest.GraphicsPipeline - RP";
    constexpr char RP2Name[]               = "ArchiveTest.GraphicsPipeline - RP 2";

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    {
        SerializationDeviceCreateInfo DeviceCI;
        DeviceCI.Metal.CompileOptionsMacOS = "-sdk macosx metal -std=macos-metal2.0 -mmacos-version-min=10.0";
        DeviceCI.Metal.CompileOptionsiOS   = "-sdk iphoneos metal -std=ios-metal2.0 -mios-version-min=10.0";
        DeviceCI.Metal.MslPreprocessorCmd  = "ls";

        pArchiverFactory->CreateSerializationDevice(DeviceCI, &pSerializationDevice);
        ASSERT_NE(pSerializationDevice, nullptr);
    }

    RefCntAutoPtr<IRenderPass> pRenderPass;
    RefCntAutoPtr<IRenderPass> pSerializedRenderPass, pSerializedRenderPassClone;
    CreateTestRenderPass1(pDevice, pSerializationDevice, pSwapChain, RPName, &pRenderPass, &pSerializedRenderPass);
    CreateTestRenderPass1(pDevice, pSerializationDevice, pSwapChain, RPName, nullptr, &pSerializedRenderPassClone);

    {
        RefCntAutoPtr<IRenderPass> pRenderPass2;
        RefCntAutoPtr<IRenderPass> pSerializedRenderPass2;
        CreateTestRenderPass2(pDevice, pSerializationDevice, pSwapChain, RP2Name, &pRenderPass2, &pSerializedRenderPass2);
    }

    constexpr auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS;
    RefCntAutoPtr<IPipelineResourceSignature> pSerializedPRS;
    {
        constexpr PipelineResourceDesc Resources[] = //
            {
                {SHADER_TYPE_PIXEL, "g_GBuffer_Color", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_PIXEL, "g_GBuffer_Normal", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_PIXEL, "g_GBuffer_Depth", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, VarType},
                {SHADER_TYPE_ALL_GRAPHICS, "cbConstants", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, VarType} //
            };
        constexpr ImmutableSamplerDesc ImmutableSamplers[] = //
            {
                {SHADER_TYPE_PIXEL, "g_GBuffer_Color_sampler", SamplerDesc{}},
                {SHADER_TYPE_PIXEL, "g_GBuffer_Normal_sampler", SamplerDesc{}},
                {SHADER_TYPE_PIXEL, "g_GBuffer_Depth_sampler", SamplerDesc{}} //
            };
        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name                 = PRSName;
        PRSDesc.Resources            = Resources;
        PRSDesc.NumResources         = _countof(Resources);
        PRSDesc.ImmutableSamplers    = ImmutableSamplers;
        PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

        pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ResourceSignatureArchiveInfo{GetDeviceBits()}, &pSerializedPRS);
        ASSERT_NE(pSerializedPRS, nullptr);

        pDevice->CreatePipelineResourceSignature(PRSDesc, &pRefPRS);
        ASSERT_NE(pRefPRS, nullptr);
    }

    RefCntAutoPtr<IPipelineState>       pRefPSOWithLayout;
    RefCntAutoPtr<IPipelineState>       pRefPSOWithSign;
    RefCntAutoPtr<IDeviceObjectArchive> pArchive;
    {
        RefCntAutoPtr<IArchiver> pArchiver;
        pArchiverFactory->CreateArchiver(pSerializationDevice, &pArchiver);
        ASSERT_NE(pArchiver, nullptr);

        ShaderMacroHelper Macros;
        Macros.AddShaderMacro("TEST_MACRO", 1u);

        ShaderCreateInfo VertexShaderCI;
        VertexShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        VertexShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(VertexShaderCI.SourceLanguage);
        VertexShaderCI.UseCombinedTextureSamplers = true;
        VertexShaderCI.Macros                     = Macros;

        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/Archiver", &pShaderSourceFactory);
        VertexShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        RefCntAutoPtr<IShader> pVS;
        RefCntAutoPtr<IShader> pSerializedVS;
        {
            VertexShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            VertexShaderCI.EntryPoint      = "main";
            VertexShaderCI.Desc.Name       = "Archive test vertex shader";
            VertexShaderCI.FilePath        = "VertexShader.vsh";

            pDevice->CreateShader(VertexShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);

            pSerializationDevice->CreateShader(VertexShaderCI, ShaderArchiveInfo{GetDeviceBits()}, &pSerializedVS);
            ASSERT_NE(pSerializedVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        RefCntAutoPtr<IShader> pSerializedPS;

        auto PixelShaderCI = VertexShaderCI;
        {
            PixelShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            PixelShaderCI.EntryPoint      = "main";
            PixelShaderCI.Desc.Name       = "Archive test pixel shader";
            PixelShaderCI.FilePath        = "PixelShader.psh";

            pDevice->CreateShader(PixelShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);

            pSerializationDevice->CreateShader(PixelShaderCI, ShaderArchiveInfo{GetDeviceBits()}, &pSerializedPS);
            ASSERT_NE(pSerializedPS, nullptr);
        }

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& GraphicsPipeline{PSOCreateInfo.GraphicsPipeline};
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        constexpr LayoutElement InstancedElems[] =
            {
                LayoutElement{0, 0, 4, VT_FLOAT32},
                LayoutElement{1, 0, 3, VT_FLOAT32},
                LayoutElement{2, 0, 2, VT_FLOAT32} //
            };
        GraphicsPipeline.InputLayout.LayoutElements = InstancedElems;
        GraphicsPipeline.InputLayout.NumElements    = _countof(InstancedElems);

        PSOCreateInfo.pPSOCache = pPSOCache;

        PipelineStateArchiveInfo ArchiveInfo;
        ArchiveInfo.DeviceFlags = GetDeviceBits();
        ArchiveInfo.PSOFlags    = ArchiveFlags;

        // PSO 1 - with resource layout
        {
            PSOCreateInfo.PSODesc.Name = PSOWithResLayoutName;

            constexpr ImmutableSamplerDesc ImmutableSamplers[] = //
                {
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Color", SamplerDesc{}},
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Normal", SamplerDesc{}},
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Depth", SamplerDesc{}} //
                };
            constexpr ShaderResourceVariableDesc Variables[] = //
                {
                    {SHADER_TYPE_ALL_GRAPHICS, "cbConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Color", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Normal", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                    {SHADER_TYPE_PIXEL, "g_GBuffer_Depth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
                    {SHADER_TYPE_PIXEL, "g_Dummy", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
                };
            auto& LayoutDesc{PSOCreateInfo.PSODesc.ResourceLayout};
            LayoutDesc.ImmutableSamplers    = ImmutableSamplers;
            LayoutDesc.NumImmutableSamplers = _countof(ImmutableSamplers);
            LayoutDesc.Variables            = Variables;
            LayoutDesc.NumVariables         = _countof(Variables);
            LayoutDesc.DefaultVariableType  = VarType;

            PSOCreateInfo.pVS = pVS;
            PSOCreateInfo.pPS = pPS;
            pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pRefPSOWithLayout);
            ASSERT_NE(pRefPSOWithLayout, nullptr);

            PSOCreateInfo.pVS = pSerializedVS;
            PSOCreateInfo.pPS = pSerializedPS;
            {
                RefCntAutoPtr<IPipelineState> pSerializedPSO;
                pSerializationDevice->CreateGraphicsPipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
                ASSERT_NE(pSerializedPSO, nullptr);
                ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));

                for (auto Flags = ArchiveInfo.DeviceFlags; Flags != ARCHIVE_DEVICE_DATA_FLAG_NONE;)
                {
                    const auto Flag        = ExtractLSB(Flags);
                    const auto ShaderCount = pSerializedPSO.Cast<ISerializedPipelineState>(IID_SerializedPipelineState)->GetPatchedShaderCount(Flag);
                    EXPECT_EQ(ShaderCount, 2u);
                    for (Uint32 ShaderId = 0; ShaderId < ShaderCount; ++ShaderId)
                    {
                        auto ShaderCI = pSerializedPSO.Cast<ISerializedPipelineState>(IID_SerializedPipelineState)->GetPatchedShaderCreateInfo(Flag, ShaderId);

                        EXPECT_TRUE(ShaderCI.Desc.ShaderType == SHADER_TYPE_VERTEX || ShaderCI.Desc.ShaderType == SHADER_TYPE_PIXEL);
                        const auto& RefCI = ShaderCI.Desc.ShaderType == SHADER_TYPE_VERTEX ? VertexShaderCI : PixelShaderCI;
                        EXPECT_STREQ(ShaderCI.Desc.Name, RefCI.Desc.Name);
                        EXPECT_STREQ(ShaderCI.EntryPoint, RefCI.EntryPoint);
                        EXPECT_EQ(ShaderCI.UseCombinedTextureSamplers, RefCI.UseCombinedTextureSamplers);
                        EXPECT_STREQ(ShaderCI.CombinedSamplerSuffix, RefCI.CombinedSamplerSuffix);
                        EXPECT_TRUE(ShaderCI.ByteCodeSize > 0 || ShaderCI.SourceLength > 0);
                    }
                }
            }

            {
                PSOCreateInfo.PSODesc.Name        = PSOWithResLayoutName2;
                GraphicsPipeline.NumRenderTargets = 0;
                GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;
                // Use render pass clone to test render pass deduplication later
                GraphicsPipeline.pRenderPass = pSerializedRenderPassClone;
                RefCntAutoPtr<IPipelineState> pSerializedPSO;
                pSerializationDevice->CreateGraphicsPipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
                ASSERT_NE(pSerializedPSO, nullptr);
                ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));
            }

            PSOCreateInfo.PSODesc.ResourceLayout = {};
        }

        // PSO 2 - with explicit resource signatures and render pass
        {
            PSOCreateInfo.PSODesc.Name        = PSOWithSignName;
            GraphicsPipeline.NumRenderTargets = 0;
            GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;

            {
                PSOCreateInfo.pVS = pSerializedVS;
                PSOCreateInfo.pPS = pSerializedPS;

                GraphicsPipeline.pRenderPass = pSerializedRenderPass;

                IPipelineResourceSignature* SerializedSignatures[]{pSerializedPRS};
                PSOCreateInfo.ResourceSignaturesCount = _countof(SerializedSignatures);
                PSOCreateInfo.ppResourceSignatures    = SerializedSignatures;

                RefCntAutoPtr<IPipelineState> pSerializedPSO;
                // Note that render should be deduplicated by the archiver
                pSerializationDevice->CreateGraphicsPipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
                ASSERT_NE(pSerializedPSO, nullptr);
                ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));
            }

            {
                PSOCreateInfo.pVS = pVS;
                PSOCreateInfo.pPS = pPS;

                GraphicsPipeline.pRenderPass = pRenderPass;

                IPipelineResourceSignature* Signatures[] = {pRefPRS};
                PSOCreateInfo.ResourceSignaturesCount    = _countof(Signatures);
                PSOCreateInfo.ppResourceSignatures       = Signatures;

                pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pRefPSOWithSign);
                ASSERT_NE(pRefPSOWithSign, nullptr);
            }
        }

        RefCntAutoPtr<IDataBlob> pBlob;
        pArchiver->SerializeToBlob(&pBlob);
        ASSERT_NE(pBlob, nullptr);

        RefCntAutoPtr<IArchive> pSource{MakeNewRCObj<ArchiveMemoryImpl>{}(pBlob)};
        pDearchiver->CreateDeviceObjectArchive(pSource, &pArchive);
        ASSERT_NE(pArchive, nullptr);
    }

    // Unpack Render pass
    RefCntAutoPtr<IRenderPass> pUnpackedRenderPass;
    {
        RenderPassUnpackInfo UnpackInfo;
        UnpackInfo.Name     = RPName;
        UnpackInfo.pArchive = pArchive;
        UnpackInfo.pDevice  = pDevice;

        pDearchiver->UnpackRenderPass(UnpackInfo, &pUnpackedRenderPass);
        ASSERT_NE(pUnpackedRenderPass, nullptr);
        EXPECT_EQ(pUnpackedRenderPass->GetDesc(), pRenderPass->GetDesc());
    }

    // Unpack PSO 1
    {
        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.Name         = PSOWithResLayoutName;
        UnpackInfo.pArchive     = pArchive;
        UnpackInfo.pDevice      = pDevice;
        UnpackInfo.PipelineType = PIPELINE_TYPE_GRAPHICS;
        UnpackInfo.pCache       = pPSOCache;

        RefCntAutoPtr<IPipelineState> pUnpackedPSOWithLayout;
        pDearchiver->UnpackPipelineState(UnpackInfo, &pUnpackedPSOWithLayout);
        ASSERT_NE(pUnpackedPSOWithLayout, nullptr);

        EXPECT_EQ(pUnpackedPSOWithLayout->GetGraphicsPipelineDesc(), pRefPSOWithLayout->GetGraphicsPipelineDesc());
        EXPECT_EQ(pUnpackedPSOWithLayout->GetResourceSignatureCount(), pRefPSOWithLayout->GetResourceSignatureCount());

        // OpenGL PRS has immutable samplers as resources, which is not supported in comparator.
        // Metal PRS in the Archiver is generated from SPIRV, in the Engine - from the reflection, and they thus may have different resource order.
        if (!pDevice->GetDeviceInfo().IsGLDevice() && !pDevice->GetDeviceInfo().IsMetalDevice())
        {
            for (Uint32 s = 0, SCnt = std::min(pUnpackedPSOWithLayout->GetResourceSignatureCount(), pRefPSOWithLayout->GetResourceSignatureCount()); s < SCnt; ++s)
            {
                auto* pLhsSign = pUnpackedPSOWithLayout->GetResourceSignature(s);
                auto* pRhsSign = pRefPSOWithLayout->GetResourceSignature(s);
                EXPECT_EQ((pLhsSign != nullptr), (pRhsSign != nullptr));
                if ((pLhsSign != nullptr) != (pRhsSign != nullptr))
                    continue;

                EXPECT_EQ(pLhsSign->GetDesc(), pRhsSign->GetDesc());
                EXPECT_TRUE(pLhsSign->IsCompatibleWith(pRhsSign));
            }
        }

        // Check default PRS cache
        RefCntAutoPtr<IPipelineState> pUnpackedPSOWithLayout2;
        UnpackInfo.Name = PSOWithResLayoutName2;
        pDearchiver->UnpackPipelineState(UnpackInfo, &pUnpackedPSOWithLayout2);
        ASSERT_NE(pUnpackedPSOWithLayout2, nullptr);

        EXPECT_EQ(pUnpackedPSOWithLayout2->GetResourceSignatureCount(), 1u);
        EXPECT_TRUE(pUnpackedPSOWithLayout2->GetResourceSignature(0)->IsCompatibleWith(pUnpackedPSOWithLayout->GetResourceSignature(0)));
    }

    // Unpack PSO 2
    RefCntAutoPtr<IPipelineState> pUnpackedPSOWithSign;
    {
        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.Name         = PSOWithSignName;
        UnpackInfo.pArchive     = pArchive;
        UnpackInfo.pDevice      = pDevice;
        UnpackInfo.PipelineType = PIPELINE_TYPE_GRAPHICS;
        UnpackInfo.pCache       = pPSOCache;

        pDearchiver->UnpackPipelineState(UnpackInfo, &pUnpackedPSOWithSign);
        ASSERT_NE(pUnpackedPSOWithSign, nullptr);

        EXPECT_EQ(pUnpackedPSOWithSign->GetGraphicsPipelineDesc(), pRefPSOWithSign->GetGraphicsPipelineDesc());
        EXPECT_EQ(pUnpackedPSOWithSign->GetGraphicsPipelineDesc().pRenderPass, pUnpackedRenderPass);
        EXPECT_EQ(pUnpackedPSOWithSign->GetResourceSignatureCount(), pRefPSOWithSign->GetResourceSignatureCount());

        for (Uint32 s = 0, SCnt = std::min(pUnpackedPSOWithSign->GetResourceSignatureCount(), pRefPSOWithSign->GetResourceSignatureCount()); s < SCnt; ++s)
        {
            auto* pLhsSign = pUnpackedPSOWithSign->GetResourceSignature(s);
            auto* pRhsSign = pRefPSOWithSign->GetResourceSignature(s);
            EXPECT_EQ((pLhsSign != nullptr), (pRhsSign != nullptr));
            if ((pLhsSign != nullptr) != (pRhsSign != nullptr))
                continue;

            EXPECT_EQ(pLhsSign->GetDesc(), pRhsSign->GetDesc());
            EXPECT_TRUE(pLhsSign->IsCompatibleWith(pRhsSign));
        }
    }

    auto* pContext = pEnv->GetDeviceContext();

    auto pVB = CreateTestVertexBuffer(pDevice, pContext);
    ASSERT_NE(pVB, nullptr);

    auto GBuffer = CreateTestGBuffer(pEnv, pContext);
    for (const auto& Buff : GBuffer)
        ASSERT_NE(Buff, nullptr);

    RefCntAutoPtr<IBuffer> pConstants;
    {
        struct Constants
        {
            float4 UVScale;
            float4 ColorScale;
            float4 NormalScale;
            float4 DepthScale;
        } Const;
        Const.UVScale     = float4{0.9f, 0.8f, 0.0f, 0.0f};
        Const.ColorScale  = float4{0.15f};
        Const.NormalScale = float4{0.2f};
        Const.DepthScale  = float4{0.1f};

        BufferDesc BuffDesc;
        BuffDesc.Name      = "Constant buffer";
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage     = USAGE_IMMUTABLE;
        BuffDesc.Size      = sizeof(Const);

        BufferData InitialData{&Const, sizeof(Const)};
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pConstants);
        ASSERT_NE(pConstants, nullptr);

        StateTransitionDesc Barrier{pConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    {
        pRefPRS->CreateShaderResourceBinding(&pSRB);
        ASSERT_NE(pSRB, nullptr);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Color")->Set(GBuffer[0]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Normal")->Set(GBuffer[1]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Depth")->Set(GBuffer[2]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "cbConstants")->Set(pConstants);
    }

    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    {
        ITextureView* pTexViews[] = {pSwapChain->GetCurrentBackBufferRTV()};

        FramebufferDesc FBDesc;
        FBDesc.Name            = "Framebuffer 1";
        FBDesc.pRenderPass     = pRenderPass;
        FBDesc.AttachmentCount = _countof(pTexViews);
        FBDesc.ppAttachments   = pTexViews;
        pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
        ASSERT_NE(pFramebuffer, nullptr);
    }

    // Draw reference
    if (RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain})
    {
        RenderGraphicsPSOTestImage(pContext, pRefPSOWithSign, pRenderPass, pSRB, pFramebuffer, pVB);

        // Transition to CopySrc state to use in TakeSnapshot()
        auto                pRT = pSwapChain->GetCurrentBackBufferRTV()->GetTexture();
        StateTransitionDesc Barrier{pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

        pTestingSwapChain->TakeSnapshot(pRT);
    }

    // Draw
    RenderGraphicsPSOTestImage(pContext, pUnpackedPSOWithSign, pUnpackedRenderPass, pSRB, pFramebuffer, pVB);
    pSwapChain->Present();

    if (pPSOCache)
    {
        RefCntAutoPtr<IDataBlob> pCacheData;
        pPSOCache->GetData(&pCacheData);
        //ASSERT_NE(pCacheData, nullptr); // not implemented for all backends
        //ASSERT_NE(pCacheData->GetSize(), 0u);
    }
}

TEST(ArchiveTest, GraphicsPipeline)
{
    TestGraphicsPipeline(PSO_ARCHIVE_FLAG_NONE);
}

TEST(ArchiveTest, GraphicsPipeline_NoReflection)
{
    TestGraphicsPipeline(PSO_ARCHIVE_FLAG_STRIP_REFLECTION);
}

namespace HLSL
{

// Test shader source as string
const std::string ComputePSOTest_CS{R"(

RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 ui2Dim;
    g_tex2DUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
    if (DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y)
        return;

    g_tex2DUAV[DTid.xy] = float4(float2(DTid.xy % 256u) / 256.0, 0.0, 1.0);
}

)"};

} // namespace HLSL

void TestComputePipeline(PSO_ARCHIVE_FLAGS ArchiveFlags)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    if (!pDevice->GetDeviceInfo().Features.ComputeShaders)
        GTEST_SKIP() << "Compute shaders are not supported by device";

    constexpr char PSO1Name[] = "ArchiveTest.ComputePipeline - PSO";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    auto*       pSwapChain = pEnv->GetSwapChain();
    const auto& SCDesc     = pSwapChain->GetDesc();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    if (!pTestingSwapChain)
    {
        GTEST_SKIP() << "Compute shader test requires testing swap chain";
    }

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCreateInfo{}, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    RefCntAutoPtr<IPipelineResourceSignature> pRefPRS;
    RefCntAutoPtr<IPipelineResourceSignature> pSerializedPRS;
    {
        constexpr PipelineResourceDesc Resources[] = {{SHADER_TYPE_COMPUTE, "g_tex2DUAV", 1, SHADER_RESOURCE_TYPE_TEXTURE_UAV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}};

        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name         = "ArchiveTest.ComputePipeline - PRS";
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);

        pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ResourceSignatureArchiveInfo{GetDeviceBits()}, &pSerializedPRS);
        ASSERT_NE(pSerializedPRS, nullptr);

        pDevice->CreatePipelineResourceSignature(PRSDesc, &pRefPRS);
        ASSERT_NE(pRefPRS, nullptr);
    }

    RefCntAutoPtr<IPipelineState>       pRefPSO;
    RefCntAutoPtr<IDeviceObjectArchive> pArchive;
    {
        RefCntAutoPtr<IArchiver> pArchiver;
        pArchiverFactory->CreateArchiver(pSerializationDevice, &pArchiver);
        ASSERT_NE(pArchiver, nullptr);

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pCS;
        RefCntAutoPtr<IShader> pSerializedCS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Compute shader test";
            // Test shader source string
            ShaderCI.Source = HLSL::ComputePSOTest_CS.c_str();

            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);

            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{GetDeviceBits()}, &pSerializedCS);
            ASSERT_NE(pSerializedCS, nullptr);
        }
        {
            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = PSO1Name;
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pCS;

            IPipelineResourceSignature* Signatures[] = {pRefPRS};
            PSOCreateInfo.ResourceSignaturesCount    = _countof(Signatures);
            PSOCreateInfo.ppResourceSignatures       = Signatures;

            pDevice->CreateComputePipelineState(PSOCreateInfo, &pRefPSO);
            ASSERT_NE(pRefPSO, nullptr);
        }
        {
            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = PSO1Name;
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pSerializedCS;

            IPipelineResourceSignature* Signatures[] = {pSerializedPRS};
            PSOCreateInfo.ResourceSignaturesCount    = _countof(Signatures);
            PSOCreateInfo.ppResourceSignatures       = Signatures;

            PipelineStateArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = GetDeviceBits();
            ArchiveInfo.PSOFlags    = ArchiveFlags;
            RefCntAutoPtr<IPipelineState> pSerializedPSO;
            pSerializationDevice->CreateComputePipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
            ASSERT_NE(pSerializedPSO, nullptr);
            ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));
        }
        RefCntAutoPtr<IDataBlob> pBlob;
        pArchiver->SerializeToBlob(&pBlob);
        ASSERT_NE(pBlob, nullptr);

        RefCntAutoPtr<IArchive> pSource{MakeNewRCObj<ArchiveMemoryImpl>{}(pBlob)};
        pDearchiver->CreateDeviceObjectArchive(pSource, &pArchive);
        ASSERT_NE(pArchive, nullptr);
    }

    // Unpack PSO
    RefCntAutoPtr<IPipelineState> pUnpackedPSO;
    {
        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.Name         = PSO1Name;
        UnpackInfo.pArchive     = pArchive;
        UnpackInfo.pDevice      = pDevice;
        UnpackInfo.PipelineType = PIPELINE_TYPE_COMPUTE;

        pDearchiver->UnpackPipelineState(UnpackInfo, &pUnpackedPSO);
        ASSERT_NE(pUnpackedPSO, nullptr);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pRefPRS->CreateShaderResourceBinding(&pSRB);
    ASSERT_NE(pSRB, nullptr);

    auto*      pContext = pEnv->GetDeviceContext();
    const auto Dispatch = [&](IPipelineState* pPSO, ITextureView* pTextureUAV) //
    {
        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex2DUAV")->Set(pTextureUAV);

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DispatchComputeAttribs DispatchAttribs;
        DispatchAttribs.ThreadGroupCountX = (SCDesc.Width + 15) / 16;
        DispatchAttribs.ThreadGroupCountY = (SCDesc.Height + 15) / 16;
        pContext->DispatchCompute(DispatchAttribs);
    };

    // Dispatch reference
    Dispatch(pRefPSO, pTestingSwapChain->GetCurrentBackBufferUAV());

    ITexture*           pTexUAV = pTestingSwapChain->GetCurrentBackBufferUAV()->GetTexture();
    StateTransitionDesc Barrier{pTexUAV, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
    pContext->TransitionResourceStates(1, &Barrier);

    pContext->Flush();
    pContext->InvalidateState(); // because TakeSnapshot() will clear state in D3D11

    pTestingSwapChain->TakeSnapshot(pTexUAV);

    // Dispatch
    Dispatch(pUnpackedPSO, pTestingSwapChain->GetCurrentBackBufferUAV());

    pSwapChain->Present();
}

TEST(ArchiveTest, ComputePipeline)
{
    TestComputePipeline(PSO_ARCHIVE_FLAG_NONE);
}

TEST(ArchiveTest, ComputePipeline_NoReflection)
{
    TestComputePipeline(PSO_ARCHIVE_FLAG_STRIP_REFLECTION);
}

TEST(ArchiveTest, RayTracingPipeline)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pDevice          = pEnv->GetDevice();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();
    auto* pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    if (!pEnv->SupportsRayTracing())
        GTEST_SKIP() << "Ray tracing shaders are not supported by device";

    constexpr char PSO1Name[] = "ArchiveTest.RayTracingPipeline - PSO";

    GPUTestingEnvironment::ScopedReleaseResources AutoreleaseResources;

    auto* pSwapChain = pEnv->GetSwapChain();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
    if (!pTestingSwapChain)
    {
        GTEST_SKIP() << "Ray tracing shader test requires testing swap chain";
    }

    SerializationDeviceCreateInfo DeviceCI;
    DeviceCI.D3D12.ShaderVersion = Version{6, 5};
    DeviceCI.Vulkan.ApiVersion   = Version{1, 2};

    DeviceCI.AdapterInfo.RayTracing.CapFlags          = RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS | RAY_TRACING_CAP_FLAG_INLINE_RAY_TRACING;
    DeviceCI.AdapterInfo.RayTracing.MaxRecursionDepth = 32;

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(DeviceCI, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    const auto DeviceBits = GetDeviceBits() & (ARCHIVE_DEVICE_DATA_FLAG_D3D12 | ARCHIVE_DEVICE_DATA_FLAG_VULKAN);

    RefCntAutoPtr<IPipelineState>       pRefPSO;
    RefCntAutoPtr<IDeviceObjectArchive> pArchive;
    {
        RefCntAutoPtr<IArchiver> pArchiver;
        pArchiverFactory->CreateArchiver(pSerializationDevice, &pArchiver);
        ASSERT_NE(pArchiver, nullptr);

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
        ShaderCI.HLSLVersion    = {6, 3};
        ShaderCI.EntryPoint     = "main";

        // Create ray generation shader.
        RefCntAutoPtr<IShader> pRG;
        RefCntAutoPtr<IShader> pSerializedRG;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_GEN;
            ShaderCI.Desc.Name       = "Ray tracing RG";
            ShaderCI.Source          = HLSL::RayTracingTest1_RG.c_str();
            pDevice->CreateShader(ShaderCI, &pRG);
            ASSERT_NE(pRG, nullptr);
            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{DeviceBits}, &pSerializedRG);
            ASSERT_NE(pSerializedRG, nullptr);
        }

        // Create ray miss shader.
        RefCntAutoPtr<IShader> pRMiss;
        RefCntAutoPtr<IShader> pSerializedRMiss;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_MISS;
            ShaderCI.Desc.Name       = "Miss shader";
            ShaderCI.Source          = HLSL::RayTracingTest1_RM.c_str();
            pDevice->CreateShader(ShaderCI, &pRMiss);
            ASSERT_NE(pRMiss, nullptr);
            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{DeviceBits}, &pSerializedRMiss);
            ASSERT_NE(pSerializedRMiss, nullptr);
        }

        // Create ray closest hit shader.
        RefCntAutoPtr<IShader> pClosestHit;
        RefCntAutoPtr<IShader> pSerializedClosestHit;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_RAY_CLOSEST_HIT;
            ShaderCI.Desc.Name       = "Ray closest hit shader";
            ShaderCI.Source          = HLSL::RayTracingTest1_RCH.c_str();
            pDevice->CreateShader(ShaderCI, &pClosestHit);
            ASSERT_NE(pClosestHit, nullptr);
            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{DeviceBits}, &pSerializedClosestHit);
            ASSERT_NE(pSerializedClosestHit, nullptr);
        }

        RayTracingPipelineStateCreateInfo PSOCreateInfo;

        PSOCreateInfo.PSODesc.Name         = "Ray tracing PSO";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_RAY_TRACING;

        PSOCreateInfo.RayTracingPipeline.MaxRecursionDepth       = 1;
        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        {
            const RayTracingGeneralShaderGroup     GeneralShaders[]     = {{"Main", pRG}, {"Miss", pRMiss}};
            const RayTracingTriangleHitShaderGroup TriangleHitShaders[] = {{"HitGroup", pClosestHit}};

            PSOCreateInfo.pGeneralShaders        = GeneralShaders;
            PSOCreateInfo.GeneralShaderCount     = _countof(GeneralShaders);
            PSOCreateInfo.pTriangleHitShaders    = TriangleHitShaders;
            PSOCreateInfo.TriangleHitShaderCount = _countof(TriangleHitShaders);

            pDevice->CreateRayTracingPipelineState(PSOCreateInfo, &pRefPSO);
            ASSERT_NE(pRefPSO, nullptr);
        }
        {
            const RayTracingGeneralShaderGroup     GeneralSerializedShaders[]     = {{"Main", pSerializedRG}, {"Miss", pSerializedRMiss}};
            const RayTracingTriangleHitShaderGroup TriangleHitSerializedShaders[] = {{"HitGroup", pSerializedClosestHit}};

            PSOCreateInfo.pGeneralShaders        = GeneralSerializedShaders;
            PSOCreateInfo.GeneralShaderCount     = _countof(GeneralSerializedShaders);
            PSOCreateInfo.pTriangleHitShaders    = TriangleHitSerializedShaders;
            PSOCreateInfo.TriangleHitShaderCount = _countof(TriangleHitSerializedShaders);
            PSOCreateInfo.PSODesc.Name           = PSO1Name;

            PipelineStateArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = DeviceBits;
            RefCntAutoPtr<IPipelineState> pSerializedPSO;
            pSerializationDevice->CreateRayTracingPipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
            ASSERT_NE(pSerializedPSO, nullptr);
            ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));
        }
        RefCntAutoPtr<IDataBlob> pBlob;
        pArchiver->SerializeToBlob(&pBlob);
        ASSERT_NE(pBlob, nullptr);

        RefCntAutoPtr<IArchive> pSource{MakeNewRCObj<ArchiveMemoryImpl>{}(pBlob)};
        pDearchiver->CreateDeviceObjectArchive(pSource, &pArchive);
        ASSERT_NE(pArchive, nullptr);
    }

    // Unpack PSO
    RefCntAutoPtr<IPipelineState> pUnpackedPSO;
    {
        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.Name         = PSO1Name;
        UnpackInfo.pArchive     = pArchive;
        UnpackInfo.pDevice      = pDevice;
        UnpackInfo.PipelineType = PIPELINE_TYPE_RAY_TRACING;

        pDearchiver->UnpackPipelineState(UnpackInfo, &pUnpackedPSO);
        ASSERT_NE(pUnpackedPSO, nullptr);
    }

    RefCntAutoPtr<IShaderResourceBinding> pRayTracingSRB;
    pRefPSO->CreateShaderResourceBinding(&pRayTracingSRB, true);
    ASSERT_NE(pRayTracingSRB, nullptr);

    // Create BLAS & TLAS
    RefCntAutoPtr<IBottomLevelAS> pBLAS;
    RefCntAutoPtr<ITopLevelAS>    pTLAS;
    auto*                         pContext       = pEnv->GetDeviceContext();
    const Uint32                  HitGroupStride = 1;
    {
        const auto& Vertices = TestingConstants::TriangleClosestHit::Vertices;

        RefCntAutoPtr<IBuffer> pVertexBuffer;
        {
            BufferDesc BuffDesc;
            BuffDesc.Name      = "Triangle vertices";
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = sizeof(Vertices);
            pDevice->CreateBuffer(BuffDesc, nullptr, &pVertexBuffer);
            ASSERT_NE(pVertexBuffer, nullptr);

            pContext->UpdateBuffer(pVertexBuffer, 0, sizeof(Vertices), Vertices, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }

        // Create & Build BLAS
        {
            BLASBuildTriangleData Triangle;
            Triangle.GeometryName         = "Triangle";
            Triangle.pVertexBuffer        = pVertexBuffer;
            Triangle.VertexStride         = sizeof(Vertices[0]);
            Triangle.VertexOffset         = 0;
            Triangle.VertexCount          = _countof(Vertices);
            Triangle.VertexValueType      = VT_FLOAT32;
            Triangle.VertexComponentCount = 3;
            Triangle.Flags                = RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            Triangle.PrimitiveCount       = Triangle.VertexCount / 3;

            BLASTriangleDesc TriangleDesc;
            TriangleDesc.GeometryName         = Triangle.GeometryName;
            TriangleDesc.MaxVertexCount       = Triangle.VertexCount;
            TriangleDesc.VertexValueType      = Triangle.VertexValueType;
            TriangleDesc.VertexComponentCount = Triangle.VertexComponentCount;
            TriangleDesc.MaxPrimitiveCount    = Triangle.PrimitiveCount;
            TriangleDesc.IndexType            = Triangle.IndexType;

            BottomLevelASDesc ASDesc;
            ASDesc.Name          = "Triangle BLAS";
            ASDesc.pTriangles    = &TriangleDesc;
            ASDesc.TriangleCount = 1;

            pDevice->CreateBLAS(ASDesc, &pBLAS);
            ASSERT_NE(pBLAS, nullptr);

            // Create scratch buffer
            RefCntAutoPtr<IBuffer> ScratchBuffer;

            BufferDesc BuffDesc;
            BuffDesc.Name      = "BLAS Scratch Buffer";
            BuffDesc.Usage     = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = std::max(pBLAS->GetScratchBufferSizes().Build, pBLAS->GetScratchBufferSizes().Update);

            pDevice->CreateBuffer(BuffDesc, nullptr, &ScratchBuffer);
            ASSERT_NE(ScratchBuffer, nullptr);

            BuildBLASAttribs Attribs;
            Attribs.pBLAS                       = pBLAS;
            Attribs.BLASTransitionMode          = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.GeometryTransitionMode      = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.pTriangleData               = &Triangle;
            Attribs.TriangleDataCount           = 1;
            Attribs.pScratchBuffer              = ScratchBuffer;
            Attribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

            pContext->BuildBLAS(Attribs);
        }

        // Create & Build TLAS
        {
            TLASBuildInstanceData Instance;
            Instance.InstanceName = "Instance";
            Instance.pBLAS        = pBLAS;
            Instance.Flags        = RAYTRACING_INSTANCE_NONE;

            // Create TLAS
            TopLevelASDesc TLASDesc;
            TLASDesc.Name             = "TLAS";
            TLASDesc.MaxInstanceCount = 1;

            pDevice->CreateTLAS(TLASDesc, &pTLAS);
            ASSERT_NE(pTLAS, nullptr);

            // Create scratch buffer
            RefCntAutoPtr<IBuffer> ScratchBuffer;

            BufferDesc BuffDesc;
            BuffDesc.Name      = "TLAS Scratch Buffer";
            BuffDesc.Usage     = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = pTLAS->GetScratchBufferSizes().Build;

            pDevice->CreateBuffer(BuffDesc, nullptr, &ScratchBuffer);
            ASSERT_NE(ScratchBuffer, nullptr);

            // create instance buffer
            RefCntAutoPtr<IBuffer> InstanceBuffer;

            BuffDesc.Name      = "TLAS Instance Buffer";
            BuffDesc.Usage     = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = TLAS_INSTANCE_DATA_SIZE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &InstanceBuffer);
            ASSERT_NE(InstanceBuffer, nullptr);

            // Build
            BuildTLASAttribs Attribs;
            Attribs.pTLAS                        = pTLAS;
            Attribs.pInstances                   = &Instance;
            Attribs.InstanceCount                = 1;
            Attribs.HitGroupStride               = HitGroupStride;
            Attribs.BindingMode                  = HIT_GROUP_BINDING_MODE_PER_GEOMETRY;
            Attribs.TLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.BLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.pInstanceBuffer              = InstanceBuffer;
            Attribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.pScratchBuffer               = ScratchBuffer;
            Attribs.ScratchBufferTransitionMode  = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

            pContext->BuildTLAS(Attribs);
        }
    }

    const auto CreateSBT = [&](RefCntAutoPtr<IShaderBindingTable>& pSBT, IPipelineState* pPSO) //
    {
        ShaderBindingTableDesc SBTDesc;
        SBTDesc.Name = "SBT";
        SBTDesc.pPSO = pPSO;

        pDevice->CreateSBT(SBTDesc, &pSBT);
        ASSERT_NE(pSBT, nullptr);

        pSBT->BindRayGenShader("Main");
        pSBT->BindMissShader("Miss", 0);
        pSBT->BindHitGroupForGeometry(pTLAS, "Instance", "Triangle", 0, "HitGroup");

        pContext->UpdateSBT(pSBT);
    };

    RefCntAutoPtr<IShaderBindingTable> pRefPSO_SBT;
    CreateSBT(pRefPSO_SBT, pRefPSO);

    RefCntAutoPtr<IShaderBindingTable> pUnpackedPSO_SBT;
    CreateSBT(pUnpackedPSO_SBT, pUnpackedPSO);

    pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_GEN, "g_TLAS")->Set(pTLAS);

    const auto& SCDesc    = pSwapChain->GetDesc();
    const auto  TraceRays = [&](IPipelineState* pPSO, ITextureView* pTextureUAV, IShaderBindingTable* pSBT) //
    {
        pRayTracingSRB->GetVariableByName(SHADER_TYPE_RAY_GEN, "g_ColorBuffer")->Set(pTextureUAV);

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pRayTracingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        TraceRaysAttribs Attribs;
        Attribs.DimensionX = SCDesc.Width;
        Attribs.DimensionY = SCDesc.Height;
        Attribs.pSBT       = pSBT;

        pContext->TraceRays(Attribs);
    };

    // Reference
    TraceRays(pRefPSO, pTestingSwapChain->GetCurrentBackBufferUAV(), pRefPSO_SBT);

    ITexture*           pTexUAV = pTestingSwapChain->GetCurrentBackBufferUAV()->GetTexture();
    StateTransitionDesc Barrier{pTexUAV, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
    pContext->TransitionResourceStates(1, &Barrier);

    pContext->Flush();

    pTestingSwapChain->TakeSnapshot(pTexUAV);

    // Unpacked
    TraceRays(pUnpackedPSO, pTestingSwapChain->GetCurrentBackBufferUAV(), pUnpackedPSO_SBT);

    pSwapChain->Present();
}


TEST(ArchiveTest, ResourceSignatureBindings)
{
    auto* pEnv             = GPUTestingEnvironment::GetInstance();
    auto* pArchiverFactory = pEnv->GetArchiverFactory();

    if (!pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCreateInfo{}, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    for (auto AllDeviceBits = GetDeviceBits() & ~ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS; AllDeviceBits != 0;)
    {
        const auto DeviceBit  = ExtractLSB(AllDeviceBits);
        const auto DeviceType = static_cast<RENDER_DEVICE_TYPE>(PlatformMisc::GetLSB(DeviceBit));

        const auto VS_PS = SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX;
        const auto PS    = SHADER_TYPE_PIXEL;
        const auto VS    = SHADER_TYPE_VERTEX;

        // PRS 1
        RefCntAutoPtr<IPipelineResourceSignature> pPRS1;
        {
            const auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

            std::vector<PipelineResourceDesc> Resources =
                {
                    // clang-format off
                    {PS,    "g_DiffuseTexs",  100, SHADER_RESOURCE_TYPE_TEXTURE_SRV,      VarType, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                    {PS,    "g_NormalTexs",   100, SHADER_RESOURCE_TYPE_TEXTURE_SRV,      VarType, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
                    {VS_PS, "ConstBuff_1",      1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VarType},
                    {VS_PS, "PerObjectConst",   8, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VarType},
                    {PS,    "g_SubpassInput",   1, SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, VarType}
                    // clang-format on
                };

            if (DeviceType == RENDER_DEVICE_TYPE_D3D12 || DeviceType == RENDER_DEVICE_TYPE_VULKAN)
                Resources.emplace_back(PS, "g_TLAS", 1, SHADER_RESOURCE_TYPE_ACCEL_STRUCT, VarType);

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS 1";
            PRSDesc.BindingIndex = 0;
            PRSDesc.Resources    = Resources.data();
            PRSDesc.NumResources = static_cast<Uint32>(Resources.size());

            const ImmutableSamplerDesc ImmutableSamplers[] = //
                {
                    {PS, "g_Sampler", SamplerDesc{}} //
                };
            PRSDesc.ImmutableSamplers    = ImmutableSamplers;
            PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

            pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ResourceSignatureArchiveInfo{DeviceBit}, &pPRS1);
            ASSERT_NE(pPRS1, nullptr);
        }

        // PRS 2
        RefCntAutoPtr<IPipelineResourceSignature> pPRS2;
        {
            const auto VarType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

            const PipelineResourceDesc Resources[] = //
                {
                    // clang-format off
                    {PS,    "g_RWTex2D",   2, SHADER_RESOURCE_TYPE_TEXTURE_UAV, VarType},
                    {VS_PS, "g_TexelBuff", 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,  VarType, PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}
                    // clang-format on
                };

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS 2";
            PRSDesc.BindingIndex = 2;
            PRSDesc.Resources    = Resources;
            PRSDesc.NumResources = _countof(Resources);

            pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ResourceSignatureArchiveInfo{DeviceBit}, &pPRS2);
            ASSERT_NE(pPRS2, nullptr);
        }

        IPipelineResourceSignature* Signatures[] = {pPRS2, pPRS1};
        Char const* const           VBNames[]    = {"VBPosition", "VBTexcoord"};

        PipelineResourceBindingAttribs Info;
        Info.ppResourceSignatures    = Signatures;
        Info.ResourceSignaturesCount = _countof(Signatures);
        Info.ShaderStages            = SHADER_TYPE_ALL_GRAPHICS;
        Info.DeviceType              = DeviceType;

        if (DeviceType == RENDER_DEVICE_TYPE_METAL)
        {
            Info.NumVertexBuffers  = _countof(VBNames);
            Info.VertexBufferNames = VBNames;
        }

        Uint32                         NumBindings = 0;
        const PipelineResourceBinding* Bindings    = nullptr;
        pSerializationDevice->GetPipelineResourceBindings(Info, NumBindings, Bindings);
        ASSERT_NE(NumBindings, 0u);
        ASSERT_NE(Bindings, nullptr);

        const auto CompareBindings = [NumBindings, Bindings](const PipelineResourceBinding* RefBindings, Uint32 Count) //
        {
            EXPECT_EQ(NumBindings, Count);
            if (NumBindings != Count)
                return;

            struct Key
            {
                HashMapStringKey Name;
                SHADER_TYPE      Stages;

                Key(const char* _Name, SHADER_TYPE _Stages) :
                    Name{_Name}, Stages{_Stages} {}

                bool operator==(const Key& Rhs) const
                {
                    return Name == Rhs.Name && Stages == Rhs.Stages;
                }

                struct Hasher
                {
                    size_t operator()(const Key& key) const
                    {
                        size_t Hash = key.Name.GetHash();
                        HashCombine(Hash, key.Stages);
                        return Hash;
                    }
                };
            };

            std::unordered_map<Key, const PipelineResourceBinding*, Key::Hasher> BindingMap;
            for (Uint32 i = 0; i < NumBindings; ++i)
                BindingMap.emplace(Key{Bindings[i].Name, Bindings[i].ShaderStages}, Bindings + i);

            for (Uint32 i = 0; i < Count; ++i)
            {
                auto Iter = BindingMap.find(Key{RefBindings[i].Name, RefBindings[i].ShaderStages});
                EXPECT_TRUE(Iter != BindingMap.end());
                if (Iter == BindingMap.end())
                    continue;

                const auto& Lhs = *Iter->second;
                const auto& Rhs = RefBindings[i];

                EXPECT_EQ(Lhs.Register, Rhs.Register);
                EXPECT_EQ(Lhs.Space, Rhs.Space);
                EXPECT_EQ(Lhs.ArraySize, Rhs.ArraySize);
                EXPECT_EQ(Lhs.ResourceType, Rhs.ResourceType);
            }
        };

        constexpr Uint32 RuntimeArray = 0;
        switch (DeviceType)
        {
            case RENDER_DEVICE_TYPE_D3D11:
            {
                const PipelineResourceBinding RefBindings[] =
                    {
                        // clang-format off
                        {"g_DiffuseTexs",  SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0,   0, RuntimeArray},
                        {"g_NormalTexs",   SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0, 100, RuntimeArray},
                        {"g_SubpassInput", SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, PS,  0, 200, 1},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       PS,  0, 201, 1},
                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   1, 8},
                        {"g_RWTex2D",      SHADER_RESOURCE_TYPE_TEXTURE_UAV,      PS,  0,   0, 2},
                        {"g_Sampler",      SHADER_RESOURCE_TYPE_SAMPLER,          PS,  0,   0, 1},

                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   1, 8},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS,  0,   0, 1}
                        // clang-format on

                    };
                CompareBindings(RefBindings, _countof(RefBindings));
                break;
            }
            case RENDER_DEVICE_TYPE_D3D12:
            {
                const PipelineResourceBinding RefBindings[] =
                    {
                        // clang-format off
                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS_PS, 0, 0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS_PS, 0, 1, 8},
                        {"g_SubpassInput", SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, PS,    0, 0, 1},
                        {"g_TLAS",         SHADER_RESOURCE_TYPE_ACCEL_STRUCT,     PS,    0, 1, 1},
                        {"g_DiffuseTexs",  SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,    1, 0, RuntimeArray},
                        {"g_NormalTexs",   SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,    2, 0, RuntimeArray},
                        {"g_RWTex2D",      SHADER_RESOURCE_TYPE_TEXTURE_UAV,      PS,    3, 0, 2},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS_PS, 3, 0, 1}
                        // clang-format on
                    };
                CompareBindings(RefBindings, _countof(RefBindings));
                break;
            }
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
            {
                const PipelineResourceBinding RefBindings[] =
                    {
                        // clang-format off
                        {"g_DiffuseTexs",  SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0,   0, RuntimeArray},
                        {"g_NormalTexs",   SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0, 100, RuntimeArray},
                        {"g_SubpassInput", SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, PS,  0, 200, 1},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       PS,  0, 201, 1},
                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   1, 8},
                        {"g_RWTex2D",      SHADER_RESOURCE_TYPE_TEXTURE_UAV,      PS,  0,   0, 2},

                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   1, 8},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS,  0, 201, 1},
                        // clang-format on
                    };
                CompareBindings(RefBindings, _countof(RefBindings));
                break;
            }
            case RENDER_DEVICE_TYPE_VULKAN:
            {
                const PipelineResourceBinding RefBindings[] =
                    {
                        // clang-format off
                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS_PS, 0, 0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS_PS, 0, 1, 8},
                        {"g_DiffuseTexs",  SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,    0, 2, RuntimeArray},
                        {"g_NormalTexs",   SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,    0, 3, RuntimeArray},
                        {"g_SubpassInput", SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, PS,    0, 4, 1},
                        {"g_TLAS",         SHADER_RESOURCE_TYPE_ACCEL_STRUCT,     PS,    0, 5, 1},
                        {"g_RWTex2D",      SHADER_RESOURCE_TYPE_TEXTURE_UAV,      PS,    1, 0, 2},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS_PS, 1, 1, 1}
                        // clang-format on
                    };
                CompareBindings(RefBindings, _countof(RefBindings));
                break;
            }
            case RENDER_DEVICE_TYPE_METAL:
            {
                const PipelineResourceBinding RefBindings[] =
                    {
                        // clang-format off
                        {"g_DiffuseTexs",  SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0,   0, RuntimeArray},
                        {"g_NormalTexs",   SHADER_RESOURCE_TYPE_TEXTURE_SRV,      PS,  0, 100, RuntimeArray},
                        {"g_SubpassInput", SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, PS,  0, 200, 1},
                        {"g_RWTex2D",      SHADER_RESOURCE_TYPE_TEXTURE_UAV,      PS,  0, 201, 2},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       PS,  0, 203, 1},
                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  PS,  0,   1, 8},

                        {"ConstBuff_1",    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   0, 1},
                        {"PerObjectConst", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,  VS,  0,   1, 8},
                        {"g_TexelBuff",    SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS,  0,   0, 1},
                        {"VBPosition",     SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS,  0,  29, 1},
                        {"VBTexcoord",     SHADER_RESOURCE_TYPE_BUFFER_SRV,       VS,  0,  30, 1}
                        // clang-format on
                    };
                CompareBindings(RefBindings, _countof(RefBindings));
                break;
            }
            default:
                GTEST_FAIL() << "Unsupported device type";
        }
    }
}

using TestSamplersParamType = std::tuple<bool, SHADER_SOURCE_LANGUAGE, bool>;
class TestSamplers : public testing::TestWithParam<TestSamplersParamType>
{};

TEST_P(TestSamplers, GraphicsPipeline)
{
    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* const pEnv             = GPUTestingEnvironment::GetInstance();
    auto* const pDevice          = pEnv->GetDevice();
    auto* const pSwapChain       = pEnv->GetSwapChain();
    const auto& deviceCaps       = pDevice->GetDeviceInfo();
    auto* const pArchiverFactory = pEnv->GetArchiverFactory();
    auto* const pDearchiver      = pDevice->GetEngineFactory()->GetDearchiver();

    const auto& Param = GetParam();

    const auto UseImtblSamplers = std::get<0>(Param);
    const auto ShaderLang       = std::get<1>(Param);
    const auto UseSignature     = std::get<2>(Param);

    if (ShaderLang != SHADER_SOURCE_LANGUAGE_HLSL && deviceCaps.IsD3DDevice())
        GTEST_SKIP() << "Direct3D backends support HLSL only";

    if (!pDearchiver || !pArchiverFactory)
        GTEST_SKIP() << "Archiver library is not loaded";

    float ClearColor[] = {0.125, 0.75, 0.5, 0.5};
    RenderDrawCommandReference(pSwapChain, ClearColor);

    // Texture indices for vertex/shader bindings
    static constexpr size_t Tex2D_StaticIdx[] = {2, 10};
    static constexpr size_t Tex2D_MutIdx[]    = {0, 11};
    static constexpr size_t Tex2D_DynIdx[]    = {1, 9};

    static constexpr size_t Tex2DArr_StaticIdx[] = {7, 0};
    static constexpr size_t Tex2DArr_MutIdx[]    = {3, 5};
    static constexpr size_t Tex2DArr_DynIdx[]    = {9, 2};

    const Uint32 VSResArrId = 0;
    const Uint32 PSResArrId = deviceCaps.Features.SeparablePrograms ? 1 : 0;
    VERIFY_EXPR(deviceCaps.IsGLDevice() || PSResArrId != VSResArrId);


    // Prepare reference textures filled with different colors
    // Texture array sizes in the shader
    static constexpr Uint32 StaticTexArraySize  = 2;
    static constexpr Uint32 MutableTexArraySize = 4;
    static constexpr Uint32 DynamicTexArraySize = 3;

    ReferenceTextures RefTextures{
        3 + StaticTexArraySize + MutableTexArraySize + DynamicTexArraySize,
        128, 128,
        USAGE_DEFAULT,
        BIND_SHADER_RESOURCE,
        TEXTURE_VIEW_SHADER_RESOURCE //
    };

    static constexpr size_t Buff_StaticIdx[] = {2, 1};
    static constexpr size_t Buff_MutIdx[]    = {3, 0};
    static constexpr size_t Buff_DynIdx[]    = {5, 4};

    ReferenceBuffers RefBuffers{
        6,
        USAGE_DEFAULT,
        BIND_UNIFORM_BUFFER //
    };


    if (!UseImtblSamplers)
    {
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc{}, &pSampler);
        for (Uint32 i = 0; i < RefTextures.GetTextureCount(); ++i)
            RefTextures.GetView(i)->SetSampler(pSampler);
    }

    constexpr char PSOName[] = "Archiver sampler test";
    constexpr char PRSName[] = "SamplerTest - PRS";

    RefCntAutoPtr<ISerializationDevice> pSerializationDevice;
    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCreateInfo{}, &pSerializationDevice);
    ASSERT_NE(pSerializationDevice, nullptr);

    RefCntAutoPtr<IDeviceObjectArchive> pArchive;
    {
        RefCntAutoPtr<IArchiver> pArchiver;
        pArchiverFactory->CreateArchiver(pSerializationDevice, &pArchiver);
        ASSERT_NE(pArchiver, nullptr);

        // clang-format off
        constexpr ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Static",    SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Mut",       SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",       SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Static", SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut",    SamplerDesc{}},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn",    SamplerDesc{}}
        };
        // clang-format on

        ShaderMacroHelper Macros;

        auto PrepareMacros = [&](Uint32 s) {
            Macros.Clear();

            if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
                Macros.AddShaderMacro("float4", "vec4");

            Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
            Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
            Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

            // Add macros that define reference colors

            RefTextures.ClearUsedValues();
            Macros.AddShaderMacro("Tex2D_Static_Ref", RefTextures.GetColor(Tex2D_StaticIdx[s]));
            Macros.AddShaderMacro("Tex2D_Mut_Ref", RefTextures.GetColor(Tex2D_MutIdx[s]));
            Macros.AddShaderMacro("Tex2D_Dyn_Ref", RefTextures.GetColor(Tex2D_DynIdx[s]));

            RefBuffers.ClearUsedValues();
            Macros.AddShaderMacro("Buff_Static_Ref", RefBuffers.GetValue(Buff_StaticIdx[s]));
            Macros.AddShaderMacro("Buff_Mut_Ref", RefBuffers.GetValue(Buff_MutIdx[s]));
            Macros.AddShaderMacro("Buff_Dyn_Ref", RefBuffers.GetValue(Buff_DynIdx[s]));

            for (Uint32 i = 0; i < StaticTexArraySize; ++i)
                Macros.AddShaderMacro((std::string{"Tex2DArr_Static_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_StaticIdx[s] + i));

            for (Uint32 i = 0; i < MutableTexArraySize; ++i)
                Macros.AddShaderMacro((std::string{"Tex2DArr_Mut_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_MutIdx[s] + i));

            for (Uint32 i = 0; i < DynamicTexArraySize; ++i)
                Macros.AddShaderMacro((std::string{"Tex2DArr_Dyn_Ref"} + std::to_string(i)).c_str(), RefTextures.GetColor(Tex2DArr_DynIdx[s] + i));

            return static_cast<const ShaderMacro*>(Macros);
        };

        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/Archiver", &pShaderSourceFactory);

        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.UseCombinedTextureSamplers = deviceCaps.IsGLDevice();
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        ShaderCI.SourceLanguage = ShaderLang;
        switch (ShaderLang)
        {
            case SHADER_SOURCE_LANGUAGE_HLSL:
                ShaderCI.FilePath = "Samplers.hlsl";
                // Immutable sampler arrays are not allowed in 5.1, and DXC only supports 6.0+
                ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
                // Note that due to bug in D3D12 WARP, we have to use SM5.0 with old compiler
                ShaderCI.HLSLVersion = ShaderVersion{5, 0};
                break;
            case SHADER_SOURCE_LANGUAGE_GLSL:
                ShaderCI.FilePath = "Samplers.glsl";
                break;
            default:
                UNEXPECTED("Unexpected shader language");
        }

        auto PackFlags = GetDeviceBits();
        if (ShaderLang != SHADER_SOURCE_LANGUAGE_HLSL)
            PackFlags &= ~(ARCHIVE_DEVICE_DATA_FLAG_D3D11 | ARCHIVE_DEVICE_DATA_FLAG_D3D12);

        RefCntAutoPtr<IShader> pSerializedVS;
        {
            PrepareMacros(VSResArrId);
            ShaderCI.Macros          = Macros;
            ShaderCI.Desc.Name       = "Archiver.Samplers - VS";
            ShaderCI.EntryPoint      = ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL ? "main" : "VSMain";
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;

            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{PackFlags}, &pSerializedVS);
            ASSERT_NE(pSerializedVS, nullptr);
        }

        RefCntAutoPtr<IShader> pSerializedPS;
        {
            PrepareMacros(PSResArrId);
            ShaderCI.Macros          = Macros;
            ShaderCI.Desc.Name       = "Archiver.Samplers - PS";
            ShaderCI.EntryPoint      = ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL ? "main" : "PSMain";
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;

            pSerializationDevice->CreateShader(ShaderCI, ShaderArchiveInfo{PackFlags}, &pSerializedPS);
            ASSERT_NE(pSerializedPS, nullptr);
        }

        std::vector<ShaderResourceVariableDesc> Vars;
        std::vector<PipelineResourceDesc>       Resources;
        std::vector<PipelineResourceDesc>       Samplers;
        std::unordered_set<std::string>         StringPool;

        auto AddResourceOrVar = [&](const char* Name, Uint32 ArraySize, SHADER_RESOURCE_TYPE ResType, SHADER_RESOURCE_VARIABLE_TYPE VarType) //
        {
            auto Add = [&](SHADER_TYPE Stage) {
                if (UseSignature)
                {
                    auto Flags = PIPELINE_RESOURCE_FLAG_NONE;
                    if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL && ResType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
                        Flags = PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
                    Resources.emplace_back(Stage, Name, ArraySize, ResType, VarType, Flags);

                    if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL && ResType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
                    {
                        const auto it = StringPool.emplace(std::string{Name} + PipelineResourceSignatureDesc{}.CombinedSamplerSuffix).first;
                        Samplers.emplace_back(Stage, it->c_str(), ArraySize, SHADER_RESOURCE_TYPE_SAMPLER, VarType);
                    }
                }
                else
                {
                    Vars.emplace_back(Stage, Name, VarType);
                }
            };

            if (deviceCaps.Features.SeparablePrograms)
            {
                // Use separate variables for each stage
                Add(SHADER_TYPE_VERTEX);
                Add(SHADER_TYPE_PIXEL);
            }
            else
            {
                // Use one shared variable
                Add(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL);
            }
        };

        // clang-format off
        AddResourceOrVar("UniformBuff_Stat",  1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        AddResourceOrVar("UniformBuff_Mut",   1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        AddResourceOrVar("UniformBuff_Dyn",   1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

        AddResourceOrVar("g_Tex2D_Static",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (UseSignature)
            AddResourceOrVar("g_Tex2D_Mut",   1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE); // Default type
        AddResourceOrVar("g_Tex2D_Dyn",       1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);

        AddResourceOrVar("g_Tex2DArr_Static", 2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        AddResourceOrVar("g_Tex2DArr_Mut",    4, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        AddResourceOrVar("g_Tex2DArr_Dyn",    3, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
        // clang-format on

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = PSOName;

        RefCntAutoPtr<IPipelineResourceSignature> pSerializedPRSature;

        IPipelineResourceSignature* ppResourceSignatures[1] = {};
        if (UseSignature)
        {
            // Add samplers in the reverse order to make them use registers that are not
            // the same as texture registers.
            Resources.insert(Resources.end(), Samplers.rbegin(), Samplers.rend());

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                       = PRSName;
            PRSDesc.BindingIndex               = 0;
            PRSDesc.Resources                  = Resources.data();
            PRSDesc.NumResources               = StaticCast<Uint32>(Resources.size());
            PRSDesc.ImmutableSamplers          = UseImtblSamplers ? ImtblSamplers : nullptr;
            PRSDesc.NumImmutableSamplers       = UseImtblSamplers ? _countof(ImtblSamplers) : 0;
            PRSDesc.UseCombinedTextureSamplers = true;

            pSerializationDevice->CreatePipelineResourceSignature(PRSDesc, ResourceSignatureArchiveInfo{GetDeviceBits()}, &pSerializedPRSature);
            ASSERT_TRUE(pSerializedPRSature);

            ppResourceSignatures[0]               = pSerializedPRSature;
            PSOCreateInfo.ppResourceSignatures    = ppResourceSignatures;
            PSOCreateInfo.ResourceSignaturesCount = 1;
        }
        else
        {
            auto& ResourceLayout{PSODesc.ResourceLayout};
            ResourceLayout.DefaultVariableType  = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
            ResourceLayout.Variables            = Vars.data();
            ResourceLayout.NumVariables         = static_cast<Uint32>(Vars.size());
            ResourceLayout.ImmutableSamplers    = UseImtblSamplers ? ImtblSamplers : nullptr;
            ResourceLayout.NumImmutableSamplers = UseImtblSamplers ? _countof(ImtblSamplers) : 0;
        }

        PSOCreateInfo.pVS = pSerializedVS;
        PSOCreateInfo.pPS = pSerializedPS;

        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.NumRenderTargets  = 1;
        GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
        GraphicsPipeline.DSVFormat         = TEX_FORMAT_UNKNOWN;

        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        PipelineStateArchiveInfo ArchiveInfo;
        ArchiveInfo.DeviceFlags = PackFlags;
        RefCntAutoPtr<IPipelineState> pSerializedPSO;
        pSerializationDevice->CreateGraphicsPipelineState(PSOCreateInfo, ArchiveInfo, &pSerializedPSO);
        ASSERT_NE(pSerializedPSO, nullptr);
        ASSERT_TRUE(pArchiver->AddPipelineState(pSerializedPSO));

        RefCntAutoPtr<IDataBlob> pBlob;
        pArchiver->SerializeToBlob(&pBlob);
        ASSERT_NE(pBlob, nullptr);

        RefCntAutoPtr<IArchive> pSource{MakeNewRCObj<ArchiveMemoryImpl>{}(pBlob)};
        pDearchiver->CreateDeviceObjectArchive(pSource, &pArchive);
        ASSERT_NE(pArchive, nullptr);
    }

    RefCntAutoPtr<IPipelineState> pPSO;
    {
        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.Name         = PSOName;
        UnpackInfo.pArchive     = pArchive;
        UnpackInfo.pDevice      = pDevice;
        UnpackInfo.PipelineType = PIPELINE_TYPE_GRAPHICS;

        pDearchiver->UnpackPipelineState(UnpackInfo, &pPSO);
        ASSERT_NE(pPSO, nullptr);
    }

    RefCntAutoPtr<IPipelineResourceSignature> pSignature;
    if (UseSignature)
    {
        ResourceSignatureUnpackInfo UnpackInfo;
        UnpackInfo.Name     = PRSName;
        UnpackInfo.pArchive = pArchive;
        UnpackInfo.pDevice  = pDevice;

        pDearchiver->UnpackResourceSignature(UnpackInfo, &pSignature);
        ASSERT_NE(pSignature, nullptr);
    }

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    if (pSignature)
        pSignature->CreateShaderResourceBinding(&pSRB, false);
    else
        pPSO->CreateShaderResourceBinding(&pSRB, false);
    ASSERT_NE(pSRB, nullptr);

    auto BindResources = [&](SHADER_TYPE ShaderType) {
        const auto id = ShaderType == SHADER_TYPE_VERTEX ? VSResArrId : PSResArrId;

        if (pSignature)
        {
            pSignature->GetStaticVariableByName(ShaderType, "UniformBuff_Stat")->Set(RefBuffers.GetBuffer(Buff_StaticIdx[id]));
            pSignature->GetStaticVariableByName(ShaderType, "g_Tex2DArr_Static")->SetArray(RefTextures.GetViewObjects(Tex2DArr_StaticIdx[id]), 0, StaticTexArraySize);
            pSignature->GetStaticVariableByName(ShaderType, "g_Tex2D_Static")->Set(RefTextures.GetViewObjects(Tex2D_StaticIdx[id])[0]);
        }
        else
        {
            pPSO->GetStaticVariableByName(ShaderType, "UniformBuff_Stat")->Set(RefBuffers.GetBuffer(Buff_StaticIdx[id]));
            pPSO->GetStaticVariableByName(ShaderType, "g_Tex2DArr_Static")->SetArray(RefTextures.GetViewObjects(Tex2DArr_StaticIdx[id]), 0, StaticTexArraySize);
            pPSO->GetStaticVariableByName(ShaderType, "g_Tex2D_Static")->Set(RefTextures.GetViewObjects(Tex2D_StaticIdx[id])[0]);
        }

        pSRB->GetVariableByName(ShaderType, "UniformBuff_Mut")->Set(RefBuffers.GetBuffer(Buff_MutIdx[id]));
        pSRB->GetVariableByName(ShaderType, "g_Tex2DArr_Mut")->SetArray(RefTextures.GetViewObjects(Tex2DArr_MutIdx[id]), 0, MutableTexArraySize);
        pSRB->GetVariableByName(ShaderType, "g_Tex2D_Mut")->Set(RefTextures.GetViewObjects(Tex2D_MutIdx[id])[0]);

        pSRB->GetVariableByName(ShaderType, "UniformBuff_Dyn")->Set(RefBuffers.GetBuffer(Buff_DynIdx[id]));
        pSRB->GetVariableByName(ShaderType, "g_Tex2DArr_Dyn")->SetArray(RefTextures.GetViewObjects(Tex2DArr_DynIdx[id]), 0, DynamicTexArraySize);
        pSRB->GetVariableByName(ShaderType, "g_Tex2D_Dyn")->Set(RefTextures.GetViewObjects(Tex2D_DynIdx[id])[0]);
    };
    BindResources(SHADER_TYPE_VERTEX);
    BindResources(SHADER_TYPE_PIXEL);

    if (pSignature)
        pSignature->InitializeStaticSRBResources(pSRB);
    else
        pPSO->InitializeStaticSRBResources(pSRB);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearRenderTarget(ppRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pSwapChain->Present();
}

INSTANTIATE_TEST_SUITE_P(ArchiveTest,
                         TestSamplers,
                         testing::Combine(
                             testing::Values<bool>(true, false),
                             testing::Values<SHADER_SOURCE_LANGUAGE>(SHADER_SOURCE_LANGUAGE_HLSL, SHADER_SOURCE_LANGUAGE_GLSL),
                             testing::Values<bool>(true, false)),
                         [](const testing::TestParamInfo<TestSamplersParamType>& info) //
                         {
                             const auto UseImtblSamplers = std::get<0>(info.param);
                             const auto ShaderLang       = std::get<1>(info.param);
                             const auto UseSignature     = std::get<2>(info.param);

                             std::stringstream name_ss;
                             if (ShaderLang == SHADER_SOURCE_LANGUAGE_HLSL)
                                 name_ss << "HLSL";
                             else if (ShaderLang == SHADER_SOURCE_LANGUAGE_GLSL)
                                 name_ss << "GLSL";
                             if (UseImtblSamplers)
                                 name_ss << "__ImtblSamplers";
                             if (UseSignature)
                                 name_ss << "__Signature";

                             return name_ss.str();
                         });

} // namespace
