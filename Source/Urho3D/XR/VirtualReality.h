// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Ray.h"
#include "Urho3D/Math/Vector3.h"
#include "Urho3D/RenderPipeline/StereoRenderPipeline.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

class VRRig;

/// Parameters for initializing a VR session.
struct VRSessionParameters
{
    ea::string manifestPath_;
    int multiSample_{};
    float resolutionScale_{1.0f};
};

/// Description of VR rig that links VR subsystem to the scene.
struct URHO3D_API VRRigDesc
{
    SharedPtr<Viewport> viewport_;
    SharedPtr<StereoRenderPipeline> pipeline_;
    WeakPtr<Scene> scene_;
    WeakPtr<Node> head_;
    WeakPtr<Camera> leftEye_;
    WeakPtr<Camera> rightEye_;
    WeakPtr<Node> leftHandPose_;
    WeakPtr<Node> rightHandPose_;
    WeakPtr<Node> leftHandAim_;
    WeakPtr<Node> rightHandAim_;
    WeakPtr<Node> leftController_;
    WeakPtr<Node> rightController_;
    float nearDistance_{};
    float farDistance_{};

    bool IsValid() const;
};

/// Backend implementation of VirtualReality interface.
enum class VRRuntime
{
    OpenXR,
};

/// Hand ID.
enum class VRHand
{
    None = -1,
    Left = 0,
    Right = 1,
    Count
};

/// Eye ID.
enum class VREye
{
    None = -1,
    Left = 0,
    Right = 1,
    Count
};

/// Wraps an input binding. Subclassed as required by interface implementations.
class URHO3D_API XRBinding : public Object
{
    URHO3D_OBJECT(XRBinding, Object);

public:
    XRBinding(Context* context, const ea::string& name, const ea::string& localizedName, VRHand hand,
        VariantType dataType, bool isPose, bool isAimPose);

    const ea::string& GetName() const { return name_; }
    const ea::string& GetLocalizedName() const { return localizedName_; }

    /// Returns true if this action has changed state since the last update.
    bool IsChanged() const { return changed_; }
    /// Returns true if this action is actively being used.
    bool IsActive() const { return active_; }
    /// Returns true if this action is bound to a hand.
    bool IsHanded() const { return hand_ != VRHand::None; }
    /// Returns the hand this action is bound to.
    VRHand GetHand() const { return hand_; }

    /// Variant accessors.
    /// @{
    bool GetBool(float pressThreshold) const { return storedData_.GetFloat() > pressThreshold; }
    bool GetBool() const { return storedData_.GetBool(); }
    float GetFloat() const { return storedData_.GetFloat(); }
    Vector2 GetVector2() const { return storedData_.GetVector2(); }
    Vector3 GetVector3() const { return storedData_.GetVector3(); }
    Vector3 GetPosition() const { return storedData_.GetMatrix3x4().Translation(); }
    Quaternion GetRotation() const { return storedData_.GetMatrix3x4().Rotation(); }
    const Matrix3x4& GetTransformMatrix() const { return storedData_.GetMatrix3x4(); }
    /// @}

    /// Returns stored variant value.
    const Variant& GetData() const { return storedData_; }
    /// Returns linear velocity of the pose.
    const Vector3& GetLinearVelocity() const { return linearVelocity_; }
    /// Returns angular velocity of the pose.
    const Vector3& GetAngularVelocity() const { return angularVelocity_; }

    /// Returns true if this action is bound as a live input possibility.
    bool IsBound() const { return isBound_; }

    /// Returns true if this is an input method action.
    bool IsInput() const { return !haptic_; }
    /// Returns true if this is an output haptic action.
    bool IsHaptic() const { return haptic_; }
    /// Return true if this action is hand grip pose.
    bool IsGripPose() const { return isPose_; }
    /// Return true if this action is hand aim pose.
    bool IsAimPose() const { return isAimPose_; }

protected:
    /// Internal name for the action.
    const ea::string name_;
    /// Localized "friendly" name for the action, ie. "Trigger"
    const ea::string localizedName_;
    /// Hand this action is attached to if a hand relevant action.
    const VRHand hand_;
    /// Data-type that the stored data can be expected to be.
    const VariantType dataType_;
    /// Indicates this is a haptic output action.
    const bool haptic_;
    /// Indicates this action pulls the base pose information for the given hand.
    const bool isPose_;
    /// Indicates this action pulls the aim pose information for the given hand.
    const bool isAimPose_;

