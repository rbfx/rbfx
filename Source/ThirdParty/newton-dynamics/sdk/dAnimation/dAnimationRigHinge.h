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

#ifndef __D_ANIMATION_RIG_HINGE_H__
#define __D_ANIMATION_RIG_HINGE_H__

#include "dAnimationRigLimb.h"


class dAnimationRigHinge: public dAnimationRigLimb, public dCustomHinge
{
	public:
	dAnimationRigHinge(const dMatrix& basicMatrix, dAnimationRigJoint* const parent, NewtonBody* const body);
	virtual ~dAnimationRigHinge();

	void *operator new (size_t size) {return dAnimationRigJoint::Alloc(size);}
	void operator delete (void* ptr) {dAnimationRigJoint::Free(ptr);}

	protected:
	void Debug(dDebugDisplay* const debugDisplay) const;
	virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	virtual void UpdateJointAcceleration();
	virtual void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);

	dComplementaritySolver::dJacobian m_jacobial01;
	dComplementaritySolver::dJacobian m_jacobial10;
	dFloat m_rowAccel;
};

#endif

