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

#include <functional>

#include "Vulkan/TestingEnvironmentVk.hpp"
#include "Vulkan/TestingSwapChainVk.hpp"
#include "Align.hpp"
#include "BasicMath.hpp"

#include "DeviceContextVk.h"

#include "volk.h"

#include "InlineShaders/RayTracingTestGLSL.h"
#include "RayTracingTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

namespace
{

struct RTContext
{
    struct AccelStruct
    {
        VkDevice                   vkDevice    = VK_NULL_HANDLE;
        VkDeviceMemory             vkMemory    = VK_NULL_HANDLE;
        VkBuffer                   vkBuffer    = VK_NULL_HANDLE;
        VkAccelerationStructureKHR vkAS        = VK_NULL_HANDLE;
        VkDeviceAddress            vkAddress   = 0;
        VkDeviceSize               ScratchSize = 0;

        AccelStruct()
        {}

        ~AccelStruct()
        {
            if (vkBuffer)
                vkDestroyBuffer(vkDevice, vkBuffer, nullptr);
            if (vkAS)
                vkDestroyAccelerationStructureKHR(vkDevice, vkAS, nullptr);
            if (vkMemory)
                vkFreeMemory(vkDevice, vkMemory, nullptr);
        }
    };

    VkDevice                                           vkDevice           = VK_NULL_HANDLE;
    VkCommandBuffer                                    vkCmdBuffer        = VK_NULL_HANDLE;
    VkImage                                            vkRenderTarget     = VK_NULL_HANDLE;
    VkImageView                                        vkRenderTargetView = VK_NULL_HANDLE;
    VkPipelineLayout                                   vkLayout           = VK_NULL_HANDLE;
    VkPipeline                                         vkPipeline         = VK_NULL_HANDLE;
    VkDescriptorSetLayout                              vkSetLayout        = VK_NULL_HANDLE;
    VkDescriptorPool                                   vkDescriptorPool   = VK_NULL_HANDLE;
    VkDescriptorSet                                    vkDescriptorSet    = VK_NULL_HANDLE;
    AccelStruct                                        BLAS;
    AccelStruct                                        TLAS;
    VkBuffer                                           vkSBTBuffer             = VK_NULL_HANDLE;
    VkBuffer                                           vkScratchBuffer         = VK_NULL_HANDLE;
    VkBuffer                                           vkInstanceBuffer        = VK_NULL_HANDLE;
    VkBuffer                                           vkVertexBuffer          = VK_NULL_HANDLE;
    VkBuffer                                           vkIndexBuffer           = VK_NULL_HANDLE;
    VkDeviceAddress                                    vkScratchBufferAddress  = 0;
    VkDeviceAddress                                    vkSBTBufferAddress      = 0;
    VkDeviceAddress                                    vkInstanceBufferAddress = 0;
    VkDeviceAddress                                    vkVertexBufferAddress   = 0;
    VkDeviceAddress                                    vkIndexBufferAddress    = 0;
    VkDeviceMemory                                     vkBufferMemory          = VK_NULL_HANDLE;
    VkPhysicalDeviceLimits                             DeviceLimits            = {};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR AccelStructProps        = {};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR    RayTracingProps         = {};

    RTContext()
    {}

    ~RTContext()
    {
        if (vkPipeline)
            vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
        if (vkLayout)
            vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
        if (vkSetLayout)
            vkDestroyDescriptorSetLayout(vkDevice, vkSetLayout, nullptr);
        if (vkDescriptorPool)
            vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
        if (vkBufferMemory)
            vkFreeMemory(vkDevice, vkBufferMemory, nullptr);
        if (vkSBTBuffer)
            vkDestroyBuffer(vkDevice, vkSBTBuffer, nullptr);
        if (vkScratchBuffer)
            vkDestroyBuffer(vkDevice, vkScratchBuffer, nullptr);
        if (vkVertexBuffer)
            vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);
        if (vkIndexBuffer)
            vkDestroyBuffer(vkDevice, vkIndexBuffer, nullptr);
        if (vkInstanceBuffer)
            vkDestroyBuffer(vkDevice, vkInstanceBuffer, nullptr);
    }

    void ClearRenderTarget(TestingSwapChainVk* pTestingSwapChainVk)
    {
        pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);

        VkImageSubresourceRange Range      = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkClearColorValue       ClearValue = {};
        vkCmdClearColorImage(vkCmdBuffer, vkRenderTarget, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearValue, 1, &Range);

        pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, 0);
    }
};


struct RTGroupsHelper
{
    std::vector<VkDescriptorSetLayoutBinding>         Bindings;
    std::vector<VkShaderModule>                       Modules;
    std::vector<VkPipelineShaderStageCreateInfo>      Stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> Groups;

    void SetShaderCount(Uint32 NumShaders, Uint32 NumGroups)
    {
        Modules.resize(NumShaders);
        Stages.resize(NumShaders);
        Groups.resize(NumGroups);
    }

    void SetStage(Uint32 StageIndex, SHADER_TYPE ShaderType, const String& Source)
    {
        auto* pEnv                = TestingEnvironmentVk::GetInstance();
        Modules[StageIndex]       = pEnv->CreateShaderModule(ShaderType, Source);
        Stages[StageIndex].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        Stages[StageIndex].module = Modules[StageIndex];
        Stages[StageIndex].pName  = "main";

        switch (ShaderType)
        {
            // clang-format off
            case SHADER_TYPE_RAY_GEN:          Stages[StageIndex].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;       break;
            case SHADER_TYPE_RAY_MISS:         Stages[StageIndex].stage = VK_SHADER_STAGE_MISS_BIT_KHR;         break;
            case SHADER_TYPE_RAY_CLOSEST_HIT:  Stages[StageIndex].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;  break;
            case SHADER_TYPE_RAY_ANY_HIT:      Stages[StageIndex].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;      break;
            case SHADER_TYPE_RAY_INTERSECTION: Stages[StageIndex].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR; break;
            case SHADER_TYPE_CALLABLE:         Stages[StageIndex].stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR;     break;
            // clang-format on
            default:
                UNEXPECTED("Unexpected ray tracing shader type");
        }
    }

    void SetGeneralGroup(Uint32 GroupIndex, Uint32 StageIndex)
    {
        Groups[GroupIndex].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Groups[GroupIndex].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        Groups[GroupIndex].generalShader      = StageIndex;
        Groups[GroupIndex].closestHitShader   = VK_SHADER_UNUSED_KHR;
        Groups[GroupIndex].anyHitShader       = VK_SHADER_UNUSED_KHR;
        Groups[GroupIndex].intersectionShader = VK_SHADER_UNUSED_KHR;
    }

    void SetTriangleHitGroup(Uint32 GroupIndex, Uint32 ClosestHitShader, Uint32 AnyHitShader = VK_SHADER_UNUSED_KHR)
    {
        Groups[GroupIndex].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Groups[GroupIndex].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        Groups[GroupIndex].generalShader      = VK_SHADER_UNUSED_KHR;
        Groups[GroupIndex].closestHitShader   = ClosestHitShader;
        Groups[GroupIndex].anyHitShader       = AnyHitShader;
        Groups[GroupIndex].intersectionShader = VK_SHADER_UNUSED_KHR;
    }

    void SetProceduralHitGroup(Uint32 GroupIndex, Uint32 IntersectionShader, Uint32 ClosestHitShader, Uint32 AnyHitShader = VK_SHADER_UNUSED_KHR)
    {
        Groups[GroupIndex].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Groups[GroupIndex].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        Groups[GroupIndex].generalShader      = VK_SHADER_UNUSED_KHR;
        Groups[GroupIndex].closestHitShader   = ClosestHitShader;
        Groups[GroupIndex].anyHitShader       = AnyHitShader;
        Groups[GroupIndex].intersectionShader = IntersectionShader;
    }

