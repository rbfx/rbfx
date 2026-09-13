// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Script/Script.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/IO/Log.h"

namespace Urho3D
{

ScriptRuntimeApi* Script::api_{};

Script::Script(Context* context)
    : Object(context)
{
}

Script::~Script()
{
}

ScriptRuntimeApi::~ScriptRuntimeApi()
{
}

void ScriptRuntimeApi::DereferenceAndDispose(RefCounted* instance)
{
    if (instance == nullptr || !instance->HasScriptObject())
        return;

    if (instance->Refs() > 2)
    {
        URHO3D_LOGERROR("Disposing of object with multiple native references is not allowed. It leads to crashes.");
        assert(false);
        return;
    }
    Dispose(instance);
}

GCHandleRef::GCHandleRef(void* handle) noexcept
    : handle_(handle)
{
}

GCHandleRef::~GCHandleRef()
{
    if (handle_ != nullptr)
    {
        Script::GetRuntimeApi()->FreeGCHandle(handle_);
        handle_ = nullptr;
    }
}

GCHandleRef::GCHandleRef(const GCHandleRef& other)
{
    operator=(other);
}

GCHandleRef::GCHandleRef(GCHandleRef&& other) noexcept
{
    ea::swap(handle_, other.handle_);
}

GCHandleRef& GCHandleRef::operator=(void* handle)
{
    if (handle_ != nullptr)
        Script::GetRuntimeApi()->FreeGCHandle(handle_);
    handle_ = handle;
    return *this;
}

GCHandleRef& GCHandleRef::operator=(const GCHandleRef& rhs)
{
    handle_ = Script::GetRuntimeApi()->CloneGCHandle(rhs.handle_);
    return *this;
}

GCHandleRef& GCHandleRef::operator=(GCHandleRef&& rhs) noexcept
{
    ea::swap(handle_, rhs.handle_);
    return *this;
}

}
