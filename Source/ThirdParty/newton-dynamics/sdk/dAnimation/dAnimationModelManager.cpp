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
#include "dAnimationModelManager.h"


dAnimationModelManager::dAnimationModelManager(NewtonWorld* const world, const char* const name)
	:dCustomListener(world, name)
{
}

dAnimationModelManager::~dAnimationModelManager()
{
}

dAnimationJointRoot* dAnimationModelManager::CreateModel(NewtonBody* const bone, const dMatrix& bindMatrix)
{
	dAnimationJointRoot* const root = new dAnimationJointRoot(bone, bindMatrix);
	m_controllerList.Append(root);
	return root;
}

void dAnimationModelManager::DestroyModel(dAnimationJointRoot* const model)
{
	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node;) {
		if (node->GetInfo() == model) {
			delete node->GetInfo();
			m_controllerList.Remove(node);
			break;
		}
	}
}

void dAnimationModelManager::OnDestroy()
{
	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node;) {
		dAnimationJointRoot* const controller = node->GetInfo();
		node = node->GetNext();
		DestroyModel(controller);
	}
}

void dAnimationModelManager::PreUpdate(dFloat timestep)
{
	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		dAnimationJointRoot* const controller = node->GetInfo();
		OnPreUpdate(controller, timestep, 0);
	}
}

void dAnimationModelManager::PostUpdate(dFloat timestep)
{
	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		dAnimationJointRoot* const controller = node->GetInfo();
		controller->PostUpdate(this, timestep);
	}
}
