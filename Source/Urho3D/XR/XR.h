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

#pragma once

#include "../XR/VRInterface.h"
#include "../Graphics/Model.h"
#include "../Core/Variant.h"

#include <ThirdParty/OpenXRSDK/include/openxr/openxr.h>

namespace Urho3D
{
class AnimatedModel;
class XMLFile;

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

    virtual VRRuntime GetRuntime() const override { return VRRuntime::OPENXR; }
    virtual const char* GetRuntimeName() const override { return "OPEN_XR"; }

    virtual void QueryExtensions();

    virtual bool Initialize(const ea::string& manifestPath) override;
    virtual void Shutdown() override;

    // XR is currently single-texture only.
    virtual void SetSingleTexture(bool state) override { }

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

    virtual void CreateEyeTextures() override;

    void HandlePreUpdate(StringHash, VariantMap&);
    void HandlePreRender();
    void HandlePostRender(StringHash, VariantMap&);

    virtual void BindActions(SharedPtr<XMLFile>);
    /// Sets the current action set.
    virtual void SetCurrentActionSet(SharedPtr<XRActionGroup>) override;

    SharedPtr<Node> GetControllerModel(VRHand hand);
    void UpdateControllerModel(VRHand hand, SharedPtr<Node>);

    const StringVector GetExtensions() const { return extensions_; }
    void SetExtraExtensions(const StringVector& ext) { extraExtensions_ = ext; }

protected:
    bool OpenSession();
    bool CreateSwapchain();
    void CloseSession();
    void DestroySwapchain();
    void UpdateBindings(float time);
    void UpdateBindingBound();
    void GetHiddenAreaMask();
    /// Attempts to load controller models, note that this can only be done if there are grip actions bound for some reason.
    void LoadControllerModels();

    SharedPtr<XMLFile> manifest_;
    XrInstance instance_ = { };
    XrSystemId system_ = { };
    XrSession session_ = { };
    XrSwapchain swapChain_ = { };
    XrSwapchain depthChain_ = { };
    XrView views_[2] = { { XR_TYPE_VIEW }, { XR_TYPE_VIEW } };
    XrDebugUtilsMessengerEXT messenger_ = { };

    // OXR headers are a complete mess, getting platform specific object defs creates a mess of platform includes, including "WIN32_EXTRA_FAT"
    struct Opaque;
    /// Contains hidden objects that are obscured due to header inclusion complexities with #defines.
    ea::unique_ptr<Opaque> opaque_;

    SharedPtr<Texture2D> eyeColorTextures_[4];
    SharedPtr<Texture2D> eyeDepthTextures_[4];
    unsigned imgCount_ = { };
    unsigned depthImgCount_{ };

    // Pointless head-space.
    XrSpace headSpace_ = { };   
    XrSpace viewSpace_ = { };
    /// Location tracking of the head.
    XrSpaceLocation headLoc_ = { XR_TYPE_SPACE_LOCATION };
    /// Velocity tracking information of the head.
    XrSpaceVelocity headVel_ = { XR_TYPE_SPACE_VELOCITY };

    /// Blending mode the compositor will be told to use. Assumed that when not opaque the correct mode will be be received from querying.
    XrEnvironmentBlendMode blendMode_ = { XR_ENVIRONMENT_BLEND_MODE_OPAQUE  };
    /// Predicted time for display of the next frame.
    XrTime predictedTime_ = { };
    /// Whether the session is currently active or not.
    bool sessionLive_ = false;
    /// Indicates whether visibility mask is supported.
    bool supportsMask_ = false;
    /// Indicates that controller model is supported.
    bool supportsControllerModel_ = false;
    /// Indicates that we support the depth extension so we can provide that to help out time-warp.
    bool supportsDepthExt_ = false;

    struct ControllerModel {
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
        XRActionBinding(Context* ctx, OpenXR* xr) : XRBinding(ctx), xr_(xr) { }
        virtual ~XRActionBinding();

        /// If haptic this will trigger a vibration.
        virtual void Vibrate(float duration, float freq, float amplitude) override;

        /// Reference to owning OpenXR instance.
        OpenXR* xr_;
        /// Action itself, possibly shared in the case of sub-path handed actions.
        XrAction action_ = { };
        /// Owning actionset that contains this action.
        XrActionSet set_ = { };
        /// Indicates handed-ness for the OXR query.
        XrPath subPath_ = XR_NULL_PATH;
        /// If we're a space action we'll have an action space.
        XrSpace actionSpace_ = { };

        /// Position and orientation from space location.
        XrSpaceLocation location_ = { XR_TYPE_SPACE_LOCATION };
        /// Linear and Angular velocity from space location.
        XrSpaceVelocity velocity_ = { XR_TYPE_SPACE_VELOCITY };
        /// only 1 of the subpath handlers will do deletion, this indicates who will do it.
        bool responsibleForDelete_ = true;
    };

    class XRActionSet : public XRActionGroup
    {
        URHO3D_OBJECT(XRActionSet, XRActionGroup)
    public:
        XRActionSet(Context* ctx) : XRActionGroup(ctx) { }
        virtual ~XRActionSet();

        XrActionSet actionSet_ = { };
    };

    /// Cached grip pose bindings to avoid constant queries.
    SharedPtr<XRActionBinding> handGrips_[2];
    /// Cached aim pose bindings to avoid constant queries.
    SharedPtr<XRActionBinding> handAims_[2];
    /// Cached haptic outputs to avoid constant queries.
    SharedPtr<XRActionBinding> handHaptics_[2];

    /// List of extensions reported.
    StringVector extensions_;
    /// List of additional extensions you want.
    StringVector extraExtensions_;
};

}
