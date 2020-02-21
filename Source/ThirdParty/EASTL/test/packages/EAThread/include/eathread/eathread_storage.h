///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Defines thread-local storage and related concepts in a platform-independent
// and thread-safe manner.
//
// As of this writing (10/2003), documentation concerning thread-local 
// storage implementations under GCC, pthreads, and MSVC/Windows can be found at:
//    http://gcc.gnu.org/onlinedocs/gcc-3.3.2/gcc/Thread-Local.html#Thread-Local
//    http://java.icmc.sc.usp.br/library/books/ibm_pthreads/users-33.htm#324811
//    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vccore/html/_core_Thread_Local_Storage_.28.TLS.29.asp
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_STORAGE_H
#define EATHREAD_EATHREAD_STORAGE_H


#include <eathread/internal/config.h>

EA_DISABLE_VC_WARNING(4574)
#include <stddef.h>
EA_RESTORE_VC_WARNING()

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace EA
{
	namespace Thread
	{
		/////////////////////////////////////////////////////////////////////////
		/// EA_THREAD_LOCAL
		/// 
		/// Documentation (partially culled from online information):
		/// Thread Local Storage (a.k.a. TLS and Thread Specific Storage) is a 
		/// mechanism by which each thread in a multithreaded process allocates 
		/// storage for thread-specific data. In standard multithreaded programs, 
		/// data is shared among all threads of a given process, whereas thread 
		/// local storage is the mechanism for allocating per-thread data.
		///
		/// The EA_THREAD_LOCAL specifier may be used alone, with the extern or 
		/// static specifiers, but with no other storage class specifier. 
		/// When used with extern or static, EA_THREAD_LOCAL must appear 
		/// immediately after the other storage class specifier.
		///
		/// The EA_THREAD_LOCAL specifier may be applied to any global, file-scoped 
		/// static, function-scoped static, or static data member of a class. 
		/// It may not be applied to block-scoped automatic or non-static data member.
		///
		/// When the address-of operator is applied to a thread-local variable, 
		/// it is evaluated at run-time and returns the address of the current 
		/// thread's instance of that variable. An address so obtained may be used 
		/// by any thread. When a thread terminates, any pointers to thread-local
		/// variables in that thread become invalid.
		///
		/// No static initialization may refer to the address of a thread-local variable.
		/// In C++, if an initializer is present for a thread-local variable, 
		/// it must be a constant-expression, as defined in 5.19.2 of the ANSI/ISO C++ standard. 
		/// 
		/// Windows has special considerations for using thread local storage in a DLL.  
		/// 
		/// Example usage:
		///    #if defined(EA_THREAD_LOCAL)
		///        EA_THREAD_LOCAL int n = 0;                       // OK
		///        extern EA_THREAD_LOCAL struct Data s;            // OK
		///        static EA_THREAD_LOCAL char* p;                  // OK
		///        EA_THREAD_LOCAL int i = sizeof(i);               // OK.
		///        EA_THREAD_LOCAL std::string s("hello");          // Bad -- Can't be used for initialized objects.
		///        EA_THREAD_LOCAL int Function();                  // Bad -- Can't be used as return value.
		///        void Function(){ EA_THREAD_LOCAL int i = 0; }    // Bad -- Can't be used in function.
		///        void Function(EA_THREAD_LOCAL int i){ }          // Bad -- can't be used as argument.
		///        extern int i; EA_THREAD_LOCAL int i;             // Bad -- Declarations differ.
		///        int EA_THREAD_LOCAL i;                           // Bad -- Can't be used as a type modifier.
		///        EA_THREAD_LOCAL int i = i;                       // Bad -- Can't reference self before initialization.
		///    #else
		///        Need to use EA::Thread::ThreadLocalStorage.
		///    #endif

		#if !EA_THREADS_AVAILABLE
			#define EA_THREAD_LOCAL

		// Disabled until we have at least one C++11 compiler that supports this which can be tested.
		//#elif (EABASE_VERSION_N >= 20040) && !defined(EA_COMPILER_NO_THREAD_LOCAL)
		//    #define EA_THREAD_LOCAL thread_local

		#elif EA_USE_CPP11_CONCURRENCY
			#if defined(EA_COMPILER_MSVC11_0) // VC11 doesn't support C++11 thread_local storage class yet
				#define EA_THREAD_LOCAL __declspec(thread)
			#else
				#define EA_THREAD_LOCAL thread_local
			#endif

		#elif defined(__APPLE__)
			// http://clang.llvm.org/docs/LanguageExtensions.html
			#if __has_feature(cxx_thread_local)
				#define EA_THREAD_LOCAL thread_local
			#else
				#define EA_THREAD_LOCAL 
			#endif
		#elif (defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))) && (defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX)) // Any of the Unix variants, including Mac OSX.
			// While GNUC v3.3 is the first version that supports thread local storage
			// declarators, not all versions of GNUC for all platforms support it, 
			// as it requires support from other tools and libraries beyond the compiler.
			#if defined(__CYGWIN__) // Cygwin's branch of the GCC toolchain does not currently support TLS.
				// Not supported.
			#else
				#define EA_THREAD_LOCAL __thread
			#endif

		#elif defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_BORLAND) || (defined(EA_PLATFORM_WINDOWS) &&  defined(EA_COMPILER_INTEL))
			// This appears to be supported by VC++, Borland C++.
			// And it is supported by all compilers for the Windows platform.
			#define EA_THREAD_LOCAL __declspec(thread)

		#elif defined(EA_PLATFORM_SONY) || defined(CS_UNDEFINED_STRING)
			#define EA_THREAD_LOCAL __thread

		#else
			// Else don't define it as anything. This will result in a compilation 
			// error reporting the problem. We cannot simply #define away the 
			// EA_THEAD_LOCAL term, as doing so would defeat the purpose of the 
			// specifier. Dynamic thread local storage is a more flexible and
			// portable solution to the problem.
			// #define EA_THREAD_LOCAL
		#endif
		/////////////////////////////////////////////////////////////////////////

	} // namespace Thread

} // namespace EA





