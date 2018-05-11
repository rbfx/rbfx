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

#include "../Precompiled.h"
#include "../Core/Profiler.h"
#include "../Core/StringUtils.h"
#include "../Core/Context.h"

namespace Urho3D
{

const char* noProfilingSupportMssage = "Profiler can not be enabled because engine is built without profiling support.";

Profiler::Profiler(Context* context)
    : Object(context)
{
}

Profiler::~Profiler() = default;

void Profiler::SetEnabled(bool enabled)
{
#if URHO3D_PROFILING
    ::profiler::setEnabled(enabled);
#else
    URHO3D_LOGWARNING(noProfilingSupportMssage);
#endif
}

bool Profiler::GetEnabled() const
{
#if URHO3D_PROFILING
    return ::profiler::isEnabled();
#else
    return false;
#endif
}

void Profiler::StartListen(unsigned short port)
{
#if URHO3D_PROFILING
    ::profiler::startListen(port);
#else
    URHO3D_LOGWARNING(noProfilingSupportMssage);
#endif
}

void Profiler::StopListen()
{
#if URHO3D_PROFILING
    ::profiler::stopListen();
#endif
}

bool Profiler::GetListening() const
{
#if URHO3D_PROFILING
    return ::profiler::isListening();
#else
    return false;
#endif
}

void Profiler::SetEventTracingEnabled(bool enable)
{
#if URHO3D_PROFILING
    ::profiler::setEventTracingEnabled(enable);
#else
    URHO3D_LOGWARNING(noProfilingSupportMssage);
#endif
}

bool Profiler::GetEventTracingEnabled()
{
#if URHO3D_PROFILING
    return ::profiler::isEventTracingEnabled();
#else
    return false;
#endif
}

void Profiler::SetLowPriorityEventTracing(bool isLowPriority)
{
#if URHO3D_PROFILING
    ::profiler::setLowPriorityEventTracing(isLowPriority);
#else
    URHO3D_LOGWARNING(noProfilingSupportMssage);
#endif
}

bool Profiler::GetLowPriorityEventTracing()
{
#if URHO3D_PROFILING
    return ::profiler::isLowPriorityEventTracing();
#else
    return false;
#endif
}

void Profiler::SaveProfilerData(const String& filePath)
{
#if URHO3D_PROFILING
    ::profiler::dumpBlocksToFile(filePath.CString());
#else
    URHO3D_LOGWARNING("Profiler data can not be saved because engine is built without profiling support.");
#endif
}

void Profiler::SetEventProfilingEnabled(bool enabled)
{
    enableEventProfiling_ = enabled;
}

bool Profiler::GetEventProfilingEnabled() const
{
    return enableEventProfiling_;
}

#if URHO3D_PROFILING
thread_local HashMap<unsigned, ::profiler::BaseBlockDescriptor*> blockDescriptorCacheMainThread_;
thread_local HashMap<unsigned, ::profiler::BaseBlockDescriptor*> blockDescriptorCacheThreadLocal_;
#endif

void Profiler::BeginBlock(const char* name, const char* file, int line, unsigned int argb)
{
#if URHO3D_PROFILING
    decltype(blockDescriptorCacheMainThread_)* blockDescriptorCache;
    if (Thread::IsMainThread())
        blockDescriptorCache = &blockDescriptorCacheMainThread_;
    else
        blockDescriptorCache = &blockDescriptorCacheThreadLocal_;

    // Line used as starting hash value for efficiency.
    // This is likely to not play well with hot code reload.
    unsigned hash = StringHash::Calculate(file, (unsigned)line);    // TODO: calculate hash at compile time
    HashMap<unsigned, ::profiler::BaseBlockDescriptor*>::Iterator it = blockDescriptorCache->Find(hash);
    const ::profiler::BaseBlockDescriptor* desc = nullptr;
    if (it == blockDescriptorCache->End())
    {
        String uniqueName = ToString("%s at %s:%d", name, file, line);
        desc = ::profiler::registerDescription(::profiler::EasyBlockStatus::ON, uniqueName.CString(), name, file,
                                               line, ::profiler::BlockType::Block, argb, true);
    }
    else
        desc = it->second_;
    ::profiler::beginNonScopedBlock(desc, name);
#endif
}

void Profiler::EndBlock()
{
#if URHO3D_PROFILING
    ::profiler::endBlock();
#endif
}

void Profiler::RegisterCurrentThread(const char* name)
{
#if URHO3D_PROFILING
    static thread_local const char* profilerThreadName = nullptr;
    if (profilerThreadName == nullptr)
        profilerThreadName = ::profiler::registerThread(name);
#endif
}
ProfilerDescriptor::ProfilerDescriptor(const char* name, const char* file, int line, unsigned int argb)
{
#if URHO3D_PROFILING
    String uniqueName = ToString("%p", this);
    descriptor_ = (void*) ::profiler::registerDescription(::profiler::EasyBlockStatus::ON, uniqueName.CString(),
        name, file, line, ::profiler::BlockType::Block, argb, true);
#endif
}

}
