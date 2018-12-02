
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

#ifndef __DCLASS_INFO_H__
#define __DCLASS_INFO_H__

#include "dRtti.h"
#include "dRefCounter.h"

class dClassInfo: public dRefCounter
{
	public:
	dClassInfo(void)
	{
	}
	virtual DCONTAINERS_API  ~dClassInfo()
	{
	}

	dRttiRootClassSupportDeclare(dClassInfo,DCONTAINERS_API);
};

#endif