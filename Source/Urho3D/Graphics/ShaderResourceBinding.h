#pragma once
#include "./ConstantBuffer.h"
#include "./Texture.h"
#include "./Graphics.h"
namespace Urho3D
{
    class URHO3D_API ShaderResourceBinding : public RefCounted {
    public:
        ShaderResourceBinding(Graphics* graphics, void* shaderResBindingObj = nullptr);
        ~ShaderResourceBinding();
        void SetConstantBuffer(ShaderParameterGroup group, SharedPtr<ConstantBuffer> cbuffer);
        void SetTexture(TextureUnit texUnit, SharedPtr<Texture> texture);
        SharedPtr<ConstantBuffer> GetConstantBuffer(ShaderParameterGroup group) { return constantBuffers_[group]; }
        SharedPtr<Texture> GetTexture(TextureUnit texUnit) { return textures_[texUnit]; }
        bool IsDirty() { return dirty_; }
        void MakeDirty() { dirty_ = true; }
        void UpdateBindings();
        unsigned ToHash() { return hash_; }
#ifdef URHO3D_DILIGENT
        void* GetShaderResourceBinding() { return shaderResBindingObj_; }
#endif
    private:
#ifdef URHO3D_DILIGENT
        void UpdateInternalBindings();
        void ReleaseResources();
#endif
        ea::array<SharedPtr<ConstantBuffer>, MAX_SHADER_PARAMETER_GROUPS> constantBuffers_;
        ea::array<SharedPtr<Texture>, MAX_TEXTURE_UNITS> textures_;

        bool dirty_;
        unsigned hash_;

#ifdef URHO3D_DILIGENT
        // Pointer to Diligent Shader Resource Binding
        void* shaderResBindingObj_;
#endif
        WeakPtr<Graphics> graphics_;
    };
}
