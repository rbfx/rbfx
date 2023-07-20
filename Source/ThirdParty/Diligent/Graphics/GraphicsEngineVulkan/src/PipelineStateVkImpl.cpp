/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "pch.h"

#include "PipelineStateVkImpl.hpp"

#include <array>
#include <unordered_map>

#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "ShaderVkImpl.hpp"
#include "RenderPassVkImpl.hpp"
#include "ShaderResourceBindingVkImpl.hpp"
#include "PipelineStateCacheVkImpl.hpp"

#include "VulkanTypeConversions.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"

#if !DILIGENT_NO_HLSL
#    include "SPIRVTools.hpp"
#endif

namespace Diligent
{

constexpr INTERFACE_ID PipelineStateVkImpl::IID_InternalImpl;

namespace
{

void InitPipelineShaderStages(const VulkanUtilities::VulkanLogicalDevice&        LogicalDevice,
                              PipelineStateVkImpl::TShaderStages&                ShaderStages,
                              std::vector<VulkanUtilities::ShaderModuleWrapper>& ShaderModules,
                              std::vector<VkPipelineShaderStageCreateInfo>&      Stages)
{
    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        const auto& Shaders    = ShaderStages[s].Shaders;
        auto&       SPIRVs     = ShaderStages[s].SPIRVs;
        const auto  ShaderType = ShaderStages[s].Type;

        VERIFY_EXPR(Shaders.size() == SPIRVs.size());

        VkPipelineShaderStageCreateInfo StageCI{};
        StageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        StageCI.pNext = nullptr;
        StageCI.flags = 0; //  reserved for future use
        StageCI.stage = ShaderTypeToVkShaderStageFlagBit(ShaderType);

        VkShaderModuleCreateInfo ShaderModuleCI{};
        ShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderModuleCI.pNext = nullptr;
        ShaderModuleCI.flags = 0;

        for (size_t i = 0; i < Shaders.size(); ++i)
        {
            auto* pShader = Shaders[i];
            auto& SPIRV   = SPIRVs[i];

            ShaderModuleCI.codeSize = SPIRV.size() * sizeof(uint32_t);
            ShaderModuleCI.pCode    = SPIRV.data();

            ShaderModules.push_back(LogicalDevice.CreateShaderModule(ShaderModuleCI, pShader->GetDesc().Name));

            StageCI.module              = ShaderModules.back();
            StageCI.pName               = pShader->GetEntryPoint();
            StageCI.pSpecializationInfo = nullptr;

            Stages.push_back(StageCI);
        }
    }

