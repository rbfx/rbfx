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

#include "../../Precompiled.h"

#include "../../Graphics/ComputeBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../IO/Log.h"

#if defined(URHO3D_COMPUTE)

namespace Urho3D
{

void ComputeBuffer::OnDeviceLost()
{
    dataLost_ = true;
    Release();
}

void ComputeBuffer::OnDeviceReset()
{
    // ComputeBuffers do not attempt to restore with a shadow or such.
    if (dataLost_)
        SetSize(size_, structureSize_);
    dataLost_ = false;
}

void ComputeBuffer::Release()
{
    if (object_.name_)
        glDeleteBuffers(1, &object_.name_);
    object_.name_ = 0;
}

bool ComputeBuffer::SetSize(unsigned bytes, unsigned structureSize)
{
    if (object_.name_)
        Release();

    if (size_ == 0 || structureSize_ == 0)
    {
        URHO3D_LOGERROR("Unable to created ComputeBuffer with size: {} and struct-size: {}", size_, structureSize_);
        return false;
    }

    size_ = bytes;
    structureSize_ = structureSize;

    glGenBuffers(1, &object_.name_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, object_.name_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, nullptr, GL_DYNAMIC_COPY);
    return true;
}

bool ComputeBuffer::SetData(void* data, unsigned dataSize, unsigned structureSize)
{
    if (object_.name_ == 0)
        return false;

    if (!graphics_->IsDeviceLost())
    {
        URHO3D_LOGERROR("ComptueBuffer::SetData, attempted to call while device is lost");
        return false;
    }

    if (dataSize != size_ || structureSize_ != structureSize)
    {
        if (!SetSize(dataSize, structureSize))
        {
            URHO3D_LOGERROR("Failed to resize compute buffer to {} bytes with struct-size {}", dataSize, structureSize);
            return false;
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, object_.name_);
    GLvoid* mappedData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(mappedData, &data, dataSize);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    return true;
}

bool ComputeBuffer::GetData(void* writeInto, unsigned offset, unsigned readLength)
{
    if (object_.name_ == 0)
        return false;

    if (!graphics_->IsDeviceLost())
    {
        URHO3D_LOGERROR("ComptueBuffer::GetData, attempted to call while device is lost");
        return false;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, object_.name_);
    GLvoid* mappedData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    memcpy(writeInto, ((unsigned char*)mappedData) + offset, readLength);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    return true;
}

}

#endif
