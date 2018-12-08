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
#include "dAnimationRigHinge.h"
#include "dAnimationRigEffector.h"
#include "dAnimationCharacterRig.h"
#include "dAnimationEffectorBlendRoot.h"
#include "dAnimationCharacterRigManager.h"

dAnimationCharacterRig::dAnimationCharacterRig()
	:dCustomControllerBase()
	,dAnimationRigJoint(NULL)
	,m_localFrame(dGetIdentityMatrix())
	,m_staticWorld(NULL)
	,m_solver()
	,m_effectors()
	,m_animationTree(NULL)
{
	m_root = this;
	m_staticWorld.SetLoopNode(true);
}

dAnimationCharacterRig::~dAnimationCharacterRig ()
{
	if (m_animationTree) {
		delete m_animationTree;
	}
}

dAnimationEffectorBlendRoot* dAnimationCharacterRig::GetAnimationTree() const
{
	return m_animationTree;
}

void dAnimationCharacterRig::SetAnimationTree(dAnimationEffectorBlendRoot* const animTree)
{
	if (m_animationTree) {
		delete m_animationTree;
	}
	m_animationTree = animTree;
}

void dAnimationCharacterRig::Init(NewtonBody* const body, const dMatrix& localFrameInGlobalSpace)
{
	dCustomControllerBase::m_body = body;
	dAnimationRigJoint::Init(body);

	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	m_localFrame = localFrameInGlobalSpace * matrix.Inverse();
}

dMatrix dAnimationCharacterRig::GetBasePoseMatrix() const
{
	dMatrix matrix;
	NewtonBodyGetMatrix(GetNewtonBody(), &matrix[0][0]);
	return m_localFrame * matrix;
}

void dAnimationCharacterRig::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
//	dAnimationRigJoint::Debug(debugContext);
	if (m_animationTree) {
		m_animationTree->Debug(debugContext);
	}
}

void dAnimationCharacterRig::Finalize()
{
	dAnimationRigJoint::Finalize();
	m_solver.Finalize(this);
}

NewtonBody* dAnimationCharacterRig::GetNewtonBody() const 
{
	return dCustomControllerBase::GetBody();
}

void dAnimationCharacterRig::PreUpdate(dFloat timestep, int threadIndex)
{
	RigidBodyToStates();
	if (m_animationTree) {
		m_animationTree->Update(timestep);
	}
	m_solver.Update(timestep);
	UpdateJointAcceleration();
}

void dAnimationCharacterRig::PostUpdate(dFloat timestep, int threadIndex)
{
	dAnimationCharacterRigManager* const manager = (dAnimationCharacterRigManager*)GetManager();
	UpdateLocalTransforms (manager);
}
