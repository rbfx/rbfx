//
// Copyright (c) 2017-2020 the rbfx project.
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
