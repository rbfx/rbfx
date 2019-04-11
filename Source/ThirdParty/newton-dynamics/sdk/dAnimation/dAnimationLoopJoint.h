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

#ifndef __D_ANIMATION_LOOP_JOINT_H__
#define __D_ANIMATION_LOOP_JOINT_H__

#include "dAnimationStdAfx.h"
#include "dAnimationJoint.h"

//class dAnimationJoint;

//class dAnimationLoopJoint: public dCustomAlloc, public dComplementaritySolver::dBilateralJoint
class dAnimationLoopJoint: public dCustomAlloc, public dAnimationContraint
{
	public:
	dAnimationLoopJoint(dAnimationBody* const owner0, dAnimationBody* const owner1);
	virtual ~dAnimationLoopJoint() {}
	bool IsActive() const { return m_isActive; }
	dAnimationBody* GetOwner0() const { return (dAnimationBody*)m_state0;}
	dAnimationBody* GetOwner1() const { return (dAnimationBody*)m_state1;}

	virtual void Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const {}
	virtual int GetMaxDof() const = 0;

//	dAnimationBody* m_owner0;
//	dAnimationBody* m_owner1;
	bool m_isActive;
};

#endif

