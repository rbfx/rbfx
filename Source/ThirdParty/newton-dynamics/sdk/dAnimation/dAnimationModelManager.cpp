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
	,m_controllerList()
	,m_timestep(0.0f)
{
}

dAnimationModelManager::~dAnimationModelManager()
{
}

void dAnimationModelManager::AddModel(dAnimationJointRoot* const model)
{
	dAssert(!model->m_managerNode);
	model->m_manager = this;
	model->m_managerNode = m_controllerList.Append(model);
}

void dAnimationModelManager::RemoveModel(dAnimationJointRoot* const model)
{
	dAssert(model->m_managerNode);
	dAssert(model->m_manager == this);
	dAssert(model->m_managerNode->GetInfo() == model);
	m_controllerList.Remove(model->m_managerNode);
	model->m_manager = NULL;
	model->m_managerNode = NULL;
}

void dAnimationModelManager::OnDestroy()
{
	while (m_controllerList.GetFirst()) {
		dAnimationJointRoot* const model = m_controllerList.GetFirst()->GetInfo();
		dAssert(model->m_managerNode == m_controllerList.GetFirst());
		delete model;
	}
}

void dAnimationModelManager::OnPreUpdate(dAnimationJointRoot* const model, dFloat timestep)
{
	D_TRACKTIME();
	model->PreUpdate(timestep);
}

void dAnimationModelManager::PreUpdate(NewtonWorld* const world, void* const context, int threadIndex)
{
	D_TRACKTIME();
	dAnimationJointRoot* const model = (dAnimationJointRoot*)context;
	dAnimationModelManager* const me = model->m_manager;
	me->OnPreUpdate(model, me->m_timestep);
}

void dAnimationModelManager::PostUpdate(NewtonWorld* const world, void* const context, int threadIndex)
{
	D_TRACKTIME();
	dAnimationJointRoot* const model = (dAnimationJointRoot*)context;
	dAnimationModelManager* const me = model->m_manager;
	me->OnPostUpdate(model, me->m_timestep);
	model->UpdateTransforms(me->m_timestep);
}

void dAnimationModelManager::PreUpdate(dFloat timestep)
{
	D_TRACKTIME();
//	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
//		dAnimationJointRoot* const model = node->GetInfo();
//		model->PreUpdate(this, timestep);
//	}

	m_timestep = timestep;
	NewtonWorld* const world = GetWorld();

	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		NewtonDispachThreadJob(world, PreUpdate, node->GetInfo(), "dAnimationModelManager");
	}
	NewtonSyncThreadJobs(world);
}

void dAnimationModelManager::PostUpdate(dFloat timestep)
{
	D_TRACKTIME();
//	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
//		dAnimationJointRoot* const model = node->GetInfo();
//		model->PostUpdate(this, timestep);
//	}

	m_timestep = timestep;
	NewtonWorld* const world = GetWorld();

	for (dList<dAnimationJointRoot*>::dListNode* node = m_controllerList.GetFirst(); node; node = node->GetNext()) {
		NewtonDispachThreadJob(world, PostUpdate, node->GetInfo(), "dAnimationModelManager");
	}
	NewtonSyncThreadJobs(world);
}
