//
// Copyright (c) 2022 the RBFX project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../XR/XR.h"

#include "../Graphics/AnimatedModel.h"
#include "../Core/CoreEvents.h"
#include "../Engine/Engine.h"
#include "../IO/File.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/IndexBuffer.h"
#include "../Resource/Localization.h"
#include "../IO/Log.h"
#include "../Graphics/Material.h"
#include "../IO/MemoryBuffer.h"
#include "../Scene/Node.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/VertexBuffer.h"
#include "../XR/VREvents.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"

#include "../DebugNew.h"

// need this for loading the GLBs
#include <ThirdParty/tinygltf/tiny_gltf.h>

#include <iostream>

#ifdef URHO3D_D3D9
    #error D3D9 is not supported for openXR
#endif

#ifdef URHO3D_D3D11
    #include "../Graphics/Direct3D11/D3D11GraphicsImpl.h"
    #include <d3d11.h>
#endif

#ifdef URHO3D_OPENGL
    #include "../Graphics/OpenGL/OGLGraphicsImpl.h"
    #ifdef WIN32
        #include <Unknwn.h>
    #endif
#endif

#include <ThirdParty/OpenXRSDK/include/openxr/openxr_platform_defines.h>
#include <ThirdParty/OpenXRSDK/include/openxr/openxr_platform.h>

namespace Urho3D
{

SharedPtr<Node> LoadGLTFModel(Context* ctx, tinygltf::Model& model);

const XrPosef xrPoseIdentity = { {0,0,0,1}, {0,0,0} };

#define XR_INIT_TYPE(D, T) for (auto& a : D) a.type = T

#define XR_FOREACH(X)\
  X(xrDestroyInstance)\
  X(xrPollEvent)\
  X(xrResultToString)\
  X(xrGetSystem)\
  X(xrGetSystemProperties)\
  X(xrCreateSession)\
  X(xrDestroySession)\
  X(xrCreateReferenceSpace)\
  X(xrGetReferenceSpaceBoundsRect)\
  X(xrCreateActionSpace)\
  X(xrLocateSpace)\
  X(xrDestroySpace)\
  X(xrEnumerateViewConfigurations)\
  X(xrEnumerateViewConfigurationViews)\
  X(xrCreateSwapchain)\
  X(xrDestroySwapchain)\
  X(xrEnumerateSwapchainImages)\
  X(xrAcquireSwapchainImage)\
  X(xrWaitSwapchainImage)\
  X(xrReleaseSwapchainImage)\
  X(xrBeginSession)\
  X(xrEndSession)\
  X(xrWaitFrame)\
  X(xrBeginFrame)\
  X(xrEndFrame)\
  X(xrLocateViews)\
  X(xrStringToPath)\
  X(xrCreateActionSet)\
  X(xrDestroyActionSet)\
  X(xrCreateAction)\
  X(xrDestroyAction)\
  X(xrSuggestInteractionProfileBindings)\
  X(xrAttachSessionActionSets)\
  X(xrGetActionStateBoolean)\
  X(xrGetActionStateFloat)\
  X(xrGetActionStateVector2f)\
  X(xrSyncActions)\
  X(xrApplyHapticFeedback)\
  X(xrCreateHandTrackerEXT)\
  X(xrDestroyHandTrackerEXT)\
  X(xrLocateHandJointsEXT) \
  X(xrGetVisibilityMaskKHR) \
  X(xrCreateDebugUtilsMessengerEXT) \
  X(xrDestroyDebugUtilsMessengerEXT)

#ifdef URHO3D_D3D11
    #define XR_PLATFORM(X) \
        X(xrGetD3D11GraphicsRequirementsKHR)
#elif URHO3D_OPENGL
    #define XR_PLATFORM(X) \
        X(xrGetOpenGLGraphicsRequirementsKHR)
#endif

#define XR_EXTENSION_BASED(X) \
    X(xrLoadControllerModelMSFT) \
    X(xrGetControllerModelKeyMSFT) \
    X(xrGetControllerModelStateMSFT) \
    X(xrGetControllerModelPropertiesMSFT)

#define XR_DECLARE(fn) static PFN_##fn fn;
#define XR_LOAD(fn) xrGetInstanceProcAddr(instance_, #fn, (PFN_xrVoidFunction*) &fn);

    XR_FOREACH(XR_DECLARE)
    XR_PLATFORM(XR_DECLARE)
    XR_EXTENSION_BASED(XR_DECLARE)

#define XR_ERRNAME(ENUM) { ENUM, #ENUM },

        static ea::map<int, const char*> xrErrorNames = {
            XR_ERRNAME(XR_SUCCESS)
            XR_ERRNAME(XR_TIMEOUT_EXPIRED)
            XR_ERRNAME(XR_SESSION_LOSS_PENDING)
            XR_ERRNAME(XR_EVENT_UNAVAILABLE)
            XR_ERRNAME(XR_SPACE_BOUNDS_UNAVAILABLE)
            XR_ERRNAME(XR_SESSION_NOT_FOCUSED)
            XR_ERRNAME(XR_FRAME_DISCARDED)
            XR_ERRNAME(XR_ERROR_VALIDATION_FAILURE)
            XR_ERRNAME(XR_ERROR_RUNTIME_FAILURE)
            XR_ERRNAME(XR_ERROR_OUT_OF_MEMORY)
            XR_ERRNAME(XR_ERROR_API_VERSION_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_INITIALIZATION_FAILED)
            XR_ERRNAME(XR_ERROR_FUNCTION_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_FEATURE_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_EXTENSION_NOT_PRESENT)
            XR_ERRNAME(XR_ERROR_LIMIT_REACHED)
            XR_ERRNAME(XR_ERROR_SIZE_INSUFFICIENT)
            XR_ERRNAME(XR_ERROR_HANDLE_INVALID)
            XR_ERRNAME(XR_ERROR_INSTANCE_LOST)
            XR_ERRNAME(XR_ERROR_SESSION_RUNNING)
            XR_ERRNAME(XR_ERROR_SESSION_NOT_RUNNING)
            XR_ERRNAME(XR_ERROR_SESSION_LOST)
            XR_ERRNAME(XR_ERROR_SYSTEM_INVALID)
            XR_ERRNAME(XR_ERROR_PATH_INVALID)
            XR_ERRNAME(XR_ERROR_PATH_COUNT_EXCEEDED)
            XR_ERRNAME(XR_ERROR_PATH_FORMAT_INVALID)
            XR_ERRNAME(XR_ERROR_PATH_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_LAYER_INVALID)
            XR_ERRNAME(XR_ERROR_LAYER_LIMIT_EXCEEDED)
            XR_ERRNAME(XR_ERROR_SWAPCHAIN_RECT_INVALID)
            XR_ERRNAME(XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_ACTION_TYPE_MISMATCH)
            XR_ERRNAME(XR_ERROR_SESSION_NOT_READY)
            XR_ERRNAME(XR_ERROR_SESSION_NOT_STOPPING)
            XR_ERRNAME(XR_ERROR_TIME_INVALID)
            XR_ERRNAME(XR_ERROR_REFERENCE_SPACE_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_FILE_ACCESS_ERROR)
            XR_ERRNAME(XR_ERROR_FILE_CONTENTS_INVALID)
            XR_ERRNAME(XR_ERROR_FORM_FACTOR_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_FORM_FACTOR_UNAVAILABLE)
            XR_ERRNAME(XR_ERROR_API_LAYER_NOT_PRESENT)
            XR_ERRNAME(XR_ERROR_CALL_ORDER_INVALID)
            XR_ERRNAME(XR_ERROR_GRAPHICS_DEVICE_INVALID)
            XR_ERRNAME(XR_ERROR_POSE_INVALID)
            XR_ERRNAME(XR_ERROR_INDEX_OUT_OF_RANGE)
            XR_ERRNAME(XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED)
            XR_ERRNAME(XR_ERROR_NAME_DUPLICATED)
            XR_ERRNAME(XR_ERROR_NAME_INVALID)
            XR_ERRNAME(XR_ERROR_ACTIONSET_NOT_ATTACHED)
            XR_ERRNAME(XR_ERROR_ACTIONSETS_ALREADY_ATTACHED)
            XR_ERRNAME(XR_ERROR_LOCALIZED_NAME_DUPLICATED)
            XR_ERRNAME(XR_ERROR_LOCALIZED_NAME_INVALID)
            XR_ERRNAME(XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING)
            XR_ERRNAME(XR_ERROR_RUNTIME_UNAVAILABLE)
            XR_ERRNAME(XR_ERROR_ANDROID_THREAD_SETTINGS_ID_INVALID_KHR)
            XR_ERRNAME(XR_ERROR_ANDROID_THREAD_SETTINGS_FAILURE_KHR)
            XR_ERRNAME(XR_ERROR_CREATE_SPATIAL_ANCHOR_FAILED_MSFT)
            XR_ERRNAME(XR_ERROR_SECONDARY_VIEW_CONFIGURATION_TYPE_NOT_ENABLED_MSFT)
            XR_ERRNAME(XR_ERROR_CONTROLLER_MODEL_KEY_INVALID_MSFT)
            XR_ERRNAME(XR_ERROR_REPROJECTION_MODE_UNSUPPORTED_MSFT)
            XR_ERRNAME(XR_ERROR_COMPUTE_NEW_SCENE_NOT_COMPLETED_MSFT)
            XR_ERRNAME(XR_ERROR_SCENE_COMPONENT_ID_INVALID_MSFT)
            XR_ERRNAME(XR_ERROR_SCENE_COMPONENT_TYPE_MISMATCH_MSFT)
            XR_ERRNAME(XR_ERROR_SCENE_MESH_BUFFER_ID_INVALID_MSFT)
            XR_ERRNAME(XR_ERROR_SCENE_COMPUTE_FEATURE_INCOMPATIBLE_MSFT)
            XR_ERRNAME(XR_ERROR_SCENE_COMPUTE_CONSISTENCY_MISMATCH_MSFT)
            XR_ERRNAME(XR_ERROR_DISPLAY_REFRESH_RATE_UNSUPPORTED_FB)
            XR_ERRNAME(XR_ERROR_COLOR_SPACE_UNSUPPORTED_FB)
            XR_ERRNAME(XR_ERROR_SPATIAL_ANCHOR_NAME_NOT_FOUND_MSFT)
            XR_ERRNAME(XR_ERROR_SPATIAL_ANCHOR_NAME_INVALID_MSFT)
    };

