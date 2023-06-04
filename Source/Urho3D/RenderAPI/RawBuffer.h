// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/ByteVector.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderAPI/DeviceObject.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/Buffer.h>

#include <EASTL/functional.h>
#include <EASTL/optional.h>
#include <EASTL/shared_array.h>

namespace Urho3D
{

/// Common class for buffer on GPU and/or CPU.
class URHO3D_API RawBuffer : public Object, public DeviceObject
{
    URHO3D_OBJECT(RawBuffer, Object);

public:
    explicit RawBuffer(Context* context);
    ~RawBuffer() override;

    /// Implement DeviceObject.
    /// @{
    void Invalidate() override;
    void Restore() override;
    void Destroy() override;
    /// @}

    /// Update buffer data. If size is not set, the buffer size is used.
    void Update(const void* data, unsigned size = 0);
    /// Update buffer data range.
    void UpdateRange(const void* data, unsigned offset, unsigned size);

    /// Map buffer contents onto CPU-writeable memory. Return data pointer if successful. Old buffer data is discarded.
    /// @note Memory is not guaranteed to be CPU-readable! Consider using GetShadowData() if you need to read the data.
    void* Map();
    /// Unmap the buffer. Should always be called before buffer is used or frame ends.
    void Unmap();

    /// For dynamic buffers, ensure that the buffer can be used in this frame.
    /// Don't access the GPU buffer data until the buffer was resolved!
    void Resolve();

    /// Getters.
    /// @{
    BufferType GetBufferType() const { return type_; }
    unsigned GetSize() const { return size_; }
    unsigned GetStride() const { return stride_; }
    BufferFlags GetFlags() const { return flags_; }
    bool IsLocked() const { return !!unlockImpl_; }
    bool IsShadowed() const { return flags_.Test(BufferFlag::Shadowed); }
    unsigned char* GetShadowData() { return shadowData_.get(); }
    ea::shared_array<unsigned char> GetShadowDataShared() { return shadowData_; }
    const unsigned char* GetShadowData() const { return shadowData_.get(); }
    Diligent::IBuffer* GetHandle() const { return const_cast<Diligent::IBuffer*>(handle_.RawPtr()); }
    /// @}

protected:
    /// Create buffer. If data is provided, it should contain exactly `size` bytes.
    /// TODO(diligent): This function should be public but it's easy to misuse if called for derived classes.
    bool Create(BufferType type, unsigned size, unsigned stride, BufferFlags flags, const void* data = nullptr);

private:
    bool CreateGPU(const void* data);

    BufferType type_{};
    unsigned size_{};
    unsigned stride_{};
    BufferFlags flags_{};

    Diligent::RefCntAutoPtr<Diligent::IBuffer> handle_;
    Diligent::USAGE internalUsage_{};
    ea::shared_array<unsigned char> shadowData_;

    bool needResolve_{};
    ea::optional<FrameIndex> lastUpdateFrameIndex_;

    ea::function<void()> unlockImpl_;
};

} // namespace Urho3D
