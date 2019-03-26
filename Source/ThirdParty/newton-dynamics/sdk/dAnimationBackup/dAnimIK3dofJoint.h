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


#ifndef __D_ANIM_IK_3DOF_JOINT_H__
#define __D_ANIM_IK_3DOF_JOINT_H__

#include "dAnimationStdAfx.h"
#include "dAnimPose.h"
#include "dAnimIKRigJoint.h"


class dAnimIK3dofJoint: public dAnimIKRigJoint
{
	public:
	dAnimIK3dofJoint(dAnimIKRigJoint* const parent);
	~dAnimIK3dofJoint ();

	protected:
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;
};


#endif 

