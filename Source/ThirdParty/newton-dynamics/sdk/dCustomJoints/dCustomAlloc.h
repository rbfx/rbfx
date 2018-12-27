/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _D_CUSTOM_ALLOC_H_
#define _D_CUSTOM_ALLOC_H_



#ifdef _CUSTOM_JOINTS_STATIC_LIB
	#define CUSTOM_JOINTS_API
#else 
	#ifdef _CUSTOM_JOINTS_BUILD_DLL
        #ifdef WIN32
            #define CUSTOM_JOINTS_API __declspec (dllexport)
        #elif defined(__GNUC__)
            #define CUSTOM_JOINTS_API __attribute__((visibility("default")))
        #endif
	#else
        #ifdef WIN32
            #define CUSTOM_JOINTS_API __declspec (dllimport)
        #else
            #define CUSTOM_JOINTS_API
        #endif
	#endif
#endif


class dCustomScopeLock
{
	public:
	dCustomScopeLock (unsigned* const lock)
		:m_atomicLock(lock)
	{
		while (NewtonAtomicSwap((int*)m_atomicLock, 1)) {
			NewtonYield();
		}
	}
	~dCustomScopeLock()
	{
		NewtonAtomicSwap((int*)m_atomicLock, 0);	
	}

	unsigned* m_atomicLock;
};


class dCustomAlloc  
{
	public:
	CUSTOM_JOINTS_API void *operator new (size_t size);
	CUSTOM_JOINTS_API void operator delete (void* ptr);

	dCustomAlloc()
	{
	}

	virtual ~dCustomAlloc() 
	{
	}
};

#endif
