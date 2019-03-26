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


#ifndef __D_ANIMATION_JOINT_ROOT_H__
#define __D_ANIMATION_JOINT_ROOT_H__

#include "dAnimationStdAfx.h"
#include "dAnimationJoint.h"
#include "dAnimationJointSolver.h"

class dAnimationModelManager;

class dAnimationJointRoot: public dAnimationJoint
{
	public:
	dAnimationJointRoot(NewtonBody* const body, const dMatrix& bindMarix);
	virtual ~dAnimationJointRoot();

	void SetCalculateLocalTransforms(bool val);
	bool GetCalculateLocalTransforms() const;

	private:
	void PostUpdate(dAnimationModelManager* const manager, dFloat timestep) const;

	dAnimationJointSolver m_solver;
	dComplementaritySolver::dBodyState m_staticBody;
	bool m_calculateLocalTransform;
	friend class dAnimationModelManager;
};

inline void dAnimationJointRoot::SetCalculateLocalTransforms(bool val) 
{ 
	m_calculateLocalTransform = val; 
}

inline bool dAnimationJointRoot::GetCalculateLocalTransforms() const 
{ 
	return m_calculateLocalTransform; 
}

inline void dAnimationJointRoot::PostUpdate(dAnimationModelManager* const manager, dFloat timestep) const
{
	if (m_calculateLocalTransform) {
		dAnimationJoint::PostUpdate(manager, timestep);
	}
}

#endif 

