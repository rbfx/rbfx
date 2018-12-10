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

#pragma once


#include <limits>
#include "../Container/HashSet.h"
#include "../Container/Vector.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Engine/PluginApplication.h"


namespace Urho3D
{

/// Script runtime command handler callback type.
typedef std::uintptr_t(*ScriptRuntimeCommandHandler)(int command, void** args);
///
using ScriptCommandRange = Pair<int, int>;
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
    PluginApplication* LoadAssembly(const String& path) { return reinterpret_cast<PluginApplication*>(Command(ScriptRuntimeCommand::LoadAssembly, path.CString())); }
    /// Checks if specified assembly is loadable by script runtime.
    bool VerifyAssembly(const String& path) { return Command(ScriptRuntimeCommand::VerifyAssembly, path.CString()) == ScriptCommandSuccess; }
    ///
    void LoadRuntime() { Command(ScriptRuntimeCommand::LoadRuntime); }
    ///
    void UnloadRuntime() { Command(ScriptRuntimeCommand::UnloadRuntime); }
    /// Script runtime may release references from GC thread. It may be unsafe to run destructors from non-main thread therefore this method queues them to run at the end of next frame on the main thread.
    bool QueueReleaseRef(RefCounted* object);

protected:
    ///
    template<typename... Args>
    std::uintptr_t Command(unsigned command, const Args&... args)
    {
        unsigned index = 0;
        PODVector<void*> runtimeArgs;
        runtimeArgs.Resize(sizeof...(args));
        ConvertArguments(runtimeArgs, index, std::forward<const Args&>(args)...);
        std::uintptr_t result = ScriptCommandFailed;
        for (auto handler : commandHandlers_)
        {
            if (!IsInRange(handler.first_, command))
                continue;
            result = handler.second_(command, runtimeArgs.Buffer());
            break;
        }
        return result;
    }
    ///
    template<typename T, typename... Args>
    void ConvertArguments(PODVector<void*>& runtimeArgs, unsigned index, T arg, Args... args)
    {
        runtimeArgs[index] = ConvertArgument(arg);
        if (sizeof...(args) > 0)
            ConvertArguments(runtimeArgs, ++index, std::forward<Args>(args)...);
    }
    void ConvertArguments(PODVector<void*>& runtimeArgs, unsigned index) { }
    template<typename T>
    void* ConvertArgument(T value) { return const_cast<void*>(reinterpret_cast<const void*>(value)); }
    ///
    bool IsInRange(const ScriptCommandRange& range, unsigned command) const
    {
        return range.first_ <= command && command <= range.second_;
    }
    ///
    Vector<Pair<ScriptCommandRange, ScriptRuntimeCommandHandler>> commandHandlers_;
    ///
    Mutex destructionQueueLock_;
    ///
    PODVector<RefCounted*> destructionQueue_;
};


}
