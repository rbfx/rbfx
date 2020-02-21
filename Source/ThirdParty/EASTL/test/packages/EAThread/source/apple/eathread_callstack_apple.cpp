///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread.h>
#include <eathread/eathread_futex.h>
#include <eathread/eathread_storage.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/apple/eathread_callstack_apple.h>
#include <mach/thread_act.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <new>


#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
#include <mach-o/dyld_images.h> //dyld_all_image_infos
#include <mach-o/dyld.h> //segment_command(_64)
#include <mach/task.h> //task_info

#if defined(EA_PLATFORM_IPHONE)
    //On iPhone, this gets pulled in dynamically through libproc.dylib
    extern "C" int proc_regionfilename(int pid, uint64_t address, void * buffer, uint32_t buffersize);
#else
    #include <libproc.h> //proc_regionfilename
#endif
#endif


#if defined(__LP64__)
typedef struct mach_header_64     MachHeader;
typedef struct segment_command_64 SegmentCommand;
typedef struct section_64         Section;
#define kLCSegment                LC_SEGMENT_64
#else
typedef struct mach_header        MachHeader;
typedef struct segment_command    SegmentCommand;
typedef struct section            Section;
#define kLCSegment                LC_SEGMENT
#endif


#if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
	#include <signal.h>
	#include <execinfo.h>
#endif



