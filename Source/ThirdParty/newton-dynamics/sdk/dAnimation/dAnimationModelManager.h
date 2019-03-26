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


#ifndef __D_ANIM_ID_MANAGER_H__
#define __D_ANIMATION_MODEL_ANAGER_H__

#include "dAnimationStdAfx.h"
//#include "dAnimationJoint.h"
#include "dAnimationJointRoot.h"

#define D_ANIMATION_MODEL_MANAGER	"__dAnimationModelManager__"


class dAnimationModelManager: public dCustomListener
{
	public:
	dAnimationModelManager(NewtonWorld* const world, const char* const name = D_ANIMATION_MODEL_MANAGER);
	virtual ~dAnimationModelManager();

	virtual dAnimationJointRoot* CreateModel(NewtonBody* const bone, const dMatrix& bindMatrix);
	virtual void DestroyModel(dAnimationJointRoot* const model);

	//virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext) = 0;
	virtual void OnPreUpdate(dAnimationJointRoot* const controller, dFloat timestep, int threadIndex) const = 0;
	virtual void OnUpdateTransform(const dAnimationJoint* const bone, const dMatrix& localMatrix) const = 0;

	protected:
	virtual void OnDestroy();
	virtual void PreUpdate(dFloat timestep);
	virtual void PostUpdate(dFloat timestep);

	private:
	dList<dAnimationJointRoot*> m_controllerList;
};


#endif 

