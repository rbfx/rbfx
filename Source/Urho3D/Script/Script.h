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

/// Script API implemented in target scripting language.
class URHO3D_API ScriptRuntimeApi : public Object
{
    URHO3D_OBJECT(ScriptRuntimeApi, Object);
public:
    explicit ScriptRuntimeApi(Context* context) : Object(context) { }
    ~ScriptRuntimeApi(){
        int a = 2;
    }

    ///
    virtual bool LoadRuntime() = 0;
    ///
    virtual bool UnloadRuntime() = 0;
    ///
    virtual bool VerifyAssembly(const ea::string& path) = 0;
    ///
    virtual PluginApplication* LoadAssembly(const ea::string& path) = 0;
};

/// Script runtime subsystem.
class URHO3D_API Script : public Object
{
    URHO3D_OBJECT(Script, Object);
public:
    explicit Script(Context* context);
    /// Script runtime may release references from GC thread. It may be unsafe to run destructors from non-main thread therefore this method queues them to run at the end of next frame on the main thread.
    bool ReleaseRefOnMainThread(RefCounted* object);
    ///
    ScriptRuntimeApi* GetRuntimeApi() const { return api_.Get(); }
    ///
    void SetRuntimeApi(ScriptRuntimeApi* impl) { api_ = impl; }

protected:
    ///
    Mutex destructionQueueLock_;
    ///
    ea::vector<RefCounted*> destructionQueue_;
    ///
    SharedPtr<ScriptRuntimeApi> api_;
};


}
