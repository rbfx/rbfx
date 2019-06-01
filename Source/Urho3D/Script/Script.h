//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <limits>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Engine/PluginApplication.h"


namespace Urho3D
{
#if _WIN32
#  define URHO3D_STDCALL __stdcall
#else
#  define URHO3D_STDCALL
#endif

/// Script runtime command handler callback type.
typedef std::uintptr_t(URHO3D_STDCALL*ScriptRuntimeCommandHandler)(int command, void** args);
///
using ScriptCommandRange = ea::pair<int, int>;
/// Value indicating failure.
static const std::uintptr_t ScriptCommandFailed = ~0U;
/// Value indicating success.
static const std::uintptr_t ScriptCommandSuccess = 0;

/// List of runtime commands. Warning! When reordering enum items inspect all calls to RegisterCommandHandler!
enum ScriptRuntimeCommand: int
{
    LoadRuntime,
    UnloadRuntime,
    LoadAssembly,
    VerifyAssembly,
};

class URHO3D_API Script : public Object
{
    URHO3D_OBJECT(Script, Object);
public:
    explicit Script(Context* context);
    /// Do not use.
    void RegisterCommandHandler(int first, int last, void* handler);
    /// Loads specified assembly into script runtime.
    PluginApplication* LoadAssembly(const ea::string& path) { return reinterpret_cast<PluginApplication*>(Command(ScriptRuntimeCommand::LoadAssembly, path.c_str())); }
    /// Checks if specified assembly is loadable by script runtime.
    bool VerifyAssembly(const ea::string& path) { return Command(ScriptRuntimeCommand::VerifyAssembly, path.c_str()) == ScriptCommandSuccess; }
    ///
    void LoadRuntime() { Command(ScriptRuntimeCommand::LoadRuntime); }
    ///
    void UnloadRuntime() { Command(ScriptRuntimeCommand::UnloadRuntime); }
    /// Script runtime may release references from GC thread. It may be unsafe to run destructors from non-main thread therefore this method queues them to run at the end of next frame on the main thread.
    bool ReleaseRefOnMainThread(RefCounted* object);

protected:
    ///
    template<typename... Args>
    std::uintptr_t Command(unsigned command, const Args&... args)
    {
        unsigned index = 0;
        ea::vector<void*> runtimeArgs;
        runtimeArgs.resize(sizeof...(args));
        ConvertArguments(runtimeArgs, index, std::forward<const Args&>(args)...);
        std::uintptr_t result = ScriptCommandFailed;
        for (auto handler : commandHandlers_)
        {
            if (!IsInRange(handler.first, command))
                continue;
            result = handler.second(command, runtimeArgs.data());
            break;
        }
        return result;
    }
    ///
    template<typename T, typename... Args>
    void ConvertArguments(ea::vector<void*>& runtimeArgs, unsigned index, T arg, Args... args)
    {
        runtimeArgs[index] = ConvertArgument(arg);
        if (sizeof...(args) > 0)
            ConvertArguments(runtimeArgs, ++index, std::forward<Args>(args)...);
    }
    void ConvertArguments(ea::vector<void*>& runtimeArgs, unsigned index) { }
    template<typename T>
    void* ConvertArgument(T value) { return const_cast<void*>(reinterpret_cast<const void*>(value)); }
    ///
    bool IsInRange(const ScriptCommandRange& range, unsigned command) const
    {
        return range.first <= command && command <= range.second;
    }
    ///
    ea::vector<ea::pair<ScriptCommandRange, ScriptRuntimeCommandHandler>> commandHandlers_;
    ///
    Mutex destructionQueueLock_;
    ///
    ea::vector<RefCounted*> destructionQueue_;
};


}
