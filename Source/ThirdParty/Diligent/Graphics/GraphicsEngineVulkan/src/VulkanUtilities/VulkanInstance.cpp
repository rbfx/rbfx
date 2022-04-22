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

#include "VulkanUtilities/VulkanInstance.hpp"
#include "APIInfo.h"

#include <vector>
#include <cstring>
#include <algorithm>

#if DILIGENT_USE_VOLK
#    define VOLK_IMPLEMENTATION
#    include "volk.h"
#endif

#include "VulkanErrors.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"

#if !DILIGENT_NO_GLSLANG
#    include "GLSLangUtils.hpp"
#endif

#include "PlatformDefinitions.h"

namespace VulkanUtilities
{

bool VulkanInstance::IsLayerAvailable(const char* LayerName, uint32_t& Version) const
{
    for (const auto& Layer : m_Layers)
    {
        if (strcmp(Layer.layerName, LayerName) == 0)
        {
            Version = Layer.specVersion;
            return true;
        }
    }
    return false;
}

bool VulkanInstance::IsExtensionAvailable(const char* ExtensionName) const
{
    for (const auto& Extension : m_Extensions)
        if (strcmp(Extension.extensionName, ExtensionName) == 0)
            return true;

    return false;
}

bool VulkanInstance::IsExtensionEnabled(const char* ExtensionName) const
{
    for (const auto& Extension : m_EnabledExtensions)
        if (strcmp(Extension, ExtensionName) == 0)
            return true;

    return false;
}

std::shared_ptr<VulkanInstance> VulkanInstance::Create(const CreateInfo& CI)
{
    auto Instance = new VulkanInstance{CI};
    return std::shared_ptr<VulkanInstance>{Instance};
}

VulkanInstance::VulkanInstance(const CreateInfo& CI) :
    m_pVkAllocator{CI.pVkAllocator}
{
#if DILIGENT_USE_VOLK
    if (volkInitialize() != VK_SUCCESS)
    {
        LOG_ERROR_AND_THROW("Failed to load Vulkan.");
    }
#endif

    {
        // Enumerate available layers
        uint32_t LayerCount = 0;

        auto res = vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
        CHECK_VK_ERROR_AND_THROW(res, "Failed to query layer count");
        m_Layers.resize(LayerCount);
        vkEnumerateInstanceLayerProperties(&LayerCount, m_Layers.data());
        CHECK_VK_ERROR_AND_THROW(res, "Failed to enumerate extensions");
        VERIFY_EXPR(LayerCount == m_Layers.size());
    }

    {
        // Enumerate available extensions
        uint32_t ExtensionCount = 0;

        auto res = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);
        CHECK_VK_ERROR_AND_THROW(res, "Failed to query extension count");
        m_Extensions.resize(ExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, m_Extensions.data());
        CHECK_VK_ERROR_AND_THROW(res, "Failed to enumerate extensions");
        VERIFY_EXPR(ExtensionCount == m_Extensions.size());
    }

    std::vector<const char*> InstanceExtensions =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

