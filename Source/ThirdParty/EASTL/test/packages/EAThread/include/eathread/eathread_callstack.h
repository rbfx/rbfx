///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#ifndef EATHREAD_EATHREAD_CALLSTACK_H
#define EATHREAD_EATHREAD_CALLSTACK_H

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <stddef.h>

namespace EA
{
	namespace Thread
	{
		/// CallstackContext
		/// 
		/// This is forward-declared here and fully declared at the bottom of this file.
		///
		struct CallstackContext;
		struct Context;


		/// InitCallstack
		///
		/// Allows the user to explicitly initialize the callstack mechanism.
		/// Only the first call to InitCallstack will have effect. Calls to 
		/// InitCallstack must be matched by calls to ShutdownCallstack.
		///
		EATHREADLIB_API void InitCallstack();


		/// ShutdownCallstack
		///
		/// Allows the user to explicitly shutdown the callstack mechanism.
		/// Calls to InitCallstack must be matched by calls to ShutdownCallstack.
		/// The last call to ShutdownCallstack will shutdown and free the callstack mechanism.
		///
		EATHREADLIB_API void ShutdownCallstack();


		/// GetCallstack
		///
		/// Gets the addresses of the calling instructions of a call stack.
		/// If the CallstackContext parameter is used, then that execution context is used;
		/// otherwise the current execution context is used.
		/// The return value is the number of entries written to the callstack array.
		/// The item at callstack[0] is from the function calling the GetCallstack function.
		/// For most platforms the addresses reported are the addresses of the instruction 
		/// that will next be executed upon returning from the function it is calling.
		/// The maxDepth parameter must be at least one and callstack must be able to hold
		/// at least one entry (a terminating 0 NULL entry).
		///
		EATHREADLIB_API size_t GetCallstack(void* callstack[], size_t maxDepth, const CallstackContext* pContext = NULL);


		/// GetCallstack
		///
		/// Gets the callstack based on the thread id as opposed to register context.
		///
		#if defined(EA_PLATFORM_SONY)
			EATHREADLIB_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, EA::Thread::ThreadId& pthread);
		#endif




		#if defined(EA_PLATFORM_MICROSOFT)
			/// Microsoft thread handles are opaque types which are non-unique per thread.
			/// That is, two different thread handles might refer to the same thread.
			/// threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
			/// This is not the same as a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
			EATHREADLIB_API bool ThreadHandlesAreEqual(intptr_t threadId1, intptr_t threadId2);

