// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/OpenXR.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Engine/Engine.h"
#include "Urho3D/Engine/EngineDefs.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsEvents.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Input/InputEvents.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/Localization.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLElement.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/PrefabReference.h"
#include "Urho3D/Scene/PrefabResource.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Utility/GLTFImporter.h"
#include "Urho3D/XR/OpenXRAPI.h"
#include "Urho3D/XR/VREvents.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#if D3D11_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h>
#endif
#if D3D12_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/CommandQueueD3D12.h>
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h>
#endif
#if VULKAN_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/CommandQueueVk.h>
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h>
#endif

#include <ThirdParty/OpenXRSDK/include/openxr/openxr_platform.h>
#include <ThirdParty/OpenXRSDK/include/openxr/openxr_platform_defines.h>

#include <EASTL/optional.h>

#include <SDL.h>

#include "../DebugNew.h"

#if URHO3D_PLATFORM_ANDROID
// TODO: This is a hack to get EGLConfig in SDL2.
// Replace with SDL_EGL_GetCurrentEGLConfig in SDL3.
extern "C" EGLConfig SDL_EGL_GetConfig();
#endif

namespace Urho3D
{

namespace
{

bool IsNativeOculusQuest2()
{
#ifdef URHO3D_OCULUS_QUEST
    return true;
#else
    return false;
#endif
}

StringVector EnumerateExtensionsXR()
{
    uint32_t count = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr);

    ea::vector<XrExtensionProperties> extensions;
    extensions.resize(count, XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extensions.size(), &count, extensions.data());

    StringVector result;
    for (const XrExtensionProperties& extension : extensions)
        result.push_back(extension.extensionName);
    return result;
}

bool IsExtensionSupported(const StringVector& extensions, const char* name)
{
    for (const ea::string& ext : extensions)
    {
        if (ext.comparei(name) == 0)
            return true;
    }
    return false;
}

bool ActivateOptionalExtension(StringVector& result, const StringVector& extensions, const char* name)
{
    if (IsExtensionSupported(extensions, name))
    {
        result.push_back(name);
        return true;
    }
    return false;
}

const char* GetBackendExtensionName(RenderBackend backend)
{
    switch (backend)
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11: return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12: return XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan: return XR_KHR_VULKAN_ENABLE_EXTENSION_NAME;
#endif
#if GLES_SUPPORTED
    case RenderBackend::OpenGL: return XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
#endif
#if GL_SUPPORTED
    case RenderBackend::OpenGL: return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
#endif
    default: return "";
    }
}