    VERIFY_EXPR(ShaderModules.size() == Stages.size());
}


void CreateComputePipeline(RenderDeviceVkImpl*                           pDeviceVk,
                           std::vector<VkPipelineShaderStageCreateInfo>& Stages,
                           const PipelineLayoutVk&                       Layout,
                           const PipelineStateDesc&                      PSODesc,
                           VulkanUtilities::PipelineWrapper&             Pipeline,
                           VkPipelineCache                               vkPSOCache)
{
    const auto& LogicalDevice = pDeviceVk->GetLogicalDevice();

    VkComputePipelineCreateInfo PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCI.pNext = nullptr;
#ifdef DILIGENT_DEBUG
    PipelineCI.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#endif
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = -1;             // an index into the pCreateInfos parameter to use as a pipeline to derive from

    PipelineCI.stage  = Stages[0];
    PipelineCI.layout = Layout.GetVkPipelineLayout();

    Pipeline = LogicalDevice.CreateComputePipeline(PipelineCI, vkPSOCache, PSODesc.Name);
}


void CreateGraphicsPipeline(RenderDeviceVkImpl*                           pDeviceVk,
                            std::vector<VkPipelineShaderStageCreateInfo>& Stages,
                            const PipelineLayoutVk&                       Layout,
                            const PipelineStateDesc&                      PSODesc,
                            const GraphicsPipelineDesc&                   GraphicsPipeline,
                            VulkanUtilities::PipelineWrapper&             Pipeline,
                            RefCntAutoPtr<IRenderPass>&                   pRenderPass,
                            VkPipelineCache                               vkPSOCache)
{
    const auto& LogicalDevice  = pDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice = pDeviceVk->GetPhysicalDevice();
    auto&       RPCache        = pDeviceVk->GetImplicitRenderPassCache();

    if (pRenderPass == nullptr)
    {
        RenderPassCache::RenderPassCacheKey Key{
            GraphicsPipeline.NumRenderTargets,
            GraphicsPipeline.SmplDesc.Count,
            GraphicsPipeline.RTVFormats,
            GraphicsPipeline.DSVFormat,
            (GraphicsPipeline.ShadingRateFlags & PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED) != 0,
            GraphicsPipeline.ReadOnlyDSV};
        pRenderPass = RPCache.GetRenderPass(Key);
        if (pRenderPass == nullptr)
            LOG_ERROR_AND_THROW("Failed to create default render pass.");
    }

    VkGraphicsPipelineCreateInfo PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCI.pNext = nullptr;
#ifdef DILIGENT_DEBUG
    PipelineCI.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#endif

    PipelineCI.stageCount = static_cast<Uint32>(Stages.size());
    PipelineCI.pStages    = Stages.data();
    PipelineCI.layout     = Layout.GetVkPipelineLayout();

    VkPipelineVertexInputStateCreateInfo           VertexInputStateCI   = {};
    VkPipelineVertexInputDivisorStateCreateInfoEXT VertexInputDivisorCI = {};

    std::array<VkVertexInputBindingDescription, MAX_LAYOUT_ELEMENTS>           BindingDescriptions;
    std::array<VkVertexInputAttributeDescription, MAX_LAYOUT_ELEMENTS>         AttributeDescription;
    std::array<VkVertexInputBindingDivisorDescriptionEXT, MAX_LAYOUT_ELEMENTS> VertexBindingDivisors;
    InputLayoutDesc_To_VkVertexInputStateCI(GraphicsPipeline.InputLayout, VertexInputStateCI, VertexInputDivisorCI, BindingDescriptions, AttributeDescription, VertexBindingDivisors);
    PipelineCI.pVertexInputState = &VertexInputStateCI;

    if (VertexInputDivisorCI.vertexBindingDivisorCount > 0)
    {
        if (!pDeviceVk->GetFeatures().InstanceDataStepRate)
            LOG_ERROR_MESSAGE("InstanceDataStepRate device feature is not enabled");

        VertexInputStateCI.pNext = &VertexInputDivisorCI;
    }

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI{};
    InputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.pNext = nullptr;
    InputAssemblyCI.flags = 0; // reserved for future use
    InputAssemblyCI.primitiveRestartEnable =
        (GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ||
         GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ ||
         GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_LINE_STRIP ||
         GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ) ?
        VK_TRUE :
        VK_FALSE;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;


    VkPipelineTessellationStateCreateInfo TessStateCI{};
    TessStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    TessStateCI.pNext             = nullptr;
    TessStateCI.flags             = 0; // reserved for future use
    PipelineCI.pTessellationState = &TessStateCI;

    if (PSODesc.PipelineType == PIPELINE_TYPE_MESH)
    {
        // Input assembly is not used in the mesh pipeline, so topology may contain any value.
        // Validation layers may generate a warning if point_list topology is used, so use MAX_ENUM value.
        InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

        // Vertex input state and tessellation state are ignored in a mesh pipeline and should be null,
        // but there is a bug in validation layers that makes them crash.
        //PipelineCI.pVertexInputState  = nullptr;
        PipelineCI.pTessellationState = nullptr;
    }
    else
    {
        PrimitiveTopology_To_VkPrimitiveTopologyAndPatchCPCount(GraphicsPipeline.PrimitiveTopology, InputAssemblyCI.topology, TessStateCI.patchControlPoints);
    }

    VkPipelineViewportStateCreateInfo ViewPortStateCI{};
    ViewPortStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewPortStateCI.pNext = nullptr;
    ViewPortStateCI.flags = 0; // reserved for future use
    ViewPortStateCI.viewportCount =
        GraphicsPipeline.NumViewports;                            // Even though we use dynamic viewports, the number of viewports used
                                                                  // by the pipeline is still specified by the viewportCount member (23.5)
    ViewPortStateCI.pViewports   = nullptr;                       // We will be using dynamic viewport & scissor states
    ViewPortStateCI.scissorCount = ViewPortStateCI.viewportCount; // the number of scissors must match the number of viewports (23.5)
                                                                  // (why the hell it is in the struct then?)
    VkRect2D ScissorRect = {};
    if (GraphicsPipeline.RasterizerDesc.ScissorEnable)
    {
        ViewPortStateCI.pScissors = nullptr; // Ignored if the scissor state is dynamic
    }
    else
    {
        const auto& Props = PhysicalDevice.GetProperties();
        // There are limitations on the viewport width and height (23.5), but
        // it is not clear if there are limitations on the scissor rect width and
        // height
        ScissorRect.extent.width  = Props.limits.maxViewportDimensions[0];
        ScissorRect.extent.height = Props.limits.maxViewportDimensions[1];
        ViewPortStateCI.pScissors = &ScissorRect;
    }
    PipelineCI.pViewportState = &ViewPortStateCI;

    VkPipelineRasterizationStateCreateInfo RasterizerStateCI =
        RasterizerStateDesc_To_VkRasterizationStateCI(GraphicsPipeline.RasterizerDesc);
    PipelineCI.pRasterizationState = &RasterizerStateCI;

    // Multisample state (24)
    VkPipelineMultisampleStateCreateInfo MSStateCI{};
    MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MSStateCI.pNext = nullptr;
    MSStateCI.flags = 0; // reserved for future use
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
    // pMultisampleState must be the same as the sample count for those subpass attachments
    MSStateCI.rasterizationSamples = static_cast<VkSampleCountFlagBits>(GraphicsPipeline.SmplDesc.Count);
    MSStateCI.sampleShadingEnable  = VK_FALSE;
    MSStateCI.minSampleShading     = 0;                                // a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
    uint32_t SampleMask[]          = {GraphicsPipeline.SampleMask, 0}; // Vulkan spec allows up to 64 samples
    MSStateCI.pSampleMask          = SampleMask;                       // an array of static coverage information that is ANDed with
                                                                       // the coverage information generated during rasterization (25.3)
    MSStateCI.alphaToCoverageEnable = VK_FALSE;                        // whether a temporary coverage value is generated based on
                                                                       // the alpha component of the fragment's first color output
    MSStateCI.alphaToOneEnable   = VK_FALSE;                           // whether the alpha component of the fragment's first color output is replaced with one
    PipelineCI.pMultisampleState = &MSStateCI;

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI =
        DepthStencilStateDesc_To_VkDepthStencilStateCI(GraphicsPipeline.DepthStencilDesc);
    PipelineCI.pDepthStencilState = &DepthStencilStateCI;

    const auto& RPDesc           = pRenderPass->GetDesc();
    const auto  NumRTAttachments = RPDesc.pSubpasses[GraphicsPipeline.SubpassIndex].RenderTargetAttachmentCount;
    VERIFY_EXPR(GraphicsPipeline.pRenderPass != nullptr || GraphicsPipeline.NumRenderTargets == NumRTAttachments);
    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachmentStates(NumRTAttachments);

    VkPipelineColorBlendStateCreateInfo BlendStateCI{};
    BlendStateCI.pAttachments    = !ColorBlendAttachmentStates.empty() ? ColorBlendAttachmentStates.data() : nullptr;
    BlendStateCI.attachmentCount = NumRTAttachments; // must equal the colorAttachmentCount for the subpass
                                                     // in which this pipeline is used.
    BlendStateDesc_To_VkBlendStateCI(GraphicsPipeline.BlendDesc, BlendStateCI, ColorBlendAttachmentStates);
    PipelineCI.pColorBlendState = &BlendStateCI;


    VkPipelineDynamicStateCreateInfo DynamicStateCI{};
    DynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCI.pNext = nullptr;
    DynamicStateCI.flags = 0; // reserved for future use
    std::vector<VkDynamicState> DynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT, // pViewports state in VkPipelineViewportStateCreateInfo will be ignored and must be
                                       // set dynamically with vkCmdSetViewport before any draw commands. The number of viewports
                                       // used by a pipeline is still specified by the viewportCount member of
                                       // VkPipelineViewportStateCreateInfo.

            VK_DYNAMIC_STATE_BLEND_CONSTANTS, // blendConstants state in VkPipelineColorBlendStateCreateInfo will be ignored
                                              // and must be set dynamically with vkCmdSetBlendConstants

            VK_DYNAMIC_STATE_STENCIL_REFERENCE // specifies that the reference state in VkPipelineDepthStencilStateCreateInfo
                                               // for both front and back will be ignored and must be set dynamically
                                               // with vkCmdSetStencilReference
        };

    if (GraphicsPipeline.RasterizerDesc.ScissorEnable)
    {
        // pScissors state in VkPipelineViewportStateCreateInfo will be ignored and must be set
        // dynamically with vkCmdSetScissor before any draw commands. The number of scissor rectangles
        // used by a pipeline is still specified by the scissorCount member of
        // VkPipelineViewportStateCreateInfo.
        DynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }

    if (GraphicsPipeline.ShadingRateFlags != PIPELINE_SHADING_RATE_FLAG_NONE &&
        pDeviceVk->GetLogicalDevice().GetEnabledExtFeatures().ShadingRate.attachmentFragmentShadingRate != VK_FALSE)
    {
        // VkPipelineFragmentShadingRateStateCreateInfoKHR will be ignored
        // and must be set dynamically with vkCmdSetFragmentShadingRateKHR before any drawing commands.
        DynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
    }