namespace EA
{
namespace Thread
{


///////////////////////////////////////////////////////////////////////////////
// gModuleInfoApple
// 
// We keep a cached array of the module info. It's possible that the module
// info could change at runtime, though for our purposes the changes don't 
// usually matter. Nevertheless that's a limitation of this scheme and we
// may need to do something about it in the future.
// This global array is freed in ShutdownCallstack.
// Currently this array is stored per-DLL when EAThread is built as a DLL.
//
static ModuleInfoApple* gModuleInfoAppleArray      = NULL;
static size_t           gModuleInfoAppleArrayCount = 0;
static Futex*           gCallstackFutex            = NULL;


///////////////////////////////////////////////////////////////////////////////
// ReallocModuleInfoApple
//
// This is not a fully generic Realloc function. It currently reallocs only
// to a greater size or to zero, which is fine for our purposes. The caller
// of this function needs to be aware that the Realloc may fail and should
// use gModuleInfoAppleArrayCount as the array count and not the value passed
// to this function.
//
static ModuleInfoApple* ReallocModuleInfoApple(size_t newCount)
{
	if(gCallstackFutex)
		gCallstackFutex->Lock();

	EA::Thread::Allocator* pAllocator = EA::Thread::GetAllocator();

	EAT_ASSERT_MSG(pAllocator != NULL, "EA::Thread::SetAllocator needs to be called on app startup.");
	if(pAllocator)
	{
		if(newCount > gModuleInfoAppleArrayCount) // If increasing in size...
		{
			size_t allocSize   = sizeof(ModuleInfoApple) * newCount;
			void*  allocMemory = pAllocator->Alloc(allocSize);

			if(allocMemory)
			{
				ModuleInfoApple* pNew = new(allocMemory) ModuleInfoApple[newCount]; // Placement new always succeeds.
				
				if(gModuleInfoAppleArray && gModuleInfoAppleArray)
				{
					// gModuleInfoAppleArrayCount is guaranteed to be < newCount for this memcpy.
					memcpy(pNew, gModuleInfoAppleArray, sizeof(ModuleInfoApple) * gModuleInfoAppleArrayCount);
					pAllocator->Free(gModuleInfoAppleArray);
				}

				gModuleInfoAppleArray      = pNew;
				gModuleInfoAppleArrayCount = newCount;
			}
			// Else fall through and use the existing gModuleInfoAppleArray.
		}
		else if(newCount == 0) // If freeing...
		{
			if(gModuleInfoAppleArray)
			{
				pAllocator->Free(gModuleInfoAppleArray);
				gModuleInfoAppleArray      = NULL;
				gModuleInfoAppleArrayCount = 0;
			}
		}
		// Else we do nothing for the case of requesting a newCount < gModuleInfoAppleArrayCount.
	}

	if(gCallstackFutex)
		gCallstackFutex->Unlock();

	return gModuleInfoAppleArray; // gModuleInfoAppleArrayCount indicates the capacity of this array.
}

#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
// This fills a moduleInfoApple object with the information from all of the segments listed in the given mach_header's segments, starting at the given currentSegmentPos. It also puts the pModulePath info the moduleInfoApple object, which is then push_back on the given array.
//
// The results are appended to pModuleInfoAppleArray up to its capacity
// pTypeFilter is used to filter out segment types
// pModulePath is the path corresponding to the given pMachHeader. It is assumed it is NullTerminated
// currentSegmentPos is the starting segment we are iterating over
// pMachHeader is the mach_header with all the segment information
void CreateModuleInfoApple(ModuleInfoApple* pModuleInfoAppleArray, size_t arrayCapacity, size_t& requiredArraySize, size_t& arraySize,
                           const char* pTypeFilter, const char* pModulePath, uintptr_t currentSegmentPos, const MachHeader* pMachHeader, intptr_t offset)
{
    for(uint32_t i = 0; i < pMachHeader->ncmds; i++) // Look at each command, paying attention to LC_SEGMENT/LC_SEGMENT_64 (segment_command) commands.
    {
        const SegmentCommand* pSegmentCommand = reinterpret_cast<const SegmentCommand*>(currentSegmentPos); // This won't actually be a segment_command unless the type is kLCSegment
        
        if(pSegmentCommand != NULL && pSegmentCommand->cmd == kLCSegment) // If this really is a segment_command... (otherwise it is some other kind of command)
        {
            const size_t segnameBufferLen = sizeof(pSegmentCommand->segname) + 1;
            
            char segnameBuffer[segnameBufferLen];
            memcpy(segnameBuffer, pSegmentCommand->segname, sizeof(pSegmentCommand->segname));
            segnameBuffer[segnameBufferLen-1] = '\0'; // Incase segname was not 0-terminated
            
            if(!pTypeFilter || strncmp(segnameBuffer, pTypeFilter, sizeof(segnameBuffer)))
            {
                requiredArraySize++;
                
                if (arraySize < arrayCapacity)
                {
                    ModuleInfoApple& info = pModuleInfoAppleArray[arraySize++];
                
                    uint64_t uOffset = (uint64_t)offset;
                    info.mBaseAddress = (uint64_t)(pSegmentCommand->vmaddr + uOffset);
                    // info.mModuleHandle = reinterpret_cast<ModuleHandle>((uintptr_t)info.mBaseAddress);
                    info.mSize = (uint64_t)pSegmentCommand->vmsize;
                    
                    // Copy modulePath to info.mPath.
                    strlcpy(info.mPath, pModulePath, EAArrayCount(info.mPath));

                    // Get the beginning of the file name within modulePath and copy the file name to info.mName.
                    const char* pDirSeparator = strrchr(pModulePath, '/');
                    if(pDirSeparator)
                        pDirSeparator++;
                    else
                        pDirSeparator = pModulePath;
                    strlcpy(info.mName, pDirSeparator, EAArrayCount(info.mName));
                    
                    info.mPermissions[0] = (pSegmentCommand->initprot & VM_PROT_READ)    ? 'r' : '-';
                    info.mPermissions[1] = (pSegmentCommand->initprot & VM_PROT_WRITE)   ? 'w' : '-';
                    info.mPermissions[2] = (pSegmentCommand->initprot & VM_PROT_EXECUTE) ? 'x' : '-';
                    info.mPermissions[3] = '/';
                    info.mPermissions[4] = (pSegmentCommand->maxprot & VM_PROT_READ)    ? 'r' : '-';
                    info.mPermissions[5] = (pSegmentCommand->maxprot & VM_PROT_WRITE)   ? 'w' : '-';
                    info.mPermissions[6] = (pSegmentCommand->maxprot & VM_PROT_EXECUTE) ? 'x' : '-';
                    info.mPermissions[7] = '\0';
                    
                    strlcpy(info.mType,pSegmentCommand->segname,EAArrayCount(info.mType));
                    //**********************************************************************************
                    //For Debugging Purposes
                    //__TEXT                 0000000100000000-0000000100001000 [    4K] r-x/rwx SM=COW  /Build/Products/Debug/TestProject.app/Contents/MacOS/TestProject
                    //printf("%20s %llx-%llx %s %s\n", segnameBuffer, (unsigned long long)info.mBaseAddress, (unsigned long long)(info.mBaseAddress + pSegmentCommand->vmsize), info.mPermissions, pModulePath);
                    //**********************************************************************************/
                }
            }
            
        }
        currentSegmentPos += pSegmentCommand->cmdsize;
    }
}
#endif // EATHREAD_APPLE_GETMODULEINFO_ENABLED


#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
// GetModuleInfoApple
//
// This function exists for the purpose of being a central module/VM map info collecting function,
// used by a couple functions within EACallstack.
//
// We used to use vmmap and parse the output
// https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/vmmap.1.html
// But starting ~osx 10.9 vmmap can not be called due to new security restrictions
//
// I tried using ::mach_vm_region, but I was unable to find the type of the segments (__TEXT, etc.)
// and system libraries addresses were given, but their name/modulePath was not.
// ::mach_vm_region_recurse did not solve this problem either.
//
// Replaced _dyld_get_all_image_infos() call with task_info() call as the old call is no longer available
// with osx 10.13 (High Sierra).
size_t GetModuleInfoApple(ModuleInfoApple* pModuleInfoAppleArray, size_t arrayCapacity,
                            const char* pTypeFilter, bool bEnableCache)
{
    // The following is present to handle the case that the user forgot to call EA::Thread::InitCallstack().
    // We don't match this with a ShutdownCallstack, and so the user might see memory leaks in that case.
    // The user should call EA::Thread::InitCallstack on app startup and EA::Thread::ShutdownCallstack on 
    // app shutdown, at least if the user wants to use this function.
    if(!gCallstackFutex)
        InitCallstack();
        
    size_t requiredArraySize = 0;
    size_t arraySize = 0;

    if(bEnableCache)
    {
        if(gCallstackFutex)
            gCallstackFutex->Lock();

        if(gModuleInfoAppleArrayCount == 0) // If nothing is cached...
        {
            // Call ourselves recursively, for the sole purpose of getting the required size and filling the cache.
            // We call GetModuleInfoApple with a NULL filter (get all results). This may result in a required size that's
            // greater than the size needed for the user's possibly supplied filter. Thus we have a variable here 
            // called maxRequiredArraySize.
            
            const size_t maxRequiredArraySize = GetModuleInfoApple(NULL, 0, NULL, false);
            ReallocModuleInfoApple(maxRequiredArraySize); // If the realloc fails, the code below deals with it safely.

            // Call ourselves recursively, for the purpose of filling in the cache.
            GetModuleInfoApple(gModuleInfoAppleArray, gModuleInfoAppleArrayCount, NULL, false); 
        }
        
        // Copy our cache to the user's supplied array, while applying the filter and updating requiredArraySize.
        for(size_t i = 0, iEnd = gModuleInfoAppleArrayCount; i != iEnd; i++)
        {
            const ModuleInfoApple& mia = gModuleInfoAppleArray[i];

            if(!pTypeFilter || strstr(mia.mType, pTypeFilter)) // If the filter matches...
            {
                requiredArraySize++;

                if(arraySize < arrayCapacity) // If there is room in the user-supplied array...
                {
                    ModuleInfoApple& miaUser = pModuleInfoAppleArray[arraySize++];
                    memcpy(&miaUser, &mia, sizeof(ModuleInfoApple));
                }
            }
        }

        if(gCallstackFutex)
            gCallstackFutex->Unlock();
    }
    else
    {
        struct task_dyld_info t_info;
        uint32_t t_info_count = TASK_DYLD_INFO_COUNT;
        kern_return_t kr = task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&t_info, &t_info_count);
        if (kr != KERN_SUCCESS)
        {
            EAT_ASSERT_FORMATTED(false, "GetModuleInfoApple: task_info() returned %d", kr);
            return 0;
        }
        const struct dyld_all_image_infos* pAllImageInfos = (const struct dyld_all_image_infos *)t_info.all_image_info_addr;
        
        for(uint32_t i = 0; i < pAllImageInfos->infoArrayCount; i++)
        {
            const char* pModulePath = pAllImageInfos->infoArray[i].imageFilePath;
            if(pModulePath != NULL && strncmp(pModulePath, "", PATH_MAX) != 0)
            {
                uintptr_t         currentSegmentPos = (uintptr_t)pAllImageInfos->infoArray[i].imageLoadAddress;
                const MachHeader* pMachHeader       = reinterpret_cast<const MachHeader*>(currentSegmentPos);
                EAT_ASSERT(pMachHeader != NULL);
                currentSegmentPos += sizeof(*pMachHeader);
                
                // The system library addresses we obtain are the linker address.
                // So we need to get the get the dynamic loading offset
                // The offset is also stored in pAllImageInfos->sharedCacheSlide, but there is no way
                // to know whether or not it should get used on each image. (dyld and our executable images do not slide)
                // http://lists.apple.com/archives/darwin-kernel/2012/Apr/msg00012.html
                intptr_t offset = _dyld_get_image_vmaddr_slide(i);
                CreateModuleInfoApple(pModuleInfoAppleArray, arrayCapacity, requiredArraySize, arraySize,
                                      pTypeFilter, pModulePath, currentSegmentPos, pMachHeader, offset);
            }
        }
        
        // Iterating on dyld_all_image_infos->infoArray[] does not give us entries for /usr/lib/dyld.
        // We use the mach_header to get /usr/lib/dyld
        const MachHeader* pMachHeader = (const MachHeader*)pAllImageInfos->dyldImageLoadAddress;
        uintptr_t         currentSegmentPos = (uintptr_t)pMachHeader + sizeof(*pMachHeader);
        char modulePath[PATH_MAX] = "";
        pid_t  pid = getpid();
        int filenameLen = proc_regionfilename((int)pid,currentSegmentPos,modulePath,(uint32_t)sizeof(modulePath));
        EAT_ASSERT(filenameLen > 0 && modulePath != NULL && strncmp(modulePath,"",sizeof(modulePath)) != 0);
        if(filenameLen > 0)
        {
            CreateModuleInfoApple(pModuleInfoAppleArray, arrayCapacity, requiredArraySize, arraySize,
                                  pTypeFilter, modulePath, currentSegmentPos, pMachHeader, 0); // offset is 0 because dyld is already loaded
        }
        
        // Use this to compare results
        // printf("vmmap -w %lld", (int64_t)pid);
    }

