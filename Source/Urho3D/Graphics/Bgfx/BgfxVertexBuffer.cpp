//
// Copyright (c) 2008-2017 the Urho3D project.
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
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Urho3D
{

static const bgfx::Attrib::Enum bgfxAttrib[] =
{
    bgfx::Attrib::Position,
    bgfx::Attrib::Normal,
    bgfx::Attrib::Bitangent,
    bgfx::Attrib::Tangent,
    bgfx::Attrib::TexCoord0,
    bgfx::Attrib::Color0,
    bgfx::Attrib::Weight,
    bgfx::Attrib::Indices,
    bgfx::Attrib::TexCoord7 // SEM_OBJECTINDEX
};

static const uint8_t bgfxAttribSize[] = { 1, 1, 2, 3, 4, 4, 4 };

static const bgfx::AttribType::Enum bgfxAttribType[] =
{
    bgfx::AttribType::Int16,
    bgfx::AttribType::Float,
    bgfx::AttribType::Float,
    bgfx::AttribType::Float,
    bgfx::AttribType::Float,
    bgfx::AttribType::Uint8,
    bgfx::AttribType::Uint8
};

static const bgfx::Attrib::Enum bgfxAttribTexcoords[] =
{
    bgfx::Attrib::TexCoord0,
    bgfx::Attrib::TexCoord1,
    bgfx::Attrib::TexCoord2,
    bgfx::Attrib::TexCoord3,
    bgfx::Attrib::TexCoord4,
    bgfx::Attrib::TexCoord5,
    bgfx::Attrib::TexCoord6,
    bgfx::Attrib::TexCoord7
};

static const bgfx::Attrib::Enum bgfxAttribColors[] =
{
    bgfx::Attrib::Color0,
    bgfx::Attrib::Color1,
    bgfx::Attrib::Color2,
    bgfx::Attrib::Color3
};

void VertexBuffer::OnDeviceLost()
{

}

void VertexBuffer::OnDeviceReset()
{

}

void VertexBuffer::Release()
{
    Unlock();

    if (object_.idx_ != bgfx::kInvalidHandle)
    {
        if (!graphics_)
            return;

        for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
        {
            if (graphics_->GetVertexBuffer(i) == this)
                graphics_->SetVertexBuffer(nullptr);
        }

        if (dynamic_)
        {
            bgfx::DynamicVertexBufferHandle handle;
            handle.idx = object_.idx_;
            bgfx::destroy(handle);
        }
        else
        {
            bgfx::VertexBufferHandle handle;
            handle.idx = object_.idx_;
            bgfx::destroy(handle);
        }

        object_.idx_ = bgfx::kInvalidHandle;
    }
}

bool VertexBuffer::SetData(const void* data)
{
    if (!data)
    {
        URHO3D_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    if (object_.idx_ != bgfx::kInvalidHandle && !dynamic_)
    {
        URHO3D_LOGERROR("Cannot set data on a static vertex buffer");
        return false;
    }

    if (shadowData_ && data != shadowData_.Get())
        memcpy(shadowData_.Get(), data, vertexCount_ * vertexSize_);

    if (object_.idx_ != bgfx::kInvalidHandle && dynamic_)
    {
        bgfx::DynamicVertexBufferHandle handle;
        handle.idx = object_.idx_;
        bgfx::updateDynamicVertexBuffer(handle, 0, bgfx::makeRef(data, vertexCount_ * vertexSize_));
    }

    if (object_.idx_ == bgfx::kInvalidHandle && !dynamic_)
    {
        bgfx::VertexDecl decl;
        decl.begin();
        for (PODVector<VertexElement>::ConstIterator i = elements_.Begin(); i != elements_.End(); ++i)
        {
            bgfx::Attrib::Enum attrib = bgfxAttrib[i->semantic_];
            bgfx::AttribType::Enum type = bgfxAttribType[i->type_];
            bool normalized = false;
            if (attrib == bgfx::Attrib::TexCoord0)
                attrib = bgfxAttribTexcoords[i->index_];
            else if (attrib == bgfx::Attrib::Color0)
                attrib = bgfxAttribColors[i->index_];
            if (type == bgfx::AttribType::Uint8)
                normalized = true;
            decl.add(attrib, bgfxAttribSize[i->type_], type, normalized, false);
        }
        decl.end();

        bgfx::VertexBufferHandle handle;
        handle = bgfx::createVertexBuffer(bgfx::makeRef(data, vertexCount_ * vertexSize_), decl);
        object_.idx_ = handle.idx;

        if (object_.idx_ == bgfx::kInvalidHandle)
        {
            URHO3D_LOGERROR("Failed to create static vertex buffer");
            return false;
        }
    }


    dataLost_ = false;
    return true;
}

bool VertexBuffer::SetDataRange(const void* data, unsigned start, unsigned count, bool discard)
{
    if (start == 0 && count == vertexCount_)
        return SetData(data);

    if (!dynamic_ && !discard)
    {
        URHO3D_LOGERROR("Vertex Buffer is not dynamic");
        return false;
    }

    if (!data)
    {
        URHO3D_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    if (start + count > vertexCount_)
    {
        URHO3D_LOGERROR("Illegal range for setting new vertex buffer data");
        return false;
    }

    if (!count)
        return true;

    if (shadowData_ && shadowData_.Get() + start * vertexSize_ != data)
        memcpy(shadowData_.Get() + start * vertexSize_, data, count * vertexSize_);

    if (object_.idx_ != bgfx::kInvalidHandle && dynamic_)
    {
        bgfx::DynamicVertexBufferHandle handle;
        handle.idx = object_.idx_;
        bgfx::updateDynamicVertexBuffer(handle, start * vertexSize_, bgfx::makeRef(data, count * vertexSize_));
    }

    return true;
}

void* VertexBuffer::Lock(unsigned start, unsigned count, bool discard)
{
    if (lockState_ != LOCK_NONE)
    {
        URHO3D_LOGERROR("Vertex buffer already locked");
        return nullptr;
    }

    if (!vertexSize_)
    {
        URHO3D_LOGERROR("Vertex elements not defined, can not lock vertex buffer");
        return nullptr;
    }

    if (start + count > vertexCount_)
    {
        URHO3D_LOGERROR("Illegal range for locking vertex buffer");
        return nullptr;
    }

    if (!count)
        return nullptr;

    lockStart_ = start;
    lockCount_ = count;
    discardLock_ = discard;

    if (shadowData_)
    {
        lockState_ = LOCK_SHADOW;
        return shadowData_.Get() + start * vertexSize_;
    }
    else if (graphics_)
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = graphics_->ReserveScratchBuffer(count * vertexSize_);
        return lockScratchData_;
    }
    else
        return nullptr;
}

void VertexBuffer::Unlock()
{
    switch (lockState_)
    {
    case LOCK_SHADOW:
        SetDataRange(shadowData_.Get() + lockStart_ * vertexSize_, lockStart_, lockCount_, discardLock_);
        lockState_ = LOCK_NONE;
        break;

    case LOCK_SCRATCH:
        SetDataRange(lockScratchData_, lockStart_, lockCount_, discardLock_);
        if (graphics_)
            graphics_->FreeScratchBuffer(lockScratchData_);
        lockScratchData_ = nullptr;
        lockState_ = LOCK_NONE;
        break;

    default:
        break;
    }
}

bool VertexBuffer::Create()
{
    if (!vertexCount_ || !elementMask_)
    {
        Release();
        return true;
    }

    if (graphics_)
    {
        if (object_.idx_ == bgfx::kInvalidHandle && dynamic_)
        {
            bgfx::VertexDecl decl;
            decl.begin();
            for (PODVector<VertexElement>::ConstIterator i = elements_.Begin(); i != elements_.End(); ++i)
            {
                bgfx::Attrib::Enum attrib = bgfxAttrib[i->semantic_];
                bgfx::AttribType::Enum type = bgfxAttribType[i->type_];
                bool normalized = false;
                if (attrib == bgfx::Attrib::TexCoord0)
                    attrib = bgfxAttribTexcoords[i->index_];
                else if (attrib == bgfx::Attrib::Color0)
                    attrib = bgfxAttribColors[i->index_];
                if (type == bgfx::AttribType::Uint8)
                    normalized = true;
                decl.add(attrib, bgfxAttribSize[i->type_], type, normalized, false);
            }
            decl.end();

            bgfx::DynamicVertexBufferHandle handle;
            handle = bgfx::createDynamicVertexBuffer(vertexCount_, decl);
            object_.idx_ = handle.idx;
        }
        if (object_.idx_ == bgfx::kInvalidHandle && dynamic_)
        {
            URHO3D_LOGERROR("Failed to create vertex buffer");
            return false;
        }
    }

    return true;
}

bool VertexBuffer::UpdateToGPU()
{
    if (object_.name_ && shadowData_)
        return SetData(shadowData_.Get());
    else
        return false;
}

void* VertexBuffer::MapBuffer(unsigned start, unsigned count, bool discard)
{
    return nullptr;
}

void VertexBuffer::UnmapBuffer()
{
}

}
