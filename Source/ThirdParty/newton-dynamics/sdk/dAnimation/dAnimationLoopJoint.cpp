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
#include "dAnimationJoint.h"
#include "dAnimationLoopJoint.h"
//#include "dAnimationJoint.h"
//#include "dAnimationLoopJoint.h"

dAnimationLoopJoint::dAnimationLoopJoint(dAnimationBody* const owner0, dAnimationBody* const owner1)
	:dCustomAlloc()
	,dAnimationContraint()
	,m_isActive(true)
{
	Init (owner0, owner1);
}
