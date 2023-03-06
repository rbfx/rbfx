//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/ConstantBuffer.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Urho3D
{

void ConstantBuffer::OnDeviceReset()
{
    // No-op on Direct3D11
}

void ConstantBuffer::Release()
{
    URHO3D_SAFE_RELEASE(object_.ptr_);
    size_ = 0;
}

bool ConstantBuffer::SetSize(unsigned size)
{
    Release();
    if (!size)
    {
        URHO3D_LOGERROR("Can not create zero-sized constant buffer");
        return false;
    }

    // Round up to next 16 bytes
    size += 15;
    size &= 0xfffffff0;

    size_ = size;

    if (graphics_)
    {
        using namespace Diligent;
        BufferDesc bufferDesc;
        bufferDesc.Size = size_;
        bufferDesc.Usage = USAGE_DEFAULT;
        bufferDesc.CPUAccessFlags = CPU_ACCESS_NONE;
        bufferDesc.BindFlags = BIND_UNIFORM_BUFFER;
        //bufferDesc.Mode = BUFFER_MODE_RAW;

        graphics_->GetImpl()->GetDevice()->CreateBuffer(bufferDesc, 0, (IBuffer**)&object_.ptr_);
        if (object_.ptr_ == nullptr) {
            URHO3D_LOGERROR("Failed to create constant buffer. See logs!");
            return false;
        }
    }

    BuildHash();
    return true;
}

void ConstantBuffer::Update(const void* data)
{
    using namespace Diligent;
    if (object_.ptr_) {
        graphics_->GetImpl()->GetDeviceContext()->UpdateBuffer((IBuffer*)object_.ptr_, 0, size_, data, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
}

}
