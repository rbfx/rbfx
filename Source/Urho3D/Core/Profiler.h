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

#pragma once

#include "../Container/Str.h"
#include "../Core/Thread.h"
#include "../Core/Timer.h"

#ifdef URHO3D_PROFILING

namespace Urho3D
{

static const int PROFILER_DEFAULT_PORT = 28077;
static const uint32_t PROFILER_COLOR_DEFAULT = 0xffffecb3;
static const uint32_t PROFILER_COLOR_EVENTS = 0xffff9800;
static const uint32_t PROFILER_COLOR_RESOURCES = 0xff00bcd4;

// Copied from easy_profiler
enum ProfilerBlockStatus
{
    OFF = 0,
    ON = 1,
    FORCE_ON = ON | 2,
    OFF_RECURSIVE = 4,
    ON_WITHOUT_CHILDREN = ON | OFF_RECURSIVE,
    FORCE_ON_WITHOUT_CHILDREN = FORCE_ON | OFF_RECURSIVE,
};


/// Hierarchical performance profiler subsystem.
class URHO3D_API Profiler : public Object
{
    URHO3D_OBJECT(Profiler, Object);

public:
    /// Construct.
    explicit Profiler(Context* context);
    /// Destruct.
    virtual ~Profiler();

    /// Enables or disables profiler.
    void SetEnabled(bool enabled);
    /// Returns true if profiler is enabled, false otherwise.
    bool GetEnabled() const;
    /// Enables or disables event profiling.
    void SetEventProfilingEnabled(bool enabled);
    /// Returns true if event profiling is enabled, false otherwise.
    bool GetEventProfilingEnabled() const;
    /// Starts listening for incoming profiler tool connections.
    void StartListen(unsigned short port=PROFILER_DEFAULT_PORT);
    /// Stops listening for incoming profiler tool connections.
    void StopListen();
    /// Returns true if profiler is currently listening for incoming connections.
    bool GetListening() const;
    /// Enables or disables event tracing. This is windows-specific, does nothing on other OS.
    void SetEventTracingEnabled(bool enable);
    /// Returns true if event tracing is enabled, false otherwise.
    bool GetEventTracingEnabled();
    /// Enables or disables low priority event tracing. This is windows-specific, does nothing on other OS.
    void SetLowPriorityEventTracing(bool isLowPriority);
    /// Returns true if low priority event tracing is enabled, false otherwise.
    bool GetLowPriorityEventTracing();
    /// Save profiler data to a file.
    void SaveProfilerData(const String& filePath);
    /// Begin non-scoped profiled block. Block has to be terminated with call to EndBlock(). This is slow and is for
    /// integration with scripting lnaguages. Use URHO3D_PROFILE* macros when writing c++ code instead.
    static void BeginBlock(const char* name, const char* file, int line, unsigned int argb=PROFILER_COLOR_DEFAULT,
                           unsigned char status=ProfilerBlockStatus::ON);
    /// End block started with BeginBlock().
    static void EndBlock();
    /// Register name of current thread. Threads will be labeled in profiler data.
    static void RegisterCurrentThread(const char* name);

private:
    /// Flag which enables event profiling.
    bool enableEventProfiling_ = true;
};

class URHO3D_API ProfilerDescriptor
{
public:
    ProfilerDescriptor(const char* name, const char* file, int line, unsigned int argb=PROFILER_COLOR_DEFAULT,
        unsigned char status=ProfilerBlockStatus::ON);

    void* descriptor_;
};

class URHO3D_API ProfilerBlock
{
public:
    ProfilerBlock(ProfilerDescriptor& descriptor, const char* name);
    ~ProfilerBlock();
};

}

#   define URHO3D_TOKEN_JOIN(x, y) x ## y
#   define URHO3D_TOKEN_CONCATENATE(x, y) URHO3D_TOKEN_JOIN(x, y)
#   define URHO3D_PROFILE(name, ...) static Urho3D::ProfilerDescriptor URHO3D_TOKEN_CONCATENATE(__profiler_desc_, __LINE__) (#name, __FILE__, __LINE__, ##__VA_ARGS__);ProfilerBlock URHO3D_TOKEN_CONCATENATE(__profiler_block_, __LINE__) (URHO3D_TOKEN_CONCATENATE(__profiler_desc_, __LINE__), #name)
#   define URHO3D_PROFILE_SCOPED(name, ...) static Urho3D::ProfilerDescriptor URHO3D_TOKEN_CONCATENATE(__profiler_desc_, __LINE__) (name, __FILE__, __LINE__, ##__VA_ARGS__);ProfilerBlock URHO3D_TOKEN_CONCATENATE(__profiler_block_, __LINE__) (URHO3D_TOKEN_CONCATENATE(__profiler_desc_, __LINE__), name)
#   define URHO3D_PROFILE_NONSCOPED(name, ...) Urho3D::Profiler::BeginBlock(#name, __FILE__, __LINE__, ##__VA_ARGS__)
#   define URHO3D_PROFILE_END() Urho3D::Profiler::EndBlock();
#   define URHO3D_PROFILE_THREAD(name) Urho3D::Profiler::RegisterCurrentThread(#name)
#else
#   define URHO3D_PROFILE(name, ...)
#   define URHO3D_PROFILE_NONSCOPED(name, ...)
#   define URHO3D_PROFILE_SCOPED(name, ...)
#   define URHO3D_PROFILE_END(...)
#   define URHO3D_PROFILE_THREAD(name)
#endif