XrInstancePtr CreateInstanceXR(
    const StringVector& extensions, const ea::string& engineName, const ea::string& applicationName)
{
    const auto extensionNames = ToCStringVector(extensions);

    XrInstanceCreateInfo info = {XR_TYPE_INSTANCE_CREATE_INFO};
    strncpy(info.applicationInfo.engineName, engineName.c_str(), XR_MAX_ENGINE_NAME_SIZE);
    strncpy(info.applicationInfo.applicationName, applicationName.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
    info.applicationInfo.engineVersion = (1 << 24) + (0 << 16) + 0; // TODO: get an actual engine version
    info.applicationInfo.applicationVersion = 0; // TODO: application version?
    info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    info.enabledExtensionCount = extensionNames.size();
    info.enabledExtensionNames = extensionNames.data();

#if URHO3D_PLATFORM_ANDROID
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    JavaVM* vm = nullptr;
    env->GetJavaVM(&vm);

    XrInstanceCreateInfoAndroidKHR androidInfo = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    androidInfo.applicationVM = vm;
    androidInfo.applicationActivity = SDL_AndroidGetActivity();
    info.next = &androidInfo;
#endif

    XrInstance instance;
    if (!URHO3D_CHECK_OPENXR(xrCreateInstance(&info, &instance)))
        return nullptr;

    LoadOpenXRAPI(instance);

    const auto deleter = [](XrInstance instance)
    {
        xrDestroyInstance(instance);
        UnloadOpenXRAPI();
    };
    return XrInstancePtr(instance, deleter);
}

XrBool32 XRAPI_PTR DebugMessageLoggerXR(XrDebugUtilsMessageSeverityFlagsEXT severity,
    XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data)
{
    if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        URHO3D_LOGERROR("XR Error: {}, {}", msg->functionName, msg->message);
    else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        URHO3D_LOGWARNING("XR Warning: {}, {}", msg->functionName, msg->message);
    else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        URHO3D_LOGINFO("XR Info: {}, {}", msg->functionName, msg->message);
    else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        URHO3D_LOGDEBUG("XR Debug: {}, {}", msg->functionName, msg->message);

    return false;
};

XrDebugUtilsMessengerEXTPtr CreateDebugMessengerXR(XrInstance instance)
{
    XrDebugUtilsMessengerCreateInfoEXT debugUtils = {XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

    debugUtils.userCallback = DebugMessageLoggerXR;
    debugUtils.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT //
        | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT //
        | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    debugUtils.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT //
        | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT //
        | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    XrDebugUtilsMessengerEXT messenger;
    xrCreateDebugUtilsMessengerEXT(instance, &debugUtils, &messenger);
    if (!messenger)
        return nullptr;

    return XrDebugUtilsMessengerEXTPtr(messenger, xrDestroyDebugUtilsMessengerEXT);
}

ea::optional<XrSystemId> GetSystemXR(XrInstance instance)
{
    XrSystemGetInfo sysInfo = {XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId;
    if (!URHO3D_CHECK_OPENXR(xrGetSystem(instance, &sysInfo, &systemId)))
        return ea::nullopt;

    return systemId;
}

ea::string GetSystemNameXR(XrInstance instance, XrSystemId system)
{
    XrSystemProperties properties = {XR_TYPE_SYSTEM_PROPERTIES};
    if (!URHO3D_CHECK_OPENXR(xrGetSystemProperties(instance, system, &properties)))
        return "";
    return properties.systemName;
}

ea::vector<XrEnvironmentBlendMode> GetBlendModesXR(XrInstance instance, XrSystemId system)
{
    uint32_t count = 0;
    xrEnumerateEnvironmentBlendModes(instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &count, nullptr);

    ea::vector<XrEnvironmentBlendMode> result(count);
    xrEnumerateEnvironmentBlendModes(
        instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, count, &count, result.data());

    if (count == 0)
    {
        URHO3D_LOGERROR("Failed to get OpenXR blend modes");
        return {};
    }

    return result;
}

ea::vector<XrViewConfigurationType> GetViewConfigurationsXR(XrInstance instance, XrSystemId system)
{
    uint32_t count = 0;
    xrEnumerateViewConfigurations(instance, system, 0, &count, nullptr);

    ea::vector<XrViewConfigurationType> result(count);
    xrEnumerateViewConfigurations(instance, system, count, &count, result.data());

    return result;
}

ea::optional<EnumArray<XrViewConfigurationView, VREye>> GetViewConfigurationViewsXR(
    XrInstance instance, XrSystemId system)
{
    EnumArray<XrViewConfigurationView, VREye> result{{XR_TYPE_VIEW_CONFIGURATION_VIEW}};

    unsigned count = 0;
    if (URHO3D_CHECK_OPENXR(xrEnumerateViewConfigurationViews(
            instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &count, result.data())))
    {
        return result;
    }

    return ea::nullopt;
}

#if VULKAN_SUPPORTED
StringVector GetVulkanInstanceExtensionsXR(XrInstance instance, XrSystemId system)
{
    uint32_t bufferSize = 0;
    xrGetVulkanInstanceExtensionsKHR(instance, system, 0, &bufferSize, nullptr);
    ea::string buffer(bufferSize, '\0');
    xrGetVulkanInstanceExtensionsKHR(instance, system, bufferSize, &bufferSize, buffer.data());
    return buffer.split(' ');
}

StringVector GetVulkanDeviceExtensionsXR(XrInstance instance, XrSystemId system)
{
    uint32_t bufferSize = 0;
    xrGetVulkanDeviceExtensionsKHR(instance, system, 0, &bufferSize, nullptr);
    ea::string buffer(bufferSize, '\0');
    xrGetVulkanDeviceExtensionsKHR(instance, system, bufferSize, &bufferSize, buffer.data());
    return buffer.split(' ');
}
#endif

ea::vector<int64_t> GetSwapChainFormats(XrSession session)
{
    unsigned count = 0;
    xrEnumerateSwapchainFormats(session, 0, &count, 0);

    ea::vector<int64_t> result;
    result.resize(count);
    xrEnumerateSwapchainFormats(session, count, &count, result.data());

    return result;
}

/// Try to use sRGB texture formats whenever possible, i.e. linear output.
/// Oculus Quest 2 always expects linear input even if the framebuffer is not sRGB:
/// https://developer.oculus.com/resources/color-management-guide/
bool IsFallbackColorFormat(TextureFormat format)
{
    return SetTextureFormatSRGB(format, true) != format;
}

/// 16-bit depth is just not enough.
bool IsFallbackDepthFormat(TextureFormat format)
{
    return format == TextureFormat::TEX_FORMAT_D16_UNORM;
}

ea::pair<TextureFormat, int64_t> SelectColorFormat(RenderBackend backend, const ea::vector<int64_t>& formats)
{
    for (bool fallback : {false, true})
    {
        for (const auto internalFormat : formats)
        {
            const TextureFormat textureFormat = GetTextureFormatFromInternal(backend, internalFormat);

            // Oculus Quest 2 does not support sRGB framebuffers natively.
            if (IsNativeOculusQuest2() && IsTextureFormatSRGB(textureFormat))
                continue;

            if (IsColorTextureFormat(textureFormat) && IsFallbackColorFormat(textureFormat) == fallback)
                return {textureFormat, internalFormat};
        }
    }
    return {TextureFormat::TEX_FORMAT_UNKNOWN, 0};
}

ea::pair<TextureFormat, int64_t> SelectDepthFormat(RenderBackend backend, const ea::vector<int64_t>& formats)
{
    // Oculus Quest 2 returns non-framebuffer-compatible depth formats.
    if (!IsNativeOculusQuest2())
    {
        for (bool fallback : {false, true})
        {
            for (const auto internalFormat : formats)
            {
                const TextureFormat textureFormat = GetTextureFormatFromInternal(backend, internalFormat);
                if (IsDepthTextureFormat(textureFormat) && IsFallbackDepthFormat(textureFormat) == fallback)
                    return {textureFormat, internalFormat};
            }
        }
    }
    return {TextureFormat::TEX_FORMAT_UNKNOWN, 0};
}

XrSessionPtr CreateSessionXR(RenderDevice* renderDevice, XrInstance instance, XrSystemId system)
{
    XrSessionCreateInfo sessionCreateInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.systemId = system;

    XrSession session{};
    switch (renderDevice->GetBackend())
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11:
    {
        XrGraphicsRequirementsD3D11KHR requisite = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        if (!URHO3D_CHECK_OPENXR(xrGetD3D11GraphicsRequirementsKHR(instance, system, &requisite)))
            return nullptr;

        const auto renderDeviceD3D11 = static_cast<Diligent::IRenderDeviceD3D11*>(renderDevice->GetRenderDevice());

        XrGraphicsBindingD3D11KHR binding = {XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        binding.device = renderDeviceD3D11->GetD3D11Device();
        sessionCreateInfo.next = &binding;

        if (!URHO3D_CHECK_OPENXR(xrCreateSession(instance, &sessionCreateInfo, &session)))
            return nullptr;

        break;
    }
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12:
    {
        XrGraphicsRequirementsD3D12KHR requisite = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
        if (!URHO3D_CHECK_OPENXR(xrGetD3D12GraphicsRequirementsKHR(instance, system, &requisite)))
            return nullptr;

        const auto renderDeviceD3D12 = static_cast<Diligent::IRenderDeviceD3D12*>(renderDevice->GetRenderDevice());
        const auto immediateContext = renderDevice->GetImmediateContext();
        const auto commandQueue = immediateContext->LockCommandQueue();
        immediateContext->UnlockCommandQueue();
        const auto commandQueueD3D12 = static_cast<Diligent::ICommandQueueD3D12*>(commandQueue);

        XrGraphicsBindingD3D12KHR binding = {XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
        binding.device = renderDeviceD3D12->GetD3D12Device();
        binding.queue = commandQueueD3D12->GetD3D12CommandQueue();
        sessionCreateInfo.next = &binding;

        if (!URHO3D_CHECK_OPENXR(xrCreateSession(instance, &sessionCreateInfo, &session)))
            return nullptr;

        break;
    }
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
    {
        XrGraphicsRequirementsVulkanKHR requisite = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
        if (!URHO3D_CHECK_OPENXR(xrGetVulkanGraphicsRequirementsKHR(instance, system, &requisite)))
            return nullptr;

        const auto renderDeviceVk = static_cast<Diligent::IRenderDeviceVk*>(renderDevice->GetRenderDevice());
        const auto immediateContext = renderDevice->GetImmediateContext();
        const auto commandQueue = immediateContext->LockCommandQueue();
        immediateContext->UnlockCommandQueue();
        const auto commandQueueVk = static_cast<Diligent::ICommandQueueVk*>(commandQueue);

        XrGraphicsBindingVulkanKHR binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
        binding.instance = renderDeviceVk->GetVkInstance();
        binding.physicalDevice = renderDeviceVk->GetVkPhysicalDevice();
        binding.device = renderDeviceVk->GetVkDevice();
        binding.queueFamilyIndex = commandQueueVk->GetQueueFamilyIndex();
        binding.queueIndex = 0; // TODO: This would be incorrect if we use multiple immediate queues.
        sessionCreateInfo.next = &binding;

        // We cannot do anything if the device does not match, in current architecture of Diligent.
        VkPhysicalDevice requiredPhysicalDevice{};
        xrGetVulkanGraphicsDeviceKHR(instance, system, binding.instance, &requiredPhysicalDevice);
        if (requiredPhysicalDevice != binding.physicalDevice)
        {
            URHO3D_LOGERROR("OpenXR cannot use current VkPhysicalDevice");
            return nullptr;
        }

        if (!URHO3D_CHECK_OPENXR(xrCreateSession(instance, &sessionCreateInfo, &session)))
            return nullptr;

        break;
    }
#endif
#if GL_SUPPORTED && URHO3D_PLATFORM_WINDOWS
    case RenderBackend::OpenGL:
    {
        XrGraphicsRequirementsOpenGLKHR requisite = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
        if (!URHO3D_CHECK_OPENXR(xrGetOpenGLGraphicsRequirementsKHR(instance, system, &requisite)))
            return nullptr;

    #if URHO3D_PLATFORM_WINDOWS
        XrGraphicsBindingOpenGLWin32KHR binding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
        binding.hDC = wglGetCurrentDC();
        binding.hGLRC = wglGetCurrentContext();
        sessionCreateInfo.next = &binding;
    #else
        URHO3D_ASSERTLOG(false, "OpenXR is not implemented for this platform");
        return nullptr;
    #endif

        if (!URHO3D_CHECK_OPENXR(xrCreateSession(instance, &sessionCreateInfo, &session)))
            return nullptr;

        break;
    }
#endif
#if GLES_SUPPORTED && URHO3D_PLATFORM_ANDROID
    case RenderBackend::OpenGL:
    {
        XrGraphicsRequirementsOpenGLESKHR requisite = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        if (!URHO3D_CHECK_OPENXR(xrGetOpenGLESGraphicsRequirementsKHR(instance, system, &requisite)))
            return nullptr;

    #if URHO3D_PLATFORM_ANDROID
        XrGraphicsBindingOpenGLESAndroidKHR binding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
        binding.display = eglGetCurrentDisplay();
        binding.config = SDL_EGL_GetConfig();
        binding.context = eglGetCurrentContext();
        sessionCreateInfo.next = &binding;
    #else
        URHO3D_ASSERTLOG(false, "OpenXR is not implemented for this platform");
        return nullptr;
    #endif

        if (!URHO3D_CHECK_OPENXR(xrCreateSession(instance, &sessionCreateInfo, &session)))
            return nullptr;

        break;
    }
#endif
    default: URHO3D_ASSERTLOG(false, "OpenXR is not implemented for this backend"); return nullptr;
    }

    return XrSessionPtr(session, xrDestroySession);
}

ea::pair<XrSpacePtr, bool> CreateHeadSpaceXR(XrSession session)
{
    XrReferenceSpaceCreateInfo createInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    createInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    createInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};

    bool isRoomScale = true;
    XrSpace space;
    if (!URHO3D_CHECK_OPENXR(xrCreateReferenceSpace(session, &createInfo, &space)))
    {
        isRoomScale = false;

        createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        if (!URHO3D_CHECK_OPENXR(xrCreateReferenceSpace(session, &createInfo, &space)))
            return {};
    }

    const auto wrappedSpace = XrSpacePtr(space, xrDestroySpace);
    return {wrappedSpace, isRoomScale};
}

XrSpacePtr CreateViewSpaceXR(XrSession session)
{
    XrReferenceSpaceCreateInfo createInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    createInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    createInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};

    XrSpace space;
    if (!URHO3D_CHECK_OPENXR(xrCreateReferenceSpace(session, &createInfo, &space)))
        return nullptr;

    return XrSpacePtr(space, xrDestroySpace);
}

template <class T, XrStructureType ImageStructureType> class OpenXRSwapChainBase : public OpenXRSwapChain
{
public:
    OpenXRSwapChainBase(
        XrSession session, TextureFormat format, int64_t internalFormat, const IntVector2& eyeSize, int msaaLevel)
    {
        format_ = format;
        textureSize_ = arraySize_ == 1 ? eyeSize * IntVector2{2, 1} : eyeSize;

        XrSwapchainCreateInfo swapInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT;

        if (IsDepthTextureFormat(format))
            swapInfo.usageFlags |= XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        else
            swapInfo.usageFlags |= XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        swapInfo.format = internalFormat;
        swapInfo.width = textureSize_.x_;
        swapInfo.height = textureSize_.y_;
        swapInfo.sampleCount = msaaLevel;
        swapInfo.faceCount = 1;
        swapInfo.arraySize = arraySize_;
        swapInfo.mipCount = 1;

        XrSwapchain swapChain;
        if (!URHO3D_CHECK_OPENXR(xrCreateSwapchain(session, &swapInfo, &swapChain)))
            return;

        swapChain_ = XrSwapchainPtr(swapChain, xrDestroySwapchain);

        uint32_t numImages = 0;
        if (!URHO3D_CHECK_OPENXR(xrEnumerateSwapchainImages(swapChain_.Raw(), 0, &numImages, nullptr)))
            return;

        ea::vector<T> images(numImages);
        for (T& image : images)
        {
            image.type = ImageStructureType;
            image.next = nullptr;
        }

        const auto imagesPtr = reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data());
        if (!URHO3D_CHECK_OPENXR(xrEnumerateSwapchainImages(swapChain_.Raw(), numImages, &numImages, imagesPtr)))
            return;

        images_ = ea::move(images);
    }

    virtual ~OpenXRSwapChainBase()
    {
        for (Texture2D* texture : textures_)
            texture->Destroy();
    }

    const T& GetImageXR(unsigned index) const { return images_[index]; }

protected:
    ea::vector<T> images_;
    IntVector2 textureSize_;
};

#if D3D11_SUPPORTED
class OpenXRSwapChainD3D11 : public OpenXRSwapChainBase<XrSwapchainImageD3D11KHR, XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR>
{
public:
    using BaseClass = OpenXRSwapChainBase<XrSwapchainImageD3D11KHR, XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR>;

    OpenXRSwapChainD3D11(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
        const IntVector2& eyeSize, int msaaLevel)
        : BaseClass(session, format, internalFormat, eyeSize, msaaLevel)
    {
        auto renderDevice = context->GetSubsystem<RenderDevice>();

        const unsigned numImages = images_.size();
        textures_.resize(numImages);
        for (unsigned i = 0; i < numImages; ++i)
        {
            URHO3D_ASSERT(arraySize_ == 1);

            textures_[i] = MakeShared<Texture2D>(context);
            textures_[i]->CreateFromD3D11Texture2D(images_[i].texture, format, msaaLevel);
        }
    }
};
#endif

#if D3D12_SUPPORTED
class OpenXRSwapChainD3D12 : public OpenXRSwapChainBase<XrSwapchainImageD3D12KHR, XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR>
{
public:
    using BaseClass = OpenXRSwapChainBase<XrSwapchainImageD3D12KHR, XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR>;

    OpenXRSwapChainD3D12(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
        const IntVector2& eyeSize, int msaaLevel)
        : BaseClass(session, format, internalFormat, eyeSize, msaaLevel)
    {
        auto renderDevice = context->GetSubsystem<RenderDevice>();

        const unsigned numImages = images_.size();
        textures_.resize(numImages);
        for (unsigned i = 0; i < numImages; ++i)
        {
            URHO3D_ASSERT(arraySize_ == 1);

            textures_[i] = MakeShared<Texture2D>(context);
            textures_[i]->CreateFromD3D12Resource(images_[i].texture, format, msaaLevel);
        }
    }
};
#endif

#if VULKAN_SUPPORTED
class OpenXRSwapChainVulkan : public OpenXRSwapChainBase<XrSwapchainImageVulkanKHR, XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR>
{
public:
    using BaseClass = OpenXRSwapChainBase<XrSwapchainImageVulkanKHR, XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR>;

    OpenXRSwapChainVulkan(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
        const IntVector2& eyeSize, int msaaLevel)
        : BaseClass(session, format, internalFormat, eyeSize, msaaLevel)
    {
        auto renderDevice = context->GetSubsystem<RenderDevice>();

        const bool isDepth = IsDepthTextureFormat(format);
        const unsigned numImages = images_.size();
        textures_.resize(numImages);
        for (unsigned i = 0; i < numImages; ++i)
        {
            URHO3D_ASSERT(arraySize_ == 1);

            RawTextureParams params;
            params.type_ = TextureType::Texture2D;
            params.format_ = format;
            params.flags_ = isDepth ? TextureFlag::BindDepthStencil : TextureFlag::BindRenderTarget;
            params.size_ = textureSize_.ToIntVector3(1);
            params.numLevels_ = 1;
            params.multiSample_ = msaaLevel;

            textures_[i] = MakeShared<Texture2D>(context);
            textures_[i]->CreateFromVulkanImage((uint64_t)images_[i].image, params);

            // Oculus Quest 2 always expects texture data in linear space.
            if (IsNativeOculusQuest2())
                textures_[i]->SetLinear(true);
        }
    }
};
#endif

#if GL_SUPPORTED
class OpenXRSwapChainGL : public OpenXRSwapChainBase<XrSwapchainImageOpenGLKHR, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR>
{
public:
    using BaseClass = OpenXRSwapChainBase<XrSwapchainImageOpenGLKHR, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR>;

    OpenXRSwapChainGL(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
        const IntVector2& eyeSize, int msaaLevel)
        : BaseClass(session, format, internalFormat, eyeSize, msaaLevel)
    {
        auto renderDevice = context->GetSubsystem<RenderDevice>();

        const bool isDepth = IsDepthTextureFormat(format);
        const unsigned numImages = images_.size();
        textures_.resize(numImages);
        for (unsigned i = 0; i < numImages; ++i)
        {
            URHO3D_ASSERT(arraySize_ == 1);

            textures_[i] = MakeShared<Texture2D>(context);
            textures_[i]->CreateFromGLTexture(images_[i].image, TextureType::Texture2D,
                isDepth ? TextureFlag::BindDepthStencil : TextureFlag::BindRenderTarget, format, arraySize_, msaaLevel);
        }
    }
};
#endif

#if GLES_SUPPORTED
class OpenXRSwapChainGLES
    : public OpenXRSwapChainBase<XrSwapchainImageOpenGLESKHR, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR>
{
public:
    using BaseClass = OpenXRSwapChainBase<XrSwapchainImageOpenGLESKHR, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR>;

    OpenXRSwapChainGLES(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
        const IntVector2& eyeSize, int msaaLevel)
        : BaseClass(session, format, internalFormat, eyeSize, msaaLevel)
    {
        auto renderDevice = context->GetSubsystem<RenderDevice>();

        const bool isDepth = IsDepthTextureFormat(format);
        const unsigned numImages = images_.size();
        textures_.resize(numImages);
        for (unsigned i = 0; i < numImages; ++i)
        {
            URHO3D_ASSERT(arraySize_ == 1);

            textures_[i] = MakeShared<Texture2D>(context);
            textures_[i]->CreateFromGLTexture(images_[i].image, TextureType::Texture2D,
                isDepth ? TextureFlag::BindDepthStencil : TextureFlag::BindRenderTarget, format, arraySize_, msaaLevel);
            // Oculus Quest 2 always expects texture data in linear space.
            textures_[i]->SetLinear(true);
        }
    }
};
#endif

OpenXRSwapChainPtr CreateSwapChainXR(Context* context, XrSession session, TextureFormat format, int64_t internalFormat,
    const IntVector2& eyeSize, int msaaLevel)
{
    auto renderDevice = context->GetSubsystem<RenderDevice>();

    OpenXRSwapChainPtr result;
    switch (renderDevice->GetBackend())
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11:
        result = ea::make_shared<OpenXRSwapChainD3D11>(context, session, format, internalFormat, eyeSize, msaaLevel);
        break;
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12:
        result = ea::make_shared<OpenXRSwapChainD3D12>(context, session, format, internalFormat, eyeSize, msaaLevel);
        break;
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
        result = ea::make_shared<OpenXRSwapChainVulkan>(context, session, format, internalFormat, eyeSize, msaaLevel);
        break;
#endif
#if GL_SUPPORTED
    case RenderBackend::OpenGL:
        result = ea::make_shared<OpenXRSwapChainGL>(context, session, format, internalFormat, eyeSize, msaaLevel);
        break;
#endif
#if GLES_SUPPORTED
    case RenderBackend::OpenGL:
        result = ea::make_shared<OpenXRSwapChainGLES>(context, session, format, internalFormat, eyeSize, msaaLevel);
        break;
#endif
    default: URHO3D_ASSERTLOG(false, "OpenXR is not implemented for this backend"); break;
    }

    return result && result->GetNumTextures() != 0 ? result : nullptr;
}

