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
#include "dAnimIKManager.h"
#include "dAnimIK3dofJoint.h"

dAnimIK3dofJoint::dAnimIK3dofJoint(dAnimIKRigJoint* const parent)
	:dAnimIKRigJoint(parent)
{
}

dAnimIK3dofJoint::~dAnimIK3dofJoint ()
{
}

void dAnimIK3dofJoint::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dAssert (0);
}

