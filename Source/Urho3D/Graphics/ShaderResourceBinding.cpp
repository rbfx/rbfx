#include "ShaderResourceBinding.h"

#include "GraphicsDefs.h"

namespace Urho3D
{
ShaderResourceBinding::ShaderResourceBinding(Graphics* graphics, Diligent::IShaderResourceBinding* shaderResBindingObj)
    : RefCounted()
    , graphics_(graphics)
    , shaderResBindingObj_(shaderResBindingObj)
    , hash_(0u)
    , dirty_(true)
{
}
ShaderResourceBinding::~ShaderResourceBinding()
{
    ReleaseResources();

    unsigned len = MAX_SHADER_PARAMETER_GROUPS;
    while (len--)
        constantBuffers_[len] = nullptr;

    //len = MAX_TEXTURE_UNITS;
    //while (len--)
    //    textures_[len] = nullptr;
}
void ShaderResourceBinding::SetConstantBuffer(ShaderParameterGroup group, WeakPtr<ConstantBuffer> cbuffer)
{
    if (constantBuffers_[group] != cbuffer)
    {
        constantBuffers_[group] = cbuffer;
        MakeDirty();
    }
}
void ShaderResourceBinding::SetTexture(const ea::string& texUnit, WeakPtr<Texture> texture)
{
    textures_.emplace_back(texUnit, texture);
}
void ShaderResourceBinding::UpdateBindings()
{
    if (!dirty_)
        return;
    UpdateInternalBindings();
}
} // namespace Urho3D
