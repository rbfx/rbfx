// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/XR/OpenXRAPI.h"

#include "Urho3D/IO/Log.h"

#include <EASTL/unordered_map.h>

#include <SDL.h>

#if URHO3D_PLATFORM_ANDROID
    #include <dlfcn.h>
#endif

#include "Urho3D/DebugNew.h"

#define URHO3D_DEFINE_OPENXR_API(fn) PFN_##fn fn;
#define URHO3D_LOAD_OPENXR_API(fn) xrGetInstanceProcAddrFn(instance, #fn, (PFN_xrVoidFunction*)&fn);
#define URHO3D_UNLOAD_OPENXR_API(fn) fn = nullptr;

namespace Urho3D
{

namespace
{

PFN_xrGetInstanceProcAddr xrGetInstanceProcAddrFn = nullptr;

}

URHO3D_ENUMERATE_OPENXR_API_LOADER(URHO3D_DEFINE_OPENXR_API)
URHO3D_ENUMERATE_OPENXR_API(URHO3D_DEFINE_OPENXR_API)

const char* xrGetErrorStr(XrResult result)
{
    // clang-format off
#define DEFINE_ERROR(code) { code, #code }
    // clang-format on

    static const ea::unordered_map<int, const char*> xrErrorNames = {
        DEFINE_ERROR(XR_SUCCESS),
        DEFINE_ERROR(XR_TIMEOUT_EXPIRED),
        DEFINE_ERROR(XR_SESSION_LOSS_PENDING),
        DEFINE_ERROR(XR_EVENT_UNAVAILABLE),
        DEFINE_ERROR(XR_SPACE_BOUNDS_UNAVAILABLE),
        DEFINE_ERROR(XR_SESSION_NOT_FOCUSED),
        DEFINE_ERROR(XR_FRAME_DISCARDED),
        DEFINE_ERROR(XR_ERROR_VALIDATION_FAILURE),
        DEFINE_ERROR(XR_ERROR_RUNTIME_FAILURE),
        DEFINE_ERROR(XR_ERROR_OUT_OF_MEMORY),
        DEFINE_ERROR(XR_ERROR_API_VERSION_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_INITIALIZATION_FAILED),
        DEFINE_ERROR(XR_ERROR_FUNCTION_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_FEATURE_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_EXTENSION_NOT_PRESENT),
        DEFINE_ERROR(XR_ERROR_LIMIT_REACHED),
        DEFINE_ERROR(XR_ERROR_SIZE_INSUFFICIENT),
        DEFINE_ERROR(XR_ERROR_HANDLE_INVALID),
        DEFINE_ERROR(XR_ERROR_INSTANCE_LOST),
        DEFINE_ERROR(XR_ERROR_SESSION_RUNNING),
        DEFINE_ERROR(XR_ERROR_SESSION_NOT_RUNNING),
        DEFINE_ERROR(XR_ERROR_SESSION_LOST),
        DEFINE_ERROR(XR_ERROR_SYSTEM_INVALID),
        DEFINE_ERROR(XR_ERROR_PATH_INVALID),
        DEFINE_ERROR(XR_ERROR_PATH_COUNT_EXCEEDED),
        DEFINE_ERROR(XR_ERROR_PATH_FORMAT_INVALID),
        DEFINE_ERROR(XR_ERROR_PATH_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_LAYER_INVALID),
        DEFINE_ERROR(XR_ERROR_LAYER_LIMIT_EXCEEDED),
        DEFINE_ERROR(XR_ERROR_SWAPCHAIN_RECT_INVALID),
        DEFINE_ERROR(XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_ACTION_TYPE_MISMATCH),
        DEFINE_ERROR(XR_ERROR_SESSION_NOT_READY),
        DEFINE_ERROR(XR_ERROR_SESSION_NOT_STOPPING),
        DEFINE_ERROR(XR_ERROR_TIME_INVALID),
        DEFINE_ERROR(XR_ERROR_REFERENCE_SPACE_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_FILE_ACCESS_ERROR),
        DEFINE_ERROR(XR_ERROR_FILE_CONTENTS_INVALID),
        DEFINE_ERROR(XR_ERROR_FORM_FACTOR_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_FORM_FACTOR_UNAVAILABLE),
        DEFINE_ERROR(XR_ERROR_API_LAYER_NOT_PRESENT),
        DEFINE_ERROR(XR_ERROR_CALL_ORDER_INVALID),
        DEFINE_ERROR(XR_ERROR_GRAPHICS_DEVICE_INVALID),
        DEFINE_ERROR(XR_ERROR_POSE_INVALID),
        DEFINE_ERROR(XR_ERROR_INDEX_OUT_OF_RANGE),
        DEFINE_ERROR(XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED),
        DEFINE_ERROR(XR_ERROR_NAME_DUPLICATED),
        DEFINE_ERROR(XR_ERROR_NAME_INVALID),
        DEFINE_ERROR(XR_ERROR_ACTIONSET_NOT_ATTACHED),
        DEFINE_ERROR(XR_ERROR_ACTIONSETS_ALREADY_ATTACHED),
        DEFINE_ERROR(XR_ERROR_LOCALIZED_NAME_DUPLICATED),
        DEFINE_ERROR(XR_ERROR_LOCALIZED_NAME_INVALID),
        DEFINE_ERROR(XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING),
        DEFINE_ERROR(XR_ERROR_RUNTIME_UNAVAILABLE),
        DEFINE_ERROR(XR_ERROR_ANDROID_THREAD_SETTINGS_ID_INVALID_KHR),
        DEFINE_ERROR(XR_ERROR_ANDROID_THREAD_SETTINGS_FAILURE_KHR),
        DEFINE_ERROR(XR_ERROR_CREATE_SPATIAL_ANCHOR_FAILED_MSFT),
        DEFINE_ERROR(XR_ERROR_SECONDARY_VIEW_CONFIGURATION_TYPE_NOT_ENABLED_MSFT),
        DEFINE_ERROR(XR_ERROR_CONTROLLER_MODEL_KEY_INVALID_MSFT),
        DEFINE_ERROR(XR_ERROR_REPROJECTION_MODE_UNSUPPORTED_MSFT),
        DEFINE_ERROR(XR_ERROR_COMPUTE_NEW_SCENE_NOT_COMPLETED_MSFT),
        DEFINE_ERROR(XR_ERROR_SCENE_COMPONENT_ID_INVALID_MSFT),
        DEFINE_ERROR(XR_ERROR_SCENE_COMPONENT_TYPE_MISMATCH_MSFT),
        DEFINE_ERROR(XR_ERROR_SCENE_MESH_BUFFER_ID_INVALID_MSFT),
        DEFINE_ERROR(XR_ERROR_SCENE_COMPUTE_FEATURE_INCOMPATIBLE_MSFT),
        DEFINE_ERROR(XR_ERROR_SCENE_COMPUTE_CONSISTENCY_MISMATCH_MSFT),
        DEFINE_ERROR(XR_ERROR_DISPLAY_REFRESH_RATE_UNSUPPORTED_FB),
        DEFINE_ERROR(XR_ERROR_COLOR_SPACE_UNSUPPORTED_FB),
        DEFINE_ERROR(XR_ERROR_SPATIAL_ANCHOR_NAME_NOT_FOUND_MSFT),
        DEFINE_ERROR(XR_ERROR_SPATIAL_ANCHOR_NAME_INVALID_MSFT),
    };
#undef DEFINE_ERROR

    const auto iter = xrErrorNames.find(result);
    if (iter != xrErrorNames.end())
        return iter->second;
    return "Unknown";
}

bool xrCheckResult(XrResult result, const char* expr, const char* file, int line, const char* func)
{
    if (result == XrResult::XR_SUCCESS)
        return true;

    URHO3D_LOGERROR(
        "OpenXR error {}\nexpr: {}\nfile: {}\nline: {}\nfunc: {}", xrGetErrorStr(result), expr, file, line, func);
    return false;
}

void InitializeOpenXRLoader()
{
#if URHO3D_PLATFORM_ANDROID
    void* handle = dlopen("libopenxr_loader.so", RTLD_LAZY);
    xrGetInstanceProcAddrFn = (PFN_xrGetInstanceProcAddr)dlsym(handle, "xrGetInstanceProcAddr");
#else
    xrGetInstanceProcAddrFn = &xrGetInstanceProcAddr;
#endif

    XrInstance instance{};
    URHO3D_ENUMERATE_OPENXR_API_LOADER(URHO3D_LOAD_OPENXR_API)

#if URHO3D_PLATFORM_ANDROID
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    JavaVM* vm = nullptr;
    env->GetJavaVM(&vm);

    XrLoaderInitInfoAndroidKHR loaderInitInfo = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInitInfo.applicationVM = vm;
    loaderInitInfo.applicationContext = SDL_AndroidGetActivity();
    xrInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfo);
#endif
}

