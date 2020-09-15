///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// OS globals are process-wide globals and are shared between an EXE and
// DLLs. The OS global system works at the operating system level and has
// auto-discovery logic so that no pointers or init calls need to be made
// between modules for them to link their OS global systems together.
//
// Note that the interface to OS globals is a bit convoluted because the
// core system needs to be thread-safe, cross-module, and independent of
// app-level allocators. For objects for which order of initialization is
// clearer, EASingleton is probably a better choice.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/EAGlobal.h>
#include <EAStdC/internal/Thread.h>
#include <EAStdC/EASprintf.h>
#include <stdlib.h>
#include <new>
#include <EAAssert/eaassert.h>

// Until we can test other implementations of Linux, we enable Linux only for regular desktop x86 Linux.
#if (defined(EA_PLATFORM_LINUX) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && !defined(EA_PLATFORM_ANDROID)) // What other unix-like platforms can do this?
	#include <semaphore.h>
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <unistd.h>

	#define EASTDC_EAGLOBAL_UNIX 1
#else
	#define EASTDC_EAGLOBAL_UNIX 0
#endif


#if defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(push, 0)
	#include <Windows.h>
	#pragma warning(pop)
#endif



#if defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(push)
	#pragma warning(disable: 4355)          // warning C4355: 'this' : used in base member initializer list

	#ifndef EASTDC_INIT_SEG_DEFINED
		#define EASTDC_INIT_SEG_DEFINED
		// Set initialization order between init_seg(compiler) (.CRT$XCC) and
		// init_seg(lib) (.CRT$XCL). The MSVC linker sorts the .CRT sections
		// alphabetically so we simply need to pick a name that is between
		// XCC and XCL. This works on both Windows and XBox.
		#pragma warning(disable: 4075) // "initializers put in unrecognized initialization area"
		#pragma init_seg(".CRT$XCF")
	#endif
#endif


#if EASTDC_GLOBALPTR_SUPPORT_ENABLED

namespace
{
	struct OSGlobalManager 
	{
		typedef EA::StdC::intrusive_list<EA::StdC::OSGlobalNode> OSGlobalList;

		OSGlobalList      mOSGlobalList;
		uint32_t          mRefCount;     //< Atomic reference count so that the allocator persists as long as the last module that needs it.
		EA::StdC::Mutex   mcsLock;

		OSGlobalManager();
		OSGlobalManager(const OSGlobalManager&);

		OSGlobalManager& operator=(const OSGlobalManager&);

		void Lock() {
			mcsLock.Lock();
		}

		void Unlock() {
			mcsLock.Unlock();
		}

		EA::StdC::OSGlobalNode* Find(uint32_t id);
		void                    Add(EA::StdC::OSGlobalNode* p);
		void                    Remove(EA::StdC::OSGlobalNode* p);
	};

	OSGlobalManager::OSGlobalManager() {
		EA::StdC::AtomicSet(&mRefCount, 0);
	}

	EA::StdC::OSGlobalNode* OSGlobalManager::Find(uint32_t id) {
		OSGlobalList::iterator it(mOSGlobalList.begin()), itEnd(mOSGlobalList.end());

		for(; it!=itEnd; ++it) {
			EA::StdC::OSGlobalNode& node = *it;

			if (node.mOSGlobalID == id)
				return &node;
		}

		return NULL;
	}

	void OSGlobalManager::Add(EA::StdC::OSGlobalNode *p) {
		mOSGlobalList.push_front(*p);
	}

	void OSGlobalManager::Remove(EA::StdC::OSGlobalNode *p) {
		OSGlobalList::iterator it = mOSGlobalList.locate(*p);
		mOSGlobalList.erase(it);
	}

	OSGlobalManager* gpOSGlobalManager = NULL;
	uint32_t         gOSGlobalRefs     = 0;

} // namespace



