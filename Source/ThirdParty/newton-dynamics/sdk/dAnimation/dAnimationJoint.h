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

#ifndef __D_ANIMATION_JOINT_H__
#define __D_ANIMATION_JOINT_H__

#include "dAnimationStdAfx.h"

class dAnimationJoint;
class dAnimationModelManager;

class dAnimationJointChildren: public dList<dAnimationJoint*>
{
};

class dAnimationJoint: public dCustomAlloc
{
	public:
	dAnimationJoint(NewtonBody* const body, const dMatrix& bindMarix, dAnimationJoint* const parent);
	virtual ~dAnimationJoint();
	
	dAnimationJoint* GetParent() const;
	const dMatrix& GetBindMatrix() const;

	void* GetUserData() const;
	void SetUserData(void* const data);
	
	dAnimationJointChildren& GetChidren();
	const dAnimationJointChildren& GetChidren() const;

	NewtonBody* GetBody() const;
	dCustomJoint* GetJoint() const;
	dComplementaritySolver::dBodyState* GetProxyBody(); 

	protected:
	void PostUpdate(dAnimationModelManager* const manager, dFloat timestep) const;

	dComplementaritySolver::dBodyState m_proxyBody;
	dMatrix m_bindMatrix;
	void* m_userData;
	NewtonBody* m_body;
	dCustomJoint* m_joint;
	dAnimationJoint* m_parent;
	dComplementaritySolver::dBilateralJoint* m_proxyJoint;
	dAnimationJointChildren m_children;
};


inline dAnimationJoint* dAnimationJoint::GetParent() const
{
	return m_parent;
}

inline const dMatrix& dAnimationJoint::GetBindMatrix() const
{ 
	return m_bindMatrix; 
}

inline void* dAnimationJoint::GetUserData() const 
{ 
	return m_userData; 
}

inline void dAnimationJoint::SetUserData(void* const data) 
{ 
	m_userData = data; 
}

inline NewtonBody* dAnimationJoint::GetBody() const
{ 
	return m_body; 
}

inline dCustomJoint* dAnimationJoint::GetJoint() const
{
	return NULL;
}

inline dComplementaritySolver::dBodyState* dAnimationJoint::GetProxyBody()
{
	return &m_proxyBody;
}

inline const dAnimationJointChildren& dAnimationJoint::GetChidren() const
{ 
	return m_children; 
}

inline dAnimationJointChildren& dAnimationJoint::GetChidren()
{
	return m_children; 
}
#endif

