//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "../Scene/Component.h"
#include "../Graphics/Material.h"
#include "../Graphics/DecalSet.h"

namespace Urho3D
{
namespace DecalProjectionDetail
{

enum class SubscriptionMask : unsigned
{
    None = 0,
    Update = 1 << 0,
    PreRender = 1 << 1
};

URHO3D_FLAGSET(SubscriptionMask, SubscriptionFlags);

} // namespace DirectionAggregatorDetail

/// %Decal projection component.
class URHO3D_API DecalProjection : public Urho3D::Component
{
    URHO3D_OBJECT(DecalProjection, Component);

    typedef DecalProjectionDetail::SubscriptionFlags SubscriptionFlags;
    typedef DecalProjectionDetail::SubscriptionMask SubscriptionMask;

public:
    /// Construct.
    explicit DecalProjection(Context* context);
    /// Destruct.
    ~DecalProjection() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void OnSceneSet(Scene* scene) override;

    /// Set material. The material should use a small negative depth bias to avoid Z-fighting.
    void SetMaterial(Material* material);
    /// Set maximum number of decal vertices per decal set.
    void SetMaxVertices(unsigned num);
    /// Set maximum number of decal vertex indices per decal set.
    void SetMaxIndices(unsigned num);
    /// Set orthographic mode enabled/disabled.
    void SetOrthographic(bool enable);
    /// Set near clip distance.
    void SetNearClip(float nearClip);
    /// Set far clip distance.
    void SetFarClip(float farClip);
    /// Set vertical field of view in degrees.
    void SetFov(float fov);
    /// Set orthographic size attribute.
    void SetOrthoSize(float orthoSize);
    /// Set aspect ratio manually. Disables the auto aspect ratio -mode.
    void SetAspectRatio(float aspectRatio);
    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Set time to live in seconds.
    void SetTimeToLive(float timeToLive);
    /// Return normal threshold value.
    void SetNormalCutoff(float normalCutoff);
    /// Return view mask.
    void SetViewMask(unsigned);
    /// Set to remove either the component or its owner node from the scene automatically on decal time to live completion. Disabled by default.
    void SetAutoRemoveMode(AutoRemoveMode mode);

    /// Return material.
    Material* GetMaterial() const;
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;
    /// Return maximum number of decal vertices.
    unsigned GetMaxVertices() const { return maxVertices_; }
    /// Return maximum number of decal vertex indices.
    unsigned GetMaxIndices() const { return maxIndices_; }
    /// Return orthographic flag.
    bool IsOrthographic() const { return orthographic_; }
    /// Return far clip distance.
    float GetFarClip() const { return farClip_; }
    /// Return near clip distance.
    float GetNearClip() const { return nearClip_; }
    /// Return vertical field of view in degrees.
    float GetFov() const { return fov_; }
    /// Return orthographic mode size.
    float GetOrthoSize() const { return orthoSize_; }
    /// Return aspect ratio.
    float GetAspectRatio() const { return aspectRatio_; }
    /// Return time to live in seconds.
    float GetTimeToLive() const { return timeToLive_; }
    /// Return normal threshold value.
    float GetNormalCutoff() const { return normalCutoff_; }
    /// Return view mask.
    unsigned GetViewMask() const { return viewMask_; }
    /// Return automatic removal mode on decal time to live completion.
    AutoRemoveMode GetAutoRemoveMode() const { return autoRemove_; }

    /// Get view-projection matrix
    Matrix4 GetViewProj() const;

    /// Update projection.
    void UpdateGeometry();
    /// Inline projection.
    void Inline();

private:
    void UpdateSubscriptions(bool needGeometryUpdate = true);
    void HandlePreRenderEvent(StringHash eventName, VariantMap& eventData);
    void HandleSceneUpdate(StringHash eventName, VariantMap& eventData);
    bool IsValidDrawable(Drawable* drawable);

    /// Material.
    SharedPtr<Material> material_;

    static constexpr float DEFAULT_NEAR_CLIP = -1.0f;
    static constexpr float DEFAULT_FAR_CLIP = 1.0f;
    static constexpr float DEFAULT_FOV = 45.0f;
    static constexpr float DEFAULT_ASPECT_RATIO = 1.0f;
    static constexpr float DEFAULT_ORTHO_SIZE = 1.0f;
    static constexpr bool DEFAULT_ORTHO = true;
    static constexpr float DEFAULT_TIME_TO_LIVE = 0.0f;
    static constexpr float DEFAULT_NORMAL_CUTOFF = 0.1f;
    static constexpr unsigned DEFAULT_VIEWMASK = M_MAX_UNSIGNED;
    static constexpr unsigned DEFAULT_MAX_VERTICES = 512;
    static constexpr unsigned DEFAULT_MAX_INDICES = 1024;

    /// Orthographic mode flag.
    bool orthographic_{DEFAULT_ORTHO};
    /// Near clip distance.
    float nearClip_{DEFAULT_NEAR_CLIP};
    /// Far clip distance.
    float farClip_{DEFAULT_FAR_CLIP};
    /// Field of view.
    float fov_{DEFAULT_FOV};
    /// Orthographic view size.
    float orthoSize_{DEFAULT_ORTHO_SIZE};
    /// Aspect ratio.
    float aspectRatio_{DEFAULT_ASPECT_RATIO};
    /// Time to live. The projection going to remove itself after the timeout.
    float timeToLive_{DEFAULT_TIME_TO_LIVE};
    /// Automatic removal mode.
    AutoRemoveMode autoRemove_{REMOVE_DISABLED};
    /// Elapsed time in seconds.
    float elapsedTime_{0.0f};
    /// Projection normal threshold.
    float normalCutoff_{DEFAULT_NORMAL_CUTOFF};
    /// Query mask.
    unsigned viewMask_{DEFAULT_VIEWMASK};
    /// Maximum vertices.
    unsigned maxVertices_{DEFAULT_MAX_VERTICES};
    /// Maximum indices.
    unsigned maxIndices_{DEFAULT_MAX_INDICES};
    /// Active decal sets attached to objects in the scene.
    ea::vector<SharedPtr<DecalSet>> activeDecalSets_;
    /// Active subscriptions bitmask.
    SubscriptionFlags subscriptionFlags_{SubscriptionMask::None};
    /// Saved projection transform.
    Matrix3x4 projectionTransform_;
};

} // namespace Urho3D
