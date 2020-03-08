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

#pragma once

#include "../Core/Object.h"
#include "../Graphics/GPUObject.h"
#include "../Graphics/GraphicsDefs.h"

class ID3D11UnorderedAccessView;

#if defined(URHO3D_COMPUTE)

namespace Urho3D
{

class URHO3D_API ComputeBuffer : public Object, public GPUObject
{
    URHO3D_OBJECT(ComputeBuffer, Object);

    using GPUObject::GetGraphics;
public:
    ComputeBuffer(Context*);
    virtual ~ComputeBuffer();

    /// Register object with the engine.
    static void RegisterObject(Context* context);

    /// Mark the buffer destroyed on graphics context destruction. May be a no-op depending on the API.
    void OnDeviceLost() override;
    /// Recreate the buffer and restore data if applicable. May be a no-op depending on the API.
    void OnDeviceReset() override;
    /// Release buffer.
    void Release() override;

    /// Sets the size and attempts to construct the buffer.
    bool SetSize(unsigned bytes, unsigned structureSize);
    /// Sets the data if possible.
    bool SetData(void* data, unsigned dataSize, unsigned structureSize);
    /// Gets the data from the GPU.
    bool GetData(void* writeInto, unsigned offset, unsigned lengthOfRead);

    /// Return total size in bytes of the buffer.
    inline unsigned GetSize() const { return size_; }
    /// Return the size of a single struct/element of the buffer.
    inline unsigned GetStructSize() const { return structureSize_; }
    /// Return the number of structs in the buffer.
    inline unsigned GetNumElements() const { return size_ / structureSize_; }

#if defined(URHO3D_D3D11)
    ID3D11UnorderedAccessView* GetUAV() const { return uav_; }
#endif

private:
    unsigned size_;
    unsigned structureSize_;
#if defined(URHO3D_D3D11)
    ID3D11UnorderedAccessView* uav_;
#endif
};

}

#endif
