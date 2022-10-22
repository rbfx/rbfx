//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Graphics/Texture2D.h>
#include "../Math/Vector3.h"

namespace Urho3D
{

    class Node;
    class PipelineState;
    class Scene;
    class StereoRenderPipeline;

    /// Identifier of backing runtime for VRInterface. Currently only OpenXR is implmented.
    enum class VRRuntime
    {
        OPENVR,       // SteamVR/OpenVR runtime, not yet ported to RBFX - only in order to have a fallback should open-xr runtime be temporarilly broken for a PC reason (ie. Windows Update broke it).
        OPENXR,       // System OpenXR runtime
        OCULUS,       // RESERVED, not implemented OVR PC SDK
        OCULUS_MOBILE // RESERVED, not implemented OVR Mobile SDK
    };

    /// Hand identification ID, not classed as it's frequently used as an index.
    enum VRHand
    {
        VR_HAND_NONE = -1,
        VR_HAND_LEFT = 0,
        VR_HAND_RIGHT = 1
    };

    /// Eye identification ID, not classed as it's frequently used as an index.
    enum VREye
    {
        VR_EYE_NONE = -1,
        VR_EYE_LEFT = 0,
        VR_EYE_RIGHT = 1
    };

    // Single-pass stereo rendering mode setting.
    enum class VRRenderMode
    {
        SINGLE_TEXTURE,  // 1 double size texture containing both eyes
        LAYERED          // render-target array, RESERVED, not implemented
    };

    /// Wraps an input binding. Subclassed as required by interface implementations.
    class URHO3D_API XRBinding : public Object
    {
        friend class SteamVR;
        friend class OpenXR;
        friend class VRInterface;

        URHO3D_OBJECT(XRBinding, Object);
    public:
        XRBinding(Context*);
        virtual ~XRBinding();

        ea::string GetLocalizedName() const { return localizedName_; }
        void SetLocalizedName(const ea::string& name) { localizedName_ = name; }

        /// Returns true if this action has changed state since the last update.
        bool IsChanged() const { return changed_; }
        /// Returns true if this action is actively being used.
        bool IsActive() const { return active_; }
        /// Returns true if this action is bound to a hand.
        bool IsHanded() const { return hand_ != VR_HAND_NONE; }
        /// Returns the hand this action is bound to.
        VRHand Hand() const { return hand_; }

        bool GetBool(float pressThreshold) const { return storedData_.GetFloat() > pressThreshold; }
        bool GetBool() const { return storedData_.GetBool(); }
        float GetFloat() const { return storedData_.GetFloat(); }
        Vector2 GetVec2() const { return storedData_.GetVector2(); }
        Vector3 GetVec3() const { return storedData_.GetVector3(); }
        Vector3 GetPos() const { return storedData_.GetMatrix3x4().Translation(); }
        Quaternion GetRot() const { return storedData_.GetMatrix3x4().Rotation(); }
        Matrix3x4 GetTransform() const { return storedData_.GetMatrix3x4(); }

        /// Retrieve direct variant value stored.
        Variant GetData() const { return storedData_; }
        /// Retrieve the delta variant stored.
        Variant GetDelta() const { return delta_; }
        /// Retrieve secondary extra delta value.
        Variant GetExtraDelta() const { return extraDelta_; }

        /// Returns true if this action is bound as a live input possibility.
        bool IsBound() const { return isBound_; }

        /// If an output haptic action, induce vibration with the provided parameters.
        virtual void Vibrate(float duration, float frequency, float amplitude) { }

        /// Returns true if this is an input method action.
        bool IsInput() const { return haptic_ == false; }
        /// Returns true if this is an output haptic action.
        bool IsHaptic() const { return haptic_; }

