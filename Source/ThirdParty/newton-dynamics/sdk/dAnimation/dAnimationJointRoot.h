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

class dAnimationLoopJoint;
class dAnimationModelManager;

class dAnimationLoopJointList: public dList<dAnimationLoopJoint*>
{
};

class dAnimationJointRoot: public dAnimationJoint
{
	public:
	dAnimationJointRoot(NewtonBody* const body, const dMatrix& bindMarix);
	virtual ~dAnimationJointRoot();

	dAnimationLoopJointList& GetLoops();
	const dAnimationLoopJointList& GetLoops() const;

	dAnimationBody* GetStaticWorld();

	void SetCalculateLocalTransforms(bool val);
	bool GetCalculateLocalTransforms() const;

	void Finalize();

	protected:
	virtual void PreUpdate(dFloat timestep) {};
	virtual void PostUpdate(dFloat timestep) {};
	void UpdateTransforms(dFloat timestep) const;

	dAnimationBody m_staticBody;
	dAnimationJointSolver m_solver;
	dAnimationLoopJointList m_loopJoints;

	dAnimationModelManager* m_manager;
	dList<dAnimationJointRoot*>::dListNode* m_managerNode;
	bool m_calculateLocalTransform;
	friend class dAnimationModelManager;
};


inline dAnimationLoopJointList& dAnimationJointRoot::GetLoops()
{
	return m_loopJoints;
}

inline const dAnimationLoopJointList& dAnimationJointRoot::GetLoops() const
{
	return m_loopJoints;
}

inline dAnimationBody* dAnimationJointRoot::GetStaticWorld()
{
	return &m_staticBody;
}

inline void dAnimationJointRoot::SetCalculateLocalTransforms(bool val) 
{ 
	m_calculateLocalTransform = val; 
}

inline bool dAnimationJointRoot::GetCalculateLocalTransforms() const 
{ 
	return m_calculateLocalTransform; 
}



#endif 