ea::optional<VariantType> ParseBindingType(ea::string_view type)
{
    if (type == "boolean")
        return VAR_BOOL;
    else if (type == "vector1" || type == "single")
        return VAR_FLOAT;
    else if (type == "vector2")
        return VAR_VECTOR2;
    else if (type == "vector3")
        return VAR_VECTOR3;
    else if (type == "pose")
        return VAR_MATRIX3X4;
    else if (type == "haptic")
        return VAR_NONE;
    else
        return ea::nullopt;
}

XrActionType ToActionType(VariantType type)
{
    switch (type)
    {
    case VAR_BOOL: return XR_ACTION_TYPE_BOOLEAN_INPUT;
    case VAR_FLOAT: return XR_ACTION_TYPE_FLOAT_INPUT;
    case VAR_VECTOR2: return XR_ACTION_TYPE_VECTOR2F_INPUT;
    case VAR_VECTOR3: return XR_ACTION_TYPE_POSE_INPUT;
    case VAR_MATRIX3X4: return XR_ACTION_TYPE_POSE_INPUT;
    case VAR_NONE: return XR_ACTION_TYPE_VIBRATION_OUTPUT;
    default: URHO3D_ASSERT(false); return XR_ACTION_TYPE_BOOLEAN_INPUT;
    }
}