/////////////////////////////////////////////////////////////////////////
/// EAThreadLocalStorageData
///
/// This is used internally by class ThreadLocalStorage.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
///
#if defined(EA_PLATFORM_SONY)
	#include <kernel.h>

	struct EAThreadLocalStorageData{
		ScePthreadKey mKey;     // This is usually a pointer.
		int           mResult;  // Result of call to scePthreadKeyCreate, so we can know if mKey is valid.
	};
#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && !defined(CS_UNDEFINED_STRING)
	// In this case we will be using pthread_key_create, pthread_key_delete, pthread_getspecific, pthread_setspecific.
	#include <pthread.h>

	struct EAThreadLocalStorageData{
		pthread_key_t mKey;     // This is usually a pointer.
		int           mResult;  // Result of call to pthread_key_create, so we can know if mKey is valid.
	};

#elif defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_WINDOWS_PHONE) && !(defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)) 
	// In this case we will be using TlsAlloc, TlsFree, TlsGetValue, TlsSetValue.
	typedef uint32_t EAThreadLocalStorageData;

#elif (!EA_THREADS_AVAILABLE || defined(EA_PLATFORM_CONSOLE)) && !defined(CS_UNDEFINED_STRING)
	#include <eathread/eathread.h>

	struct EAThreadLocalStorageData
	{
		struct ThreadToDataPair
		{
			EA::Thread::ThreadUniqueId mThreadID;
			const void* mpData;
		};
		#ifndef EA_TLS_MAX_COUNT
			#define EA_TLS_MAX_COUNT 16 // This is the max number of threads that might want to use the given thread-local-storage item.
		#endif
		ThreadToDataPair* GetTLSEntry(bool bCreateIfNotFound);
		ThreadToDataPair  mDataArray[EA_TLS_MAX_COUNT];
		int               mDataArrayCount;
	};

#else // STL version which uses less memory but uses heap memory.

	// If you use this version, then you want to make sure your STL is using new/delete
	// by default and then make sure you are globally mapping new/delete to your 
	// custom allocation system. STLPort, for example, tends to want to use its
	// own internal allocator which is non-optimal for serious uses.

	EA_DISABLE_VC_WARNING(4574 4350)
	#include <map> // Note that this dependency on STL map is only present if you use this pathway, which is disabled by default.
	EA_RESTORE_VC_WARNING()

	#include <eathread/eathread.h>
	#include <eathread/eathread_futex.h>

	struct EAThreadLocalStorageData
	{
		EAThreadLocalStorageData() : mThreadToDataMap(NULL) {}
		~EAThreadLocalStorageData() { delete mThreadToDataMap; mThreadToDataMap = NULL; }
		void** GetTLSEntry(bool bCreateIfNotFound);
		// We allocate this map only when needed
		// This prevents too early allocations before our allocator initialization
		std::map<EA::Thread::ThreadUniqueId, const void*> *mThreadToDataMap;
		EA::Thread::Futex mFutex;
	private:
		// Disable copy and assignment
		EAThreadLocalStorageData(const EAThreadLocalStorageData&);
		EAThreadLocalStorageData operator=(const EAThreadLocalStorageData&);
	};
#endif
/////////////////////////////////////////////////////////////////////////



