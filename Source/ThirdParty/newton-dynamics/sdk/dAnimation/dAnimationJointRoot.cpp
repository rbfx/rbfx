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

#include "dAnimationStdAfx.h"
#include "dAnimationJointRoot.h"


dAnimationJointRoot::dAnimationJointRoot(NewtonBody* const body, const dMatrix& bindMarix)
	:dAnimationJoint(body, bindMarix, NULL)
	,m_solver()
	,m_staticBody()
	,m_calculateLocalTransform(true)
{
}

dAnimationJointRoot::~dAnimationJointRoot()
{
}