EnumArray<XrPath, VRHand> GetHandPaths(XrInstance instance)
{
    EnumArray<XrPath, VRHand> handPaths{};
    xrStringToPath(instance, "/user/hand/left", &handPaths[VRHand::Left]);
    xrStringToPath(instance, "/user/hand/right", &handPaths[VRHand::Right]);
    return handPaths;
}

ea::pair<XrSpacePtr, XrSpacePtr> CreateActionSpaces(
    XrInstance instance, XrSession session, XrAction action, bool isHanded)
{
    XrActionSpaceCreateInfo spaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
    spaceInfo.action = action;
    spaceInfo.poseInActionSpace = xrPoseIdentity;

    if (!isHanded)
    {
        XrSpace space{};
        if (!URHO3D_CHECK_OPENXR(xrCreateActionSpace(session, &spaceInfo, &space)))
            return {};

        const auto wrappedSpace = XrSpacePtr(space, xrDestroySpace);
        return {wrappedSpace, wrappedSpace};
    }

    const auto handPaths = GetHandPaths(instance);

    XrSpace spaceLeft{};
    spaceInfo.subactionPath = handPaths[VRHand::Left];
    if (!URHO3D_CHECK_OPENXR(xrCreateActionSpace(session, &spaceInfo, &spaceLeft)))
        return {};
    const auto wrappedSpaceLeft = XrSpacePtr(spaceLeft, xrDestroySpace);

    XrSpace spaceRight{};
    spaceInfo.subactionPath = handPaths[VRHand::Right];
    if (!URHO3D_CHECK_OPENXR(xrCreateActionSpace(session, &spaceInfo, &spaceRight)))
        return {};
    const auto wrappedSpaceRight = XrSpacePtr(spaceRight, xrDestroySpace);

    return {wrappedSpaceLeft, wrappedSpaceRight};
}

ea::pair<SharedPtr<OpenXRBinding>, SharedPtr<OpenXRBinding>> CreateBinding(
    XrInstance instance, XrSession session, XrActionSet actionSet, XMLElement element)
{
    Context* context = Context::GetInstance();
    auto localization = context->GetSubsystem<Localization>();

    const auto handPaths = GetHandPaths(instance);

    const ea::string name = element.GetAttribute("name");
    const ea::string typeName = element.GetAttribute("type");
    const bool handed = element.GetBool("handed");

    // Create action
    XrActionCreateInfo createInfo = {XR_TYPE_ACTION_CREATE_INFO};
    XrPath customPath;

    if (handed)
    {
        createInfo.countSubactionPaths = 2;
        createInfo.subactionPaths = handPaths.data();
    }
    else if (element.HasAttribute("subaction"))
    {
        // User specified subaction path (originally for vive trackers), currently preferring fully specified paths in the manifest,
        // but a case where that isn't workable isn't unlikely to pop in the future, so support it ahead of time.
        xrStringToPath(instance, element.GetAttributeCString("subaction"), &customPath);
        createInfo.subactionPaths = &customPath;
    }

    const ea::string localizedName = localization->Get(name);
    strcpy_s(createInfo.actionName, 64, name.c_str());
    strcpy_s(createInfo.localizedActionName, 128, localizedName.c_str());

    const auto type = ParseBindingType(typeName);
    if (!type)
    {
        URHO3D_LOGERROR("Unknown XR action type '{}' for action '{}'", typeName, name);
        return {};
    }
    createInfo.actionType = ToActionType(*type);

    XrAction action{};
    if (!URHO3D_CHECK_OPENXR(xrCreateAction(actionSet, &createInfo, &action)))
        return {};
    const auto wrappedAction = XrActionPtr(action, xrDestroyAction);

    const bool needActionSpace = createInfo.actionType == XR_ACTION_TYPE_POSE_INPUT;
    const auto actionSpaces =
        needActionSpace ? CreateActionSpaces(instance, session, action, handed) : ea::pair<XrSpacePtr, XrSpacePtr>{};

    if (handed)
    {
        const bool isPose = element.GetBool("grip");
        const bool isAimPose = element.GetBool("aim");

        const auto bindingLeft = MakeShared<OpenXRBinding>(context, name, localizedName, //
            VRHand::Left, *type, isPose, isAimPose, actionSet, wrappedAction, handPaths[VRHand::Left],
            actionSpaces.first);
        const auto bindingRight = MakeShared<OpenXRBinding>(context, name, localizedName, //
            VRHand::Right, *type, isPose, isAimPose, actionSet, wrappedAction, handPaths[VRHand::Right],
            actionSpaces.second);

        return {bindingLeft, bindingRight};
    }
    else
    {
        const auto binding = MakeShared<OpenXRBinding>(context, name, localizedName, //
            VRHand::None, *type, false, false, actionSet, wrappedAction, XrPath{}, actionSpaces.first);
        return {binding, binding};
    }
}

void SuggestInteractionProfile(XrInstance instance, XMLElement element, OpenXRActionGroup* actionGroup)
{
    const ea::string device = element.GetAttribute("device");
    XrPath devicePath{};
    xrStringToPath(instance, device.c_str(), &devicePath);

    XrInteractionProfileSuggestedBinding suggest = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggest.interactionProfile = devicePath;

    ea::vector<XrActionSuggestedBinding> bindings;
    for (auto child = element.GetChild("bind"); child.NotNull(); child = child.GetNext("bind"))
    {
        ea::string action = child.GetAttribute("action");
        ea::string bindPathString = child.GetAttribute("path");

        XrPath bindPath;
        xrStringToPath(instance, bindPathString.c_str(), &bindPath);

        if (OpenXRBinding* binding = actionGroup->FindBindingImpl(action))
        {
            XrActionSuggestedBinding suggestedBinding{};
            suggestedBinding.action = binding->action_.Raw();
            suggestedBinding.binding = bindPath;
            bindings.push_back(suggestedBinding);
        }
    }

    if (!bindings.empty())
    {
        suggest.countSuggestedBindings = bindings.size();
        suggest.suggestedBindings = bindings.data();

        URHO3D_CHECK_OPENXR(xrSuggestInteractionProfileBindings(instance, &suggest));
    }
}

SharedPtr<OpenXRActionGroup> CreateActionGroup(
    XrInstance instance, XrSession session, XMLElement element, const StringVector& activeExtensions)
{
    Context* context = Context::GetInstance();
    auto localization = context->GetSubsystem<Localization>();

    const ea::string name = element.GetAttribute("name");
    const ea::string localizedName = localization->Get(name);

    XrActionSetCreateInfo createInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy(createInfo.actionSetName, name.c_str(), XR_MAX_ACTION_SET_NAME_SIZE);
    strncpy(createInfo.localizedActionSetName, localizedName.c_str(), XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);

    XrActionSet actionSet{};
    if (!URHO3D_CHECK_OPENXR(xrCreateActionSet(instance, &createInfo, &actionSet)))
        return nullptr;

    const auto wrappedActionSet = XrActionSetPtr(actionSet, xrDestroyActionSet);
    auto actionGroup = MakeShared<OpenXRActionGroup>(context, name, localizedName, wrappedActionSet);

    auto actionsElement = element.GetChild("actions");
    for (auto child = actionsElement.GetChild("action"); child.NotNull(); child = child.GetNext("action"))
    {
        const auto [bindingLeft, bindingRight] = CreateBinding(instance, session, actionSet, child);
        if (!bindingLeft || !bindingRight)
            return nullptr;

        actionGroup->AddBinding(bindingLeft);
        if (bindingLeft != bindingRight)
            actionGroup->AddBinding(bindingRight);
    }

    for (auto child = element.GetChild("profile"); child.NotNull(); child = child.GetNext("profile"))
    {
        const ea::string extension = child.GetAttribute("extension");
        if (!extension.empty() && !IsExtensionSupported(activeExtensions, extension.c_str()))
            continue;

        SuggestInteractionProfile(instance, child, actionGroup);
    }

    return actionGroup;
}

} // namespace

Texture2D* OpenXRSwapChain::AcquireImage()
{
    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    unsigned textureIndex{};
    if (!URHO3D_CHECK_OPENXR(xrAcquireSwapchainImage(swapChain_.Raw(), &acquireInfo, &textureIndex)))
        return nullptr;

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    if (!URHO3D_CHECK_OPENXR(xrWaitSwapchainImage(swapChain_.Raw(), &waitInfo)))
        return nullptr;

    return GetTexture(textureIndex);
}

void OpenXRSwapChain::ReleaseImage()
{
    XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    URHO3D_CHECK_OPENXR(xrReleaseSwapchainImage(swapChain_.Raw(), &releaseInfo));
}

OpenXRBinding::OpenXRBinding(Context* context, const ea::string& name, const ea::string& localizedName, VRHand hand,
    VariantType dataType, bool isPose, bool isAimPose, XrActionSet set, XrActionPtr action, XrPath subPath,
    XrSpacePtr actionSpace)
    : XRBinding(context, name, localizedName, hand, dataType, isPose, isAimPose)
    , action_(action)
    , set_(set)
    , subPath_(subPath)
    , actionSpace_(actionSpace)
{
}