    DynamicStateCI.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
    DynamicStateCI.pDynamicStates    = DynamicStates.data();
    PipelineCI.pDynamicState         = &DynamicStateCI;


    PipelineCI.renderPass         = pRenderPass.RawPtr<IRenderPassVk>()->GetVkRenderPass();
    PipelineCI.subpass            = GraphicsPipeline.SubpassIndex;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = -1;             // an index into the pCreateInfos parameter to use as a pipeline to derive from

    Pipeline = LogicalDevice.CreateGraphicsPipeline(PipelineCI, vkPSOCache, PSODesc.Name);
}


void CreateRayTracingPipeline(RenderDeviceVkImpl*                                      pDeviceVk,
                              const std::vector<VkPipelineShaderStageCreateInfo>&      vkStages,
                              const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& vkShaderGroups,
                              const PipelineLayoutVk&                                  Layout,
                              const PipelineStateDesc&                                 PSODesc,
                              const RayTracingPipelineDesc&                            RayTracingPipeline,
                              VulkanUtilities::PipelineWrapper&                        Pipeline,
                              VkPipelineCache                                          vkPSOCache)
{
    const auto& LogicalDevice = pDeviceVk->GetLogicalDevice();

    VkRayTracingPipelineCreateInfoKHR PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    PipelineCI.pNext = nullptr;
#ifdef DILIGENT_DEBUG
    PipelineCI.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#endif

    PipelineCI.stageCount                   = static_cast<Uint32>(vkStages.size());
    PipelineCI.pStages                      = vkStages.data();
    PipelineCI.groupCount                   = static_cast<Uint32>(vkShaderGroups.size());
    PipelineCI.pGroups                      = vkShaderGroups.data();
    PipelineCI.maxPipelineRayRecursionDepth = RayTracingPipeline.MaxRecursionDepth;
    PipelineCI.pLibraryInfo                 = nullptr;
    PipelineCI.pLibraryInterface            = nullptr;
    PipelineCI.pDynamicState                = nullptr;
    PipelineCI.layout                       = Layout.GetVkPipelineLayout();
    PipelineCI.basePipelineHandle           = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex            = -1;             // an index into the pCreateInfos parameter to use as a pipeline to derive from

    Pipeline = LogicalDevice.CreateRayTracingPipeline(PipelineCI, vkPSOCache, PSODesc.Name);
}


std::vector<VkRayTracingShaderGroupCreateInfoKHR> BuildRTShaderGroupDescription(
    const RayTracingPipelineStateCreateInfo&            CreateInfo,
    const std::unordered_map<HashMapStringKey, Uint32>& NameToGroupIndex,
    const PipelineStateVkImpl::TShaderStages&           ShaderStages)
{
    // Returns the shader module index in the PSO create info
    auto GetShaderModuleIndex = [&ShaderStages](const IShader* pShader) {
        if (pShader == nullptr)
            return VK_SHADER_UNUSED_KHR;

        RefCntAutoPtr<ShaderVkImpl> pShaderVk{const_cast<IShader*>(pShader), ShaderVkImpl::IID_InternalImpl};
        VERIFY(pShaderVk, "Unexpected shader object implementation");

        const auto ShaderType = pShaderVk->GetDesc().ShaderType;
        // Shader modules are initialized in the same order by InitPipelineShaderStages().
        uint32_t idx = 0;
        for (const auto& Stage : ShaderStages)
        {
            if (ShaderType == Stage.Type)
            {
                for (Uint32 i = 0; i < Stage.Shaders.size(); ++i, ++idx)
                {
                    if (Stage.Shaders[i] == pShaderVk)
                        return idx;
                }
                UNEXPECTED("Unable to find shader '", pShaderVk->GetDesc().Name, "' in the shader stage. This should never happen and is a bug.");
                return VK_SHADER_UNUSED_KHR;
            }
            else
            {
                idx += static_cast<Uint32>(Stage.Count());
            }
        }
        UNEXPECTED("Unable to find corresponding shader stage for shader '", pShaderVk->GetDesc().Name, "'. This should never happen and is a bug.");
        return VK_SHADER_UNUSED_KHR;
    };

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups;
    ShaderGroups.reserve(size_t{CreateInfo.GeneralShaderCount} + size_t{CreateInfo.TriangleHitShaderCount} + size_t{CreateInfo.ProceduralHitShaderCount});

    for (Uint32 i = 0; i < CreateInfo.GeneralShaderCount; ++i)
    {
        const auto& GeneralShader = CreateInfo.pGeneralShaders[i];

        VkRayTracingShaderGroupCreateInfoKHR Group = {};

        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        Group.generalShader      = GetShaderModuleIndex(GeneralShader.pShader);
        Group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        Group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;

#ifdef DILIGENT_DEBUG
        {
            auto Iter = NameToGroupIndex.find(GeneralShader.Name);
            VERIFY(Iter != NameToGroupIndex.end(),
                   "Can't find general shader '", GeneralShader.Name,
                   "'. This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same general shaders.");
            VERIFY(Iter->second == ShaderGroups.size(),
                   "General shader group '", GeneralShader.Name, "' index mismatch: (", Iter->second, " != ", ShaderGroups.size(),
                   "). This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same shaders in the same order.");
        }
#endif

        ShaderGroups.push_back(Group);
    }

    for (Uint32 i = 0; i < CreateInfo.TriangleHitShaderCount; ++i)
    {
        const auto& TriHitShader = CreateInfo.pTriangleHitShaders[i];

        VkRayTracingShaderGroupCreateInfoKHR Group{};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        Group.generalShader      = VK_SHADER_UNUSED_KHR;
        Group.closestHitShader   = GetShaderModuleIndex(TriHitShader.pClosestHitShader);
        Group.anyHitShader       = GetShaderModuleIndex(TriHitShader.pAnyHitShader);
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;

#ifdef DILIGENT_DEBUG
        {
            auto Iter = NameToGroupIndex.find(TriHitShader.Name);
            VERIFY(Iter != NameToGroupIndex.end(),
                   "Can't find triangle hit group '", TriHitShader.Name,
                   "'. This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same hit groups.");
            VERIFY(Iter->second == ShaderGroups.size(),
                   "Triangle hit group '", TriHitShader.Name, "' index mismatch: (", Iter->second, " != ", ShaderGroups.size(),
                   "). This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same hit groups in the same order.");
        }
#endif

        ShaderGroups.push_back(Group);
    }

    for (Uint32 i = 0; i < CreateInfo.ProceduralHitShaderCount; ++i)
    {
        const auto& ProcHitShader = CreateInfo.pProceduralHitShaders[i];

        VkRayTracingShaderGroupCreateInfoKHR Group{};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        Group.generalShader      = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = GetShaderModuleIndex(ProcHitShader.pIntersectionShader);
        Group.closestHitShader   = GetShaderModuleIndex(ProcHitShader.pClosestHitShader);
        Group.anyHitShader       = GetShaderModuleIndex(ProcHitShader.pAnyHitShader);

#ifdef DILIGENT_DEBUG
        {
            auto Iter = NameToGroupIndex.find(ProcHitShader.Name);
            VERIFY(Iter != NameToGroupIndex.end(),
                   "Can't find procedural hit group '", ProcHitShader.Name,
                   "'. This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same hit groups.");
            VERIFY(Iter->second == ShaderGroups.size(),
                   "Procedural hit group '", ProcHitShader.Name, "' index mismatch: (", Iter->second, " != ", ShaderGroups.size(),
                   "). This looks to be a bug as NameToGroupIndex is initialized by "
                   "CopyRTShaderGroupNames() that processes the same hit groups in the same order.");
        }
#endif

        ShaderGroups.push_back(Group);
    }

    return ShaderGroups;
}

