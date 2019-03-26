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

#ifndef __D_ANIM_ID_RIG_JOINT_H__
#define __D_ANIM_ID_RIG_JOINT_H__

#include "dAnimationStdAfx.h"
#include "dAnimAcyclicJoint.h"

class dAnimIDRigLimb;
class dAnimIDManager;
class dAnimIDController;

class dAnimIDRigJoint: public dAnimAcyclicJoint
{
	public:
	dAnimIDRigJoint(dAnimIDRigJoint* const parent);
	virtual ~dAnimIDRigJoint();

	dAnimIDController* GetRoot() {return m_root;}
	virtual NewtonBody* GetNewtonBody() const {return NULL;}
	virtual dAnimIDRigJoint* GetAsRigJoint() { return this; }

	virtual void Init(NewtonBody* const body);

	protected:
	void RigidBodyToStates();
	void ApplyExternalForce(dFloat timestep) {}
	void UpdateLocalTransforms (dAnimIDManager* const manager) const;
	
	dAnimIDController* m_root;
};

#endif

