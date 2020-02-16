#include "ComputeDevice.h"

#include "../Graphics/ShaderVariation.h"

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

}

ComputeDevice::~ComputeDevice()
{
    ReleaseLocalState();
}

}