void VerifyResourceMerge(const char*                       PSOName,
                         const SPIRVShaderResourceAttribs& ExistingRes,
                         const SPIRVShaderResourceAttribs& NewResAttribs)
{
#define LOG_RESOURCE_MERGE_ERROR_AND_THROW(PropertyName)                                                  \
    LOG_ERROR_AND_THROW("Shader variable '", NewResAttribs.Name,                                          \
                        "' is shared between multiple shaders in pipeline '", (PSOName ? PSOName : ""),   \
                        "', but its " PropertyName " varies. A variable shared between multiple shaders " \
                        "must be defined identically in all shaders. Either use separate variables for "  \
                        "different shader stages, change resource name or make sure that " PropertyName " is consistent.");

    if (ExistingRes.Type != NewResAttribs.Type)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("type");

    if (ExistingRes.ResourceDim != NewResAttribs.ResourceDim)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("resource dimension");

    if (ExistingRes.ArraySize != NewResAttribs.ArraySize)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("array size");

    if (ExistingRes.IsMS != NewResAttribs.IsMS)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("multisample state");
#undef LOG_RESOURCE_MERGE_ERROR_AND_THROW
}

} // namespace


PipelineStateVkImpl::ShaderStageInfo::ShaderStageInfo(const ShaderVkImpl* pShader) :
    Type{pShader->GetDesc().ShaderType},
    Shaders{pShader},
    SPIRVs{pShader->GetSPIRV()}
{}

void PipelineStateVkImpl::ShaderStageInfo::Append(const ShaderVkImpl* pShader)
{
    VERIFY_EXPR(pShader != nullptr);
    VERIFY(std::find(Shaders.begin(), Shaders.end(), pShader) == Shaders.end(),
           "Shader '", pShader->GetDesc().Name, "' already exists in the stage. Shaders must be deduplicated.");

    const auto NewShaderType = pShader->GetDesc().ShaderType;
    if (Type == SHADER_TYPE_UNKNOWN)
    {
        VERIFY_EXPR(Shaders.empty() && SPIRVs.empty());
        Type = NewShaderType;
    }
    else
    {
        VERIFY(Type == NewShaderType, "The type (", GetShaderTypeLiteralName(NewShaderType),
               ") of shader '", pShader->GetDesc().Name, "' being added to the stage is inconsistent with the stage type (",
               GetShaderTypeLiteralName(Type), ").");
    }
    Shaders.push_back(pShader);
    SPIRVs.push_back(pShader->GetSPIRV());
}

size_t PipelineStateVkImpl::ShaderStageInfo::Count() const
{
    VERIFY_EXPR(Shaders.size() == SPIRVs.size());
    return Shaders.size();
}

PipelineResourceSignatureDescWrapper PipelineStateVkImpl::GetDefaultResourceSignatureDesc(
    const TShaderStages&              ShaderStages,
    const char*                       PSOName,
    const PipelineResourceLayoutDesc& ResourceLayout,
    Uint32                            SRBAllocationGranularity) noexcept(false)
{
    PipelineResourceSignatureDescWrapper SignDesc{PSOName, ResourceLayout, SRBAllocationGranularity};

    std::unordered_map<ShaderResourceHashKey, const SPIRVShaderResourceAttribs&, ShaderResourceHashKey::Hasher> UniqueResources;
    for (auto& Stage : ShaderStages)
    {
        for (auto* pShader : Stage.Shaders)
        {
            const auto& ShaderResources = *pShader->GetShaderResources();
            ShaderResources.ProcessResources(
                [&](const SPIRVShaderResourceAttribs& Attribs, Uint32) //
                {
                    // We can't skip immutable samplers because immutable sampler arrays have to be defined
                    // as both resource and sampler.
                    //if (Res.Type == SPIRVShaderResourceAttribs::SeparateSampler &&
                    //    FindImmutableSampler(ResourceLayout.ImmutableSamplers, ResourceLayout.NumImmutableSamplers, Stage.Type, Res.Name,
                    //                         ShaderResources.GetCombinedSamplerSuffix()) >= 0)
                    //{
                    //    // Skip separate immutable samplers - they are not resources
                    //    return;
                    //}

                    const char* const SamplerSuffix =
                        (ShaderResources.IsUsingCombinedSamplers() && Attribs.Type == SPIRVShaderResourceAttribs::ResourceType::SeparateSampler) ?
                        ShaderResources.GetCombinedSamplerSuffix() :
                        nullptr;

                    const auto VarDesc = FindPipelineResourceLayoutVariable(ResourceLayout, Attribs.Name, Stage.Type, SamplerSuffix);
                    // Note that Attribs.Name != VarDesc.Name for combined samplers
                    const auto it_assigned = UniqueResources.emplace(ShaderResourceHashKey{VarDesc.ShaderStages, Attribs.Name}, Attribs);
                    if (it_assigned.second)
                    {
                        if (Attribs.ArraySize == 0)
                        {
                            LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", pShader->GetDesc().Name, "' is a runtime-sized array. ",
                                                "You must use explicit resource signature to specify the array size.");
                        }

                        const auto ResType = SPIRVShaderResourceAttribs::GetShaderResourceType(Attribs.Type);
                        const auto Flags   = SPIRVShaderResourceAttribs::GetPipelineResourceFlags(Attribs.Type) | ShaderVariableFlagsToPipelineResourceFlags(VarDesc.Flags);
                        SignDesc.AddResource(VarDesc.ShaderStages, Attribs.Name, Attribs.ArraySize, ResType, VarDesc.Type, Flags);
                    }
                    else
                    {
                        VerifyResourceMerge(PSOName, it_assigned.first->second, Attribs);
                    }
                });

            // Merge combined sampler suffixes
            if (ShaderResources.IsUsingCombinedSamplers() && ShaderResources.GetNumSepSmplrs() > 0)
            {
                SignDesc.SetCombinedSamplerSuffix(ShaderResources.GetCombinedSamplerSuffix());
            }
        }
    }

    return SignDesc;
}