			/// This function is the same as EA::Thread::GetSysThreadId(ThreadId id).
			/// This function converts from one type of Microsoft thread identifier to another.
			/// threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
			/// The return value is a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
			/// Upon failure, the return value will be zero.
			EATHREADLIB_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId);
		#endif


		/// GetCallstackContext
		///
		/// Gets the CallstackContext associated with the given thread.
		/// The thread must be in a non-running state.
		/// If the threadID is EAThread::kThreadIdInvalid, the current thread context is retrieved.
		/// However, it's of little use to get the context of the current thread, since upon return
		/// from the GetCallstackContext the data will not apply to the current thread any more;
		/// thus this information is probably useful only for diagnostic purposes.
		/// The threadId parameter is the same type as an EAThread ThreadId. It is important to 
		/// note that an EAThread ThreadId under Microsoft platforms is a thread handle and not what 
		/// Microsoft calls a thread id. This is by design as Microsoft thread ids are second class
		/// citizens and likely wouldn't exist if it not were for quirks in the Windows API evolution.
		///
		/// Note that threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
		/// This is not the same as a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
		///
		/// EACallstack has a general struct for each CPU type called Context, defined in EACallstack/Context.h. 
		/// The Context struct contains the entire CPU register context information. In order to walk a thread's 
		/// callstack, you really need only two or three of the register values from the Context. So there is a 
		/// mini struct called CallstackContext which is just those registers needed to read a thread's callstack.
		///
		// ThreadId constants
		#if EA_USE_CPP11_CONCURRENCY
			EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, EA::Thread::ThreadId threadId);
		#else
			EATHREADLIB_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId = 0);
		#endif


		/// GetCallstackContextSysThreadId
		///
		/// This is the same as GetCallstackContext, except it uses what EAThread calls SysThreadId.
		/// On Microsoft platforms a SysThreadId is a "thread id" whereas ThreadId is "thread handle."
		/// On non-Microsoft platforms a SysThreadId is defined to be the same as ThreadId and is often
		/// just an integer or opaque identifier (e.g. pthread).
		/// This function exists because it may be more convenient to work with SysThreadIds in some cases.
		/// You can convert from a ThreadId (Microsoft thread handle) to a SysThreadId (Microsoft thread id)
		/// with the GetThreadIdFromThreadHandle function.
		EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId = 0);


		/// GetCallstackContext
		///
		/// Gets the CallstackContext from a full Context struct. Note that the Context struct
		/// defines the entire machine context, whereas the CallstackContext is a tiny struct
		/// with just a couple integer members and is all that's needed to describe a callstack.
		///
		EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* pContext = NULL);


		/// GetModuleFromAddress
		///
		/// Given an address, this function tells what module it comes from. 
		/// The primary use of this is to tell what DLL an instruction pointer comes from.
		/// Returns the required strlen of the pModuleFileName. If the return value is >= moduleNameCapacity,
		/// there wasn't enough space. pModuleFileName is written with as many characters as possible
		/// and will always be zero terminated. moduleNameCapacity must be at least one.
		///
		EATHREADLIB_API size_t GetModuleFromAddress(const void* pAddress, char* pModuleFileName, size_t moduleNameCapacity);


		/// ModuleHandle
		/// This is a runtime module identifier. For Microsoft Windows-like platforms
		/// this is the same thing as HMODULE. For other platforms it is a shared library
		/// runtime library pointer, id, or handle. For Microsoft platforms, each running
		/// DLL has a module handle.
		#if defined(EA_PLATFORM_MICROSOFT)
			typedef void*            ModuleHandle;  // HMODULE, from LoadLibrary()
		#elif defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE)
			typedef void*            ModuleHandle;  // void*, from dlopen()
		#else
			typedef uintptr_t        ModuleHandle;
		#endif


		/// GetModuleHandleFromAddress
		///
		/// Returns the module handle from a code address.
		/// Returns 0/NULL if no associated module could be found.
		///
		EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress);


		/// EAGetInstructionPointer
		///
		/// Returns the current instruction pointer (a.k.a. program counter).
		/// This function is implemented as a macro, it acts as if its declaration 
		/// were like so:
		///     void EAGetInstructionPointer(void*& p);
		///
		/// For portability, this function should only be used as a standalone 
		/// statement on its own line.
		///
		/// Example usage:
		///    void* pInstruction;
		///    EAGetInstructionPointer(pInstruction);
		///
		#if defined(_MSC_VER) && defined(EA_PROCESSOR_X86)
			// We implement this via calling the next line of code as a function.
			// Then we continue as if we were exiting that function but with no
			// return statement. The result is that the instruction pointer will
			// be placed on the stack and we merely pop it off the stack and 
			// into a local variable.
			#define EAGetInstructionPointer(p)   \
			{                                    \
				uintptr_t eip;                   \
				__asm {                          \
					__asm call GetEIP            \
					__asm GetEIP:                \
					__asm pop eip                \
				}                                \
				p = (void*)eip;                  \
			}

			EA_DISABLE_VC_WARNING(4740) 
			inline void GetInstructionPointer(void*& p) 
				{EAGetInstructionPointer(p);}
			EA_RESTORE_VC_WARNING()

		#elif defined(_MSC_VER) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM))

			EATHREADLIB_API EA_NO_INLINE void GetInstructionPointer(void*& p);

			#define EAGetInstructionPointer(p) EA::Thread::GetInstructionPointer(p)

		#elif defined(__ARMCC_VERSION) // ARM compiler

			// Even if there are compiler intrinsics that let you get the instruction pointer, 
			// this function can still be useful. For example, on ARM platforms this function
			// returns the address with the 'thumb bit' set if it's thumb code. We need this info sometimes.
			EATHREADLIB_API void GetInstructionPointer(void*& p);

			// The ARM compiler provides a __current_pc() instrinsic, which returns an unsigned integer type.
			#define EAGetInstructionPointer(p) { uintptr_t pc = (uintptr_t)__current_pc(); p = reinterpret_cast<void*>(pc); }

		//#elif defined(EA_COMPILER_CLANG) // Disabled until implemented. The GCC code below works under clang, though it wouldn't if compiler extensions were disabled.
		//    EATHREADLIB_API void GetInstructionPointer(void*& p);
		//
		//    // To do: implement this directly instead of via a call to GetInstructionPointer.
		//    #define EAGetInstructionPointer(p) EA::Thread::GetInstructionPointer(p)
			
		#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG) // This covers EA_PLATFORM_UNIX, EA_PLATFORM_OSX 

			// Even if there are compiler intrinsics that let you get the instruction pointer, 
			// this function can still be useful. For example, on ARM platforms this function
			// returns the address with the 'thumb bit' set if it's thumb code. We need this info sometimes.
			EATHREADLIB_API void GetInstructionPointer(void*& p) __attribute__((noinline));

			// It turns out that GCC has an extension that allows you to take the address 
			// of a label. The code here looks a little wacky, but that's how it's done.
			// Basically, this generates a global variable called 'label' and the assignment
			// to 'p' reads that variable into p. One possible downside to this technique is
			// that it relies on registers and global memory not being corrupted, yet one of
			// reasons why we might want to be getting the instruction pointer is in dealing
			// with some sort or processor exception which may be due to memory corruption.
			// To consider: Make a version of this which calculates the value dynamically via asm.
			#define EAGetInstructionPointer(p) EA::Thread::GetInstructionPointer(p)
		#else
			#error
		#endif


		/// EASetStackBase / SetStackBase / GetStackBase / GetStackLimit
		///
		/// EASetStackBase as a macro and acts as if its declaration were like so:
		///     void EASetStackBase();
		/// 
		/// EASetStackBase sets the current stack pointer as the bottom (beginning)
		/// of the stack. Depending on the platform, the "bottom" may be up or down
		/// depending on whether the stack grows upward or downward (usually it grows
		/// downward and so "bottom" actually refers to an address that is above child
		/// stack frames in memory.
		/// This function is intended to be called on application startup as early as 
		/// possible, and in each created thread, as early as possible. Its purpose 
		/// is to record the beginning stack pointer because the platform doesn't provide
		/// APIs to tell what it is, and we need to know it (e.g. so we don't overrun
		/// it during stack unwinds). 
		///
		/// For portability, EASetStackBase should be used only as a standalone 
		/// statement on its own line, as it may include statements that can't work otherwise.
		///
		/// Example usage:
		///    int main(int argc, char** argv) {
		///       EASetStackBase();
		///       . . .
		///    }
		///
		/// SetStackBase is a function which lets you explicitly set a stack bottom instead
		/// of doing it automatically with EASetStackBase. If you pass NULL for pStackBase
		/// then the function uses its stack location during its execution, which will be 
		/// a little less optimal than calling EASetStackBase.
		///
		/// GetStackBase returns the stack bottom set by EASetStackBase or SetStackBase.
		/// It returns NULL if no stack bottom was set or could be set.
		///
		/// GetStackLimit returns the current stack "top", which will be lower than the stack
		/// bottom in memory if the platform grows its stack downward.

		EATHREADLIB_API void  SetStackBase(void* pStackBase);
		inline          void  SetStackBase(uintptr_t pStackBase){ SetStackBase((void*)pStackBase); }
		EATHREADLIB_API void* GetStackBase();
		EATHREADLIB_API void* GetStackLimit();


		#if defined(_MSC_VER) && defined(EA_PROCESSOR_X86)
			#define EASetStackBase()               \
			{                                      \
				void* esp;                         \
				__asm { mov esp, ESP }             \
				::EA::Thread::SetStackBase(esp);   \
			}                               

		#elif defined(_MSC_VER) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM))
			// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
			#define EASetStackBase()                     \
			{                                            \
				::EA::Thread::SetStackBase((void*)NULL); \
			}                                            \

		#elif defined(__ARMCC_VERSION)          // ARM compiler

			#define EASetStackBase()  \
				::EA::Thread::SetStackBase((void*)__current_sp())

		#elif defined(__GNUC__) // This covers EA_PLATFORM_UNIX, EA_PLATFORM_OSX

			#define EASetStackBase()  \
				::EA::Thread::SetStackBase((void*)__builtin_frame_address(0));

		#else
			// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
			#define EASetStackBase()                     \
			{                                            \
				::EA::Thread::SetStackBase((void*)NULL); \
			}                                            \

		#endif

		#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_SONY)
			// GetPthreadStackInfo
			//
			// With some implementations of pthread, the stack base is returned by pthread as NULL if it's the main thread,
			// or possibly if it's a thread you created but didn't call pthread_attr_setstack manually to provide your 
			// own stack. It's impossible for us to tell here whether will be such a NULL return value, so we just do what
			// we can and the user nees to beware that a NULL return value means that the system doesn't provide the 
			// given information for the current thread. This function returns false and sets pBase and pLimit to NULL in 
			// the case that the thread base and limit weren't returned by the system or were returned as NULL.

			bool GetPthreadStackInfo(void** pBase, void** pLimit);
		#endif

	} // namespace Thread

} // namespace EA


#endif // Header include guard.



