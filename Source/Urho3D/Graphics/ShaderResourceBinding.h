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