void PipelineStateVkImpl::RemapOrVerifyShaderResources(
    TShaderStages&                                       ShaderStages,
    const RefCntAutoPtr<PipelineResourceSignatureVkImpl> pSignatures[],
    const Uint32                                         SignatureCount,
    const TBindIndexToDescSetIndex&                      BindIndexToDescSetIndex,
    bool                                                 bVerifyOnly,
    bool                                                 bStripReflection,
    const char*                                          PipelineName,
    TShaderResources*                                    pDvpShaderResources,
    TResourceAttibutions*                                pDvpResourceAttibutions) noexcept(false)
{
    if (PipelineName == nullptr)
        PipelineName = "<null>";

    // Verify that pipeline layout is compatible with shader resources and
    // remap resource bindings.
    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        const auto& Shaders    = ShaderStages[s].Shaders;
        auto&       SPIRVs     = ShaderStages[s].SPIRVs;
        const auto  ShaderType = ShaderStages[s].Type;

        VERIFY_EXPR(Shaders.size() == SPIRVs.size());

        for (size_t i = 0; i < Shaders.size(); ++i)
        {
            auto* pShader = Shaders[i];
            auto& SPIRV   = SPIRVs[i];

            const auto& pShaderResources = pShader->GetShaderResources();
            VERIFY_EXPR(pShaderResources);

            if (pDvpShaderResources)
                pDvpShaderResources->emplace_back(pShaderResources);

            pShaderResources->ProcessResources(
                [&](const SPIRVShaderResourceAttribs& SPIRVAttribs, Uint32) //
                {
                    const auto ResAttribution = GetResourceAttribution(SPIRVAttribs.Name, ShaderType, pSignatures, SignatureCount);
                    if (!ResAttribution)
                    {
                        LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource '", SPIRVAttribs.Name,
                                            "' that is not present in any pipeline resource signature used to create pipeline state '",
                                            PipelineName, "'.");
                    }

                    const auto& SignDesc = ResAttribution.pSignature->GetDesc();
                    const auto  ResType  = SPIRVShaderResourceAttribs::GetShaderResourceType(SPIRVAttribs.Type);
                    const auto  Flags    = SPIRVShaderResourceAttribs::GetPipelineResourceFlags(SPIRVAttribs.Type);

                    Uint32 ResourceBinding = ~0u;
                    Uint32 DescriptorSet   = ~0u;
                    if (ResAttribution.ResourceIndex != ResourceAttribution::InvalidResourceIndex)
                    {
                        const auto& ResDesc = ResAttribution.pSignature->GetResourceDesc(ResAttribution.ResourceIndex);
                        ValidatePipelineResourceCompatibility(ResDesc, ResType, Flags, SPIRVAttribs.ArraySize,
                                                              pShader->GetDesc().Name, SignDesc.Name);

                        const auto& ResAttribs{ResAttribution.pSignature->GetResourceAttribs(ResAttribution.ResourceIndex)};
                        ResourceBinding = ResAttribs.BindingIndex;
                        DescriptorSet   = ResAttribs.DescrSet;
                    }
                    else if (ResAttribution.ImmutableSamplerIndex != ResourceAttribution::InvalidResourceIndex)
                    {
                        if (ResType != SHADER_RESOURCE_TYPE_SAMPLER)
                        {
                            LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource with name '", SPIRVAttribs.Name,
                                                "' and type '", GetShaderResourceTypeLiteralName(ResType),
                                                "' that is not compatible with immutable sampler defined in pipeline resource signature '",
                                                SignDesc.Name, "'.");
                        }
                        const auto& SamAttribs{ResAttribution.pSignature->GetImmutableSamplerAttribs(ResAttribution.ImmutableSamplerIndex)};
                        ResourceBinding = SamAttribs.BindingIndex;
                        DescriptorSet   = SamAttribs.DescrSet;
                    }
                    else
                    {
                        UNEXPECTED("Either immutable sampler or resource index should be valid");
                    }

                    VERIFY_EXPR(ResourceBinding != ~0u && DescriptorSet != ~0u);
                    DescriptorSet += BindIndexToDescSetIndex[SignDesc.BindingIndex];
                    if (bVerifyOnly)
                    {
                        const auto SpvBinding  = SPIRV[SPIRVAttribs.BindingDecorationOffset];
                        const auto SpvDescrSet = SPIRV[SPIRVAttribs.DescriptorSetDecorationOffset];
                        if (SpvBinding != ResourceBinding)
                        {
                            LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' maps resource '", SPIRVAttribs.Name,
                                                "' to binding ", SpvBinding, ", but the same resource in pipeline resource signature '",
                                                SignDesc.Name, "' is mapped to binding ", ResourceBinding, '.');
                        }
                        if (SpvDescrSet != DescriptorSet)
                        {
                            LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' maps resource '", SPIRVAttribs.Name,
                                                "' to descriptor set ", SpvDescrSet, ", but the same resource in pipeline resource signature '",
                                                SignDesc.Name, "' is mapped to set ", DescriptorSet, '.');
                        }
                    }
                    else
                    {
                        SPIRV[SPIRVAttribs.BindingDecorationOffset]       = ResourceBinding;
                        SPIRV[SPIRVAttribs.DescriptorSetDecorationOffset] = DescriptorSet;
                    }

                    if (pDvpResourceAttibutions)
                        pDvpResourceAttibutions->emplace_back(ResAttribution);
                });

            if (bStripReflection)
            {
#if !DILIGENT_NO_HLSL
                // We have to strip reflection instructions to fix the following validation error:
                //     SPIR-V module not valid: DecorateStringGOOGLE requires one of the following extensions: SPV_GOOGLE_decorate_string
                // Optimizer also performs validation and may catch problems with the byte code.
                // NB: SPIRV offsets become INVALID after this operation.
                auto StrippedSPIRV = OptimizeSPIRV(SPIRV, SPV_ENV_MAX, SPIRV_OPTIMIZATION_FLAG_STRIP_REFLECTION);
                if (!StrippedSPIRV.empty())
                    SPIRV = std::move(StrippedSPIRV);
                else
                    LOG_ERROR("Failed to strip reflection information from shader '", pShader->GetDesc().Name, "'. This may indicate a problem with the byte code.");
#endif
            }
        }
    }
}