    void AddBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags)
    {
        VkDescriptorSetLayoutBinding DSBinding = {};

        DSBinding.binding            = binding;
        DSBinding.descriptorType     = descriptorType;
        DSBinding.descriptorCount    = descriptorCount;
        DSBinding.stageFlags         = stageFlags;
        DSBinding.pImmutableSamplers = nullptr;

        Bindings.push_back(DSBinding);
    }
};


template <typename PSOCtorType>
void InitializeRTContext(RTContext& Ctx, ISwapChain* pSwapChain, PSOCtorType&& PSOCtor)
{
    auto*    pEnv                = TestingEnvironmentVk::GetInstance();
    auto*    pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);
    VkResult res                 = VK_SUCCESS;

    Ctx.vkDevice           = pEnv->GetVkDevice();
    Ctx.vkCmdBuffer        = pEnv->AllocateCommandBuffer();
    Ctx.vkRenderTarget     = pTestingSwapChainVk->GetVkRenderTargetImage();
    Ctx.vkRenderTargetView = pTestingSwapChainVk->GetVkRenderTargetImageView();

    VkPhysicalDeviceProperties2 Props2{};
    Props2.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    Props2.pNext               = &Ctx.RayTracingProps;
    Ctx.RayTracingProps.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    Ctx.RayTracingProps.pNext  = &Ctx.AccelStructProps;
    Ctx.AccelStructProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    vkGetPhysicalDeviceProperties2KHR(pEnv->GetVkPhysicalDevice(), &Props2);
    Ctx.DeviceLimits = Props2.properties.limits;

    // Create ray tracing pipeline
    {
        VkDescriptorSetLayoutCreateInfo   DescriptorSetCI  = {};
        VkPipelineLayoutCreateInfo        PipelineLayoutCI = {};
        VkRayTracingPipelineCreateInfoKHR PipelineCI       = {};
        RTGroupsHelper                    Helper;

        PSOCtor(Helper);

        Helper.AddBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        Helper.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        DescriptorSetCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        DescriptorSetCI.bindingCount = static_cast<Uint32>(Helper.Bindings.size());
        DescriptorSetCI.pBindings    = Helper.Bindings.data();

        res = vkCreateDescriptorSetLayout(Ctx.vkDevice, &DescriptorSetCI, nullptr, &Ctx.vkSetLayout);
        ASSERT_GE(res, 0);
        ASSERT_TRUE(Ctx.vkSetLayout != VK_NULL_HANDLE);

        PipelineLayoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutCI.setLayoutCount = 1;
        PipelineLayoutCI.pSetLayouts    = &Ctx.vkSetLayout;

        vkCreatePipelineLayout(Ctx.vkDevice, &PipelineLayoutCI, nullptr, &Ctx.vkLayout);
        ASSERT_TRUE(Ctx.vkLayout != VK_NULL_HANDLE);

        PipelineCI.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        PipelineCI.stageCount                   = static_cast<Uint32>(Helper.Stages.size());
        PipelineCI.pStages                      = Helper.Stages.data();
        PipelineCI.groupCount                   = static_cast<Uint32>(Helper.Groups.size());
        PipelineCI.pGroups                      = Helper.Groups.data();
        PipelineCI.maxPipelineRayRecursionDepth = 0;
        PipelineCI.layout                       = Ctx.vkLayout;

        res = vkCreateRayTracingPipelinesKHR(Ctx.vkDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &Ctx.vkPipeline);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkPipeline != VK_NULL_HANDLE);

        for (auto& SM : Helper.Modules)
        {
            vkDestroyShaderModule(Ctx.vkDevice, SM, nullptr);
        }
    }

    // Create descriptor set
    {
        VkDescriptorPoolCreateInfo  DescriptorPoolCI = {};
        VkDescriptorPoolSize        PoolSizes[3]     = {};
        VkDescriptorSetAllocateInfo SetAllocInfo     = {};

        static constexpr uint32_t MaxSetsInPool        = 16;
        static constexpr uint32_t MaxDescriptorsInPool = 16;

        DescriptorPoolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        DescriptorPoolCI.maxSets       = MaxSetsInPool;
        DescriptorPoolCI.poolSizeCount = _countof(PoolSizes);
        DescriptorPoolCI.pPoolSizes    = PoolSizes;

        PoolSizes[0].type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        PoolSizes[0].descriptorCount = MaxDescriptorsInPool;
        PoolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        PoolSizes[1].descriptorCount = MaxDescriptorsInPool;
        PoolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        PoolSizes[2].descriptorCount = MaxDescriptorsInPool;

        res = vkCreateDescriptorPool(Ctx.vkDevice, &DescriptorPoolCI, nullptr, &Ctx.vkDescriptorPool);
        ASSERT_GE(res, 0);
        ASSERT_TRUE(Ctx.vkDescriptorPool != VK_NULL_HANDLE);

        SetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        SetAllocInfo.descriptorPool     = Ctx.vkDescriptorPool;
        SetAllocInfo.descriptorSetCount = 1;
        SetAllocInfo.pSetLayouts        = &Ctx.vkSetLayout;

        vkAllocateDescriptorSets(Ctx.vkDevice, &SetAllocInfo, &Ctx.vkDescriptorSet);
        ASSERT_TRUE(Ctx.vkDescriptorSet != VK_NULL_HANDLE);
    }
}

