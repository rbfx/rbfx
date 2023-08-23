// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Variant.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/XR/VRInterface.h"

#include <ThirdParty/OpenXRSDK/include/openxr/openxr.h>

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

class AnimatedModel;
class Texture2D;
class XMLFile;

template <class T> class XrObjectPtr
{
public:
    XrObjectPtr() = default;
    XrObjectPtr(std::nullptr_t) {}

    template <class U> XrObjectPtr(T object, U deleter)
    {
        const auto wrappedDeleter = [deleter](T* ptr)
        {
            deleter(*ptr);
            delete ptr;
        };

        ptr_ = ea::shared_ptr<T>{new T{object}, wrappedDeleter};
    }

    T get() const { return ptr_ ? *ptr_ : T{}; }

    operator bool() const { return ptr_ && *ptr_; }

private:
    ea::shared_ptr<T> ptr_;
};

using XrInstancePtr = XrObjectPtr<XrInstance>;
using XrDebugUtilsMessengerEXTPtr = XrObjectPtr<XrDebugUtilsMessengerEXT>;
using XrSessionPtr = XrObjectPtr<XrSession>;
using XrSwapchainPtr = XrObjectPtr<XrSwapchain>;

/// Tweaks that should be applied before graphics initialization.
struct OpenXRTweaks
{
    StringVector vulkanInstanceExtensions_;
    StringVector vulkanDeviceExtensions_;
    unsigned adapterId_{};
    ea::optional<ea::string> orientation_;
};

/// Interface that wraps OpenXR swap chain and integrates it with the engine rendering API.
class OpenXRSwapChain
{
public:
    virtual ~OpenXRSwapChain() = default;

    Texture2D* GetTexture(unsigned index) const { return textures_[index]; }
    unsigned GetNumTextures() const { return textures_.size(); }
    TextureFormat GetFormat() const { return format_; }
    XrSwapchain GetHandle() const { return swapChain_.get(); }

protected:
    OpenXRSwapChain() = default;

    ea::vector<SharedPtr<Texture2D>> textures_;
    unsigned arraySize_{1}; // note: if eventually supported array targets this will change
    TextureFormat format_;
    XrSwapchainPtr swapChain_;
};

using OpenXRSwapChainPtr = ea::shared_ptr<OpenXRSwapChain>;

/**

Register as a subsystem, Initialize sometime after GFX has been initialized but before Audio is initialized ...
otherwise it won't get the right audio target. Or reinit audio querying for the default device.

Currently setup around a VR experience. Minor changes will be required for additive / HoloLens. Mobile AR
would be best done with another class that's pared down to the specifics that apply instead of trying to make
this class into a monster.

Expectations for the VR-Rig:

Scene
    - "VRRig" NETWORKED, this is effectively the tracking volume center floor
        - "Head" NETWORKED
            - "Left_Eye" LOCAL
                - Camera
            - "Right_Eye" LOCAL
                - Camera
        - "Left_Hand" NETWORKED, will have enabled status set based on controller availability
            - StaticModel[0] = controller model
        - "Right_Hand" NETWORKED, will have enabled status set based on controller availability
            - StaticModel[0] = controller model

To-Do:
- Trackers
- Multiple Action-Sets
- Hand Skeleton

*/
class URHO3D_API OpenXR : public VRInterface
{
    URHO3D_OBJECT(OpenXR, VRInterface);

public:
    OpenXR(Context*);
    virtual ~OpenXR();

    /// Initialize the OpenXR subsystem. Renderer backend is not yet initialized at this point.
    bool InitializeSystem(RenderBackend backend);

    virtual VRRuntime GetRuntime() const override { return VRRuntime::OPENXR; }
    virtual const char* GetRuntimeName() const override { return "OPEN_XR"; }

    virtual bool InitializeSession(const VRSessionParameters& params) override;
    virtual void ShutdownSession() override;

    // XR is currently single-texture only.
    virtual void SetSingleTexture(bool state) override {}

    /// XR is successfully initialized. Session may not be live though.
    virtual bool IsConnected() const override { return instance_ && session_; }
    /// XR is successfully initialized and our session is active.
    virtual bool IsLive() const override { return session_ && sessionLive_; }

    /// Attempt a haptic vibration targeting a hand, just hides action query for convenience.
    virtual void TriggerHaptic(VRHand hand, float durationSeconds, float cyclesPerSec, float amplitude) override;

    virtual Matrix3x4 GetHandTransform(VRHand) const override;
    virtual Matrix3x4 GetHandAimTransform(VRHand hand) const override;
    virtual Ray GetHandAimRay(VRHand) const override;
    virtual void GetHandVelocity(VRHand hand, Vector3* linear, Vector3* angular) const override;
    virtual Matrix3x4 GetEyeLocalTransform(VREye eye) const override;
    virtual Matrix4 GetProjection(VREye eye, float nearDist, float farDist) const override;
    virtual Matrix3x4 GetHeadTransform() const override;

