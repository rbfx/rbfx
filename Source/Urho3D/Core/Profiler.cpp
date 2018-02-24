//
// Copyright (c) 2017 the Urho3D project.
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
#if URHO3D_PROFILING
#   include <easy/profiler.h>

namespace Urho3D
{

Profiler::Profiler(Context* context)
    : Object(context)
{
}

Profiler::~Profiler()
{
}

void Profiler::SetEnabled(bool enabled)
{
    ::profiler::setEnabled(enabled);
}

bool Profiler::GetEnabled() const
{
    return ::profiler::isEnabled();
}

void Profiler::StartListen(unsigned short port)
{
    ::profiler::startListen(port);
}

void Profiler::StopListen()
{
    ::profiler::stopListen();
}

bool Profiler::GetListening() const
{
    return ::profiler::isListening();
}

void Profiler::SetEventTracingEnabled(bool enable)
{
    ::profiler::setEventTracingEnabled(enable);
}

bool Profiler::GetEventTracingEnabled()
{
    return ::profiler::isEventTracingEnabled();
}

void Profiler::SetLowPriorityEventTracing(bool isLowPriority)
{
    ::profiler::setLowPriorityEventTracing(isLowPriority);
}

bool Profiler::GetLowPriorityEventTracing()
{
    return ::profiler::isLowPriorityEventTracing();
}

void Profiler::SaveProfilerData(const String& filePath)
{
    ::profiler::dumpBlocksToFile(filePath.CString());
}

void Profiler::SetEventProfilingEnabled(bool enabled)
{
    enableEventProfiling_ = enabled;
}

bool Profiler::GetEventProfilingEnabled() const
{
    return enableEventProfiling_;
}

HashMap<unsigned, ::profiler::BaseBlockDescriptor*> blockDescriptorCache_;

void Profiler::BeginBlock(const char* name, const char* file, int line, unsigned int argb, ProfilerBlockStatus status)
{
    // Line used as starting hash value for efficiency.
    // This is likely to not play well with hot code reload.
    unsigned hash = StringHash::Calculate(file, (unsigned)line);    // TODO: calculate hash at compile time
    HashMap<unsigned, ::profiler::BaseBlockDescriptor*>::Iterator it = blockDescriptorCache_.Find(hash);
    const ::profiler::BaseBlockDescriptor* desc = 0;
    if (it == blockDescriptorCache_.End())
    {
        String uniqueName = ToString("%s at %s:%d", name, file, line);
        desc = ::profiler::registerDescription((::profiler::EasyBlockStatus)status, uniqueName.CString(), name, file,
                                               line, ::profiler::BLOCK_TYPE_BLOCK, argb, true);
    }
    else
        desc = it->second_;
    ::profiler::beginNonScopedBlock(desc, name);
}

void Profiler::EndBlock()
{
    ::profiler::endBlock();
}

void Profiler::RegisterCurrentThread(const char* name)
{
    static thread_local const char* profilerThreadName = 0;
    if (profilerThreadName == nullptr)
        profilerThreadName = ::profiler::registerThread(name);
}

ProfilerDescriptor::ProfilerDescriptor(const char* name, const char* file, int line, unsigned int argb,
                                       ProfilerBlockStatus status)
{
    String uniqueName = ToString("%p", this);
    descriptor_ = (void*) ::profiler::registerDescription((::profiler::EasyBlockStatus)status, uniqueName.CString(),
        name, file, line, ::profiler::BLOCK_TYPE_BLOCK, argb, true);
}

ProfilerBlock::ProfilerBlock(ProfilerDescriptor& descriptor, const char* name)
{
    ::profiler::beginNonScopedBlock(static_cast<const profiler::BaseBlockDescriptor*>(descriptor.descriptor_), name);
}

ProfilerBlock::~ProfilerBlock()
{
    ::profiler::endBlock();
}

}

#endif