    const char* xrGetErrorStr(XrResult r)
    {
        auto found = xrErrorNames.find(r);
        if (found != xrErrorNames.end())
            return found->second;
        return "Unknown XR Error";
    }

    Vector3 uxrGetVec(XrVector3f v)
    {
        return Vector3(v.x, v.y, -v.z);
    }

    Urho3D::Quaternion uxrGetQuat(XrQuaternionf q)
    {
        Quaternion out;
        out.x_ = -q.x;
        out.y_ = -q.y;
        out.z_ = q.z;
        out.w_ = q.w;
        return out;
    }

    Urho3D::Matrix3x4 uxrGetTransform(XrPosef pose, float scale)
    {
        return Matrix3x4(uxrGetVec(pose.position), uxrGetQuat(pose.orientation), scale);
    }

    Urho3D::Matrix4 uxrGetProjection(float nearZ, float farZ, float angleLeft, float angleTop, float angleRight, float angleBottom)
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

    ea::pair<Vector3, Urho3D::Matrix4> uxrGetSharedProjection(float nearZ, float farZ, XrFovf left, XrFovf right, Vector3 eyeLeftLocal, Vector3 eyeRightLocal)
    {
        // Check if we're reasonably possible to do, if not return Matrix4::ZERO so we know this isn't viable.
        if (Abs(M_RADTODEG * left.angleLeft) + Abs(M_RADTODEG * right.angleRight) > 160.0f)
            return { Vector3::ZERO, Matrix4::ZERO };

        // just bottom out the vertical angles, have one for each eye so take the most extreme
        const float trueDown = Min(left.angleDown, right.angleDown);
        const float trueUp = Max(left.angleUp, right.angleUp);

        if (Abs(M_RADTODEG * trueDown) + Abs(M_RADTODEG * trueUp) > 160.0f)
            return { Vector3::ZERO, Matrix4::ZERO };

        // Reference:
        // https://computergraphics.stackexchange.com/questions/1736/vr-and-frustum-culling
        // using generalized, note that the above assumes POSITIVE angles, hence -angleLeft here and there below
        const float ipd = Abs(eyeRightLocal.x_ - eyeLeftLocal.x_);

        /// how deeply it needs move back
        float recess = ipd / (tanf(-left.angleLeft) + tanf(right.angleRight));
        const float upDownRecess = Abs(eyeRightLocal.y_ - eyeLeftLocal.y_) / (tanf(-trueDown) + tanf(trueUp));

        // how far along we need to center the moved back point
        const float leftDist = tanf(-left.angleLeft) * recess;
        const float downDist = tanf(-trueDown) * upDownRecess;

        // we may have to go back even further because of up/down instead of left/right being the big issue
        recess = Max(recess, upDownRecess);

        Vector3 outLocalPos = Vector3(eyeLeftLocal.x_ + leftDist, eyeLeftLocal.y_ + downDist, eyeLeftLocal.z_ - recess);

        nearZ += recess;

        const float tanLeft = tanf(left.angleLeft);
        const float tanRight = tanf(right.angleRight);
        const float tanDown = tanf(trueDown);
        const float tanUp = tanf(trueUp);
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

        return { outLocalPos, projection };
    }

    struct OpenXR::Opaque
    {
#ifdef URHO3D_D3D11
        XrSwapchainImageD3D11KHR swapImages_[4] = { };
        XrSwapchainImageD3D11KHR depthImages_[4] = { };
#endif
#ifdef URHO3D_OPENGL
    #ifndef GL_ES_VERSION_2_0
        XrSwapchainImageOpenGLKHR swapImages_[4] = { };
        XrSwapchainImageOpenGLKHR depthImages_[4] = { };
    #else
        XrSwapchainImageOpenGLESKHR swapImages_[4] = { };
        XrSwapchainImageOpenGLESKHR depthImages_[4] = { };
    #endif
#endif
    };

