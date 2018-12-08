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
#include "dAnimationRigJoint.h"

class dAnimationRigEffector;

class dAnimationRigLimb: public dAnimationRigJoint, public dComplementaritySolver::dBilateralJoint
{
	public:
	dAnimationRigLimb(dAnimationRigJoint* const parent, NewtonBody* const body);
	virtual ~dAnimationRigLimb();

	virtual NewtonBody* GetNewtonBody() const;
	virtual dAnimationRigLimb* GetAsRigLimb() { return this; }

	protected:
	virtual void Finalize();
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const;
	virtual int GetKinematicLoops(dAnimationKinematicLoopJoint** const jointArray);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	NewtonBody* m_newtonBody;
	dAnimationRigEffector* m_effector;

	friend class dAnimationRigEffector;
};

#endif

