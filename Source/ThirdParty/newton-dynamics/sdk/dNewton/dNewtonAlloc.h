/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_ALLOC_H_
#define _D_NEWTON_ALLOC_H_

#include "dStdAfxNewton.h"


//typedef void* (*CNewtonAllocMemory) (int sizeInBytes);
//typedef void (*CNewtonFreeMemory) (void* const ptr, int sizeInBytes);


class dNewtonAlloc  
{
	public:
	CNEWTON_API void *operator new (size_t size);
	CNEWTON_API void operator delete (void* ptr);

	CNEWTON_API dNewtonAlloc()
	{
	}

	CNEWTON_API virtual ~dNewtonAlloc() 
	{
	}

	// default memory allocation functions if not other is provided 
	//CNEWTON_API static void SetAllocationDrivers (CNewtonAllocMemory alloc, CNewtonFreeMemory free);
};

#endif
