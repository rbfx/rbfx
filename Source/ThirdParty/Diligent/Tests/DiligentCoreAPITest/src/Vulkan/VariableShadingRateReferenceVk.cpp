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

#include "Vulkan/TestingEnvironmentVk.hpp"
#include "Vulkan/TestingSwapChainVk.hpp"

#include "volk.h"

#include "InlineShaders/VariableShadingRateTestGLSL.h"
#include "VariableShadingRateTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

void VariableShadingRatePerDrawTestReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto  vkDevice            = pEnv->GetVkDevice();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc  = pSwapChain->GetDesc();
    const auto& SRProps = pEnv->GetDevice()->GetAdapterInfo().ShadingRate;
    ASSERT_TRUE(SRProps.Format == SHADING_RATE_FORMAT_PALETTE);

    VkResult res = VK_SUCCESS;
    (void)res;

    auto vkVSModule = pEnv->CreateShaderModule(SHADER_TYPE_VERTEX, GLSL::PerDrawShadingRate_VS);
    ASSERT_TRUE(vkVSModule != VK_NULL_HANDLE);
    auto vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::PerDrawShadingRate_PS);
    ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

    ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module = vkVSModule;
    ShaderStages[0].pName  = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = vkFSModule;
    ShaderStages[1].pName  = "main";

    PipelineCI.pStages    = ShaderStages;
    PipelineCI.stageCount = _countof(ShaderStages);

    VkPipelineLayoutCreateInfo PipelineLayoutCI{};
    PipelineLayoutCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
    ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
    PipelineCI.layout = vkLayout;

    VkPipelineVertexInputStateCreateInfo VertexInputStateCI{};
    VertexInputStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    PipelineCI.pVertexInputState = &VertexInputStateCI;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI{};
    InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;

    VkViewport Viewport{};
    Viewport.y        = static_cast<float>(SCDesc.Height);
    Viewport.width    = static_cast<float>(SCDesc.Width);
    Viewport.height   = -static_cast<float>(SCDesc.Height);
    Viewport.maxDepth = 1;

    VkRect2D ScissorRect{};
    ScissorRect.extent.width  = SCDesc.Width;
    ScissorRect.extent.height = SCDesc.Height;

    VkPipelineViewportStateCreateInfo ViewPortStateCI{};
    ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewPortStateCI.viewportCount = 1;
    ViewPortStateCI.pViewports    = &Viewport;
    ViewPortStateCI.scissorCount  = ViewPortStateCI.viewportCount;
    ViewPortStateCI.pScissors     = &ScissorRect;
    PipelineCI.pViewportState     = &ViewPortStateCI;

    VkPipelineRasterizationStateCreateInfo RasterizerStateCI{};
    RasterizerStateCI.sType        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerStateCI.polygonMode  = VK_POLYGON_MODE_FILL;
    RasterizerStateCI.cullMode     = VK_CULL_MODE_BACK_BIT;
    RasterizerStateCI.frontFace    = VK_FRONT_FACE_CLOCKWISE;
    RasterizerStateCI.lineWidth    = 1;
    PipelineCI.pRasterizationState = &RasterizerStateCI;

    VkPipelineMultisampleStateCreateInfo MSStateCI{};
    MSStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MSStateCI.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MSStateCI.sampleShadingEnable   = VK_FALSE;
    MSStateCI.minSampleShading      = 0;
    uint32_t SampleMask[]           = {0xFFFFFFFF, 0};
    MSStateCI.pSampleMask           = SampleMask;
    MSStateCI.alphaToCoverageEnable = VK_FALSE;
    MSStateCI.alphaToOneEnable      = VK_FALSE;
    PipelineCI.pMultisampleState    = &MSStateCI;

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI{};
    DepthStencilStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilStateCI.depthTestEnable   = VK_FALSE;
    DepthStencilStateCI.stencilTestEnable = VK_FALSE;
    DepthStencilStateCI.minDepthBounds    = 0.F;
    DepthStencilStateCI.maxDepthBounds    = 1.F;
    PipelineCI.pDepthStencilState         = &DepthStencilStateCI;

    VkPipelineColorBlendAttachmentState Attachment{};
    Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo BlendStateCI{};
    BlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    BlendStateCI.pAttachments    = &Attachment;
    BlendStateCI.attachmentCount = 1;
    PipelineCI.pColorBlendState  = &BlendStateCI;

    const VkDynamicState             DynamicStates[] = {VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR};
    VkPipelineDynamicStateCreateInfo DynamicStateCI{};
    DynamicStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCI.pDynamicStates    = DynamicStates;
    DynamicStateCI.dynamicStateCount = _countof(DynamicStates);
    PipelineCI.pDynamicState         = &DynamicStateCI;

    PipelineCI.renderPass         = pTestingSwapChainVk->GetRenderPass();
    PipelineCI.subpass            = 0;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCI.basePipelineIndex  = -1;

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    res                   = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
    ASSERT_EQ(res, VK_SUCCESS);
    ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

    const VkExtent2D                         FragmentSize{2, 2};
    const VkFragmentShadingRateCombinerOpKHR CombinerOps[] = {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR};
    vkCmdSetFragmentShadingRateKHR(vkCmdBuffer, &FragmentSize, CombinerOps);

    vkCmdDraw(vkCmdBuffer, 3, 1, 0, 0);
    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res == VK_SUCCESS, "Failed to end command buffer");

    pEnv->SubmitCommandBuffer(vkCmdBuffer);

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkVSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
}


void VariableShadingRatePerPrimitiveTestReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto  vkDevice            = pEnv->GetVkDevice();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc  = pSwapChain->GetDesc();
    const auto& SRProps = pEnv->GetDevice()->GetAdapterInfo().ShadingRate;
    ASSERT_TRUE(SRProps.Format == SHADING_RATE_FORMAT_PALETTE);

    const auto& Verts = VRSTestingConstants::PerPrimitive::Vertices;

    VkResult res = VK_SUCCESS;
    (void)res;

    auto vkVSModule = pEnv->CreateShaderModule(SHADER_TYPE_VERTEX, GLSL::PerPrimitiveShadingRate_VS);
    ASSERT_TRUE(vkVSModule != VK_NULL_HANDLE);
    auto vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::PerPrimitiveShadingRate_PS);
    ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

    ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module = vkVSModule;
    ShaderStages[0].pName  = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = vkFSModule;
    ShaderStages[1].pName  = "main";

    PipelineCI.pStages    = ShaderStages;
    PipelineCI.stageCount = _countof(ShaderStages);

    VkPipelineLayoutCreateInfo PipelineLayoutCI{};
    PipelineLayoutCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
    ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
    PipelineCI.layout = vkLayout;

    const VkVertexInputAttributeDescription VertexInputAttribs[] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PosAndRate, Pos)},
        {1, 0, VK_FORMAT_R32_SINT, offsetof(PosAndRate, Rate)} //
    };
    const VkVertexInputBindingDescription VertexInputBindingDesc = {0, sizeof(Verts[0]), VK_VERTEX_INPUT_RATE_VERTEX};

    VkPipelineVertexInputStateCreateInfo VertexInputStateCI{};
    VertexInputStateCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputStateCI.vertexBindingDescriptionCount   = 1;
    VertexInputStateCI.pVertexBindingDescriptions      = &VertexInputBindingDesc;
    VertexInputStateCI.vertexAttributeDescriptionCount = _countof(VertexInputAttribs);
    VertexInputStateCI.pVertexAttributeDescriptions    = VertexInputAttribs;
    PipelineCI.pVertexInputState                       = &VertexInputStateCI;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI{};
    InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;

    VkViewport Viewport{};
    Viewport.y        = static_cast<float>(SCDesc.Height);
    Viewport.width    = static_cast<float>(SCDesc.Width);
    Viewport.height   = -static_cast<float>(SCDesc.Height);
    Viewport.maxDepth = 1;

    VkRect2D ScissorRect{};
    ScissorRect.extent.width  = SCDesc.Width;
    ScissorRect.extent.height = SCDesc.Height;

    VkPipelineViewportStateCreateInfo ViewPortStateCI{};
    ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewPortStateCI.viewportCount = 1;
    ViewPortStateCI.pViewports    = &Viewport;
    ViewPortStateCI.scissorCount  = ViewPortStateCI.viewportCount;
    ViewPortStateCI.pScissors     = &ScissorRect;
    PipelineCI.pViewportState     = &ViewPortStateCI;

    VkPipelineRasterizationStateCreateInfo RasterizerStateCI{};
    RasterizerStateCI.sType        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerStateCI.polygonMode  = VK_POLYGON_MODE_FILL;
    RasterizerStateCI.cullMode     = VK_CULL_MODE_BACK_BIT;
    RasterizerStateCI.frontFace    = VK_FRONT_FACE_CLOCKWISE;
    RasterizerStateCI.lineWidth    = 1;
    PipelineCI.pRasterizationState = &RasterizerStateCI;

    VkPipelineMultisampleStateCreateInfo MSStateCI{};
    MSStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MSStateCI.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MSStateCI.sampleShadingEnable   = VK_FALSE;
    MSStateCI.minSampleShading      = 0;
    uint32_t SampleMask[]           = {0xFFFFFFFF, 0};
    MSStateCI.pSampleMask           = SampleMask;
    MSStateCI.alphaToCoverageEnable = VK_FALSE;
    MSStateCI.alphaToOneEnable      = VK_FALSE;
    PipelineCI.pMultisampleState    = &MSStateCI;

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI{};
    DepthStencilStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    PipelineCI.pDepthStencilState = &DepthStencilStateCI;

    VkPipelineColorBlendAttachmentState Attachment{};
    Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo BlendStateCI{};
    BlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    BlendStateCI.pAttachments    = &Attachment;
    BlendStateCI.attachmentCount = 1;
    PipelineCI.pColorBlendState  = &BlendStateCI;

    const VkDynamicState             DynamicStates[] = {VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR};
    VkPipelineDynamicStateCreateInfo DynamicStateCI{};
    DynamicStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCI.pDynamicStates    = DynamicStates;
    DynamicStateCI.dynamicStateCount = _countof(DynamicStates);
    PipelineCI.pDynamicState         = &DynamicStateCI;

    PipelineCI.renderPass         = pTestingSwapChainVk->GetRenderPass();
    PipelineCI.subpass            = 0;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCI.basePipelineIndex  = -1;

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    res                   = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
    ASSERT_EQ(res, VK_SUCCESS);
    ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);

    RefCntAutoPtr<IBuffer> pVB;
    {
        BufferData BuffData{Verts, sizeof(Verts)};
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Vertex buffer";
        BuffDesc.Size      = BuffData.DataSize;
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage     = USAGE_IMMUTABLE;

        pEnv->GetDevice()->CreateBuffer(BuffDesc, &BuffData, &pVB);
        ASSERT_NE(pVB, nullptr);
    }
    const VkBuffer     vkVB     = BitCast<VkBuffer>(pVB->GetNativeHandle());
    const VkDeviceSize VBOffset = 0;

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

    const VkExtent2D                         FragmentSize{1, 1};
    const VkFragmentShadingRateCombinerOpKHR CombinerOps[] = {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR};
    vkCmdSetFragmentShadingRateKHR(vkCmdBuffer, &FragmentSize, CombinerOps);

    vkCmdBindVertexBuffers(vkCmdBuffer, 0, 1, &vkVB, &VBOffset);
    vkCmdDraw(vkCmdBuffer, _countof(Verts), 1, 0, 0);

    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res == VK_SUCCESS, "Failed to end command buffer");

    pEnv->SubmitCommandBuffer(vkCmdBuffer);

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkVSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
}


