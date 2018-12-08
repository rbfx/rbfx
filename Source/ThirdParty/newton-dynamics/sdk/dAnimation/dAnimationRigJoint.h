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

#ifndef __D_ANIMATION_RIG_JOINT_H__
#define __D_ANIMATION_RIG_JOINT_H__

#include "dAnimationStdAfx.h"
#include "dAnimationAcyclicJoint.h"

class dAnimationRigLimb;
class dAnimationCharacterRig;
class dAnimationCharacterRigManager;

class dAnimationRigJoint: public dAnimationAcyclicJoint
{
	public:
	dAnimationRigJoint(dAnimationRigJoint* const parent);
	virtual ~dAnimationRigJoint();

	dAnimationCharacterRig* GetRoot() {return m_root;}
	virtual NewtonBody* GetNewtonBody() const {return NULL;}
	virtual dAnimationRigJoint* GetAsRigJoint() { return this; }

	virtual void Init(NewtonBody* const body);

	protected:
	void RigidBodyToStates();
	void ApplyExternalForce(dFloat timestep) {}
	void UpdateLocalTransforms (dAnimationCharacterRigManager* const manager) const;
	
	
	dAnimationCharacterRig* m_root;
	//static dMatrix m_boneConvertionMatrix;
};

#endif

