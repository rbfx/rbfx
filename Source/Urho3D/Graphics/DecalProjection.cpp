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

#include "../Precompiled.h"

#include "../Graphics/DecalProjection.h"

#include "../Core/CoreEvents.h"
#include "../Graphics/CustomGeometry.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/DecalSet.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/TerrainPatch.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{
extern const char* autoRemoveModeNames[];

DecalProjection::DecalProjection(Context* context)
    : Component(context)
{
}

DecalProjection::~DecalProjection() = default;

void DecalProjection::RegisterObject(Context* context)
{
    context->AddFactoryReflection<DecalProjection>(Category_Geometry);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
        "Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Max Vertices", GetMaxVertices, SetMaxVertices, unsigned, DEFAULT_MAX_VERTICES, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Indices", GetMaxIndices, SetMaxIndices, unsigned, DEFAULT_MAX_INDICES, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Near Clip", GetNearClip, SetNearClip, float, DEFAULT_NEAR_CLIP, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Far Clip", GetFarClip, SetFarClip, float, DEFAULT_FAR_CLIP, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("FOV", GetFov, SetFov, float, DEFAULT_FOV, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Aspect Ratio", GetAspectRatio, SetAspectRatio, float, DEFAULT_ASPECT_RATIO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Orthographic", IsOrthographic, SetOrthographic, bool, DEFAULT_ORTHO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Orthographic Size", GetOrthoSize, SetOrthoSize, float, DEFAULT_ORTHO_SIZE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Normal Cutoff", GetNormalCutoff, SetNormalCutoff, float, DEFAULT_NORMAL_CUTOFF, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Time To Live", GetTimeToLive, SetTimeToLive, float, DEFAULT_TIME_TO_LIVE, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Autoremove Mode", autoRemove_, autoRemoveModeNames, REMOVE_DISABLED, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Elapsed Time", float, elapsedTime_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("View Mask", GetViewMask, SetViewMask, unsigned, DEFAULT_VIEWMASK, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Component);
    URHO3D_ACTION_STATIC_LABEL("Update", UpdateGeometry, "");
    URHO3D_ACTION_STATIC_LABEL("Inline", Inline, "");
}

void DecalProjection::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    // Workaround for Editor.
    if (elapsedTime_ == 0)
    {
        auto transform = GetNode()->GetWorldTransform();
        if (!transform.Equals(projectionTransform_))
        {
            projectionTransform_ = transform;
            UpdateSubscriptions(true);
        }
    }

    // Render projection frustum.
    Matrix4 viewProj = GetViewProj();
    Frustum frustum;
    frustum.Define(viewProj);
    debug->AddFrustum(frustum, Color::WHITE, depthTest);
}
Material* DecalProjection::GetMaterial() const
{
    return material_;
}


void DecalProjection::SetMaterial(Material* material)
{
    if (material_ != material)
    {
        material_ = material;
        UpdateSubscriptions();
    }
}

void DecalProjection::SetMaxVertices(unsigned num)
{
    maxVertices_ = num;
}

void DecalProjection::SetMaxIndices(unsigned num)
{
    maxIndices_ = num;
}

void DecalProjection::SetOrthographic(bool enable)
{
    orthographic_ = enable;
    UpdateSubscriptions();
}

void DecalProjection::SetNearClip(float nearClip)
{
    nearClip_ = nearClip;
    UpdateSubscriptions();
}

void DecalProjection::SetFarClip(float farClip)
{
    farClip_ = farClip;
    UpdateSubscriptions();
}

void DecalProjection::SetFov(float fov)
{
    fov_ = Clamp(fov, 0.0f, M_MAX_FOV);
    UpdateSubscriptions();
}

void DecalProjection::SetOrthoSize(float orthoSize)
{
    if (orthoSize_ != orthoSize)
    {
        orthoSize_ = orthoSize;
        UpdateSubscriptions();
    }
}

void DecalProjection::SetAspectRatio(float aspectRatio)
{
    if (aspectRatio_ != aspectRatio)
    {
        aspectRatio_ = aspectRatio;
        UpdateSubscriptions();
    }
}

void DecalProjection::SetMaterialAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.name_));
}

void DecalProjection::SetTimeToLive(float timeToLive)
{
    timeToLive_ = timeToLive;
    UpdateSubscriptions();
}

void DecalProjection::SetNormalCutoff(float normalCutoff)
{
    normalCutoff_ = normalCutoff;
    UpdateSubscriptions();
}

void DecalProjection::SetViewMask(unsigned viewMask)
{
    viewMask_ = viewMask;
    UpdateSubscriptions();
}

void DecalProjection::SetAutoRemoveMode(AutoRemoveMode mode)
{
    autoRemove_ = mode;
    UpdateSubscriptions();
}

void DecalProjection::OnSceneSet(Scene* scene)
{
    BaseClassName::OnSceneSet(scene);

    UpdateSubscriptions(subscriptionFlags_ & SubscriptionMask::PreRender);
}

