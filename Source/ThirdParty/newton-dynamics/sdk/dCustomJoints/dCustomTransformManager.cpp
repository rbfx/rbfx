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

dCustomJoint* dSkeletonBone::GetParentJoint() const
{
	if (m_parent) {
		for (NewtonJoint* joint = NewtonBodyGetFirstJoint(m_body); joint; joint = NewtonBodyGetNextJoint(m_body, joint)) {
			dCustomJoint* const customJoint = (dCustomJoint*)NewtonJointGetUserData(joint);
			dAssert(customJoint);
			if (((customJoint->GetBody0() == m_body) && (customJoint->GetBody1() == m_parent->m_body)) ||
				((customJoint->GetBody1() == m_body) && (customJoint->GetBody0() == m_parent->m_body))) {
				return customJoint;
			}
		}
		dAssert(0);
	}
	return NULL;
}

dSkeletonBone* dCustomTransformController::AddBone(NewtonBody* const boneBody, const dMatrix& bindMatrix, dSkeletonBone* const parentBone)
{
	dAssert(parentBone);
	dSkeletonBone* const bone = &parentBone->Append()->GetInfo();

	bone->m_body = boneBody;
//	bone->m_controller = this;
	bone->m_parent = parentBone;
	bone->m_bindMatrix = bindMatrix;

//	if (m_collisionAggregate) {
//		NewtonCollisionAggregateAddBody(m_collisionAggregate, boneBody);
//	}
	return bone;
}

void dCustomTransformController::PostUpdate(dCustomTransformManager* const manager, dFloat timestep) const
{
	if (m_calculateLocalTransform) {

		dMatrix parentMatrixPool[128];
		const dSkeletonBone* stackPool[128];

		int stack = 1;
		stackPool[0] = this;
		parentMatrixPool[0] = dGetIdentityMatrix();

		while (stack) {
			dMatrix matrix;
			stack--;

			dMatrix parentMatrix(parentMatrixPool[stack]);
			const dSkeletonBone* const bone = stackPool[stack];

			NewtonBodyGetMatrix(bone->GetBody(), &matrix[0][0]);
			manager->OnUpdateTransform(bone, matrix * parentMatrix * bone->GetBindMatrix());

			parentMatrix = matrix.Inverse();
			for (dList<dSkeletonBone>::dListNode* ptrNode = bone->GetFirst(); ptrNode; ptrNode = ptrNode->GetNext()) {
				parentMatrixPool[stack] = parentMatrix;
				stackPool[stack] = &ptrNode->GetInfo();
				stack++;
			}
		}
	}
}


dCustomTransformManager::dCustomTransformManager(NewtonWorld* const world, const char* const name)
	:dCustomListener(world, name)
{
}

dCustomTransformManager::~dCustomTransformManager()
{
}

dCustomTransformController* dCustomTransformManager::CreateController(NewtonBody* const body, const dMatrix& bindMatrix)
{
	dCustomTransformController* const controller = &m_controllerList.Append()->GetInfo();
	controller->m_body = body;
	controller->m_bindMatrix = bindMatrix;
	return controller;
}

void dCustomTransformManager::PreUpdate(dFloat timestep)
{
	for (dList<dCustomTransformController>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		dCustomTransformController* const controller = &node->GetInfo();
		OnPreUpdate(controller, timestep, 0);
	}
}

void dCustomTransformManager::PostUpdate(dFloat timestep)
{
	for (dList<dCustomTransformController>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		dCustomTransformController* const controller = &node->GetInfo();
		controller->PostUpdate(this, timestep);
	}
}

void dCustomTransformManager::OnDestroy()
{
	for (dList<dCustomTransformController>::dListNode* node = m_controllerList.GetFirst(); node; ) {
		dCustomTransformController* const controller = &node->GetInfo();
		node = node->GetNext();
		DestroyController (controller);
	}
}

void dCustomTransformManager::DestroyController (dCustomTransformController* const controller)
{
	dList<dCustomTransformController>::dListNode* const node = m_controllerList.GetNodeFromInfo(*controller);
	m_controllerList.Remove(node);
}

/*

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
*/