void OpenXRBinding::Update(XrSession session, float scaleCorrection)
{
    if (!action_ || haptic_)
        return;

    XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = action_.Raw();
    getInfo.subactionPath = subPath_;

    switch (dataType_)
    {
    case VAR_BOOL:
    {
        XrActionStateBoolean boolState = {XR_TYPE_ACTION_STATE_BOOLEAN};
        if (URHO3D_CHECK_OPENXR(xrGetActionStateBoolean(session, &getInfo, &boolState)))
        {
            changed_ = boolState.changedSinceLastSync != XR_FALSE;
            active_ = boolState.isActive;
            storedData_ = boolState.currentState;
        }
        break;
    }
    case VAR_FLOAT:
    {
        XrActionStateFloat floatState = {XR_TYPE_ACTION_STATE_FLOAT};
        if (URHO3D_CHECK_OPENXR(xrGetActionStateFloat(session, &getInfo, &floatState)))
        {
            changed_ = floatState.changedSinceLastSync || !Equals(floatState.currentState, GetFloat());
            active_ = floatState.isActive;
            storedData_ = floatState.currentState;
        }
        break;
    }
    case VAR_VECTOR2:
    {
        XrActionStateVector2f vectorState = {XR_TYPE_ACTION_STATE_VECTOR2F};
        if (URHO3D_CHECK_OPENXR(xrGetActionStateVector2f(session, &getInfo, &vectorState)))
        {
            changed_ = vectorState.changedSinceLastSync;
            active_ = vectorState.isActive;
            storedData_ = Vector2{vectorState.currentState.x, vectorState.currentState.y};
        }
        break;
    }
    case VAR_VECTOR3:
    {
        XrActionStatePose poseState = {XR_TYPE_ACTION_STATE_POSE};
        if (URHO3D_CHECK_OPENXR(xrGetActionStatePose(session, &getInfo, &poseState)))
        {
            changed_ = true;
            active_ = poseState.isActive;
            storedData_ = ToVector3(location_.pose.position) * scaleCorrection;
            linearVelocity_ = ToVector3(velocity_.linearVelocity) * scaleCorrection;
        }
        break;
    }
    case VAR_MATRIX3X4:
    {
        XrActionStatePose poseState = {XR_TYPE_ACTION_STATE_POSE};
        if (URHO3D_CHECK_OPENXR(xrGetActionStatePose(session, &getInfo, &poseState)))
        {
            changed_ = true;
            active_ = poseState.isActive;
            storedData_ = ToMatrix3x4(location_.pose, scaleCorrection);
            linearVelocity_ = ToVector3(velocity_.linearVelocity) * scaleCorrection;
            angularVelocity_ = ToVector3(velocity_.angularVelocity) * scaleCorrection;
        }
        break;
    }
    default: URHO3D_ASSERT(false); break;
    }

    // Send events for changed bindings, except spatial bindings which effectively always change
    if (changed_ && dataType_ != VAR_VECTOR3 && dataType_ != VAR_MATRIX3X4)
    {
        VariantMap& eventData = GetEventDataMap();
        eventData[VRBindingChange::P_BINDING] = this;
        SendEvent(E_VRBINDINGCHANGED, eventData);
    }
}

void OpenXRBinding::UpdateBoundState(XrSession session)
{
    XrBoundSourcesForActionEnumerateInfo info = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
    info.action = action_.Raw();

    unsigned numSources = 0;
    xrEnumerateBoundSourcesForAction(session, &info, 0, &numSources, nullptr);

    isBound_ = numSources > 0;
}

OpenXRActionGroup::OpenXRActionGroup(
    Context* context, const ea::string& name, const ea::string& localizedName, XrActionSetPtr set)
    : XRActionGroup(context, name, localizedName)
    , actionSet_(set)
{
}

void OpenXRActionGroup::AddBinding(OpenXRBinding* binding)
{
    bindings_.emplace_back(binding);
}

OpenXRBinding* OpenXRActionGroup::FindBindingImpl(const ea::string& name)
{
    return static_cast<OpenXRBinding*>(XRActionGroup::FindBinding(name, VRHand::None));
}

void OpenXRActionGroup::AttachToSession(XrSession session)
{
    XrActionSet actionSets[] = {actionSet_.Raw()};

    XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.actionSets = actionSets;
    attachInfo.countActionSets = 1;
    xrAttachSessionActionSets(session, &attachInfo);
}

void OpenXRActionGroup::Synchronize(XrSession session)
{
    XrActiveActionSet activeSet = {};
    activeSet.actionSet = actionSet_.Raw();

    XrActionsSyncInfo sync = {XR_TYPE_ACTIONS_SYNC_INFO};
    sync.activeActionSets = &activeSet;
    sync.countActiveActionSets = 1;
    xrSyncActions(session, &sync);
}

OpenXRControllerModel::OpenXRControllerModel(Context* context, VRHand hand, XrInstance instance)
    : Object(context)
    , hand_(hand)
    , handPath_(GetHandPaths(instance)[hand])
{
}

void OpenXRControllerModel::UpdateModel(XrSession session)
{
    XrControllerModelKeyStateMSFT currentState{XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT};
    if (!URHO3D_CHECK_OPENXR(xrGetControllerModelKeyMSFT(session, handPath_, &currentState)))
        return;

    if (modelKey_ == currentState.modelKey)
        return;

    modelKey_ = currentState.modelKey;
    if (!modelKey_)
    {
        importer_ = nullptr;
        prefab_ = nullptr;
        return;
    }

    uint32_t dataSize = 0;
    if (!URHO3D_CHECK_OPENXR(xrLoadControllerModelMSFT(session, modelKey_, 0, &dataSize, nullptr)))
        return;

    ByteVector data;
    data.resize(dataSize);
    if (!URHO3D_CHECK_OPENXR(xrLoadControllerModelMSFT(session, modelKey_, dataSize, &dataSize, data.data())))
        return;

    XrControllerModelPropertiesMSFT properties{XR_TYPE_CONTROLLER_MODEL_PROPERTIES_MSFT};
    if (!URHO3D_CHECK_OPENXR(xrGetControllerModelPropertiesMSFT(session, modelKey_, &properties)))
        return;

    properties_.resize(
        properties.nodeCountOutput, XrControllerModelNodePropertiesMSFT{XR_TYPE_CONTROLLER_MODEL_NODE_PROPERTIES_MSFT});

    properties.nodeCapacityInput = properties_.size();
    properties.nodeProperties = properties_.data();
    if (!URHO3D_CHECK_OPENXR(xrGetControllerModelPropertiesMSFT(session, modelKey_, &properties)))
        return;

    GLTFImporterSettings settings;
    settings.gpuResources_ = true;
    settings.cleanupRootNodes_ = false;
    const auto importer = MakeShared<GLTFImporter>(context_, settings);
    if (!importer->LoadFileBinary(data))
        return;

    const ea::string folder = Format("manual://OpenXR/ControllerModel/{}/", hand_ == VRHand::Left ? "Left" : "Right");
    if (!importer->Process("", folder, nullptr))
        return;

    const auto cache = GetSubsystem<ResourceCache>();
    const auto prefab = cache->GetResource<PrefabResource>(folder + "Prefab.prefab");
    if (!prefab)
        return;

    importer_ = importer;
    prefab_ = prefab;
    cachedControllerNode_ = nullptr;
}

void OpenXRControllerModel::UpdateTransforms(XrSession session, Node* controllerNode)
{
    if (properties_.empty() || !prefab_)
        return;

    nodeStates_.resize(properties_.size(), XrControllerModelNodeStateMSFT{XR_TYPE_CONTROLLER_MODEL_NODE_STATE_MSFT});

    XrControllerModelStateMSFT state = {XR_TYPE_CONTROLLER_MODEL_STATE_MSFT};
    state.nodeCapacityInput = nodeStates_.size();
    state.nodeStates = nodeStates_.data();

    if (!URHO3D_CHECK_OPENXR(xrGetControllerModelStateMSFT(session, modelKey_, &state)))
        return;

    UpdateCachedNodes(controllerNode);
    for (unsigned i = 0; i < state.nodeCountOutput; ++i)
    {
        Node* node = cachedPropertyNodes_[i];
        if (!node)
            continue;

        const XrPosef& pose = nodeStates_[i].nodePose;
        const Vector3 sourcePosition{pose.position.x, pose.position.y, pose.position.z};
        const Quaternion sourceRotation{pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z};

        const Transform transform = importer_->ConvertTransform(Transform{sourcePosition, sourceRotation});
        node->SetTransform(transform);
    }
}

void OpenXRControllerModel::UpdateCachedNodes(Node* controllerNode)
{
    if (cachedControllerNode_ == controllerNode)
        return;

    cachedControllerNode_ = controllerNode;

    NodeCache cache;
    CacheNodeAndChildren(cache, controllerNode, controllerNode);

    const unsigned numProperties = properties_.size();
    cachedPropertyNodes_.resize(numProperties);
    for (unsigned i = 0; i < numProperties; ++i)
    {
        const XrControllerModelNodePropertiesMSFT& property = properties_[i];
        const auto key = ea::make_pair(StringHash{property.nodeName}, StringHash{property.parentNodeName});
        const auto it = cache.find(key);
        if (it != cache.end())
            cachedPropertyNodes_[i] = it->second;
    }
}