	return requiredArraySize;
}
#endif // EATHREAD_APPLE_GETMODULEINFO_ENABLED


///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
EATHREADLIB_API void GetInstructionPointer(void*& pInstruction)
{
	pInstruction = __builtin_return_address(0); // Works for all Apple platforms and compilers (gcc and clang).
}


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EATHREADLIB_API void InitCallstack()
{
	EA::Thread::Allocator* pAllocator = EA::Thread::GetAllocator();

	EAT_ASSERT_MSG(pAllocator != NULL, "EA::Thread::SetAllocator needs to be called on app startup.");
	if(pAllocator)
		gCallstackFutex = new(pAllocator->Alloc(sizeof(Futex))) Futex;
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EATHREADLIB_API void ShutdownCallstack()
{
	EA::Thread::Allocator* pAllocator = EA::Thread::GetAllocator();

	EAT_ASSERT_MSG(pAllocator != NULL, "EAThread requires an allocator to be available between InitCallstack and ShutdownCallstack.");
	if(pAllocator)
	{
		if(gModuleInfoAppleArray)
			ReallocModuleInfoApple(0);

		if(gCallstackFutex)
		{
			pAllocator->Free(gCallstackFutex);
			gCallstackFutex = NULL;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// Capture up to nReturnAddressArrayCapacity elements of the call stack, 
// or the whole callstack, whichever is smaller. 
//
// ARM
//      Apple defines a different ABI than the ARM eabi used by Linux and the ABI used
//      by Microsoft. It implements a predictable stack frame system using r7 as the 
//      frame pointer. Documentation:
//          http://developer.apple.com/library/ios/#documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARMv6FunctionCallingConventions.html
//
//      Basically, Apple uses r7 as a frame pointer. So for any function you are
//      executing, r7 + 4 is the LR passed to us by the caller and is the PC of 
//      the parent. And r7 + 0 is a pointer to the parent's r7. 
// x86/x64
//      The ABI is similar except using the different registers from the different CPU.
//
EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
	#if defined(EA_DEBUG)
		memset(pReturnAddressArray, 0, nReturnAddressArrayCapacity * sizeof(void*));
	#endif
	
	#if defined(EA_PROCESSOR_ARM) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
		
		struct StackFrame {
			StackFrame* mpParentStackFrame;
			void*       mpReturnPC;
		};
		
		StackFrame* pStackFrame;
		void*       pInstruction;
		size_t      index = 0;

		if(pContext)
		{
			#if defined(EA_PROCESSOR_ARM32)
				pStackFrame  = (StackFrame*)pContext->mFP;
				pInstruction = (void*)      pContext->mPC;
				#define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0x1) == 0)

			#elif defined(EA_PROCESSOR_ARM64)
				pStackFrame  = (StackFrame*)pContext->mFP;
				pInstruction = (void*)      pContext->mPC;
				#define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 0)
								
			#elif defined(EA_PROCESSOR_X86_64)
				pStackFrame  = (StackFrame*)pContext->mRBP;
				pInstruction = (void*)      pContext->mRIP;
				#define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 0)

			#elif defined(EA_PROCESSOR_X86)
				pStackFrame  = (StackFrame*)pContext->mEBP;
				pInstruction = (void*)      pContext->mEIP;
				#define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 8)
				
			#endif

			// Write the instruction to pReturnAddressArray. In this case we have this thread 
			// reading the callstack from another thread.
			if(index < nReturnAddressArrayCapacity)
				pReturnAddressArray[index++] = pInstruction;
		}
		else // Else get the current values...
		{
			pStackFrame = (StackFrame*)__builtin_frame_address(0);
			GetInstructionPointer(pInstruction); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit if this is Thumb code.

			// Don't write pInstruction to pReturnAddressArray, as pInstruction refers to the code in *this* function, whereas we want to start with caller's call frame.
		}

		// We can do some range validation if we have a pthread id.
		StackFrame* pStackBase;
		StackFrame* pStackLimit;
		const bool  bThreadIsCurrent = (pContext == NULL); // To do: allow this to also tell if the thread is current for the case that pContext is non-NULL. We can do that by reading the current frame address and walking it backwards a few times and seeing if any value matches pStackFrame. 
		
		if(bThreadIsCurrent)
		{
			pthread_t pthread = pthread_self(); // This makes the assumption that the current thread is a pthread and not just a kernel thread.
			pStackBase  = reinterpret_cast<StackFrame*>(pthread_get_stackaddr_np(pthread));
			pStackLimit = pStackBase - (pthread_get_stacksize_np(pthread) / sizeof(StackFrame));
		}
		else
		{   // Make a conservative guess.
			pStackBase  = pStackFrame + ((1024 * 1024) / sizeof(StackFrame));
			pStackLimit = pStackFrame - ((1024 * 1024) / sizeof(StackFrame));
		}

		// To consider: Do some validation of the PC. We can validate it by making sure it's with 20 MB 
		// of our PC and also verify that the instruction before it (be it Thumb or ARM) is a BL or BLX 
		// function call instruction (in the case of EA_PROCESSOR_ARM).
		// To consider: Verify that each successive pStackFrame is at a higher address than the last,
		// as otherwise the data must be corrupt.

		if((index < nReturnAddressArrayCapacity) && pStackFrame && FrameIsAligned(pStackFrame))
		{
			pReturnAddressArray[index++] = pStackFrame->mpReturnPC;  // Should happen to be equal to pContext->mLR.

			while(pStackFrame && pStackFrame->mpReturnPC && (index < nReturnAddressArrayCapacity)) 
			{
				pStackFrame = pStackFrame->mpParentStackFrame;

				if(pStackFrame && FrameIsAligned(pStackFrame) && pStackFrame->mpReturnPC && (pStackFrame < pStackBase) && (pStackFrame > pStackLimit))
					pReturnAddressArray[index++] = pStackFrame->mpReturnPC;
				else
					break;
			}
		}

		return index;

	
	#elif EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE // Mac OS X with GlibC

		// One way to get the callstack of another thread, via signal handling:
		//     https://github.com/albertz/openlierox/blob/0.59/src/common/Debug_GetCallstack.cpp
		
		size_t count = 0;

		// The pContext option is not currently supported.
		if(pContext == NULL) // To do: || pContext refers to this thread.
		{
			count = (size_t)backtrace(pReturnAddressArray, (int)nReturnAddressArrayCapacity);
			if(count > 0)
			{
				--count; // Remove the first entry, because it refers to this function and by design we don't include this function.
				memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
			}
		}
		// else fall through to code that manually reads stack frames?
		
		return count;

	#else
		EA_UNUSED(pReturnAddressArray);
		EA_UNUSED(nReturnAddressArrayCapacity);
		EA_UNUSED(pContext);

		return 0;
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Convert a full Context to a CallstackContext (subset of context).
//
EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
	#if defined(EA_PROCESSOR_X86_64)
		context.mRIP = pContext->Rip;
		context.mRSP = pContext->Rsp;
		context.mRBP = pContext->Rbp;
		
	#elif defined(EA_PROCESSOR_X86)
		context.mEIP = pContext->Eip;
		context.mESP = pContext->Esp;
		context.mEBP = pContext->Ebp;
		
	#elif defined(EA_PROCESSOR_ARM32)
		context.mFP  = pContext->mGpr[7];   // Apple uses R7 for the frame pointer in both ARM and Thumb CPU modes.
		context.mSP  = pContext->mGpr[13];
		context.mLR  = pContext->mGpr[14];
		context.mPC  = pContext->mGpr[15];

	#elif defined(EA_PROCESSOR_ARM64)
		context.mFP  = pContext->mGpr[29];   
		context.mSP  = pContext->mGpr[31]; 
		context.mLR  = pContext->mGpr[30];
		context.mPC  = pContext->mPC;
		
	#else
		EAT_FAIL_MSG("Platform unsupported");
	#endif
}



///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
// Returns the required strlen of pModuleName.
//
EATHREADLIB_API size_t GetModuleFromAddress(const void* pCodeAddress, char* pModuleName, size_t moduleNameCapacity)
{
	if(moduleNameCapacity > 0)
		pModuleName[0] = 0;

#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
	Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo)); // Just memset because dladdr sometimes leaves dli_fname untouched.
	int     result = dladdr(pCodeAddress, &dlInfo);

    if((result != 0) && dlInfo.dli_fname) // It seems that usually this fails.
		return strlcpy(pModuleName, dlInfo.dli_fname, moduleNameCapacity);

	// To do: Make this be dynamically resized as needed.
	const size_t         kCapacity = 64;
	ModuleInfoApple      moduleInfoAppleArray[kCapacity];
	size_t               requiredCapacity = GetModuleInfoApple(moduleInfoAppleArray, kCapacity, "__TEXT", true); // To consider: Make this true (use cache) configurable.
	uint64_t             codeAddress = (uint64_t)(uintptr_t)pCodeAddress;

	if(requiredCapacity > kCapacity)
		requiredCapacity = kCapacity;

	for(size_t i = 0; i < requiredCapacity; i++)
	{
		const ModuleInfoApple& miaUser = moduleInfoAppleArray[i];
		
		if((miaUser.mBaseAddress < codeAddress) && (codeAddress < (miaUser.mBaseAddress + miaUser.mSize)))
			return strlcpy(pModuleName, miaUser.mPath, moduleNameCapacity);
	}
#endif

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* pCodeAddress)
{
#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
	Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo)); // Just memset because dladdr sometimes leaves fields untouched.
	int     result = dladdr(pCodeAddress, &dlInfo);