    protected:
        /// Localized "friendly" name for the action, ie. "Trigger"
        ea::string localizedName_;
        /// Action binding path.
        ea::string path_;
        /// Hand this action is attached to if a hand relevant action.
        VRHand hand_ = VR_HAND_NONE;
        /// Data-type that the stored data can be expected to be.
        VariantType dataType_;
        /// The input has changed since the last update.
        bool changed_ = false;
        /// The input is in an active state of being used, ie. a button being held.
        bool active_ = false;
        /// Indicates this is a haptic output action.
        bool haptic_ = false;
        /// Indicates whether the action is properly bound to be used.
        bool isBound_ = false;
        /// Indicates this action pulls the base pose information for the given hand.
        bool isPose_ = false;
        /// Indicates this action pulls the aim pose information for the given hand.
        bool isAimPose_ = false;
        /// Stored data retrieved from input updates.
        Variant storedData_;
        /// Optional additional data such as velocities for a pose.
        Variant extraData_[2];
        /// Difference between the current and previous values.
        Variant delta_;
        /// Additional deltas for the extraData members.
        Variant extraDelta_[2];
    };

    /// Represents a logical action set in the underlying APIs.
    class URHO3D_API XRActionGroup : public Object
    {
        friend class SteamVR;
        friend class OpenXR;
        friend class VRInterface;

        URHO3D_OBJECT(XRActionGroup, Object);
    public:
        XRActionGroup(Context* ctx) : Object(ctx) { }
        virtual ~XRActionGroup() { }

        /// Return action name.
        const ea::string& GetName() const { return name_; }
        /// Return localized action name.
        const ea::string& GetLocalizedName() const { return localizedName_; }

        /// Return the list of contained bindings, modifiable.
        ea::vector<SharedPtr<XRBinding>>& GetBindings() { return bindings_; }
        /// Return the list of contained bindings, non-modifiable.
        const ea::vector<SharedPtr<XRBinding>>& GetBindings() const { return bindings_; }

    protected:
        /// Identifier of this action set.
        ea::string name_;
        /// Localized identifier.
        ea::string localizedName_;
        /// Contained action bindings.
        ea::vector<SharedPtr<XRBinding>> bindings_;
        /// Are we the default action-set?
        bool isDefault_;
    };

    /** %VRInterface component
    *   Base interface for a VR related subsystem. This is not expected to be utilized for mobile AR, it would be best to implement something else for that purpose.
    *
    *   TODO:
    *       Rig handling, should it anchor to the head in XZ each update?
    */
    class URHO3D_API VRInterface : public Object
    {
        URHO3D_OBJECT(VRInterface, Object);
    public:
        /// Construct.
        VRInterface(Context*);
        /// Destruct.
        virtual ~VRInterface();

        /// Returns true if this VR configuration is running at room scale.
        bool IsRoomScale() const { return isRoomScale_; }

        /// IPD correction factor in millimeters.
        float GetIPDCorrection() const { return ipdCorrection_; }
        /// Height correction factor, added into vertical positioning.
        float GetHeightCorrection() const { return heightCorrection_; }
        /// Scale correction factor, premultiplied into all transforms.
        float GetScaleCorrection() const { return scaleCorrection_; }

        /// Set a software IPD adjustment in millimeters, applied by translating each eye in local space on the X axis by half the specified amount. Only intended for small corrective changes of ~2mm.
        void SetIPDCorrection(float value) { ipdCorrection_ = value; }
        /// Height correction can also be done on the VRRig node.
        void SetHeightCorrection(float value) { heightCorrection_ = value; }
        /// Scale correction can also be done on the VRRig node.
        void SetScaleCorrection(float value) { scaleCorrection_ = value; }

        /// Returns the currently chosen MSAA level.
        int GetMSAALevel() const { return msaaLevel_; }
        /// Change MSAA level, have to call CreateEyeTextures() to update.
        void SetMSAALevel(int level);

        /// Can use render-scale to resize targets if FPS is too low.
        float GetRenderScale() const { return renderTargetScale_; }
        /// Sets the scale factor for the render-targets, have to call CreateEyeTextures() to update.
        virtual void SetRenderScale(float value);

        /// Returns whether we're rendering to 1 double-wide texture or 2 independent eye textures.
        bool IsSingleTexture() const { return useSingleTexture_; }
        /// Set to use a single texture.
        virtual void SetSingleTexture(bool state) { useSingleTexture_ = state; }