    /// The input has changed since the last update.
    bool changed_ = false;
    /// The input is in an active state of being used, ie. a button being held.
    bool active_ = false;
    /// Indicates whether the action is properly bound to be used.
    bool isBound_ = false;
    /// Stored data retrieved from input updates.
    Variant storedData_;

    /// Optional: linear velocity of the pose.
    Vector3 linearVelocity_;
    /// Optional: angular velocity of the pose.
    Vector3 angularVelocity_;
};

/// Represents a logical action set in the underlying APIs.
class URHO3D_API XRActionGroup : public Object
{
    URHO3D_OBJECT(XRActionGroup, Object);

public:
    XRActionGroup(Context* context, const ea::string& name, const ea::string& localizedName);

    /// Find binding by name, case insensitive.
    XRBinding* FindBinding(const ea::string& name, VRHand hand) const;
    /// Return all bindings.
    const ea::vector<SharedPtr<XRBinding>>& GetBindings() const { return bindings_; }

    /// Return immutable properties.
    /// @{
    const ea::string& GetName() const { return name_; }
    const ea::string& GetLocalizedName() const { return localizedName_; }
    /// @}

protected:
    /// Identifier of this action set.
    const ea::string name_;
    /// Localized identifier.
    const ea::string localizedName_;

    /// Contained action bindings.
    ea::vector<SharedPtr<XRBinding>> bindings_;
};

/// Base interface for a VR related subsystem. This is not expected to be utilized for mobile AR, it would be best
/// to implement something else for that purpose.
class URHO3D_API VirtualReality : public Object
{
    URHO3D_OBJECT(VirtualReality, Object);

public:
    explicit VirtualReality(Context* context);
    virtual ~VirtualReality();

    /// Initializes the VR session.
    virtual bool InitializeSession(const VRSessionParameters& params) = 0;
    /// Shuts down the VR session.
    virtual void ShutdownSession() = 0;
    /// Connects session to the rig.
    virtual void ConnectToRig(const VRRigDesc& rig);

    /// Returns true if this VR configuration is running at room scale.
    bool IsRoomScale() const { return isRoomScale_; }
    /// Returns currently connected rig.
    const VRRigDesc& GetRig() const { return rig_; }

    /// IPD correction factor in millimeters.
    float GetIPDCorrection() const { return ipdCorrection_; }
    /// Scale correction factor, premultiplied into all transforms.
    float GetScaleCorrection() const { return scaleCorrection_; }

    /// Set a software IPD adjustment in millimeters, applied by translating each eye in local space on the X axis by
    /// half the specified amount. Only intended for small corrective changes of ~2mm.
    void SetIPDCorrection(float value) { ipdCorrection_ = value; }
    /// Scale correction can also be done on the VRRig node.
    void SetScaleCorrection(float value) { scaleCorrection_ = value; }

    /// Returns recommended MSAA level.
    int GetRecommendedMultiSample() const { return recommendedMultiSample_; }
    /// Returns the currently chosen MSAA level.
    int GetMultiSample() const { return multiSample_; }

    /// Viewport rectangle for left eye, required for multipass single-RT.
    IntRect GetLeftEyeRect() const { return {IntVector2::ZERO, eyeTextureSize_}; }
    /// Viewport rectangle for right eye, required for multipass single-RT.
    IntRect GetRightEyeRect() const { return {eyeTextureSize_.x_, 0, eyeTextureSize_.x_ * 2, eyeTextureSize_.y_}; }

