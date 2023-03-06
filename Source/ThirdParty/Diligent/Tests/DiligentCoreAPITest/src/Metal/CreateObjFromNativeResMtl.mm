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

#include <Metal/Metal.h>

#include "RenderDeviceMtl.h"
#include "TextureMtl.h"
#include "BufferMtl.h"
#include "GPUTestingEnvironment.hpp"

#include "Metal/CreateObjFromNativeResMtl.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

void TestCreateObjFromNativeResMtl::CreateTexture(Diligent::ITexture* pTexture)
{
    RefCntAutoPtr<IRenderDeviceMtl> pDeviceMtl{m_pDevice, IID_RenderDeviceMtl};
    RefCntAutoPtr<ITextureMtl>      pTextureMtl{pTexture, IID_TextureMtl};
    ASSERT_NE(pDeviceMtl, nullptr);
    ASSERT_NE(pTextureMtl, nullptr);

    const auto& SrcTexDesc = pTexture->GetDesc();

    auto mtlHandle = (id<MTLTexture>) pTextureMtl->GetMtlResource();
    ASSERT_NE(mtlHandle, nil);

    RefCntAutoPtr<ITexture> pAttachedTexture;
    pDeviceMtl->CreateTextureFromMtlResource(mtlHandle, RESOURCE_STATE_UNKNOWN, &pAttachedTexture);
    ASSERT_NE(pAttachedTexture, nullptr);

    auto TestTexDesc = pAttachedTexture->GetDesc();

    const auto& SrcFmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
    if (SrcFmtAttribs.IsTypeless)
    {
        const auto& TestFmtAttribs = GetTextureFormatAttribs(TestTexDesc.Format);
        EXPECT_EQ(TestFmtAttribs.NumComponents, SrcFmtAttribs.NumComponents);
        EXPECT_EQ(TestFmtAttribs.ComponentSize, SrcFmtAttribs.ComponentSize);
        TestTexDesc.Format = SrcTexDesc.Format;
    }
    if (TestTexDesc.BindFlags & BIND_INPUT_ATTACHMENT)
    {
        EXPECT_TRUE((TestTexDesc.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) && (TestTexDesc.BindFlags & BIND_SHADER_RESOURCE));
        if ((SrcTexDesc.BindFlags & BIND_INPUT_ATTACHMENT) == 0)
            TestTexDesc.BindFlags &= ~BIND_INPUT_ATTACHMENT;
    }
    EXPECT_EQ(TestTexDesc, SrcTexDesc);
    EXPECT_STREQ(TestTexDesc.Name, SrcTexDesc.Name);
    RefCntAutoPtr<ITextureMtl> pAttachedTextureMtl(pAttachedTexture, IID_TextureMtl);
    ASSERT_NE(pAttachedTextureMtl, nullptr);
    EXPECT_EQ(pAttachedTextureMtl->GetMtlResource(), mtlHandle);
    EXPECT_EQ((id<MTLTexture>) pAttachedTextureMtl->GetNativeHandle(), mtlHandle);
}

void TestCreateObjFromNativeResMtl::CreateBuffer(Diligent::IBuffer* pBuffer)
{
    RefCntAutoPtr<IRenderDeviceMtl> pDeviceMtl{m_pDevice, IID_RenderDeviceMtl};
    RefCntAutoPtr<IBufferMtl>       pBufferMtl{pBuffer, IID_BufferMtl};
    ASSERT_NE(pDeviceMtl, nullptr);
    ASSERT_NE(pBufferMtl, nullptr);

    auto mtlBufferHandle = pBufferMtl->GetMtlResource();
    ASSERT_NE(mtlBufferHandle, nil);

    const auto& SrcBuffDesc = pBuffer->GetDesc();

    RefCntAutoPtr<IBuffer> pBufferFromNativeMtlHandle;
    pDeviceMtl->CreateBufferFromMtlResource(mtlBufferHandle, SrcBuffDesc, RESOURCE_STATE_UNKNOWN, &pBufferFromNativeMtlHandle);
    ASSERT_NE(pBufferFromNativeMtlHandle, nullptr);

    const auto& TestBufferDesc = pBufferFromNativeMtlHandle->GetDesc();
    EXPECT_EQ(TestBufferDesc, SrcBuffDesc);
    EXPECT_STREQ(TestBufferDesc.Name, SrcBuffDesc.Name);

    RefCntAutoPtr<IBufferMtl> pTestBufferMtl(pBufferFromNativeMtlHandle, IID_BufferMtl);
    ASSERT_NE(pTestBufferMtl, nullptr);
    EXPECT_EQ(pTestBufferMtl->GetMtlResource(), mtlBufferHandle);
    EXPECT_EQ((id<MTLBuffer>) pTestBufferMtl->GetNativeHandle(), mtlBufferHandle);
}

} // namespace Testing

} // namespace Diligent
