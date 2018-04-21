//
// Copyright (c) 2018 Rokas Kupstys.
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

#include <mono/metadata/class.h>
#include <mono/metadata/image.h>
#include <mono/metadata/object.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>

#include "../Core/CoreEvents.h"
#include "../Script/ScriptSubsystem.h"


namespace Urho3D
{

URHO3D_API ScriptSubsystem* scriptSubsystem = nullptr;

ScriptSubsystem::ScriptSubsystem(Context* context)
    : Object(context)
{
    auto* domain = mono_domain_get();
    if (domain == nullptr)
        // This library does not run in context of managed process. Subsystem is noop.
        // TODO: Support for subystem initiating hosting of .net runtime.
        return;

    // This global instance is mainly required for queueing ReleaseRef() calls. Not every RefCounted has pointer to
    // Context therefore if multiple contexts exist they may run on different threads. Then there would be no way to
    // know on which main thread ReleaseRef() should be called. Assert below limits application to having single
    // Context.
    assert(scriptSubsystem == nullptr);
    scriptSubsystem = this;

    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(ScriptSubsystem, OnEndFrame));

    auto* assembly = mono_domain_assembly_open(domain, "Urho3DNet.dll");
    auto* image =  mono_assembly_get_image(assembly);
    auto* klass = mono_class_from_name(image, "Urho3D.CSharp", "NativeInterface");

    FreeGCHandle_ = reinterpret_cast<decltype(FreeGCHandle_)>(mono_method_get_unmanaged_thunk(
        mono_class_get_method_from_name(klass, "FreeGcHandle", 1)));
    CloneGCHandle_ = reinterpret_cast<decltype(CloneGCHandle_)>(mono_method_get_unmanaged_thunk(
        mono_class_get_method_from_name(klass, "CloneGcHandle", 1)));
    CreateObject_ = reinterpret_cast<decltype(CreateObject_)>(mono_method_get_unmanaged_thunk(
        mono_class_get_method_from_name(klass, "CreateObject", 2)));
}

const TypeInfo* ScriptSubsystem::GetRegisteredType(StringHash type)
{
    const TypeInfo* typeInfo = nullptr;
    typeInfos_.TryGetValue(type, typeInfo);
    return typeInfo;
}

void ScriptSubsystem::QueueReleaseRef(RefCounted* instance)
{
    MutexLock lock(mutex_);
    releaseQueue_.Push(instance);
}

void ScriptSubsystem::OnEndFrame(StringHash, VariantMap&)
{
    MutexLock lock(mutex_);
    for (auto* instance : releaseQueue_)
        instance->ReleaseRef();
    releaseQueue_.Clear();
}

void ScriptSubsystem::FreeGCHandle(void* gcHandle)
{
    MonoException* exception = nullptr;
    FreeGCHandle_(gcHandle, (void*)&exception);
}

void* ScriptSubsystem::CloneGCHandle(void* gcHandle)
{
    MonoException* exception = nullptr;
    return CloneGCHandle_(gcHandle, (void*)&exception);
}

Object* ScriptSubsystem::CreateObject(Context* context, unsigned managedType)
{
    MonoException* exception = nullptr;
    return CreateObject_(context, managedType, (void*)&exception);
}

void ScriptSubsystem::RegisterCurrentThread()
{
    auto* domain = mono_domain_get();
    if (domain != nullptr)
        mono_thread_attach(domain);
}

}
