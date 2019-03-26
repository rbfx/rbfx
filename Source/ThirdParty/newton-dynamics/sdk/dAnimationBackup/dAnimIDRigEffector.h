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

#ifndef __D_ANIMATION_RIG_EFFECTOR_H__
#define __D_ANIMATION_RIG_EFFECTOR_H__

#include "dAnimIDRigJoint.h"
#include "dAnimIDRigKinematicLoopJoint.h"

class dAnimIDRigLimb;

class dAnimIDRigEffector: public dAnimIDRigKinematicLoopJoint
{
	public:
	dAnimIDRigEffector(const dMatrix& pivotInGlocalSpace, dAnimIDRigLimb* const parent, dAnimIDRigJoint* const targetBody);
	virtual ~dAnimIDRigEffector();

	virtual int GetMaxDof() const {return 3;}
	virtual void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	virtual void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	void SetLinearSpeed(dFloat speed);
	void SetMaxLinearFriction(dFloat friction);
	dAnimIDRigLimb* GetParent() const { return m_parent;}

	dMatrix GetBasePoseMatrix() const;
	const dMatrix& GetLocalMatrix() const { return m_effectorMatrix;}

	void SetTargetPose(const dMatrix& blobalSpaceMatrix);

	void Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const;

	dMatrix m_localMatrix;
	dMatrix m_targetMatrix;
	dMatrix m_effectorMatrix;
	dAnimIDRigLimb* m_parent;
	NewtonBody* m_referenceBody;
	dFloat m_linearSpeed;
	dFloat m_linearFriction;
};

#endif

