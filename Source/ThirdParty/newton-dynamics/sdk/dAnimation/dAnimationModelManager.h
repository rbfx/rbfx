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


#ifndef __D_ANIMATION_MODEL_MANAGER_H__
#define __D_ANIMATION_MODEL_MANAGER_H__

#include "dAnimationStdAfx.h"
//#include "dAnimationJoint.h"
#include "dAnimationJointRoot.h"

#define D_ANIMATION_MODEL_MANAGER	"__dAnimationModelManager__"


class dAnimationModelManager: public dCustomListener
{
	public:
	dAnimationModelManager(NewtonWorld* const world, const char* const name = D_ANIMATION_MODEL_MANAGER);
	virtual ~dAnimationModelManager();

	//dAnimationJointRoot* CreateModel(NewtonBody* const bone, const dMatrix& bindMatrix);
	void AddModel(dAnimationJointRoot* const model);
	void RemoveModel(dAnimationJointRoot* const model);

	//virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext) = 0;
	virtual void OnUpdateTransform(const dAnimationJoint* const bone, const dMatrix& localMatrix) const = 0;

	protected:
	virtual void OnDestroy();
	virtual void OnPreUpdate(dAnimationJointRoot* const model, dFloat timestep);
	virtual void OnPostUpdate(dAnimationJointRoot* const model, dFloat timestep) {}

	private:
	virtual void PreUpdate(dFloat timestep);
	virtual void PostUpdate(dFloat timestep);

	static void PreUpdate(NewtonWorld* const world, void* const context, int threadIndex);
	static void PostUpdate(NewtonWorld* const world, void* const context, int threadIndex);

	private:
	dList<dAnimationJointRoot*> m_controllerList;
	dFloat m_timestep;
	//unsigned m_lock;

};


#endif 

