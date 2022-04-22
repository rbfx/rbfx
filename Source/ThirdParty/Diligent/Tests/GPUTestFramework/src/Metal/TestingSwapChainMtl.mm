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

#include "Metal/TestingSwapChainMtl.hpp"
#include "Metal/TestingEnvironmentMtl.hpp"

#include "RenderDeviceMtl.h"
#include "DeviceContextMtl.h"
#include "TextureViewMtl.h"
#include "TextureMtl.h"

namespace Diligent
{

namespace Testing
{

TestingSwapChainMtl::TestingSwapChainMtl(IReferenceCounters*    pRefCounters,
                                         TestingEnvironmentMtl* pEnv,
                                         const SwapChainDesc&   SCDesc) :
    TBase //
    {
        pRefCounters,
        pEnv->GetDevice(),
        pEnv->GetDeviceContext(),
        SCDesc //
    }
{
    auto mtlDevice = pEnv->GetMtlDevice();
    m_MtlStagingBuffer =
        [mtlDevice newBufferWithLength:SCDesc.Width * SCDesc.Height * 4
                   options:MTLResourceStorageModeManaged];
}

TestingSwapChainMtl::~TestingSwapChainMtl()
{
    [m_MtlStagingBuffer release];
}

void TestingSwapChainMtl::TakeSnapshot(ITexture* pCopyFrom)
{
    auto* pEnv = TestingEnvironmentMtl::GetInstance();
    auto mtlCommandQueue = pEnv->GetMtlCommandQueue();

    id<MTLTexture> mtlTexture = nil;
    if (pCopyFrom != nullptr)
    {
        auto* pSrcTexMtl = ClassPtrCast<ITextureMtl>(pCopyFrom);
        mtlTexture = (id<MTLTexture>)pSrcTexMtl->GetMtlResource();

        VERIFY_EXPR(m_SwapChainDesc.Width == pSrcTexMtl->GetDesc().Width);
        VERIFY_EXPR(m_SwapChainDesc.Height == pSrcTexMtl->GetDesc().Height);
        VERIFY_EXPR(m_SwapChainDesc.ColorBufferFormat == pSrcTexMtl->GetDesc().Format);
    }
    else
    {
        auto* pRTV = ClassPtrCast<ITextureViewMtl>(GetCurrentBackBufferRTV());
        mtlTexture = pRTV->GetMtlTexture();
    }
    m_ReferenceDataPitch = m_SwapChainDesc.Height * 4;
    m_ReferenceData.resize(m_SwapChainDesc.Width * m_ReferenceDataPitch);

    @autoreleasepool
    {
        // Command buffer is autoreleased
        auto commandBuffer = [mtlCommandQueue commandBuffer];
        // Command encoder is autoreleased
        auto blitEncoder   = [commandBuffer blitCommandEncoder];
        [blitEncoder copyFromTexture:mtlTexture
            sourceSlice:0
            sourceLevel:0
            sourceOrigin:MTLOrigin{0,0,0}
            sourceSize:MTLSize{m_SwapChainDesc.Width, m_SwapChainDesc.Height, 1}
            toBuffer:m_MtlStagingBuffer
            destinationOffset:0
            destinationBytesPerRow:m_ReferenceDataPitch
            destinationBytesPerImage:0];
        [blitEncoder synchronizeResource:m_MtlStagingBuffer];
        [blitEncoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        memcpy(m_ReferenceData.data(), [m_MtlStagingBuffer contents], m_ReferenceData.size());
    }
}

void CreateTestingSwapChainMtl(TestingEnvironmentMtl* pEnv,
                               const SwapChainDesc&   SCDesc,
                               ISwapChain**           ppSwapChain)
{
    TestingSwapChainMtl* pTestingSC{MakeNewRCObj<TestingSwapChainMtl>()(pEnv, SCDesc)};
    pTestingSC->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
}

} // namespace Testing

} // namespace Diligent
