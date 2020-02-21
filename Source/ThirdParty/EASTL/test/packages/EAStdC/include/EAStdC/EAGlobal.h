///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// OS globals are process-wide globals (singletons, actually) and are shared 
// between an EXE and DLLs, though you can use them on platforms that don't  
// support DLLs. The Windows OS does not directly support this concept of  
// global object sharing, as the only way to accomplish this under Windows  
// is to sequester the global object in its own DLL of which the EXE and all  
// DLLs explicitly reference. OS Globals allow you to have the global object  
// anywhere (including the EXE) and does so without requiring explicit linking.  
// The OS global system works at the operating system level and has  
// auto-discovery logic so that no pointers or init calls need to be made  
// between modules for them to link their OS global systems together.  
// 
// A primary use of OS globals is in the creation of application singletons  
// such as the main heap, messaging servers, asset managers, etc. 
//  
// Note that the implementation behind OS globals (EAOSGlobal.cpp) may seem  
// a bit convoluted; this is because it needs to be thread-safe, cross-module,  
// and independent of application-level memory allocators. For objects for  
// which order of initialization is clearer and you don't need to reference  
// singletons across DLLs, EASingleton is probably a better choice, as it is  
// simpler and has lower overhead. Indeed, another way of looking at OS globals  
// is to view them as process-wide singletons. 
//
// Caveats:
//     - OS globals have the potential for resulting in code that's duplicated
//       within each DLL and thus increasing code memory usage. It might be 
//       useful to use an OS global to store a pointer to an interface rather
//       than an instance of an interface, and have just a single entity within
//       the application provide the implementation. With this approach you still
//       get the benefit of users of the global not having to know ahead of time
//       where the implementation is as you otherwise need to do with standard
//       DLL linking via "dllexport."
//     - OS globals probably aren't needed in cases where you can simply use
//       dllexport (and other platforms' equivalents).
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAGLOBAL_H
#define EASTDC_EAGLOBAL_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/internal/IntrusiveList.h>


EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
#include <new>
EA_RESTORE_ALL_VC_WARNINGS()

#if EASTDC_GLOBALPTR_SUPPORT_ENABLED

namespace EA
{
	namespace StdC
	{
		///////////////////////////////////////////////////////////////////////////
		/// GlobalPtr
		///
		/// GlobalPtr acts as a reference to a pointer which is global throughout 
		/// the process (includes the application and any loaded DLLs). The object
		/// pointed to must define a unique 32-bit kGlobalID if one is not given.
		/// The pointer is set to NULL on creation.
		///
		/// Global pointers may be used from multiple threads once initialized
		/// to point to an object, but are _not_ thread-safe when being set. 
		/// If you have a situation where two threads may attempt to set a global
		/// pointer at the same time, you should use OS globals instead to serialize 
		/// the creators on the OS global lock and prevent race conditions.
		///
		/// A GlobalPtr is not the same thing as simply declaring a pointer at 
		/// a globally accessible scope, especially on platforms with dynamic
		/// libraries such as Windows with its DLLs. A GlobalPtr allows multiple
		/// pieces of code to declare independent pointers to an object, even if
		/// the pieced of code are in independent DLLs.
		///
		/// The pointer assigned to a GlobalPointer need not be a pointer allocated
		/// dynamically on the heap. It can just as well be the address of some 
		/// static or local variable.
		///
		/// Example usage:
		///    GlobalPtr<int, 0x11111111> pInteger;
		///    GlobalPtr<int, 0x11111111> pInteger2;
		///
		///    assert(pInteger == NULL);
		///
		///    pInteger = new int[2];
		///    pInteger[0] = 10;
		///    pInteger[1] = 20;
		///    assert(pInteger2 == pInteger);
		///    assert(pInteger2[0] == pInteger[0]);
		///
		///    delete[] pInteger;
		///    pInteger = NULL;
		///    assert(pInteger2 == NULL);
		///
		template<class T, uint32_t kGlobalId = T::kGlobalId>
		class GlobalPtr
		{
		public:
			/// this_type
			/// This is an alias for this class.
			typedef GlobalPtr<T, kGlobalId> this_type;

			/// GlobalPtr
			///
			/// Default constructor. Sets member pointer to whatever the 
			/// shared version is. If this is the first usage of the shared
			/// version, the pointer will be set to NULL.
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass;
			///
			GlobalPtr() 
				: mppObject(GetStaticPtr())
			{
			}

