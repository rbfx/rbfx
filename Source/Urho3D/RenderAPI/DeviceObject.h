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

    /// Android only: Invalidate GPU data.
    virtual void Invalidate() {}
    /// Android only: Restore GPU data if possible.
    virtual void Restore() {}
    /// Destroy all GPU data on RenderDevice shutdown.
    virtual void Destroy() {}

    RenderDevice* GetRenderDevice() const { return renderDevice_; }
    bool IsDataLost() const { return dataLost_; }
    void ClearDataLost() { dataLost_ = false; }
    const ea::string& GetDebugName() const { return debugName_; }
    void SetDebugName(const ea::string& debugName) { debugName_ = debugName; }

    /// TODO(diligent): Remove this
    class Graphics* GetGraphics() const { return graphics_; }

    /// Internal: process device object event.
    void ProcessDeviceObjectEvent(DeviceObjectEvent event);

protected:
    /// Render device that owns this object.
    WeakPtr<RenderDevice> renderDevice_;
    /// Debug name of the object.
    ea::string debugName_;
    /// Android only: whether the data is lost due to context loss.
    bool dataLost_{};

    /// TODO(diligent): Remove this
    WeakPtr<class Graphics> graphics_;
};

} // namespace Urho3D
