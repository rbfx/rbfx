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

#include "InlineShaders/RayTracingTestMSL.h"
#include "RayTracingTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

void InlineRayTracingInComputePplnReferenceMtl(ISwapChain* pSwapChain)
{
    auto* const pEnv      = TestingEnvironmentMtl::GetInstance();
    auto const  mtlDevice = pEnv->GetMtlDevice();

    if (@available(macos 11.0, ios 14.0, *))
    {
        @autoreleasepool
        {
            auto* progSrc = [NSString stringWithUTF8String: MSL::RayTracingTest8_CS.c_str()];
            NSError *errors = nil; // Autoreleased
            id <MTLLibrary> library = [mtlDevice newLibraryWithSource: progSrc
                                                              options: nil
                                                                error: &errors];
            ASSERT_TRUE(library != nil);
            [library autorelease];

            id <MTLFunction> computeFunc = [library newFunctionWithName: @"CSMain"];
            ASSERT_TRUE(computeFunc != nil);
            [computeFunc autorelease];

            auto* computePipeline = [mtlDevice newComputePipelineStateWithFunction: computeFunc error: &errors];
            ASSERT_TRUE(computePipeline != nil);
            [computePipeline autorelease];

            auto* pTestingSwapChainMtl = ClassPtrCast<TestingSwapChainMtl>(pSwapChain);
            auto* pUAV = pTestingSwapChainMtl->GetCurrentBackBufferUAV();
            auto* mtlTexture = ClassPtrCast<ITextureViewMtl>(pUAV)->GetMtlTexture();
            const auto& SCDesc = pTestingSwapChainMtl->GetDesc();

            const auto& Vertices = TestingConstants::TriangleClosestHit::Vertices;
            auto* mtlVertexBuf = [mtlDevice newBufferWithLength: sizeof(Vertices)
                                                        options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(mtlVertexBuf != nil);
            [mtlVertexBuf autorelease];
            memcpy([mtlVertexBuf contents], Vertices, sizeof(Vertices));

            auto* mtlBLASDesc     = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
            auto* mtlTriangleDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
            auto* mtlTriangleArr  = [[[NSMutableArray<MTLAccelerationStructureTriangleGeometryDescriptor*> alloc] initWithCapacity: 1] autorelease];

            mtlTriangleDesc.intersectionFunctionTableOffset = 0;
            mtlTriangleDesc.allowDuplicateIntersectionFunctionInvocation = false;
            mtlTriangleDesc.opaque = true;
            mtlTriangleDesc.triangleCount = _countof(Vertices) / 3;
            mtlTriangleDesc.vertexBuffer = mtlVertexBuf;
            mtlTriangleDesc.vertexBufferOffset = 0;
            mtlTriangleDesc.vertexStride = sizeof(Vertices[0]);
            mtlTriangleDesc.indexType = MTLIndexTypeUInt32;
            [mtlTriangleArr addObject: mtlTriangleDesc];

            [mtlBLASDesc setUsage: MTLAccelerationStructureUsageNone];
            [mtlBLASDesc setGeometryDescriptors: mtlTriangleArr];

            const auto mtlBLASSizes = [mtlDevice accelerationStructureSizesWithDescriptor: mtlBLASDesc];
            auto*      mtlBLAS      = [mtlDevice newAccelerationStructureWithSize: mtlBLASSizes.accelerationStructureSize];
            ASSERT_TRUE(mtlBLAS != nil);
            [mtlBLAS autorelease];

            auto* mtlInstanceBuf = [mtlDevice newBufferWithLength: sizeof(MTLAccelerationStructureInstanceDescriptor)
                                                          options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(mtlInstanceBuf != nil);
            [mtlInstanceBuf autorelease];
            auto* Instances = static_cast<MTLAccelerationStructureInstanceDescriptor*>([mtlInstanceBuf contents]);

            Instances->transformationMatrix.columns[0] = MTLPackedFloat3{1.0f, 0.0f, 0.0f};
            Instances->transformationMatrix.columns[1] = MTLPackedFloat3{0.0f, 1.0f, 0.0f};
            Instances->transformationMatrix.columns[2] = MTLPackedFloat3{0.0f, 0.0f, 1.0f};
            Instances->transformationMatrix.columns[3] = MTLPackedFloat3{0.0f, 0.0f, 0.0f};
            Instances->accelerationStructureIndex      = 0;
            Instances->intersectionFunctionTableOffset = 0;
            Instances->mask                            = ~0u;
            Instances->options                         = MTLAccelerationStructureInstanceOptionOpaque;

            auto* mtlTLASDesc    = [MTLInstanceAccelerationStructureDescriptor descriptor];
            auto* mtlAccelStrArr = [[[NSMutableArray<id<MTLAccelerationStructure>> alloc] initWithCapacity: 1] autorelease];
            [mtlAccelStrArr addObject: mtlBLAS];
            [mtlTLASDesc setUsage: MTLAccelerationStructureUsageNone];
            [mtlTLASDesc setInstanceCount: 1];
            [mtlTLASDesc setInstanceDescriptorBuffer: mtlInstanceBuf];
            [mtlTLASDesc setInstancedAccelerationStructures: mtlAccelStrArr];

            const auto mtlTLASSizes = [mtlDevice accelerationStructureSizesWithDescriptor: mtlTLASDesc];
            auto*      mtlTLAS      = [mtlDevice newAccelerationStructureWithSize: mtlTLASSizes.accelerationStructureSize];
            ASSERT_TRUE(mtlTLAS != nil);
            [mtlTLAS autorelease];

            const auto ScratchBuffSize = std::max(mtlBLASSizes.buildScratchBufferSize, mtlTLASSizes.buildScratchBufferSize);
            auto* ScratchBuf = [mtlDevice newBufferWithLength: ScratchBuffSize
                                                      options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(ScratchBuf != nil);
            [ScratchBuf autorelease];

            auto* mtlCommandQueue = pEnv->GetMtlCommandQueue();

            // Command buffer is autoreleased
            auto* mtlCommandBuffer = [mtlCommandQueue commandBuffer];

            // Acceleration structure command encoder is autoreleased
            auto* asEncoder = [mtlCommandBuffer accelerationStructureCommandEncoder];
            ASSERT_TRUE(asEncoder != nil);

            [asEncoder buildAccelerationStructure: mtlBLAS
                                       descriptor: mtlBLASDesc
                                    scratchBuffer: ScratchBuf
                              scratchBufferOffset: 0];
            [asEncoder buildAccelerationStructure: mtlTLAS
                                       descriptor: mtlTLASDesc
                                    scratchBuffer: ScratchBuf
                              scratchBufferOffset: 0];
            [asEncoder endEncoding];

            // Compute command encoder is autoreleased
            auto* cmdEncoder = [mtlCommandBuffer computeCommandEncoder];
            ASSERT_TRUE(cmdEncoder != nil);

            [cmdEncoder setComputePipelineState: computePipeline];
            [cmdEncoder setTexture: mtlTexture atIndex: 0];
            [cmdEncoder setAccelerationStructure: mtlTLAS atBufferIndex: 0];
            [cmdEncoder dispatchThreadgroups: MTLSizeMake((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1)
                       threadsPerThreadgroup: MTLSizeMake(16, 16, 1)];

            [cmdEncoder endEncoding];
            [mtlCommandBuffer commit];
            [mtlCommandBuffer waitUntilCompleted];

        } // @autoreleasepool
    }
}

void RayTracingPRSReferenceMtl(ISwapChain* pSwapChain)
{
    auto* const pEnv      = TestingEnvironmentMtl::GetInstance();
    auto const  mtlDevice = pEnv->GetMtlDevice();

    if (@available(macos 11.0, ios 14.0, *))
    {
        @autoreleasepool
        {
            auto* progSrc = [NSString stringWithUTF8String: MSL::RayTracingTest9_CS.c_str()];
            NSError *errors = nil; // Autoreleased
            id <MTLLibrary> library = [mtlDevice newLibraryWithSource: progSrc
                                                              options: nil
                                                                error: &errors];
            ASSERT_TRUE(library != nil);
            [library autorelease];

            id <MTLFunction> computeFunc = [library newFunctionWithName: @"CSMain"];
            ASSERT_TRUE(computeFunc != nil);
            [computeFunc autorelease];

            auto* computePipeline = [mtlDevice newComputePipelineStateWithFunction: computeFunc error: &errors];
            ASSERT_TRUE(computePipeline != nil);
            [computePipeline autorelease];

            auto* pTestingSwapChainMtl = ClassPtrCast<TestingSwapChainMtl>(pSwapChain);
            auto* pUAV = pTestingSwapChainMtl->GetCurrentBackBufferUAV();
            auto* mtlTexture = ClassPtrCast<ITextureViewMtl>(pUAV)->GetMtlTexture();
            const auto& SCDesc = pTestingSwapChainMtl->GetDesc();

            const auto& Vertices = TestingConstants::TriangleClosestHit::Vertices;
            auto* mtlVertexBuf = [mtlDevice newBufferWithLength: sizeof(Vertices)
                                                        options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(mtlVertexBuf != nil);
            [mtlVertexBuf autorelease];
            memcpy([mtlVertexBuf contents], Vertices, sizeof(Vertices));

            auto* mtlBLASDesc     = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
            auto* mtlTriangleDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
            auto* mtlTriangleArr  = [[[NSMutableArray<MTLAccelerationStructureTriangleGeometryDescriptor*> alloc] initWithCapacity: 1] autorelease];

            mtlTriangleDesc.intersectionFunctionTableOffset = 0;
            mtlTriangleDesc.allowDuplicateIntersectionFunctionInvocation = false;
            mtlTriangleDesc.opaque = true;
            mtlTriangleDesc.triangleCount = _countof(Vertices) / 3;
            mtlTriangleDesc.vertexBuffer = mtlVertexBuf;
            mtlTriangleDesc.vertexBufferOffset = 0;
            mtlTriangleDesc.vertexStride = sizeof(Vertices[0]);
            mtlTriangleDesc.indexType = MTLIndexTypeUInt32;
            [mtlTriangleArr addObject: mtlTriangleDesc];

            [mtlBLASDesc setUsage: MTLAccelerationStructureUsageNone];
            [mtlBLASDesc setGeometryDescriptors: mtlTriangleArr];

            const auto mtlBLASSizes = [mtlDevice accelerationStructureSizesWithDescriptor: mtlBLASDesc];
            auto*      mtlBLAS      = [mtlDevice newAccelerationStructureWithSize: mtlBLASSizes.accelerationStructureSize];
            ASSERT_TRUE(mtlBLAS != nil);
            [mtlBLAS autorelease];

            auto* mtlInstanceBuf = [mtlDevice newBufferWithLength: sizeof(MTLAccelerationStructureInstanceDescriptor)
                                                          options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(mtlInstanceBuf != nil);
            [mtlInstanceBuf autorelease];
            auto* Instances = static_cast<MTLAccelerationStructureInstanceDescriptor*>([mtlInstanceBuf contents]);

            Instances->transformationMatrix.columns[0] = MTLPackedFloat3{1.0f, 0.0f, 0.0f};
            Instances->transformationMatrix.columns[1] = MTLPackedFloat3{0.0f, 1.0f, 0.0f};
            Instances->transformationMatrix.columns[2] = MTLPackedFloat3{0.0f, 0.0f, 1.0f};
            Instances->transformationMatrix.columns[3] = MTLPackedFloat3{0.0f, 0.0f, 0.0f};
            Instances->accelerationStructureIndex      = 0;
            Instances->intersectionFunctionTableOffset = 0;
            Instances->mask                            = ~0u;
            Instances->options                         = MTLAccelerationStructureInstanceOptionOpaque;

            auto* mtlTLASDesc    = [MTLInstanceAccelerationStructureDescriptor descriptor];
            auto* mtlAccelStrArr = [[[NSMutableArray<id<MTLAccelerationStructure>> alloc] initWithCapacity: 1] autorelease];
            [mtlAccelStrArr addObject: mtlBLAS];
            [mtlTLASDesc setUsage: MTLAccelerationStructureUsageNone];
            [mtlTLASDesc setInstanceCount: 1];
            [mtlTLASDesc setInstanceDescriptorBuffer: mtlInstanceBuf];
            [mtlTLASDesc setInstancedAccelerationStructures: mtlAccelStrArr];

            const auto mtlTLASSizes = [mtlDevice accelerationStructureSizesWithDescriptor: mtlTLASDesc];
            auto*      mtlTLAS      = [mtlDevice newAccelerationStructureWithSize: mtlTLASSizes.accelerationStructureSize];
            ASSERT_TRUE(mtlTLAS != nil);
            [mtlTLAS autorelease];

            const auto ScratchBuffSize = std::max(mtlBLASSizes.buildScratchBufferSize, mtlTLASSizes.buildScratchBufferSize);
            auto* ScratchBuf = [mtlDevice newBufferWithLength: ScratchBuffSize
                                                      options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(ScratchBuf != nil);
            [ScratchBuf autorelease];

            auto* ConstBuf = [mtlDevice newBufferWithLength: 256 * 3
                                                    options: (MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeShared | MTLResourceHazardTrackingModeDefault)];
            ASSERT_TRUE(ConstBuf != nil);
            [ConstBuf autorelease];

            uint8_t* pMapped = (uint8_t*)ConstBuf.contents;
            const float Const1[] = {0.5f, 0.9f, 0.75f, 1.0f};
            const float Const2[] = {0.2f, 0.0f, 1.0f, 0.5f};
            const float Const3[] = {0.9f, 0.1f, 0.2f, 1.0f};
            memcpy(pMapped + 0, Const1, sizeof(Const1));
            memcpy(pMapped + 256, Const2, sizeof(Const2));
            memcpy(pMapped + 512, Const3, sizeof(Const3));

            auto* mtlCommandQueue  = pEnv->GetMtlCommandQueue();
            auto* mtlCommandBuffer = [mtlCommandQueue commandBuffer]; // Autoreleased
            auto* asEncoder        = [mtlCommandBuffer accelerationStructureCommandEncoder]; // Autoreleased
            ASSERT_TRUE(asEncoder != nil);

            [asEncoder buildAccelerationStructure: mtlBLAS
                                       descriptor: mtlBLASDesc
                                    scratchBuffer: ScratchBuf
                              scratchBufferOffset: 0];
            [asEncoder buildAccelerationStructure: mtlTLAS
                                       descriptor: mtlTLASDesc
                                    scratchBuffer: ScratchBuf
                              scratchBufferOffset: 0];
            [asEncoder endEncoding];

            auto* cmdEncoder = [mtlCommandBuffer computeCommandEncoder]; // Autoreleased
            ASSERT_TRUE(cmdEncoder != nil);

            [cmdEncoder setComputePipelineState: computePipeline];
            [cmdEncoder setTexture: mtlTexture
                           atIndex: 0];
            [cmdEncoder setBuffer: ConstBuf
                           offset: 0
                          atIndex: 0];
            [cmdEncoder setBuffer: ConstBuf
                           offset: 256
                          atIndex: 1];
            [cmdEncoder setBuffer: ConstBuf
                           offset: 512
                          atIndex: 2];
            [cmdEncoder setAccelerationStructure: mtlTLAS
                                   atBufferIndex: 3];
            [cmdEncoder setAccelerationStructure: mtlTLAS
                                   atBufferIndex: 4];
            [cmdEncoder dispatchThreadgroups: MTLSizeMake((SCDesc.Width + 15) / 16, (SCDesc.Height + 15) / 16, 1)
                       threadsPerThreadgroup: MTLSizeMake(16, 16, 1)];

            [cmdEncoder endEncoding];
            [mtlCommandBuffer commit];
            [mtlCommandBuffer waitUntilCompleted];

        } // @autoreleasepool
    } // @available
}

} // namespace Testing

} // namespace Diligent
