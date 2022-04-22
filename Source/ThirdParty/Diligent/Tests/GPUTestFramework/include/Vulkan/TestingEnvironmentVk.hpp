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

#pragma once

#include <array>

#include "GPUTestingEnvironment.hpp"
#include "DXCompiler.hpp"

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

namespace Diligent
{

namespace Testing
{

class TestingEnvironmentVk final : public GPUTestingEnvironment
{
public:
    using CreateInfo = GPUTestingEnvironment::CreateInfo;
    TestingEnvironmentVk(const CreateInfo&    CI,
                         const SwapChainDesc& SCDesc);
    ~TestingEnvironmentVk();

    static TestingEnvironmentVk* GetInstance() { return ClassPtrCast<TestingEnvironmentVk>(GPUTestingEnvironment::GetInstance()); }

    void CreateImage2D(uint32_t          Width,
                       uint32_t          Height,
                       VkFormat          vkFormat,
                       VkImageUsageFlags vkUsage,
                       VkImageLayout     vkInitialLayout,
                       VkDeviceMemory&   vkMemory,
                       VkImage&          vkImage);

    void CreateBuffer(VkDeviceSize          Size,
                      VkBufferUsageFlags    vkUsage,
                      VkMemoryPropertyFlags MemoryFlags,
                      VkDeviceMemory&       vkMemory,
                      VkBuffer&             vkBuffer);

    uint32_t GetMemoryTypeIndex(uint32_t              memoryTypeBitsRequirement,
                                VkMemoryPropertyFlags requiredProperties) const;

    VkDevice GetVkDevice()
    {
        return m_vkDevice;
    }

    VkPhysicalDevice GetVkPhysicalDevice()
    {
        return m_vkPhysicalDevice;
    }

    virtual bool HasDXCompiler() const override final
    {
        return m_pDxCompiler != nullptr && m_pDxCompiler->IsLoaded();
    }

    virtual void GetDXCompilerVersion(Uint32& MajorVersion, Uint32& MinorVersion) const override final
    {
        if (m_pDxCompiler != nullptr)
            m_pDxCompiler->GetVersion(MajorVersion, MinorVersion);
    }

    virtual bool SupportsRayTracing() const override final;

    VkShaderModule CreateShaderModule(const SHADER_TYPE ShaderType, const std::string& ShaderSource);

    static VkRenderPassCreateInfo GetRenderPassCreateInfo(
        Uint32                                                       NumRenderTargets,
        const VkFormat                                               RTVFormats[],
        VkFormat                                                     DSVFormat,
        Uint32                                                       SampleCount,
        VkAttachmentLoadOp                                           DepthAttachmentLoadOp,
        VkAttachmentLoadOp                                           ColorAttachmentLoadOp,
        std::array<VkAttachmentDescription, MAX_RENDER_TARGETS + 1>& Attachments,
        std::array<VkAttachmentReference, MAX_RENDER_TARGETS + 1>&   AttachmentReferences,
        VkSubpassDescription&                                        SubpassDesc);

    VkCommandBuffer AllocateCommandBuffer();

    void SubmitCommandBuffer(VkCommandBuffer vkCmdBuffer, bool WaitForIdle = true);

    static void TransitionImageLayout(VkCommandBuffer                CmdBuffer,
                                      VkImage                        Image,
                                      VkImageLayout&                 CurrentLayout,
                                      VkImageLayout                  NewLayout,
                                      const VkImageSubresourceRange& SubresRange,
                                      VkPipelineStageFlags           EnabledGraphicsShaderStages,
                                      VkPipelineStageFlags           SrcStages  = 0,
                                      VkPipelineStageFlags           DestStages = 0);

private:
    VkDevice         m_vkDevice         = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    VkCommandPool    m_vkCmdPool        = VK_NULL_HANDLE;
    VkFence          m_vkFence          = VK_NULL_HANDLE;

    std::unique_ptr<IDXCompiler> m_pDxCompiler;

    VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};

public:
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT DescriptorIndexing = {};
    VkPhysicalDeviceProperties                    DeviceProps        = {};
};

} // namespace Testing

} // namespace Diligent
