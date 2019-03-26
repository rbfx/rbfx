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
#define __D_ANIM_ID_MANAGER_H__

#include "dAnimationStdAfx.h"
#include "dAnimIDRigJoint.h"
#include "dAnimAcyclicSolver.h"
#include "dAnimIDController.h"

#define D_ANIM_ID_MANAGER	"__dAnimIDManager__"

class dAnimIDManager: public dCustomControllerManager<dAnimIDController>
{
	public:
	dAnimIDManager(NewtonWorld* const world);
	virtual ~dAnimIDManager();

	virtual dAnimIDController* CreateCharacterRig(NewtonBody* const body, const dMatrix& localFrame);
	virtual dAnimIDController* CreateCharacterRig(NewtonCollision* const chassisShape, const dMatrix& vehicleFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);

	virtual void DestroyController(dAnimIDController* const controller);
	protected:
	virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext);
	virtual void OnUpdateTransform (const dAnimIDRigJoint* const bone, const dMatrix& localMatrix) const{}	

	friend class dAnimIDRigJoint;
	friend class dAnimIDController;
};
#endif 