void PipelineStateVkImpl::InitPipelineLayout(const PipelineStateCreateInfo& CreateInfo, TShaderStages& ShaderStages) noexcept(false)
{
    const auto InternalFlags = GetInternalCreateFlags(CreateInfo);
    if (m_UsingImplicitSignature && (InternalFlags & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) == 0)
    {
        const auto SignDesc = GetDefaultResourceSignatureDesc(ShaderStages, m_Desc.Name, m_Desc.ResourceLayout, m_Desc.SRBAllocationGranularity);
        InitDefaultSignature(SignDesc, GetActiveShaderStages(), false /*bIsDeviceInternal*/);
        VERIFY_EXPR(m_Signatures[0]);
    }

#ifdef DILIGENT_DEVELOPMENT
    DvpValidateResourceLimits();
#endif

    m_PipelineLayout.Create(GetDevice(), m_Signatures, m_SignatureCount);

    const auto RemapResources = (CreateInfo.Flags & PSO_CREATE_FLAG_DONT_REMAP_SHADER_RESOURCES) == 0;
    const auto VerifyBindings = !RemapResources && ((InternalFlags & PSO_CREATE_INTERNAL_FLAG_NO_SHADER_REFLECTION) == 0);
    if (RemapResources || VerifyBindings)
    {
        VERIFY_EXPR(RemapResources ^ VerifyBindings);
        TBindIndexToDescSetIndex BindIndexToDescSetIndex = {};
        for (Uint32 i = 0; i < m_SignatureCount; ++i)
            BindIndexToDescSetIndex[i] = m_PipelineLayout.GetFirstDescrSetIndex(i);

        // Note that we always need to strip reflection information when it is present
        RemapOrVerifyShaderResources(ShaderStages,
                                     m_Signatures,
                                     m_SignatureCount,
                                     BindIndexToDescSetIndex,
                                     VerifyBindings, // VerifyOnly
                                     true,           // bStripReflection
                                     m_Desc.Name,
#ifdef DILIGENT_DEVELOPMENT
                                     &m_ShaderResources, &m_ResourceAttibutions
#else
                                     nullptr, nullptr
#endif
        );
    }
}

template <typename PSOCreateInfoType>
PipelineStateVkImpl::TShaderStages PipelineStateVkImpl::InitInternalObjects(
    const PSOCreateInfoType&                           CreateInfo,
    std::vector<VkPipelineShaderStageCreateInfo>&      vkShaderStages,
    std::vector<VulkanUtilities::ShaderModuleWrapper>& ShaderModules) noexcept(false)
{
    TShaderStages ShaderStages;
    ExtractShaders<ShaderVkImpl>(CreateInfo, ShaderStages);

    FixedLinearAllocator MemPool{GetRawAllocator()};

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    const auto& LogicalDevice = GetDevice()->GetLogicalDevice();

    InitializePipelineDesc(CreateInfo, MemPool);

    InitPipelineLayout(CreateInfo, ShaderStages);

    // Create shader modules and initialize shader stages
    InitPipelineShaderStages(LogicalDevice, ShaderStages, ShaderModules, vkShaderStages);

    return ShaderStages;
}

