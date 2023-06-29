// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

namespace Urho3D
{

class RawBuffer;
class RawTexture;
class RenderDevice;

/// Pool for different resources used by renderer.
class URHO3D_API RenderPool : public Object
{
    URHO3D_OBJECT(RenderPool, Object);

public:
    explicit RenderPool(RenderDevice* renderDevice);
    ~RenderPool() override;

    void Invalidate();
    void Restore();

    /// Return uniform buffer.
    /// Buffers are recycled immediately, pass different ids to get different buffers.
    /// Don't store the pointer between frames.
    RawBuffer* GetUniformBuffer(unsigned id, unsigned size);

private:
    RenderDevice* renderDevice_{};

    ea::unordered_map<unsigned long long, ea::unique_ptr<RawBuffer>> uniformBuffers_;
};

} // namespace Urho3D
