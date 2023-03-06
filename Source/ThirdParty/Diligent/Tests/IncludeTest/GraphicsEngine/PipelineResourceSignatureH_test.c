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

#include "DiligentCore/Graphics/GraphicsEngine/interface/PipelineResourceSignature.h"

void TestPipelineResourceSignature(struct IPipelineResourceSignature* pSign)
{
    IPipelineResourceSignature_CreateShaderResourceBinding(pSign, (struct IShaderResourceBinding**)NULL, true);

    struct IShaderResourceVariable* pVar1 = IPipelineResourceSignature_GetStaticVariableByName(pSign, SHADER_TYPE_UNKNOWN, "name");
    (void)pVar1;

    struct IShaderResourceVariable* pVar2 = IPipelineResourceSignature_GetStaticVariableByIndex(pSign, SHADER_TYPE_UNKNOWN, 0);
    (void)pVar2;

    Uint32 Count = IPipelineResourceSignature_GetStaticVariableCount(pSign, SHADER_TYPE_UNKNOWN);
    (void)Count;

    IPipelineResourceSignature_BindStaticResources(pSign, SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, (struct IResourceMapping*)NULL, BIND_SHADER_RESOURCES_UPDATE_STATIC);

    bool Comp = IPipelineResourceSignature_IsCompatibleWith(pSign, (const struct IPipelineResourceSignature*)NULL);
    (void)Comp;

    IPipelineResourceSignature_InitializeStaticSRBResources(pSign, (struct IShaderResourceBinding*)NULL);

    IPipelineResourceSignature_CopyStaticResources(pSign, (struct IPipelineResourceSignature*)NULL);
}
