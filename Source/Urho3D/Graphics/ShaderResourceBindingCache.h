#pragma once
#include "./ShaderResourceBinding.h"
#include "./PipelineState.h"
#include "./Graphics.h"

namespace Urho3D
{
    struct ShaderResourceBindingCacheCreateInfo {
        PipelineState* pipeline_{};
        ea::array<ConstantBuffer*, MAX_SHADER_PARAMETER_GROUPS> constantBuffers_{};
        ea::array<Texture*, MAX_TEXTURE_UNITS> textures_{};

        unsigned hash_{ 0u };
        unsigned ToHash() {
            hash_ = MakeHash((unsigned long long)pipeline_);
            for (unsigned i = 0; i < constantBuffers_.size(); ++i) {
                if (constantBuffers_[i] != nullptr)
                    CombineHash(hash_, constantBuffers_[i]->ToHash());
            }
            for (unsigned i = 0; i < textures_.size(); ++i) {
                if (textures_[i] != nullptr)
                    CombineHash(hash_, (unsigned long long)textures_[i]);
            }
            return hash_;
        }

        bool Complete() {
            return pipeline_ != nullptr;
        }

        void ResetTextures() {
            unsigned i = MAX_TEXTURE_UNITS;
            while (i--)
                textures_[i] = nullptr;
        }
        void ResetConstantBuffers() {
            unsigned i = MAX_SHADER_PARAMETER_GROUPS;
            while (i--)
                constantBuffers_[i] = nullptr;
        }
    };
    class URHO3D_API ShaderResourceBindingCache : public Object {
        URHO3D_OBJECT(ShaderResourceBindingCache, Object);
    public:
        ShaderResourceBindingCache(Context* context);
        ShaderResourceBinding* GetOrCreateSRB(ShaderResourceBindingCacheCreateInfo& createInfo);
    private:
        ea::unordered_map<unsigned, WeakPtr<ShaderResourceBinding>> srbMap_;

        WeakPtr<Graphics> graphics_;
    };
}
