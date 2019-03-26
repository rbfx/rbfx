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

#ifndef __D_ANIMATION_RIG_LIMB_H__
#define __D_ANIMATION_RIG_LIMB_H__

#include "dAnimationStdAfx.h"
#include "dAnimIDRigJoint.h"

class dAnimIDRigEffector;

class dAnimIDRigLimb: public dAnimIDRigJoint, public dComplementaritySolver::dBilateralJoint
{
	public:
	dAnimIDRigLimb(dAnimIDRigJoint* const parent, NewtonBody* const body);
	virtual ~dAnimIDRigLimb();

	virtual NewtonBody* GetNewtonBody() const;
	virtual dAnimIDRigLimb* GetAsRigLimb() { return this; }

	protected:
	virtual void Finalize();
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const;
	virtual int GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	NewtonBody* m_newtonBody;
	dAnimIDRigEffector* m_effector;

	friend class dAnimIDRigEffector;
};

#endif