void UpdateDescriptorSet(RTContext& Ctx)
{
    VkWriteDescriptorSet DescriptorWrite[2] = {};

    DescriptorWrite[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite[0].dstSet          = Ctx.vkDescriptorSet;
    DescriptorWrite[0].dstBinding      = 1;
    DescriptorWrite[0].dstArrayElement = 0;
    DescriptorWrite[0].descriptorCount = 1;
    DescriptorWrite[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    DescriptorWrite[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite[1].dstSet          = Ctx.vkDescriptorSet;
    DescriptorWrite[1].dstBinding      = 0;
    DescriptorWrite[1].dstArrayElement = 0;
    DescriptorWrite[1].descriptorCount = 1;
    DescriptorWrite[1].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageView             = Ctx.vkRenderTargetView;
    ImageInfo.imageLayout           = VK_IMAGE_LAYOUT_GENERAL;
    DescriptorWrite[0].pImageInfo   = &ImageInfo;

    VkWriteDescriptorSetAccelerationStructureKHR TLASInfo = {};

    TLASInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    TLASInfo.accelerationStructureCount = 1;
    TLASInfo.pAccelerationStructures    = &Ctx.TLAS.vkAS;
    DescriptorWrite[1].pNext            = &TLASInfo;

    vkUpdateDescriptorSets(Ctx.vkDevice, _countof(DescriptorWrite), DescriptorWrite, 0, nullptr);
}

void CreateBLAS(const RTContext&                                Ctx,
                const VkAccelerationStructureGeometryKHR*       pGeometries,
                const VkAccelerationStructureBuildRangeInfoKHR* pRanges,
                Uint32                                          GeometryCount,
                RTContext::AccelStruct&                         BLAS)
{
    BLAS.vkDevice = Ctx.vkDevice;

    VkResult     res             = VK_SUCCESS;
    VkDeviceSize AccelStructSize = 0;

    {
        VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo{};
        VkAccelerationStructureBuildSizesInfoKHR    vkSizeInfo{};
        vkSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        std::vector<uint32_t> MaxPrimitives(GeometryCount);
        for (Uint32 i = 0; i < GeometryCount; ++i)
        {
            MaxPrimitives[i] = pRanges[i].primitiveCount;
        }

        vkBuildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        vkBuildInfo.flags         = 0;
        vkBuildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkBuildInfo.pGeometries   = pGeometries;
        vkBuildInfo.geometryCount = GeometryCount;

        vkGetAccelerationStructureBuildSizesKHR(Ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vkBuildInfo, MaxPrimitives.data(), &vkSizeInfo);

        BLAS.ScratchSize = vkSizeInfo.buildScratchSize;
        AccelStructSize  = vkSizeInfo.accelerationStructureSize;
    }

    VkBufferCreateInfo BuffCI = {};

    BuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BuffCI.size  = AccelStructSize;
    BuffCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

    res = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &BLAS.vkBuffer);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(BLAS.vkBuffer != VK_NULL_HANDLE);

    VkMemoryRequirements MemReqs = {};
    vkGetBufferMemoryRequirements(Ctx.vkDevice, BLAS.vkBuffer, &MemReqs);

    VkMemoryAllocateInfo MemAlloc = {};

    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = MemReqs.size;
    MemAlloc.memoryTypeIndex = TestingEnvironmentVk::GetInstance()->GetMemoryTypeIndex(MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ASSERT_TRUE(MemAlloc.memoryTypeIndex != ~0u);

    res = vkAllocateMemory(Ctx.vkDevice, &MemAlloc, nullptr, &BLAS.vkMemory);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(BLAS.vkMemory != VK_NULL_HANDLE);

    vkBindBufferMemory(Ctx.vkDevice, BLAS.vkBuffer, BLAS.vkMemory, 0);

    VkAccelerationStructureCreateInfoKHR vkAccelStrCI = {};

    vkAccelStrCI.sType       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    vkAccelStrCI.createFlags = 0;
    vkAccelStrCI.buffer      = BLAS.vkBuffer;
    vkAccelStrCI.offset      = 0;
    vkAccelStrCI.size        = AccelStructSize;
    vkAccelStrCI.type        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    res = vkCreateAccelerationStructureKHR(Ctx.vkDevice, &vkAccelStrCI, nullptr, &BLAS.vkAS);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(BLAS.vkAS != VK_NULL_HANDLE);

    VkAccelerationStructureDeviceAddressInfoKHR AddressInfo = {};

    AddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AddressInfo.accelerationStructure = BLAS.vkAS;

    BLAS.vkAddress = vkGetAccelerationStructureDeviceAddressKHR(Ctx.vkDevice, &AddressInfo);
}

void CreateTLAS(const RTContext& Ctx, Uint32 InstanceCount, RTContext::AccelStruct& TLAS)
{
    TLAS.vkDevice = Ctx.vkDevice;

    VkResult     res             = VK_SUCCESS;
    VkDeviceSize AccelStructSize = 0;

    {
        VkAccelerationStructureBuildGeometryInfoKHR      vkBuildInfo = {};
        VkAccelerationStructureGeometryKHR               vkGeometry  = {};
        VkAccelerationStructureGeometryInstancesDataKHR& vkInstances = vkGeometry.geometry.instances;
        VkAccelerationStructureBuildSizesInfoKHR         vkSizeInfo{};
        vkSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGeometry.sType            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        vkGeometry.geometryType     = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        vkInstances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        vkInstances.arrayOfPointers = VK_FALSE;

        vkBuildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        vkBuildInfo.flags         = 0;
        vkBuildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkBuildInfo.pGeometries   = &vkGeometry;
        vkBuildInfo.geometryCount = 1;

        vkGetAccelerationStructureBuildSizesKHR(Ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vkBuildInfo, &InstanceCount, &vkSizeInfo);

        TLAS.ScratchSize = vkSizeInfo.buildScratchSize;
        AccelStructSize  = vkSizeInfo.accelerationStructureSize;
    }

    VkBufferCreateInfo BuffCI = {};

    BuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BuffCI.size  = AccelStructSize;
    BuffCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

    res = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &TLAS.vkBuffer);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(TLAS.vkBuffer != VK_NULL_HANDLE);

    VkMemoryRequirements MemReqs = {};
    vkGetBufferMemoryRequirements(Ctx.vkDevice, TLAS.vkBuffer, &MemReqs);

    VkMemoryAllocateInfo MemAlloc = {};

    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = MemReqs.size;
    MemAlloc.memoryTypeIndex = TestingEnvironmentVk::GetInstance()->GetMemoryTypeIndex(MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ASSERT_TRUE(MemAlloc.memoryTypeIndex != ~0u);

    res = vkAllocateMemory(Ctx.vkDevice, &MemAlloc, nullptr, &TLAS.vkMemory);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(TLAS.vkMemory != VK_NULL_HANDLE);

    vkBindBufferMemory(Ctx.vkDevice, TLAS.vkBuffer, TLAS.vkMemory, 0);

    VkAccelerationStructureCreateInfoKHR vkAccelStrCI = {};

    vkAccelStrCI.sType       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    vkAccelStrCI.createFlags = 0;
    vkAccelStrCI.buffer      = TLAS.vkBuffer;
    vkAccelStrCI.offset      = 0;
    vkAccelStrCI.size        = AccelStructSize;
    vkAccelStrCI.type        = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    res = vkCreateAccelerationStructureKHR(Ctx.vkDevice, &vkAccelStrCI, nullptr, &TLAS.vkAS);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(TLAS.vkAS != VK_NULL_HANDLE);
}

void CreateRTBuffers(RTContext& Ctx, Uint32 VBSize, Uint32 IBSize, Uint32 InstanceCount, Uint32 NumMissShaders, Uint32 NumHitShaders, Uint32 ShaderRecordSize = 0)
{
    VkResult res = VK_SUCCESS;

    VkDeviceSize ScratchSize = std::max(Ctx.TLAS.ScratchSize, Ctx.BLAS.ScratchSize);
    VkDeviceSize MemSize     = 0;

    VkMemoryRequirements2 MemReqs = {};
    MemReqs.sType                 = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkBufferCreateInfo              BuffCI      = {};
    VkBufferMemoryRequirementsInfo2 MemInfo     = {};
    Uint32                          MemTypeBits = 0;
    VkBufferDeviceAddressInfoKHR    BufferInfo  = {};
    (void)MemTypeBits;

    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;

    BuffCI.sType  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BuffCI.usage  = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    MemInfo.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;

    std::vector<std::function<void(VkDeviceMemory Mem, VkDeviceSize & Offset)>> BindMem;

    if (VBSize > 0)
    {
        BuffCI.size = VBSize;
        res         = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &Ctx.vkVertexBuffer);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkVertexBuffer != VK_NULL_HANDLE);

        MemInfo.buffer = Ctx.vkVertexBuffer;
        vkGetBufferMemoryRequirements2(Ctx.vkDevice, &MemInfo, &MemReqs);

        MemSize = AlignUp(MemSize, MemReqs.memoryRequirements.alignment);
        MemSize += MemReqs.memoryRequirements.size;
        MemTypeBits |= MemReqs.memoryRequirements.memoryTypeBits;

        BindMem.emplace_back([&Ctx, MemReqs, &BufferInfo](VkDeviceMemory Mem, VkDeviceSize& Offset) {
            Offset = AlignUp(Offset, MemReqs.memoryRequirements.alignment);
            vkBindBufferMemory(Ctx.vkDevice, Ctx.vkVertexBuffer, Mem, Offset);
            Offset += MemReqs.memoryRequirements.size;
            BufferInfo.buffer         = Ctx.vkVertexBuffer;
            Ctx.vkVertexBufferAddress = vkGetBufferDeviceAddressKHR(Ctx.vkDevice, &BufferInfo);
            ASSERT_TRUE(Ctx.vkVertexBufferAddress > 0);
        });
    }

    if (IBSize > 0)
    {
        BuffCI.size = VBSize;
        res         = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &Ctx.vkIndexBuffer);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkIndexBuffer != VK_NULL_HANDLE);

        MemInfo.buffer = Ctx.vkIndexBuffer;
        vkGetBufferMemoryRequirements2(Ctx.vkDevice, &MemInfo, &MemReqs);

        MemSize = AlignUp(MemSize, MemReqs.memoryRequirements.alignment);
        MemSize += MemReqs.memoryRequirements.size;
        MemTypeBits |= MemReqs.memoryRequirements.memoryTypeBits;

        BindMem.emplace_back([&Ctx, MemReqs, &BufferInfo](VkDeviceMemory Mem, VkDeviceSize& Offset) {
            Offset = AlignUp(Offset, MemReqs.memoryRequirements.alignment);
            vkBindBufferMemory(Ctx.vkDevice, Ctx.vkIndexBuffer, Mem, Offset);
            Offset += MemReqs.memoryRequirements.size;
            BufferInfo.buffer        = Ctx.vkIndexBuffer;
            Ctx.vkIndexBufferAddress = vkGetBufferDeviceAddressKHR(Ctx.vkDevice, &BufferInfo);
            ASSERT_TRUE(Ctx.vkIndexBufferAddress > 0);
        });
    }

    if (InstanceCount > 0)
    {
        BuffCI.size = InstanceCount * sizeof(VkAccelerationStructureInstanceKHR);
        res         = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &Ctx.vkInstanceBuffer);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkInstanceBuffer != VK_NULL_HANDLE);

        MemInfo.buffer = Ctx.vkInstanceBuffer;
        vkGetBufferMemoryRequirements2(Ctx.vkDevice, &MemInfo, &MemReqs);

        MemSize = AlignUp(MemSize, MemReqs.memoryRequirements.alignment);
        MemSize += MemReqs.memoryRequirements.size;
        MemTypeBits |= MemReqs.memoryRequirements.memoryTypeBits;

        BindMem.emplace_back([&Ctx, MemReqs, &BufferInfo](VkDeviceMemory Mem, VkDeviceSize& Offset) {
            Offset = AlignUp(Offset, MemReqs.memoryRequirements.alignment);
            vkBindBufferMemory(Ctx.vkDevice, Ctx.vkInstanceBuffer, Mem, Offset);
            Offset += MemReqs.memoryRequirements.size;
            BufferInfo.buffer           = Ctx.vkInstanceBuffer;
            Ctx.vkInstanceBufferAddress = vkGetBufferDeviceAddressKHR(Ctx.vkDevice, &BufferInfo);
            ASSERT_TRUE(Ctx.vkInstanceBufferAddress > 0);
        });
    }

    if (ScratchSize > 0)
    {
        BuffCI.size = ScratchSize;
        res         = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &Ctx.vkScratchBuffer);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkScratchBuffer != VK_NULL_HANDLE);

        MemInfo.buffer = Ctx.vkScratchBuffer;
        vkGetBufferMemoryRequirements2(Ctx.vkDevice, &MemInfo, &MemReqs);

        MemSize = AlignUp(MemSize, MemReqs.memoryRequirements.alignment);
        MemSize += MemReqs.memoryRequirements.size;
        MemTypeBits |= MemReqs.memoryRequirements.memoryTypeBits;

        BindMem.emplace_back([&Ctx, MemReqs, &BufferInfo](VkDeviceMemory Mem, VkDeviceSize& Offset) {
            Offset = AlignUp(Offset, MemReqs.memoryRequirements.alignment);
            vkBindBufferMemory(Ctx.vkDevice, Ctx.vkScratchBuffer, Mem, Offset);
            Offset += MemReqs.memoryRequirements.size;
            BufferInfo.buffer          = Ctx.vkScratchBuffer;
            Ctx.vkScratchBufferAddress = vkGetBufferDeviceAddressKHR(Ctx.vkDevice, &BufferInfo);
            ASSERT_TRUE(Ctx.vkScratchBufferAddress > 0);
        });
    }

    // SBT
    {
        const Uint32 GroupSize = Ctx.RayTracingProps.shaderGroupHandleSize + ShaderRecordSize;

        BuffCI.size = AlignUp(GroupSize, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        BuffCI.size = AlignUp(BuffCI.size + GroupSize * NumMissShaders, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        BuffCI.size = AlignUp(BuffCI.size + GroupSize * NumHitShaders, Ctx.RayTracingProps.shaderGroupBaseAlignment);

        res = vkCreateBuffer(Ctx.vkDevice, &BuffCI, nullptr, &Ctx.vkSBTBuffer);
        ASSERT_GE(res, VK_SUCCESS);
        ASSERT_TRUE(Ctx.vkSBTBuffer != VK_NULL_HANDLE);

        MemInfo.buffer = Ctx.vkSBTBuffer;
        vkGetBufferMemoryRequirements2(Ctx.vkDevice, &MemInfo, &MemReqs);

        MemSize = AlignUp(MemSize, MemReqs.memoryRequirements.alignment);
        MemSize += MemReqs.memoryRequirements.size;
        MemTypeBits |= MemReqs.memoryRequirements.memoryTypeBits;

        BindMem.emplace_back([&Ctx, MemReqs, &BufferInfo](VkDeviceMemory Mem, VkDeviceSize& Offset) {
            Offset = AlignUp(Offset, MemReqs.memoryRequirements.alignment);
            vkBindBufferMemory(Ctx.vkDevice, Ctx.vkSBTBuffer, Mem, Offset);
            Offset += MemReqs.memoryRequirements.size;
            BufferInfo.buffer      = Ctx.vkSBTBuffer;
            Ctx.vkSBTBufferAddress = vkGetBufferDeviceAddressKHR(Ctx.vkDevice, &BufferInfo);
            ASSERT_TRUE(Ctx.vkSBTBufferAddress > 0);
        });
    }

    VkMemoryAllocateInfo      MemAlloc    = {};
    VkMemoryAllocateFlagsInfo MemFlagInfo = {};

    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = MemSize;
    MemAlloc.memoryTypeIndex = TestingEnvironmentVk::GetInstance()->GetMemoryTypeIndex(MemReqs.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ASSERT_TRUE(MemAlloc.memoryTypeIndex != ~0u);

    MemAlloc.pNext    = &MemFlagInfo;
    MemFlagInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    MemFlagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    res = vkAllocateMemory(Ctx.vkDevice, &MemAlloc, nullptr, &Ctx.vkBufferMemory);
    ASSERT_GE(res, VK_SUCCESS);
    ASSERT_TRUE(Ctx.vkBufferMemory != VK_NULL_HANDLE);

    VkDeviceSize Offset = 0;
    for (auto& Bind : BindMem)
    {
        Bind(Ctx.vkBufferMemory, Offset);
    }
    ASSERT_GE(MemSize, Offset);
}

void ClearRenderTarget(RTContext& Ctx, TestingSwapChainVk* pTestingSwapChainVk)
{
    pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);

    VkImageSubresourceRange Range      = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkClearColorValue       ClearValue = {};
    vkCmdClearColorImage(Ctx.vkCmdBuffer, Ctx.vkRenderTarget, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearValue, 1, &Range);

    pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, 0);
}