    OpenXR::OpenXR(Context* ctx) : 
        BaseClassName(ctx),
        instance_(0),
        session_(0),
        swapChain_{},
        sessionLive_(false)
    {
        useSingleTexture_ = true;

        opaque_.reset(new Opaque());

        SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(OpenXR, HandlePreUpdate));
        SubscribeToEvent(E_ENDRENDERING, URHO3D_HANDLER(OpenXR, HandlePostRender));
    }

    OpenXR::~OpenXR()
    {
        opaque_.reset();
        Shutdown();
    }

    void OpenXR::QueryExtensions()
    {
        extensions_.clear();

        unsigned extCt = 0;
        xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCt, nullptr);
        ea::vector<XrExtensionProperties> extensions;
        extensions.resize(extCt);
        for (auto& x : extensions)
            x = { XR_TYPE_EXTENSION_PROPERTIES };
        xrEnumerateInstanceExtensionProperties(nullptr, extensions.size(), &extCt, extensions.data());
        
        for (auto e : extensions)
            extensions_.push_back(ea::string(e.extensionName));
    }

    bool OpenXR::Initialize(const ea::string& manifestPath)
    {
        auto graphics = GetSubsystem<Graphics>();

        if (instance_ != 0)
        {
            URHO3D_LOGERROR("OpenXR is already initialized");
            return false;
        }

        manifest_ = new XMLFile(GetContext());
        auto cache = GetSubsystem<ResourceCache>();
        if (!manifest_->LoadFile(cache->GetResourceFileName(manifestPath)))
            manifest_.Reset();

        QueryExtensions();

        static auto SupportsExt = [](const StringVector& extensions, const char* name) {
            for (auto ext : extensions)
            {
                if (ext.comparei(name) == 0)
                    return true;
            }
            return false;
        };

        ea::vector<const char*> activeExt;
#ifdef URHO3D_D3D11
        activeExt.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
#elif defined(URHO3D_OPENGL)
    #ifdef GL_ES_VERSION_2_0
            activeExt.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
    #else   
            activeExt.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
    #endif  
#endif

        bool supportsDebug = false;
        if (SupportsExt(extensions_, XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            activeExt.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
            supportsDebug = true;
        }
        if (SupportsExt(extensions_, XR_KHR_VISIBILITY_MASK_EXTENSION_NAME))
        {
            activeExt.push_back(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);
            supportsMask_ = true;
        }
        if (SupportsExt(extensions_, XR_MSFT_CONTROLLER_MODEL_EXTENSION_NAME))
        {
            activeExt.push_back(XR_MSFT_CONTROLLER_MODEL_EXTENSION_NAME);
            supportsControllerModel_ = true;
        }
        if (SupportsExt(extensions_, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME))
        {
            activeExt.push_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
            /* at the moment we don't function correctly
            * presumably has to do with RenderBufferManager */
            supportsDepthExt_ = true;
        }

        // Controllers
        if (SupportsExt(extensions_, XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME))
            activeExt.push_back(XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME);
        if (SupportsExt(extensions_, XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME))
            activeExt.push_back(XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME);
        if (SupportsExt(extensions_, XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME))
            activeExt.push_back(XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME);
        if (SupportsExt(extensions_, XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME))
            activeExt.push_back(XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME);

        for (auto e : extraExtensions_)
            activeExt.push_back(e.c_str());

        XrInstanceCreateInfo info = { XR_TYPE_INSTANCE_CREATE_INFO };
        strcpy_s(info.applicationInfo.engineName, 128, "RBFX");
        strcpy_s(info.applicationInfo.applicationName, 128, "RBFX"); // TODO: get a proper name from some configuration
        info.applicationInfo.engineVersion = (1 << 24) + (0 << 16) + 0; // TODO: get an actual engine version
        info.applicationInfo.applicationVersion = 0; // TODO: application version?
        info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        info.enabledExtensionCount = activeExt.size();
        info.enabledExtensionNames = activeExt.data();

        instance_ = 0x0;
        auto errCode = xrCreateInstance(&info, &instance_);
        if (errCode != XrResult::XR_SUCCESS)
        {
            URHO3D_LOGERRORF("Unable to create OpenXR instance: %s", xrGetErrorStr(errCode));
            return false;
        }

        XR_FOREACH(XR_LOAD);
        XR_PLATFORM(XR_LOAD);
        XR_EXTENSION_BASED(XR_LOAD);

        // Print the runtime name and version, that will be useful information to have
        XrInstanceProperties instProps = { XR_TYPE_INSTANCE_PROPERTIES };
        if (xrGetInstanceProperties(instance_, &instProps) == XR_SUCCESS)
            URHO3D_LOGINFOF("OpenXR Runtime is: {} version {}", instProps.runtimeName, instProps.runtimeVersion);

        if (supportsDebug)
        {
            XrDebugUtilsMessengerCreateInfoEXT debugUtils = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
            debugUtils.messageTypes =
                XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
            debugUtils.messageSeverities =
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            debugUtils.userData = GetSubsystem<Log>();
            debugUtils.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT *msg, void* user_data) {
                auto log = ((Log*)user_data);

                if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                    log->GetLogger().Write(LOG_ERROR, Urho3D::ToString("XR Error: %s, %s", msg->functionName, msg->message));
                else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                    log->GetLogger().Write(LOG_WARNING, Urho3D::ToString("XR Warning: %s, %s", msg->functionName, msg->message));
                else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
                    log->GetLogger().Write(LOG_INFO, Urho3D::ToString("XR Info: %s, %s", msg->functionName, msg->message));
                else if (severity & XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
                    log->GetLogger().Write(LOG_DEBUG, Urho3D::ToString("XR Verbose: %s, %s", msg->functionName, msg->message));

                // std::cout as well so we can see it in IDE output without needing log when in stepping though code
                std::cout << msg->functionName << " : " << msg->message << std::endl;
                return (XrBool32)XR_FALSE;
            };

            xrCreateDebugUtilsMessengerEXT(instance_, &debugUtils, &messenger_);
        }

        XrSystemGetInfo sysInfo = { XR_TYPE_SYSTEM_GET_INFO };
        sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

#define XR_COMMON_ER(TERM) if (errCode != XrResult::XR_SUCCESS) { \
    URHO3D_LOGERRORF("Unable to produce OpenXR " TERM " ID: %s", xrGetErrorStr(errCode)); \
    Shutdown(); \
    return false; }

        errCode = xrGetSystem(instance_, &sysInfo, &system_);
        XR_COMMON_ER("system ID");

        uint32_t blendCount = 0;
        errCode = xrEnumerateEnvironmentBlendModes(instance_, system_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 1, &blendCount, &blendMode_);
        XR_COMMON_ER("blending mode");

        XrSystemProperties sysProps = { XR_TYPE_SYSTEM_PROPERTIES };
        errCode = xrGetSystemProperties(instance_, system_, &sysProps);
        XR_COMMON_ER("system properties");
        systemName_ = sysProps.systemName;

        unsigned viewConfigCt = 0;
        XrViewConfigurationType viewConfigurations[4];
        errCode = xrEnumerateViewConfigurations(instance_, system_, 4, &viewConfigCt, viewConfigurations);
        XR_COMMON_ER("view config");
        bool stereo = false;
        for (auto v : viewConfigurations)
        {
            if (v == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
                stereo = true;
        }

        if (!stereo)
        {
            URHO3D_LOGERROR("Stereo rendering not supported on this device");
            Shutdown();
            return false;
        }

        unsigned viewCt = 0;
        XrViewConfigurationView views[2] = { { XR_TYPE_VIEW_CONFIGURATION_VIEW }, { XR_TYPE_VIEW_CONFIGURATION_VIEW } };
        errCode = xrEnumerateViewConfigurationViews(instance_, system_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCt, views);
        XR_COMMON_ER("view config views")

        trueEyeTexWidth_ = eyeTexWidth_ = (int)Min(views[VR_EYE_LEFT].recommendedImageRectWidth, views[VR_EYE_RIGHT].recommendedImageRectWidth);
        trueEyeTexHeight_ = eyeTexHeight_ = (int)Min(views[VR_EYE_LEFT].recommendedImageRectHeight, views[VR_EYE_RIGHT].recommendedImageRectHeight);
        msaaLevel_ = Min(msaaLevel_, views[VR_EYE_LEFT].maxSwapchainSampleCount);

        eyeTexWidth_ = eyeTexWidth_ * renderTargetScale_;
        eyeTexHeight_ = eyeTexHeight_ * renderTargetScale_;

        if (!OpenSession())
        {
            Shutdown();
            return false;
        }

        if (!CreateSwapchain())
        {
            Shutdown();
            return false;
        }

        GetHiddenAreaMask();

        return true;
    }

    void OpenXR::Shutdown()
    {
        // already shutdown?
        if (instance_ == 0)
            return;

        for (int i = 0; i < 2; ++i)
        {
            wandModels_[i] = { };
            handGrips_[i].Reset();
            handAims_[i].Reset();
            handHaptics_[i].Reset();
            views_[i] = { XR_TYPE_VIEW };
        }
        manifest_.Reset();
        actionSets_.clear();
        activeActionSet_.Reset();
        sessionLive_ = false;
        extensions_.clear();

        DestroySwapchain();

#define DTOR_XR(N, S) if (S) xrDestroy ## N(S)

        DTOR_XR(Space, headSpace_);
        DTOR_XR(Space, viewSpace_);

#undef DTOR_SPACE

        CloseSession();
        session_ = { };

        if (messenger_)
            xrDestroyDebugUtilsMessengerEXT(messenger_);

        if (instance_)
            xrDestroyInstance(instance_);

        instance_ = { };
        system_ = { };
        blendMode_ = { };
        headSpace_ = { };
        viewSpace_ = { };
        messenger_ = { };
    }

    bool OpenXR::OpenSession()
    {
        auto graphics = GetSubsystem<Graphics>();

        XrActionSetCreateInfo actionCreateInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
        strcpy_s(actionCreateInfo.actionSetName, 64, "default");
        strcpy_s(actionCreateInfo.localizedActionSetName, 128, "default");
        actionCreateInfo.priority = 0;

        XrSessionCreateInfo sessionCreateInfo = { XR_TYPE_SESSION_CREATE_INFO };
        sessionCreateInfo.systemId = system_;

#if URHO3D_D3D11
        XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
        binding.device = graphics->GetImpl()->GetDevice();        
        sessionCreateInfo.next = &binding;

        XrGraphicsRequirementsD3D11KHR requisite = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
        auto errCode = xrGetD3D11GraphicsRequirementsKHR(instance_, system_, &requisite);
        XR_COMMON_ER("graphics requirements");
#endif
#if URHO3D_OPENGL
    #ifdef WIN32 // Win32 is never GLES as far as I'm aware?
        XrGraphicsRequirementsOpenGLKHR requisite = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
        auto errCode = xrGetOpenGLGraphicsRequirementsKHR(instance_, system_, &requisite);
        XR_COMMON_ER("graphics requirements");

        XrGraphicsBindingOpenGLWin32KHR binding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };

        binding.hDC = wglGetCurrentDC();
        binding.hGLRC = wglGetCurrentContext();
            
        sessionCreateInfo.next = &binding;
    #else
        #error Incomplete OpenXR intialization, require Linux/Android+GLES
    #endif
#endif        

        errCode = xrCreateSession(instance_, &sessionCreateInfo, &session_);
        XR_COMMON_ER("session");

        // attempt stage-space first
        XrReferenceSpaceCreateInfo refSpaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        refSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        refSpaceInfo.poseInReferenceSpace.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
        refSpaceInfo.poseInReferenceSpace.position = { 0.0f, 0.0f, 0.0f };

        errCode = xrCreateReferenceSpace(session_, &refSpaceInfo, &headSpace_);
        // Failed? Then do local space, but can this even fail?
        if (errCode != XR_SUCCESS)
        {
            refSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            errCode = xrCreateReferenceSpace(session_, &refSpaceInfo, &headSpace_);
            XR_COMMON_ER("reference space");
            isRoomScale_ = false;
        }
        else
            isRoomScale_ = true;

        XrReferenceSpaceCreateInfo viewSpaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        viewSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        viewSpaceInfo.poseInReferenceSpace.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
        viewSpaceInfo.poseInReferenceSpace.position = { 0.0f, 0.0f, 0.0f };
        errCode = xrCreateReferenceSpace(session_, &viewSpaceInfo, &viewSpace_);
        XR_COMMON_ER("view reference space");        

        if (manifest_)
            BindActions(manifest_);

        // if there's a default action set, then use it.
        VRInterface::SetCurrentActionSet("default");

        return true;
    }

    void OpenXR::CloseSession()
    {
        if (session_)
            xrDestroySession(session_);
        session_ = 0;
    }

    bool OpenXR::CreateSwapchain()
    {
        auto graphics = GetSubsystem<Graphics>();

        for (int j = 0; j < 4; ++j)
        {
#ifdef URHO3D_D3D11
            opaque_->swapImages_[j].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            opaque_->swapImages_[j].next = nullptr;
            opaque_->depthImages_[j].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            opaque_->depthImages_[j].next = nullptr;
#endif
#ifdef URHO3D_OPENGL
#ifndef GL_ES_VERSION_2_0
            opaque_->swapImages_[j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#else
            opaque_->swapImages_[j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#endif
            opaque_->swapImages_[j].next = nullptr;
#endif
        }

        unsigned fmtCt = 0;
        xrEnumerateSwapchainFormats(session_, 0, &fmtCt, 0);
        ea::vector<int64_t> fmts;
        fmts.resize(fmtCt);
        xrEnumerateSwapchainFormats(session_, fmtCt, &fmtCt, fmts.data());

        // take first of RGBA/SRGBA supported.
        for (auto fmt : fmts)
        {
            if (fmt == graphics->GetRGBAFormat())
            {
                backbufferFormat_ = fmt;
                break;
            }
#if URHO3D_D3D11
            else if (fmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
#elif URHO3D_OPENGL
            else if (fmt == GL_SRGB8)
#endif
            {
                backbufferFormat_ = fmt;
                break;
            }
        }

        // See if we've got a depth format we can handle, if we do this means we'll produce swapchains
        // We'll be able to count on details like MSAA-Quality being compatible.
        for (auto fmt : fmts)
        {
            if (fmt == graphics->GetDepthStencilFormat())
            {
                depthFormat_ = fmt;
                break;
            }
#ifdef URHO3D_D3D11
            if (fmt == DXGI_FORMAT_D24_UNORM_S8_UINT) // what we actually construct, as opposed to typeless
            {
                depthFormat_ = fmt;
                break;
            }
#endif
        }

        XrSwapchainCreateInfo swapInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapInfo.format = backbufferFormat_;
        swapInfo.width = eyeTexWidth_ * 2; // note: if eventually supported array targets this will change.
        swapInfo.height = eyeTexHeight_;
        swapInfo.sampleCount = msaaLevel_;
        swapInfo.faceCount = 1;
        swapInfo.arraySize = 1; // note: if we support array targets and layered this changes, we're still as single texture mode
        swapInfo.mipCount = 1;

        auto errCode = xrCreateSwapchain(session_, &swapInfo, &swapChain_);
        XR_COMMON_ER("swapchain")

        errCode = xrEnumerateSwapchainImages(swapChain_, 4, &imgCount_, (XrSwapchainImageBaseHeader*)opaque_->swapImages_);
        XR_COMMON_ER("swapchain images")

        // If we found a depth format than construct swapchains for depth.
        // Doing this should mean we've done as much as possible to prevent ourselves from blocking due to something.
        // At the present, checking we have depth-info-ext to do anything with it.
        if (supportsDepthExt_ && depthFormat_)
        {
            XrSwapchainCreateInfo swapInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
            swapInfo.format = depthFormat_;
            swapInfo.width = eyeTexWidth_ * 2; // note: if eventually supported array targets this will change.
            swapInfo.height = eyeTexHeight_;
            swapInfo.sampleCount = msaaLevel_;
            swapInfo.faceCount = 1;
            swapInfo.arraySize = 1; // note: if we support array targets and layered this changes, we're still as single texture mode
            swapInfo.mipCount = 1;

            auto errCode = xrCreateSwapchain(session_, &swapInfo, &depthChain_);
            XR_COMMON_ER("swapchain")

            errCode = xrEnumerateSwapchainImages(depthChain_, 4, &depthImgCount_, (XrSwapchainImageBaseHeader*)opaque_->depthImages_);
            XR_COMMON_ER("swapchain images")
        }
        else
            depthImgCount_ = 0;

#ifdef URHO3D_D3D11
        // bump the d3d-ref count
        for (int i = 0; i < imgCount_; ++i)
            opaque_->swapImages_[i].texture->AddRef();
        for (int i = 0; i < depthImgCount_; ++i)
            opaque_->depthImages_[i].texture->AddRef();
#endif

        CreateEyeTextures();

        return true;
    }

    void OpenXR::DestroySwapchain()
    {
        for (int i = 0; i < 4; ++i)
        {
            eyeColorTextures_[i].Reset();
            eyeDepthTextures_[i].Reset();
        }

        if (swapChain_)
            xrDestroySwapchain(swapChain_);
        if (depthChain_)
            xrDestroySwapchain(depthChain_);

        swapChain_ = { };
        depthChain_ = { };
    }

    void OpenXR::CreateEyeTextures()
    {
        sharedTexture_ = new Texture2D(GetContext());
        sharedDS_.Reset();

        // OXR we never use these, TODO: remove from SteamVR as well.
        leftTexture_.Reset();
        rightTexture_.Reset();
        leftDS_.Reset();
        rightDS_.Reset();

        if (depthChain_ == 0)
        {
            sharedDS_ = new Texture2D(GetContext());
            sharedDS_->SetNumLevels(1);
            sharedDS_->SetSize(eyeTexWidth_ * 2, eyeTexHeight_, Graphics::GetDepthStencilFormat(), TEXTURE_DEPTHSTENCIL, msaaLevel_);
        }

        for (unsigned i = 0; i < imgCount_; ++i)
        {
            eyeColorTextures_[i] = new Texture2D(GetContext());

#ifdef URHO3D_D3D11
            // NOTE: D3D11 we can get most info from queries more easily
            eyeColorTextures_[i]->CreateFromExternal(opaque_->swapImages_[i].texture, msaaLevel_, backbufferFormat_ == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
            if (depthChain_)
            {
                eyeDepthTextures_[i] = new Texture2D(GetContext());
                eyeDepthTextures_[i]->CreateFromExternal(opaque_->depthImages_[i].texture, msaaLevel_, false);
            }
#endif

#ifdef URHO3D_OPENGL
            // Queries pain, we already know this information so just pass it
            eyeColorTextures_[i]->CreateFromExternal(opaque_->swapImages_[i].image, eyeTexWidth_ * 2, eyeTexHeight_, msaaLevel_, backbufferFormat_, TEXTURE_RENDERTARGET);
            if (depthChain_)
            {
                eyeDepthTextures_[i] = new Texture2D(GetContext());
                eyeDepthTextures_[i]->CreateFromExternal(opaque_->depthImages_[i].image, eyeTexWidth_ * 2, eyeTexHeight_, msaaLevel_, depthFormat_, TEXTURE_RENDERTARGET);
            }
#endif
            eyeColorTextures_[i]->GetRenderSurface()->SetLinkedDepthStencil(depthChain_ ? eyeDepthTextures_[i]->GetRenderSurface() : sharedDS_->GetRenderSurface());
        }
    }

    void OpenXR::HandlePreUpdate(StringHash, VariantMap& data)
    {
        // Check if we need to do anything at all.
        if (instance_ == 0 || session_ == 0)
            return;

        XrEventDataBuffer eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };
        while (xrPollEvent(instance_, &eventBuffer) == XR_SUCCESS)
        {
            switch (eventBuffer.type)
            {
            case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
                GetHiddenAreaMask();
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                sessionLive_ = false;
                SendEvent(E_VREXIT); //?? does something need to be communicated beyond this?
                break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                UpdateBindingBound();
                SendEvent(E_VRINTERACTIONPROFILECHANGED);
                break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&eventBuffer;
                auto state = changed->state;
                switch (state)
                {
                case XR_SESSION_STATE_READY: {
                    XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
                    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    auto res = xrBeginSession(session_, &beginInfo);
                    if (res != XR_SUCCESS)
                    {
                        URHO3D_LOGERRORF("Failed to begin XR session: %s", xrGetErrorStr(res));
                        sessionLive_ = false;
                        SendEvent(E_VRSESSIONSTART);
                    }
                    else
                        sessionLive_ = true; // uhhh what
                } break;
                case XR_SESSION_STATE_IDLE:
                    SendEvent(E_VRPAUSE);
                    sessionLive_ = false;
                    break;
                case XR_SESSION_STATE_FOCUSED: // we're hooked up
                    sessionLive_ = true;
                    SendEvent(E_VRRESUME);
                    break;
                case XR_SESSION_STATE_STOPPING:
                    xrEndSession(session_);
                    sessionLive_ = false;
                    break;
                case XR_SESSION_STATE_EXITING:
                case XR_SESSION_STATE_LOSS_PENDING:
                    sessionLive_ = false;
                    SendEvent(E_VREXIT);
                    break;
                }

            }

            eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };
        }

        if (!IsLive())
            return;

        XrFrameState frameState = { XR_TYPE_FRAME_STATE };
        xrWaitFrame(session_, nullptr, &frameState);
        predictedTime_ = frameState.predictedDisplayTime;

        XrFrameBeginInfo begInfo = { XR_TYPE_FRAME_BEGIN_INFO };
        xrBeginFrame(session_, &begInfo);
    // head stuff
        headLoc_.next = &headVel_;
        xrLocateSpace(viewSpace_, headSpace_, frameState.predictedDisplayTime, &headLoc_);

        HandlePreRender();

        for (int i = 0; i < 2; ++i)
        {
            if (handAims_[i])
            {
                // ensure velocity is linked
                handAims_[i]->location_.next = &handAims_[i]->velocity_;
                xrLocateSpace(handAims_[i]->actionSpace_, headSpace_, frameState.predictedDisplayTime, &handAims_[i]->location_);
            }

            if (handGrips_[i])
            {
                handGrips_[i]->location_.next = &handGrips_[i]->velocity_;
                xrLocateSpace(handGrips_[i]->actionSpace_, headSpace_, frameState.predictedDisplayTime, &handGrips_[i]->location_);
            }
        }

    // eyes
        XrViewLocateInfo viewInfo = { XR_TYPE_VIEW_LOCATE_INFO };
        viewInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewInfo.space = headSpace_;
        viewInfo.displayTime = frameState.predictedDisplayTime;

        XrViewState viewState = { XR_TYPE_VIEW_STATE };
        unsigned viewCt = 0;
        xrLocateViews(session_, &viewInfo, &viewState, 2, &viewCt, views_);

    // handle actions
        if (activeActionSet_)
        {
            auto set = activeActionSet_->Cast<XRActionSet>();

            XrActiveActionSet activeSet = { };
            activeSet.actionSet = set->actionSet_;

            XrActionsSyncInfo sync = { XR_TYPE_ACTIONS_SYNC_INFO };
            sync.activeActionSets = &activeSet;
            sync.countActiveActionSets = 1;
            xrSyncActions(session_, &sync);

            using namespace BeginFrame;
            UpdateBindings(data[P_TIMESTEP].GetFloat());
        }
    }

    void OpenXR::HandlePreRender()
    {
        if (IsLive())
        {
            XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            unsigned imgID;
            auto res = xrAcquireSwapchainImage(swapChain_, &acquireInfo, &imgID);
            if (res != XR_SUCCESS)
            {
                URHO3D_LOGERRORF("Failed to acquire swapchain: %s", xrGetErrorStr(res));
                return;
            }

            XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            waitInfo.timeout = XR_INFINITE_DURATION;
            res = xrWaitSwapchainImage(swapChain_, &waitInfo);
            if (res != XR_SUCCESS)
                URHO3D_LOGERRORF("Failed to wait on swapchain: %s", xrGetErrorStr(res));

            // update which shared-texture we're using so UpdateRig will do things correctly.
            sharedTexture_ = eyeColorTextures_[imgID];

            // If we've got depth then do the same and setup the linked depth stencil for the above shared texture.
            if (depthChain_)
            {
                // still remaking the objects here, assuming that at any time these may one day do something
                // in such a fashion that reuse is not a good thing.
                unsigned depthID;
                XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
                auto res = xrAcquireSwapchainImage(depthChain_, &acquireInfo, &depthID);
                if (res == XR_SUCCESS)
                {
                    XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
                    waitInfo.timeout = XR_INFINITE_DURATION;
                    res = xrWaitSwapchainImage(depthChain_, &waitInfo);
                    sharedDS_ = eyeDepthTextures_[depthID];
                    sharedTexture_->GetRenderSurface()->SetLinkedDepthStencil(sharedDS_->GetRenderSurface());
                }
            }
        }
    }

    void OpenXR::HandlePostRender(StringHash, VariantMap&)
    {
        if (IsLive())
        {
#define CHECKVIEW(EYE) (views_[EYE].fov.angleLeft == 0 || views_[EYE].fov.angleRight == 0 || views_[EYE].fov.angleUp == 0 || views_[EYE].fov.angleDown == 0)
            
            XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(swapChain_, &releaseInfo);
            if (depthChain_)
            {
                XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                xrReleaseSwapchainImage(depthChain_, &releaseInfo);
            }

            // it's harmless but checking this will prevent early bad draws with null FOV
            // XR eats the error, but handle it anyways to keep a clean output log
            if (CHECKVIEW(VR_EYE_LEFT) || CHECKVIEW(VR_EYE_RIGHT))
                return;

            XrCompositionLayerProjectionView eyes[2] = { { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW } };
            eyes[VR_EYE_LEFT].subImage.imageArrayIndex = 0;
            eyes[VR_EYE_LEFT].subImage.swapchain = swapChain_;
            eyes[VR_EYE_LEFT].subImage.imageRect = { { 0, 0 }, { eyeTexWidth_, eyeTexHeight_} };
            eyes[VR_EYE_LEFT].fov = views_[VR_EYE_LEFT].fov;
            eyes[VR_EYE_LEFT].pose = views_[VR_EYE_LEFT].pose;

            eyes[VR_EYE_RIGHT].subImage.imageArrayIndex = 0;
            eyes[VR_EYE_RIGHT].subImage.swapchain = swapChain_;
            eyes[VR_EYE_RIGHT].subImage.imageRect = { { eyeTexWidth_, 0 }, { eyeTexWidth_, eyeTexHeight_} };
            eyes[VR_EYE_RIGHT].fov = views_[VR_EYE_RIGHT].fov;
            eyes[VR_EYE_RIGHT].pose = views_[VR_EYE_RIGHT].pose;

            static XrCompositionLayerDepthInfoKHR depth[2] = {
                    { XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR }, { XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR }
            };

            // May we ever have a depth-chain without this in theory?
            if (supportsDepthExt_ && depthChain_)
            {
                // depth
                depth[VR_EYE_LEFT].subImage.imageArrayIndex = 0;
                depth[VR_EYE_LEFT].subImage.swapchain = depthChain_;
                depth[VR_EYE_LEFT].subImage.imageRect = { { 0, 0 }, { eyeTexWidth_, eyeTexHeight_} };
                depth[VR_EYE_LEFT].minDepth = 0.0f; // spec says range of 0-1, so doesn't respect GL -1 to 1?
                depth[VR_EYE_LEFT].maxDepth = 1.0f;
                depth[VR_EYE_LEFT].nearZ = lastNearDist_;
                depth[VR_EYE_LEFT].farZ = lastFarDist_;

                depth[VR_EYE_RIGHT].subImage.imageArrayIndex = 0;
                depth[VR_EYE_RIGHT].subImage.swapchain = depthChain_;
                depth[VR_EYE_RIGHT].subImage.imageRect = { { eyeTexWidth_, 0 }, { eyeTexWidth_, eyeTexHeight_} };
                depth[VR_EYE_RIGHT].minDepth = 0.0f;
                depth[VR_EYE_RIGHT].maxDepth = 1.0f;
                depth[VR_EYE_RIGHT].nearZ = lastNearDist_;
                depth[VR_EYE_RIGHT].farZ = lastFarDist_;

                // These are chained to the relevant eye, not passed in through another mechanism.

                /* not attached at present as it's messed up, probably as referenced above in depth-info ext detection that it's probably a RenderBuffermanager copy issue */
                //eyes[VR_EYE_LEFT].next = &depth[VR_EYE_LEFT];
                //eyes[VR_EYE_RIGHT].next = &depth[VR_EYE_RIGHT];
            }

            XrCompositionLayerProjection proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
            proj.viewCount = 2;
            proj.views = eyes;
            proj.space = headSpace_;

            XrCompositionLayerBaseHeader* header = (XrCompositionLayerBaseHeader*)&proj;

            XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
            endInfo.layerCount = 1;
            endInfo.layers = &header;
            endInfo.environmentBlendMode = blendMode_;
            endInfo.displayTime = predictedTime_;

            xrEndFrame(session_, &endInfo);
        }
    }

    void OpenXR::BindActions(SharedPtr<XMLFile> doc)
    {
        auto root = doc->GetRoot();

        auto sets = root.GetChild("actionsets");

        XrPath handPaths[2];
        xrStringToPath(instance_, "/user/hand/left", &handPaths[VR_HAND_LEFT]);
        xrStringToPath(instance_, "/user/hand/right", &handPaths[VR_HAND_RIGHT]);

        for (auto set = root.GetChild("actionset"); set.NotNull(); set = set.GetNext("actionset"))
        {
            XrActionSetCreateInfo setCreateInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
            ea::string setName = set.GetAttribute("name");
            ea::string setLocalName = GetSubsystem<Localization>()->Get(setName);
            strncpy(setCreateInfo.actionSetName, setName.c_str(), XR_MAX_ACTION_SET_NAME_SIZE);
            strncpy(setCreateInfo.localizedActionSetName, setLocalName.c_str(), XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
            
            XrActionSet createSet = { };
            auto errCode = xrCreateActionSet(instance_, &setCreateInfo, &createSet);
            if (errCode != XR_SUCCESS)
            {
                URHO3D_LOGERRORF("Failed to create ActionSet: %s, error: %s", setName.c_str(), xrGetErrorStr(errCode));
                continue;
            }

            // create our wrapper
            SharedPtr<XRActionSet> actionSet(new XRActionSet(GetContext()));
            actionSet->actionSet_ = createSet;
            actionSets_.insert({ setName, actionSet });

            auto bindings = set.GetChild("actions");
            for (auto child = bindings.GetChild("action"); child.NotNull(); child = child.GetNext("action"))
            {
                ea::string name = child.GetAttribute("name");
                ea::string type = child.GetAttribute("type");
                bool handed = child.GetBool("handed");

                SharedPtr<XRActionBinding> binding(new XRActionBinding(GetContext(), this));
                SharedPtr<XRActionBinding> otherHand = binding; // if identical it won't be pushed

                XrActionCreateInfo createInfo = { XR_TYPE_ACTION_CREATE_INFO };
                if (handed)
                {
                    otherHand = new XRActionBinding(GetContext(), this);
                    binding->hand_ = VR_HAND_LEFT;
                    binding->subPath_ = handPaths[VR_HAND_LEFT];
                    otherHand->hand_ = VR_HAND_RIGHT;
                    otherHand->subPath_ = handPaths[VR_HAND_RIGHT];

                    createInfo.countSubactionPaths = 2;
                    createInfo.subactionPaths = handPaths;
                    binding->hand_ = VR_HAND_LEFT;
                    otherHand->hand_ = VR_HAND_RIGHT;
                }
                else
                    binding->hand_ = VR_HAND_NONE;

                ea::string localizedName = GetSubsystem<Localization>()->Get(name);
                strcpy_s(createInfo.actionName, 64, name.c_str());
                strcpy_s(createInfo.localizedActionName, 128, localizedName.c_str());

#define DUPLEX(F, V) binding->F = V; otherHand->F = V

                DUPLEX(path_, name);
                DUPLEX(localizedName_, localizedName);

                if (type == "boolean")
                {
                    DUPLEX(dataType_, VAR_BOOL);
                    createInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
                }
                else if (type == "vector1" || type == "single")
                {
                    DUPLEX(dataType_, VAR_FLOAT);
                    createInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
                }
                else if (type == "vector2")
                {
                    DUPLEX(dataType_, VAR_VECTOR2);
                    createInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
                }
                else if (type == "vector3")
                {
                    DUPLEX(dataType_, VAR_VECTOR3);
                    createInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
                }
                else if (type == "pose")
                {
                    DUPLEX(dataType_, VAR_MATRIX3X4);
                    createInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
                }
                else if (type == "haptic")
                {
                    DUPLEX(dataType_, VAR_NONE);
                    DUPLEX(haptic_, true);
                    createInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
                }
                else
                {
                    URHO3D_LOGERRORF("Unknown XR action type: %s", type.c_str());
                    continue;
                }

                auto result = xrCreateAction(createSet, &createInfo, &binding->action_);
                if (result != XR_SUCCESS)
                {
                    URHO3D_LOGERRORF("Failed to create action %s because %s", name.c_str(), xrGetErrorStr(result));
                    continue;
                }

                if (binding->dataType_ == VAR_MATRIX3X4 || binding->dataType_ == VAR_VECTOR3)
                {
                    XrActionSpaceCreateInfo spaceInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
                    spaceInfo.action = binding->action_;
                    spaceInfo.poseInActionSpace = xrPoseIdentity;
                    if (handed)
                    {
                        spaceInfo.subactionPath = handPaths[0];
                        xrCreateActionSpace(session_, &spaceInfo, &binding->actionSpace_);
                        spaceInfo.subactionPath = handPaths[1];
                        xrCreateActionSpace(session_, &spaceInfo, &otherHand->actionSpace_);

                        if (child.GetBool("grip"))
                        {
                            binding->isPose_ = true;
                            otherHand->isPose_ = true;
                        }
                        else if (child.GetBool("aim"))
                        {
                            binding->isAimPose_ = true;
                            otherHand->isAimPose_ = true;
                        }
                    }
                    else
                        xrCreateActionSpace(session_, &spaceInfo, &binding->actionSpace_);
                }

                DUPLEX(set_, createSet);
                otherHand->action_ = binding->action_;

                actionSet->bindings_.push_back(binding);
                if (otherHand != binding)
                {
                    otherHand->responsibleForDelete_ = false;
                    actionSet->bindings_.push_back(otherHand);
                }
            }

#undef DUPLEX

            for (auto child = set.GetChild("profile"); child.NotNull(); child = child.GetNext("profile"))
            {
                ea::string device = child.GetAttribute("device");

                XrPath devicePath;
                xrStringToPath(instance_, device.c_str(), &devicePath);

                XrInteractionProfileSuggestedBinding suggest = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
                suggest.interactionProfile = devicePath;
                ea::vector<XrActionSuggestedBinding> bindings;

                for (auto bind = child.GetChild("bind"); bind.NotNull(); bind = bind.GetNext("bind"))
                {
                    ea::string action = bind.GetAttribute("action");
                    ea::string bindStr = bind.GetAttribute("path");

                    XrPath bindPath;
                    xrStringToPath(instance_, bindStr.c_str(), &bindPath);

                    XrActionSuggestedBinding b = { };
                    
                    for (auto found : actionSet->bindings_)
                    {
                        if (found->path_.comparei(action) == 0)
                        {
                            b.action = found->Cast<XRActionBinding>()->action_;
                            b.binding = bindPath;
                            bindings.push_back(b);
                            break;
                        }
                    }
                }

                if (!bindings.empty())
                {
                    suggest.countSuggestedBindings = bindings.size();
                    suggest.suggestedBindings = bindings.data();

                    auto res = xrSuggestInteractionProfileBindings(instance_, &suggest);
                    if (res != XR_SUCCESS)
                        URHO3D_LOGERRORF("Failed to suggest bindings: %s", xrGetErrorStr(res));
                }
            }
        }

        UpdateBindingBound();
    }

    void OpenXR::SetCurrentActionSet(SharedPtr<XRActionGroup> set)
    {
        if (session_ && set != nullptr)
        {
            auto xrSet = set->Cast<XRActionSet>();
            if (xrSet->actionSet_)
            {
                activeActionSet_ = set;

                XrSessionActionSetsAttachInfo attachInfo = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
                attachInfo.actionSets = &xrSet->actionSet_;
                attachInfo.countActionSets = 1;
                xrAttachSessionActionSets(session_, &attachInfo);

                UpdateBindingBound();
            }
        }
    }

    void OpenXR::UpdateBindings(float t)
    {
        if (instance_ == 0)
            return;

        if (!IsLive())
            return;

        auto& eventData = GetEventDataMap();
        using namespace VRBindingChange;

        eventData[VRBindingChange::P_ACTIVE] = true;

        for (auto b : activeActionSet_->bindings_)
        {
            auto bind = b->Cast<XRActionBinding>();
            if (bind->action_)
            {
                eventData[P_NAME] = bind->localizedName_;
                eventData[P_BINDING] = bind;

#define SEND_EVENT eventData[P_DATA] = bind->storedData_; eventData[P_DELTA] = bind->delta_; eventData[P_EXTRADELTA] = bind->extraDelta_[0]

                XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = bind->action_;
                getInfo.subactionPath = bind->subPath_;

                switch (bind->dataType_)
                {
                case VAR_BOOL: {
                    XrActionStateBoolean boolC = { XR_TYPE_ACTION_STATE_BOOLEAN };
                    if (xrGetActionStateBoolean(session_, &getInfo, &boolC) == XR_SUCCESS)
                    {
                        bind->active_ = boolC.isActive;
                        if (boolC.changedSinceLastSync)
                        {
                            bind->storedData_ = boolC.currentState;
                            bind->changed_ = true;
                            SEND_EVENT;
                        }
                        else
                            bind->changed_ = false;
                    }
                }
                    break;
                case VAR_FLOAT: {
                    XrActionStateFloat floatC = { XR_TYPE_ACTION_STATE_FLOAT };
                    if (xrGetActionStateFloat(session_, &getInfo, &floatC) == XR_SUCCESS)
                    {
                        bind->active_ = floatC.isActive;
                        if (floatC.changedSinceLastSync || !Equals(floatC.currentState, bind->GetFloat()))
                        {
                            bind->storedData_ = floatC.currentState;
                            bind->changed_ = true;
                            SEND_EVENT;
                        }
                        else
                            bind->changed_ = false;
                    }
                }
                    break;
                case VAR_VECTOR2: {
                    XrActionStateVector2f vec = { XR_TYPE_ACTION_STATE_VECTOR2F };
                    if (xrGetActionStateVector2f(session_, &getInfo, &vec) == XR_SUCCESS)
                    {
                        bind->active_ = vec.isActive;
                        Vector2 v(vec.currentState.x, vec.currentState.y);
                        if (vec.changedSinceLastSync)
                        {
                            bind->storedData_ = v;
                            bind->changed_ = true;
                            SEND_EVENT;
                        }
                        else
                            bind->changed_ = false;
                    }
                }
                    break;
                case VAR_VECTOR3: {
                    XrActionStatePose pose = { XR_TYPE_ACTION_STATE_POSE };
                    if (xrGetActionStatePose(session_, &getInfo, &pose) == XR_SUCCESS)
                    {
                        // Should we be sending events for these? As it's tracking sensor stuff I think not? It's effectively always changing and we know that's the case.
                        bind->active_ = pose.isActive;
                        Vector3 v = uxrGetVec(bind->location_.pose.position) * scaleCorrection_;
                        bind->storedData_ = v;
                        bind->changed_ = true;
                        bind->extraData_[0] = uxrGetVec(bind->velocity_.linearVelocity) * scaleCorrection_;
                    }
                } break;
                case VAR_MATRIX3X4: {
                    XrActionStatePose pose = { XR_TYPE_ACTION_STATE_POSE };
                    if (xrGetActionStatePose(session_, &getInfo, &pose) == XR_SUCCESS)
                    {
                        // Should we be sending events for these? As it's tracking sensor stuff I think not? It's effectively always changing and we know that's the case.
                        bind->active_ = pose.isActive;
                        Matrix3x4 m = uxrGetTransform(bind->location_.pose, scaleCorrection_);
                        bind->storedData_ = m;
                        bind->changed_ = true;
                        bind->extraData_[0] = uxrGetVec(bind->velocity_.linearVelocity) * scaleCorrection_;
                        bind->extraData_[1] = uxrGetVec(bind->velocity_.angularVelocity) * scaleCorrection_;
                    }
                } break;
                }
            }
        }

#undef SEND_EVENT
    }

    void OpenXR::GetHiddenAreaMask()
    {
        // extension wasn't supported
        if (!supportsMask_)
            return;

        for (int eye = 0; eye < 2; ++eye)
        {
            XrVisibilityMaskKHR mask = { XR_TYPE_VISIBILITY_MASK_KHR };
        // hidden
            {

                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &mask);

                ea::vector<XrVector2f> verts;
                verts.resize(mask.vertexCountOutput);
                ea::vector<unsigned> indices;
                indices.resize(mask.indexCountOutput);

                mask.vertexCapacityInput = verts.size();
                mask.indexCapacityInput = indices.size();

                mask.vertices = verts.data();
                mask.indices = indices.data();

                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &mask);

                ea::vector<Vector3> vtxData;
                vtxData.resize(verts.size());
                for (unsigned i = 0; i < verts.size(); ++i)
                    vtxData[i] = Vector3(verts[i].x, verts[i].y, 0.0f);

                VertexBuffer* vtx = new VertexBuffer(GetContext());
                vtx->SetSize(vtxData.size(), { VertexElement(TYPE_VECTOR3, SEM_POSITION) });
                vtx->SetData(vtxData.data());

                IndexBuffer* idx = new IndexBuffer(GetContext());
                idx->SetSize(indices.size(), true);
                idx->SetData(indices.data());

                hiddenAreaMesh_[eye] = new Geometry(GetContext());
                hiddenAreaMesh_[eye]->SetVertexBuffer(0, vtx);
                hiddenAreaMesh_[eye]->SetIndexBuffer(idx);
                hiddenAreaMesh_[eye]->SetDrawRange(TRIANGLE_LIST, 0, indices.size());
            }

        // visible
            {
                mask.indexCapacityInput = 0;
                mask.vertexCapacityInput = 0;
                mask.indices = nullptr;
                mask.vertices = nullptr;
                mask.indexCountOutput = 0;
                mask.vertexCountOutput = 0;
                
                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH_KHR, &mask);

                ea::vector<XrVector2f> verts;
                verts.resize(mask.vertexCountOutput);
                ea::vector<unsigned> indices;
                indices.resize(mask.indexCountOutput);

                mask.vertexCapacityInput = verts.size();
                mask.indexCapacityInput = indices.size();

                mask.vertices = verts.data();
                mask.indices = indices.data();

                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH_KHR, &mask);

                ea::vector<Vector3> vtxData;
                vtxData.resize(verts.size());
                for (unsigned i = 0; i < verts.size(); ++i)
                    vtxData[i] = Vector3(verts[i].x, verts[i].y, 0.0f);

                VertexBuffer* vtx = new VertexBuffer(GetContext());
                vtx->SetSize(vtxData.size(), { VertexElement(TYPE_VECTOR3, SEM_POSITION) });
                vtx->SetData(vtxData.data());

                IndexBuffer* idx = new IndexBuffer(GetContext());
                idx->SetSize(indices.size(), true);
                idx->SetData(indices.data());

                visibleAreaMesh_[eye] = new Geometry(GetContext());
                visibleAreaMesh_[eye]->SetVertexBuffer(0, vtx);
                visibleAreaMesh_[eye]->SetIndexBuffer(idx);
                visibleAreaMesh_[eye]->SetDrawRange(TRIANGLE_LIST, 0, indices.size());
            }

        // build radial from line loop, a centroid is calculated and the triangles are laid out in a fan
            {
                // Maybe do this several times for a couple of different sizes, to do strips that ring
                // the perimiter at different %s to save on overdraw. ie. ring 25%, ring 50%, center 25% and center 50%?
                // Then vignettes only need to do their work where actually required. A 25% distance outer ring is
                // in projected space massively smaller than 25% of FOV, likewise with a 50% outer ring, though less so.
                // Question is whether to ring in reference to centroid or to the line geometry as mitred?

                mask.indexCapacityInput = 0;
                mask.vertexCapacityInput = 0;
                mask.indices = nullptr;
                mask.vertices = nullptr;
                mask.indexCountOutput = 0;
                mask.vertexCountOutput = 0;

                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_LINE_LOOP_KHR, &mask);

                ea::vector<XrVector2f> verts;
                verts.resize(mask.vertexCountOutput);
                ea::vector<unsigned> indices;
                indices.resize(mask.indexCountOutput);

                mask.vertexCapacityInput = verts.size();
                mask.indexCapacityInput = indices.size();

                mask.vertices = verts.data();
                mask.indices = indices.data();

                xrGetVisibilityMaskKHR(session_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_LINE_LOOP_KHR, &mask);

                struct V {
                    Vector3 pos;
                    unsigned color;
                };

                ea::vector<V> vtxData;
                vtxData.resize(verts.size());
                Vector3 centroid = Vector3::ZERO;
                Vector3 minVec = Vector3(10000, 10000, 10000);
                Vector3 maxVec = Vector3(-10000, -10000, -10000);

                const unsigned whiteColor = Color::WHITE.ToUInt();
                const unsigned transWhiteColor = Color(1.0f, 1.0f, 1.0f, 0.0f).ToUInt();

                for (unsigned i = 0; i < verts.size(); ++i)
                {
                    vtxData[i] = { Vector3(verts[i].x, verts[i].y, 0.0f), whiteColor };
                    centroid += vtxData[i].pos;
                }
                centroid /= verts.size();

                ea::vector<unsigned short> newIndices;
                vtxData.push_back({ { centroid.x_, centroid.y_, 0.0f }, transWhiteColor });

                // turn the line loop into a fan
                for (unsigned i = 0; i < indices.size(); ++i)
                {
                    unsigned me = indices[i];
                    unsigned next = indices[(i + 1) % indices.size()];

                    newIndices.push_back(vtxData.size() - 1); // center is at the end
                    newIndices.push_back(me);
                    newIndices.push_back(next);
                }

                VertexBuffer* vtx = new VertexBuffer(GetContext());
                vtx->SetSize(vtxData.size(), { VertexElement(TYPE_VECTOR3, SEM_POSITION), VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR) });
                vtx->SetData(vtxData.data());

                IndexBuffer* idx = new IndexBuffer(GetContext());
                idx->SetSize(newIndices.size(), false);
                idx->SetData(newIndices.data());

                radialAreaMesh_[eye] = new Geometry(GetContext());
                radialAreaMesh_[eye]->SetVertexBuffer(0, vtx);
                radialAreaMesh_[eye]->SetIndexBuffer(idx);
                radialAreaMesh_[eye]->SetDrawRange(TRIANGLE_LIST, 0, newIndices.size());
            }
        }
    }

    void OpenXR::LoadControllerModels()
    {
        if (!supportsControllerModel_ || !IsLive())
            return;

        XrPath handPaths[2];
        xrStringToPath(instance_, "/user/hand/left", &handPaths[0]);
        xrStringToPath(instance_, "/user/hand/right", &handPaths[1]);

        XrControllerModelKeyStateMSFT states[2] = { { XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT }, { XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT } };
        XrResult errCodes[2];
        errCodes[0] = xrGetControllerModelKeyMSFT(session_, handPaths[0], &states[0]);
        errCodes[1] = xrGetControllerModelKeyMSFT(session_, handPaths[1], &states[1]);

        for (int i = 0; i < 2; ++i)
        {
            // skip if we're the same, we could change
            if (states[i].modelKey == wandModels_[i].modelKey_)
                continue;

            wandModels_[i].modelKey_ = states[i].modelKey;

            if (errCodes[i] == XR_SUCCESS)
            {
                unsigned dataSize = 0;
                auto loadErr = xrLoadControllerModelMSFT(session_, states[i].modelKey, 0, &dataSize, nullptr);
                if (loadErr == XR_SUCCESS)
                {
                    std::vector<unsigned char> data;
                    data.resize(dataSize);

                    // Can we actually fail in this case if the above was successful? Assuming that data/data-size are correct I would expect not?
                    if (xrLoadControllerModelMSFT(session_, states[i].modelKey, data.size(), &dataSize, data.data()) == XR_SUCCESS)
                    {
                        tinygltf::Model model;
                        tinygltf::TinyGLTF ctx;
                        tinygltf::Scene scene;

                        std::string err, warn;
                        if (ctx.LoadBinaryFromMemory(&model, &err, &warn, data.data(), data.size()))
                        {
                            wandModels_[i].model_ = LoadGLTFModel(GetContext(), model);
                        }
                        else
                            wandModels_[i].model_.Reset();

                        XR_INIT_TYPE(wandModels_[i].properties_, XR_TYPE_CONTROLLER_MODEL_NODE_PROPERTIES_MSFT);

                        XrControllerModelPropertiesMSFT props = { XR_TYPE_CONTROLLER_MODEL_PROPERTIES_MSFT };
                        props.nodeCapacityInput = 256;
                        props.nodeCountOutput = 0;
                        props.nodeProperties = wandModels_[i].properties_;
                        if (xrGetControllerModelPropertiesMSFT(session_, states[i].modelKey, &props) == XR_SUCCESS)
                        {
                            wandModels_[i].numProperties_ = props.nodeCountOutput;
                        }
                        else
                            wandModels_[i].numProperties_ = 0;

                        auto& data = GetEventDataMap();
                        data[VRControllerChange::P_HAND] = i;
                        SendEvent(E_VRCONTROLLERCHANGE, data);
                    }
                }
                else
                    URHO3D_LOGERRORF("xrLoadControllerModelMSFT failure: %s", xrGetErrorStr(errCodes[i]));
            }
            else
                URHO3D_LOGERRORF("xrGetControllerModelKeyMSFT failure: %s", xrGetErrorStr(errCodes[i]));
        }
    }

    SharedPtr<Node> OpenXR::GetControllerModel(VRHand hand)
    {
        return wandModels_[hand].model_ != nullptr ? wandModels_[hand].model_ : nullptr;
    }

    void OpenXR::UpdateControllerModel(VRHand hand, SharedPtr<Node> model)
    {
        if (!supportsControllerModel_)
            return;

        if (model == nullptr)
            return;

        if (wandModels_[hand].modelKey_ == 0)
            return;

        // nothing to animate
        if (wandModels_[hand].numProperties_ == 0)
            return;

        XrControllerModelNodeStateMSFT nodeStates[256];
        XR_INIT_TYPE(nodeStates, XR_TYPE_CONTROLLER_MODEL_NODE_STATE_MSFT);

        XrControllerModelStateMSFT state = { XR_TYPE_CONTROLLER_MODEL_STATE_MSFT };
        state.nodeCapacityInput = 256;
        state.nodeStates = nodeStates;

        auto errCode = xrGetControllerModelStateMSFT(session_, wandModels_[hand].modelKey_, &state);
        if (errCode == XR_SUCCESS)
        {
            auto node = model;
            for (unsigned i = 0; i < state.nodeCountOutput; ++i)
            {
                SharedPtr<Node> bone;

                // If we've got a parent name, first seek that out. OXR allows name collisions, parent-name disambiguates.
                if (strlen(wandModels_[hand].properties_[i].parentNodeName))
                {
                    if (auto parent = node->GetChild(wandModels_[hand].properties_[i].parentNodeName, true))
                        bone = parent->GetChild(wandModels_[hand].properties_[i].nodeName);
                }
                else
                    bone = node->GetChild(wandModels_[hand].properties_[i].nodeName, true);
                
                if (bone != nullptr)
                {
                    // we have a 1,1,-1 scale at the root to flip gltf coordinate system to ours,
                    // because of that this transform needs to be direct and not converted, or it'll get unconverted
                    // TODO: figure out how to properly fully flip the gltf nodes and vertices
                    Vector3 t = Vector3(nodeStates[i].nodePose.position.x, nodeStates[i].nodePose.position.y, nodeStates[i].nodePose.position.z);
                    auto& q = nodeStates[i].nodePose.orientation;
                    Quaternion outQ = Quaternion(q.w, q.x, q.y, q.z);

                    bone->SetTransform(Matrix3x4(t, outQ, Vector3(1,1,1)));
                }
            }
        }
    }

    void OpenXR::TriggerHaptic(VRHand hand, float durationSeconds, float cyclesPerSec, float amplitude)
    {
        if (activeActionSet_)
        {
            // Consider memoizing? We're realistically only going to have like 15 or so actions in a set.
            for (auto b : activeActionSet_->bindings_)
            {
                if (b->IsHaptic() && b->Hand() == hand)
                    b->Vibrate(durationSeconds, cyclesPerSec, amplitude);
            }
        }
    }

    Matrix3x4 OpenXR::GetHandTransform(VRHand hand) const
    {
        if (hand == VR_HAND_NONE)
            return Matrix3x4();

        if (!handGrips_[hand])
            return Matrix3x4();

        auto q = uxrGetQuat(handGrips_[hand]->location_.pose.orientation);
        auto v = uxrGetVec(handGrips_[hand]->location_.pose.position);

        // bring it into head space instead of stage space
        auto headInv = GetHeadTransform().Inverse();
        return headInv * Matrix3x4(v, q, 1.0f);
    }

    Matrix3x4 OpenXR::GetHandAimTransform(VRHand hand) const
    {
        if (hand == VR_HAND_NONE)
            return Matrix3x4();

        if (!handAims_[hand])
            return Matrix3x4();

        // leave this in stage space, that's what we want
        auto q = uxrGetQuat(handAims_[hand]->location_.pose.orientation);
        auto v = uxrGetVec(handAims_[hand]->location_.pose.position);
        return Matrix3x4(v, q, 1.0f);
    }

    Ray OpenXR::GetHandAimRay(VRHand hand) const
    {
        if (hand == VR_HAND_NONE)
            return Ray();

        if (!handAims_[hand])
            return Ray();

        // leave this one is stage space, that's what we want
        auto q = uxrGetQuat(handAims_[hand]->location_.pose.orientation);
        auto v = uxrGetVec(handAims_[hand]->location_.pose.position);
        return Ray(v, (q * Vector3(0, 0, 1)).Normalized());
    }
    
    void OpenXR::GetHandVelocity(VRHand hand, Vector3* linear, Vector3* angular) const
    {
        if (hand == VR_HAND_NONE)
            return;

        if (!handGrips_[hand])
            return;
        
        if (linear && handGrips_[hand]->velocity_.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
            *linear = uxrGetVec(handGrips_[hand]->velocity_.linearVelocity);
        if (angular && handGrips_[hand]->velocity_.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
            *angular = uxrGetVec(handGrips_[hand]->velocity_.angularVelocity);
    }

    void OpenXR::UpdateHands(Scene* scene, Node* rigRoot, Node* leftHand, Node* rightHand)
    {
        if (!IsLive())
            return;

        // Check for changes in controller model state, if so, do reload as required.
        LoadControllerModels();

        if (leftHand == nullptr)
            leftHand = rigRoot->CreateChild("Left_Hand");
        if (rightHand == nullptr)
            rightHand = rigRoot->CreateChild("Right_Hand");

        // we need valid handles for these guys
        if (handGrips_[0] && handGrips_[1])
        {
            // TODO: can we do any tracking of our own such as using QEF for tracking recent velocity integration into position confidence
            // over the past interval of time to decide how much we trust integrating velocity when position has no-confidence / untracked.
            // May be able to fall-off a confidence factor provided the incoming velocity is still there, problem is how to rectify
            // when tracking kicks back in again later. If velocity integration is valid there should be no issue - neither a pop,
            // it'll already pop in a normal position tracking lost recovery situation anyways.

            auto lq = uxrGetQuat(handGrips_[VR_HAND_LEFT]->location_.pose.orientation);
            auto lp = uxrGetVec(handGrips_[VR_HAND_LEFT]->location_.pose.position);

            // these fields are super important to rationalize what's happpened between sample points
            // sensor reads are effectively Planck timing it between quantum space-time
            static const ea::string lastTrans = "LastTransform";
            static const ea::string lastTransWS = "LastTransformWS";

            leftHand->SetVar(lastTrans, leftHand->GetTransform());
            leftHand->SetVar(lastTransWS, leftHand->GetWorldTransform());
            leftHand->SetEnabled(handGrips_[VR_HAND_LEFT]->location_.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT));
            leftHand->SetPosition(lp);
            if (handGrips_[VR_HAND_LEFT]->location_.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))
                leftHand->SetRotation(lq);

            auto rq = uxrGetQuat(handGrips_[VR_HAND_RIGHT]->location_.pose.orientation);
            auto rp = uxrGetVec(handGrips_[VR_HAND_RIGHT]->location_.pose.position);

            rightHand->SetVar(lastTrans, leftHand->GetTransform());
            rightHand->SetVar(lastTransWS, leftHand->GetWorldTransform());
            rightHand->SetEnabled(handGrips_[VR_HAND_RIGHT]->location_.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT));
            rightHand->SetPosition(rp);
            if (handGrips_[VR_HAND_RIGHT]->location_.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))
                rightHand->SetRotation(rq);
        }
    }

    Matrix3x4 OpenXR::GetEyeLocalTransform(VREye eye) const
    {
        // TODO: fixme, why is view space not correct xrLocateViews( view-space )
        // one would expect them to be in head relative local space already ... but they're ... not?
        return GetHeadTransform().Inverse() * uxrGetTransform(views_[eye].pose, scaleCorrection_);
    }

    Matrix4 OpenXR::GetProjection(VREye eye, float nearDist, float farDist) const
    {
        return uxrGetProjection(nearDist, farDist, views_[eye].fov.angleLeft, views_[eye].fov.angleUp, views_[eye].fov.angleRight, views_[eye].fov.angleDown);
    }

    Matrix3x4 OpenXR::GetHeadTransform() const
    {
        return uxrGetTransform(headLoc_.pose, scaleCorrection_);
    }

    OpenXR::XRActionBinding::~XRActionBinding()
    {
        if (responsibleForDelete_ && action_)
            xrDestroyAction(action_);
        if (actionSpace_)
            xrDestroySpace(actionSpace_);
        action_ = 0;
    }

    OpenXR::XRActionSet::~XRActionSet()
    {
        bindings_.clear();
        if (actionSet_)
            xrDestroyActionSet(actionSet_);
        actionSet_ = 0;
    }

    void OpenXR::XRActionBinding::Vibrate(float duration, float freq, float amplitude)
    {
        if (!xr_->IsLive())
            return;

        XrHapticActionInfo info = { XR_TYPE_HAPTIC_ACTION_INFO };
        info.action = action_;
        info.subactionPath = subPath_;

        XrHapticVibration vib = { XR_TYPE_HAPTIC_VIBRATION };
        vib.amplitude = amplitude;
        vib.frequency = freq;
        vib.duration = duration * 1000.0f;
        xrApplyHapticFeedback(xr_->session_, &info, (XrHapticBaseHeader*)&vib);
    }

    void OpenXR::UpdateBindingBound()
    {
        if (session_ == 0)
            return;

        if (activeActionSet_)
        {
            for (auto b : activeActionSet_->bindings_)
            {
                auto bind = b->Cast<XRActionBinding>();
                XrBoundSourcesForActionEnumerateInfo info = { XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO };
                info.action = bind->action_;
                unsigned binds = 0;
                xrEnumerateBoundSourcesForAction(session_, &info, 0, &binds, nullptr);
                b->isBound_ = binds > 0;

                if (b->isAimPose_)
                    handAims_[b->Hand()] = b->Cast<XRActionBinding>();
                if (b->isPose_)
                    handGrips_[b->Hand()] = b->Cast<XRActionBinding>();
            }
        }
    }

    void GLTFRecurseModel(Context* ctx, tinygltf::Model& gltf, Node* parent, int nodeIndex, int parentIndex, Material* mat, Matrix3x4 matStack)
    {
        auto& n = gltf.nodes[nodeIndex];

        auto node = parent->CreateChild(n.name.c_str());

        // root node will deal with the 1,1,-1 - so just accept the transforms we get
        // same with vertex data later
        if (n.translation.size())
        {
            Vector3 translation = Vector3(n.translation[0], n.translation[1], n.translation[2]);
            Quaternion rotation = Quaternion(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]);
            Vector3 scale = Vector3(n.scale[0], n.scale[1], n.scale[2]);
            node->SetPosition(translation);
            node->SetRotation(rotation);
            node->SetScale(scale);
        }
        else if (n.matrix.size())
        {
            Matrix3x4 mat = Matrix3x4(
                n.matrix[0], n.matrix[4], n.matrix[8], n.matrix[12],
                n.matrix[1], n.matrix[5], n.matrix[9], n.matrix[13],
                n.matrix[2], n.matrix[6], n.matrix[10], n.matrix[14]
            );
            node->SetTransform(mat);
        }
        else
            node->SetTransform(Matrix3x4::IDENTITY);

        if (n.mesh != -1)
        {
            auto& mesh = gltf.meshes[n.mesh];
            BoundingBox bounds;
            bounds.Clear();
            for (auto& prim : mesh.primitives)
            {
                SharedPtr<Geometry> geom(new Geometry(ctx));

                if (prim.mode == TINYGLTF_MODE_TRIANGLES)
                {
                    SharedPtr<IndexBuffer> idxBuffer(new IndexBuffer(ctx));
                    ea::vector< SharedPtr<VertexBuffer> > vertexBuffers;

                    struct Vertex {
                        Vector3 pos;
                        Vector3 norm;
                        Vector2 tex;
                    };

                    ea::vector<Vertex> verts;
                    verts.resize(gltf.accessors[prim.attributes.begin()->second].count);

                    for (auto c : prim.attributes)
                    {
                        // only known case at the present
                        if (gltf.accessors[c.second].componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            auto& access = gltf.accessors[c.second];
                            auto& view = gltf.bufferViews[access.bufferView];
                            auto& buffer = gltf.buffers[view.buffer];

                            LegacyVertexElement element;

                            ea::string str(access.name.c_str());
                            if (str.contains("position", false))
                                element = ELEMENT_POSITION;
                            else if (str.contains("texcoord", false))
                                element = ELEMENT_TEXCOORD1;
                            else if (str.contains("normal", false))
                                element = ELEMENT_NORMAL;

                            SharedPtr<VertexBuffer> vtx(new VertexBuffer(ctx));

                            size_t sizeElem = access.type == TINYGLTF_TYPE_VEC2 ? sizeof(Vector2) : sizeof(Vector3);
                            if (access.type == TINYGLTF_TYPE_VEC3)
                            {
                                const float* d = (const float*)&buffer.data[view.byteOffset + access.byteOffset];
                                if (element == ELEMENT_NORMAL)
                                {
                                    for (unsigned i = 0; i < access.count; ++i)
                                        verts[i].norm = Vector3(d[i * 3 + 0], d[i * 3 + 1], d[i * 3 + 2]);
                                }
                                else if (element == ELEMENT_POSITION)
                                {
                                    for (unsigned i = 0; i < access.count; ++i)
                                        bounds.Merge(verts[i].pos = Vector3(d[i * 3 + 0], d[i * 3 + 1], d[i * 3 + 2]));
                                }
                            }
                            else
                            {
                                const float* d = (const float*)&buffer.data[view.byteOffset + access.byteOffset];
                                for (unsigned i = 0; i < access.count; ++i)
                                    verts[i].tex = Vector2(d[i * 2 + 0], d[i * 2 + 1]);
                            }
                        }
                        else
                            URHO3D_LOGERRORF("Found unsupported GLTF component type for vertex data: %u", gltf.accessors[prim.indices].componentType);
                    }

                    VertexBuffer* buff = new VertexBuffer(ctx);
                    buff->SetSize(verts.size(), { VertexElement(TYPE_VECTOR3, SEM_POSITION, 0, 0), VertexElement(TYPE_VECTOR3, SEM_NORMAL, 0, 0), VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0, 0) });
                    buff->SetData(verts.data());
                    vertexBuffers.push_back(SharedPtr<VertexBuffer>(buff));

                    if (prim.indices != -1)
                    {
                        auto& access = gltf.accessors[prim.indices];
                        auto& view = gltf.bufferViews[access.bufferView];
                        auto& buffer = gltf.buffers[view.buffer];

                        if (gltf.accessors[prim.indices].componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                        {
                            ea::vector<unsigned> indexData;
                            indexData.resize(access.count);

                            const unsigned* indices = (const unsigned*)&buffer.data[view.byteOffset + access.byteOffset];
                            for (int i = 0; i < access.count; ++i)
                                indexData[i] = indices[i];

                            idxBuffer->SetSize(access.count, true, false);
                            idxBuffer->SetData(indexData.data());
                        }
                        else if (gltf.accessors[prim.indices].componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        {
                            ea::vector<unsigned short> indexData;
                            indexData.resize(access.count);

                            const unsigned short* indices = (const unsigned short*)&buffer.data[view.byteOffset + access.byteOffset];
                            for (int i = 0; i < access.count; ++i)
                                indexData[i] = indices[i];
                            for (int i = 0; i < indexData.size(); i += 3)
                            {
                                ea::swap(indexData[i], indexData[i + 2]);
                            }

                            idxBuffer->SetSize(access.count, false, false);
                            idxBuffer->SetData(indexData.data());
                        }
                        else
                        {
                            URHO3D_LOGERRORF("Found unsupported GLTF component type for index data: %u", gltf.accessors[prim.indices].componentType);
                            continue;
                        }
                    }

                    SharedPtr<Geometry> geom(new Geometry(ctx));
                    geom->SetIndexBuffer(idxBuffer);
                    geom->SetNumVertexBuffers(vertexBuffers.size());
                    for (unsigned i = 0; i < vertexBuffers.size(); ++i)
                        geom->SetVertexBuffer(i, vertexBuffers[0]);
                    geom->SetDrawRange(TRIANGLE_LIST, 0, idxBuffer->GetIndexCount(), false);

                    SharedPtr<Model> m(new Model(ctx));
                    m->SetNumGeometries(1);
                    m->SetGeometry(0, 0, geom);
                    m->SetName(mesh.name.c_str());
                    m->SetBoundingBox(bounds);

                    auto sm = node->CreateComponent<StaticModel>();
                    sm->SetModel(m);
                    sm->SetMaterial(mat);
                }
            }
        }

        for (auto child : n.children)
            GLTFRecurseModel(ctx, gltf, node, child, nodeIndex, mat, node->GetWorldTransform());
    }

    SharedPtr<Texture2D> LoadGLTFTexture(Context* ctx, tinygltf::Model& gltf, int index)
    {
        auto gfx = ctx->GetSubsystem<Graphics>();

        auto img = gltf.images[index];
        SharedPtr<Texture2D> tex(new Texture2D(ctx));
        tex->SetSize(img.width, img.height, gfx->GetRGBAFormat());

        auto view = gltf.bufferViews[img.bufferView];

        MemoryBuffer buff(gltf.buffers[view.buffer].data.data() + view.byteOffset, view.byteLength);

        Image image(ctx);
        if (image.Load(buff))
        {
            tex->SetData(&image, true);
            return tex;
        }
        
        return nullptr;
    }

    SharedPtr<Node> LoadGLTFModel(Context* ctx, tinygltf::Model& gltf)
    {
        if (gltf.scenes.empty())
            return SharedPtr<Node>();

        // cloning because controllers could change or possibly even not be the same on each hand
        SharedPtr<Material> material = ctx->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/XRController.xml")->Clone();
        if (!gltf.materials.empty() && !gltf.textures.empty())
        {
            material->SetTexture(TU_DIFFUSE, LoadGLTFTexture(ctx, gltf, 0));
            if (gltf.materials[0].normalTexture.index)
                material->SetTexture(TU_NORMAL, LoadGLTFTexture(ctx, gltf, gltf.materials[0].normalTexture.index));
        }

        auto scene = gltf.scenes[gltf.defaultScene];
        SharedPtr<Node> root(new Node(ctx));
        root->SetScale(Vector3(1, 1, -1));
        //root->Rotate(Quaternion(45, Vector3::UP));
        for (auto n : scene.nodes)
            GLTFRecurseModel(ctx, gltf, root, n, -1, material, Matrix3x4::IDENTITY);

        return root;
    }

}