PipelineStateVkImpl::PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const GraphicsPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceVk, CreateInfo}
{
    try
    {
        std::vector<VkPipelineShaderStageCreateInfo>      vkShaderStages;
        std::vector<VulkanUtilities::ShaderModuleWrapper> ShaderModules;

        InitInternalObjects(CreateInfo, vkShaderStages, ShaderModules);

        const auto vkSPOCache = CreateInfo.pPSOCache != nullptr ? ClassPtrCast<PipelineStateCacheVkImpl>(CreateInfo.pPSOCache)->GetVkPipelineCache() : VK_NULL_HANDLE;
        CreateGraphicsPipeline(pDeviceVk, vkShaderStages, m_PipelineLayout, m_Desc, GetGraphicsPipelineDesc(), m_Pipeline, GetRenderPassPtr(), vkSPOCache);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateVkImpl::PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const ComputePipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceVk, CreateInfo}
{
    try
    {
        std::vector<VkPipelineShaderStageCreateInfo>      vkShaderStages;
        std::vector<VulkanUtilities::ShaderModuleWrapper> ShaderModules;

        InitInternalObjects(CreateInfo, vkShaderStages, ShaderModules);

        const auto vkSPOCache = CreateInfo.pPSOCache != nullptr ? ClassPtrCast<PipelineStateCacheVkImpl>(CreateInfo.pPSOCache)->GetVkPipelineCache() : VK_NULL_HANDLE;
        CreateComputePipeline(pDeviceVk, vkShaderStages, m_PipelineLayout, m_Desc, m_Pipeline, vkSPOCache);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateVkImpl::PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const RayTracingPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceVk, CreateInfo}
{
    try
    {
        const auto& LogicalDevice = pDeviceVk->GetLogicalDevice();

        std::vector<VkPipelineShaderStageCreateInfo>      vkShaderStages;
        std::vector<VulkanUtilities::ShaderModuleWrapper> ShaderModules;

        const auto ShaderStages   = InitInternalObjects(CreateInfo, vkShaderStages, ShaderModules);
        const auto vkShaderGroups = BuildRTShaderGroupDescription(CreateInfo, m_pRayTracingPipelineData->NameToGroupIndex, ShaderStages);
        const auto vkSPOCache     = CreateInfo.pPSOCache != nullptr ? ClassPtrCast<PipelineStateCacheVkImpl>(CreateInfo.pPSOCache)->GetVkPipelineCache() : VK_NULL_HANDLE;

        CreateRayTracingPipeline(pDeviceVk, vkShaderStages, vkShaderGroups, m_PipelineLayout, m_Desc, GetRayTracingPipelineDesc(), m_Pipeline, vkSPOCache);

        VERIFY(m_pRayTracingPipelineData->NameToGroupIndex.size() == vkShaderGroups.size(),
               "The size of NameToGroupIndex map does not match the actual number of groups in the pipeline. This is a bug.");
        // Get shader group handles from the PSO.
        auto err = LogicalDevice.GetRayTracingShaderGroupHandles(m_Pipeline, 0, static_cast<uint32_t>(vkShaderGroups.size()), m_pRayTracingPipelineData->ShaderDataSize, m_pRayTracingPipelineData->ShaderHandles);
        DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to get shader group handles");
        (void)err;
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateVkImpl::~PipelineStateVkImpl()
{
    Destruct();
}

void PipelineStateVkImpl::Destruct()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_Pipeline), m_Desc.ImmediateContextMask);
    m_PipelineLayout.Release(m_pDevice, m_Desc.ImmediateContextMask);

    TPipelineStateBase::Destruct();
}

#ifdef DILIGENT_DEVELOPMENT
void PipelineStateVkImpl::DvpVerifySRBResources(const DeviceContextVkImpl* pCtx, const ShaderResourceCacheArrayType& ResourceCaches) const
{
    auto res_info = m_ResourceAttibutions.begin();
    for (const auto& pResources : m_ShaderResources)
    {
        pResources->ProcessResources(
            [&](const SPIRVShaderResourceAttribs& ResAttribs, Uint32) //
            {
                if (!res_info->IsImmutableSampler()) // There are also immutable samplers in the list
                {
                    VERIFY_EXPR(res_info->pSignature != nullptr);
                    VERIFY_EXPR(res_info->pSignature->GetDesc().BindingIndex == res_info->SignatureIndex);
                    const auto* pResourceCache = ResourceCaches[res_info->SignatureIndex];
                    DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at index ", res_info->SignatureIndex, " is null.");
                    res_info->pSignature->DvpValidateCommittedResource(pCtx, ResAttribs, res_info->ResourceIndex, *pResourceCache,
                                                                       pResources->GetShaderName(), m_Desc.Name);
                }
                ++res_info;
            } //
        );
    }
    VERIFY_EXPR(res_info == m_ResourceAttibutions.end());
}

void PipelineStateVkImpl::DvpValidateResourceLimits() const
{
    const auto& Limits       = GetDevice()->GetPhysicalDevice().GetProperties().limits;
    const auto& ASLimits     = GetDevice()->GetPhysicalDevice().GetExtProperties().AccelStruct;
    const auto& DescIndFeats = GetDevice()->GetPhysicalDevice().GetExtFeatures().DescriptorIndexing;
    const auto& DescIndProps = GetDevice()->GetPhysicalDevice().GetExtProperties().DescriptorIndexing;
    const auto  DescCount    = static_cast<Uint32>(DescriptorType::Count);

    std::array<Uint32, DescCount>                                      DescriptorCount         = {};
    std::array<std::array<Uint32, DescCount>, MAX_SHADERS_IN_PIPELINE> PerStageDescriptorCount = {};
    std::array<bool, MAX_SHADERS_IN_PIPELINE>                          ShaderStagePresented    = {};

    for (Uint32 s = 0; s < GetResourceSignatureCount(); ++s)
    {
        const auto* pSignature = GetResourceSignature(s);
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc   = pSignature->GetResourceDesc(r);
            const auto& ResAttr   = pSignature->GetResourceAttribs(r);
            const auto  DescIndex = static_cast<Uint32>(ResAttr.DescrType);

            DescriptorCount[DescIndex] += ResAttr.ArraySize;

            for (auto ShaderStages = ResDesc.ShaderStages; ShaderStages != 0;)
            {
                const auto ShaderInd = GetShaderTypePipelineIndex(ExtractLSB(ShaderStages), m_Desc.PipelineType);
                PerStageDescriptorCount[ShaderInd][DescIndex] += ResAttr.ArraySize;
                ShaderStagePresented[ShaderInd] = true;
            }

            if ((ResDesc.Flags & PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY) != 0)
            {
                bool NonUniformIndexingSupported = false;
                bool NonUniformIndexingIsNative  = false;
                switch (ResAttr.GetDescriptorType())
                {
                    case DescriptorType::Sampler:
                        NonUniformIndexingSupported = true;
                        NonUniformIndexingIsNative  = true;
                        break;
                    case DescriptorType::CombinedImageSampler:
                    case DescriptorType::SeparateImage:
                        NonUniformIndexingSupported = DescIndFeats.shaderSampledImageArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderSampledImageArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::StorageImage:
                        NonUniformIndexingSupported = DescIndFeats.shaderStorageImageArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderStorageImageArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::UniformTexelBuffer:
                        NonUniformIndexingSupported = DescIndFeats.shaderUniformTexelBufferArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderSampledImageArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::StorageTexelBuffer:
                    case DescriptorType::StorageTexelBuffer_ReadOnly:
                        NonUniformIndexingSupported = DescIndFeats.shaderStorageTexelBufferArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderStorageBufferArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::UniformBuffer:
                    case DescriptorType::UniformBufferDynamic:
                        NonUniformIndexingSupported = DescIndFeats.shaderUniformBufferArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderUniformBufferArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::StorageBuffer:
                    case DescriptorType::StorageBuffer_ReadOnly:
                    case DescriptorType::StorageBufferDynamic:
                    case DescriptorType::StorageBufferDynamic_ReadOnly:
                        NonUniformIndexingSupported = DescIndFeats.shaderStorageBufferArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderStorageBufferArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::InputAttachment:
                        NonUniformIndexingSupported = DescIndFeats.shaderInputAttachmentArrayNonUniformIndexing;
                        NonUniformIndexingIsNative  = DescIndProps.shaderInputAttachmentArrayNonUniformIndexingNative;
                        break;
                    case DescriptorType::AccelerationStructure:
                        // There is no separate feature for acceleration structures, GLSL spec says:
                        // "If GL_EXT_nonuniform_qualifier is supported
                        // When aggregated into arrays within a shader, accelerationStructureEXT can
                        // be indexed with a non-uniform integral expressions, when decorated with the
                        // nonuniformEXT qualifier."
                        // Descriptor indexing is supported here, otherwise error will be generated in ValidatePipelineResourceSignatureDesc().
                        NonUniformIndexingSupported = true;
                        NonUniformIndexingIsNative  = true;
                        break;

                    default:
                        UNEXPECTED("Unexpected descriptor type");
                }

                // TODO: We don't know if this resource is used for non-uniform indexing or not.
                if (!NonUniformIndexingSupported)
                {
                    LOG_WARNING_MESSAGE("PSO '", m_Desc.Name, "', resource signature '", pSignature->GetDesc().Name, "' contains shader resource '",
                                        ResDesc.Name, "' that is defined with RUNTIME_ARRAY flag, but current device does not support non-uniform indexing for this resource type.");
                }
                else if (!NonUniformIndexingIsNative)
                {
                    LOG_WARNING_MESSAGE("Performance warning in PSO '", m_Desc.Name, "', resource signature '", pSignature->GetDesc().Name, "': shader resource '",
                                        ResDesc.Name, "' is defined with RUNTIME_ARRAY flag, but non-uniform indexing is emulated on this device.");
                }
            }
        }
    }

    // Check total descriptor count
    {
        const Uint32 NumSampledImages =
            DescriptorCount[static_cast<Uint32>(DescriptorType::CombinedImageSampler)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::SeparateImage)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::UniformTexelBuffer)];
        const Uint32 NumStorageImages =
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageImage)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageTexelBuffer)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageTexelBuffer_ReadOnly)];
        const Uint32 NumStorageBuffers =
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageBuffer)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageBuffer_ReadOnly)];
        const Uint32 NumDynamicStorageBuffers =
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageBufferDynamic)] +
            DescriptorCount[static_cast<Uint32>(DescriptorType::StorageBufferDynamic_ReadOnly)];
        const Uint32 NumSamplers               = DescriptorCount[static_cast<Uint32>(DescriptorType::Sampler)];
        const Uint32 NumUniformBuffers         = DescriptorCount[static_cast<Uint32>(DescriptorType::UniformBuffer)];
        const Uint32 NumDynamicUniformBuffers  = DescriptorCount[static_cast<Uint32>(DescriptorType::UniformBufferDynamic)];
        const Uint32 NumInputAttachments       = DescriptorCount[static_cast<Uint32>(DescriptorType::InputAttachment)];
        const Uint32 NumAccelerationStructures = DescriptorCount[static_cast<Uint32>(DescriptorType::AccelerationStructure)];

        DEV_CHECK_ERR(NumSamplers <= Limits.maxDescriptorSetSamplers,
                      "In PSO '", m_Desc.Name, "', the number of samplers (", NumSamplers, ") exceeds the limit (", Limits.maxDescriptorSetSamplers, ").");
        DEV_CHECK_ERR(NumSampledImages <= Limits.maxDescriptorSetSampledImages,
                      "In PSO '", m_Desc.Name, "', the number of sampled images (", NumSampledImages, ") exceeds the limit (", Limits.maxDescriptorSetSampledImages, ").");
        DEV_CHECK_ERR(NumStorageImages <= Limits.maxDescriptorSetStorageImages,
                      "In PSO '", m_Desc.Name, "', the number of storage images (", NumStorageImages, ") exceeds the limit (", Limits.maxDescriptorSetStorageImages, ").");
        DEV_CHECK_ERR(NumStorageBuffers <= Limits.maxDescriptorSetStorageBuffers,
                      "In PSO '", m_Desc.Name, "', the number of storage buffers (", NumStorageBuffers, ") exceeds the limit (", Limits.maxDescriptorSetStorageBuffers, ").");
        DEV_CHECK_ERR(NumDynamicStorageBuffers <= Limits.maxDescriptorSetStorageBuffersDynamic,
                      "In PSO '", m_Desc.Name, "', the number of dynamic storage buffers (", NumDynamicStorageBuffers, ") exceeds the limit (", Limits.maxDescriptorSetStorageBuffersDynamic, ").");
        DEV_CHECK_ERR(NumUniformBuffers <= Limits.maxDescriptorSetUniformBuffers,
                      "In PSO '", m_Desc.Name, "', the number of uniform buffers (", NumUniformBuffers, ") exceeds the limit (", Limits.maxDescriptorSetUniformBuffers, ").");
        DEV_CHECK_ERR(NumDynamicUniformBuffers <= Limits.maxDescriptorSetUniformBuffersDynamic,
                      "In PSO '", m_Desc.Name, "', the number of dynamic uniform buffers (", NumDynamicUniformBuffers, ") exceeds the limit (", Limits.maxDescriptorSetUniformBuffersDynamic, ").");
        DEV_CHECK_ERR(NumInputAttachments <= Limits.maxDescriptorSetInputAttachments,
                      "In PSO '", m_Desc.Name, "', the number of input attachments (", NumInputAttachments, ") exceeds the limit (", Limits.maxDescriptorSetInputAttachments, ").");
        DEV_CHECK_ERR(NumAccelerationStructures <= ASLimits.maxDescriptorSetAccelerationStructures,
                      "In PSO '", m_Desc.Name, "', the number of acceleration structures (", NumAccelerationStructures, ") exceeds the limit (", ASLimits.maxDescriptorSetAccelerationStructures, ").");
    }

    // Check per stage descriptor count
    for (Uint32 ShaderInd = 0; ShaderInd < PerStageDescriptorCount.size(); ++ShaderInd)
    {
        if (!ShaderStagePresented[ShaderInd])
            continue;

        const auto& NumDesc    = PerStageDescriptorCount[ShaderInd];
        const auto  ShaderType = GetShaderTypeFromPipelineIndex(ShaderInd, m_Desc.PipelineType);
        const char* StageName  = GetShaderTypeLiteralName(ShaderType);

        const Uint32 NumSampledImages =
            NumDesc[static_cast<Uint32>(DescriptorType::CombinedImageSampler)] +
            NumDesc[static_cast<Uint32>(DescriptorType::SeparateImage)] +
            NumDesc[static_cast<Uint32>(DescriptorType::UniformTexelBuffer)];
        const Uint32 NumStorageImages =
            NumDesc[static_cast<Uint32>(DescriptorType::StorageImage)] +
            NumDesc[static_cast<Uint32>(DescriptorType::StorageTexelBuffer)] +
            NumDesc[static_cast<Uint32>(DescriptorType::StorageTexelBuffer_ReadOnly)];
        const Uint32 NumStorageBuffers =
            NumDesc[static_cast<Uint32>(DescriptorType::StorageBuffer)] +
            NumDesc[static_cast<Uint32>(DescriptorType::StorageBuffer_ReadOnly)] +
            NumDesc[static_cast<Uint32>(DescriptorType::StorageBufferDynamic)] +
            NumDesc[static_cast<Uint32>(DescriptorType::StorageBufferDynamic_ReadOnly)];
        const Uint32 NumUniformBuffers =
            NumDesc[static_cast<Uint32>(DescriptorType::UniformBuffer)] +
            NumDesc[static_cast<Uint32>(DescriptorType::UniformBufferDynamic)];
        const Uint32 NumSamplers               = NumDesc[static_cast<Uint32>(DescriptorType::Sampler)];
        const Uint32 NumInputAttachments       = NumDesc[static_cast<Uint32>(DescriptorType::InputAttachment)];
        const Uint32 NumAccelerationStructures = NumDesc[static_cast<Uint32>(DescriptorType::AccelerationStructure)];
        const Uint32 NumResources              = NumSampledImages + NumStorageImages + NumStorageBuffers + NumUniformBuffers + NumSamplers + NumInputAttachments + NumAccelerationStructures;

        DEV_CHECK_ERR(NumResources <= Limits.maxPerStageResources,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the total number of resources (", NumResources, ") exceeds the per-stage limit (", Limits.maxPerStageResources, ").");
        DEV_CHECK_ERR(NumSamplers <= Limits.maxPerStageDescriptorSamplers,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of samplers (", NumSamplers, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorSamplers, ").");
        DEV_CHECK_ERR(NumSampledImages <= Limits.maxPerStageDescriptorSampledImages,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of sampled images (", NumSampledImages, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorSampledImages, ").");
        DEV_CHECK_ERR(NumStorageImages <= Limits.maxPerStageDescriptorStorageImages,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of storage images (", NumStorageImages, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorStorageImages, ").");
        DEV_CHECK_ERR(NumStorageBuffers <= Limits.maxPerStageDescriptorStorageBuffers,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of storage buffers (", NumStorageBuffers, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorStorageBuffers, ").");
        DEV_CHECK_ERR(NumUniformBuffers <= Limits.maxPerStageDescriptorUniformBuffers,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of uniform buffers (", NumUniformBuffers, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorUniformBuffers, ").");
        DEV_CHECK_ERR(NumInputAttachments <= Limits.maxPerStageDescriptorInputAttachments,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of input attachments (", NumInputAttachments, ") exceeds the per-stage limit (", Limits.maxPerStageDescriptorInputAttachments, ").");
        DEV_CHECK_ERR(NumAccelerationStructures <= ASLimits.maxPerStageDescriptorAccelerationStructures,
                      "In PSO '", m_Desc.Name, "' shader stage '", StageName, "', the number of acceleration structures (", NumAccelerationStructures, ") exceeds the per-stage limit (", ASLimits.maxPerStageDescriptorAccelerationStructures, ").");
    }
}
#endif // DILIGENT_DEVELOPMENT

} // namespace Diligent
