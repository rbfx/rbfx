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
#include "dAnimIKController.h"
#include "dAnimIKBlendNodeRoot.h"

dAnimIKController::dAnimIKController()
	:dCustomControllerBase()
	,dAnimIKRigJoint(NULL)
	,m_basePose()
//	,dAnimIDRigJoint(NULL)
//	,m_localFrame(dGetIdentityMatrix())
//	,m_staticWorld(NULL)
//	,m_solver()
//	,m_effectors()
	,m_animationTree(NULL)
{
	m_root = this;
//	m_staticWorld.SetLoopNode(true);
}

dAnimIKController::~dAnimIKController ()
{
	if (m_animationTree) {
		delete m_animationTree;
	}
}

void* dAnimIKController::GetUserData() const
{
	return dCustomControllerBase::GetUserData();
}

void dAnimIKController::SetUserData(void* const userData)
{
	dCustomControllerBase::SetUserData(userData);
	dAnimIKRigJoint::SetUserData(userData);
}

dAnimIKBlendNodeRoot* dAnimIKController::GetAnimationTree() const
{
	return m_animationTree;
}

void dAnimIKController::SetAnimationTree(dAnimIKBlendNodeRoot* const animTree)
{
	if (m_animationTree) {
		delete m_animationTree;
	}
	m_animationTree = animTree;
}

/*
dAnimationEffectorBlendRoot* dAnimIKController::GetAnimationTree() const
{
	return m_animationTree;
}

dMatrix dAnimIKController::GetBasePoseMatrix() const
{
	dMatrix matrix;
	NewtonBodyGetMatrix(GetNewtonBody(), &matrix[0][0]);
	return m_localFrame * matrix;
}

void dAnimIKController::Finalize()
{
	dAnimIDRigJoint::Finalize();
	m_solver.Finalize(this);
}

NewtonBody* dAnimIKController::GetNewtonBody() const 
{
	return dCustomControllerBase::GetBody();
}

*/


void dAnimIKController::Init(const dMatrix& localFrameInGlobalSpace)
{
	dAssert(0);
//	dCustomControllerBase::m_body = body;
//	dAnimIDRigJoint::Init(body);
//	dMatrix matrix;
//	NewtonBodyGetMatrix(body, &matrix[0][0]);
//	m_localFrame = localFrameInGlobalSpace * matrix.Inverse();
}

void dAnimIKController::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dAssert(0);
	//	dAnimIDRigJoint::Debug(debugContext);
//	if (m_animationTree) {
//		m_animationTree->Debug(debugContext);
//	}
}


void dAnimIKController::PreUpdate(dFloat timestep, int threadIndex)
{
//	dAssert(0);
//	RigidBodyToStates();
	if (m_animationTree) {
		m_animationTree->Update(timestep);
	}
//	m_solver.Update(timestep);
//	UpdateJointAcceleration();
	dAnimIKManager* const manager = (dAnimIKManager*)GetManager();
	manager->UpdatePlayer(this, timestep);
}

void dAnimIKController::PostUpdate(dFloat timestep, int threadIndex)
{
//	dAssert(0);
//	dAnimationInverseDynamicsManager* const manager = (dAnimationInverseDynamicsManager*)GetManager();
//	UpdateLocalTransforms(manager);
}
