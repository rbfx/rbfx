#include "DiligentConstantBufferManager.h"
#include "../../Graphics/Light.h"
namespace Urho3D
{
    // TODO: This value must match DrawableProcessorSettings, default in the code is 4
    // but we can create in the future a macro to normalize this.
    static constexpr unsigned sMaxVertexLight = 4;
    // TODO: Change this to a macro
    static constexpr unsigned sMaxBones = 128;
    // This sizes has been based on _Uniforms.glsl file
    static constexpr unsigned sBufferSizes[] = {
        // Frame CB Size
        sizeof(float) * 4,
        // Camera CB Size
        (sizeof(Matrix4) * 3) + sizeof(Vector4) +
        sizeof(Vector3) + (sizeof(float) * 2) +
        sizeof(Vector4) + sizeof(Vector3) +
        (sizeof(Vector4) * 2) + sizeof(Vector2) +
        (sizeof(Vector4) * 2) + sizeof(Vector3) + sizeof(float),
        // Zone CB Size
        (sizeof(Vector4) * 6) + (sizeof(float) * 3),
        // Light CB Size
        sizeof(Vector4) + sizeof(Vector3) + sizeof(Vector2) +
        sizeof(Matrix4) + (sizeof(Vector4) * sMaxVertexLight * 3) +
        (sizeof(Matrix4) * MAX_CASCADE_SPLITS) + (sizeof(Vector4) * 2) +
        sizeof(Vector2) + sizeof(Vector4) + (sizeof(Vector2) * 2) +
        sizeof(Vector4) + sizeof(Vector2) + (sizeof(float) * 2),
        // Material CB Size
        (sizeof(Vector4) * 2) + sizeof(Vector4) +
        sizeof(Vector4)+sizeof(Vector3) + sizeof(float) +
        sizeof(Vector3) + sizeof(float) + sizeof(Vector4) +
        sizeof(Vector2) + (sizeof(float) * 2) + sizeof(Vector4) * 4,
        // Object CB Size
        sizeof(Matrix4) + (sizeof(Vector4) * 7) + sizeof(Vector4) +
        sizeof(Matrix3) + (sizeof(Vector4) * sMaxBones * 3),
        // Custom CB Size
        16384 // Same size of OpenGL uniform buffer
    };
    static constexpr const char* sBufferNames[] = {
        "FrameCB",
        "CameraCB",
        "ZoneCB",
        "LightCB",
        "MaterialCB",
        "ObjectCB",
        "CustomCB"
    };
    DiligentConstantBufferManager::DiligentConstantBufferManager(Graphics* graphics) :
        graphics_(graphics) {
        unsigned length = ShaderParameterGroup::MAX_SHADER_PARAMETER_GROUPS;
        while (length--)
            buffers_[length] = nullptr;
    }

    void DiligentConstantBufferManager::Release()
    {
        // De-allocate all buffers
        unsigned length = buffers_.size();
        while (--length) {
            if (buffers_[length]) {
                buffers_[length]->Release();
                buffers_[length] = nullptr;
            }
        }
    }

    Diligent::IBuffer* DiligentConstantBufferManager::GetBufferBySize(unsigned bufferSize)
    {
        for (unsigned i = 0; i < buffers_.size(); ++i) {
            unsigned currSize = sBufferSizes[i];
            if (bufferSize < currSize)
                return GetOrCreateBuffer(static_cast<ShaderParameterGroup>(i));
        }
        return nullptr;
    }

    Diligent::IBuffer* DiligentConstantBufferManager::GetOrCreateBuffer(ShaderParameterGroup shaderParamGrp)
    {
        Diligent::IBuffer* buffer = buffers_[shaderParamGrp];
        if (!buffer)
            buffer = Allocate(shaderParamGrp);
        return buffer;
    }
    Diligent::IBuffer* DiligentConstantBufferManager::Allocate(ShaderParameterGroup shaderParamGrp)
    {
        using namespace Diligent;
        BufferDesc desc;
        desc.Name = sBufferNames[shaderParamGrp];
        desc.Size = sBufferSizes[shaderParamGrp];
        desc.Usage = USAGE_DYNAMIC;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        desc.BindFlags = BIND_UNIFORM_BUFFER;

        Diligent::IBuffer* buffer = nullptr;
        graphics_->GetImpl()->GetDevice()->CreateBuffer(desc, nullptr, &buffer);
        return buffers_[shaderParamGrp] = buffer;
    }
}
