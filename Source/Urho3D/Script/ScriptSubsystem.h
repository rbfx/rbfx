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


#include "../Core/Mutex.h"
#include "../Core/Object.h"

namespace Urho3D
{

using gchandle = void*;

// When updating runtime structures do not forget to perform same changes in NativeInterface.cs

struct ManagedRuntime
{
    gchandle(*Lock)(void* managedObject, bool pin) = nullptr;
    void(*Unlock)(gchandle handle) = nullptr;
    gchandle(*CloneHandle)(gchandle handle) = nullptr;
    Object*(*CreateObject)(Context* context, unsigned managedType) = nullptr;
    void(*HandleEvent)(gchandle gcHandle, unsigned type, VariantMap* args) = nullptr;
    gchandle(*InvokeMethod)(gchandle callback, int paramCount, void* parameters[]) = nullptr;
};

struct NativeRuntime
{
    void*(*AllocateMemory)(unsigned size) = nullptr;
    void(*FreeMemory)(void* memory) = nullptr;
    void*(*InteropAlloc)(int) = nullptr;
    void(*InteropFree)(void*) = nullptr;
};

class URHO3D_API ScriptSubsystem : public Object
{
    URHO3D_OBJECT(ScriptSubsystem, Object);

public:
    struct RuntimeSettings
    {
        /// Name of new jit domain.
        String domainName_ = "DefaultDomain";
        /// Path to executable which will be executed. Must have static main method. Can be empty.
        String defaultExecutable_;
        /// Jit options.
        Vector<String> jitOptions_ = {
            "--debugger-agent=transport=dt_socket,address=127.0.0.1:53631,server=y,suspend=n",
            "--optimize=float32"
        };
    };

    explicit ScriptSubsystem(Context* context);

    /// Register types of inheritable native objects. They are used in factory which creates managed subclasses of these objects.
    template<typename T>
    static void RegisterType() { typeInfos_[T::GetTypeStatic()] = T::GetTypeInfoStatic(); }
    /// Returns type information of registered base native type.
    const TypeInfo* GetRegisteredType(StringHash type);
    /// Schedule ReleaseRef() call to be called on the main thread.
    static void QueueReleaseRef(RefCounted* instance);
    /// Registers current thread with .net runtime.
    void RegisterCurrentThread();

    /// Creates a new managed domain and optionally executes an assembly.
    void* HostManagedRuntime(RuntimeSettings& settings);
    /// Load managed assembly and return it.
    void* LoadAssembly(const String& pathToAssembly, void* domain=nullptr);
    /// Call a managed method and return result.
    Variant CallMethod(void* assembly, const String& methodDesc, void* object = nullptr,
        const VariantVector& args = Variant::emptyVariantVector);

    static ManagedRuntime managed_;
    static NativeRuntime native_;

protected:
    /// Perform housekeeping tasks.
    void OnEndFrame(StringHash, VariantMap&);

    /// Queue of objects that should have their ReleaseRef() method called on main thread.
    static PODVector<RefCounted*> releaseQueue_;
    /// Mutex protecting resources related to queuing ReleaseRef() calls.
    static Mutex mutex_;
    /// Types of inheritable native classes.
    static HashMap<StringHash, const TypeInfo*> typeInfos_;
};

/// LIFO allocator used for marshalling data between managed and native worlds.
class URHO3D_API MarshalAllocator
{
public:
    MarshalAllocator() : memory_(0x100000) { }
    /// Get thread-local instance of this allocator
    static MarshalAllocator& Get();
    /// Allocate a block of memory. Last allocated block must be freed first.
    void* Alloc(unsigned length);
    /// Free previously allocated block.
    void Free(void* ptr);
    /// Return number of bytes remaining in scratch buffer.
    unsigned Remaining() const { return memory_.Size() - static_cast<unsigned>(used_); }
    /// Return length of allocated memory block.
    static inline unsigned GetMemoryLength(void* memory) { return *((uint8_t*)memory - sizeof(unsigned)) & ALLOCATOR_MALLOC; }

private:
    enum
    {
        /// Tag indicating that allocator allocated memory using malloc.
        ALLOCATOR_MALLOC = 1 << 31
    };

    /// Memory used by allocator.
    PODVector<uint8_t> memory_;
    /// Current used memory size.
    int used_ = 0;
    /// New size of memory block which will be applied next time all items in allocator are freed.
    unsigned nextSize_ = 0;

};

struct URHO3D_API GcHandleContainer
{
    GcHandleContainer(gchandle handle)
        : handle_(handle)
    {
    }

    GcHandleContainer(const GcHandleContainer& rhs)
        : handle_(nullptr)
    {
        if (rhs.handle_ != nullptr)
            handle_ = ScriptSubsystem::managed_.CloneHandle(rhs.handle_);
    }

    GcHandleContainer(GcHandleContainer&& rhs)
    {
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
    }

    ~GcHandleContainer()
    {
        ReleaseHandle();
    }

    GcHandleContainer& operator =(const GcHandleContainer& rhs)
    {
        ReleaseHandle();
        if (rhs.handle_ != nullptr)
            handle_ = ScriptSubsystem::managed_.CloneHandle(rhs.handle_);
        return *this;
    }

    void ReleaseHandle()
    {
        if (handle_ != nullptr)
        {
            ScriptSubsystem::managed_.Unlock(handle_);
            handle_ = nullptr;
        }
    }

    gchandle handle_;
};

}
