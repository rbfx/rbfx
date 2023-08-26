// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/Matrix3x4.h"
#include "Urho3D/Math/Matrix4.h"
#include "Urho3D/Math/Quaternion.h"
#include "Urho3D/Math/Vector3.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
    #include <EGL/egl.h>
#endif

#ifdef XR_USE_PLATFORM_ANDROID
    #include <jni.h>
#endif

#include <ThirdParty/OpenXRSDK/include/openxr/openxr.h>
#include <ThirdParty/OpenXRSDK/include/openxr/openxr_platform.h>

namespace Urho3D
{

#define URHO3D_ENUMERATE_OPENXR_API_LOADER(X) \
    X(xrEnumerateInstanceExtensionProperties) \
    X(xrEnumerateApiLayerProperties) \
    X(xrCreateInstance) \
    X(xrInitializeLoaderKHR)

#define URHO3D_ENUMERATE_OPENXR_API_CORE(X) \
    X(xrDestroyInstance) \
    X(xrGetInstanceProperties) \
    X(xrEnumerateEnvironmentBlendModes) \
    X(xrEnumerateSwapchainFormats) \
    X(xrEnumerateBoundSourcesForAction) \
    X(xrGetActionStatePose) \
    X(xrPollEvent) \
    X(xrResultToString) \
    X(xrGetSystem) \
    X(xrGetSystemProperties) \
    X(xrCreateSession) \
    X(xrDestroySession) \
    X(xrCreateReferenceSpace) \
    X(xrGetReferenceSpaceBoundsRect) \
    X(xrCreateActionSpace) \
    X(xrLocateSpace) \
    X(xrDestroySpace) \
    X(xrEnumerateViewConfigurations) \
    X(xrEnumerateViewConfigurationViews) \
    X(xrCreateSwapchain) \
    X(xrDestroySwapchain) \
    X(xrEnumerateSwapchainImages) \
    X(xrAcquireSwapchainImage) \
    X(xrWaitSwapchainImage) \
    X(xrReleaseSwapchainImage) \
    X(xrBeginSession) \
    X(xrEndSession) \
    X(xrWaitFrame) \
    X(xrBeginFrame) \
    X(xrEndFrame) \
    X(xrLocateViews) \
    X(xrStringToPath) \
    X(xrCreateActionSet) \
    X(xrDestroyActionSet) \
    X(xrCreateAction) \
    X(xrDestroyAction) \
    X(xrSuggestInteractionProfileBindings) \
    X(xrAttachSessionActionSets) \
    X(xrGetActionStateBoolean) \
    X(xrGetActionStateFloat) \
    X(xrGetActionStateVector2f) \
    X(xrSyncActions) \
    X(xrApplyHapticFeedback) \
    X(xrCreateHandTrackerEXT) \
    X(xrDestroyHandTrackerEXT) \
    X(xrLocateHandJointsEXT) \
    X(xrGetVisibilityMaskKHR) \
    X(xrCreateDebugUtilsMessengerEXT) \
    X(xrDestroyDebugUtilsMessengerEXT)

#define URHO3D_ENUMERATE_OPENXR_API_EXT(X) \
    X(xrLoadControllerModelMSFT) \
    X(xrGetControllerModelKeyMSFT) \
    X(xrGetControllerModelStateMSFT) \
    X(xrGetControllerModelPropertiesMSFT)

// clang-format off
#if D3D11_SUPPORTED
    #define URHO3D_ENUMERATE_OPENXR_API_D3D11(X) \
        X(xrGetD3D11GraphicsRequirementsKHR)
#else
    #define URHO3D_ENUMERATE_OPENXR_API_D3D11(X)
#endif

#if D3D12_SUPPORTED
    #define URHO3D_ENUMERATE_OPENXR_API_D3D12(X) \
        X(xrGetD3D12GraphicsRequirementsKHR)
#else
    #define URHO3D_ENUMERATE_OPENXR_API_D3D12(X)
#endif

#if VULKAN_SUPPORTED
    #define URHO3D_ENUMERATE_OPENXR_API_VULKAN(X) \
        X(xrGetVulkanInstanceExtensionsKHR) \
        X(xrGetVulkanDeviceExtensionsKHR) \
        X(xrGetVulkanGraphicsDeviceKHR) \
        X(xrGetVulkanGraphicsRequirementsKHR)
#else
    #define URHO3D_ENUMERATE_OPENXR_API_VULKAN(X)
#endif

#if GL_SUPPORTED
    #define URHO3D_ENUMERATE_OPENXR_API_GL(X) \
        X(xrGetOpenGLGraphicsRequirementsKHR)
#else
    #define URHO3D_ENUMERATE_OPENXR_API_GL(X)
#endif

#if GLES_SUPPORTED
    #define URHO3D_ENUMERATE_OPENXR_API_GLES(X) \
        X(xrGetOpenGLESGraphicsRequirementsKHR)
#else
    #define URHO3D_ENUMERATE_OPENXR_API_GLES(X)
#endif
// clang-format on

#define URHO3D_ENUMERATE_OPENXR_API(X) \
    URHO3D_ENUMERATE_OPENXR_API_CORE(X) \
    URHO3D_ENUMERATE_OPENXR_API_EXT(X) \
    URHO3D_ENUMERATE_OPENXR_API_D3D11(X) \
    URHO3D_ENUMERATE_OPENXR_API_D3D12(X) \
    URHO3D_ENUMERATE_OPENXR_API_VULKAN(X) \
    URHO3D_ENUMERATE_OPENXR_API_GL(X) \
    URHO3D_ENUMERATE_OPENXR_API_GLES(X)

#define URHO3D_DECLARE_OPENXR_API(fn) extern PFN_##fn fn;

URHO3D_ENUMERATE_OPENXR_API_LOADER(URHO3D_DECLARE_OPENXR_API)
URHO3D_ENUMERATE_OPENXR_API(URHO3D_DECLARE_OPENXR_API)

const char* xrGetErrorStr(XrResult result);
bool xrCheckResult(XrResult result, const char* expr, const char* file, int line, const char* func);

#define URHO3D_CHECK_OPENXR(expr) xrCheckResult(expr, #expr, __FILE__, __LINE__, __FUNCTION__)

/// Initialize OpenXR Loader API.
void InitializeOpenXRLoader();
/// Initialize OpenXR API functions from given instance.
void LoadOpenXRAPI(XrInstance instance);
/// Reset OpenXR API functions.
void UnloadOpenXRAPI();

static const XrPosef xrPoseIdentity = {{0, 0, 0, 1}, {0, 0, 0}};

/// Cast from OpenXR vector to engine format.
inline Vector3 ToVector3(XrVector3f v)
{
    return Vector3(v.x, v.y, -v.z);
}

/// Cast from OpenXR quaternion to engine format.
inline Quaternion ToQuaternion(XrQuaternionf q)
{
    Quaternion out;
    out.x_ = -q.x;
    out.y_ = -q.y;
    out.z_ = q.z;
    out.w_ = q.w;
    return out;
}

/// Cast from OpenXR pose to engine format.
inline Matrix3x4 ToMatrix3x4(XrPosef pose, float scale)
{
    return Matrix3x4(ToVector3(pose.position), ToQuaternion(pose.orientation), scale);
}

/// Calculate projection matrix from angles.
/// TODO: Move to Math
Matrix4 ToProjectionMatrix(
    float nearZ, float farZ, float angleLeft, float angleTop, float angleRight, float angleBottom);
Matrix4 ToProjectionMatrix(float nearZ, float farZ, XrFovf fov);

} // namespace Urho3D
