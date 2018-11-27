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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Thread.h>

#if _WIN32
#  define SWIGSTDCALL __stdcall
#else
#  define SWIGSTDCALL
#endif

extern "C"
{

const Urho3D::TypeInfo* Urho3DGetDirectorTypeInfo(Urho3D::StringHash type);
typedef void* (SWIGSTDCALL* Urho3D_CSharpCreateObjectCallback)(Urho3D::Context* context, unsigned type);
extern Urho3D_CSharpCreateObjectCallback Urho3D_CSharpCreateObject;

}

namespace Urho3D
{

class ManagedObjectFactoryEventSubscriber : public Object
{
    URHO3D_OBJECT(ManagedObjectFactoryEventSubscriber, Object);
public:
    ManagedObjectFactoryEventSubscriber(Context* context) : Object(context) { }
};

class ManagedObjectFactory : public ObjectFactory
{
public:
    ManagedObjectFactory(Context* context, const char* typeName, StringHash baseType)
        : ObjectFactory(context)
        , baseType_(baseType)
        , managedType_(typeName)
        , eventReceiver_(context)
    {
        typeInfo_ = new TypeInfo(typeName, Urho3DGetDirectorTypeInfo(baseType));
        eventReceiver_.SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { OnEndFrame(); });
    }

    ~ManagedObjectFactory() override
    {
        delete typeInfo_;
    }

    SharedPtr<Object> CreateObject() override
    {
        SharedPtr<Object> result((Object*)Urho3D_CSharpCreateObject(context_, managedType_.Value()));
        auto deleter = result->GetDeleter();
        result->SetDeleter([this, deleter](RefCounted* instance)
        {
            if (Thread::IsMainThread())
            {
                // It is safe to delete objects on the main thread
                if (deleter)
                    // Objects may have a custom deleter (set by default factory for example).
                    deleter(instance);
                else
                    delete instance;
            }
            else
            {
                MutexLock lock(mutex_);
                deletionQueue_.Push({instance, deleter});
            }
        });
        return result;
    }

protected:
    /// Delete one queued object per frame.
    void OnEndFrame()
    {
        MutexLock lock(mutex_);
        if (deletionQueue_.Empty())
            return;

        auto& pair = deletionQueue_.Back();
        if (pair.second_)
            pair.second_(pair.first_);
        else
            delete pair.first_;
        deletionQueue_.Pop();
    }

    /// Hash of base type (SWIG director class).
    StringHash baseType_;
    /// Hash of managed type. This is different from `baseType_`.
    StringHash managedType_;
    /// Helper object for receiving E_ENDFRAME event.
    ManagedObjectFactoryEventSubscriber eventReceiver_;
    /// Lock for synchronizing `deletionQueue_` access.
    Mutex mutex_;
    /// LIFO deletion queue for objects that would otherwise get deleted by GC thread.
    Vector<Pair<RefCounted*, std::function<void(RefCounted*)>>> deletionQueue_;
};

extern "C"
{

URHO3D_EXPORT_API void Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType, const char* category)
{
    context->RegisterFactory(new ManagedObjectFactory(context, typeName, StringHash(baseType)), category);
}

}

}

