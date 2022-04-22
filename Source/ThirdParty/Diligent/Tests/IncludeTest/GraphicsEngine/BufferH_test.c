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

#include "DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"

void TestBuffer_CInterface(IBuffer* pBuffer)
{
    const BufferDesc* pDesc = IBuffer_GetDesc(pBuffer);
    (void)pDesc;
    IBuffer_CreateView(pBuffer, (const BufferViewDesc*)NULL, (IBufferView**)NULL);
    IBufferView* pView = IBuffer_GetDefaultView(pBuffer, BUFFER_VIEW_UNDEFINED);
    (void)pView;
    Uint64 Handle = IBuffer_GetNativeHandle(pBuffer);
    (void)Handle;
    IBuffer_SetState(pBuffer, RESOURCE_STATE_UNKNOWN);
    RESOURCE_STATE State = IBuffer_GetState(pBuffer);
    (void)State;
    MEMORY_PROPERTIES MemProps = IBuffer_GetMemoryProperties(pBuffer);
    (void)MemProps;
    IBuffer_FlushMappedRange(pBuffer, (Uint64)0, (Uint64)128);
    IBuffer_InvalidateMappedRange(pBuffer, (Uint64)0, (Uint64)128);
    SparseBufferProperties SparseProps = IBuffer_GetSparseProperties(pBuffer);
    (void)SparseProps;
}
