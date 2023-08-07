//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Graphics/OutlineGroup.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Scene/Scene.h"

#include "../Core/Context.h"
#include "../Graphics/Material.h"

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
