// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Container/RefCounted.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/string.h>

namespace Urho3D
{

class Context;
class RenderDevice;

/// Base class for GPU resources.
class URHO3D_API DeviceObject
{
public:
    explicit DeviceObject(Context* context);
    virtual ~DeviceObject();

    /// Invalidate GPU data.
    virtual void Invalidate() {}
    /// Restore GPU data if possible.
    virtual void Restore() {}
    /// Destroy all GPU data on RenderDevice shutdown.
    virtual void Destroy() {}

    RenderDevice* GetRenderDevice() const { return renderDevice_; }
    bool IsDataLost() const { return dataLost_; }
    void ClearDataLost() { dataLost_ = false; }
    const ea::string& GetDebugName() const { return debugName_; }
    void SetDebugName(const ea::string& debugName) { debugName_ = debugName; }

    /// Internal: process device object event.
    void ProcessDeviceObjectEvent(DeviceObjectEvent event);

protected:
    /// Helper function to restore another object if present.
    void RestoreDependency(DeviceObject* dependency);

    /// Render device that owns this object.
    WeakPtr<RenderDevice> renderDevice_;
    /// Debug name of the object.
    ea::string debugName_;
    /// Android only: whether the data is lost due to context loss.
    bool dataLost_{};
};

} // namespace Urho3D
