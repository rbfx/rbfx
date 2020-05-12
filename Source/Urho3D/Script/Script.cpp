//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/Thread.h"
#include "../IO/Log.h"
#include "../Script/Script.h"


namespace Urho3D
{

ScriptRuntimeApi* Script::api_{nullptr};

Script::Script(Context* context)
    : Object(context)
{
}

Script::~Script()
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