void PrepareForTraceRays(const RTContext& Ctx)
{
    // Barrier for TLAS & SBT
    VkMemoryBarrier Barrier = {};
    Barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    Barrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(Ctx.vkCmdBuffer,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0, 1, &Barrier, 0, nullptr, 0, nullptr);

    vkCmdBindPipeline(Ctx.vkCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, Ctx.vkPipeline);
    vkCmdBindDescriptorSets(Ctx.vkCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, Ctx.vkLayout, 0, 1, &Ctx.vkDescriptorSet, 0, nullptr);
}

void AccelStructBarrier(const RTContext& Ctx)
{
    // Barrier for vertex & index buffers, BLAS, scratch buffer, instance buffer
    VkMemoryBarrier Barrier = {};
    Barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    Barrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(Ctx.vkCmdBuffer,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0, 1, &Barrier, 0, nullptr, 0, nullptr);
}

} // namespace


void RayTracingTriangleClosestHitReferenceVk(ISwapChain* pSwapChain)
{
    enum
    {
        RAYGEN_SHADER,
        MISS_SHADER,
        HIT_SHADER,
        NUM_SHADERS
    };
    enum
    {
        RAYGEN_GROUP,
        MISS_GROUP,
        HIT_GROUP,
        NUM_GROUPS
    };

    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain,
                        [](RTGroupsHelper& rtGroups) {
                            rtGroups.SetShaderCount(NUM_SHADERS, NUM_GROUPS);
                            rtGroups.SetStage(RAYGEN_SHADER, SHADER_TYPE_RAY_GEN, GLSL::RayTracingTest1_RG);
                            rtGroups.SetStage(MISS_SHADER, SHADER_TYPE_RAY_MISS, GLSL::RayTracingTest1_RM);
                            rtGroups.SetStage(HIT_SHADER, SHADER_TYPE_RAY_CLOSEST_HIT, GLSL::RayTracingTest1_RCH);

                            rtGroups.SetGeneralGroup(RAYGEN_GROUP, RAYGEN_SHADER);
                            rtGroups.SetGeneralGroup(MISS_GROUP, MISS_SHADER);
                            rtGroups.SetTriangleHitGroup(HIT_GROUP, HIT_SHADER);
                        });

    // Create acceleration structures
    {
        const auto& Vertices = TestingConstants::TriangleClosestHit::Vertices;

        VkAccelerationStructureBuildGeometryInfoKHR     ASBuildInfo = {};
        VkAccelerationStructureBuildRangeInfoKHR        Offset      = {};
        VkAccelerationStructureGeometryKHR              Geometry    = {};
        VkAccelerationStructureBuildRangeInfoKHR const* OffsetPtr   = &Offset;

        Geometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometry.flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        Geometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        Geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Geometry.geometry.triangles.maxVertex    = _countof(Vertices);
        Geometry.geometry.triangles.vertexStride = sizeof(Vertices[0]);
        Geometry.geometry.triangles.maxVertex    = _countof(Vertices);
        Geometry.geometry.triangles.indexType    = VK_INDEX_TYPE_NONE_KHR;

        CreateBLAS(Ctx, &Geometry, OffsetPtr, 1, Ctx.BLAS);
        CreateTLAS(Ctx, 1, Ctx.TLAS);
        CreateRTBuffers(Ctx, sizeof(Vertices), 0, 1, 1, 1);

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkVertexBuffer, 0, sizeof(Vertices), Vertices);
        AccelStructBarrier(Ctx);

        Geometry.geometry.triangles.vertexData.deviceAddress = Ctx.vkVertexBufferAddress;
        Offset.primitiveCount                                = 1;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.BLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);

        VkAccelerationStructureInstanceKHR InstanceData     = {};
        InstanceData.instanceShaderBindingTableRecordOffset = 0;
        InstanceData.mask                                   = 0xFF;
        InstanceData.accelerationStructureReference         = Ctx.BLAS.vkAddress;
        InstanceData.transform.matrix[0][0]                 = 1.0f;
        InstanceData.transform.matrix[1][1]                 = 1.0f;
        InstanceData.transform.matrix[2][2]                 = 1.0f;

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkInstanceBuffer, 0, sizeof(InstanceData), &InstanceData);
        AccelStructBarrier(Ctx);

        Geometry.flags                                 = 0;
        Geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        Geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        Geometry.geometry.instances.pNext              = nullptr;
        Geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
        Geometry.geometry.instances.data.deviceAddress = Ctx.vkInstanceBufferAddress;

        Offset.primitiveCount = 1;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.TLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainVk);

    UpdateDescriptorSet(Ctx);

    // Trace rays
    {
        VkStridedDeviceAddressRegionKHR RaygenShaderBindingTable   = {};
        VkStridedDeviceAddressRegionKHR MissShaderBindingTable     = {};
        VkStridedDeviceAddressRegionKHR HitShaderBindingTable      = {};
        VkStridedDeviceAddressRegionKHR CallableShaderBindingTable = {};
        const Uint32                    ShaderGroupHandleSize      = Ctx.RayTracingProps.shaderGroupHandleSize;
        VkDeviceSize                    Offset                     = 0;

        char ShaderHandle[64] = {};
        ASSERT_GE(sizeof(ShaderHandle), ShaderGroupHandleSize);

        RaygenShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress;
        RaygenShaderBindingTable.size          = ShaderGroupHandleSize;
        RaygenShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, RAYGEN_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                               = AlignUp(Offset + RaygenShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        MissShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        MissShaderBindingTable.size          = ShaderGroupHandleSize;
        MissShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, MISS_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                              = AlignUp(Offset + MissShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        HitShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        HitShaderBindingTable.size          = ShaderGroupHandleSize;
        HitShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, HIT_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        PrepareForTraceRays(Ctx);
        vkCmdTraceRaysKHR(Ctx.vkCmdBuffer, &RaygenShaderBindingTable, &MissShaderBindingTable, &HitShaderBindingTable, &CallableShaderBindingTable, SCDesc.Width, SCDesc.Height, 1);

        pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);
    }

    auto res = vkEndCommandBuffer(Ctx.vkCmdBuffer);
    EXPECT_TRUE(res >= 0) << "Failed to end command buffer";

    pEnv->SubmitCommandBuffer(Ctx.vkCmdBuffer, true);
}


