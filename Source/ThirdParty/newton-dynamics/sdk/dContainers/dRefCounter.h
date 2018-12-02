/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#ifndef __DREF_COUNTER_H__
#define __DREF_COUNTER_H__

#include "dContainersAlloc.h"


class dRefCounter: public dContainersAlloc
{
	public:
	DCONTAINERS_API dRefCounter(void);
	DCONTAINERS_API int GetRef() const;
	DCONTAINERS_API int Release();
	DCONTAINERS_API void AddRef() const;

	protected:
	DCONTAINERS_API virtual ~dRefCounter(void);

	private:
	mutable int m_refCount;
};

#endif