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

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomAlloc.h"

#ifdef _NEWTON_BUILD_DLL
	void* operator new (size_t size) 
	{ 
		return NewtonAlloc(int (size));
	}

	void operator delete (void* ptr) 
	{ 
		NewtonFree(ptr);
	}

	BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
			case DLL_PROCESS_ATTACH:
			case DLL_THREAD_ATTACH:
				// check for memory leaks
				#if defined(_DEBUG) && defined(_MSC_VER)
				// Track all memory leaks at the operating system level.
				// make sure no Newton tool or utility leaves leaks behind.
				_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF));
				#endif

			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				break;
		}
		return TRUE;
	}
#endif


void* dCustomAlloc::operator new (size_t size)
{
	return NewtonAlloc(int (size));
}

void dCustomAlloc::operator delete (void* ptr)
{
	NewtonFree(ptr);
}

