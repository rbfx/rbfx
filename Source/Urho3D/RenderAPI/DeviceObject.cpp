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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/DeviceObject.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

RenderDevice* GetRenderDeviceFrom(Context* context)
{
    auto graphics = context->GetSubsystem<Graphics>();
    return graphics ? graphics->GetRenderDevice() : nullptr;
}

} // namespace

DeviceObject::DeviceObject(Context* context)
    : renderDevice_(GetRenderDeviceFrom(context))
    , graphics_(context->GetSubsystem<Graphics>())
{
    if (renderDevice_)
        renderDevice_->AddDeviceObject(this);
}

DeviceObject::~DeviceObject()
{
    if (renderDevice_)
        renderDevice_->RemoveDeviceObject(this);
}

void DeviceObject::ProcessDeviceObjectEvent(DeviceObjectEvent event)
{
    switch (event)
    {
    case DeviceObjectEvent::Invalidate:
    {
        dataLost_ = true;
        Invalidate();
        break;
    }
    case DeviceObjectEvent::Restore:
    {
        Restore();
        break;
    }
    case DeviceObjectEvent::Destroy:
    {
        Destroy();
        renderDevice_ = nullptr;
        break;
    }
    }
}

} // namespace Urho3D
