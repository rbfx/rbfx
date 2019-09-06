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

#include "../Core/Macros.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Engine/PluginApplication.h"


namespace Urho3D
{

/// Script API implemented in target scripting language.
class URHO3D_API ScriptRuntimeApi
{
public:
    /// Returns true if path contains a valid managed assembly with a class that inherits from PluginApplication.
    virtual bool VerifyAssembly(const ea::string& path) = 0;
    /// Modifies specified assembly by setting it's version to specified one.
    virtual bool SetAssemblyVersion(const ea::string& path, unsigned version) = 0;
    /// Loads specified managed assembly and returns it's gc handle.
    virtual int LoadAssembly(const ea::string& path) = 0;
    /// Looks for class inheriting from PluginApplication and creates an instance of it.
    virtual PluginApplication* CreatePluginApplication(int assembly) = 0;
    /// Invokes managed instance.Dispose() method.
    virtual void Dispose(RefCounted* instance) = 0;
    /// Release specified gc handle. It becomes invalid.
    virtual void FreeGCHandle(int handle) = 0;
    /// Creates a new gc handle pointing to same object as specified gc handle. Specified gc handle will be freed.
    virtual int RecreateGCHandle(int handle, bool strong) = 0;
    /// Warning! This is slow! Perform a full garbage collection.
    virtual void FullGC() = 0;
};

/// Script runtime subsystem.
class URHO3D_API Script : public Object
{
    URHO3D_OBJECT(Script, Object);
public:
    /// Construct.
    explicit Script(Context* context);
    /// Destruct.
    ~Script() override;
    /// Script runtime may release references from GC thread. It may be unsafe to run destructors from non-main thread
    /// therefore this method queues them to run at the end of next frame on the main thread.
    void ReleaseRefOnMainThread(RefCounted* object);
    /// Returns script runtime api implemented in managed code.
    static ScriptRuntimeApi* GetRuntimeApi() { return api_; }
    /// Should be called from managed code and provide implementation of ScriptRuntimeApi.
    static void SetRuntimeApi(ScriptRuntimeApi* impl) { api_ = impl; }

protected:
    /// This lock protects access to `destructionQueue_`.
    Mutex destructionQueueLock_;
    /// A list of objects whose finalizers executed before .Dispose() was called. ReleaseRef() will be called on those
    /// objects in a main thread. One call per frame. Last inserted object will be released first.
    ea::vector<RefCounted*> destructionQueue_;
    /// API implemented in scripting environment.
    static ScriptRuntimeApi* api_;
};


}
