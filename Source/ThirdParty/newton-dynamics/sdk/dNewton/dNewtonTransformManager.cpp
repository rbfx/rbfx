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

//////////////////////////////////////////////////////////////////////
#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonBody.h"
#include "dNewtonTransformManager.h"


dNewtonTransformManager::dNewtonTransformManager (dNewton* const world)
	:dCustomControllerManager<dNewtonTransformController>(world->GetNewton(), "__dNewton_transformManager__")
{
}

dNewtonTransformManager::~dNewtonTransformManager()
{
}


void dNewtonTransformManager::PostUpdate (dFloat timestep)
{
	NewtonWorld* const world = GetWorld();
	for (NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		NewtonDispachThreadJob (world, UpdateTransformKernel, body, "UpdateTransformKernel");
	}
	NewtonSyncThreadJobs(world);
}

void dNewtonTransformManager::UpdateTransformKernel (NewtonWorld* const world, void* const context, int threadIndex)
{
	NewtonBody* const body	= (NewtonBody*) context;
	dNewtonBody* const dBody = (dNewtonBody*) NewtonBodyGetUserData(body);

	const dNewtonBody* const dParent = dBody->GetParent();
	if (dParent) {
		NewtonBody* const parent = dParent->GetNewtonBody();
		if (!(NewtonBodyGetSleepState(body) & NewtonBodyGetSleepState(parent))) {
			dMatrix parentMatrix;
			dMatrix childMatrix;
			NewtonBodyGetMatrix(body, &childMatrix[0][0]);
			NewtonBodyGetMatrix(parent, &parentMatrix[0][0]);
			childMatrix = childMatrix * parentMatrix.Inverse();

			dBody->OnBodyTransform (&childMatrix[0][0], threadIndex);
		}
	} else {
		
		if (!NewtonBodyGetSleepState(body)) {
			dMatrix matrix;
			NewtonBodyGetMatrix(body, &matrix[0][0]);
			dBody->OnBodyTransform (&matrix[0][0], threadIndex);
		}
	}
}

