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
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomJoint.h"
#include "dCustomTransformManager.h"


dCustomTransformManager::dCustomTransformManager(NewtonWorld* const world, const char* const name)
	:dCustomControllerManager<dCustomTransformController>(world, name)
{
}

dCustomTransformManager::~dCustomTransformManager()
{
}

dCustomTransformController* dCustomTransformManager::CreateTransformController ()
{
	dCustomTransformController* const controller = (dCustomTransformController*) CreateController();
	controller->Init ();
	return controller;
}

dCustomTransformController::dCustomTransformController()
	:m_collisionAggregate(NULL)
{
}

dCustomTransformController::~dCustomTransformController()
{
	if (m_collisionAggregate) {
		NewtonCollisionAggregateDestroy(m_collisionAggregate);
	}
}

void dCustomTransformController::Init ()
{
	dCustomTransformManager* const manager = (dCustomTransformManager*) GetManager();
	NewtonWorld* const world = manager->GetWorld();

	m_calculateLocalTransform = false;
	m_collisionAggregate = NewtonCollisionAggregateCreate(world);
}

void dCustomTransformController::SetSelfCollision(bool selfCollision)
{
	NewtonCollisionAggregateSetSelfCollision(m_collisionAggregate, selfCollision ? 1 : 0);
}

void dCustomTransformController::PreUpdate(dFloat timestep, int threadIndex)
{
	dCustomTransformManager* const manager = (dCustomTransformManager*) GetManager();
	manager->OnPreUpdate(this, timestep, threadIndex);
}

void dCustomTransformController::PostUpdate(dFloat timestep, int threadIndex)
{
	if (m_calculateLocalTransform && m_bones.GetCount()) {

		dAssert(m_bones.GetCount() == 1);
		dCustomTransformManager* const manager = (dCustomTransformManager*) GetManager();

		dMatrix parentMatrixPool[128];
		dList<dSkeletonBone>::dListNode* stackPool[128];

		int stack = 1;
		stackPool[0] = m_bones.GetFirst();
		parentMatrixPool[0] = dGetIdentityMatrix();

		while (stack) {
			dMatrix matrix;
			stack --;

			dMatrix parentMatrix (parentMatrixPool[stack]);
			dList<dSkeletonBone>::dListNode* const node = stackPool[stack];

			const dSkeletonBone& bone = node->GetInfo();
			NewtonBodyGetMatrix(bone.m_body, &matrix[0][0]);
			manager->OnUpdateTransform (&bone, matrix * parentMatrix * bone.m_bindMatrix);

			parentMatrix = matrix.Inverse();
			for (dList<dSkeletonBone>::dListNode* ptrNode = bone.GetFirst(); ptrNode; ptrNode = ptrNode->GetNext()) {
				parentMatrixPool[stack] = parentMatrix;
				stackPool[stack] = ptrNode;
				stack ++;
			}
		}
	}
}

dCustomTransformController::dSkeletonBone* dCustomTransformController::AddBone (NewtonBody* const boneBody, const dMatrix& bindMatrix, dSkeletonBone* const parentBone)
{
	dSkeletonBone* const bone = parentBone ? &parentBone->Append()->GetInfo() : &m_bones.Append()->GetInfo();

	bone->m_body = boneBody;
	bone->m_controller = this;
	bone->m_parent = parentBone;
	bone->m_bindMatrix = bindMatrix;

	if (m_collisionAggregate) {
		NewtonCollisionAggregateAddBody (m_collisionAggregate, boneBody);
	}
	return bone;
}

dCustomTransformController::dSkeletonBone* dCustomTransformController::AddRoot (NewtonBody* const boneBody, const dMatrix& bindMatrix)
{
	return AddBone (boneBody, bindMatrix, NULL);
}

dCustomTransformController::dSkeletonBone* dCustomTransformController::GetRoot () const
{
	return m_bones.GetCount() ? &m_bones.GetFirst()->GetInfo() : NULL;
}


dCustomJoint* dCustomTransformController::dSkeletonBone::FindJoint() const
{
	if (m_parent) {
		for (NewtonJoint* joint = NewtonBodyGetFirstJoint(m_body); joint; joint = NewtonBodyGetNextJoint(m_body, joint)) {
			dCustomJoint* const customJoint = (dCustomJoint*)NewtonJointGetUserData(joint);
			dAssert (customJoint);
			if (((customJoint->GetBody0() == m_body) && (customJoint->GetBody1() == m_parent->m_body)) ||
				((customJoint->GetBody1() == m_body) && (customJoint->GetBody0() == m_parent->m_body))) {
				return customJoint;
			}
		}
		dAssert (0);
	}
	return NULL;
}