    /// Return the classification of VR runtime being used,
    virtual VRRuntime GetRuntime() const = 0;
    /// Return a string name for the runtime, spaces are not allowed as this will be passed along to shaders.
    virtual const char* GetRuntimeName() const = 0;

    /// Activates a haptic for a given hand.
    virtual void TriggerHaptic(VRHand hand, float durationSeconds, float cyclesPerSec, float amplitude) = 0;

    /// Returns the transform for a given hand in head relative space.
    virtual Matrix3x4 GetHandTransform(VRHand hand) const = 0;
    /// Transform matrix of the hand aim base position.
    virtual Matrix3x4 GetHandAimTransform(VRHand hand) const = 0;
    /// Returns the aiming ray for a given hand.
    virtual Ray GetHandAimRay(VRHand hand) const = 0;
    /// Return linear and/or angular velocity of a hand.
    virtual void GetHandVelocity(VRHand hand, Vector3* linear, Vector3* angular) const = 0;
    /// Return the head transform in stage space (or local if no stage).
    virtual Matrix3x4 GetHeadTransform() const = 0;
    /// Return the head-relative eye transform.
    virtual Matrix3x4 GetEyeLocalTransform(VREye eye) const = 0;
    /// Return the projection matrix for an eye.
    virtual Matrix4 GetProjection(VREye eye, float nearDist, float farDist) const = 0;

    /// Returns whether the engine is connected to VR session.
    virtual bool IsConnected() const = 0;
    /// Returns whether the VR frame loop is running.
    virtual bool IsRunning() const = 0;
    /// Returns whether the VR session is presented to the user.
    virtual bool IsVisible() const = 0;
    /// Return whether the VR session accepts user input.
    virtual bool IsFocused() const = 0;

    /// Attempts to retrieve an input binding.
    XRBinding* GetInputBinding(const ea::string& path) const;
    /// Attempts to retrieve a hand specific input binding.
    XRBinding* GetInputBinding(const ea::string& path, VRHand hand) const;
    /// Returns the currently bound action set, null if no action set.
    XRActionGroup* GetCurrentActionSet() const { return activeActionSet_; }

    /// Sets the current action set by name.
    virtual void SetCurrentActionSet(const ea::string& setName);
    virtual void SetCurrentActionSet(SharedPtr<XRActionGroup> set) = 0;

    /// Returns the system name, ie. Windows Mixed Reality.
    ea::string GetSystemName() const { return systemName_; }

protected:
    void CreateDefaultRig();
    void ValidateCurrentRig();
    void UpdateCurrentRig();

    /// Name of the system being run, ie. Windows Mixed Reality
    ea::string systemName_;
    /// MSAA level recommended by API.
    int recommendedMultiSample_{1};
    /// Texture size recommended by API.
    IntVector2 recommendedEyeTextureSize_;

    /// Effective MSAA level to use.
    int multiSample_{};
    /// Effective texture size.
    IntVector2 eyeTextureSize_;

    /// External IPD adjustment.
    float ipdCorrection_ = 0.0f;
    /// Scaling factor correct by.
    float scaleCorrection_ = 1.0f;
    /// Whether to automatically invoke the hidden area masks, if on then renderpath must not clear (or not clear depth
    /// at least)
    bool autoClearMasks_ = true;
    /// Indicates we have room scale tracking.
    bool isRoomScale_ = false;

    /// Default scene for rendering.
    SharedPtr<Scene> defaultScene_;
    /// Default rig to use.
    SharedPtr<VRRig> defaultRig_;

    /// Link to currently used rig.
    VRRigDesc rig_;

    /// Back buffer textures active in current frame.
    SharedPtr<Texture2D> currentBackBufferColor_;
    SharedPtr<Texture2D> currentBackBufferDepth_;

    /// Currently bound action-set.
    SharedPtr<XRActionGroup> activeActionSet_;
    /// Table of action sets registered.
    ea::map<ea::string, SharedPtr<XRActionGroup>> actionSets_;
};

void RegisterVRLibrary(Context* context);

} // namespace Urho3D