void RayTracingTriangleAnyHitReferenceVk(ISwapChain* pSwapChain)
{
    enum
    {
        RAYGEN_SHADER,
        MISS_SHADER,
        HIT_SHADER,
        ANY_HIT_SHADER,
        NUM_SHADERS
    };
    enum
    {
        RAYGEN_GROUP,
        MISS_GROUP,
        HIT_GROUP,
        NUM_GROUPS
    };

    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain,
                        [](RTGroupsHelper& rtGroups) {
                            rtGroups.SetShaderCount(NUM_SHADERS, NUM_GROUPS);
                            rtGroups.SetStage(RAYGEN_SHADER, SHADER_TYPE_RAY_GEN, GLSL::RayTracingTest2_RG);
                            rtGroups.SetStage(MISS_SHADER, SHADER_TYPE_RAY_MISS, GLSL::RayTracingTest2_RM);
                            rtGroups.SetStage(HIT_SHADER, SHADER_TYPE_RAY_CLOSEST_HIT, GLSL::RayTracingTest2_RCH);
                            rtGroups.SetStage(ANY_HIT_SHADER, SHADER_TYPE_RAY_ANY_HIT, GLSL::RayTracingTest2_RAH);

                            rtGroups.SetGeneralGroup(RAYGEN_GROUP, RAYGEN_SHADER);
                            rtGroups.SetGeneralGroup(MISS_GROUP, MISS_SHADER);
                            rtGroups.SetTriangleHitGroup(HIT_GROUP, HIT_SHADER, ANY_HIT_SHADER);
                        });

    // Create acceleration structure
    {
        const auto& Vertices = TestingConstants::TriangleAnyHit::Vertices;

        VkAccelerationStructureBuildGeometryInfoKHR     ASBuildInfo = {};
        VkAccelerationStructureBuildRangeInfoKHR        Offset      = {};
        VkAccelerationStructureGeometryKHR              Geometry    = {};
        VkAccelerationStructureBuildRangeInfoKHR const* OffsetPtr   = &Offset;

        Geometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometry.flags                           = 0;
        Geometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        Geometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        Geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Geometry.geometry.triangles.vertexStride = sizeof(Vertices[0]);
        Geometry.geometry.triangles.maxVertex    = _countof(Vertices);
        Geometry.geometry.triangles.indexType    = VK_INDEX_TYPE_NONE_KHR;

        CreateBLAS(Ctx, &Geometry, OffsetPtr, 1, Ctx.BLAS);
        CreateTLAS(Ctx, 1, Ctx.TLAS);
        CreateRTBuffers(Ctx, sizeof(Vertices), 0, 1, 1, 1);

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkVertexBuffer, 0, sizeof(Vertices), Vertices);
        AccelStructBarrier(Ctx);

        Geometry.geometry.triangles.vertexData.deviceAddress = Ctx.vkVertexBufferAddress;
        Offset.primitiveCount                                = 3;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.BLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);

        VkAccelerationStructureInstanceKHR InstanceData     = {};
        InstanceData.instanceShaderBindingTableRecordOffset = 0;
        InstanceData.mask                                   = 0xFF;
        InstanceData.accelerationStructureReference         = Ctx.BLAS.vkAddress;
        InstanceData.transform.matrix[0][0]                 = 1.0f;
        InstanceData.transform.matrix[1][1]                 = 1.0f;
        InstanceData.transform.matrix[2][2]                 = 1.0f;

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkInstanceBuffer, 0, sizeof(InstanceData), &InstanceData);
        AccelStructBarrier(Ctx);

        Geometry.flags                                 = 0;
        Geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        Geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        Geometry.geometry.instances.pNext              = nullptr;
        Geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
        Geometry.geometry.instances.data.deviceAddress = Ctx.vkInstanceBufferAddress;

        Offset.primitiveCount = 1;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.TLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainVk);

    UpdateDescriptorSet(Ctx);

    // Trace rays
    {
        VkStridedDeviceAddressRegionKHR RaygenShaderBindingTable   = {};
        VkStridedDeviceAddressRegionKHR MissShaderBindingTable     = {};
        VkStridedDeviceAddressRegionKHR HitShaderBindingTable      = {};
        VkStridedDeviceAddressRegionKHR CallableShaderBindingTable = {};
        const Uint32                    ShaderGroupHandleSize      = Ctx.RayTracingProps.shaderGroupHandleSize;
        VkDeviceSize                    Offset                     = 0;

        char ShaderHandle[64] = {};
        ASSERT_GE(sizeof(ShaderHandle), ShaderGroupHandleSize);

        RaygenShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress;
        RaygenShaderBindingTable.size          = ShaderGroupHandleSize;
        RaygenShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, RAYGEN_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                               = AlignUp(Offset + RaygenShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        MissShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        MissShaderBindingTable.size          = ShaderGroupHandleSize;
        MissShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, MISS_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                              = AlignUp(Offset + MissShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        HitShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        HitShaderBindingTable.size          = ShaderGroupHandleSize;
        HitShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, HIT_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        PrepareForTraceRays(Ctx);
        vkCmdTraceRaysKHR(Ctx.vkCmdBuffer, &RaygenShaderBindingTable, &MissShaderBindingTable, &HitShaderBindingTable, &CallableShaderBindingTable, SCDesc.Width, SCDesc.Height, 1);

        pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);
    }

    auto res = vkEndCommandBuffer(Ctx.vkCmdBuffer);
    EXPECT_TRUE(res >= 0) << "Failed to end command buffer";

    pEnv->SubmitCommandBuffer(Ctx.vkCmdBuffer, true);
}


