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

#ifndef _D_NEWTON_SCOPE_BUFFER_H_
#define _D_NEWTON_SCOPE_BUFFER_H_

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"


class dNewtonScopeBufferBase: public dNewtonAlloc  
{
	protected:
	CNEWTON_API dNewtonScopeBufferBase ();
	CNEWTON_API ~dNewtonScopeBufferBase ();
	CNEWTON_API void SetBuffer (int sizeInBytes);
	void* m_ptr;
};


template<class T>
class dNewtonScopeBuffer: public dNewtonScopeBufferBase
{
	public:

	dNewtonScopeBuffer ()
		:dNewtonScopeBufferBase ()
		,m_size(0)
	{
	}

	dNewtonScopeBuffer (int size)
		:dNewtonScopeBufferBase()
		,m_size(0)
	{
		Init (size);
	}

	~dNewtonScopeBuffer ()
	{
	}

	void Init (int size)
	{
		m_size = size;
		SetBuffer (size * sizeof (T));
	}


	int GetSizeInBytes() const
	{
		return m_size * sizeof (T);
	}

	int GetElementsCount() const
	{
		return m_size;
	}


	T& operator[] (int entry)
	{
		dAssert (entry >= 0);
		dAssert ((entry < m_size) || ((m_size == 0) && (entry == 0)));
		return ((T*)m_ptr)[entry];
	}

	const T& operator[] (int entry) const
	{
		dAssert (entry >= 0);
		dAssert ((entry < m_size) || ((m_size == 0) && (entry == 0)));
		return ((T*)m_ptr)[entry];
	}

	private:
	int m_size;
};

#endif