void OpenXRControllerModel::CacheNodeAndChildren(NodeCache& cache, Node* node, Node* rootNode) const
{
    const WeakPtr<Node> weakNode{node};
    const ea::string nodeName = node->GetName();
    const ea::string parentName = node != rootNode ? node->GetParent()->GetName() : "";

    const auto fullKey = ea::make_pair(StringHash{nodeName}, StringHash{parentName});
    const auto partialKey = ea::make_pair(StringHash{nodeName}, StringHash{""});

    cache.emplace(fullKey, weakNode);
    if (fullKey != partialKey)
        cache.emplace(partialKey, weakNode);

    for (Node* child : node->GetChildren())
        CacheNodeAndChildren(cache, child, rootNode);
}

OpenXRControllerModel::~OpenXRControllerModel()
{
}

OpenXR::OpenXR(Context* ctx)
    : BaseClassName(ctx)
{
    SubscribeToEvent(E_BEGINFRAME, &OpenXR::HandleBeginFrame);
    SubscribeToEvent(E_ENDRENDERING, &OpenXR::HandleEndRendering);
}

OpenXR::~OpenXR()
{
    // Do it manually so the VirtualReality and OpenXR members are destroyed in the right order.
    ShutdownSession();
}

bool OpenXR::InitializeSystem(RenderBackend backend)
{
    if (instance_)
    {
        URHO3D_LOGERROR("OpenXR is already initialized");
        return false;
    }

    InitializeOpenXRLoader();

    supportedExtensions_ = EnumerateExtensionsXR();
    if (!IsExtensionSupported(supportedExtensions_, GetBackendExtensionName(backend)))
    {
        URHO3D_LOGERROR("Renderer backend is not supported by OpenXR runtime");
        return false;
    }

    InitializeActiveExtensions(backend);

    auto engine = GetSubsystem<Engine>();
    const ea::string& engineName = "Rebel Fork of Urho3D";
    const ea::string& applicationName = engine->GetParameter(EP_APPLICATION_NAME).GetString();
    instance_ = CreateInstanceXR(activeExtensions_, engineName, applicationName);
    if (!instance_)
        return false;

    XrInstanceProperties instProps = {XR_TYPE_INSTANCE_PROPERTIES};
    if (xrGetInstanceProperties(instance_.Raw(), &instProps) == XR_SUCCESS)
        URHO3D_LOGINFO("OpenXR Runtime is: {} version 0x{:x}", instProps.runtimeName, instProps.runtimeVersion);

    if (features_.debugOutput_)
        debugMessenger_ = CreateDebugMessengerXR(instance_.Raw());

    const auto systemId = GetSystemXR(instance_.Raw());
    if (!systemId)
        return false;

    system_ = *systemId;
    systemName_ = GetSystemNameXR(instance_.Raw(), system_);

    const auto blendModes = GetBlendModesXR(instance_.Raw(), system_);
    if (blendModes.empty())
        return false;

    blendMode_ = blendModes[0];

    const auto viewConfigurations = GetViewConfigurationsXR(instance_.Raw(), system_);
    if (!viewConfigurations.contains(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO))
    {
        URHO3D_LOGERROR("Stereo rendering not supported on this device");
        return false;
    }

    const auto views = GetViewConfigurationViewsXR(instance_.Raw(), system_);
    if (!views)
        return false;

    recommendedMultiSample_ = (*views)[VREye::Left].recommendedSwapchainSampleCount;
    recommendedEyeTextureSize_.x_ =
        ea::min((*views)[VREye::Left].recommendedImageRectWidth, (*views)[VREye::Right].recommendedImageRectWidth);
    recommendedEyeTextureSize_.y_ =
        ea::min((*views)[VREye::Left].recommendedImageRectHeight, (*views)[VREye::Right].recommendedImageRectHeight);

    if (!InitializeTweaks(backend))
        return false;

    if (features_.controllerModel_)
    {
        for (const VRHand hand : {VRHand::Left, VRHand::Right})
            controllerModels_[hand] = MakeShared<OpenXRControllerModel>(context_, hand, instance_.Raw());
    }

    return true;
}

void OpenXR::InitializeActiveExtensions(RenderBackend backend)
{
    activeExtensions_ = {GetBackendExtensionName(backend)};

    features_.debugOutput_ = ActivateOptionalExtension( //
        activeExtensions_, supportedExtensions_, XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    features_.visibilityMask_ = ActivateOptionalExtension( //
        activeExtensions_, supportedExtensions_, XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);
    features_.controllerModel_ = ActivateOptionalExtension( //
        activeExtensions_, supportedExtensions_, XR_MSFT_CONTROLLER_MODEL_EXTENSION_NAME);
    features_.depthLayer_ = ActivateOptionalExtension(
        activeExtensions_, supportedExtensions_, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);

    // Controllers
    ActivateOptionalExtension(
        activeExtensions_, supportedExtensions_, XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME);
    ActivateOptionalExtension(
        activeExtensions_, supportedExtensions_, XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME);
    ActivateOptionalExtension( //
        activeExtensions_, supportedExtensions_, XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME);
    ActivateOptionalExtension( //
        activeExtensions_, supportedExtensions_, XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME);

    // Trackers
    ActivateOptionalExtension(
        activeExtensions_, supportedExtensions_, XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);

    for (const ea::string& extension : userExtensions_)
        ActivateOptionalExtension(activeExtensions_, supportedExtensions_, extension.c_str());
}

bool OpenXR::InitializeTweaks(RenderBackend backend)
{
    if (IsNativeOculusQuest2())
        tweaks_.orientation_ = ea::string{"LandscapeRight"};

#if VULKAN_SUPPORTED
    if (backend == RenderBackend::Vulkan)
    {
        tweaks_.vulkanInstanceExtensions_ = GetVulkanInstanceExtensionsXR(instance_.Raw(), system_);
        tweaks_.vulkanDeviceExtensions_ = GetVulkanDeviceExtensionsXR(instance_.Raw(), system_);

        // TODO: If we want to know required physical device ahead of time,
        // we should create dedicated OpenXR instance and system for this check.
        return true;
    }
#endif

    // SteamVR currently is reporting depth modes (D32_FLOAT) that it doesn't actually support as frame depth attachments
    // Expect to see something like "SteamVR / OpenXR : holographic" in system name
    // TODO: in the future when it's somewhat known what sort of other strange oddities like this exist
    //       coalesce them into something like a json overrides rules file like the graphics tweaks stuff.
    if (systemName_.contains("steamvr", false))
        features_.depthLayer_ = false;

    return true;
}

bool OpenXR::InitializeSession(const VRSessionParameters& params)
{
    auto cache = GetSubsystem<ResourceCache>();

    manifest_ = cache->GetResource<XMLFile>(params.manifestPath_);
    if (!manifest_)
    {
        URHO3D_LOGERROR("Unable to load OpenXR manifest '{}'", params.manifestPath_);
        return false;
    }

    multiSample_ = params.multiSample_ ? params.multiSample_ : recommendedMultiSample_;
    eyeTextureSize_ = VectorRoundToInt(recommendedEyeTextureSize_.ToVector2() * params.resolutionScale_);

    if (!OpenSession())
    {
        ShutdownSession();
        return false;
    }

    CreateDefaultRig();
    return true;
}

void OpenXR::ShutdownSession()
{
    controllerModels_ = {};
    handGrips_ = {};
    handAims_ = {};
    handHaptics_ = {};
    views_ = {{XR_TYPE_VIEW}};

    manifest_ = nullptr;
    actionSets_.clear();
    activeActionSet_ = nullptr;
    sessionState_ = {};

    swapChain_ = nullptr;
    depthChain_ = nullptr;

    headSpace_ = nullptr;
    viewSpace_ = nullptr;
    session_ = nullptr;
}

bool OpenXR::IsConnected() const
{
    return instance_ && session_;
}

bool OpenXR::IsRunning() const
{
    if (!IsConnected())
        return false;

    switch (sessionState_)
    {
    case XR_SESSION_STATE_READY:
    case XR_SESSION_STATE_SYNCHRONIZED:
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED: return true;

    default: return false;
    }
}

bool OpenXR::IsVisible() const
{
    if (!IsConnected())
        return false;

    switch (sessionState_)
    {
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED: return true;

    default: return false;
    }
}

bool OpenXR::IsFocused() const
{
    if (!IsConnected())
        return false;

    switch (sessionState_)
    {
    case XR_SESSION_STATE_FOCUSED: return true;

    default: return false;
    }
}

bool OpenXR::OpenSession()
{
    auto renderDevice = GetSubsystem<RenderDevice>();

    session_ = CreateSessionXR(renderDevice, instance_.Raw(), system_);
    if (!session_)
        return false;

    const auto [headSpace, isRoomScale] = CreateHeadSpaceXR(session_.Raw());
    headSpace_ = headSpace;
    isRoomScale_ = isRoomScale;
    viewSpace_ = CreateViewSpaceXR(session_.Raw());

    if (!headSpace_ || !viewSpace_)
        return false;

    if (manifest_)
        BindActions(manifest_);

    // if there's a default action set, then use it.
    VirtualReality::SetCurrentActionSet("default");

    // Create swap chains
    const auto internalFormats = GetSwapChainFormats(session_.Raw());
    const auto [colorFormat, colorFormatInternal] = SelectColorFormat(renderDevice->GetBackend(), internalFormats);
    const auto [depthFormat, depthFormatInternal] = SelectDepthFormat(renderDevice->GetBackend(), internalFormats);

    swapChain_ = CreateSwapChainXR(
        GetContext(), session_.Raw(), colorFormat, colorFormatInternal, eyeTextureSize_, multiSample_);
    if (!swapChain_)
        return false;

    if (features_.depthLayer_ && depthFormatInternal)
    {
        depthChain_ = CreateSwapChainXR(
            GetContext(), session_.Raw(), depthFormat, depthFormatInternal, eyeTextureSize_, multiSample_);
    }

    return true;
}

void OpenXR::PollEvents()
{
    XrEventDataBuffer eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(instance_.Raw(), &eventBuffer) == XR_SUCCESS)
    {
        switch (eventBuffer.type)
        {
        case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
        {
            // TODO: Implement visibility mask
            break;
        }
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
            // This state is not recoverable, so we need to exit.
            SendEvent(E_VREXIT);
            SendEvent(E_EXITREQUESTED);
            ShutdownSession();
            break;
        }
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
            UpdateBindingBound();
            SendEvent(E_VRINTERACTIONPROFILECHANGED);
            break;
        }
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            const auto& event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&eventBuffer);
            if (!UpdateSessionState(event.state))
                ShutdownSession();
            break;
        }
        default: break;
        }

        eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

