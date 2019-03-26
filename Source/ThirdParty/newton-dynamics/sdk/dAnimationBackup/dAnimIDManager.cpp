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

#include "dAnimationStdAfx.h"
//#include "dAnimIDRigJoint.h"
//#include "dAnimIDManager.h"

#if 0
dAnimIDManager::dAnimIDManager(NewtonWorld* const world)
	:dCustomControllerManager<dAnimIDController>(world, D_ANIM_ID_MANAGER)
{
}

dAnimIDManager::~dAnimIDManager()
{
}

dAnimIDController* dAnimIDManager::CreateCharacterRig(NewtonBody* const body, const dMatrix& localFrame)
{
	dAnimIDController* const rig = CreateController();
	rig->Init(body, localFrame);
	return rig;
}

dAnimIDController* dAnimIDManager::CreateCharacterRig(NewtonCollision* const chassisShape, const dMatrix& rigFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag)
{
//	dAnimationCharacterRig* const rig = CreateController();
//	rig->Init(chassisShape, mass, rigFrame, forceAndTorque, gravityMag);
//	return rig;
	dAssert(0);
	return NULL;
}

void dAnimIDManager::DestroyController(dAnimIDController* const rig)
{
//	rig->Cleanup();
	dCustomControllerManager<dAnimIDController>::DestroyController(rig);
}

void dAnimIDManager::OnDebug(dCustomJoint::dDebugDisplay* const debugContext)
{
	for (dCustomControllerManager<dAnimIDController>::dListNode* rigNode = GetFirst(); rigNode; rigNode = rigNode->GetNext()) {
		dAnimIDController* const rig = &rigNode->GetInfo();
		rig->Debug(debugContext);
	}
}
#endif