			/// GlobalPtr (copy constructor)
			///
			/// Default constructor. Sets member pointer to whatever the 
			/// shared version is. If this is the first usage of the shared
			/// version, the pointer will be set to NULL.
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass1;
			///    pSomeClass1 = new pSomeClass;
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass2(pSomeClass1);
			///    pSomeClass2->DoSomething();
			///
			explicit GlobalPtr(const this_type& /*globalPtr*/)
				: mppObject(GetStaticPtr())
			{
			}

			/// operator =
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass1;
			///    pSomeClass1 = new pSomeClass;
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass2(pSomeClass1);
			///    pSomeClass2->DoSomething();
			///
			this_type& operator=(const this_type& /*globalPtr*/) {
				// We need do nothing, as both this and the function argument 
				// are references to the same thing.
				// mppObject = globalPtr.mppObject;  
				return *this;
			}

			/// operator =
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass1;
			///    pSomeClass1 = new pSomeClass;
			///    delete pSomeClass1;
			///    pSomeClass1 = new pSomeClass;
			///
			this_type& operator=(T* p)
			{
				mppObject = p;
				return *this;
			}

			/// operator T*
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass;
			///    FunctionWhichUsesSomeClassPtr(pSomeClass);
			///
			operator T*() const
			{
				return mppObject;
			}

			/// operator T*
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass;
			///    CallFunctionWhichUsesSomeClassPtr(pSomeClass);
			///
			T& operator*() const
			{
				return *mppObject;
			}

			/// operator ->
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass;
			///    pSomeClass->DoSomething();
			///
			T* operator->() const
			{ 
				return  mppObject;
			}

			/// operator !
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass;
			///    if(!pSomeClass)
			///        pSomeClass = new SomeClass;
			///
			bool operator!() const
			{
				return mppObject == NULL;
			}

			/// get
			///
			/// Returns the owned pointer.
			///
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass = new SomeClass;
			///    SomeClass* pSC = pSomeClass.get();
			///    pSC->DoSomething();
			///
			T* get() const
			{
				return mppObject;
			}

			/// Implicit operator bool
			///
			/// Allows for using a scoped_ptr as a boolean.
			/// 
			/// Example usage:
			///    GlobalPtr<SomeClass, 0x12345678> pSomeClass = new SomeClass;
			///    if(pSomeClass)
			///        pSomeClass->DoSomething();
			///
			/// Note that below we do not use operator bool(). The reason for this
			/// is that booleans automatically convert up to short, int, float, etc.
			/// The result is that this: if(scopedPtr == 1) would yield true (bad).
			///
			///////////////////////////////////////////////////////////////////////
			// Disabled as long as operatorT*() exists, as these two are 
			// incompatible. By some arguments, such an operator isn't safe.
			//
			// typedef T* (this_type::*bool_)() const;
			// operator bool_() const
			// {
			//     if(mppObject)
			//         return &this_type::get;
			//     return NULL;
			// }

		protected:
			T*& mppObject;

			static T*& GetStaticPtr();
		};


		//////////////////////////////////////////////////////////////////////////
		/// OSGlobalNode
		///
		/// All OS globals must derive from this object or act as if they do.
		/// If you are using AutoOSGlobalPtr or AutoStaticOSGlobalPtr this 
		/// derivation is handled for you.
		///
		/// Note that above we say 'or act as if they do'. You can make a smart
		/// pointer container that overrides operator ->, operator *, etc. while
		/// referring to an actual object that is somewhere else. In fact, the 
		/// AutoOSGlobalPtr and AutoStaticOSGlobalPtr templates we provide do this.
		///
		struct EASTDC_API OSGlobalNode : public EA::StdC::intrusive_list_node
		{
			uint32_t mOSGlobalID;          // Globally unique id.
			uint32_t mOSGlobalRefCount;    // Reference count. This is modified via atomic operations.
		};


		//////////////////////////////////////////////////////////////////////////
		/// OSGlobalFactoryPtr
		///
		/// Defines a factory function for a given OSGlobalNode.
		/// 
		typedef OSGlobalNode *(*OSGlobalFactoryPtr)();


