#include "ShaderResourceBindingCache.h"

#include "GraphicsImpl.h"

namespace Urho3D
{
ShaderResourceBindingCache::ShaderResourceBindingCache(Context* context)
    : Object(context)
{
    graphics_ = GetSubsystem<Graphics>();
}

ShaderResourceBinding* ShaderResourceBindingCache::GetOrCreateSRB(ShaderResourceBindingCacheCreateInfo& createInfo)
{
    assert(createInfo.Complete());
    unsigned hash = createInfo.ToHash();
    auto srbIt = srbMap_.find(hash);

    ShaderResourceBinding* result = nullptr;
    if (srbIt != srbMap_.end() && !srbIt->second.Expired())
        return srbIt->second;

    ShaderResourceBinding* srb = createInfo.pipeline_->CreateSRB();

    for (unsigned i = 0; i < createInfo.constantBuffers_.size(); ++i)
    {
        ConstantBuffer* cbuffer = createInfo.constantBuffers_[i];
        if (cbuffer)
            srb->SetConstantBuffer((ShaderParameterGroup)i, SharedPtr<ConstantBuffer>(cbuffer));
    }
    for (unsigned i = 0; i < createInfo.textures_.size(); ++i)
    {
        Texture* tex = createInfo.textures_[i];
        if (tex)
            srb->SetTexture((TextureUnit)i, SharedPtr<Texture>(tex));
    }

    srbMap_[hash] = WeakPtr<ShaderResourceBinding>(srb);
    return srb;
}

} // namespace Urho3D
