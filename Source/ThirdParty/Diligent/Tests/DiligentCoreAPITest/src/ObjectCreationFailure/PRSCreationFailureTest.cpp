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

#include <array>
#include <limits>

#include "GPUTestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static void TestCreatePRSFailure(PipelineResourceSignatureDesc CI, const char* ExpectedErrorSubstring)
{
    auto* const pEnv    = GPUTestingEnvironment::GetInstance();
    auto* const pDevice = pEnv->GetDevice();

    RefCntAutoPtr<IPipelineResourceSignature> pSignature;
    pEnv->SetErrorAllowance(2, "Errors below are expected: testing PRS creation failure\n");
    pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
    pDevice->CreatePipelineResourceSignature(CI, &pSignature);
    ASSERT_FALSE(pSignature);

    CI.Name = nullptr;
    pEnv->SetErrorAllowance(2);
    pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
    pDevice->CreatePipelineResourceSignature(CI, &pSignature);
    ASSERT_FALSE(pSignature);

    pEnv->SetErrorAllowance(0);
}

TEST(PRSCreationFailureTest, InvalidBindingIndex)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name         = "Invalid binding index";
    PRSDesc.BindingIndex = std::numeric_limits<decltype(PRSDesc.BindingIndex)>::max();
    TestCreatePRSFailure(PRSDesc, "Desc.BindingIndex (255) exceeds the maximum allowed value");
}

TEST(PRSCreationFailureTest, InvalidNumResources)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name         = "Invalid NumResources";
    PRSDesc.NumResources = std::numeric_limits<decltype(PRSDesc.NumResources)>::max();
    TestCreatePRSFailure(PRSDesc, "Desc.NumResources (4294967295) exceeds the maximum allowed value");
}

TEST(PRSCreationFailureTest, NullResources)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name         = "Null Resources";
    PRSDesc.NumResources = 10;
    PRSDesc.Resources    = nullptr;
    TestCreatePRSFailure(PRSDesc, "Desc.NumResources (10) is not zero, but Desc.Resources is null");
}

TEST(PRSCreationFailureTest, NullImmutableSamplers)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name                 = "Null Resources";
    PRSDesc.NumImmutableSamplers = 12;
    PRSDesc.ImmutableSamplers    = nullptr;
    TestCreatePRSFailure(PRSDesc, "Desc.NumImmutableSamplers (12) is not zero, but Desc.ImmutableSamplers is null");
}

TEST(PRSCreationFailureTest, NullCombinedSamplerSuffix)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name                       = "Null CombinedSamplerSuffix";
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.CombinedSamplerSuffix      = nullptr;
    TestCreatePRSFailure(PRSDesc, "Desc.UseCombinedTextureSamplers is true, but Desc.CombinedSamplerSuffix is null or empty");

    PRSDesc.Name                  = "Null CombinedSamplerSuffix 2";
    PRSDesc.CombinedSamplerSuffix = "";
    TestCreatePRSFailure(PRSDesc, "Desc.UseCombinedTextureSamplers is true, but Desc.CombinedSamplerSuffix is null or empty");
}

TEST(PRSCreationFailureTest, NullResourceName)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Null resource name";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, nullptr, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[1].Name must not be null");

    PRSDesc.Name      = "Null resource name 2";
    Resources[1].Name = "";
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[1].Name must not be empty");
}

TEST(PRSCreationFailureTest, UnknownResourceShaderStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Unknown resource ShaderStages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Buffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_UNKNOWN, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[1].ShaderStages must not be SHADER_TYPE_UNKNOWN");
}

TEST(PRSCreationFailureTest, ZeroArraySize)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Zero array size";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Buffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_Texture", 0, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[1].ArraySize must not be 0");
}

TEST(PRSCreationFailureTest, OverlappingStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Overlapping Shader Stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Multiple resources with name 'g_Texture' specify overlapping shader stages");
}