		//////////////////////////////////////////////////////////////////////////
		/// GetOSGlobal
		///
		/// Browses for a OS global with the given ID and either returns the
		/// existing object or attempts to create and register one. Returns NULL
		/// if the OS global could not be created. This won't happen in practice
		/// because the memory allocator will have already successfully 
		/// initialized the OSGlobal system.
		///
		/// Each successful call to GetOSGlobal must be matched with a call to
		/// ReleaseOSGlobal.
		///
		/// This function doesn't directly allocate any memory. However, the 
		/// user-supplied factory function may allocate memory, depending on 
		/// the implementation of the function.
		///
		/// This function can safely be called from multiple threads. 
		///
		EASTDC_API OSGlobalNode *GetOSGlobal(uint32_t id, OSGlobalFactoryPtr pFactory);

		/// Kettle doesn't have native support for OS Globals, so instead we have to
		/// search physical memory for a fixed free location to share globals from
		/// starting at the very end of virtual memory and working backwards.
		/// This constant defines the size of the search space. Normally this would
		/// be a hidden implementation detail, however it's useful for memory systems
		/// to know where EAStdC might be consuming memory in an otherwise empty heap
		const uint64_t kKettleOSGlobalSearchSpace = 256 * 1024 * 1024;


		//////////////////////////////////////////////////////////////////////////
		/// SetOSGlobal
		///
		/// Adds a user-specified OSGlobal. 
		/// This is useful for setting a specifi instance of an object before
		/// any automatic creation of the object is done.
		///
		EASTDC_API bool SetOSGlobal(uint32_t id, OSGlobalNode *);


		//////////////////////////////////////////////////////////////////////////
		/// ReleaseOSGlobal
		///
		/// Releases a reference to a OS global obtained from GetOSGlobal.
		///
		/// Returns false if the OS global is still in use, and true if the last
		/// reference was just released. The caller is responsible for destroying
		/// the OS global in the latter case.
		///
		/// This function can safely be called from multiple threads. 
		///
		EASTDC_API bool ReleaseOSGlobal(OSGlobalNode *p);




		//////////////////////////////////////////////////////////////////////////
		/// AutoOSGlobalPtr
		///
		/// Holds a reference to an OS global of the specified type and Id. 
		/// If the OS global does not exist, a new one is created in the 
		/// shared heap. The Id parameter is an arbitrary guid and allows the
		/// user to have multiple OSGlobalPtrs of the same stored type T.
		///
		/// AutoStaticOSGlobalPtrs and AutoOSGlobalPtrs should not be mixed when
		/// referring to a OS global. You should reference an OSGlobal via either
		/// one or more AutoOSGlobalPtrs, one or more AutoStaticOSGlobalPtrs, but
		/// not both at the same time.
		///
		/// OS global lookup is not very fast so the preferred usage of this
		/// class is to wrap it in an accessor. This also ensures that the
		/// OS global stays around while created.
		///
		/// This class can safely be used from multiple threads. 
		///
		/// Example usage:
		///     AutoOSGlobalPtr<Widget, 0x11111111> gpWidget1A;
		///     AutoOSGlobalPtr<Widget, 0x11111111> gpWidget1B;
		///     AutoOSGlobalPtr<Widget, 0x22222222> gpWidget2;
		///     
		///     void Foo() {
		///         assert(gpWidget1A == gpWidget1B);
		///         assert(gpWidget1A != gpWidget2);
		///
		///         gpWidget1A->DoSomething();
		///         gpWidget2->DoSomething();
		///     }
		///
		/// Example of how to write an accessor wrapper:
		///     Foo *GetFoo() {
		///         static AutoOSGlobalPtr<Foo, kFooId> sPtr;
		///         return sPtr;
		///     }
		///
		///     Better yet, double-wrap it so clients don't have 
		///     to #include this header:
		///
		///     Foo *InlineGetFoo() {
		///        extern void GetFoo();
		///        static Foo * const sPtr = GetFoo();
		///        return sPtr;
		///     }
		///
		template<class T, uint32_t id>
		class AutoOSGlobalPtr
		{
		public:
			/// value_type
			/// This is an alias for the contained class T.
			typedef T value_type;

			/// this_type
			/// This is an alias for this class.
			typedef AutoOSGlobalPtr<T, id> this_type;

		public:
			/// AutoOSGlobalPtr
			///
			/// Default constructor. Creates a new version of the OSGlobalPtr
			/// object if it hasn't been created yet and sets its reference
			/// count to one.
			///
			AutoOSGlobalPtr();

			/// AutoOSGlobalPtr (copy constructor)
			///
			/// Copy constructor. Copies the OSGlobalPtr from the source.
			/// Increases the reference count on the copied OSGlobalPtr.
			///
			AutoOSGlobalPtr(const this_type& x);

