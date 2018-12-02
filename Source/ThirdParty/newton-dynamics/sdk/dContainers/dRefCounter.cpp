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

#include "dContainersStdAfx.h"
#include "dRefCounter.h"

dRefCounter::dRefCounter(void)
	:dContainersAlloc()
{
	m_refCount = 1;
}

dRefCounter::~dRefCounter(void)
{
}


int dRefCounter::GetRef() const
{
	return m_refCount;
}

int dRefCounter::Release()
{
	m_refCount --;
	dAssert (m_refCount >= 0);
	if (!m_refCount) {
		delete this;
		return 0;
	}
	return m_refCount;
}

void dRefCounter::AddRef() const
{
	m_refCount ++;
}

