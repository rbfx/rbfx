// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/DeviceObject.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

DeviceObject::DeviceObject(Context* context)
    : renderDevice_(context->GetSubsystem<RenderDevice>())
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

void DeviceObject::RestoreDependency(DeviceObject* dependency)
{
    if (dependency)
        dependency->Restore();
}

} // namespace Urho3D
