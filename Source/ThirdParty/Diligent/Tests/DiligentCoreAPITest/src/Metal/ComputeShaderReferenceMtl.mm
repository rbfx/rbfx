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

#include "Metal/TestingEnvironmentMtl.hpp"
#include "Metal/TestingSwapChainMtl.hpp"

#include "DeviceContextMtl.h"
#include "TextureViewMtl.h"

#include "InlineShaders/ComputeShaderTestMSL.h"

namespace Diligent
{

namespace Testing
{

void ComputeShaderReferenceMtl(ISwapChain* pSwapChain)
{
    auto* const pEnv      = TestingEnvironmentMtl::GetInstance();
    auto const  mtlDevice = pEnv->GetMtlDevice();

    @autoreleasepool
    {
        // Autoreleased
        auto* progSrc = [NSString stringWithUTF8String:MSL::FillTextureCS.c_str()];
        NSError *errors = nil; // Autoreleased
        id <MTLLibrary> library = [mtlDevice newLibraryWithSource:progSrc
                                   options:nil
                                   error:&errors];
        ASSERT_TRUE(library != nil);
        [library autorelease];

        id <MTLFunction> computeFunc = [library newFunctionWithName:@"CSMain"];
        ASSERT_TRUE(computeFunc != nil);
        [computeFunc autorelease];

        auto* computePipeline = [mtlDevice newComputePipelineStateWithFunction:computeFunc error:&errors];
        ASSERT_TRUE(computePipeline != nil);
        [computePipeline autorelease];

        auto* pTestingSwapChainMtl = ClassPtrCast<TestingSwapChainMtl>(pSwapChain);
        auto* pUAV = pTestingSwapChainMtl->GetCurrentBackBufferUAV();
        auto* mtlTexture = ClassPtrCast<ITextureViewMtl>(pUAV)->GetMtlTexture();
        const auto& SCDesc = pTestingSwapChainMtl->GetDesc();

        auto* mtlCommandQueue = pEnv->GetMtlCommandQueue();

        // Command buffer is autoreleased
        id <MTLCommandBuffer> mtlCommandBuffer = [mtlCommandQueue commandBuffer];
        // Command encoder is autoreleased
        auto* cmdEncoder = [mtlCommandBuffer computeCommandEncoder];
        ASSERT_TRUE(cmdEncoder != nil);

        [cmdEncoder setComputePipelineState:computePipeline];
        [cmdEncoder setTexture:mtlTexture atIndex:0];
        [cmdEncoder dispatchThreadgroups:MTLSizeMake((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1)
                   threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];

        [cmdEncoder endEncoding];
        [mtlCommandBuffer commit];
    }
}

} // namespace Testing

} // namespace Diligent
