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

#include "InlineShaders/RasterizationRateMapTestMSL.h"
#include "VariableShadingRateTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

using VRSTestingConstants::TextureBased::GenColRowFp32;

void RasterizationRateMapReferenceMtl(ISwapChain* pSwapChain)
{
    auto* const pEnv      = TestingEnvironmentMtl::GetInstance();
    auto const  mtlDevice = pEnv->GetMtlDevice();

    @autoreleasepool
    {
        if (@available(macos 10.15.4, ios 13.0, *))
        {
            auto* pTestingSwapChainMtl = ClassPtrCast<TestingSwapChainMtl>(pSwapChain);
            const auto& SCDesc = pTestingSwapChainMtl->GetDesc();

            auto* pRTV = pTestingSwapChainMtl->GetCurrentBackBufferRTV();
            auto* mtlBackBuffer = ClassPtrCast<ITextureViewMtl>(pRTV)->GetMtlTexture();

            id<MTLRenderPipelineState> mtlPass1PSO;
            {
                auto* progSrc = [NSString stringWithUTF8String:MSL::RasterRateMapTest_Pass1.c_str()]; // Autoreleased
                NSError* errors = nil; // Autoreleased
                id<MTLLibrary> library = [[mtlDevice newLibraryWithSource:progSrc
                                                                  options:nil
                                                                    error:&errors] autorelease];
                if (library == nil)
                    LOG_ERROR_AND_THROW("Failed to create Metal library: ", [errors.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);

                id<MTLFunction> vertFunc = [[library newFunctionWithName:@"VSmain"] autorelease];
                id<MTLFunction> fragFunc = [[library newFunctionWithName:@"PSmain"] autorelease];
                if (vertFunc == nil)
                    LOG_ERROR_AND_THROW("Unable to find vertex function");
                if (fragFunc == nil)
                    LOG_ERROR_AND_THROW("Unable to find fragment function");

                MTLRenderPipelineDescriptor* renderPipelineDesc = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
                renderPipelineDesc.vertexFunction   = vertFunc;
                renderPipelineDesc.fragmentFunction = fragFunc;

                renderPipelineDesc.colorAttachments[0].pixelFormat = mtlBackBuffer.pixelFormat;

                mtlPass1PSO = [mtlDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                                                                        error:&errors];
            }

            id<MTLRenderPipelineState> mtlPass2PSO;
            {
                auto* progSrc = [NSString stringWithUTF8String:MSL::RasterRateMapTest_Pass2.c_str()]; // Autoreleased
                NSError* errors = nil; // Autoreleased
                id<MTLLibrary> library = [[mtlDevice newLibraryWithSource:progSrc
                                                                  options:nil
                                                                    error:&errors] autorelease];
                if (library == nil)
                    LOG_ERROR_AND_THROW("Failed to create Metal library: ", [errors.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding]);

                id<MTLFunction> vertFunc = [[library newFunctionWithName:@"VSmain"] autorelease];
                id<MTLFunction> fragFunc = [[library newFunctionWithName:@"PSmain"] autorelease];
                if (vertFunc == nil)
                    LOG_ERROR_AND_THROW("Unable to find vertex function");
                if (fragFunc == nil)
                    LOG_ERROR_AND_THROW("Unable to find fragment function");

                MTLRenderPipelineDescriptor* renderPipelineDesc = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
                renderPipelineDesc.vertexFunction   = vertFunc;
                renderPipelineDesc.fragmentFunction = fragFunc;

                renderPipelineDesc.colorAttachments[0].pixelFormat = mtlBackBuffer.pixelFormat;

                mtlPass2PSO = [mtlDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                                                                        error:&errors];
            }

            id<MTLRasterizationRateMap> mtlRRM;
            id<MTLTexture>              mtlIntermediateRT;
            id<MTLBuffer>               mtlParamBuffer;
            {
                const Uint32 TileSize = 4;
                auto *layerDescriptor = [[MTLRasterizationRateLayerDescriptor alloc] initWithSampleCount:MTLSizeMake(SCDesc.Width / TileSize, SCDesc.Height / TileSize, 0)];
                for (Uint32 i = 0; i < layerDescriptor.sampleCount.width; ++i)
                    layerDescriptor.horizontalSampleStorage[i] = GenColRowFp32(i, layerDescriptor.sampleCount.width);

                for (Uint32 i = 0; i < layerDescriptor.sampleCount.height; ++i)
                    layerDescriptor.verticalSampleStorage[i] = GenColRowFp32(i, layerDescriptor.sampleCount.height);

                auto *mtlRRMdesc = [MTLRasterizationRateMapDescriptor rasterizationRateMapDescriptorWithScreenSize: MTLSizeMake(SCDesc.Width, SCDesc.Height, 0)];
                [mtlRRMdesc setLayer:layerDescriptor atIndex:0];
                mtlRRM = [mtlDevice newRasterizationRateMapWithDescriptor: mtlRRMdesc];
                ASSERT_TRUE(mtlRRM != nil);

                MTLSize rtSize = [mtlRRM physicalSizeForLayer:0];

                auto* mTLTexDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: mtlBackBuffer.pixelFormat
                                                                                      width: rtSize.width
                                                                                     height: rtSize.height
                                                                                  mipmapped: NO];
                mTLTexDesc.storageMode = MTLStorageModePrivate;
                mTLTexDesc.usage       = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
                mtlIntermediateRT = [mtlDevice newTextureWithDescriptor: mTLTexDesc];
                ASSERT_TRUE(mtlIntermediateRT != nil);

                mtlParamBuffer = [mtlDevice newBufferWithLength: [mtlRRM parameterBufferSizeAndAlign].size
                                                        options: MTLResourceStorageModeShared];
                ASSERT_TRUE(mtlParamBuffer != nil);

                [mtlRRM copyParameterDataToBuffer: mtlParamBuffer
                                           offset: 0];
            }

            const auto&   Verts = VRSTestingConstants::PerPrimitive::Vertices;
            id<MTLBuffer> mtlVertBuffer;
            {
                mtlVertBuffer = [mtlDevice newBufferWithLength: sizeof(Verts)
                                                       options: MTLResourceStorageModeShared];
                ASSERT_TRUE(mtlVertBuffer != nil);

                memcpy([mtlVertBuffer contents], Verts, sizeof(Verts));
            }

            auto* mtlCommandQueue  = pEnv->GetMtlCommandQueue();
            auto* mtlCommandBuffer = [mtlCommandQueue commandBuffer]; // Autoreleased

            // Pass 1
            {
                MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor]; // Autoreleased
                renderPassDesc.colorAttachments[0].texture     = mtlIntermediateRT;
                renderPassDesc.colorAttachments[0].loadAction  = MTLLoadActionClear;
                renderPassDesc.colorAttachments[0].clearColor  = MTLClearColorMake(1.0, 0.0, 0.0, 1.0);
                renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
                renderPassDesc.rasterizationRateMap            = mtlRRM;
                renderPassDesc.renderTargetWidth               = mtlBackBuffer.width;
                renderPassDesc.renderTargetHeight              = mtlBackBuffer.height;

                id<MTLRenderCommandEncoder> renderEncoder = [mtlCommandBuffer renderCommandEncoderWithDescriptor:renderPassDesc]; // Autoreleased
                ASSERT_TRUE(renderEncoder != nil);

                [renderEncoder setViewport:MTLViewport{0, 0, (double)mtlBackBuffer.width, (double)mtlBackBuffer.height, 0, 1}];

                [renderEncoder setRenderPipelineState:mtlPass1PSO];
                [renderEncoder setVertexBuffer: mtlVertBuffer
                                        offset: 0
                                       atIndex: 30];
                [renderEncoder drawPrimitives: MTLPrimitiveTypeTriangle
                                  vertexStart: 0
                                  vertexCount: _countof(Verts)];
                [renderEncoder endEncoding];
            }

            // Pass 2
            {
                MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor]; // Autoreleased
                renderPassDesc.colorAttachments[0].texture     = mtlBackBuffer;
                renderPassDesc.colorAttachments[0].loadAction  = MTLLoadActionClear;
                renderPassDesc.colorAttachments[0].clearColor  = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
                renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

                id<MTLRenderCommandEncoder> renderEncoder = [mtlCommandBuffer renderCommandEncoderWithDescriptor:renderPassDesc]; // Autoreleased
                ASSERT_TRUE(renderEncoder != nil);

                [renderEncoder setRenderPipelineState:mtlPass2PSO];

                [renderEncoder setFragmentBuffer: mtlParamBuffer
                                          offset: 0
                                         atIndex: 0];
                [renderEncoder setFragmentTexture: mtlIntermediateRT
                                          atIndex: 0];

                [renderEncoder drawPrimitives: MTLPrimitiveTypeTriangle
                                  vertexStart: 0
                                  vertexCount: 3];
                [renderEncoder endEncoding];
            }

            [mtlCommandBuffer commit];
            [mtlCommandBuffer waitUntilCompleted];
        }
    } // @autoreleasepool
}

} // namespace Testing

} // namespace Diligent