        /// Renders the eye-masks to depth 0 (-1 in GL) so depth-test discards pixels. Also clears the render-targets in question. So the renderpath must not clear.
        bool IsAutoDrawEyeMasks() const { return autoClearMasks_; }
        /// Set whether to render depth-0 (-1 in GL) masks so depth-test discards pixels. if true the renderpath must not clear.
        void SetAutoDrawEyeMasks(bool state) { autoClearMasks_ = state; }

        /// Viewport rectangle for left eye, required for multipass single-RT.
        IntRect GetLeftEyeRect() const { return IntRect(0, 0, eyeTexWidth_, eyeTexHeight_); }
        /// Viewport rectangle for right eye, required for multipass single-RT.
        IntRect GetRightEyeRect() const { return useSingleTexture_ ? IntRect(eyeTexWidth_, 0, eyeTexWidth_ * 2, eyeTexHeight_) : IntRect(0, 0, eyeTexWidth_, eyeTexHeight_); }

        /// Return the classification of VR runtime being used, 
        virtual VRRuntime GetRuntime() const = 0;
        /// Return a string name for the runtime, spaces are not allowed as this will be passed along to shaders.
        virtual const char* GetRuntimeName() const = 0;

        /// Constructs the backing eye textures. Overridable for APIs that work with swapchains.
        virtual void CreateEyeTextures();

        /// Constructs the head, eye, and hand nodes for a rig at a given node. The rig is effectively the worldspace locator. This rig is considered standard.
        void PrepareRig(Node* vrRig);
        /// Updates the rig calling the overloaded version by using the standardized rig constructed by PrepareRig.
        void UpdateRig(Node* vrRig, float nearDist, float farDist, bool forSinglePass);
        /// Detailed rig update with explicit parameters for non-standardized rig setup.
        void UpdateRig(Scene* scene, Node* head, Node* leftEye, Node* rightEye, float nearDist, float farDist, bool forSinglePass);
        
        /// Called by ConfigureRig to setup hands. Responsible for models and transforms
        virtual void UpdateHands(Scene* scene, Node* rigRoot, Node* leftHand, Node* rightHand) = 0; 

        /// Initializes the VR system providing a manifest.
        virtual bool Initialize(const ea::string& manifestPath) = 0;
        /// Shutsdown the VR system.
        virtual void Shutdown() = 0;

        /// Activates a haptic for a given hand.
        virtual void TriggerHaptic(VRHand hand, float durationSeconds, float cyclesPerSec, float amplitude) = 0;

        /// Returns the transform for a given hand in head relative space.
        virtual Matrix3x4 GetHandTransform(VRHand) const = 0;
        /// Transform matrix of the hand aim base position.
        virtual Matrix3x4 GetHandAimTransform(VRHand) const = 0;
        /// Returns the aiming ray for a given hand.
        virtual Ray GetHandAimRay(VRHand) const = 0;
        /// Return linear and/or angular velocity of a hand.
        virtual void GetHandVelocity(VRHand hand, Vector3* linear, Vector3* angular) const = 0;
        /// Return the head transform in stage space (or local if no stage).
        virtual Matrix3x4 GetHeadTransform() const = 0;
        /// Return the head-relative eye transform.
        virtual Matrix3x4 GetEyeLocalTransform(VREye eye) const = 0;
        /// Return the projection matrix for an eye.
        virtual Matrix4 GetProjection(VREye eye, float nearDist, float farDist) const = 0;

        /// Draws the hidden area mask.
        virtual void DrawEyeMask();
        /// Draws an inner radial mask suitable for simple vignette effects, lerps the two colors as lerp(inside, outside, pow(vertex_alpha, power)).
        virtual void DrawRadialMask(Color inside, Color outside, float power);

        /// Returns true if our VR system is alive, but may not necessarilly actively rendering.
        virtual bool IsConnected() const = 0;
        /// Returns true if our VR system is alive, and actively rendering.
        virtual bool IsLive() const = 0;