bool OpenXR::UpdateSessionState(XrSessionState state)
{
    sessionState_ = state;

    switch (sessionState_)
    {
    case XR_SESSION_STATE_IDLE:
    {
        break;
    }
    case XR_SESSION_STATE_READY:
    {
        XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        if (!URHO3D_CHECK_OPENXR(xrBeginSession(session_.Raw(), &beginInfo)))
            return false;
        break;
    }
    case XR_SESSION_STATE_SYNCHRONIZED:
    {
        break;
    }
    case XR_SESSION_STATE_VISIBLE:
    {
        break;
    }
    case XR_SESSION_STATE_FOCUSED:
    {
        SendEvent(E_VRRESUME);
        break;
    }
    case XR_SESSION_STATE_STOPPING:
    {
        SendEvent(E_VRPAUSE);
        if (!URHO3D_CHECK_OPENXR(xrEndSession(session_.Raw())))
            return false;
        break;
    }
    case XR_SESSION_STATE_EXITING:
    case XR_SESSION_STATE_LOSS_PENDING:
    {
        SendEvent(E_VREXIT);
        break;
    }
    }

    return true;
}

void OpenXR::BeginFrame()
{
    XrFrameState frameState = {XR_TYPE_FRAME_STATE};
    xrWaitFrame(session_.Raw(), nullptr, &frameState);

    XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(session_.Raw(), &beginInfo);

    predictedTime_ = frameState.predictedDisplayTime;
}

void OpenXR::LocateViewsAndSpaces()
{
    // Head
    headLocation_.next = &headVelocity_;
    xrLocateSpace(viewSpace_.Raw(), headSpace_.Raw(), predictedTime_, &headLocation_);

    // All pose related actions will now need their locations updated.
    if (activeActionSet_)
    {
        for (auto binding : activeActionSet_->GetBindings())
        {
            OpenXRBinding* xrBind = binding->Cast<OpenXRBinding>();
            if (xrBind)
            {
                auto expected = xrBind->GetExpectedType();

                // Check if we're bound and we're presumed to be a pose type
                if (xrBind->IsBound() && (expected == VAR_MATRIX3X4 || expected == VAR_VECTOR3))
                    xrLocateSpace(xrBind->actionSpace_.Raw(), headSpace_.Raw(), predictedTime_, &xrBind->location_);
            }
        }
    }

    // Eyes
    XrViewLocateInfo viewInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    viewInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewInfo.space = headSpace_.Raw();
    viewInfo.displayTime = predictedTime_;

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    unsigned numViews = 0;
    xrLocateViews(session_.Raw(), &viewInfo, &viewState, 2, &numViews, views_.data());
}

void OpenXR::SynchronizeActions()
{
    if (!activeActionSet_)
        return;

    auto setImpl = static_cast<OpenXRActionGroup*>(activeActionSet_.Get());
    setImpl->Synchronize(session_.Raw());

    for (XRBinding* binding : activeActionSet_->GetBindings())
    {
        const auto bindingImpl = static_cast<OpenXRBinding*>(binding);
        bindingImpl->Update(session_.Raw(), scaleCorrection_);
    }
}

void OpenXR::HandleBeginFrame()
{
    if (!IsConnected())
        return;

    PollEvents();

    if (IsRunning())
    {
        BeginFrame();

        if (IsVisible())
        {
            AcquireSwapChainImages();
            LocateViewsAndSpaces();
            SynchronizeActions();

            ValidateCurrentRig();
            UpdateCurrentRig();
            UpdateHands();
        }
    }
}

void OpenXR::AcquireSwapChainImages()
{
    Texture2D* colorTexture = swapChain_->AcquireImage();
    if (colorTexture)
    {
        currentBackBufferColor_ = colorTexture;

        if (depthChain_)
        {
            Texture2D* depthTexture = depthChain_->AcquireImage();
            if (depthTexture)
            {
                currentBackBufferDepth_ = depthTexture;

                currentBackBufferColor_->GetRenderSurface()->SetLinkedDepthStencil(
                    currentBackBufferDepth_->GetRenderSurface());
            }
        }
    }
}

void OpenXR::ReleaseSwapChainImages()
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    renderDevice->GetImmediateContext()->Flush();

    swapChain_->ReleaseImage();
    if (depthChain_)
        depthChain_->ReleaseImage();
}

void OpenXR::LinkImagesToFrameInfo(XrFrameEndInfo& endInfo)
{
    // It's harmless but checking this will prevent early bad draws with null FOV.
    // XR eats the error, but handle it anyways to keep a clean output log.
    for (VREye eye : {VREye::Left, VREye::Right})
    {
        const XrFovf& fov = views_[eye].fov;
        if (fov.angleLeft == 0 || fov.angleRight == 0 || fov.angleUp == 0 || fov.angleDown == 0)
            return;
    }

    temp_.eyes_[VREye::Left].subImage.imageArrayIndex = 0;
    temp_.eyes_[VREye::Left].subImage.swapchain = swapChain_->GetHandle();
    temp_.eyes_[VREye::Left].subImage.imageRect = {{0, 0}, {eyeTextureSize_.x_, eyeTextureSize_.y_}};
    temp_.eyes_[VREye::Left].fov = views_[VREye::Left].fov;
    temp_.eyes_[VREye::Left].pose = views_[VREye::Left].pose;

    temp_.eyes_[VREye::Right].subImage.imageArrayIndex = 0;
    temp_.eyes_[VREye::Right].subImage.swapchain = swapChain_->GetHandle();
    temp_.eyes_[VREye::Right].subImage.imageRect = {{eyeTextureSize_.x_, 0}, {eyeTextureSize_.x_, eyeTextureSize_.y_}};
    temp_.eyes_[VREye::Right].fov = views_[VREye::Right].fov;
    temp_.eyes_[VREye::Right].pose = views_[VREye::Right].pose;

    if (depthChain_)
    {
        temp_.depth_[VREye::Left].subImage.imageArrayIndex = 0;
        temp_.depth_[VREye::Left].subImage.swapchain = depthChain_->GetHandle();
        temp_.depth_[VREye::Left].subImage.imageRect = {{0, 0}, {eyeTextureSize_.x_, eyeTextureSize_.y_}};
        temp_.depth_[VREye::Left].minDepth = 0.0f; // spec says range of 0-1, so doesn't respect GL -1 to 1?
        temp_.depth_[VREye::Left].maxDepth = 1.0f;
        temp_.depth_[VREye::Left].nearZ = rig_.nearDistance_;
        temp_.depth_[VREye::Left].farZ = rig_.farDistance_;

        temp_.depth_[VREye::Right].subImage.imageArrayIndex = 0;
        temp_.depth_[VREye::Right].subImage.swapchain = depthChain_->GetHandle();
        temp_.depth_[VREye::Right].subImage.imageRect = {
            {eyeTextureSize_.x_, 0}, {eyeTextureSize_.x_, eyeTextureSize_.y_}};
        temp_.depth_[VREye::Right].minDepth = 0.0f;
        temp_.depth_[VREye::Right].maxDepth = 1.0f;
        temp_.depth_[VREye::Right].nearZ = rig_.nearDistance_;
        temp_.depth_[VREye::Right].farZ = rig_.farDistance_;

        // These are chained to the relevant eye, not passed in through another mechanism.
        temp_.eyes_[VREye::Left].next = &temp_.depth_[VREye::Left];
        temp_.eyes_[VREye::Right].next = &temp_.depth_[VREye::Right];
    }
    else
    {
        temp_.eyes_[VREye::Left].next = nullptr;
        temp_.eyes_[VREye::Right].next = nullptr;
    }

    temp_.projectionLayer_.viewCount = 2;
    temp_.projectionLayer_.views = temp_.eyes_.data();
    temp_.projectionLayer_.space = headSpace_.Raw();

    temp_.layers_[0] = reinterpret_cast<XrCompositionLayerBaseHeader*>(&temp_.projectionLayer_);

    endInfo.layerCount = 1;
    endInfo.layers = temp_.layers_;
}

