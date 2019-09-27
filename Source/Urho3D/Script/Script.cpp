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
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
        if (destructionQueue_.empty())
            return;

        URHO3D_PROFILE("ReleaseFinalizedObjects");
        MutexLock lock(destructionQueueLock_);
        if (!destructionQueue_.empty())
        {
            destructionQueue_.back()->ReleaseRef();
            destructionQueue_.pop_back();
        }
    });
}

Script::~Script()
{
    URHO3D_PROFILE("Script::~Script");
    while (!destructionQueue_.empty())
    {
        destructionQueue_.back()->ReleaseRef();
        destructionQueue_.pop_back();
    }
}

void Script::ReleaseRefOnMainThread(RefCounted* object)
{
    if (object == nullptr)
        return;

    if (Thread::IsMainThread())
        object->ReleaseRef();
    else
    {
        MutexLock lock(destructionQueueLock_);
        destructionQueue_.push_back(object);
    }
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
}
