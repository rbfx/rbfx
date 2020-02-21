///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <eathread/eathread_callstack.h>
#include <stdlib.h>

#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE)
	#include <pthread.h>
#endif

namespace EA
{
namespace Thread
{

#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_SONY)
	// With some implementations of pthread, the stack base is returned by pthread as NULL if it's the main thread,
	// or possibly if it's a thread you created but didn't call pthread_attr_setstack manually to provide your 
	// own stack. It's impossible for us to tell here whether will be such a NULL return value, so we just do what
	// we can and the user nees to beware that a NULL return value means that the system doesn't provide the 
	// given information for the current thread. This function returns false and sets pBase and pLimit to NULL in 
	// the case that the thread base and limit weren't returned by the system or were returned as NULL.

	#if defined(EA_PLATFORM_APPLE)
		bool GetPthreadStackInfo(void** pBase, void** pLimit)
		{
			pthread_t thread    = pthread_self();
			void*     pBaseTemp = pthread_get_stackaddr_np(thread);
			size_t    stackSize = pthread_get_stacksize_np(thread);

			if(pBase)
				*pBase = pBaseTemp;
			if(pLimit)
			{
				if(pBaseTemp)
					*pLimit = (void*)((size_t)pBaseTemp - stackSize);
				else
					*pLimit = NULL;
			}

			return (pBaseTemp != NULL);
		}

	#elif defined(EA_PLATFORM_SONY)
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
					pBaseTemp   = (void*)((uintptr_t)pLimitTemp + stackSize); // p is returned by pthread_attr_getstack as the lowest address in the stack, and not the stack base.
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
	#else
		bool GetPthreadStackInfo(void** pBase, void** pLimit)
		{
			bool  returnValue = false;
			void* pBaseTemp   = NULL;
			void* pLimitTemp  = NULL;

			pthread_attr_t attr;
			pthread_attr_init(&attr);

			#if defined(EA_PLATFORM_LINUX)
				int result = pthread_getattr_np(pthread_self(), &attr);
			#else
				int result = pthread_attr_get_np(pthread_self(), &attr); // __BSD__ or __FreeBSD__
			#endif

			if(result == 0)
			{
				// The pthread_attr_getstack() function returns the stack address and stack size
				// attributes of the thread attributes object referred to by attr in the buffers
				// pointed to by stackaddr and stacksize, respectively. According to the documentation,
				// the stack address reported is the lowest memory address and not the stack 'base'.
				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_attr_setstack.html
				size_t stackSize;
				result = pthread_attr_getstack(&attr, &pLimitTemp, &stackSize);

				if((result == 0) && (pLimitTemp != NULL)) // If success...
				{
					pBaseTemp   = (void*)((uintptr_t)pLimitTemp + stackSize); // p is returned by pthread_attr_getstack as the lowest address in the stack, and not the stack base.
					returnValue = true;
				}
				else
				{
					pBaseTemp  = NULL;
					pLimitTemp = NULL;
				}
			}

			pthread_attr_destroy(&attr);

			if(pBase)
				*pBase = pBaseTemp;
			if(pLimit)
				*pLimit = pLimitTemp;

			return returnValue;
		}
	#endif
#endif


} // namespace Callstack
} // namespace EA





