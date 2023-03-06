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

#include "DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"

void TestPipelineState_CInterface(IPipelineState* pPSO)
{
    const GraphicsPipelineDesc*   pGDesc  = IPipelineState_GetGraphicsPipelineDesc(pPSO);
    const RayTracingPipelineDesc* pRTDesc = IPipelineState_GetRayTracingPipelineDesc(pPSO);
    (void)pGDesc;
    (void)pRTDesc;

    IPipelineState_BindStaticResources(pPSO, (Uint32)0, (IResourceMapping*)NULL, (Uint32)1);

    Uint32                   VarCount = IPipelineState_GetStaticVariableCount(pPSO, SHADER_TYPE_UNKNOWN);
    IShaderResourceVariable* pSRV1    = IPipelineState_GetStaticVariableByName(pPSO, SHADER_TYPE_UNKNOWN, "Resource name");
    IShaderResourceVariable* pSRV2    = IPipelineState_GetStaticVariableByIndex(pPSO, SHADER_TYPE_UNKNOWN, (Uint32)1);
    (void)VarCount;
    (void)pSRV1;
    (void)pSRV2;

    IPipelineState_CreateShaderResourceBinding(pPSO, (IShaderResourceBinding**)NULL, true);

    IPipelineState_CopyStaticResources(pPSO, (IPipelineState*)NULL);

    bool Compatible = IPipelineState_IsCompatibleWith(pPSO, (IPipelineState*)NULL);
    (void)Compatible;

    IPipelineState_InitializeStaticSRBResources(pPSO, (struct IShaderResourceBinding*)NULL);
}
