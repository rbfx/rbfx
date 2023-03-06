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

#include "DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceVariable.h"

void TestShaderResourceVariable_CInterface(IShaderResourceVariable* pVar)
{
    IShaderResourceVariable_Set(pVar, (struct IDeviceObject*)NULL, SET_SHADER_RESOURCE_FLAG_NONE);
    IShaderResourceVariable_SetArray(pVar, (struct IDeviceObject* const*)NULL, (Uint32)1, (Uint32)2, SET_SHADER_RESOURCE_FLAG_NONE);
    IShaderResourceVariable_SetBufferRange(pVar, (struct IDeviceObject*)NULL, (Uint64)0, (Uint64)16, (Uint32)1, SET_SHADER_RESOURCE_FLAG_NONE);
    IShaderResourceVariable_SetBufferOffset(pVar, (Uint32)1024, (Uint32)1);
    SHADER_RESOURCE_VARIABLE_TYPE Type = IShaderResourceVariable_GetType(pVar);
    (void)Type;
    ShaderResourceDesc ResDesc;
    IShaderResourceVariable_GetResourceDesc(pVar, &ResDesc);
    Uint32 Idx = IShaderResourceVariable_GetIndex(pVar);
    (void)Idx;
    struct IDeviceObject* pObj = IShaderResourceVariable_Get(pVar, (Uint32)1);
    (void)pObj;
}
