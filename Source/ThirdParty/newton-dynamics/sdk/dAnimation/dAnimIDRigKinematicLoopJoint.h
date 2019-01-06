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

#ifndef __D_KINEMATIC_LOOP_JOINT_H__
#define __D_KINEMATIC_LOOP_JOINT_H__

#include "dAnimationStdAfx.h"

class dAnimAcyclicJoint;

class dAnimIDRigKinematicLoopJoint: public dContainersAlloc, public dComplementaritySolver::dBilateralJoint
{
	public:
	dAnimIDRigKinematicLoopJoint();
	~dAnimIDRigKinematicLoopJoint() {}
	bool IsActive() const { return m_isActive; }
	dAnimAcyclicJoint* GetOwner0() const { return m_owner0; }
	dAnimAcyclicJoint* GetOwner1() const { return m_owner1; }
	void SetOwners(dAnimAcyclicJoint* const owner0, dAnimAcyclicJoint* const owner1);

	virtual void Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const {}
	virtual int GetMaxDof() const = 0;

	dAnimAcyclicJoint* m_owner0;
	dAnimAcyclicJoint* m_owner1;
	bool m_isActive;
};

#endif

