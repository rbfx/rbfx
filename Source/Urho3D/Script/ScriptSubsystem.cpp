//
// Copyright (c) 2018 Rokas Kupstys
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

#if URHO3D_WITH_MONO
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/image.h>
#include <mono/metadata/object.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-debug.h>
#include <mono/jit/jit.h>
#endif

#include <cstdlib>

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../IO/Log.h"
#include "../Script/ScriptSubsystem.h"


namespace Urho3D
{

URHO3D_API ScriptSubsystem* scriptSubsystem = nullptr;

ScriptSubsystem::ScriptSubsystem(Context* context)
    : Object(context)
{

    if (managed_.CreateObject == nullptr)
        // This library does not run in context of managed process.
        return;

    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(ScriptSubsystem, OnEndFrame));

    Init();
}

void ScriptSubsystem::Init()
{
    // This global instance is mainly required for queueing ReleaseRef() calls. Not every RefCounted has pointer to
    // Context therefore if multiple contexts exist they may run on different threads. Then there would be no way to
    // know on which main thread ReleaseRef() should be called. Assert below limits application to having single
    // Context.
    if (scriptSubsystem == nullptr)
        scriptSubsystem = this;
    else
        assert(scriptSubsystem == this);
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

void ScriptSubsystem::RegisterCurrentThread()
{
#if URHO3D_WITH_MONO
    auto* domain = mono_domain_get();
    if (domain != nullptr)
        mono_thread_attach(domain);
#endif
}

void* ScriptSubsystem::LoadAssembly(const String& pathToAssembly, void* domain)
{
    // TODO: Implement through managed_ object
    return nullptr;
}

void* ScriptSubsystem::HostManagedRuntime(ScriptSubsystem::RuntimeSettings& settings)
{
#if URHO3D_WITH_MONO
    mono_config_parse(nullptr);
    const auto** options = new const char*[settings.jitOptions_.Size()];
    int i = 0;
    for (const auto& opt : settings.jitOptions_)
    {
        options[i++] = opt.CString();
        if (opt.StartsWith("--debugger-agent"))
            mono_debug_init(MONO_DEBUG_FORMAT_MONO);
    }
    mono_jit_parse_options(settings.jitOptions_.Size(), (char**)options);

    auto* domain = mono_jit_init_version(settings.domainName_.CString(), "v4.0.30319");

    Init();

    return domain;
#else
    return nullptr;
#endif
}

Variant ScriptSubsystem::CallMethod(void* assembly, const String& methodDesc, void* object, const VariantVector& args)
{
    // TODO: Implement through managed_ object
#if 0
    void* monoArgs[20]{};
    auto maxArgs = std::extent<decltype(monoArgs)>::value;
    if (args.Size() > maxArgs)
    {
        URHO3D_LOGERRORF("ScriptSubsystem::CallMethod: unsupported argument count. Max %d arguments allowed.", maxArgs);
        return Variant::EMPTY;
    }

    auto* image = mono_assembly_get_image(static_cast<MonoAssembly*>(assembly));
    if (!image)
    {
        URHO3D_LOGERRORF("ScriptSubsystem::CallMethod failed getting assembly image.");
        return Variant::EMPTY;
    }

    auto* desc = mono_method_desc_new(methodDesc.CString(), true);
    if (desc == nullptr)
    {
        URHO3D_LOGERROR("ScriptSubsystem::CallMethod: invalid method descriptor.");
        return Variant::EMPTY;
    }

    auto* method = mono_method_desc_search_in_image(desc, image);
    if (method == nullptr)
    {
        URHO3D_LOGERROR("ScriptSubsystem::CallMethod: requested method not found.");
        mono_method_desc_free(desc);
        return Variant::EMPTY;
    }

    MonoObject* exception;
    for (auto i = 0; i < args.Size(); i++)
    {
        auto& value = args[i];
        switch (value.GetType())
        {
        case VAR_NONE:
            monoArgs[i] = nullptr;
            break;
        case VAR_VECTOR2:
            monoArgs[i] = (void*)&value.GetVector2();
            break;
        case VAR_VECTOR3:
            monoArgs[i] = (void*)&value.GetVector3();
            break;
        case VAR_VECTOR4:
            monoArgs[i] = (void*)&value.GetVector4();
            break;
        case VAR_QUATERNION:
            monoArgs[i] = (void*)&value.GetQuaternion();
            break;
        case VAR_COLOR:
            monoArgs[i] = (void*)&value.GetColor();
            break;
        case VAR_STRING:
            monoArgs[i] = mono_string_new(mono_domain_get(), value.GetString().CString());
            break;
        case VAR_BUFFER:
            monoArgs[i] = (void*)&value.GetBuffer().Front();
            break;
        case VAR_VOIDPTR:
            monoArgs[i] = value.GetVoidPtr();
            break;
        case VAR_INTRECT:
            monoArgs[i] = (void*)&value.GetIntRect();
            break;
        case VAR_INTVECTOR2:
            monoArgs[i] = (void*)&value.GetIntVector2();
            break;
        case VAR_PTR:
            monoArgs[i] = (void*)value.GetPtr();
            break;
        case VAR_MATRIX3:
            monoArgs[i] = (void*)&value.GetMatrix3();
            break;
        case VAR_MATRIX3X4:
            monoArgs[i] = (void*)&value.GetMatrix3x4();
            break;
        case VAR_MATRIX4:
            monoArgs[i] = (void*)&value.GetMatrix4();
            break;
        case VAR_RECT:
            monoArgs[i] = (void*)&value.GetRect();
            break;
        case VAR_INTVECTOR3:
            monoArgs[i] = (void*)&value.GetIntVector3();
            break;
        case VAR_INT:   // TODO: In order to support litterals they must be stored somewhere in memory and their addresses must be added to monoArgs.
        case VAR_BOOL:
        case VAR_FLOAT:
        case VAR_RESOURCEREF:
        case VAR_RESOURCEREFLIST:
        case VAR_VARIANTVECTOR:
        case VAR_VARIANTMAP:
        case VAR_DOUBLE:
        case VAR_STRINGVECTOR:
        case VAR_INT64:
        case VAR_CUSTOM_HEAP:
        case VAR_CUSTOM_STACK:
        default:
            URHO3D_LOGERRORF("ScriptSubsystem::CallMethod() called with unsupported argument type.");
            assert(false);
            break;
        }
    }

    auto* result = mono_runtime_invoke(method, object, monoArgs, &exception);
    assert(exception == nullptr);

    mono_free_method(method);
    mono_method_desc_free(desc);
    return result;
#else
    return Variant::EMPTY;
#endif
}

ManagedRuntime ScriptSubsystem::managed_{};
NativeRuntime ScriptSubsystem::native_{};

extern "C"
{

URHO3D_API void Urho3D_InitializeCSharp(ManagedRuntime* managed, NativeRuntime* native)
{
    ScriptSubsystem::managed_ = *managed;

    ScriptSubsystem::native_.AllocateMemory = [](unsigned size) { return malloc(size); };
    ScriptSubsystem::native_.FreeMemory = &free;

    *native = ScriptSubsystem::native_;
}

}

}
