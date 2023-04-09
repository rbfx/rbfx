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

#include "DecalSet.h"
#include "Octree.h"
#include "../Resource/ResourceCache.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

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
    URHO3D_COPY_BASE_ATTRIBUTES(Component);
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
        SheduleUpdate();
    }
}

void DecalProjection::SetNearClip(float nearClip)
{
    nearClip_ = Max(nearClip, M_MIN_NEARCLIP);
    SheduleUpdate();
}

void DecalProjection::SetFarClip(float farClip)
{
    farClip_ = Max(farClip, M_MIN_NEARCLIP);
    SheduleUpdate();
}

void DecalProjection::SetFov(float fov)
{
    fov_ = Clamp(fov, 0.0f, M_MAX_FOV);
    SheduleUpdate();
}

void DecalProjection::SetAspectRatio(float aspectRatio)
{
    if (aspectRatio_ != aspectRatio)
    {
        aspectRatio_ = aspectRatio;
        SheduleUpdate();
    }
}

void DecalProjection::SetMaterialAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.name_));
}


ResourceRef DecalProjection::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void DecalProjection::SheduleUpdate()
{
    if (isDirty_)
        return;
    isDirty_ = true;

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(DecalProjection, HandlePreRenderEvent));
}

void DecalProjection::HandlePreRenderEvent(StringHash eventName, VariantMap& eventData)
{
    Update();
}

void DecalProjection::Update()
{
    if (isDirty_)
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
        isDirty_ = false;
    }

    auto* octree = GetScene()->GetComponent<Octree>();
    if (!octree)
        return;

    ea::vector<Drawable*> drawables;
    Matrix3x4 frustumTransform(
        node_ ? Matrix3x4(node_->GetWorldPosition(), node_->GetWorldRotation(), 1.0f) : Matrix3x4::IDENTITY);
    Frustum ret;
    ret.Define(fov_, aspectRatio_, 1.0f, nearClip_, farClip_, frustumTransform);

    Frustum frustum;
    FrustumOctreeQuery query{drawables, frustum, DRAWABLE_GEOMETRY};
    octree->GetDrawables(query);

    for (Drawable* drawable: drawables)
    {
        auto* node = drawable->GetNode();
        if (node)
        {
            DecalSet * decalSet = node->CreateComponent<DecalSet>();
            decalSet->SetMaterial(material_);
            //decalSet->AddDecal(drawable, )
        }
    }
}


} // namespace Urho3D
