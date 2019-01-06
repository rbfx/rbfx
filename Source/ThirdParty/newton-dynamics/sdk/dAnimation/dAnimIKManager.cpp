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
#include "dAnimIKManager.h"

dAnimIKManager::dAnimIKManager(NewtonWorld* const world)
	:dCustomControllerManager<dAnimIKController>(world, D_ANIM_IK_MANAGER)
{
}

dAnimIKManager::~dAnimIKManager()
{
}

/*
dAnimIKController* dAnimIKManager::CreateCharacterRig(NewtonBody* const body, const dMatrix& localFrame)
{
	dAssert(0);
//	dAnimIKController* const rig = CreateController();
//	rig->Init(body, localFrame);
//	return rig;
	return NULL;
}
*/

//dAnimIKController* dAnimIKManager::CreateCharacterRig(NewtonCollision* const chassisShape, const dMatrix& rigFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag)
dAnimIKController* dAnimIKManager::CreateIKController()
{
	dAnimIKController* const rig = CreateController();
//	rig->Init(chassisShape, mass, rigFrame, forceAndTorque, gravityMag);
	return rig;
}

void dAnimIKManager::DestroyController(dAnimIKController* const rig)
{
//	rig->Cleanup();
	dCustomControllerManager<dAnimIKController>::DestroyController(rig);
}

void dAnimIKManager::OnDebug(dCustomJoint::dDebugDisplay* const debugContext)
{
	for (dCustomControllerManager<dAnimIKController>::dListNode* rigNode = GetFirst(); rigNode; rigNode = rigNode->GetNext()) {
		dAnimIKController* const rig = &rigNode->GetInfo();
		rig->Debug(debugContext);
	}
}