        /// Attempts to retrieve an input binding.
        SharedPtr<XRBinding> GetInputBinding(const ea::string& path);
        /// Attempts to retrieve a hand specific input binding.
        SharedPtr<XRBinding> GetInputBinding(const ea::string& path, VRHand forHand);
        /// Returns the currently bound action set, null if no action set.
        SharedPtr<XRActionGroup> GetCurrentActionSet() const { return activeActionSet_; }
        /// Sets the current action set by name.
        virtual void SetCurrentActionSet(const ea::string& setName);
        /// INTERFACE: Sets the current action set.
        virtual void SetCurrentActionSet(SharedPtr<XRActionGroup>) = 0;

        /// Returns the side-by-side color texture.
        SharedPtr<Texture2D> GetSharedTexture() const { return sharedTexture_; }
        /// Returns the side-by-side depth texture.
        SharedPtr<Texture2D> GetSharedDepth() const { return sharedDS_; }

        /// Returns the system name, ie. Windows Mixed Reality.
        ea::string GetSystemName() const { return systemName_; }

        /// Returns the assigned viewport.
        SharedPtr<Viewport> GetViewport() const { return viewport_; }
        /// Sets the viewport that should be used for rendering, will warn and construct a default if never specified.
        void SetViewport(SharedPtr<Viewport> vp) { viewport_ = vp; }

    protected:
        /// Name of the system being run, ie. Windows Mixed Reality
        ea::string systemName_;
        /// MSAA level to use, 4 is generally what is recommended.
        int msaaLevel_ = 1;
        /// Texture width API recommended.
        int trueEyeTexWidth_ = 0;
        /// Texture height API recommended.
        int trueEyeTexHeight_ = 0;
        /// Texture width after scaling.
        int eyeTexWidth_;
        /// Texture height after scaling.
        int eyeTexHeight_;
        /// External IPD adjustment.
        float ipdCorrection_ = 0.0f;
        /// Vertical correction factor.
        float heightCorrection_ = 0.0f;
        /// Scaling factor correct by.
        float scaleCorrection_ = 1.0f;
        /// Scaling factor to apply to recommended render-target resolutions. Such as going lower res or higher res.
        float renderTargetScale_ = 1.0f;
        /// Whether to automatically invoke the hidden area masks, if on then renderpath must not clear (or not clear depth at least)
        bool autoClearMasks_ = true;
        /// Indicates if using a single double-wide texture via instanced-stereo instead of separate images.
        bool useSingleTexture_ = true;
        /// Indicates we have room scale tracking.
        bool isRoomScale_ = false;

        /// Due to swap chain texture counts it's necessary to manage the viewport ourself to change the textures.
        SharedPtr<Viewport> viewport_;
        /// Only exists if it was necessary to create a pipeline ourselves for the viewport as a default.
        SharedPtr<StereoRenderPipeline> pipeline_;
        /// Graphics format for the backbuffer from the swapchain.
        unsigned backbufferFormat_;
        /// Active backbuffer textures.
        SharedPtr<Texture2D> leftTexture_, rightTexture_, sharedTexture_, leftDS_, rightDS_, sharedDS_;
        /// Hidden area mesh.
        SharedPtr<Geometry> hiddenAreaMesh_[2];
        /// Visible area mesh.
        SharedPtr<Geometry> visibleAreaMesh_[2];
        /// Radial area mesh. Setup with 1.0 alpha at the edges, and 0.0 at the center can be used for edge darkening / glows / etc.
        SharedPtr<Geometry> radialAreaMesh_[2];
        /// Currently bound action-set.
        SharedPtr<XRActionGroup> activeActionSet_;
        /// Table of action sets registered.
        ea::map<ea::string, SharedPtr<XRActionGroup> > actionSets_;

        struct ControlMesh
        {
            SharedPtr<Geometry> geometry_;
            SharedPtr<Texture2D> colorTex_;
            Urho3D::BoundingBox bounds_;
        };

        /// Pipeline state for the hidden area eye mask.
        SharedPtr<PipelineState> eyeMaskPipelineState_;
        /// Pipeline state for the trivial vignette drawing.
        SharedPtr<PipelineState> simpleVignettePipelineState_;
    };

    void RegisterVR(Context*);
}
