#include "ComputeDevice.h"

#include "../IO/Log.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/ShaderVariation.h"
#include "../Graphics/Texture.h"


namespace Urho3D
{

ComputeDevice::ComputeDevice(Context* context, Graphics* graphics) :
    Object(context),
    graphics_(graphics),
    constantBuffersDirty_(false),
    uavsDirty_(false),
    samplersDirty_(false),
    texturesDirty_(false),
    programDirty_(false)
{
    Init();
}

ComputeDevice::~ComputeDevice()
{
    //ReleaseLocalState();
}

bool ComputeDevice::SetProgram(SharedPtr<ShaderVariation> shaderVariation)
{
    if (shaderVariation && shaderVariation->GetShaderType() != CS)
    {
        URHO3D_LOGERROR("Attempted to provide a non-compute shader to compute");
        return false;
    }

    computeShader_ = shaderVariation;
    programDirty_ = true;
    return true;
}

}
