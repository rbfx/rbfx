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
#include "../Graphics//Material.h"

namespace Urho3D
{

/// %Decal projection component.
class URHO3D_API DecalProjection : public Urho3D::Component
{
    URHO3D_OBJECT(DecalProjection, Component);

public:
    /// Construct.
    explicit DecalProjection(Context* context);
    /// Destruct.
    ~DecalProjection() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set material. The material should use a small negative depth bias to avoid Z-fighting.
    void SetMaterial(Material* material);
    /// Set near clip distance.
    void SetNearClip(float nearClip);
    /// Set far clip distance.
    void SetFarClip(float farClip);
    /// Set vertical field of view in degrees.
    void SetFov(float fov);
    /// Set aspect ratio manually. Disables the auto aspect ratio -mode.
    void SetAspectRatio(float aspectRatio);
    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);

    /// Return material.
    Material* GetMaterial() const;
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;
    /// Return far clip distance.
    float GetFarClip() const;
    /// Return near clip distance.
    float GetNearClip() const;
    /// Return vertical field of view in degrees.
    float GetFov() const { return fov_; }
    /// Return aspect ratio.
    float GetAspectRatio() const { return aspectRatio_; }

    /// Update projection.
    void Update();

private:
    void SheduleUpdate();
    void HandlePreRenderEvent(StringHash eventName, VariantMap& eventData);

    /// Material.
    SharedPtr<Material> material_;

    /// Is projection needs to be updated.
    bool isDirty_{};

    /// Near clip distance.
    float nearClip_{0.1f};
    /// Far clip distance.
    float farClip_{1.0f};
    /// Field of view.
    float fov_{90.0f};
    /// Aspect ratio.
    float aspectRatio_{1.0f};
};

} // namespace Urho3D
