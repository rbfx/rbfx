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

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"
#include "ShaderMacroHelper.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/ThreadgroupMemoryMtl.h"
#include "RayTracingTestConstants.hpp"

#include <Metal/Metal.h>
#include "DeviceContextMtl.h"

namespace Diligent
{
namespace Testing
{

void SetComputeThreadgroupMemoryLengthReferenceMtl(ISwapChain* pSwapChain);

} // namespace Testing
} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ThreadgroupMemoryTest, SetComputeThreadgroupMemoryLength)
{
    auto*       pEnv       = GPUTestingEnvironment::GetInstance();
    auto*       pDevice    = pEnv->GetDevice();
    const auto& deviceInfo = pDevice->GetDeviceInfo();
    if (!deviceInfo.IsMetalDevice())
        GTEST_SKIP();

    auto*       pSwapChain = pEnv->GetSwapChain();
    auto*       pContext   = pEnv->GetDeviceContext();
    const auto& SCDesc     = pSwapChain->GetDesc();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        SetComputeThreadgroupMemoryLengthReferenceMtl(pSwapChain);
        pTestingSwapChain->TakeSnapshot();
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    RefCntAutoPtr<IShader> pCS;
    {
        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_MSL;
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.Desc.Name       = "CS";
        ShaderCI.EntryPoint      = "CSmain";
        ShaderCI.Source          = MSL::SetComputeThreadgroupMemoryLength_CS.c_str();

        pDevice->CreateShader(ShaderCI, &pCS);
        ASSERT_NE(pCS, nullptr);
    }

    ComputePipelineStateCreateInfo PSOCreateInfo;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    PSOCreateInfo.pCS                                        = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB);
    ASSERT_NE(pSRB, nullptr);

    pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_OutImage")->Set(pTestingSwapChain->GetCurrentBackBufferUAV());

    const Uint32                     LocalSize = 8;
    RefCntAutoPtr<IDeviceContextMtl> pContextMtl{pContext, IID_DeviceContextMtl};
    pContextMtl->SetComputeThreadgroupMemoryLength(LocalSize * LocalSize * sizeof(float) * 4, 0);

    DispatchComputeAttribs dispatchAttrs;
    dispatchAttrs.ThreadGroupCountX = (SCDesc.Width + LocalSize - 1) / LocalSize;
    dispatchAttrs.ThreadGroupCountY = (SCDesc.Height + LocalSize - 1) / LocalSize;

    dispatchAttrs.MtlThreadGroupSizeX = LocalSize;
    dispatchAttrs.MtlThreadGroupSizeY = LocalSize;
    dispatchAttrs.MtlThreadGroupSizeZ = 1;

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->DispatchCompute(dispatchAttrs);

    pSwapChain->Present();
}

} // namespace
