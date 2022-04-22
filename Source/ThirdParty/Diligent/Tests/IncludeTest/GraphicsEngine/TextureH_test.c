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

#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

void TestTexture_CInterface(ITexture* pTexture)
{
    const TextureDesc* pDesc = ITexture_GetDesc(pTexture);
    (void)pDesc;
    ITexture_CreateView(pTexture, (const TextureViewDesc*)NULL, (ITextureView**)NULL);
    ITextureView* pView = ITexture_GetDefaultView(pTexture, TEXTURE_VIEW_UNDEFINED);
    (void)pView;
    Uint64 Handle = ITexture_GetNativeHandle(pTexture);
    (void)Handle;
    ITexture_SetState(pTexture, RESOURCE_STATE_UNKNOWN);
    RESOURCE_STATE State = ITexture_GetState(pTexture);
    (void)State;
    const SparseTextureProperties* pSparseProps = ITexture_GetSparseProperties(pTexture);
    (void)pSparseProps;
}
