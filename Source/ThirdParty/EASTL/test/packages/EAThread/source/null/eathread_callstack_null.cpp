///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_callstack.h>
#include <eathread/eathread_callstack_context.h>
#include <eathread/eathread_storage.h>
#include <string.h>


#if defined(_MSC_VER)
	#pragma warning(disable: 4172) // returning address of local variable or temporary
#endif


namespace EA
{
namespace Thread
{


EATHREADLIB_API void InitCallstack()
{
}

EATHREADLIB_API void ShutdownCallstack()
{
}

EATHREADLIB_API size_t GetCallstack(void* /*callstack*/[], size_t /*maxDepth*/, const CallstackContext* /*pContext*/)
{
	return 0;
}

EATHREADLIB_API bool GetCallstackContext(CallstackContext& /*context*/, intptr_t /*threadId*/)
{
	return false;
}

EATHREADLIB_API bool GetCallstackContextSysThreadId(CallstackContext& /*context*/, intptr_t /*sysThreadId*/)
{
	return false;
}

EATHREADLIB_API void GetCallstackContext(CallstackContext& context, const Context* /*pContext*/)
{
	memset(&context, 0, sizeof(context));
}

EATHREADLIB_API size_t GetModuleFromAddress(const void* /*pAddress*/, char* /*pModuleFileName*/, size_t /*moduleNameCapacity*/)
{
	return 0;
}

EATHREADLIB_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
	return (ModuleHandle)0;
}


#if EA_THREADS_AVAILABLE
	static EA::Thread::ThreadLocalStorage sStackBase;
#else
	static void* sStackBase;
#endif

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EATHREADLIB_API void SetStackBase(void* pStackBase)
{
	if(pStackBase)
	{
		#if EA_THREADS_AVAILABLE
			sStackBase.SetValue(pStackBase);
		#else
			sStackBase = pStackBase;
		#endif
	}
	else
	{
		pStackBase = GetStackBase();
		SetStackBase(pStackBase);
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EATHREADLIB_API void* GetStackBase()
{
	#if EA_THREADS_AVAILABLE
		void* pStackBase = sStackBase.GetValue();
	#else
		void* pStackBase = sStackBase;
	#endif

	if(!pStackBase)
		pStackBase = (void*)(((uintptr_t)GetStackLimit() + 4095) & ~4095); // Align up to nearest page, as the stack grows downward.

	return pStackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EATHREADLIB_API void* GetStackLimit()
{
	void* pStack = NULL;

	pStack = &pStack;

	return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page, as the stack grows downward.
}

} // namespace Thread
} // namespace EA