			/// AutoOSGlobalPtr
			///
			/// Decrements the reference count on the held OSGlobalPtr if
			/// it has been set. Destroys the OSGlobalPtr node if the reference
			/// count goes to zero.
			///
		   ~AutoOSGlobalPtr();

			/// operator =
			///
			/// Assignment operator. Copies the OSGlobalPtr from the source.
			/// Increases the reference count on the copied OSGlobalPtr.
			/// Decreases the reference count on the previous OSGlobalPtr.
			///
			this_type& operator=(const this_type& x);

			/// operator T*
			///
			/// Allows this class to act like a pointer.
			///
			operator T*() const;

			/// operator *
			///
			/// Allows this class to act like a pointer.
			///
			T& operator*() const;

			/// operator ->
			///
			/// Allows this class to act like a pointer.
			///
			T *operator->() const;

			/// get
			///
			/// Returns the owned pointer.
			///
			T *get() const;

		protected:
			struct Node : public OSGlobalNode {
				T mObject;
				Node() : mObject() {}
			};

			enum NodeAlignment {
				kNodeAlignment = EA_ALIGN_OF(Node)
			};

			static OSGlobalNode *Create();

			Node * const mpOSGlobal;
		};




		namespace AutoOSGlobalInternal
		{
			/// AlignedBuffer
			///
			/// The AlignedBuffer template is required due to a weakness of VC++ __declspec(align). 
			/// In particular, VC++ is only able to use __declspec(align) via an integral constant. 
			/// Thus, you cannot do this: __declspec(align(__alignof(T))) but can only do things like 
			/// this __declspec(align(4)). This has the unfortunate result of making it impossible to 
			/// declare an object X as having the same alignment as object Y. Yet that's what we need
			/// to do. It turns out that the template constructs below allow for us to work around 
			/// the VC++ limitation, albeit via some template trickery.
			///
			template <size_t size, size_t alignment>  struct AlignedBuffer { typedef char Type[size]; };

			#if defined(EA_COMPILER_MSVC) && EA_COMPILER_VERSION >= 1900	// VS2015+
				EA_DISABLE_VC_WARNING(5029);  // nonstandard extension used: alignment attributes in C++ apply to variables, data members and tag types only
			#endif
			template<size_t size> struct AlignedBuffer<size, 2>    { typedef EA_PREFIX_ALIGN(2)    char Type[size] EA_POSTFIX_ALIGN(2);    };
			template<size_t size> struct AlignedBuffer<size, 4>    { typedef EA_PREFIX_ALIGN(4)    char Type[size] EA_POSTFIX_ALIGN(4);    };
			template<size_t size> struct AlignedBuffer<size, 8>    { typedef EA_PREFIX_ALIGN(8)    char Type[size] EA_POSTFIX_ALIGN(8);    };
			template<size_t size> struct AlignedBuffer<size, 16>   { typedef EA_PREFIX_ALIGN(16)   char Type[size] EA_POSTFIX_ALIGN(16);   };
			template<size_t size> struct AlignedBuffer<size, 32>   { typedef EA_PREFIX_ALIGN(32)   char Type[size] EA_POSTFIX_ALIGN(32);   };
			template<size_t size> struct AlignedBuffer<size, 64>   { typedef EA_PREFIX_ALIGN(64)   char Type[size] EA_POSTFIX_ALIGN(64);   };
			template<size_t size> struct AlignedBuffer<size, 128>  { typedef EA_PREFIX_ALIGN(128)  char Type[size] EA_POSTFIX_ALIGN(128);  };
			template<size_t size> struct AlignedBuffer<size, 256>  { typedef EA_PREFIX_ALIGN(256)  char Type[size] EA_POSTFIX_ALIGN(256);  };
			template<size_t size> struct AlignedBuffer<size, 512>  { typedef EA_PREFIX_ALIGN(512)  char Type[size] EA_POSTFIX_ALIGN(512);  };
			template<size_t size> struct AlignedBuffer<size, 1024> { typedef EA_PREFIX_ALIGN(1024) char Type[size] EA_POSTFIX_ALIGN(1024); };
			template<size_t size> struct AlignedBuffer<size, 2048> { typedef EA_PREFIX_ALIGN(2048) char Type[size] EA_POSTFIX_ALIGN(2048); };
			template<size_t size> struct AlignedBuffer<size, 4096> { typedef EA_PREFIX_ALIGN(4096) char Type[size] EA_POSTFIX_ALIGN(4096); };
			#if defined(EA_COMPILER_MSVC) && EA_COMPILER_VERSION >= 1900	// VS2015+
				EA_RESTORE_VC_WARNING();
			#endif
		}



