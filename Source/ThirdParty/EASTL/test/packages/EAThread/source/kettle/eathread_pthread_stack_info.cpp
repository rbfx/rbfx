///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <eathread/eathread_callstack.h>
#include <stdlib.h>

namespace EA
{
namespace Thread
{
	// With some implementations of pthread_disabled, the stack base is returned by pthread_disabled as NULL if it's the main thread,
	// or possibly if it's a thread you created but didn't call pthread_disabled_attr_setstack manually to provide your 
	// own stack. It's impossible for us to tell here whether will be such a NULL return value, so we just do what
	// we can and the user nees to beware that a NULL return value means that the system doesn't provide the 
	// given information for the current thread. This function returns false and sets pBase and pLimit to NULL in 
	// the case that the thread base and limit weren't returned by the system or were returned as NULL.    
	bool GetPthreadStackInfo(void** pBase, void** pLimit)
	{
		bool  returnValue = false;
		size_t stackSize;
		void* pBaseTemp = NULL;
		void* pLimitTemp  = NULL;

		ScePthreadAttr attr;
			
		scePthreadAttrInit(&attr);

		int result = scePthreadAttrGet(scePthreadSelf(), &attr);
		if(result == 0)  // SCE_OK (=0)
		{
			result = scePthreadAttrGetstack(&attr, &pLimitTemp, &stackSize);
			if((result == 0) && (pLimitTemp != NULL)) // If success...
			{
				pBaseTemp   = (void*)((uintptr_t)pLimitTemp + stackSize); // p is returned by pthread_disabled_attr_getstack as the lowest address in the stack, and not the stack base.
				returnValue = true;
			}
			else
			{
				pBaseTemp  = NULL;
				pLimitTemp = NULL;
			}
		}

		scePthreadAttrDestroy(&attr);

		if(pBase)
			*pBase = pBaseTemp;
		if(pLimit)
			*pLimit = pLimitTemp;

		return returnValue;
	}

} // namespace Callstack
} // namespace EA