namespace EA
{
	namespace Thread
	{
		/////////////////////////////////////////////////////////////////////////
		/// class ThreadLocalStorage
		///
		/// This is a class that lets you store a pointer to data uniquely for 
		/// each thread. It thus allows access to a pointer as if it were local
		/// but each thread gets its own copy.
		///
		/// The implementation behind this class maps to the PThreads API under
		/// Unix-like systems, maps to the Windows TLS SPI under Windows, and 
		/// maps to a custom implementation otherwise. The PThreads API has a 
		/// mechanism whereby you can set a callback to execute when a thread
		/// exits; the callback will call the callback once for each pointer 
		/// that was stored in all thread local storage objects. Due to the 
		/// general weaknesses of the PThread mechanism and due to our interest
		/// in being as lean as possible, we don't support automatic callbacks
		/// such as with PThreads. The same effect can be achieved manually 
		/// when needed.
		///
		/// Example usage:
		///     ThreadLocalStorage tls;
		///     void* pValue;
		///     bool bResult;
		///     
		///     pValue  = tls.GetValue();              // Return value will be NULL.
		///     bResult = tls.SetValue(NULL);          // This is fine and bResult should be true.
		///     pValue  = tls.GetValue();              // Return value will be NULL.
		///     bResult = tls.SetValue(pSomeObject);   // Set thread-specific value to pSomeObject.
		///     bResult = tls.SetValue(pOtherObject);  // Set thread-specific value to pOtherObject.
		///     pValue  = tls.GetValue();              // Return value will be pOtherObject.
		///     bResult = tls.SetValue(NULL);          // This is fine and bResult should be true.
		///
		class EATHREADLIB_API ThreadLocalStorage
		{
		public:
			ThreadLocalStorage();
		   ~ThreadLocalStorage();

			/// GetValue
			/// Returns the pointer previous stored via GetValue or returns NULL if there
			/// is not stored value or if the user stored NULL.
			void* GetValue();

			/// SetValue
			/// Stores a pointer, returns true if the storage was possible. In general,
			/// the only reason that false would ever be returned is if there wasn't 
			/// sufficient memory remaining for the operation. When a thread exits, 
			/// it should call SetValue(NULL), as there is currently no mechanism to 
			/// automatically detect thread exits on some platforms and thus there is
			/// no way to automatically clear these values.
			bool SetValue(const void* pData);

			/// GetPlatformData
			/// Returns the platform-specific thread local storage handle for debugging
			/// uses or other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mTLSData; }

		protected:
			EAThreadLocalStorageData mTLSData;

		private:
			// Disable copy and assignment
			ThreadLocalStorage(const ThreadLocalStorage&);
			ThreadLocalStorage operator=(const ThreadLocalStorage&);
		};
		/////////////////////////////////////////////////////////////////////////



		/// ThreadLocalStorageFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class ThreadLocalStorage.
		/// A primary use of this would be to allow the ThreadLocalStorage implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API ThreadLocalStorageFactory
		{
		public:
			static ThreadLocalStorage* CreateThreadLocalStorage();                           // Internally implemented as: return new ThreadLocalStorage;
			static void                DestroyThreadLocalStorage(ThreadLocalStorage* pTLS);  // Internally implemented as: delete pTLS;

			static size_t              GetThreadLocalStorageSize();                          // Internally implemented as: return sizeof(ThreadLocalStorage);
			static ThreadLocalStorage* ConstructThreadLocalStorage(void* pMemory);           // Internally implemented as: return new(pMemory) ThreadLocalStorage;
			static void                DestructThreadLocalStorage(ThreadLocalStorage* pTLS); // Internally implemented as: pTLS->~ThreadLocalStorage();
		};



		// ThreadLocalPointer
		// This is a class that adds pointer type awareness to ThreadLocalStorage.
		// The interface is designed to look like the standard auto_ptr class.
		//
		// The following is disabled until we provide a way to enumerate and delete
		// the pointers when the object goes out of scope or delete the thread-specific 
		// pointer when the thread ends. Both are require before this class fully acts
		// as one would expect.
		//
		//template <typename T>
		//class ThreadLocalPointer
		//{
		//public:
		//    T* get()        const { return  static_cast<T*>(mTLS.GetValue()); }
		//    T* operator->() const { return  static_cast<T*>(mTLS.GetValue()); }
		//    T& operator*()  const { return *static_cast<T*>(mTLS.GetValue()); }
		//    void reset(T* pNew = 0){
		//        T* const pTemp = get();
		//        if(pNew != pTemp){
		//            delete pTemp;
		//            mTLS.SetValue(pTemp);
		//        }
		//    }
		//
		//protected:
		//    ThreadLocalStorage mTLS;
		//
		//private:
		//    ThreadLocalPointer(const ThreadLocalPointer&);
		//    const ThreadLocalPointer& operator=(const ThreadLocalPointer&);
		//};
		/////////////////////////////////////////////////////////////////////////


	} // namespace Thread

} // namespace EA


#endif // #ifdef EATHREAD_EATHREAD_STORAGE_H








