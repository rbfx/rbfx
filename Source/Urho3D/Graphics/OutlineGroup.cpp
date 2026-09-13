// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/OutlineGroup.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Scene/Scene.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Material.h"

namespace Urho3D
{

OutlineGroup::MaterialKey::MaterialKey(const Material& material)
{
    for (const auto& [nameHash, nameValue] : material.GetShaderParameters())
    {
        if (nameHash != ShaderConsts::Material_MatDiffColor)
        {
            unsigned hash = 0;
            CombineHash(hash, nameHash.Value());
            CombineHash(hash, nameValue.value_.ToHash());
            parametersHash_ += hash;
        }
    }

    for (const auto& [nameHash, texture] : material.GetTextures())
    {
        unsigned hash = 0;
        CombineHash(hash, nameHash.Value());
        CombineHash(hash, MakeHash(texture.value_.Get()));
        resourcesHash_ += hash;
    }
}

bool OutlineGroup::MaterialKey::operator==(const MaterialKey& rhs) const
{
    return parametersHash_ == rhs.parametersHash_ && resourcesHash_ == rhs.resourcesHash_;
}

unsigned OutlineGroup::MaterialKey::ToHash() const
{
    unsigned hash = 0;
    CombineHash(hash, parametersHash_);
    CombineHash(hash, resourcesHash_);
    return hash;
}

OutlineGroup::OutlineGroup(Context* context)
    : BaseClassName(context)
{
}

OutlineGroup::~OutlineGroup()
{
}

void OutlineGroup::RegisterObject(Context* context)
{
    context->AddFactoryReflection<OutlineGroup>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColor, SetColor, Color, Color::WHITE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Render Order", GetRenderOrder, SetRenderOrder, unsigned, DEFAULT_RENDER_ORDER, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Debug", IsDebug, SetDebug, bool, false, AM_DEFAULT);
    // TODO: Not resolved on load
    URHO3D_ACCESSOR_ATTRIBUTE("Drawables", GetDrawablesAttr, SetDrawablesAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
}

void OutlineGroup::ApplyAttributes()
{
    auto scene = GetScene();
    if (drawablesDirty_ && scene)
    {
        drawablesDirty_ = false;
        drawables_.clear();
        for (const Variant& drawableId : drawablesAttr_)
        {
            if (Component* component = scene->GetComponent(drawableId.GetUInt()))
            {
                if (Drawable* drawable = component->Cast<Drawable>())
                    AddDrawable(drawable);
            }
        }
    }
}

void OutlineGroup::SetColor(const Color& color)
{
    if (color_ != color)
    {
        color_ = color;

        for (const auto& [_, material] : materials_)
            material->SetShaderParameter(ShaderConsts::Custom_OutlineColor, color_.ToVector4(), true);
    }
}

void OutlineGroup::SetRenderOrder(unsigned renderOrder)
{
    if (renderOrder_ != renderOrder)
    {
        renderOrder_ = renderOrder;

        for (const auto& [_, material] : materials_)
            material->SetRenderOrder(renderOrder_);
    }
}

void OutlineGroup::SetDrawablesAttr(const VariantVector& drawables)
{
    drawables_.clear();
    drawablesAttr_ = drawables;
    drawablesDirty_ = !drawablesAttr_.empty();
}

const VariantVector& OutlineGroup::GetDrawablesAttr() const
{
    drawablesAttr_.clear();
    for (Drawable* drawable : drawables_)
    {
        if (drawable)
            drawablesAttr_.push_back(drawable->GetID());
    }
    return drawablesAttr_;
}

void OutlineGroup::ClearDrawables()
{
    drawables_.clear();
}

bool OutlineGroup::HasDrawable(Drawable* drawable) const
{
    return drawables_.contains(WeakPtr<Drawable>(drawable));
}

bool OutlineGroup::AddDrawable(Drawable* drawable)
{
    return drawables_.emplace(drawable).second;
}

bool OutlineGroup::RemoveDrawable(Drawable* drawable)
{
    return drawables_.erase(WeakPtr<Drawable>(drawable)) > 0;
}

Material* OutlineGroup::GetOutlineMaterial(Material* referenceMaterial)
{
    const MaterialKey key{*referenceMaterial};
    if (auto iter = materials_.find(key); iter != materials_.end())
        return iter->second.Get();

    auto material = MakeShared<Material>(context_);
    for (const auto& [_, nameValue] : referenceMaterial->GetShaderParameters())
        material->SetShaderParameter(nameValue.name_, nameValue.value_);
    for (const auto& [_, texture] : referenceMaterial->GetTextures())
        material->SetTexture(texture.name_, texture.value_);

    material->SetShaderParameter(ShaderConsts::Custom_OutlineColor, color_.ToVector4(), true);
    material->SetRenderOrder(renderOrder_);

    materials_[key] = material;
    return material;
}

}