void RayTracingProceduralIntersectionReferenceVk(ISwapChain* pSwapChain)
{
    enum
    {
        RAYGEN_SHADER,
        MISS_SHADER,
        HIT_SHADER,
        INTERSECTION_SHADER,
        NUM_SHADERS
    };
    enum
    {
        RAYGEN_GROUP,
        MISS_GROUP,
        HIT_GROUP,
        NUM_GROUPS
    };

    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain,
                        [](RTGroupsHelper& rtGroups) {
                            rtGroups.SetShaderCount(NUM_SHADERS, NUM_GROUPS);
                            rtGroups.SetStage(RAYGEN_SHADER, SHADER_TYPE_RAY_GEN, GLSL::RayTracingTest3_RG);
                            rtGroups.SetStage(MISS_SHADER, SHADER_TYPE_RAY_MISS, GLSL::RayTracingTest3_RM);
                            rtGroups.SetStage(HIT_SHADER, SHADER_TYPE_RAY_CLOSEST_HIT, GLSL::RayTracingTest3_RCH);
                            rtGroups.SetStage(INTERSECTION_SHADER, SHADER_TYPE_RAY_INTERSECTION, GLSL::RayTracingTest3_RI);

                            rtGroups.SetGeneralGroup(RAYGEN_GROUP, RAYGEN_SHADER);
                            rtGroups.SetGeneralGroup(MISS_GROUP, MISS_SHADER);
                            rtGroups.SetProceduralHitGroup(HIT_GROUP, INTERSECTION_SHADER, HIT_SHADER);
                        });

    // Create acceleration structures
    {
        const auto& Boxes = TestingConstants::ProceduralIntersection::Boxes;

        VkAccelerationStructureBuildGeometryInfoKHR     ASBuildInfo = {};
        VkAccelerationStructureBuildRangeInfoKHR        Offset      = {};
        VkAccelerationStructureGeometryKHR              Geometry    = {};
        VkAccelerationStructureBuildRangeInfoKHR const* OffsetPtr   = &Offset;

        Geometry.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometry.flags                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometry.geometryType          = VK_GEOMETRY_TYPE_AABBS_KHR;
        Geometry.geometry.aabbs.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        Geometry.geometry.aabbs.pNext  = nullptr;
        Geometry.geometry.aabbs.stride = sizeof(float3) * 2;

        CreateBLAS(Ctx, &Geometry, OffsetPtr, 1, Ctx.BLAS);
        CreateTLAS(Ctx, 1, Ctx.TLAS);
        CreateRTBuffers(Ctx, sizeof(Boxes), 0, 1, 1, 1);

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkVertexBuffer, 0, sizeof(Boxes), Boxes);
        AccelStructBarrier(Ctx);

        Geometry.geometry.aabbs.data.deviceAddress = Ctx.vkVertexBufferAddress;
        Offset.primitiveCount                      = 1;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.BLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);

        VkAccelerationStructureInstanceKHR InstanceData     = {};
        InstanceData.instanceShaderBindingTableRecordOffset = 0;
        InstanceData.mask                                   = 0xFF;
        InstanceData.accelerationStructureReference         = Ctx.BLAS.vkAddress;
        InstanceData.transform.matrix[0][0]                 = 1.0f;
        InstanceData.transform.matrix[1][1]                 = 1.0f;
        InstanceData.transform.matrix[2][2]                 = 1.0f;

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkInstanceBuffer, 0, sizeof(InstanceData), &InstanceData);
        AccelStructBarrier(Ctx);

        Geometry.flags                                 = 0;
        Geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        Geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        Geometry.geometry.instances.pNext              = nullptr;
        Geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
        Geometry.geometry.instances.data.deviceAddress = Ctx.vkInstanceBufferAddress;

        Offset.primitiveCount = 1;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.TLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Geometry;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainVk);

    UpdateDescriptorSet(Ctx);

    // Trace rays
    {
        VkStridedDeviceAddressRegionKHR RaygenShaderBindingTable   = {};
        VkStridedDeviceAddressRegionKHR MissShaderBindingTable     = {};
        VkStridedDeviceAddressRegionKHR HitShaderBindingTable      = {};
        VkStridedDeviceAddressRegionKHR CallableShaderBindingTable = {};
        const Uint32                    ShaderGroupHandleSize      = Ctx.RayTracingProps.shaderGroupHandleSize;
        VkDeviceSize                    Offset                     = 0;

        char ShaderHandle[64] = {};
        ASSERT_GE(sizeof(ShaderHandle), ShaderGroupHandleSize);

        RaygenShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress;
        RaygenShaderBindingTable.size          = ShaderGroupHandleSize;
        RaygenShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, RAYGEN_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                               = AlignUp(Offset + RaygenShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        MissShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        MissShaderBindingTable.size          = ShaderGroupHandleSize;
        MissShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, MISS_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                              = AlignUp(Offset + MissShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        HitShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        HitShaderBindingTable.size          = ShaderGroupHandleSize;
        HitShaderBindingTable.stride        = ShaderGroupHandleSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, HIT_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        PrepareForTraceRays(Ctx);
        vkCmdTraceRaysKHR(Ctx.vkCmdBuffer, &RaygenShaderBindingTable, &MissShaderBindingTable, &HitShaderBindingTable, &CallableShaderBindingTable, SCDesc.Width, SCDesc.Height, 1);

        pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);
    }

    auto res = vkEndCommandBuffer(Ctx.vkCmdBuffer);
    EXPECT_TRUE(res >= 0) << "Failed to end command buffer";

    pEnv->SubmitCommandBuffer(Ctx.vkCmdBuffer, true);
}


