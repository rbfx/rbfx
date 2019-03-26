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

#ifndef __D_ANIM_IK_RIG_JOINT_H__
#define __D_ANIM_IK_RIG_JOINT_H__

#include "dAnimationStdAfx.h"
#include "dAnimAcyclicJoint.h"

class dAnimIKController;

class dAnimIKRigJoint: public dAnimAcyclicJoint
{
	public:
	dAnimIKRigJoint(dAnimIKRigJoint* const parent);
	virtual ~dAnimIKRigJoint();

	dAnimIKController* GetRoot() {return m_root;}
//	virtual NewtonBody* GetNewtonBody() const {return NULL;}
	virtual dAnimIKRigJoint* GetAsIKRigJoint() { return this; }
//	virtual void Init(NewtonBody* const body);

	protected:
//	void RigidBodyToStates();
//	void ApplyExternalForce(dFloat timestep) {}
//	void UpdateLocalTransforms (dAnimIDManager* const manager) const;
	
	dAnimIKController* m_root;
};

#endif