TEST(PRSCreationFailureTest, InvalidResourceFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Overlapping Shader Stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Buffer", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (COMBINED_SAMPLER|FORMATTED_BUFFER). Only the following flags are valid for a constant buffer: NO_DYNAMIC_BUFFERS, RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidTexSRVFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid Tex SRV Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Texture2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (NO_DYNAMIC_BUFFERS|COMBINED_SAMPLER|FORMATTED_BUFFER). Only the following flags are valid for a texture SRV: COMBINED_SAMPLER, RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidBuffSRVFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid Buff SRV Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Buffer", 1, SHADER_RESOURCE_TYPE_BUFFER_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (COMBINED_SAMPLER). Only the following flags are valid for a buffer SRV: NO_DYNAMIC_BUFFERS, FORMATTED_BUFFER, RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidTexUAVFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid Tex UAV Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Texture2", 1, SHADER_RESOURCE_TYPE_TEXTURE_UAV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (COMBINED_SAMPLER). Only the following flags are valid for a texture UAV: RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidBuffUAVFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid Buff UAV Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Buffer", 1, SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (COMBINED_SAMPLER|FORMATTED_BUFFER). Only the following flags are valid for a buffer UAV: NO_DYNAMIC_BUFFERS, FORMATTED_BUFFER, RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidSamplerFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid sampler Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX, "g_Sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Incorrect Desc.Resources[1].Flags (NO_DYNAMIC_BUFFERS|FORMATTED_BUFFER). Only the following flags are valid for a sampler: RUNTIME_ARRAY");
}

TEST(PRSCreationFailureTest, InvalidInputAttachmentFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid input attachment Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_InputAttachment", 1, SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    const char* ExpectedErrorSubstring;
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.ShaderResourceRuntimeArray)
        ExpectedErrorSubstring = "Incorrect Desc.Resources[1].Flags (RUNTIME_ARRAY). Only the following flags are valid for a input attachment: GENERAL_INPUT_ATTACHMENT";
    else
        ExpectedErrorSubstring = "Incorrect Desc.Resources[1].Flags (RUNTIME_ARRAY). The flag can only be used if ShaderResourceRuntimeArray device feature is enabled";

    TestCreatePRSFailure(PRSDesc, ExpectedErrorSubstring);
}

TEST(PRSCreationFailureTest, InvalidAccelStructFlag)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid accel struct Flag";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_AS", 1, SHADER_RESOURCE_TYPE_ACCEL_STRUCT, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    const char* ExpectedErrorSubstring;
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.RayTracing)
        ExpectedErrorSubstring = "Incorrect Desc.Resources[1].Flags (NO_DYNAMIC_BUFFERS). Only the following flags are valid for a acceleration structure: RUNTIME_ARRAY";
    else
        ExpectedErrorSubstring = "Incorrect Desc.Resources[1].ResourceType (ACCEL_STRUCT): ray tracing is not supported by device";

    TestCreatePRSFailure(PRSDesc, ExpectedErrorSubstring);
}

TEST(PRSCreationFailureTest, InvalidCombinedSamplerFlag)
{
    const auto& DeviceInfo = GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo();
    if (!(DeviceInfo.IsD3DDevice() || DeviceInfo.IsMetalDevice()))
    {
        GTEST_SKIP() << "Direct3D11, Direct3D12 and Metal only";
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name                       = "Invalid combined sampler Flag";
    PRSDesc.UseCombinedTextureSamplers = false;
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[0].Flags contain COMBINED_SAMPLER flag, but Desc.UseCombinedTextureSamplers is false");
}

TEST(PRSCreationFailureTest, InvalidAssignedSamplerResourceType)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid assigned sampler resource type";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Resource 'g_Texture_sampler' combined with texture 'g_Texture' is not a sampler");
}

TEST(PRSCreationFailureTest, InvalidAssignedSamplerStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid assigned sampler shader stage";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Texture 'g_Texture' is defined for the following shader stages: SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL, but sampler 'g_Texture_sampler' assigned to it uses only some of these stages: SHADER_TYPE_PIXEL");
}

TEST(PRSCreationFailureTest, InvalidAssignedSamplerVarType)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid assigned sampler var type";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "The type (mutable) of texture resource 'g_Texture' does not match the type (static) of sampler 'g_Texture_sampler' that is assigned to it");
}

#if 0 // Unassigned sampler is a warning
TEST(PRSCreationFailureTest, UnassignedSampler)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Unassigned sampler";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_Texture2_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Sampler 'g_Texture2_sampler' is not assigned to any texture");
}
#endif

TEST(PRSCreationFailureTest, NullImmutableSamplerName)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Null Immutable Sampler Name";
    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_PIXEL, "g_ImmutableSampler", SamplerDesc{}},
        {SHADER_TYPE_PIXEL, nullptr, SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);
    TestCreatePRSFailure(PRSDesc, "Desc.ImmutableSamplers[1].SamplerOrTextureName must not be null");

    PRSDesc.Name = "Null Immutable Sampler Name 2";

    ImmutableSamplers[1].SamplerOrTextureName = "";
    TestCreatePRSFailure(PRSDesc, "Desc.ImmutableSamplers[1].SamplerOrTextureName must not be empty");
}