    // Enable surface extensions depending on OS
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
        VK_MVK_IOS_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
    };

    // This extension added to core in 1.1, but current version is 1.0
    if (IsExtensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        InstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    if (CI.ppInstanceExtensionNames != nullptr)
    {
        for (uint32_t ext = 0; ext < CI.InstanceExtensionCount; ++ext)
            InstanceExtensions.push_back(CI.ppInstanceExtensionNames[ext]);
    }
    else
    {
        if (CI.InstanceExtensionCount != 0)
        {
            LOG_ERROR_MESSAGE("Global extensions pointer is null while extensions count is ", CI.InstanceExtensionCount,
                              ". Please initialize 'ppInstanceExtensionNames' member of EngineVkCreateInfo struct.");
        }
    }

    for (const auto* ExtName : InstanceExtensions)
    {
        if (!IsExtensionAvailable(ExtName))
            LOG_ERROR_AND_THROW("Required extension ", ExtName, " is not available");
    }

    if (CI.EnableValidation)
    {
        m_DebugUtilsEnabled = IsExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (m_DebugUtilsEnabled)
        {
            InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else
        {
            LOG_WARNING_MESSAGE("Extension ", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, " is not available.");
        }
    }

    auto ApiVersion = CI.ApiVersion;
#if DILIGENT_USE_VOLK
    if (vkEnumerateInstanceVersion != nullptr && ApiVersion > VK_API_VERSION_1_0)
    {
        uint32_t MaxApiVersion = 0;
        vkEnumerateInstanceVersion(&MaxApiVersion);
        ApiVersion = std::min(ApiVersion, MaxApiVersion);
    }
    else
    {
        // Only Vulkan 1.0 is supported.
        ApiVersion = VK_API_VERSION_1_0;
    }
#else
    // Without Volk we can only use Vulkan 1.0
    ApiVersion = VK_API_VERSION_1_0;
#endif

    std::vector<const char*> InstanceLayers;

    // Use VK_DEVSIM_FILENAME environment variable to define the simulation layer
    if (CI.EnableDeviceSimulation)
    {
        static const char* DeviceSimulationLayer = "VK_LAYER_LUNARG_device_simulation";

        uint32_t LayerVer = 0;
        if (IsLayerAvailable(DeviceSimulationLayer, LayerVer))
        {
            InstanceLayers.push_back(DeviceSimulationLayer);
        }
    }

    if (CI.EnableValidation)
    {
        for (size_t l = 0; l < _countof(VulkanUtilities::ValidationLayerNames); ++l)
        {
            auto*    pLayerName = VulkanUtilities::ValidationLayerNames[l];
            uint32_t LayerVer   = ~0u; // Prevent warning if the layer is not found
            if (IsLayerAvailable(pLayerName, LayerVer))
                InstanceLayers.push_back(pLayerName);
            else
                LOG_WARNING_MESSAGE("Failed to find '", pLayerName, "' layer. Validation will be disabled");

            // Beta extensions may vary and result in a crash.
            // New enums are not supported and may cause validation error.
            if (LayerVer < VK_HEADER_VERSION_COMPLETE)
            {
                LOG_WARNING_MESSAGE("Layer '", pLayerName, "' version (", VK_VERSION_MAJOR(LayerVer), ".", VK_VERSION_MINOR(LayerVer), ".", VK_VERSION_PATCH(LayerVer),
                                    ") is less than header version (",
                                    VK_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE), ".", VK_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE), ".", VK_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE),
                                    ").");
            }
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr; // Pointer to an extension-specific structure.
    appInfo.pApplicationName   = nullptr;
    appInfo.applicationVersion = 0; // Developer-supplied version number of the application
    appInfo.pEngineName        = "Diligent Engine";
    appInfo.engineVersion      = DILIGENT_API_VERSION; // Developer-supplied version number of the engine used to create the application.
    appInfo.apiVersion         = ApiVersion;

    VkInstanceCreateInfo InstanceCreateInfo{};
    InstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pNext                   = nullptr; // Pointer to an extension-specific structure.
    InstanceCreateInfo.flags                   = 0;       // Reserved for future use.
    InstanceCreateInfo.pApplicationInfo        = &appInfo;
    InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(InstanceExtensions.size());
    InstanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.empty() ? nullptr : InstanceExtensions.data();
    InstanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(InstanceLayers.size());
    InstanceCreateInfo.ppEnabledLayerNames     = InstanceLayers.empty() ? nullptr : InstanceLayers.data();

    auto res = vkCreateInstance(&InstanceCreateInfo, m_pVkAllocator, &m_VkInstance);
    CHECK_VK_ERROR_AND_THROW(res, "Failed to create Vulkan instance");

#if DILIGENT_USE_VOLK
    volkLoadInstance(m_VkInstance);
#endif

    m_EnabledExtensions = std::move(InstanceExtensions);
    m_VkVersion         = ApiVersion;

    // If requested, we enable the default validation layers for debugging
    if (m_DebugUtilsEnabled)
    {
        VkDebugUtilsMessageSeverityFlagsEXT messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        VkDebugUtilsMessageTypeFlagsEXT messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        VulkanUtilities::SetupDebugging(m_VkInstance, messageSeverity, messageType, nullptr);
    }

    // Enumerate physical devices
    {
        // Physical device
        uint32_t PhysicalDeviceCount = 0;
        // Get the number of available physical devices
        auto err = vkEnumeratePhysicalDevices(m_VkInstance, &PhysicalDeviceCount, nullptr);
        CHECK_VK_ERROR(err, "Failed to get physical device count");
        if (PhysicalDeviceCount == 0)
            LOG_ERROR_AND_THROW("No physical devices found on the system");

        // Enumerate devices
        m_PhysicalDevices.resize(PhysicalDeviceCount);
        err = vkEnumeratePhysicalDevices(m_VkInstance, &PhysicalDeviceCount, m_PhysicalDevices.data());
        CHECK_VK_ERROR(err, "Failed to enumerate physical devices");
        VERIFY_EXPR(m_PhysicalDevices.size() == PhysicalDeviceCount);
    }
#if !DILIGENT_NO_GLSLANG
    Diligent::GLSLangUtils::InitializeGlslang();
#endif
}

