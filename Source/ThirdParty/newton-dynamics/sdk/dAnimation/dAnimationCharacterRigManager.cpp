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
#include "dAnimationRigJoint.h"
#include "dAnimationCharacterRigManager.h"

dAnimationCharacterRigManager::dAnimationCharacterRigManager(NewtonWorld* const world)
	:dCustomControllerManager<dAnimationCharacterRig>(world, D_ANINAMTION_CHARACTER_RIG_NAME)
{
}

dAnimationCharacterRigManager::~dAnimationCharacterRigManager()
{
}

dAnimationCharacterRig* dAnimationCharacterRigManager::CreateCharacterRig(NewtonBody* const body, const dMatrix& localFrame)
{
	dAnimationCharacterRig* const rig = CreateController();
	rig->Init(body, localFrame);
	return rig;
}

dAnimationCharacterRig* dAnimationCharacterRigManager::CreateCharacterRig(NewtonCollision* const chassisShape, const dMatrix& rigFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag)
{
//	dAnimationCharacterRig* const rig = CreateController();
//	rig->Init(chassisShape, mass, rigFrame, forceAndTorque, gravityMag);
//	return rig;
	dAssert(0);
	return NULL;
}

void dAnimationCharacterRigManager::DestroyController(dAnimationCharacterRig* const rig)
{
//	rig->Cleanup();
	dCustomControllerManager<dAnimationCharacterRig>::DestroyController(rig);
}

void dAnimationCharacterRigManager::OnDebug(dCustomJoint::dDebugDisplay* const debugContext)
{
	for (dCustomControllerManager<dAnimationCharacterRig>::dListNode* rigNode = GetFirst(); rigNode; rigNode = rigNode->GetNext()) {
		dAnimationCharacterRig* const rig = &rigNode->GetInfo();
		rig->Debug(debugContext);
	}
}

