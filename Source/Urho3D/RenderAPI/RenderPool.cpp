// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderPool.h"

#include "Urho3D/RenderAPI/RawBuffer.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

RenderPool::RenderPool(RenderDevice* renderDevice)
    : Object(renderDevice->GetContext())
    , renderDevice_(renderDevice)
{
}

RenderPool::~RenderPool()
{
}

void RenderPool::Invalidate()
{
    uniformBuffers_.clear();
}

void RenderPool::Restore()
{
}

RawBuffer* RenderPool::GetUniformBuffer(unsigned id, unsigned size)
{
    const unsigned sizeQuant = 512;
    size = (size + sizeQuant - 1) / sizeQuant * sizeQuant;

    const unsigned long long key = (static_cast<unsigned long long>(id) << 32) | size;
    auto& uniformBuffer = uniformBuffers_[key];

    if (!uniformBuffer)
    {
        RawBufferParams params;
        params.type_ = BufferType::Uniform;
        params.size_ = size;
        params.flags_ = BufferFlag::Dynamic | BufferFlag::Discard;

        uniformBuffer = ea::make_unique<RawBuffer>(context_, params);
    }
    return uniformBuffer.get();
}

} // namespace Urho3D
