#pragma once
#include "../Graphics.h"
#include "DiligentGraphicsImpl.h"
#include <EASTL/vector.h>
namespace Urho3D
{
    /// <summary>
    /// This class will be used to store and handle constant buffer allocations.
    /// </summary>
    class DiligentConstantBufferManager {
    public:
        DiligentConstantBufferManager(Graphics* graphics);
        void Release();
        Diligent::IBuffer* GetBufferBySize(unsigned bufferSize);
        Diligent::IBuffer* GetOrCreateBuffer(ShaderParameterGroup shaderParamGrp);
    private:
        Diligent::IBuffer* Allocate(ShaderParameterGroup shaderParamGrp);

        ea::array<Diligent::IBuffer*, ShaderParameterGroup::MAX_SHADER_PARAMETER_GROUPS> buffers_;
        Graphics* graphics_;
    };
}
