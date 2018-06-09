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
    void(*HandleEventWithType)(gchandle gcHandle, unsigned type, VariantMap* args) = nullptr;
    void(*HandleEventWithoutType)(gchandle gcHandle, unsigned type, VariantMap* args) = nullptr;
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
    void RegisterType() { typeInfos_[T::GetTypeStatic()] = T::GetTypeInfoStatic(); }
    /// Returns type information of registered base native type.
    const TypeInfo* GetRegisteredType(StringHash type);
    /// Schedule ReleaseRef() call to be called on the main thread.
    void QueueReleaseRef(RefCounted* instance);
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
    /// Initializes object.
    void Init();
    /// Perform housekeeping tasks.
    void OnEndFrame(StringHash, VariantMap&);

    /// Types of inheritable native classes.
    HashMap<StringHash, const TypeInfo*> typeInfos_;
    /// Queue of objects that should have their ReleaseRef() method called on main thread.
    PODVector<RefCounted*> releaseQueue_;
    /// Mutex protecting resources related to queuing ReleaseRef() calls.
    Mutex mutex_;
};

/// Allocator used for data marshalling between managed and unmanaged worlds. Lifetime of allocated memory is controlled
/// by custom marshallers used in binding code.
class URHO3D_API MarshalAllocator
{
public:
    struct Block
    {
        void* memory_;
        int32_t itemCount_;
        int32_t sizeOfItem_;
        int32_t allocatorIndex_;
    };

    enum AllocationType
    {
        /// If `index` parameter in the header is less than this value then it is index of allocator in `allocators_` array.
        CustomAllocationType = 200,
        /// If `index` parameter in header is this value then memory is to be freed using OS functions instead of allocator.
        HeapAllocator = 255,
    };

    struct AllocatorInfo
    {
        AllocatorBlock* Allocator;
        int BlockSize;

        AllocatorInfo(int size, int capacity);
    };

protected:
    /// Construct
    MarshalAllocator();

public:
    /// Destruct
    ~MarshalAllocator();
    /// Get thread-local instance of this allocator
    static MarshalAllocator& Get();
    /// Allocate memory for array of items.
    template<typename T>
    Block* AllocArray(int count)
    {
        auto* block = AllocInternal(sizeof(T) * count);
        block->sizeOfItem_ = sizeof(T);
        block->itemCount_ = count;
        return block;
    }
    /// Allocate block of memory.
    Block* Alloc(int length)
    {
        return AllocArray<uint8_t>(length);
    }

    /// Free block of memory.
    void Free(Block* memory);

protected:
    /// Allocate block of memory.
    Block* AllocInternal(int length);

    AllocatorInfo allocators_[3];
};

}