void LoadOpenXRAPI(XrInstance instance)
{
    URHO3D_ENUMERATE_OPENXR_API(URHO3D_LOAD_OPENXR_API)
}

void UnloadOpenXRAPI()
{
    URHO3D_ENUMERATE_OPENXR_API(URHO3D_UNLOAD_OPENXR_API);
}

Matrix4 ToProjectionMatrix(
    float nearZ, float farZ, float angleLeft, float angleTop, float angleRight, float angleBottom)
{
    const float tanLeft = tanf(angleLeft);
    const float tanRight = tanf(angleRight);
    const float tanDown = tanf(angleBottom);
    const float tanUp = tanf(angleTop);
    const float tanAngleWidth = tanRight - tanLeft;
    const float tanAngleHeight = tanUp - tanDown;
    const float q = farZ / (farZ - nearZ);
    const float r = -q * nearZ;

    Matrix4 projection = Matrix4::ZERO;
    projection.m00_ = 2 / tanAngleWidth;
    projection.m11_ = 2 / tanAngleHeight;

    projection.m02_ = -(tanRight + tanLeft) / tanAngleWidth;
    projection.m12_ = -(tanUp + tanDown) / tanAngleHeight;

    projection.m22_ = q;
    projection.m23_ = r;
    projection.m32_ = 1.0f;
    return projection;
}

Matrix4 ToProjectionMatrix(float nearZ, float farZ, XrFovf fov)
{
    return ToProjectionMatrix(nearZ, farZ, fov.angleLeft, fov.angleUp, fov.angleRight, fov.angleDown);
}

} // namespace Urho3D
