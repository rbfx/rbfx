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
#include "dAnimIDRigHinge.h"
#include "dAnimIDRigEffector.h"
#include "dAnimIDBlendNodeRoot.h"
#include "dAnimIDManager.h"
#include "dAnimIDController.h"

dAnimIDController::dAnimIDController()
	:dCustomControllerBase()
	,dAnimIDRigJoint(NULL)
	,m_localFrame(dGetIdentityMatrix())
	,m_staticWorld(NULL)
	,m_solver()
	,m_effectors()
	,m_animationTree(NULL)
{
	m_root = this;
	m_staticWorld.SetLoopNode(true);
}

dAnimIDController::~dAnimIDController ()
{
	if (m_animationTree) {
		delete m_animationTree;
	}
}

void* dAnimIDController::GetUserData() const
{
	return dCustomControllerBase::GetUserData();
}

void dAnimIDController::SetUserData(void* const userData)
{
	dAssert(0);
	dAnimIDRigJoint::SetUserData(userData);
	dCustomControllerBase::SetUserData(userData);
}


dAnimIDBlendNodeRoot* dAnimIDController::GetAnimationTree() const
{
	return m_animationTree;
}

void dAnimIDController::SetAnimationTree(dAnimIDBlendNodeRoot* const animTree)
{
	if (m_animationTree) {
		delete m_animationTree;
	}
	m_animationTree = animTree;
}

void dAnimIDController::Init(NewtonBody* const body, const dMatrix& localFrameInGlobalSpace)
{
	dCustomControllerBase::m_body = body;
	dAnimIDRigJoint::Init(body);

	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	m_localFrame = localFrameInGlobalSpace * matrix.Inverse();
}

dMatrix dAnimIDController::GetBasePoseMatrix() const
{
	dMatrix matrix;
	NewtonBodyGetMatrix(GetNewtonBody(), &matrix[0][0]);
	return m_localFrame * matrix;
}

void dAnimIDController::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
//	dAnimIDRigJoint::Debug(debugContext);
	if (m_animationTree) {
		m_animationTree->Debug(debugContext);
	}
}

void dAnimIDController::Finalize()
{
	dAnimIDRigJoint::Finalize();
	m_solver.Finalize(this);
}

NewtonBody* dAnimIDController::GetNewtonBody() const 
{
	return dCustomControllerBase::GetBody();
}

void dAnimIDController::PreUpdate(dFloat timestep, int threadIndex)
{
	RigidBodyToStates();
	if (m_animationTree) {
		m_animationTree->Update(timestep);
	}
	m_solver.Update(timestep);
	UpdateJointAcceleration();
}

void dAnimIDController::PostUpdate(dFloat timestep, int threadIndex)
{
	dAnimIDManager* const manager = (dAnimIDManager*)GetManager();
	UpdateLocalTransforms (manager);
}