void RayTracingMultiGeometryReferenceVk(ISwapChain* pSwapChain)
{
    static constexpr Uint32 InstanceCount = TestingConstants::MultiGeometry::InstanceCount;
    static constexpr Uint32 GeometryCount = 3;
    static constexpr Uint32 HitGroupCount = InstanceCount * GeometryCount;

    enum
    {
        RAYGEN_SHADER,
        MISS_SHADER,
        HIT_SHADER_1,
        HIT_SHADER_2,
        NUM_SHADERS
    };
    enum
    {
        RAYGEN_GROUP,
        MISS_GROUP,
        HIT_GROUP_1,
        HIT_GROUP_2,
        NUM_GROUPS
    };

    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain,
                        [](RTGroupsHelper& rtGroups) {
                            rtGroups.SetShaderCount(NUM_SHADERS, NUM_GROUPS);
                            rtGroups.SetStage(RAYGEN_SHADER, SHADER_TYPE_RAY_GEN, GLSL::RayTracingTest4_RG);
                            rtGroups.SetStage(MISS_SHADER, SHADER_TYPE_RAY_MISS, GLSL::RayTracingTest4_RM);
                            rtGroups.SetStage(HIT_SHADER_1, SHADER_TYPE_RAY_CLOSEST_HIT, GLSL::RayTracingTest4_RCH1);
                            rtGroups.SetStage(HIT_SHADER_2, SHADER_TYPE_RAY_CLOSEST_HIT, GLSL::RayTracingTest4_RCH2);

                            rtGroups.SetGeneralGroup(RAYGEN_GROUP, RAYGEN_SHADER);
                            rtGroups.SetGeneralGroup(MISS_GROUP, MISS_SHADER);
                            rtGroups.SetTriangleHitGroup(HIT_GROUP_1, HIT_SHADER_1);
                            rtGroups.SetTriangleHitGroup(HIT_GROUP_2, HIT_SHADER_2);

                            rtGroups.AddBinding(2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, InstanceCount, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
                            rtGroups.AddBinding(3u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
                            rtGroups.AddBinding(4u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
                        });

    const auto& PrimitiveOffsets = TestingConstants::MultiGeometry::PrimitiveOffsets;
    const auto& Primitives       = TestingConstants::MultiGeometry::Primitives;

    // Create acceleration structures
    {
        const auto& Vertices = TestingConstants::MultiGeometry::Vertices;
        const auto& Indices  = TestingConstants::MultiGeometry::Indices;

        VkAccelerationStructureBuildGeometryInfoKHR     ASBuildInfo   = {};
        VkAccelerationStructureBuildRangeInfoKHR        Offsets[3]    = {};
        VkAccelerationStructureGeometryKHR              Geometries[3] = {};
        VkAccelerationStructureBuildRangeInfoKHR const* OffsetPtr     = Offsets;
        static_assert(_countof(Offsets) == _countof(Geometries), "size mismatch");
        static_assert(GeometryCount == _countof(Geometries), "size mismatch");

        Geometries[0].sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometries[0].flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometries[0].geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        Geometries[0].geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        Geometries[0].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Geometries[0].geometry.triangles.vertexStride = sizeof(Vertices[0]);
        Geometries[0].geometry.triangles.maxVertex    = PrimitiveOffsets[1] * 3;
        Geometries[0].geometry.triangles.indexType    = VK_INDEX_TYPE_UINT32;

        Geometries[1].sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometries[1].flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometries[1].geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        Geometries[1].geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        Geometries[1].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Geometries[1].geometry.triangles.vertexStride = sizeof(Vertices[0]);
        Geometries[1].geometry.triangles.maxVertex    = (PrimitiveOffsets[2] - PrimitiveOffsets[1]) * 3;
        Geometries[1].geometry.triangles.indexType    = VK_INDEX_TYPE_UINT32;

        Geometries[2].sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Geometries[2].flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometries[2].geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        Geometries[2].geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        Geometries[2].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Geometries[2].geometry.triangles.vertexStride = sizeof(Vertices[0]);
        Geometries[2].geometry.triangles.maxVertex    = (_countof(Primitives) - PrimitiveOffsets[2]) * 3;
        Geometries[2].geometry.triangles.indexType    = VK_INDEX_TYPE_UINT32;

        CreateBLAS(Ctx, Geometries, OffsetPtr, _countof(Geometries), Ctx.BLAS);
        CreateTLAS(Ctx, InstanceCount, Ctx.TLAS);
        CreateRTBuffers(Ctx, sizeof(Vertices), sizeof(Indices), InstanceCount, 1, HitGroupCount, TestingConstants::MultiGeometry::ShaderRecordSize);

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkVertexBuffer, 0, sizeof(Vertices), Vertices);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkIndexBuffer, 0, sizeof(Indices), Indices);
        AccelStructBarrier(Ctx);

        Geometries[0].geometry.triangles.vertexData.deviceAddress = Ctx.vkVertexBufferAddress;
        Geometries[0].geometry.triangles.indexData.deviceAddress  = Ctx.vkIndexBufferAddress + PrimitiveOffsets[0] * sizeof(uint) * 3;
        Geometries[1].geometry.triangles.vertexData.deviceAddress = Ctx.vkVertexBufferAddress;
        Geometries[1].geometry.triangles.indexData.deviceAddress  = Ctx.vkIndexBufferAddress + PrimitiveOffsets[1] * sizeof(uint) * 3;
        Geometries[2].geometry.triangles.vertexData.deviceAddress = Ctx.vkVertexBufferAddress;
        Geometries[2].geometry.triangles.indexData.deviceAddress  = Ctx.vkIndexBufferAddress + PrimitiveOffsets[2] * sizeof(uint) * 3;

        Offsets[0].primitiveCount = PrimitiveOffsets[1];
        Offsets[1].primitiveCount = PrimitiveOffsets[2] - PrimitiveOffsets[1];
        Offsets[2].primitiveCount = _countof(Primitives) - PrimitiveOffsets[2];

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.BLAS.vkAS;
        ASBuildInfo.geometryCount             = _countof(Geometries);
        ASBuildInfo.pGeometries               = Geometries;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);

        VkAccelerationStructureInstanceKHR InstanceData[2] = {};
        static_assert(_countof(InstanceData) == InstanceCount, "size mismatch");

        InstanceData[0].instanceShaderBindingTableRecordOffset = 0;
        InstanceData[0].mask                                   = 0xFF;
        InstanceData[0].accelerationStructureReference         = Ctx.BLAS.vkAddress;
        InstanceData[0].transform.matrix[0][0]                 = 1.0f;
        InstanceData[0].transform.matrix[1][1]                 = 1.0f;
        InstanceData[0].transform.matrix[2][2]                 = 1.0f;

        InstanceData[1].instanceShaderBindingTableRecordOffset = HitGroupCount / 2;
        InstanceData[1].mask                                   = 0xFF;
        InstanceData[1].accelerationStructureReference         = Ctx.BLAS.vkAddress;
        InstanceData[1].transform.matrix[0][0]                 = 1.0f;
        InstanceData[1].transform.matrix[1][1]                 = 1.0f;
        InstanceData[1].transform.matrix[2][2]                 = 1.0f;
        InstanceData[1].transform.matrix[0][3]                 = 0.1f;
        InstanceData[1].transform.matrix[1][3]                 = 0.5f;
        InstanceData[1].transform.matrix[2][3]                 = 0.0f;

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkInstanceBuffer, 0, sizeof(InstanceData), InstanceData);
        AccelStructBarrier(Ctx);

        VkAccelerationStructureBuildRangeInfoKHR InstOffsets = {};
        VkAccelerationStructureGeometryKHR       Instances   = {};

        OffsetPtr                  = &InstOffsets;
        InstOffsets.primitiveCount = _countof(InstanceData);

        Instances.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        Instances.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        Instances.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        Instances.geometry.instances.arrayOfPointers    = VK_FALSE;
        Instances.geometry.instances.data.deviceAddress = Ctx.vkInstanceBufferAddress;

        ASBuildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        ASBuildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        ASBuildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        ASBuildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
        ASBuildInfo.dstAccelerationStructure  = Ctx.TLAS.vkAS;
        ASBuildInfo.geometryCount             = 1;
        ASBuildInfo.pGeometries               = &Instances;
        ASBuildInfo.scratchData.deviceAddress = Ctx.vkScratchBufferAddress;

        vkCmdBuildAccelerationStructuresKHR(Ctx.vkCmdBuffer, 1, &ASBuildInfo, &OffsetPtr);
    }

    ClearRenderTarget(Ctx, pTestingSwapChainVk);
    UpdateDescriptorSet(Ctx);

    VkBuffer       vkPerInstanceBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory vkPerInstanceBufferMemory = VK_NULL_HANDLE;
    VkBuffer       vkPrimitiveBuffer         = VK_NULL_HANDLE;
    VkDeviceMemory vkPrimitiveBufferMemory   = VK_NULL_HANDLE;
    {
        pEnv->CreateBuffer(sizeof(PrimitiveOffsets), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkPerInstanceBufferMemory, vkPerInstanceBuffer);
        pEnv->CreateBuffer(sizeof(Primitives), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkPrimitiveBufferMemory, vkPrimitiveBuffer);

        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, vkPerInstanceBuffer, 0, sizeof(PrimitiveOffsets), PrimitiveOffsets);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, vkPrimitiveBuffer, 0, sizeof(Primitives), Primitives);

        VkWriteDescriptorSet   DescriptorWrite = {};
        VkDescriptorBufferInfo BufInfo         = {};

        DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrite.dstSet          = Ctx.vkDescriptorSet;
        DescriptorWrite.dstBinding      = 4;
        DescriptorWrite.dstArrayElement = 0;
        DescriptorWrite.descriptorCount = 1;
        DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        DescriptorWrite.pBufferInfo     = &BufInfo;
        BufInfo.buffer                  = Ctx.vkVertexBuffer;
        BufInfo.range                   = VK_WHOLE_SIZE;
        vkUpdateDescriptorSets(Ctx.vkDevice, 1, &DescriptorWrite, 0, nullptr);

        DescriptorWrite.dstBinding = 3;
        BufInfo.buffer             = vkPrimitiveBuffer;
        vkUpdateDescriptorSets(Ctx.vkDevice, 1, &DescriptorWrite, 0, nullptr);

        DescriptorWrite.dstBinding = 2;
        BufInfo.buffer             = vkPerInstanceBuffer;
        vkUpdateDescriptorSets(Ctx.vkDevice, 1, &DescriptorWrite, 0, nullptr);

        DescriptorWrite.dstArrayElement = 1;
        vkUpdateDescriptorSets(Ctx.vkDevice, 1, &DescriptorWrite, 0, nullptr);
    }

    // Trace rays
    {
        VkStridedDeviceAddressRegionKHR RaygenShaderBindingTable   = {};
        VkStridedDeviceAddressRegionKHR MissShaderBindingTable     = {};
        VkStridedDeviceAddressRegionKHR HitShaderBindingTable      = {};
        VkStridedDeviceAddressRegionKHR CallableShaderBindingTable = {};
        const Uint32                    ShaderGroupHandleSize      = Ctx.RayTracingProps.shaderGroupHandleSize;
        const Uint32                    ShaderRecordSize           = ShaderGroupHandleSize + TestingConstants::MultiGeometry::ShaderRecordSize;
        const auto&                     Weights                    = TestingConstants::MultiGeometry::Weights;
        VkDeviceSize                    Offset                     = 0;

        char ShaderHandle[64] = {};
        ASSERT_GE(sizeof(ShaderHandle), ShaderGroupHandleSize);

        RaygenShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress;
        RaygenShaderBindingTable.size          = ShaderRecordSize;
        RaygenShaderBindingTable.stride        = ShaderRecordSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, RAYGEN_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                               = AlignUp(Offset + RaygenShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        MissShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        MissShaderBindingTable.size          = ShaderRecordSize;
        MissShaderBindingTable.stride        = ShaderRecordSize;

        vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, MISS_GROUP, 1, ShaderGroupHandleSize, ShaderHandle);
        vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, Offset, ShaderGroupHandleSize, ShaderHandle);

        Offset                              = AlignUp(Offset + MissShaderBindingTable.size, Ctx.RayTracingProps.shaderGroupBaseAlignment);
        HitShaderBindingTable.deviceAddress = Ctx.vkSBTBufferAddress + Offset;
        HitShaderBindingTable.size          = ShaderRecordSize * HitGroupCount;
        HitShaderBindingTable.stride        = ShaderRecordSize;

        const auto SetHitGroup = [&](Uint32 Index, Uint32 ShaderIndex, const void* ShaderRecord) {
            VERIFY_EXPR(Index < HitGroupCount);
            VkDeviceSize GroupOffset = Offset + Index * ShaderRecordSize;
            vkGetRayTracingShaderGroupHandlesKHR(Ctx.vkDevice, Ctx.vkPipeline, ShaderIndex, 1, ShaderGroupHandleSize, ShaderHandle);
            vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, GroupOffset, ShaderGroupHandleSize, ShaderHandle);
            vkCmdUpdateBuffer(Ctx.vkCmdBuffer, Ctx.vkSBTBuffer, GroupOffset + ShaderGroupHandleSize, sizeof(Weights[0]), ShaderRecord);
        };
        // instance 1
        SetHitGroup(0, HIT_GROUP_1, &Weights[0]); // geometry 1
        SetHitGroup(1, HIT_GROUP_1, &Weights[1]); // geometry 2
        SetHitGroup(2, HIT_GROUP_1, &Weights[2]); // geometry 3
        // instance 2
        SetHitGroup(3, HIT_GROUP_2, &Weights[3]); // geometry 1
        SetHitGroup(4, HIT_GROUP_2, &Weights[4]); // geometry 2
        SetHitGroup(5, HIT_GROUP_2, &Weights[5]); // geometry 3

        PrepareForTraceRays(Ctx);
        vkCmdTraceRaysKHR(Ctx.vkCmdBuffer, &RaygenShaderBindingTable, &MissShaderBindingTable, &HitShaderBindingTable, &CallableShaderBindingTable, SCDesc.Width, SCDesc.Height, 1);

        pTestingSwapChainVk->TransitionRenderTarget(Ctx.vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);
    }

    auto res = vkEndCommandBuffer(Ctx.vkCmdBuffer);
    VERIFY(res >= 0, "Failed to end command buffer");

    pEnv->SubmitCommandBuffer(Ctx.vkCmdBuffer, true);

    vkDestroyBuffer(Ctx.vkDevice, vkPerInstanceBuffer, nullptr);
    vkDestroyBuffer(Ctx.vkDevice, vkPrimitiveBuffer, nullptr);
    vkFreeMemory(Ctx.vkDevice, vkPerInstanceBufferMemory, nullptr);
    vkFreeMemory(Ctx.vkDevice, vkPrimitiveBufferMemory, nullptr);
}


} // namespace Testing

} // namespace Diligent
