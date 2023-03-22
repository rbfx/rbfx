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
#include "./ConstantBuffer.h"
#include "./Texture.h"
#include "./Graphics.h"

#ifdef URHO3D_DILIGENT
#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
#endif

namespace Urho3D
{
    class URHO3D_API ShaderResourceBinding : public RefCounted {
    public:
        ShaderResourceBinding(Graphics* graphics, Diligent::IShaderResourceBinding* shaderResBindingObj = nullptr);
        ~ShaderResourceBinding();
        void SetConstantBuffer(ShaderParameterGroup group, WeakPtr<ConstantBuffer> cbuffer);
        void SetTexture(TextureUnit texUnit, WeakPtr<Texture> texture);
        WeakPtr<ConstantBuffer> GetConstantBuffer(ShaderParameterGroup group) { return constantBuffers_[group]; }
        WeakPtr<Texture> GetTexture(TextureUnit texUnit) { return textures_[texUnit]; }
        bool IsDirty() { return dirty_; }
        void MakeDirty() { dirty_ = true; }
        void UpdateBindings();
        unsigned ToHash() { return hash_; }
#ifdef URHO3D_DILIGENT
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> GetShaderResourceBinding() const { return shaderResBindingObj_; }
#endif
    private:
#ifdef URHO3D_DILIGENT
        void UpdateInternalBindings();
        void ReleaseResources();
#endif
        ea::array<WeakPtr<ConstantBuffer>, MAX_SHADER_PARAMETER_GROUPS> constantBuffers_;
        ea::array<WeakPtr<Texture>, MAX_TEXTURE_UNITS> textures_;

        bool dirty_;
        unsigned hash_;

#ifdef URHO3D_DILIGENT
        // Pointer to Diligent Shader Resource Binding
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResBindingObj_{};
#endif
        WeakPtr<Graphics> graphics_;
    };
}