TEST(PRSCreationFailureTest, OverlappingImmutableSamplerStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Overlapping Immutable Sampler Stages";
    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "g_ImmutableSampler", SamplerDesc{}},
        {SHADER_TYPE_PIXEL | SHADER_TYPE_HULL, "g_ImmutableSampler", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);
    TestCreatePRSFailure(PRSDesc, "Multiple immutable samplers with name 'g_ImmutableSampler' specify overlapping shader stages.");
}

TEST(PRSCreationFailureTest, UnknownImmutableSamplerShareStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Unknown Immutable Sampler ShaderStages";
    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_PIXEL, "g_ImmutableSampler", SamplerDesc{}},
        {SHADER_TYPE_UNKNOWN, "g_ImmutableSampler2", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);
    TestCreatePRSFailure(PRSDesc, "Desc.ImmutableSamplers[1].ShaderStages must not be SHADER_TYPE_UNKNOWN");
}



TEST(PRSCreationFailureTest, NonSeparableProgs_ResourceStages)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP() << "This test is specific for non-separable programs";
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Non-separable progs - resource stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "there are separate resources with the name 'g_Texture' in shader stages SHADER_TYPE_PIXEL and SHADER_TYPE_VERTEX");
}

TEST(PRSCreationFailureTest, NonSeparableProgs_ImtblSamplerStages)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Features.SeparablePrograms)
    {
        GTEST_SKIP() << "This test is specific for non-separable programs";
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Non-separable progs - immutable sampler stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_VERTEX, "g_Texture", SamplerDesc{}},
        {SHADER_TYPE_PIXEL, "g_Texture", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);
    TestCreatePRSFailure(PRSDesc, "there are separate immutable samplers with the name 'g_Texture' in shader stages SHADER_TYPE_PIXEL and SHADER_TYPE_VERTEX");
}


TEST(PRSCreationFailureTest, D3D12_MultiStageResources)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D12)
    {
        GTEST_SKIP() << "This test is specific for Direct3D12";
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "D3D12 - multi stage resources";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "separate resources with the name 'g_Texture' in shader stages SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL and SHADER_TYPE_HULL, SHADER_TYPE_DOMAIN");
}

TEST(PRSCreationFailureTest, D3D12_MultiStageImtblSamplers)
{
    if (GPUTestingEnvironment::GetInstance()->GetDevice()->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D12)
    {
        GTEST_SKIP() << "This test is specific for Direct3D12";
    }

    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "D3D12 - multi stage immutable samplers";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
    PRSDesc.Resources    = Resources;
    PRSDesc.NumResources = _countof(Resources);

    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN, "g_Texture_sampler", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

    TestCreatePRSFailure(PRSDesc, "separate immutable samplers with the name 'g_Texture_sampler' in shader stages SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL and SHADER_TYPE_HULL, SHADER_TYPE_DOMAIN");
}

#if 0 // Unassigned immutable sampler is a warning
TEST(PRSCreationFailureTest, UnassignedImmutableSampler)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Unassigned immutable sampler";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};
    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_PIXEL, "g_Texture", SamplerDesc{}},
        {SHADER_TYPE_PIXEL, "g_Texture2", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Immutable sampler 'g_Texture2' is not assigned to any texture or sampler");
}
#endif

TEST(PRSCreationFailureTest, InvalidImmutableSamplerStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid immutable sampler stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};
    ImmutableSamplerDesc ImmutableSamplers[]{
        {SHADER_TYPE_VERTEX, "g_Texture", SamplerDesc{}}};
    PRSDesc.ImmutableSamplers    = ImmutableSamplers;
    PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Texture 'g_Texture' is defined for the following shader stages: SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL, but immutable sampler that is assigned to it uses only some of these stages: SHADER_TYPE_VERTEX");
}

TEST(PRSCreationFailureTest, InvalidInputAttachmentStages)
{
    PipelineResourceSignatureDesc PRSDesc;
    PRSDesc.Name = "Invalid input attachment stages";
    PipelineResourceDesc Resources[]{
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_InputAttachment", 1, SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};

    PRSDesc.UseCombinedTextureSamplers = true;
    PRSDesc.Resources                  = Resources;
    PRSDesc.NumResources               = _countof(Resources);
    TestCreatePRSFailure(PRSDesc, "Desc.Resources[0].ResourceType (INPUT_ATTACHMENT) is only supported in pixel shader but ShaderStages are SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL");
}

} // namespace
