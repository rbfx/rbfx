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

#include "../Container/Ptr.h"

#ifdef URHO3D_DILIGENT
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceObject.h>
#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#endif
namespace Urho3D
{

class Graphics;

/// API-specific GPU object representation.
union GPUObjectHandle
{
    /// Object pointer (Direct3D).
    void* ptr_;
    /// Object name (OpenGL).
    unsigned name_;
};

/// Base class for GPU resources.
class URHO3D_API GPUObject
{
public:
    /// Construct with graphics subsystem pointer.
    explicit GPUObject(Graphics* graphics);
    /// Destruct. Remove from the Graphics.
    virtual ~GPUObject();

    /// Mark the GPU resource destroyed on graphics context destruction.
    virtual void OnDeviceLost();
    /// Recreate the GPU resource and restore data if applicable.
    virtual void OnDeviceReset();
    /// Unconditionally release the GPU resource.
    virtual void Release();

    /// Clear the data lost flag.
    void ClearDataLost();

    /// Return the graphics subsystem associated with this GPU object.
    Graphics* GetGraphics() const;
#ifdef URHO3D_DILIGENT
    Diligent::RefCntAutoPtr<Diligent::IDeviceObject> GetGPUObject() const { return object_; }
#else
    /// Return the object pointer. Applicable only on Direct3D.
    void* GetGPUObject() const { return object_.ptr_; }
    /// Return the object name. Applicable only on OpenGL.
    unsigned GetGPUObjectName() const { return object_.name_; }
#endif
    /// Return whether data is lost due to context loss.
    /// @property
    bool IsDataLost() const { return dataLost_; }
    /// Return whether has pending data assigned while graphics context was lost.
    bool HasPendingData() const { return dataPending_; }

protected:
    /// Graphics subsystem.
    WeakPtr<Graphics> graphics_;
    /// Object pointer or name.
#ifdef URHO3D_DILIGENT
    /// Object pointer (Diligent)
    /// Note: Is more safe to use smart pointers from Diligent
    Diligent::RefCntAutoPtr<Diligent::IDeviceObject> object_;
#else
    GPUObjectHandle object_{};
#endif
    /// Data lost flag.
    bool dataLost_{};
    /// Data pending flag.
    bool dataPending_{};
};

}