VulkanInstance::~VulkanInstance()
{
    if (m_DebugUtilsEnabled)
    {
        VulkanUtilities::FreeDebugging(m_VkInstance);
    }
    vkDestroyInstance(m_VkInstance, m_pVkAllocator);

#if !DILIGENT_NO_GLSLANG
    Diligent::GLSLangUtils::FinalizeGlslang();
#endif
}

VkPhysicalDevice VulkanInstance::SelectPhysicalDevice(uint32_t AdapterId) const
{
    const auto IsGraphicsAndComputeQueueSupported = [](VkPhysicalDevice Device) {
        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);
        VERIFY_EXPR(QueueFamilyCount > 0);
        std::vector<VkQueueFamilyProperties> QueueFamilyProperties(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilyProperties.data());
        VERIFY_EXPR(QueueFamilyCount == QueueFamilyProperties.size());

        // If an implementation exposes any queue family that supports graphics operations,
        // at least one queue family of at least one physical device exposed by the implementation
        // must support both graphics and compute operations.
        for (const auto& QueueFamilyProps : QueueFamilyProperties)
        {
            if ((QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
                (QueueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
            {
                return true;
            }
        }
        return false;
    };

    VkPhysicalDevice SelectedPhysicalDevice = VK_NULL_HANDLE;

    if (AdapterId < m_PhysicalDevices.size() &&
        IsGraphicsAndComputeQueueSupported(m_PhysicalDevices[AdapterId]))
    {
        SelectedPhysicalDevice = m_PhysicalDevices[AdapterId];
    }

    // Select a device that exposes a queue family that supports both compute and graphics operations.
    // Prefer discrete GPU.
    if (SelectedPhysicalDevice == VK_NULL_HANDLE)
    {
        for (auto Device : m_PhysicalDevices)
        {
            VkPhysicalDeviceProperties DeviceProps;
            vkGetPhysicalDeviceProperties(Device, &DeviceProps);

            if (IsGraphicsAndComputeQueueSupported(Device))
            {
                SelectedPhysicalDevice = Device;
                if (DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    break;
            }
        }
    }

    if (SelectedPhysicalDevice != VK_NULL_HANDLE)
    {
        VkPhysicalDeviceProperties SelectedDeviceProps;
        vkGetPhysicalDeviceProperties(SelectedPhysicalDevice, &SelectedDeviceProps);
        LOG_INFO_MESSAGE("Using physical device '", SelectedDeviceProps.deviceName,
                         "', API version ",
                         VK_VERSION_MAJOR(SelectedDeviceProps.apiVersion), '.',
                         VK_VERSION_MINOR(SelectedDeviceProps.apiVersion), '.',
                         VK_VERSION_PATCH(SelectedDeviceProps.apiVersion),
                         ", Driver version ",
                         VK_VERSION_MAJOR(SelectedDeviceProps.driverVersion), '.',
                         VK_VERSION_MINOR(SelectedDeviceProps.driverVersion), '.',
                         VK_VERSION_PATCH(SelectedDeviceProps.driverVersion), '.');
    }
    else
    {
        LOG_ERROR_MESSAGE("Failed to find suitable physical device");
    }

    return SelectedPhysicalDevice;
}

} // namespace VulkanUtilities