    virtual void UpdateHands(Scene* scene, Node* rigRoot, Node* leftHand, Node* rightHand) override;

    void HandlePreUpdate(StringHash, VariantMap&);
    void HandlePreRender();
    void HandlePostRender(StringHash, VariantMap&);

    virtual void BindActions(SharedPtr<XMLFile>);
    /// Sets the current action set.
    virtual void SetCurrentActionSet(SharedPtr<XRActionGroup>) override;

    SharedPtr<Node> GetControllerModel(VRHand hand);
    void UpdateControllerModel(VRHand hand, SharedPtr<Node>);

    const OpenXRTweaks& GetTweaks() const { return tweaks_; }
    const StringVector GetExtensions() const { return supportedExtensions_; }
    void SetUserExtensions(const StringVector& ext) { userExtensions_ = ext; }

protected:
    void InitializeActiveExtensions(RenderBackend backend);
    bool InitializeTweaks(RenderBackend backend);

    bool OpenSession();
    void UpdateBindings(float time);
    void UpdateBindingBound();
    void GetHiddenAreaMask();
    /// Attempts to load controller models, note that this can only be done if there are grip actions bound for some
    /// reason.
    void LoadControllerModels();

    StringVector supportedExtensions_;
    StringVector userExtensions_;
    StringVector activeExtensions_;

    struct ExtensionFeatures
    {
        bool debugOutput_{};
        bool visibilityMask_{};
        bool controllerModel_{};
        bool depthLayer_{};
    } features_;

    SharedPtr<XMLFile> manifest_;

    XrInstancePtr instance_;
    XrDebugUtilsMessengerEXTPtr debugMessenger_;
    XrSystemId system_{};
    OpenXRTweaks tweaks_;

    XrSessionPtr session_;
    OpenXRSwapChainPtr swapChain_;
    OpenXRSwapChainPtr depthChain_;
    XrView views_[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};

    // Pointless head-space.
    XrSpace headSpace_ = {};
    XrSpace viewSpace_ = {};
    /// Location tracking of the head.
    XrSpaceLocation headLoc_ = {XR_TYPE_SPACE_LOCATION};
    /// Velocity tracking information of the head.
    XrSpaceVelocity headVel_ = {XR_TYPE_SPACE_VELOCITY};

    /// Blending mode the compositor will be told to use. Assumed that when not opaque the correct mode will be be
    /// received from querying.
    XrEnvironmentBlendMode blendMode_ = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
    /// Predicted time for display of the next frame.
    XrTime predictedTime_ = {};
    /// Whether the session is currently active or not.
    bool sessionLive_ = false;

    struct ControllerModel
    {
        XrControllerModelKeyMSFT modelKey_ = 0;
        SharedPtr<Node> model_;
        XrControllerModelNodePropertiesMSFT properties_[256];
        unsigned numProperties_ = 0;
    };

    /// Loaded wand model mesh and texture data.
    ControllerModel wandModels_[2];

    class XRActionBinding : public XRBinding
    {
        URHO3D_OBJECT(XRActionBinding, XRBinding);

    public:
        XRActionBinding(Context* ctx, OpenXR* xr)
            : XRBinding(ctx)
            , xr_(xr)
        {
        }
        virtual ~XRActionBinding();

        /// If haptic this will trigger a vibration.
        virtual void Vibrate(float duration, float freq, float amplitude) override;

        /// Reference to owning OpenXR instance.
        OpenXR* xr_;
        /// Action itself, possibly shared in the case of sub-path handed actions.
        XrAction action_ = {};
        /// Owning actionset that contains this action.
        XrActionSet set_ = {};
        /// Indicates handed-ness for the OXR query.
        XrPath subPath_ = XR_NULL_PATH;
        /// If we're a space action we'll have an action space.
        XrSpace actionSpace_ = {};

        /// Position and orientation from space location.
        XrSpaceLocation location_ = {XR_TYPE_SPACE_LOCATION};
        /// Linear and Angular velocity from space location.
        XrSpaceVelocity velocity_ = {XR_TYPE_SPACE_VELOCITY};
        /// only 1 of the subpath handlers will do deletion, this indicates who will do it.
        bool responsibleForDelete_ = true;
    };

    class XRActionSet : public XRActionGroup
    {
        URHO3D_OBJECT(XRActionSet, XRActionGroup)
    public:
        XRActionSet(Context* ctx)
            : XRActionGroup(ctx)
        {
        }
        virtual ~XRActionSet();

        XrActionSet actionSet_ = {};
    };

    /// Cached grip pose bindings to avoid constant queries.
    SharedPtr<XRActionBinding> handGrips_[2];
    /// Cached aim pose bindings to avoid constant queries.
    SharedPtr<XRActionBinding> handAims_[2];
    /// Cached haptic outputs to avoid constant queries.
    SharedPtr<XRActionBinding> handHaptics_[2];
};

} // namespace Urho3D