ResourceRef DecalProjection::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void DecalProjection::UpdateSubscriptions(bool needGeometryUpdate)
{
    SubscriptionFlags flags{SubscriptionMask::None};
    Scene* scene = GetScene();
    if (timeToLive_ > 0.0f && autoRemove_ != REMOVE_DISABLED && scene)
        flags |= SubscriptionMask::Update;
    if (needGeometryUpdate)
        flags |= SubscriptionMask::PreRender;
    
    const auto toSubscribe = flags & ~subscriptionFlags_;
    const auto toUnsubscribe = subscriptionFlags_ & ~flags;

    if (toSubscribe == SubscriptionMask::None && toUnsubscribe == SubscriptionMask::None)
        return;

    subscriptionFlags_ = flags;

    if (toSubscribe & SubscriptionMask::Update)
    {
        SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(DecalProjection, HandleSceneUpdate));
    }
    else if (toUnsubscribe & SubscriptionMask::Update)
    {
        UnsubscribeFromEvent(scene, E_SCENEUPDATE);
    }

    if (toSubscribe & SubscriptionMask::PreRender)
    {
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(DecalProjection, HandlePreRenderEvent));
    }
    else if (toUnsubscribe & SubscriptionMask::PreRender)
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
    }
}

void DecalProjection::HandleSceneUpdate(StringHash eventName, VariantMap& eventData)
{
    using namespace  SceneUpdate;
    elapsedTime_ += eventData[P_TIMESTEP].GetFloat();
    if (elapsedTime_ > timeToLive_)
    {
        DoAutoRemove(autoRemove_);
    }
}

bool DecalProjection::IsValidDrawable(Drawable* drawable)
{
    auto current = drawable->GetTypeInfo();
    while (current)
    {
        if (current->GetType() == DecalSet::GetTypeNameStatic())
            return false;
        if (current->GetType() == Skybox::GetTypeNameStatic())
            return false;
        if (current->GetType() == StaticModel::GetTypeNameStatic())
            return true;
        if (current->GetType() == TerrainPatch::GetTypeNameStatic())
            return true;
        if (current->GetType() == CustomGeometry::GetTypeNameStatic())
            return true;

        current = current->GetBaseTypeInfo();
    }
    return false;
}

void DecalProjection::HandlePreRenderEvent(StringHash eventName, VariantMap& eventData)
{
    UpdateGeometry();
}

Matrix4 DecalProjection::GetViewProj() const
{
    const Matrix3x4 frustumTransform(
        node_ ? Matrix3x4(node_->GetWorldPosition(), node_->GetWorldRotation(), 1.0f) : Matrix3x4::IDENTITY);
    Matrix4 proj;
    if (orthographic_)
    {
        proj.SetOrthographic(orthoSize_, 1.0f, aspectRatio_, nearClip_, farClip_, Vector2::ZERO);
    }
    else
    {
        proj.SetPerspective(fov_, 1.0f, aspectRatio_, Max(nearClip_, M_MIN_NEARCLIP), Max(farClip_, M_MIN_NEARCLIP*2.0f), Vector2::ZERO);
    }
    return  proj * frustumTransform.Inverse();
}

void DecalProjection::Inline()
{
    if (subscriptionFlags_ & SubscriptionMask::PreRender)
        UpdateGeometry();

    for (auto& activeDecal : activeDecalSets_)
    {
        activeDecal->SetTemporary(false);
    }
    activeDecalSets_.clear();
    Remove();
}

void DecalProjection::UpdateGeometry()
{
    UpdateSubscriptions(false);

    auto* octree = GetScene()->GetComponent<Octree>();
    if (!octree)
        return;

    for (auto& activeDecal: activeDecalSets_)
    {
        auto* node = activeDecal->GetNode();
        if (node)
            node->RemoveComponent(activeDecal);
    }
    activeDecalSets_.clear();

    ea::vector<Drawable*> drawables;
    Matrix4 viewProj = GetViewProj();
    Frustum frustum;
    frustum.Define(viewProj);

    FrustumOctreeQuery query{drawables, frustum, DRAWABLE_GEOMETRY, viewMask_};
    octree->GetDrawables(query);

    for (Drawable* drawable: drawables)
    {
        if (IsValidDrawable(drawable))
        {
            auto* node = drawable->GetNode();
            if (node)
            {
                auto decalSet = node->CreateComponent<DecalSet>();
                decalSet->SetTemporary(true);
                decalSet->SetMaterial(material_);
                decalSet->SetMaxIndices(maxIndices_);
                decalSet->SetMaxVertices(maxVertices_);
                const float timeLive = (timeToLive_ > 0) ? Urho3D::Max(M_EPSILON, timeToLive_ - elapsedTime_) : 0.0f;
                decalSet->AddDecal(drawable, viewProj, Vector2::ZERO, Vector2::ONE, timeLive, normalCutoff_);
                activeDecalSets_.push_back(SharedPtr<DecalSet>(decalSet));
            }
        }
    }
}


} // namespace Urho3D