RefCntAutoPtr<ITextureView> CreateShadingRateTexture(IRenderDevice* pDevice, ISwapChain* pSwapChain, Uint32 SampleCount = 1, Uint32 ArraySize = 1);

void VariableShadingRateTextureBasedTestReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv                = TestingEnvironmentVk::GetInstance();
    auto  vkDevice            = pEnv->GetVkDevice();
    auto* pTestingSwapChainVk = ClassPtrCast<TestingSwapChainVk>(pSwapChain);

    const auto& SCDesc  = pSwapChain->GetDesc();
    const auto& SRProps = pEnv->GetDevice()->GetAdapterInfo().ShadingRate;
    ASSERT_TRUE(SRProps.Format == SHADING_RATE_FORMAT_PALETTE);

    VkResult res = VK_SUCCESS;
    (void)res;

    // Create render pass
    VkRenderPass vkRenderPass = VK_NULL_HANDLE;
    {
        VkRenderPassCreateInfo2 RenderPassCI{};
        RenderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        RenderPassCI.pNext = nullptr;
        RenderPassCI.flags = 0;

        VkAttachmentDescription2 vkAttachments[2] = {};

        ASSERT_TRUE(SCDesc.ColorBufferFormat == TEX_FORMAT_RGBA8_UNORM);
        vkAttachments[0].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        vkAttachments[0].format         = VK_FORMAT_R8G8B8A8_UNORM;
        vkAttachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        vkAttachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vkAttachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        vkAttachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vkAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vkAttachments[0].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vkAttachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        vkAttachments[1].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        vkAttachments[1].format         = VK_FORMAT_R8_UINT;
        vkAttachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        vkAttachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
        vkAttachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vkAttachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vkAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vkAttachments[1].initialLayout  = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        vkAttachments[1].finalLayout    = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

        RenderPassCI.attachmentCount = _countof(vkAttachments);
        RenderPassCI.pAttachments    = vkAttachments;

        VkSubpassDescription2 vkSubpass{};

        vkSubpass.sType                = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
        vkSubpass.flags                = 0;
        vkSubpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vkSubpass.inputAttachmentCount = 0;
        vkSubpass.pInputAttachments    = nullptr;

        VkAttachmentReference2 ColorAttachmentRef{};
        ColorAttachmentRef.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachmentRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkSubpass.pColorAttachments       = &ColorAttachmentRef;
        vkSubpass.colorAttachmentCount    = 1;
        vkSubpass.pResolveAttachments     = nullptr;
        vkSubpass.pDepthStencilAttachment = nullptr;
        vkSubpass.preserveAttachmentCount = 0;
        vkSubpass.pPreserveAttachments    = nullptr;

        VkAttachmentReference2 ShadingRateAttachmentRef{};
        ShadingRateAttachmentRef.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        ShadingRateAttachmentRef.attachment = 1;
        ShadingRateAttachmentRef.layout     = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        ShadingRateAttachmentRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkFragmentShadingRateAttachmentInfoKHR vkShadingRate{};
        vkShadingRate.sType                          = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
        vkShadingRate.pNext                          = nullptr;
        vkShadingRate.shadingRateAttachmentTexelSize = {SRProps.MaxTileSize[0], SRProps.MaxTileSize[1]};
        vkShadingRate.pFragmentShadingRateAttachment = &ShadingRateAttachmentRef;

        vkSubpass.pNext = &vkShadingRate;

        RenderPassCI.subpassCount = 1;
        RenderPassCI.pSubpasses   = &vkSubpass;

        RenderPassCI.correlatedViewMaskCount = 0;
        RenderPassCI.pCorrelatedViewMasks    = nullptr;

        res = vkCreateRenderPass2KHR(vkDevice, &RenderPassCI, nullptr, &vkRenderPass);
        ASSERT_EQ(res, VK_SUCCESS);
        ASSERT_TRUE(vkRenderPass != VK_NULL_HANDLE);
    }

    // Create shading rate texture
    RefCntAutoPtr<ITexture> pSRTex;
    VkImageView             vkShadingRateView = VK_NULL_HANDLE;
    {
        auto pVRSView = CreateShadingRateTexture(pEnv->GetDevice(), pSwapChain);
        ASSERT_NE(pVRSView, nullptr);
        pSRTex = pVRSView->GetTexture();

        auto vkShadingRateImage = BitCast<VkImage>(pSRTex->GetNativeHandle());

        VkImageViewCreateInfo ViewCI{};
        ViewCI.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ViewCI.image      = vkShadingRateImage;
        ViewCI.viewType   = VK_IMAGE_VIEW_TYPE_2D;
        ViewCI.format     = VK_FORMAT_R8_UINT;
        ViewCI.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};

        ViewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewCI.subresourceRange.baseMipLevel   = 0;
        ViewCI.subresourceRange.levelCount     = 1;
        ViewCI.subresourceRange.baseArrayLayer = 0;
        ViewCI.subresourceRange.layerCount     = 1;

        res = vkCreateImageView(vkDevice, &ViewCI, nullptr, &vkShadingRateView);
        ASSERT_EQ(res, VK_SUCCESS);
        ASSERT_TRUE(vkShadingRateView != VK_NULL_HANDLE);
    }

    // Create framebuffer
    VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
    {
        VkImageView vkAttachments[] = {
            pTestingSwapChainVk->GetVkRenderTargetImageView(),
            vkShadingRateView //
        };

        VkFramebufferCreateInfo FramebufferCI{};
        FramebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferCI.renderPass      = vkRenderPass;
        FramebufferCI.attachmentCount = _countof(vkAttachments);
        FramebufferCI.pAttachments    = vkAttachments;
        FramebufferCI.width           = SCDesc.Width;
        FramebufferCI.height          = SCDesc.Height;
        FramebufferCI.layers          = 1;

        res = vkCreateFramebuffer(vkDevice, &FramebufferCI, nullptr, &vkFramebuffer);
        ASSERT_EQ(res, VK_SUCCESS);
        ASSERT_TRUE(vkFramebuffer != VK_NULL_HANDLE);
    }

    // Create pipeline
    VkShaderModule   vkVSModule = VK_NULL_HANDLE;
    VkShaderModule   vkFSModule = VK_NULL_HANDLE;
    VkPipelineLayout vkLayout   = VK_NULL_HANDLE;
    VkPipeline       vkPipeline = VK_NULL_HANDLE;
    {
        vkVSModule = pEnv->CreateShaderModule(SHADER_TYPE_VERTEX, GLSL::TextureBasedShadingRate_VS);
        ASSERT_TRUE(vkVSModule != VK_NULL_HANDLE);
        vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::TextureBasedShadingRate_PS);
        ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

        VkGraphicsPipelineCreateInfo PipelineCI{};
        PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

        ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[0].module = vkVSModule;
        ShaderStages[0].pName  = "main";

        ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStages[1].module = vkFSModule;
        ShaderStages[1].pName  = "main";

        PipelineCI.pStages    = ShaderStages;
        PipelineCI.stageCount = _countof(ShaderStages);

        VkPipelineLayoutCreateInfo PipelineLayoutCI{};
        PipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
        ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
        PipelineCI.layout = vkLayout;

        VkPipelineVertexInputStateCreateInfo VertexInputStateCI{};
        VertexInputStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        PipelineCI.pVertexInputState = &VertexInputStateCI;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI{};
        InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineCI.pInputAssemblyState = &InputAssemblyCI;

        VkViewport Viewport{};
        Viewport.y        = static_cast<float>(SCDesc.Height);
        Viewport.width    = static_cast<float>(SCDesc.Width);
        Viewport.height   = -static_cast<float>(SCDesc.Height);
        Viewport.maxDepth = 1;

        VkRect2D ScissorRect{};
        ScissorRect.extent.width  = SCDesc.Width;
        ScissorRect.extent.height = SCDesc.Height;

        VkPipelineViewportStateCreateInfo ViewPortStateCI{};
        ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewPortStateCI.viewportCount = 1;
        ViewPortStateCI.pViewports    = &Viewport;
        ViewPortStateCI.scissorCount  = ViewPortStateCI.viewportCount;
        ViewPortStateCI.pScissors     = &ScissorRect;
        PipelineCI.pViewportState     = &ViewPortStateCI;

        VkPipelineRasterizationStateCreateInfo RasterizerStateCI{};
        RasterizerStateCI.sType        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizerStateCI.polygonMode  = VK_POLYGON_MODE_FILL;
        RasterizerStateCI.cullMode     = VK_CULL_MODE_BACK_BIT;
        RasterizerStateCI.frontFace    = VK_FRONT_FACE_CLOCKWISE;
        RasterizerStateCI.lineWidth    = 1;
        PipelineCI.pRasterizationState = &RasterizerStateCI;

        VkPipelineMultisampleStateCreateInfo MSStateCI{};
        MSStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MSStateCI.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        MSStateCI.sampleShadingEnable   = VK_FALSE;
        MSStateCI.minSampleShading      = 0;
        uint32_t SampleMask[]           = {0xFFFFFFFF, 0};
        MSStateCI.pSampleMask           = SampleMask;
        MSStateCI.alphaToCoverageEnable = VK_FALSE;
        MSStateCI.alphaToOneEnable      = VK_FALSE;
        PipelineCI.pMultisampleState    = &MSStateCI;

        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI{};
        DepthStencilStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        PipelineCI.pDepthStencilState = &DepthStencilStateCI;

        VkPipelineColorBlendAttachmentState Attachment{};
        Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo BlendStateCI{};
        BlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        BlendStateCI.pAttachments    = &Attachment;
        BlendStateCI.attachmentCount = 1;
        PipelineCI.pColorBlendState  = &BlendStateCI;

        const VkDynamicState             DynamicStates[] = {VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR};
        VkPipelineDynamicStateCreateInfo DynamicStateCI{};
        DynamicStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCI.pDynamicStates    = DynamicStates;
        DynamicStateCI.dynamicStateCount = _countof(DynamicStates);
        PipelineCI.pDynamicState         = &DynamicStateCI;

        PipelineCI.renderPass         = vkRenderPass;
        PipelineCI.subpass            = 0;
        PipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCI.basePipelineIndex  = -1;

        res = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
        ASSERT_EQ(res, VK_SUCCESS);
        ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);
    }

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();
    {
        VkImageSubresourceRange SubresRange{};
        SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresRange.layerCount = 1;
        SubresRange.levelCount = 1;

        pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        TestingEnvironmentVk::TransitionImageLayout(vkCmdBuffer, BitCast<VkImage>(pSRTex->GetNativeHandle()),
                                                    CurrentLayout, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
                                                    SubresRange, 0,
                                                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR);
    }

    VkClearValue          ClearValues[2] = {};
    VkRenderPassBeginInfo BeginInfo{};
    BeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.renderPass        = vkRenderPass;
    BeginInfo.framebuffer       = vkFramebuffer;
    BeginInfo.renderArea.extent = VkExtent2D{SCDesc.Width, SCDesc.Height};
    BeginInfo.clearValueCount   = 2;
    BeginInfo.pClearValues      = ClearValues;
    vkCmdBeginRenderPass(vkCmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
    {
        const VkExtent2D                         FragmentSize{1, 1};
        const VkFragmentShadingRateCombinerOpKHR CombinerOps[] = {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR};
        vkCmdSetFragmentShadingRateKHR(vkCmdBuffer, &FragmentSize, CombinerOps);

        vkCmdDraw(vkCmdBuffer, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(vkCmdBuffer);

    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res == VK_SUCCESS, "Failed to end command buffer");

    pEnv->SubmitCommandBuffer(vkCmdBuffer);

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkVSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
    vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
    vkDestroyFramebuffer(vkDevice, vkFramebuffer, nullptr);
    vkDestroyImageView(vkDevice, vkShadingRateView, nullptr);
}

} // namespace Testing
} // namespace Diligent