		///////////////////////////////////////////////////////////////////////////
		/// AutoStaticOSGlobalPtr
		///
		/// Holds a reference to a OS global of the specified type and ID. 
		/// If the OS global does not exist, a new one is created using static memory.
		/// The Id parameter is an arbitrary guid and allows the user to have 
		/// multiple OSGlobalPtrs of the same stored type T.
		///
		/// AutoStaticOSGlobalPtrs and AutoOSGlobalPtrs should not be mixed when
		/// referring to a OS global. You should reference an OSGlobal via either
		/// one or more AutoOSGlobalPtrs, one or more AutoStaticOSGlobalPtrs, but
		/// not both at the same time.
		///
		/// The advantage of this class is that it uses static memory, so it does
		/// not contribute to heap usage and always succeeds in allocating the object. 
		/// The disadvantage is that if multiple DLLs are involved each will have 
		/// its own static space reserved for the OS global.
		///
		/// This class can safely be used from multiple threads. 
		///
		/// Example usage:
		///     AutoStaticOSGlobalPtr<Widget, 0x11111111> gpWidget1A;
		///     AutoStaticOSGlobalPtr<Widget, 0x11111111> gpWidget1B;
		///     AutoStaticOSGlobalPtr<Widget, 0x22222222> gpWidget2;
		///     
		///     void Foo() {
		///         assert(gpWidget1A == gpWidget1B);
		///         assert(gpWidget1A != gpWidget2);
		///
		///         gpWidget1A->DoSomething();
		///         gpWidget2->DoSomething();
		///     }
		///
		template<class T, uint32_t id>
		class AutoStaticOSGlobalPtr
		{
		public:
			/// value_type
			/// This is an alias for the contained class T.
			typedef T value_type;

			/// this_type
			/// This is an alias for this class.
			typedef AutoStaticOSGlobalPtr<T, id> this_type;

		public:
			/// AutoStaticOSGlobalPtr
			///
			/// Default constructor. Creates a new version of the OSGlobalPtr
			/// object if it hasn't been created yet and sets its reference
			/// count to one.
			///
			AutoStaticOSGlobalPtr();

			/// AutoStaticOSGlobalPtr (copy constructor)
			///
			/// Copy constructor. Copies the OSGlobalPtr from the source.
			/// Increases the reference count on the copied OSGlobalPtr.
			///
			AutoStaticOSGlobalPtr(const this_type& x);

			/// ~AutoStaticOSGlobalPtr
			///
			/// Decrements the reference count on the held OSGlobalPtr if
			/// it has been set. Destroys the OSGlobalPtr node if the reference
			/// count goes to zero.
			///
		   ~AutoStaticOSGlobalPtr();

			/// operator =
			///
			/// Assignment operator. Copies the OSGlobalPtr from the source.
			/// Increases the reference count on the copied OSGlobalPtr.
			/// Decreases the reference count on the previous OSGlobalPtr.
			///
			this_type& operator=(const this_type& x);

			/// operator T*
			///
			/// Allows this class to act like a pointer.
			///
			operator T*() const;

			/// operator *
			///
			/// Allows this class to act like a pointer.
			///
			T& operator*() const;

			/// operator ->
			///
			/// Allows this class to act like a pointer.
			///
			T* operator->() const;

			/// get
			///
			/// Returns the owned pointer.
			///
			T* get() const;

		protected:
			static OSGlobalNode* Create();

			struct Node : public OSGlobalNode {
				T mObject;
				Node() : mObject() {}
			};

			static const size_t kNodeAlignment   = EA_ALIGN_OF(Node);
			static const int    kStorageTypeSize = sizeof(Node) + (kNodeAlignment - 1); // Add sufficient additional storage space so that we can align the base pointer to the correct boundary in the worst case.

			Node * const mpOSGlobal;
			static char sStorage[kStorageTypeSize];
		};

	} // namespace StdC

} // namespace EA






