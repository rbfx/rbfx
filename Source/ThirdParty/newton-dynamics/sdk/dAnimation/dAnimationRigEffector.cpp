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
#include "dAnimationRigLimb.h"
#include "dAnimationRigEffector.h"
#include "dAnimationCharacterRigManager.h"


dAnimationRigEffector::dAnimationRigEffector(const dMatrix& pivotInGlocalSpace, dAnimationRigLimb* const parent, dAnimationRigJoint* const targetBody)
	:dAnimationKinematicLoopJoint()
	,m_localMatrix(dGetIdentityMatrix())
	,m_targetMatrix(dGetIdentityMatrix())
	,m_effectorMatrix(dGetIdentityMatrix())
	,m_parent(parent)
	,m_referenceBody(NULL)
	,m_linearSpeed(1.0f)
	,m_linearFriction(10000.0f)
{
	m_isActive = true;
	dAssert (!parent->m_effector);
	parent->m_effector = this;
	dAnimationCharacterRig* const root = parent->GetRoot();
	root->m_effectors.Append(this);

	dAssert(parent != targetBody);
	Init(parent->GetProxyBody(), targetBody->GetProxyBody());
	SetOwners(parent, targetBody);

	NewtonBody* const newtonBody = parent->GetNewtonBody();
	dAssert (parent->GetNewtonBody());
	NewtonBodyGetMatrix(newtonBody, &m_localMatrix[0][0]);
	m_localMatrix = pivotInGlocalSpace * m_localMatrix.Inverse();
	m_targetMatrix = root->GetBasePoseMatrix();
	m_effectorMatrix.m_posit = m_targetMatrix.UntransformVector(pivotInGlocalSpace.m_posit);

	m_targetMatrix.m_posit = pivotInGlocalSpace.m_posit;
}

dAnimationRigEffector::~dAnimationRigEffector()
{
}

dMatrix dAnimationRigEffector::GetBasePoseMatrix() const
{
	dAnimationCharacterRig* const root = m_parent->GetRoot();
	return m_effectorMatrix * root->GetBasePoseMatrix();
}

void dAnimationRigEffector::SetLinearSpeed(dFloat speed)
{
	m_linearSpeed = dAbs (speed);
}

void dAnimationRigEffector::SetMaxLinearFriction(dFloat friction)
{
	m_linearFriction = dAbs(friction);
}

void dAnimationRigEffector::SetTargetPose(const dMatrix& globalSpaceMatrix)
{
	m_targetMatrix = globalSpaceMatrix;
}

void dAnimationRigEffector::Debug(dCustomJoint::dDebugDisplay* const debugDisplay) const
{
	const dMatrix& matrix1 = m_targetMatrix;
	dMatrix matrix0(m_localMatrix * m_state0->GetMatrix());

	debugDisplay->DrawFrame(matrix0);
	debugDisplay->DrawFrame(matrix1);
}

void dAnimationRigEffector::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	const dMatrix& matrix1 = m_targetMatrix;
	dMatrix matrix0(m_localMatrix * m_state0->GetMatrix());

	dVector omega(0.0f);
	dVector relPosit(matrix1.m_posit - matrix0.m_posit);

	const dVector& omega0 = m_state0->GetOmega();
	const dVector& omega1 = m_state1->GetOmega();
	const dVector& veloc0 = m_state0->GetVelocity();
	const dVector& veloc1 = m_state1->GetVelocity();

	dAssert(m_linearSpeed >= 0.0f);
	const dFloat timestep = constraintParams->m_timestep;
	const dFloat invTimestep = constraintParams->m_timestepInv;
	const dFloat step = m_linearSpeed * timestep;

	for (int i = 0; i < 3; i++) {

		dFloat currentSpeed = 0.0f;
		dFloat dist = relPosit.DotProduct3(matrix1[i]);
		if (dist > step) {
			currentSpeed = m_linearSpeed;
		} else if (dist < -step) {
			currentSpeed = -m_linearSpeed;
		} else {
			currentSpeed = 0.3f * dist * invTimestep;
		}

		AddLinearRowJacobian(constraintParams, matrix0.m_posit, matrix1[i], omega);

		dVector stopVeloc (constraintParams->m_jacobians[i].m_jacobian_J01.m_linear * veloc0 +
						   constraintParams->m_jacobians[i].m_jacobian_J01.m_angular * omega0 +
						   constraintParams->m_jacobians[i].m_jacobian_J10.m_linear * veloc1 +
						   constraintParams->m_jacobians[i].m_jacobian_J10.m_angular * omega1);
		currentSpeed -= (stopVeloc.m_x + stopVeloc.m_y + stopVeloc.m_z);

		constraintParams->m_jointLowFrictionCoef[i] = -m_linearFriction;
		constraintParams->m_jointHighFrictionCoef[i] = m_linearFriction;
		constraintParams->m_jointAccel[i] = currentSpeed * invTimestep;
		constraintParams->m_normalIndex[i] = 0;
	}

	m_dof = 2;
	m_count = 2;
	constraintParams->m_count = 2;
}
