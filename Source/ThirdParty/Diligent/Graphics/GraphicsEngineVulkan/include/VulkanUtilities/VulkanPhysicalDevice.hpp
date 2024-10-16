/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include <memory>
#include <vector>
#include "VulkanInstance.hpp"
#include "IndexWrapper.hpp"

namespace VulkanUtilities
{
using Diligent::HardwareQueueIndex;

class VulkanPhysicalDevice
{
public:
    struct CreateInfo
    {
        const VulkanInstance&  Instance;
        const VkPhysicalDevice vkDevice;
        bool                   LogExtensions = false;
    };

    struct ExtensionFeatures
    {
        VkPhysicalDeviceMeshShaderFeaturesEXT             MeshShader             = {};
        VkPhysicalDevice16BitStorageFeaturesKHR           Storage16Bit           = {};
        VkPhysicalDevice8BitStorageFeaturesKHR            Storage8Bit            = {};
        VkPhysicalDeviceShaderFloat16Int8FeaturesKHR      ShaderFloat16Int8      = {};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR  AccelStruct            = {};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR     RayTracingPipeline     = {};
        VkPhysicalDeviceRayQueryFeaturesKHR               RayQuery               = {};
        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR    BufferDeviceAddress    = {};
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT     DescriptorIndexing     = {};
        VkPhysicalDevicePortabilitySubsetFeaturesKHR      PortabilitySubset      = {};
        VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT VertexAttributeDivisor = {};
        VkPhysicalDeviceTimelineSemaphoreFeaturesKHR      TimelineSemaphore      = {};
        VkPhysicalDeviceHostQueryResetFeatures            HostQueryReset         = {};
        VkPhysicalDeviceFragmentShadingRateFeaturesKHR    ShadingRate            = {};
        VkPhysicalDeviceFragmentDensityMapFeaturesEXT     FragmentDensityMap     = {}; // Only for desktop devices
        VkPhysicalDeviceFragmentDensityMap2FeaturesEXT    FragmentDensityMap2    = {}; // Only for mobile devices
        VkPhysicalDeviceMultiviewFeaturesKHR              Multiview              = {}; // Required for RenderPass2
        VkPhysicalDeviceMultiDrawFeaturesEXT              MultiDraw              = {};
        VkPhysicalDeviceShaderDrawParametersFeatures      ShaderDrawParameters   = {};

        bool Spirv14              = false; // Ray tracing requires Vulkan 1.2 or SPIRV 1.4 extension
        bool Spirv15              = false; // DXC shaders with ray tracing requires Vulkan 1.2 with SPIRV 1.5
        bool SubgroupOps          = false; // Requires Vulkan 1.1
        bool HasPortabilitySubset = false;
        bool RenderPass2          = false;
        bool DrawIndirectCount    = false;
    };

    struct ExtensionProperties
    {
        VkPhysicalDeviceMeshShaderPropertiesEXT             MeshShader             = {};
        VkPhysicalDeviceAccelerationStructurePropertiesKHR  AccelStruct            = {};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR     RayTracingPipeline     = {};
        VkPhysicalDeviceDescriptorIndexingPropertiesEXT     DescriptorIndexing     = {};
        VkPhysicalDevicePortabilitySubsetPropertiesKHR      PortabilitySubset      = {};
        VkPhysicalDeviceSubgroupProperties                  Subgroup               = {};
        VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT VertexAttributeDivisor = {};
        VkPhysicalDeviceTimelineSemaphorePropertiesKHR      TimelineSemaphore      = {};
        VkPhysicalDeviceFragmentShadingRatePropertiesKHR    ShadingRate            = {};
        VkPhysicalDeviceFragmentDensityMapPropertiesEXT     FragmentDensityMap     = {};
        VkPhysicalDeviceMultiviewPropertiesKHR              Multiview              = {};
        VkPhysicalDeviceMaintenance3Properties              Maintenance3           = {};
        VkPhysicalDeviceFragmentDensityMap2PropertiesEXT    FragmentDensityMap2    = {};
        VkPhysicalDeviceMultiDrawPropertiesEXT              MultiDraw              = {};
    };

public:
    // clang-format off
    VulkanPhysicalDevice             (const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice             (VulkanPhysicalDevice&&)      = delete;
    VulkanPhysicalDevice& operator = (const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice& operator = (VulkanPhysicalDevice&&)      = delete;
    // clang-format on

    static std::unique_ptr<VulkanPhysicalDevice> Create(const CreateInfo& CI);

    HardwareQueueIndex FindQueueFamily(VkQueueFlags QueueFlags) const;

    bool IsExtensionSupported(const char* ExtensionName) const;
    bool CheckPresentSupport(HardwareQueueIndex queueFamilyIndex, VkSurfaceKHR VkSurface) const;

    static constexpr uint32_t InvalidMemoryTypeIndex = ~uint32_t{0};

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

    VkPhysicalDevice                            GetVkDeviceHandle() const { return m_VkDevice; }
    uint32_t                                    GetVkVersion() const { return m_VkVersion; }
    const VkPhysicalDeviceProperties&           GetProperties() const { return m_Properties; }
    const VkPhysicalDeviceFeatures&             GetFeatures() const { return m_Features; }
    const ExtensionFeatures&                    GetExtFeatures() const { return m_ExtFeatures; }
    const ExtensionProperties&                  GetExtProperties() const { return m_ExtProperties; }
    const VkPhysicalDeviceMemoryProperties&     GetMemoryProperties() const { return m_MemoryProperties; }
    VkFormatProperties                          GetPhysicalDeviceFormatProperties(VkFormat imageFormat) const;
    const std::vector<VkQueueFamilyProperties>& GetQueueProperties() const { return m_QueueFamilyProperties; }

private:
    VulkanPhysicalDevice(const CreateInfo& CI);

    const VkPhysicalDevice               m_VkDevice;
    uint32_t                             m_VkVersion        = 0;
    VkPhysicalDeviceProperties           m_Properties       = {};
    VkPhysicalDeviceFeatures             m_Features         = {};
    VkPhysicalDeviceMemoryProperties     m_MemoryProperties = {};
    ExtensionFeatures                    m_ExtFeatures      = {};
    ExtensionProperties                  m_ExtProperties    = {};
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
    std::vector<VkExtensionProperties>   m_SupportedExtensions;
};

} // namespace VulkanUtilities