void OpenXR::EndFrame(XrFrameEndInfo& endInfo)
{
    endInfo.environmentBlendMode = blendMode_;
    endInfo.displayTime = predictedTime_;

    URHO3D_CHECK_OPENXR(xrEndFrame(session_.Raw(), &endInfo));
}

void OpenXR::HandleEndRendering()
{
    if (!IsConnected())
        return;

    if (IsRunning())
    {
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        if (IsVisible())
        {
            ReleaseSwapChainImages();
            LinkImagesToFrameInfo(endInfo);
        }

        EndFrame(endInfo);
    }
}

void OpenXR::BindActions(XMLFile* xmlFile)
{
    auto rootElement = xmlFile->GetRoot();
    for (auto child = rootElement.GetChild("actionset"); child.NotNull(); child = child.GetNext("actionset"))
    {
        auto actionGroup = CreateActionGroup(instance_.Raw(), session_.Raw(), child, activeExtensions_);
        actionSets_.insert({actionGroup->GetName(), actionGroup});
    }

    UpdateBindingBound();
}

void OpenXR::SetCurrentActionSet(SharedPtr<XRActionGroup> set)
{
    if (session_ && set != nullptr)
    {
        activeActionSet_ = set;

        const auto setImpl = static_cast<OpenXRActionGroup*>(set.Get());
        setImpl->AttachToSession(session_.Raw());
        UpdateBindingBound();
    }
}

void OpenXR::TriggerHaptic(VRHand hand, float durationSeconds, float cyclesPerSec, float amplitude)
{
    if (!activeActionSet_ || !IsFocused())
        return;

    for (XRBinding* binding : activeActionSet_->GetBindings())
    {
        if (!binding->IsHaptic() || binding->GetHand() != hand)
            continue;

        const auto bindingImpl = static_cast<OpenXRBinding*>(binding);

        XrHapticActionInfo info = {XR_TYPE_HAPTIC_ACTION_INFO};
        info.action = bindingImpl->action_.Raw();
        info.subactionPath = bindingImpl->subPath_;

        XrHapticVibration vibration = {XR_TYPE_HAPTIC_VIBRATION};
        vibration.amplitude = amplitude;
        vibration.frequency = cyclesPerSec;
        vibration.duration = durationSeconds * 1000.0f;

        xrApplyHapticFeedback(session_.Raw(), &info, reinterpret_cast<XrHapticBaseHeader*>(&vibration));
    }
}

Matrix3x4 OpenXR::GetHandTransform(VRHand hand) const
{
    if (hand == VRHand::None)
        return Matrix3x4();

    if (!handGrips_[hand])
        return Matrix3x4();

    auto q = ToQuaternion(handGrips_[hand]->location_.pose.orientation);
    auto v = ToVector3(handGrips_[hand]->location_.pose.position);

    // bring it into head space instead of stage space
    auto headInv = GetHeadTransform().Inverse();
    return headInv * Matrix3x4(v, q, 1.0f);
}

Matrix3x4 OpenXR::GetHandAimTransform(VRHand hand) const
{
    if (hand == VRHand::None)
        return Matrix3x4();

    if (!handAims_[hand])
        return Matrix3x4();

    // leave this in stage space, that's what we want
    auto q = ToQuaternion(handAims_[hand]->location_.pose.orientation);
    auto v = ToVector3(handAims_[hand]->location_.pose.position);
    return Matrix3x4(v, q, 1.0f);
}

Ray OpenXR::GetHandAimRay(VRHand hand) const
{
    if (hand == VRHand::None)
        return Ray();

    if (!handAims_[hand])
        return Ray();

    // leave this one is stage space, that's what we want
    auto q = ToQuaternion(handAims_[hand]->location_.pose.orientation);
    auto v = ToVector3(handAims_[hand]->location_.pose.position);
    return Ray(v, (q * Vector3(0, 0, 1)).Normalized());
}

void OpenXR::GetHandVelocity(VRHand hand, Vector3* linear, Vector3* angular) const
{
    if (hand == VRHand::None)
        return;

    if (!handGrips_[hand])
        return;

    if (linear && handGrips_[hand]->velocity_.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
        *linear = ToVector3(handGrips_[hand]->velocity_.linearVelocity);
    if (angular && handGrips_[hand]->velocity_.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
        *angular = ToVector3(handGrips_[hand]->velocity_.angularVelocity);
}

void OpenXR::UpdateHands()
{
    if (!rig_.IsValid())
        return;

    // Check for changes in controller model state, if so, do reload as required.
    UpdateControllerModels();

    Node* leftHand = rig_.leftHandPose_;
    Node* rightHand = rig_.rightHandPose_;
    Node* leftAim = rig_.leftHandAim_;
    Node* rightAim = rig_.rightHandAim_;

    // we need valid handles for these guys
    if (handGrips_[VRHand::Left] && handGrips_[VRHand::Right])
    {
        // TODO: can we do any tracking of our own such as using QEF for tracking recent velocity integration into
        // position confidence over the past interval of time to decide how much we trust integrating velocity when
        // position has no-confidence / untracked. May be able to fall-off a confidence factor provided the incoming
        // velocity is still there, problem is how to rectify when tracking kicks back in again later. If velocity
        // integration is valid there should be no issue - neither a pop, it'll already pop in a normal position
        // tracking lost recovery situation anyways.

        const Quaternion leftRotation = ToQuaternion(handGrips_[VRHand::Left]->location_.pose.orientation);
        const Vector3 leftPosition = ToVector3(handGrips_[VRHand::Left]->location_.pose.position);

        // these fields are super important to rationalize what's happened between sample points
        // sensor reads are effectively Planck timing it between quantum space-time
        leftHand->SetVar("PreviousTransformLocal", leftHand->GetTransformMatrix());
        leftHand->SetVar("PreviousTransformWorld", leftHand->GetWorldTransform());
        leftHand->SetEnabled(handGrips_[VRHand::Left]->location_.locationFlags
            & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT));
        leftHand->SetPosition(leftPosition);
        if (handGrips_[VRHand::Left]->location_.locationFlags
            & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))
        {
            leftHand->SetRotation(leftRotation);
        }

        const Quaternion rightRotation = ToQuaternion(handGrips_[VRHand::Right]->location_.pose.orientation);
        const Vector3 rightPosition = ToVector3(handGrips_[VRHand::Right]->location_.pose.position);

        rightHand->SetVar("PreviousTransformLocal", leftHand->GetTransformMatrix());
        rightHand->SetVar("PreviousTransformWorld", leftHand->GetWorldTransform());
        rightHand->SetEnabled(handGrips_[VRHand::Right]->location_.locationFlags
            & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT));
        rightHand->SetPosition(rightPosition);
        if (handGrips_[VRHand::Right]->location_.locationFlags
            & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))
        {
            rightHand->SetRotation(rightRotation);
        }

        // Setup aim nodes too
        leftAim->SetTransformMatrix(GetHandAimTransform(VRHand::Left));
        rightAim->SetTransformMatrix(GetHandAimTransform(VRHand::Right));
    }
}

void OpenXR::UpdateControllerModels()
{
    if (!features_.controllerModel_)
        return;

    for (const VRHand hand : {VRHand::Left, VRHand::Right})
        controllerModels_[hand]->UpdateModel(session_.Raw());

    if (rig_.leftController_)
        UpdateControllerModel(VRHand::Left, rig_.leftController_);

    if (rig_.rightController_)
        UpdateControllerModel(VRHand::Right, rig_.rightController_);
}

void OpenXR::UpdateControllerModel(VRHand hand, Node* instanceNode)
{
    OpenXRControllerModel* model = controllerModels_[hand];
    auto prefabReference = instanceNode->GetOrCreateComponent<PrefabReference>();

    if (prefabReference->GetPrefab() != model->GetPrefab())
    {
        prefabReference->SetPrefab(model->GetPrefab());

        VariantMap& eventData = GetEventDataMap();
        eventData[VRControllerChange::P_HAND] = static_cast<int>(hand);
        SendEvent(E_VRCONTROLLERCHANGE, eventData);
    }

    instanceNode->SetRotation(Quaternion{180.0f, Vector3::UP});
    model->UpdateTransforms(session_.Raw(), instanceNode);
}

Matrix3x4 OpenXR::GetEyeLocalTransform(VREye eye) const
{
    // TODO: fixme, why is view space not correct xrLocateViews( view-space )
    // one would expect them to be in head relative local space already ... but they're ... not?
    return GetHeadTransform().Inverse() * ToMatrix3x4(views_[eye].pose, scaleCorrection_);
}

Matrix4 OpenXR::GetProjection(VREye eye, float nearDist, float farDist) const
{
    return ToProjectionMatrix(nearDist, farDist, views_[eye].fov);
}

Matrix3x4 OpenXR::GetHeadTransform() const
{
    return ToMatrix3x4(headLocation_.pose, scaleCorrection_);
}

void OpenXR::UpdateBindingBound()
{
    if (!session_)
        return;

    if (activeActionSet_)
    {
        for (XRBinding* binding : activeActionSet_->GetBindings())
        {
            const auto bindingImpl = static_cast<OpenXRBinding*>(binding);
            bindingImpl->UpdateBoundState(session_.Raw());

            if (binding->IsGripPose())
                handGrips_[binding->GetHand()] = bindingImpl;
            if (binding->IsAimPose())
                handAims_[binding->GetHand()] = bindingImpl;
        }
    }
}

} // namespace Urho3D
