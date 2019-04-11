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

class dAnimationBody: public dComplementaritySolver::dBodyState
{
	public:
	dAnimationBody ()
		:dComplementaritySolver::dBodyState()
		,m_owner(NULL)
	{
	}

	dAnimationJoint* m_owner;
};

class dAnimationContraint: public dComplementaritySolver::dBilateralJoint
{
	public:
	dAnimationContraint()
		:dComplementaritySolver::dBilateralJoint()
	{
	}
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
	
	dAnimationJointChildren& GetChildren();
	const dAnimationJointChildren& GetChildren() const;

	NewtonBody* GetBody() const;
	dCustomJoint* GetJoint() const;
	dAnimationBody* GetProxyBody();
	dAnimationContraint* GetProxyJoint();

	void CopyRigidBodyMassToStates();
	protected:
	int GetIndex() const;
	void SetIndex(int index);
	void CopyRigidBodyMassToStatesLow();
	virtual void RigidBodyToStates();
	virtual void ApplyExternalForce(dFloat timestep);

	dAnimationBody m_proxyBody;
	dMatrix m_bindMatrix;
	void* m_userData;
	NewtonBody* m_body;
	dCustomJoint* m_joint;
	dAnimationJoint* m_parent;
	dAnimationContraint* m_proxyJoint;
	dAnimationJointChildren m_children;
	int m_index;

	friend class dAnimationJointSolver;
};


inline int dAnimationJoint::GetIndex() const
{
	return m_index;
}

inline void dAnimationJoint::SetIndex(int index)
{
	m_index = index;
}

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

inline dAnimationBody* dAnimationJoint::GetProxyBody()
{
	return &m_proxyBody;
}

inline dAnimationContraint* dAnimationJoint::GetProxyJoint()
{
	return m_proxyJoint;
}

inline const dAnimationJointChildren& dAnimationJoint::GetChildren() const
{ 
	return m_children; 
}

inline dAnimationJointChildren& dAnimationJoint::GetChildren()
{
	return m_children; 
}



#endif

