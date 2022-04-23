//
// Copyright (c) 2017-2020 the Urho3D project.
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

#include "../Precompiled.h"

#include "ComputeDevice.h"

#include "../IO/Log.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/ShaderVariation.h"
#include "../Graphics/Texture.h"
#include "../Graphics/VertexBuffer.h"

#if defined(URHO3D_COMPUTE)

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
    ReleaseLocalState();
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

bool ComputeDevice::SetWriteBuffer(ConstantBuffer* buffer, unsigned unit)
{
    return SetWritableBuffer(buffer, unit);
}

bool ComputeDevice::SetWriteBuffer(VertexBuffer* buffer, unsigned unit)
{
    return SetWritableBuffer(buffer, unit);
}

bool ComputeDevice::SetWriteBuffer(IndexBuffer* buffer, unsigned unit)
{
    return SetWritableBuffer(buffer, unit);
}

bool ComputeDevice::SetWriteBuffer(ComputeBuffer* buffer, unsigned unit)
{
    return SetWritableBuffer(buffer, unit);
}

}

#endif