    if(result != 0)
		return dlInfo.dli_fbase; // Is the object load base the same as the module handle? 

	// Try using GetModuleInfoApple to get the information.
	// To do: Make this be dynamically resized as needed.
    const size_t         kCapacity = 256;
	ModuleInfoApple      moduleInfoAppleArray[kCapacity];
	size_t               requiredCapacity = GetModuleInfoApple(moduleInfoAppleArray, kCapacity, "__TEXT", true); // To consider: Make this true (use cache) configurable.
	uint64_t             codeAddress = (uint64_t)(uintptr_t)pCodeAddress;

	if(requiredCapacity > kCapacity)
		requiredCapacity = kCapacity;

	for(size_t i = 0; i < requiredCapacity; i++)
	{
		ModuleInfoApple& miaUser = moduleInfoAppleArray[i];
		
		if((miaUser.mBaseAddress < codeAddress) && (codeAddress < (miaUser.mBaseAddress + miaUser.mSize)))
			return (ModuleHandle)miaUser.mBaseAddress;
	}
#endif

	return 0;
}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
	// For Apple pthread_t is typedef'd to an internally defined _opaque_pthread_t*.
	
	bool threadIsSelf = (threadId == (intptr_t)EA::Thread::kThreadIdInvalid) || // Due to a specification mistake, this function 
						(threadId == (intptr_t)EA::Thread::kThreadIdCurrent) || // accepts kThreadInvalid to mean current.
						(threadId == (intptr_t)pthread_self());
	
	if(threadIsSelf)
	{
		bool result = true;
		context.mStackBase  = (uintptr_t)GetStackBase();
		context.mStackLimit = (uintptr_t)GetStackLimit();

		#if defined(EA_PROCESSOR_ARM32)
			void* p;
			EAGetInstructionPointer(p);
			context.mPC = (uint32_t)p;
			context.mFP = (uint32_t)__builtin_frame_address(0);  // This data isn't exactly right. We want to return the registers as they 
			context.mSP = (uint32_t)__builtin_frame_address(0);  // are for the caller, not for us. Without doing that we end up reporting 
			context.mLR = (uint32_t)__builtin_return_address(0); // an extra frame (this one) on the top of callstacks.

		#elif defined(EA_PROCESSOR_ARM64)
			void* p;
			EAGetInstructionPointer(p);
			context.mPC = (uint64_t)p;
			context.mFP = (uint64_t)__builtin_frame_address(0);
			context.mSP = (uint64_t)__builtin_frame_address(0);  
			context.mLR = (uint64_t)__builtin_return_address(0); 

		#elif defined(EA_PROCESSOR_X86_64)
			context.mRIP = (uint64_t)__builtin_return_address(0);
			context.mRSP = 0;
			context.mRBP = (uint64_t)__builtin_frame_address(1);

		#elif defined(EA_PROCESSOR_X86)
			context.mEIP = (uint32_t)__builtin_return_address(0);
			context.mESP = 0;
			context.mEBP = (uint32_t)__builtin_frame_address(1);

		#else
			// platform not supported 
			result = false;

		#endif
	   
		return result;
	}
	else
	{
		// Pause the thread, get its state, unpause it. 
		//
		// Question: Is it truly necessary to suspend a thread in Apple platforms in order to read
		// their state? It is usually so for other platforms doing the same kind of thing.
		//
		// Question: Is it dangerous to suspend an arbitrary thread? Often such a thing is dangerous
		// because that other thread might for example have some kernel mutex locked that we need.
		// We'll have to see, as it's a great benefit for us to be able to read callstack contexts.
		// Another solution would be to inject a signal handler into the thread and signal it and 
		// have the handler read context information, if that can be useful. There's example code
		// on the Internet for that.
		// Some documentation:
		//     http://www.linuxselfhelp.com/gnu/machinfo/html_chapter/mach_7.html
		
		mach_port_t   thread = pthread_mach_thread_np((pthread_t)threadId); // Convert pthread_t to kernel thread id.
		kern_return_t result = thread_suspend(thread);
		
		if(result == KERN_SUCCESS)
		{
			#if defined(EA_PROCESSOR_ARM32)                            
				arm_thread_state_t threadState; memset(&threadState, 0, sizeof(threadState));
				mach_msg_type_number_t stateCount = MACHINE_THREAD_STATE_COUNT;
				result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

				context.mFP = threadState.__r[7]; // Apple uses R7 for the frame pointer in both ARM and Thumb CPU modes.
				context.mPC = threadState.__pc;
				context.mSP = threadState.__sp;
				context.mLR = threadState.__lr;        

			#elif defined(EA_PROCESSOR_ARM64)                            
				__darwin_arm_thread_state64 threadState; memset(&threadState, 0, sizeof(threadState));
				mach_msg_type_number_t stateCount = MACHINE_THREAD_STATE_COUNT;
				result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

				context.mFP = threadState.__fp;
				context.mPC = threadState.__pc;
				context.mSP = threadState.__sp;
				context.mLR = threadState.__lr;

			#elif defined(EA_PROCESSOR_X86_64)
				// Note: This is yielding gibberish data for me, despite everything seemingly being done correctly.
							
				x86_thread_state_t     threadState; memset(&threadState, 0, sizeof(threadState));
				mach_msg_type_number_t stateCount  = MACHINE_THREAD_STATE_COUNT;
				result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

				context.mRIP = threadState.uts.ts64.__rip;
				context.mRSP = threadState.uts.ts64.__rsp;
				context.mRBP = threadState.uts.ts64.__rbp;

			#elif defined(EA_PROCESSOR_X86)
				// Note: This is yielding gibberish data for me, despite everything seemingly being done correctly.
							
				x86_thread_state_t     threadState; memset(&threadState, 0, sizeof(threadState));
				mach_msg_type_number_t stateCount  = MACHINE_THREAD_STATE_COUNT;
				result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

				context.mEIP = threadState.uts.ts32.__eip;
				context.mESP = threadState.uts.ts32.__esp;
				context.mEBP = threadState.uts.ts32.__ebp;

			#endif

			thread_resume(thread); 
			return (result == KERN_SUCCESS);
		}
	}
	
	// Not currently implemented for the given platform.
	memset(&context, 0, sizeof(context));
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
	pthread_t pthread = pthread_from_mach_thread_np((mach_port_t)sysThreadId);
	
	return GetCallstackContext(context, (intptr_t)pthread);
}


// To do: Remove the usage of sStackBase for the platforms that it's not needed,
// as can be seen from the logic below. For example Mac OSX probably doesn't need it.
static EA::Thread::ThreadLocalStorage sStackBase;

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* pStackBase)
{
	if(pStackBase)
		sStackBase.SetValue(pStackBase);
	else
	{
		pStackBase = __builtin_frame_address(0);

		if(pStackBase)
			SetStackBase(pStackBase);
		// Else failure; do nothing.
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE)
		void* pBase;
		if(GetPthreadStackInfo(&pBase, NULL))
			return pBase;
	#endif

	// Else we require the user to have set this previously, usually via a call 
	// to SetStackBase() in the start function of this currently executing
	// thread (or main for the main thread).
	return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE)
		void* pLimit;
		if(GetPthreadStackInfo(NULL, &pLimit))
			return pLimit;
	#endif

	// If this fails then we might have an issue where you are using GCC but not 
	// using the GCC standard library glibc. Or maybe glibc doesn't support 
	// __builtin_frame_address on this platform. Or maybe you aren't using GCC but
	// rather a compiler that masquerades as GCC (common situation).
	void* pStack = __builtin_frame_address(0);
	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page.
}


} // namespace Thread
} // namespace EA