///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
	namespace StdC
	{

		template<class T, uint32_t kGlobalId>
		T*& GlobalPtr<T, kGlobalId>::GetStaticPtr()
		{
			// For now, we use the OS global system to hold the pointer. 
			// This may change in the future. The pointer must be static 
			// because we need something to hold onto the OS global.
			// Note: Using static here may be problematic with GCC,  
			// because it doesn't do static variable shutdown properly.
			static EA::StdC::AutoOSGlobalPtr<T*, kGlobalId> sSharedPtr;

			return *sSharedPtr;
		}


		///////////////////////////////////////////////////////////////////////////
		// AutoOSGlobalPtr
		///////////////////////////////////////////////////////////////////////////

		template <class T, uint32_t id>
		inline AutoOSGlobalPtr<T, id>::AutoOSGlobalPtr()
			: mpOSGlobal(static_cast<Node *>(GetOSGlobal(id, Create)))
		{
		}

		template <class T, uint32_t id>
		inline AutoOSGlobalPtr<T, id>::AutoOSGlobalPtr(const this_type&)
			: mpOSGlobal(static_cast<Node *>(GetOSGlobal(id, Create)))
		{
		}

		template <class T, uint32_t id>
		inline AutoOSGlobalPtr<T, id>::~AutoOSGlobalPtr() {
			if (ReleaseOSGlobal(mpOSGlobal))
				delete mpOSGlobal; // Matches the new from Create.
		}

		template <class T, uint32_t id>
		inline typename AutoOSGlobalPtr<T, id>::this_type& 
		AutoOSGlobalPtr<T, id>::operator=(const this_type&) { 
			// Intentionally do nothing. We should already point to the correct object.
			return *this;
		}

		template <class T, uint32_t id>
		inline AutoOSGlobalPtr<T, id>::operator T*() const {
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T * AutoOSGlobalPtr<T, id>::operator->() const {
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T& AutoOSGlobalPtr<T, id>::operator*()  const {
			return mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T * AutoOSGlobalPtr<T, id>::get() const {
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline OSGlobalNode * AutoOSGlobalPtr<T, id>::Create() {
			return EASTDC_NEW_ALIGNED(kNodeAlignment, (size_t)0, EASTDC_ALLOC_PREFIX "OSGlobal") Node;
		}




		///////////////////////////////////////////////////////////////////////////
		// AutoStaticOSGlobalPtr
		///////////////////////////////////////////////////////////////////////////

		template <class T, uint32_t id>
		inline AutoStaticOSGlobalPtr<T, id>::AutoStaticOSGlobalPtr()
			: mpOSGlobal(static_cast<Node *>(GetOSGlobal(id, Create)))
		{
		}

		template <class T, uint32_t id>
		inline AutoStaticOSGlobalPtr<T, id>::AutoStaticOSGlobalPtr(const this_type& /*x*/)
			: mpOSGlobal(static_cast<Node *>(GetOSGlobal(id, Create)))
		{
		}

		template <class T, uint32_t id>
		inline AutoStaticOSGlobalPtr<T, id>::~AutoStaticOSGlobalPtr() {
			if (ReleaseOSGlobal(mpOSGlobal))
				mpOSGlobal->~Node();
		}

		template <class T, uint32_t id>
		inline typename AutoStaticOSGlobalPtr<T, id>::this_type& 
		AutoStaticOSGlobalPtr<T, id>::operator=(const this_type&) { 
			// Intentionally do nothing. We should already point to the correct object.
			return *this;
		}

		template <class T, uint32_t id>
		inline AutoStaticOSGlobalPtr<T, id>::operator T*() const { 
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T * AutoStaticOSGlobalPtr<T, id>::operator->() const {
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T& AutoStaticOSGlobalPtr<T, id>::operator*() const {
			return mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline T * AutoStaticOSGlobalPtr<T, id>::get() const
		{
			return &mpOSGlobal->mObject;
		}

		template <class T, uint32_t id>
		inline OSGlobalNode * AutoStaticOSGlobalPtr<T, id>::Create() {
			char* pStorageAligned = (char*)(((uintptr_t)&sStorage[0] + (kNodeAlignment - 1)) & ~(kNodeAlignment - 1));
			return ::new(pStorageAligned) Node;
		}

		// AutoStaticOSGlobalPtr::sStorage declaration
		template<class T, uint32_t id>
		char AutoStaticOSGlobalPtr<T, id>::sStorage[kStorageTypeSize];

	} // namespace StdC

} // namespace EA

#endif // EASTDC_GLOBALPTR_SUPPORT_ENABLED

#endif // Header include guard






