#if defined(EA_PLATFORM_MICROSOFT)

	namespace {

		#define EA_GLOBAL_UNIQUE_NAME_FORMAT    "SingleMgrMutex%08x"
		#define EA_GLOBAL_UNIQUE_NAME_FORMAT_W L"SingleMgrMutex%08x"

		// For the Microsoft desktop API (e.g. Win32, Win64) we can use Get/SetEnvironmentVariable to 
		// read/write a process-global variable. But other Microsoft APIs (e.g. XBox 360) don't support
		// this and we resort to using a semaphore to store pointer bits.
		#if !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (EA_PLATFORM_PTR_SIZE == 4)
			HANDLE ghOSGlobalManagerPtrSemaphore;
		#elif !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)  && (EA_PLATFORM_PTR_SIZE == 8)
			HANDLE ghOSGlobalManagerPtrSemaphoreHi = NULL;
			HANDLE ghOSGlobalManagerPtrSemaphoreLo = NULL;
		#endif

		bool             InitOSGlobalSystem();
		void             ShutdownOSGlobalSystem();
		OSGlobalManager* CreateOSGlobalManager();
		void             DestroyOSGlobalManager(OSGlobalManager* pOSGlobalManager);


		OSGlobalManager* CreateOSGlobalManager()
		{
			// Allocate the OSGlobal manager in the heap. We use the heap so that it can
			// hop between DLLs if the EXE itself doesn't use the manager. Note that this
			// must be the operating system heap and not an app-level heap (i.e. PPMalloc).
			// We store the pointer to the originally allocated memory at p[-1], because we
			// may have moved it during alignment.
			const size_t kAlignment = 16;

			void* p = HeapAlloc(GetProcessHeap(), 0, sizeof(OSGlobalManager) + kAlignment - 1 + sizeof(void *));
			void* pAligned = (void *)(((uintptr_t)p + sizeof(void *) + kAlignment - 1) & ~(kAlignment-1));
			((void**)pAligned)[-1] = p;

			// Placement-new the global manager into the new memory.
			return new(pAligned) OSGlobalManager;
		}


		void DestroyOSGlobalManager(OSGlobalManager* pOSGlobalManager)
		{
			if(pOSGlobalManager)
			{
				gpOSGlobalManager->~OSGlobalManager();
				HeapFree(GetProcessHeap(), 0, ((void**)gpOSGlobalManager)[-1]);
			}
		}


		bool InitOSGlobalSystem()
		{
			// The following check is not thread-safe. On most platforms this isn't an 
			// issue in practice because this function is called on application startup
			// and other threads won't be active. The primary concern is if the 
			// memory changes that result below are visible to other processors later.
			if (!gpOSGlobalManager)
			{
				// We create a named (process-global) mutex. Other threads or modules within 
				// this process share this same underlying mutex. 
				#if   !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
					// The kernel object namespace is global on Win32 so we have to choose a unique name.
					wchar_t uniqueName[64];

					EA::StdC::Sprintf(uniqueName, EA_GLOBAL_UNIQUE_NAME_FORMAT_W, (unsigned)GetCurrentProcessId());
					HANDLE hMutex = CreateMutexExW(NULL, uniqueName, CREATE_MUTEX_INITIAL_OWNER, SYNCHRONIZE);
				#else
					// The kernel object namespace is global on Win32 so we have to choose a unique name.
					char uniqueName[64];

					EA::StdC::Sprintf(uniqueName, EA_GLOBAL_UNIQUE_NAME_FORMAT, (unsigned)GetCurrentProcessId());
					HANDLE hMutex = CreateMutexA(NULL, FALSE, uniqueName);
				#endif

				if (!hMutex)
					return false;

				if (WAIT_FAILED != WaitForSingleObjectEx(hMutex, INFINITE, FALSE))
				{
					// If we don't have access to GetEnvironmentVariable and are a 32 bit platform...
					#if !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (EA_PLATFORM_PTR_SIZE == 4)
						// Xenon does not have memory mapping so we can't use the same technique
						// as for Win32, and lacks GetModuleHandle() so the old "get global proc
						// from main executable" trick won't work. What we do here is create
						// a semaphore whose value is the pointer to gpOSGlobalManager. Semaphores
						// can't hold negative values so we shift the pointer down by 4 bits
						// before storing it (it has 16 byte alignment so this is OK). Also,
						// there is no way to read a semaphore without modifying it so a mutex
						// is needed around the operation.

						wchar_t uniqueSemaphoreName[32];
						EA::StdC::Sprintf(uniqueSemaphoreName, L"SingleMgr%u", (unsigned)GetCurrentProcessId());
						ghOSGlobalManagerPtrSemaphore = CreateSemaphoreExW(NULL, 0, 0x7FFFFFFF, uniqueSemaphoreName, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);

						if (ghOSGlobalManagerPtrSemaphore)
						{
							const bool bSemaphoreExists = (GetLastError() == ERROR_ALREADY_EXISTS);

							if (bSemaphoreExists) // If somebody within our process already created it..
							{
								LONG ptrValue;

								// Read the semaphore value.
								if (ReleaseSemaphore(ghOSGlobalManagerPtrSemaphore, 1, &ptrValue)) {
									// Undo the +1 we added to the semaphore.
									WaitForSingleObjectEx(ghOSGlobalManagerPtrSemaphore, INFINITE, FALSE);

									// Recover the allocator pointer from the semaphore's original value.
									gpOSGlobalManager = (OSGlobalManager *)((uintptr_t)ptrValue << 4);
								}
								else
									EA_FAIL();
							} 
							else
							{
								gpOSGlobalManager = CreateOSGlobalManager();

								// Set the semaphore to the pointer value. It was created with
								// zero as the initial value so we add the desired value here.
								ReleaseSemaphore(ghOSGlobalManagerPtrSemaphore, (LONG)((uintptr_t)gpOSGlobalManager >> 4), NULL);
							}
						}
						else
						{
							// We have a system failure we have no way of handling.
							EA_FAIL();
						}

					// If we don't have access to GetEnvironmentVariable and are a 64 bit platform...
					#elif !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (EA_PLATFORM_PTR_SIZE == 8)

						// Semaphore counts are limited to 31 bits (LONG_MAX), but we need to store a 64 bit pointer
						// in those bits. But 64 bit pointers are always 64 bit aligned, so we need only 61 bits
						// to store a 64 bit pointer. So we store the upper 31 bits in one semaphore and the lower
						// 30 bits in another semaphore. Take the resulting 61 bits and shift left by 3 to get the 
						// full 64 bit pointer.
						// We use CreateSemaphoreExW instead of CreateSemaphoreW because the latter isn't always
						// present in the non-desktop API.

						// The kernel object namespace is session-local (not the same as app-local) so we have to choose a unique name.
						wchar_t uniqueNameHi[64];
						wchar_t uniqueNameLo[64];
						DWORD   dwProcessId = GetCurrentProcessId();
						EA::StdC::Sprintf(uniqueNameHi, L"SingleMgrHi%u", dwProcessId);
						EA::StdC::Sprintf(uniqueNameLo, L"SingleMgrLo%u", dwProcessId);

						// Create the high semaphore with a max of 31 bits of storage (0x7FFFFFFF).
						ghOSGlobalManagerPtrSemaphoreHi = CreateSemaphoreExW(NULL, 0, 0x7FFFFFFF, uniqueNameHi, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);

						if(ghOSGlobalManagerPtrSemaphoreHi)
						{
							const bool bSemaphoreExists = (GetLastError() == ERROR_ALREADY_EXISTS);
							LONG ptrValueHi;
							LONG ptrValueLo;

							if(bSemaphoreExists) // If somebody within our process already created it..
							{
								// Read the semaphore value.
								if(ReleaseSemaphore(ghOSGlobalManagerPtrSemaphoreHi, 1, &ptrValueHi))
								{
									WaitForSingleObjectEx(ghOSGlobalManagerPtrSemaphoreHi, INFINITE, FALSE);     // Undo the +1 we added with ReleaseSemaphore.

									// We still need to create our ghOSGlobalManagerPtrSemaphoreLo, which should also already exist, 
									// since some other module in this process has already exected this function. Create it with a 
									// max of 30 bits of storage (0x3FFFFFFF).
									EA_ASSERT(ghOSGlobalManagerPtrSemaphoreLo == NULL);
									ghOSGlobalManagerPtrSemaphoreLo = CreateSemaphoreExW(NULL, 0, 0x3FFFFFFF, uniqueNameLo, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
									EA_ASSERT(GetLastError() == ERROR_ALREADY_EXISTS);

									ReleaseSemaphore(ghOSGlobalManagerPtrSemaphoreLo, 1, &ptrValueLo);
									WaitForSingleObjectEx(ghOSGlobalManagerPtrSemaphoreLo, INFINITE, FALSE);     // Undo the +1 we added with ReleaseSemaphore.

									// Recover the allocator pointer from the semaphore's original value.
									uintptr_t ptr = (((uintptr_t)ptrValueHi) << 30 | ptrValueLo) << 3; // Combine the pair into 61 bits, and shift left by 3. This is the reverse of the code below.
									gpOSGlobalManager = (OSGlobalManager*)ptr;
								}
								else
									EA_FAIL(); // In practice this cannot happen unless the machine is cripped beyond repair.
							}
							else // Else our CreateSemaphorExW call was the first one to create ghOSGlobalManagerPtrSemaphoreHi.
							{
								gpOSGlobalManager = CreateOSGlobalManager();
								EA_ASSERT(gpOSGlobalManager && (((uintptr_t)gpOSGlobalManager & 7) == 0)); // All pointers on 64 bit platforms should have their lower 3 bits unused.

								// Set the semaphore to the pointer value. It was created with
								// zero as the initial value so we add the desired value here.
								uintptr_t ptr = (uintptr_t)gpOSGlobalManager >> 3; // ptr now has the 61 significant bits of gpOSGlobalManager.

								ptrValueHi = static_cast<LONG>(ptr >> 30);         // ptrValueHi has the upper 31 of the 61 bits.
								ptrValueLo = static_cast<LONG>(ptr & 0x3FFFFFFF);  // ptrValueLo has the lower 30 of the 61 bits.

								// We still need to create our ghOSGlobalManagerPtrSemaphoreLo, which should also not already exist.
								// Create it with a max of 30 bits of storage (0x3FFFFFFF).
								EA_ASSERT(ghOSGlobalManagerPtrSemaphoreLo == NULL);
								ghOSGlobalManagerPtrSemaphoreLo = CreateSemaphoreExW(NULL, 0, 0x3FFFFFFF, uniqueNameLo, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
								EA_ASSERT(GetLastError() != ERROR_ALREADY_EXISTS);

								EA_ASSERT((ghOSGlobalManagerPtrSemaphoreHi != NULL) && (ghOSGlobalManagerPtrSemaphoreLo != NULL)); // Should always be true, due to the logic in this function.
								ReleaseSemaphore(ghOSGlobalManagerPtrSemaphoreHi, ptrValueHi, NULL);
								ReleaseSemaphore(ghOSGlobalManagerPtrSemaphoreLo, ptrValueLo, NULL);

								// Now semaphoreHi has the upper 31 significant bits of the gpOSGlobalManager pointer value.
								// and semaphoreLo has the lower 30 significant bits of the gpOSGlobalManager pointer value.
							}
						}
						else
						{
							// We have a system failure we have no way of handling.
							EA_FAIL();
						}

					#else

						// Under Win32 and Win64, we use system environment variables to store the gpOSGlobalManager value.
						char stringPtr[32];
						const DWORD dwResult = GetEnvironmentVariableA(uniqueName, stringPtr, 32);

						if((dwResult > 0) && dwResult < 32 && stringPtr[0]) // If the variable was found...
							gpOSGlobalManager = (OSGlobalManager *)(uintptr_t)_strtoui64(stringPtr, NULL, 16); // _strtoui64 is a VC++ extension function.
						else
						{
							// GetLastError() should be ERROR_ENVVAR_NOT_FOUND. But what do we do if it isn't?
							gpOSGlobalManager = CreateOSGlobalManager();

							EA::StdC::Sprintf(stringPtr, "%I64x", (uint64_t)(uintptr_t)gpOSGlobalManager);
							SetEnvironmentVariableA(uniqueName, stringPtr); // There's not much we can do if this call fails.
						}

					#endif

					EA_ASSERT(gpOSGlobalManager && (gpOSGlobalManager->mRefCount < UINT32_MAX));
					EA::StdC::AtomicIncrement(&gpOSGlobalManager->mRefCount);

					BOOL result = ReleaseMutex(hMutex);
					EA_ASSERT(result); EA_UNUSED(result);
				}

				BOOL result = CloseHandle(hMutex);
				EA_ASSERT(result); EA_UNUSED(result);

				if (!gpOSGlobalManager)
				{
					ShutdownOSGlobalSystem();
					return false;
				}

				EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
				EA::StdC::AtomicIncrement(&gOSGlobalRefs);  // Increment it once for the init of this system (InitOSGlobalSystem). This increment will be matched by a decrement in ShutdownOSGlobalSystem.
			}

			return true;
		}

		void ShutdownOSGlobalSystem()
		{
			if (EA::StdC::AtomicDecrement(&gOSGlobalRefs) == 0) // If the (atomic) integer decrement results in a refcount of zero...
			{
				if (gpOSGlobalManager)
				{
					if (EA::StdC::AtomicDecrement(&gpOSGlobalManager->mRefCount) == 0)
						DestroyOSGlobalManager(gpOSGlobalManager);

					gpOSGlobalManager = NULL;
				}

				#if !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (EA_PLATFORM_PTR_SIZE == 4)
					if (ghOSGlobalManagerPtrSemaphore)
					{
						CloseHandle(ghOSGlobalManagerPtrSemaphore);
						ghOSGlobalManagerPtrSemaphore = NULL;
					}
				#elif !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (EA_PLATFORM_PTR_SIZE == 8)
					if (ghOSGlobalManagerPtrSemaphoreHi)
					{
						CloseHandle(ghOSGlobalManagerPtrSemaphoreHi);
						ghOSGlobalManagerPtrSemaphoreHi = NULL;
					}

					if (ghOSGlobalManagerPtrSemaphoreLo)
					{
						CloseHandle(ghOSGlobalManagerPtrSemaphoreLo);
						ghOSGlobalManagerPtrSemaphoreLo = NULL;
					}
				#else
					// Clear the gpOSGlobalManager environment variable.
					// This code needs to be called in a thread-safe way by the user, usually by calling it once on shutdown. 
					// We have a problem if this function is executing at the same time some other entity in this process 
					// is currently doing some new use of the OS global system, as that can cause an instance of gpOSGlobalManager
					// to be created while we are in the process of destroying it.
					char uniqueName[64]; uniqueName[0] = 0;

					EA::StdC::Sprintf(uniqueName, EA_GLOBAL_UNIQUE_NAME_FORMAT, (unsigned)GetCurrentProcessId());
					SetEnvironmentVariableA(uniqueName, NULL);
				#endif
			}
		}

	} // namespace

#elif defined(EA_PLATFORM_SONY)

	// On this platform we use sceKernelReserveVirtualRange with a specific predetermined address, and place our
	// OSGlobalManager there. There's a bit of careful code below to deal with possible snafus while doing this,
	// and even with that this code still has some caveats about its usage, as described below. Most of the time
	// those caveats won't come into play, as they are relevant only for very unusual application usage patterns
	// and can still be worked around if those patterns happen to come into play.
 
    #include <sys/dmem.h>
	#include <kernel.h>
	#include <string.h>
	#include <sceerror.h>
	#include <eathread/eathread_sync.h>
	#include <EAStdC/EAStopwatch.h>


	struct OSGlobalManagerContainer
	{
		uint8_t         mMagicNumber[16];  // This is a unique value which guarantees that this is the address of the OSGlobalManager.
		OSGlobalManager mOSGlobalManager;
	};

	const size_t   kMemSize = 16384;
	const uint64_t kAddr64             = SCE_KERNEL_APP_MAP_AREA_END_ADDR - kMemSize;    // End of appication memory space. https://ps4.scedev.net/resources/documents/SDK/3.000/Kernel-Overview/0004.html
	const uint8_t  kMagic[16]          = { 0xD1, 0x4B, 0x78, 0x49, 0x81, 0xF1, 0x45, 0x0D, 0x91, 0x08, 0x1B, 0xA8, 0xE7, 0x8A, 0xD1, 0xD3 }; // Random bytes.
	const size_t   kDirectMemoryLength = 16 * 1024; // 16 KB minimum for sceKernelAllocateDirectMemory
	bool gbKeepDirectMemory            = false;
	off_t gDirectMemoryStartAddr           = 0;

	bool InitOSGlobalSystem()
	{
		// The code below involves a number of careful steps to implement sharing a memory address between DLLs.
		// We have this code because this platform provides no other means for sharing a global piece of memory
		// between modules. On other platforms (e.g. Unix) there are environment variables we can use, and on 
		// others (e.g. Windows) there are uniquely named synchronization primitives we can use. On still others
		// there are writable semaphore disk files that can be used. On this platform there is no environment variable
		// support, synchronization primitives have names but are not unique like on Windows, and disk files do 
		// exist but only if you tag your application manifest to support the /download0 file mount. 

		if(!gpOSGlobalManager)
		{
			uint64_t currentAddr = kAddr64;
			
			int32_t err = sceKernelAllocateDirectMemory(
					0,
					SCE_KERNEL_MAIN_DMEM_SIZE,
					kDirectMemoryLength,
					0,
				#if defined(EA_PLATFORM_PS4)
					SCE_KERNEL_WB_ONION,
				#else
					SCE_KERNEL_MTYPE_C_SHARED,
				#endif
					&gDirectMemoryStartAddr);
			EA_ASSERT(err == SCE_OK);
			
			if(err != SCE_OK)
			{
				return false;
			}
					
			gbKeepDirectMemory = false;

			while (!gpOSGlobalManager && (currentAddr >= (kAddr64 - EA::StdC::kKettleOSGlobalSearchSpace)))
			{
				void*                     addr   = reinterpret_cast<void*>(currentAddr);
				OSGlobalManagerContainer* pOSGlobalManagerContainer = static_cast<OSGlobalManagerContainer*>(addr);
				int                       result = SCE_OK; // Disabled because it doesn't work how we need it to: = sceKernelReserveVirtualRange(&addr, kMemSize, SCE_KERNEL_MAP_FIXED | SCE_KERNEL_MAP_NO_OVERWRITE, 0);
				// Possible return values are SCE_OK, SCE_KERNEL_ERROR_EINVAL, and SCE_KERNEL_ERROR_ENOMEM.

				if((result == SCE_KERNEL_ERROR_ENOMEM) || (result == SCE_OK))
				{
					// SCE_KERNEL_ERROR_ENOMEM occurs when the address has already been reserved, which may have been 
					// done by a previous EAGlobal init occurred. With either that or SCE_OK, we call scekernelMapDirectMemory.
					// We call scekernelMapDirectMemory even if we get SCE_KERNEL_ERROR_ENOMEM because another thread may 
					// have called sceKernelReserveVirtualRange first, but we may be executing simultaneously and execute
					// sceKernelMapDirectMemory first. 

					result = sceKernelMapNamedDirectMemory(&addr, kMemSize, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE, 
														SCE_KERNEL_MAP_FIXED | SCE_KERNEL_MAP_NO_OVERWRITE, gDirectMemoryStartAddr, 0, "EAOSGlobal");
					// Possible return values are SCE_OK, SCE_KERNEL_ERROR_EACCES, SCE_KERNEL_ERROR_EINVAL, and SCE_KERNEL_ERROR_ENOMEM.
					// Normally we expect that SCE_KERNEL_ERROR_ENOMEM or SCE_OK will be returned. If the sceKernelReserveVirtualRange
					// returned SCE_OK then usually sceKernalMapDirectMemory will return SCE_OK for us, because if we were the first
					// to call the reserve function then we will probably be the first to call the map function.

					if((result == SCE_KERNEL_ERROR_ENOMEM) || (result == SCE_OK))
					{
						// At this point the memory at addr is mapped to our address space and we can attempt to read and write it.
						// To make sure this memory really is read/write for us, we query the kernel for its protection type.
						bool weWereHereFirst = (result == SCE_OK); // IF we were here first then we take care of initializing the pOSGlobalManagerContainer->mOSGlobalManager instance.

						if(weWereHereFirst)
						{
							// We use some low level synchronization primitives here to accomplish memory synchronization. We are unable
							// to use a mutex to do this because there is no way to share a mutex anonymously between modules. The whole
							// reason EAGlobal exists is to allow sharing variables anonymously between modules. So we have a chicken and
							// egg problem: we can't share a mutex between modules until InitOSGlobalSystem has completed.

							::new(&pOSGlobalManagerContainer->mOSGlobalManager) OSGlobalManager;
							EA::StdC::AtomicIncrement(&pOSGlobalManagerContainer->mOSGlobalManager.mRefCount);
							gpOSGlobalManager = &pOSGlobalManagerContainer->mOSGlobalManager;
							EAWriteBarrier(); // Make sure this is seen as written before the memcpy is seen as written.
							EACompilerMemoryBarrier(); // Don't let the compiler move the above code to after the below code.

							memcpy(pOSGlobalManagerContainer->mMagicNumber, kMagic, sizeof(pOSGlobalManagerContainer->mMagicNumber));
							EAWriteBarrier(); // Make sure other threads see this write.

							gbKeepDirectMemory = true;
						}
						else // Else somebody before us mapped this memory. We need to validate it before trying to use it.
						{
							// We have a problem in that as we execute this code, another thread might have just started executing the 
							// the weWereHere = true pathway above. So the magic value might not have been written yet. We don't currently
							// have an easy means to deal with this other than to loop for N milliseconds while waiting for the other
							// thread to complete. 
							// Another potential problem can occur if two threads of differing priorities execute on the same CPU and 
							// one blocks the other from completing this code. That would be a 

							int protection = 0;
							result = sceKernelQueryMemoryProtection(addr, NULL, NULL, &protection);

							if((result == SCE_OK) && (protection & SCE_KERNEL_PROT_CPU_READ) && (protection & SCE_KERNEL_PROT_CPU_WRITE))
							{
								EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsMilliseconds, 500, true);

								while(!limitStopwatch.IsTimeUp())
								{
									EAReadBarrier();  // Make sure we see the previous writes of other threads prior to the read below.

									if(memcmp(kMagic, pOSGlobalManagerContainer->mMagicNumber, sizeof(pOSGlobalManagerContainer->mMagicNumber)) == 0) // If it's our memory...
									{
										// At this point we were able to create a new shared memory area or acquire the shared memory area that
										// some other InitOSGlobalSystem function call previously created.
										EA::StdC::AtomicIncrement(&pOSGlobalManagerContainer->mOSGlobalManager.mRefCount);
										gpOSGlobalManager = &pOSGlobalManagerContainer->mOSGlobalManager;
										EAWriteBarrier(); // This isn't strictly needed, but it can theoretically make things go faster for other threads.

										break;
									}
									// Else either the other thread is still initializing pOSGlobalManagerContainer or some other completely
									// unrelated entity is using this memory for something else. We don't have a good means of detecting 
									// the latter other than timing out. Maybe if sceKernelMapDirectMemory guarantees writing 0 to the memory
									// then we can exit this loop much faster.

									// There must be another thread executing the weWereHereFirst = true pathway. We sleep so that thread 
									// can complete that pathway. Possibly are are a higher priority thread which could be blocking it.
									SceKernelTimespec ts = { 0, 2000000 };
									sceKernelNanosleep(&ts, NULL);
								}
							}
						}
					}
				}

				// Try another address. The logic above has determined that the address was used by some other entity. 
				// That may well indicate that we need to pick a new starting address to try, as we really want this to 
				// always succeed the first time through. Note that this bumping up is safe and works as intended, because
				// if one module fails to work at the original address, all will (assuming that during startup some thread
				// doesn't map it before we try to get it and then later unmap it before the other modules that use this have loaded).
				currentAddr -= (1024 * 1024);

			} // while ... 

			if(!gbKeepDirectMemory)
			{
				sceKernelReleaseDirectMemory(gDirectMemoryStartAddr, kDirectMemoryLength);
				gDirectMemoryStartAddr = 0;
			}
		} // if(!gpOSGlobalManager)

		if(gpOSGlobalManager) // (We have an AddRef on gpOSGlobalManager from above, so gpOSGlobalManager can't possibly have become invalid at this point)
		{
			// gOSGlobalRefs measures the number of times InitOSGlobalSystem/ShutdowOSGlobalSystem was successfully called.
			EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
			EA::StdC::AtomicIncrement(&gOSGlobalRefs);  // Increment it once for the init of this system (InitOSGlobalSystem). This increment will be matched by a decrement in ShutdownOSGlobalSystem.

			return true;
		}

		return false;
	}


	void ShutdownOSGlobalSystem()
	{
		// gOSGlobalRefs measures the number of times InitOSGlobalSystem/ShutdowOSGlobalSystem was successfully called.
		if(EA::StdC::AtomicDecrement(&gOSGlobalRefs) == 0) // If the (atomic) integer decrement results in a refcount of zero...
		{ 
			if(gpOSGlobalManager)
			{
				if(EA::StdC::AtomicDecrement(&gpOSGlobalManager->mRefCount) == 0) // mRefCount measures the use count of glOSGlobalManager.
				{
					// To consider: we can unmap the memory at gpOSGlobalManager - 16 bytes here. It doesn't buy us anything 
					// aside from possibly making some tools see that we freed the mapped kernel memory.
				}
				if(gbKeepDirectMemory)
				{
					sceKernelReleaseDirectMemory(gDirectMemoryStartAddr, kDirectMemoryLength);
					gDirectMemoryStartAddr = 0;
				}
				gpOSGlobalManager = NULL;
			}
		}
	}

#elif EASTDC_EAGLOBAL_UNIX

	namespace {

		#define EA_GLOBAL_UNIQUE_NAME_FORMAT "/SingleMgrMutex%llu"

		OSGlobalManager* CreateOSGlobalManager();
		bool             InitOSGlobalSystem();
		void             ShutdownOSGlobalSystem();
		

		OSGlobalManager* CreateOSGlobalManager()
		{
			// Allocate the OSGlobal manager in shared memory. 
			#if defined(__APPLE__)
				void* pMemory = mmap(NULL, sizeof(OSGlobalManager), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,      -1, 0);
			#else
				void* pMemory = mmap(NULL, sizeof(OSGlobalManager), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			#endif

			if(pMemory) // Some Unix variants (e.g. mobile) can fail this call due to lack of support for it.
			{
				EA_ASSERT(((uintptr_t)pMemory & 15) == 0); // Make sure mmap returns at least 16 byte alignment.
 
				// Placement-new the global manager into the new memory.
				return new(pMemory) OSGlobalManager;
			}

			return NULL;
		}


		void DestroyOSGlobalManager(OSGlobalManager* pOSGlobalManager)
		{
			if(pOSGlobalManager)
			{
				gpOSGlobalManager->~OSGlobalManager();
				munmap(pOSGlobalManager, sizeof(OSGlobalManager));
			}
		}


		bool InitOSGlobalSystem()
		{
			// The following check is not thread-safe. On most platforms this isn't an 
			// issue in practice because this function is called on application startup
			// and other threads won't be active. The primary concern is if the 
			// memory changes that result below are visible to other processors later.

			if(!gpOSGlobalManager)
			{
				// We make a process-unique name based on the process id.
				char  uniqueName[96];
				pid_t processID = getpid();

				EA::StdC::Sprintf(uniqueName, EA_GLOBAL_UNIQUE_NAME_FORMAT, (unsigned long long)processID);
				sem_t* mutex = sem_open(uniqueName, O_CREAT, 0644, 1); // Unix has named semaphores but doesn't really have named mutexes, so we use a semaphore as a mutex.

				if(mutex == SEM_FAILED)
					return false;

				if(sem_wait(mutex) == 0) // If locking the mutex was successful...
				{
					// As of this writing, we are using getenv/setenv to write a shared variable pointer. It turns out that this
					// is not a good idea, because getenv/setenv is not thread-safe. getenv returns a pointer to static memory
					// which another thread (who isn't using our mutex) might call setenv in a way that changes that memory.
					// The opinion of the Linux people is that you just shouldn't ever call setenv during application runtime.
					// A better solution for us is to use shared mapped memory (shm_open(), mmap()): http://www.ibm.com/developerworks/aix/library/au-spunix_sharedmemory/index.html

					const char* pName = getenv(uniqueName);

					if(pName && pName[0]) // If the variable was found...
						gpOSGlobalManager = (OSGlobalManager*)(uintptr_t)strtoull(pName, NULL, 16);
					else
					{
						gpOSGlobalManager = CreateOSGlobalManager();

						if(gpOSGlobalManager)
						{
							char buffer[32];
							EA::StdC::Sprintf(buffer, "%I64x", (uint64_t)(uintptr_t)gpOSGlobalManager);
							/*int result =*/ setenv(uniqueName, buffer, 1);
						}
					}

					if(gpOSGlobalManager)
					{
						EA_ASSERT(gpOSGlobalManager->mRefCount < UINT32_MAX);
						EA::StdC::AtomicIncrement(&gpOSGlobalManager->mRefCount);
					}

					sem_post(mutex);
					sem_close(mutex);
					sem_unlink(uniqueName);
				}

				if(!gpOSGlobalManager)
				{
					ShutdownOSGlobalSystem();
					return false;
				}

				EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
				EA::StdC::AtomicIncrement(&gOSGlobalRefs);  // Increment it once for the init of this system (InitOSGlobalSystem). This increment will be matched by a decrement in ShutdownOSGlobalSystem.
			}

			return true;
		}

		void ShutdownOSGlobalSystem()
		{
			if(EA::StdC::AtomicDecrement(&gOSGlobalRefs) == 0) // If the (atomic) integer decrement results in a refcount of zero...
			{ 
				if(gpOSGlobalManager)
				{
					if(EA::StdC::AtomicDecrement(&gpOSGlobalManager->mRefCount) == 0)
						DestroyOSGlobalManager(gpOSGlobalManager);

					gpOSGlobalManager = NULL;
				}

				// Clear the gpOSGlobalManager environment variable.
				// This code needs to be called in a thread-safe way by the user, usually by calling it once on shutdown. 
				// We have a problem if this function is executing at the same time some other entity in this process 
				// is currently doing some new use of the OS global system, as that can cause an instance of gpOSGlobalManager
				// to be created while we are in the process of destroying it.
				char  uniqueName[96]; uniqueName[0] = 0;
				pid_t processID = getpid();

				EA::StdC::Sprintf(uniqueName, EA_GLOBAL_UNIQUE_NAME_FORMAT, (unsigned long long)processID);
				 /*int result =*/ unsetenv(uniqueName);
			}
		}

	} // namespace

#else // #if defined(EA_PLATFORM_MICROSOFT)

	namespace {

		static uint64_t sOSGlobalMgrMemory[(sizeof(OSGlobalManager) + 1) / sizeof(uint64_t)];
		
		bool InitOSGlobalSystem()
		{
			// Theoretical problem: If you keep calling this function, eventually gOSGlobalRefs will overflow.
			EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
			if(EA::StdC::AtomicIncrement(&gOSGlobalRefs) == 1)
				gpOSGlobalManager = new(sOSGlobalMgrMemory) OSGlobalManager;
			return true;
		}

		void ShutdownOSGlobalSystem()
		{
			if(EA::StdC::AtomicDecrement(&gOSGlobalRefs) == 0) // If the (atomic) integer decrement results in a refcount of zero...
				gpOSGlobalManager = NULL;
		}
	}

#endif // #if defined(EA_PLATFORM_MICROSOFT)




EASTDC_API EA::StdC::OSGlobalNode* EA::StdC::GetOSGlobal(uint32_t id, OSGlobalFactoryPtr pFactory)
{
	// Initialize up the OSGlobal system if we are getting called before
	// static init, i.e. allocator
	if (!InitOSGlobalSystem())
		return NULL;

	gpOSGlobalManager->Lock();

	EA::StdC::OSGlobalNode* p = gpOSGlobalManager->Find(id);

	if (!p && pFactory)
	{
		p = pFactory();
		p->mOSGlobalID = id;
		AtomicSet(&p->mOSGlobalRefCount, 0);
		gpOSGlobalManager->Add(p);
	}

	if (p)
	{
		EA_ASSERT(p->mOSGlobalRefCount < UINT32_MAX);
		AtomicIncrement(&p->mOSGlobalRefCount);

		EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
		AtomicIncrement(&gOSGlobalRefs);
	}

	gpOSGlobalManager->Unlock();

	return p;
}


EASTDC_API bool EA::StdC::SetOSGlobal(uint32_t id, EA::StdC::OSGlobalNode *p)
{
	// Initialize up the OSGlobal system if we are getting called before
	// static init, i.e. allocator
	if (!InitOSGlobalSystem())
		return false;

	gpOSGlobalManager->Lock();

	EA::StdC::OSGlobalNode* const pTemp = gpOSGlobalManager->Find(id);

	if (pTemp == NULL) // If there isn't one already...
	{
		p->mOSGlobalID = id;
		AtomicSet(&p->mOSGlobalRefCount, 0);
		gpOSGlobalManager->Add(p);

		EA_ASSERT(p->mOSGlobalRefCount < UINT32_MAX);
		AtomicIncrement(&p->mOSGlobalRefCount);

		EA_ASSERT(gOSGlobalRefs < UINT32_MAX);
		AtomicIncrement(&gOSGlobalRefs);
	}
	
	gpOSGlobalManager->Unlock();

	return (pTemp == NULL);
}


EASTDC_API bool EA::StdC::ReleaseOSGlobal(EA::StdC::OSGlobalNode *p)
{
	gpOSGlobalManager->Lock();

	const bool shouldDestroyManager  = AtomicDecrement(&gOSGlobalRefs) == 0;
	const bool shouldDestroyOSGlobal = AtomicDecrement(&p->mOSGlobalRefCount) == 0;

	if (shouldDestroyOSGlobal)
		gpOSGlobalManager->Remove(p);

	gpOSGlobalManager->Unlock();

	// Note by Paul Pedriana (10/2009): It seems to me that shouldDestroyManager will never 
	// be true here because InitOSGlobalSystem will have been called at app startup and 
	// its gOSGlobalRefs increment will still be live. So only when that last explicit 
	// call to ShutdownOSGlobalSystem is called will gOSGlobalRefs go to zero.
	if (shouldDestroyManager)
		ShutdownOSGlobalSystem(); // This function decrements gOSGlobalRefs.

	return shouldDestroyOSGlobal;
}


// Force the OSGlobal manager to be available for the life of the app.
// It's OK if this comes up too late for some uses because GetOSGlobal()
// will bring it online earlier in that case.
namespace 
{
		struct AutoinitOSGlobalManager
		{
			AutoinitOSGlobalManager()
			{
				bool result = InitOSGlobalSystem();
				EA_ASSERT(result); EA_UNUSED(result);
			}

			~AutoinitOSGlobalManager()
			{
				ShutdownOSGlobalSystem();
			}
		};

		AutoinitOSGlobalManager gAutoinitOSGlobalManager;
}


#endif // EASTDC_GLOBALPTR_SUPPORT_ENABLED

#if defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(pop) // symmetric for pop for the above -->   #pragma warning(disable: 4355)          // warning C4355: 'this' : used in base member initializer list
#endif






