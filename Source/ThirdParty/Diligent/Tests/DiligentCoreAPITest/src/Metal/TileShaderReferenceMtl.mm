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

#include "InlineShaders/TileShaderTestMSL.h"

namespace Diligent
{

namespace Testing
{

void TileShaderDrawReferenceMtl(ISwapChain* pSwapChain)
{
    auto* const pEnv      = TestingEnvironmentMtl::GetInstance();
    auto const  mtlDevice = pEnv->GetMtlDevice();

    if (@available(macos 11.0, ios 11.0, *))
    {
        @autoreleasepool
        {
            auto* progSrc = [NSString stringWithUTF8String: MSL::TileShaderTest1.c_str()];
            NSError *errors = nil; // Autoreleased
            id <MTLLibrary> library = [mtlDevice newLibraryWithSource: progSrc
                                                              options: nil
                                                                error: &errors];
            ASSERT_TRUE(library != nil);
            [library autorelease];

            auto* vsFunc = [library newFunctionWithName: @"VSmain"];
            ASSERT_TRUE(vsFunc != nil);
            [vsFunc autorelease];
            auto* psFunc = [library newFunctionWithName: @"PSmain"];
            ASSERT_TRUE(psFunc != nil);
            [psFunc autorelease];
            auto* tlsFunc = [library newFunctionWithName: @"TLSmain"];
            ASSERT_TRUE(tlsFunc != nil);
            [tlsFunc autorelease];

            auto* pTestingSwapChainMtl = ClassPtrCast<TestingSwapChainMtl>(pSwapChain);
            const auto& SCDesc = pTestingSwapChainMtl->GetDesc();
            auto* pRTV = pTestingSwapChainMtl->GetCurrentBackBufferRTV();
            auto* mtlBackBuffer = ClassPtrCast<ITextureViewMtl>(pRTV)->GetMtlTexture();

            auto* renderPipeDesc = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
            renderPipeDesc.vertexFunction = vsFunc;
            renderPipeDesc.fragmentFunction = psFunc;
            renderPipeDesc.colorAttachments[0].pixelFormat = mtlBackBuffer.pixelFormat;

            auto* renderPipeline = [mtlDevice newRenderPipelineStateWithDescriptor:renderPipeDesc error:&errors];
            ASSERT_TRUE(renderPipeline != nil);
            [renderPipeline autorelease];

            auto* tilePipeDesc = [[[MTLTileRenderPipelineDescriptor alloc] init] autorelease];
            tilePipeDesc.tileFunction = tlsFunc;
            tilePipeDesc.colorAttachments[0].pixelFormat = mtlBackBuffer.pixelFormat;

            auto* tilePipeline = [mtlDevice newRenderPipelineStateWithTileDescriptor: tilePipeDesc
                                                                             options: MTLPipelineOptionNone
                                                                          reflection: nil
                                                                               error: &errors];
            ASSERT_TRUE(tilePipeline != nil);
            [tilePipeline autorelease];

            auto* renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor]; // Autoreleased
            renderPassDesc.colorAttachments[0].texture     = mtlBackBuffer;
            renderPassDesc.colorAttachments[0].loadAction  = MTLLoadActionClear;
            renderPassDesc.colorAttachments[0].clearColor  = MTLClearColorMake(0.0, 0.7, 1.0, 1.0);
            renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

            auto* mtlCommandQueue = pEnv->GetMtlCommandQueue();

            // Command buffer is autoreleased
            auto* mtlCommandBuffer = [mtlCommandQueue commandBuffer];

            // Render command encoder is autoreleased
            auto* renderEncoder = [mtlCommandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
            ASSERT_TRUE(renderEncoder != nil);

            [renderEncoder setViewport:MTLViewport{0, 0, (double)SCDesc.Width, (double)SCDesc.Height, 0, 1}];

            [renderEncoder setRenderPipelineState:renderPipeline];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

            [renderEncoder setRenderPipelineState:tilePipeline];
            [renderEncoder dispatchThreadsPerTile:MTLSizeMake(1,1,1)];

            [renderEncoder endEncoding];

            [mtlCommandBuffer commit];
            [mtlCommandBuffer waitUntilCompleted];

        } // @autoreleasepool
    }
}

} // namespace Testing

} // namespace Diligent
