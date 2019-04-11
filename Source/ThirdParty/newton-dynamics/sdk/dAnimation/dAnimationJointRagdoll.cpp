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
//#include "dAnimAcyclicJoint.h"
#include "dAnimationJointRagdoll.h"
#include "dAnimationModelManager.h"

/*
dAnimAcyclicJoint::dAnimAcyclicJoint(dAnimAcyclicJoint* const parent)
	:dContainersAlloc()
	,m_proxyBody()
	,m_proxyJoint()
	,m_parent(parent)
	,m_userData(NULL)
	,m_world(NULL)
	,m_solverIndex(-1)
	,m_isLoop(false)
	,m_children()
{
	if (parent) {
		parent->m_children.Append(this);
	}
}

dAnimAcyclicJoint::~dAnimAcyclicJoint()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		delete child->GetInfo();
	}
}

void* dAnimAcyclicJoint::GetUserData()
{
	return m_userData;
}

void dAnimAcyclicJoint::SetUserData(void* const userData)
{
	m_userData = userData;
}


void dAnimAcyclicJoint::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Debug(debugContext);
	}
}

void dAnimAcyclicJoint::ApplyExternalForce(dFloat timestep)
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->ApplyExternalForce(timestep);
	}
}

int dAnimAcyclicJoint::GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray)
{
	int count = 0;
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		count += child->GetInfo()->GetKinematicLoops(&jointArray[count]);
	}
	return count;
}

void dAnimAcyclicJoint::UpdateJointAcceleration()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->UpdateJointAcceleration();
	}
}

void dAnimAcyclicJoint::Finalize()
{
	for (dList<dAnimAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		child->GetInfo()->Finalize();
	}
}
*/

class dAnimationJointRagdoll;
class dAnimationJointRagdoll::dRagDollMotor: public dCustomBallAndSocket
{
	public:
	dRagDollMotor(dAnimationJointRagdoll* const owner, const dMatrix& pinAndPivotFrame0, const dMatrix& pinAndPivotFrame1, NewtonBody* const child, NewtonBody* const parent)
		:dCustomBallAndSocket(pinAndPivotFrame0, pinAndPivotFrame1, child, parent)
		,m_owner(owner)
	{
		m_coneFriction = 100.0f;
		m_twistFriction = 100.0f;
	}

	void SubmitConstraints(dFloat timestep, int threadIndex)
	{
		dMatrix matrix0;
		dMatrix matrix1;

		// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
		CalculateGlobalMatrix(matrix0, matrix1);
		SubmitLinearRows(0x07, matrix0, matrix1);

		const dVector& coneDir0 = matrix0.m_front;
		const dVector& coneDir1 = matrix1.m_front;

		dFloat cosAngleCos = coneDir1.DotProduct3(coneDir0);
		dMatrix coneRotation(dGetIdentityMatrix());
		dVector lateralDir(matrix0.m_up);

		if (cosAngleCos < 0.9999f) {
			lateralDir = coneDir1.CrossProduct(coneDir0);
			dFloat mag2 = lateralDir.DotProduct3(lateralDir);
			if (mag2 > 1.0e-4f) {
				lateralDir = lateralDir.Scale(1.0f / dSqrt(mag2));
				coneRotation = dMatrix(dQuaternion(lateralDir, dAcos(dClamp(cosAngleCos, dFloat(-1.0f), dFloat(1.0f)))), matrix1.m_posit);
			} else {
				lateralDir = matrix0.m_up.Scale(-1.0f);
				coneRotation = dMatrix(dQuaternion(matrix0.m_up, 180 * dDegreeToRad), matrix1.m_posit);
			}
		}

		dVector omega0(0.0f);
		dVector omega1(0.0f);
		NewtonBodyGetOmega(m_body0, &omega0[0]);
		NewtonBodyGetOmega(m_body1, &omega1[0]);

		dVector relOmega(omega0 - omega1);

		// do twist angle calculations
//		dMatrix twistMatrix(matrix0 * (matrix1 * coneRotation).Inverse());
//		dFloat twistAngle = m_twistAngle.Update(dAtan2(twistMatrix[1][2], twistMatrix[1][1]));
		if (m_options.m_option0) {
			dAssert(0);
/*
			if ((m_minTwistAngle == 0.0f) && (m_minTwistAngle == 0.0f)) {
				NewtonUserJointAddAngularRow(m_joint, -twistAngle, &matrix0.m_front[0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			} else {
				if (m_options.m_option1) {
					// TODO spring option
					dAssert(0);
				} else {
					SubmitConstraintTwistLimits(matrix0, matrix1, relOmega, timestep);
				}
			}
*/
		} else {
			NewtonUserJointAddAngularRow(m_joint, 0, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

			NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_twistFriction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_twistFriction);
		}

		// do twist cone angle calculations
		if (m_options.m_option2) {
			dAssert(0);
/*
			if ((m_maxConeAngle == 0.0f)) {
				dMatrix localMatrix(matrix0 * matrix1.Inverse());
				dVector euler0;
				dVector euler1;
				localMatrix.GetEulerAngles(euler0, euler1, m_pitchRollYaw);
				NewtonUserJointAddAngularRow(m_joint, -euler0[1], &matrix1[1][0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
				NewtonUserJointAddAngularRow(m_joint, -euler0[2], &matrix1[2][0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
			} else {
				if (m_options.m_option3) {
					// TODO spring option
					dAssert(0);
				}
				else {
					dFloat jointOmega = relOmega.DotProduct3(lateralDir);
					dFloat currentAngle = dAcos(dClamp(cosAngleCos, dFloat(-1.0f), dFloat(1.0f)));
					dFloat coneAngle = currentAngle + jointOmega * timestep;
					if (coneAngle >= m_maxConeAngle) {
						//dQuaternion rot(lateralDir, coneAngle);
						//dVector frontDir(rot.RotateVector(coneDir1));
						//dVector upDir(lateralDir.CrossProduct(frontDir));

						dVector upDir(lateralDir.CrossProduct(coneDir0));
						NewtonUserJointAddAngularRow(m_joint, 0.0f, &upDir[0]);
						NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
						NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

						NewtonUserJointAddAngularRow(m_joint, 0.0f, &lateralDir[0]);
						NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
						NewtonUserJointSetRowMaximumFriction(m_joint, m_coneFriction);
						const dFloat invtimestep = 1.0f / timestep;
						const dFloat speed = 0.5f * (m_maxConeAngle - currentAngle) * invtimestep;
						const dFloat stopAccel = NewtonUserJointCalculateRowZeroAccelaration(m_joint) + speed * invtimestep;
						NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

					}
					else if (m_coneFriction != 0) {
						NewtonUserJointAddAngularRow(m_joint, 0.0f, &lateralDir[0]);
						NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
						NewtonUserJointSetRowMinimumFriction(m_joint, -m_coneFriction);
						NewtonUserJointSetRowMaximumFriction(m_joint, m_coneFriction);

						dVector upDir(lateralDir.CrossProduct(coneDir0));
						NewtonUserJointAddAngularRow(m_joint, 0.0f, &upDir[0]);
						NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
						NewtonUserJointSetRowMinimumFriction(m_joint, -m_coneFriction);
						NewtonUserJointSetRowMaximumFriction(m_joint, m_coneFriction);
					}
				}
			}
			*/
		} else {
			NewtonUserJointAddAngularRow(m_joint, 0.0f, &lateralDir[0]);
			NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_coneFriction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_coneFriction);

			dVector upDir(lateralDir.CrossProduct(coneDir0));
			NewtonUserJointAddAngularRow(m_joint, 0.0f, &upDir[0]);
			NewtonUserJointSetRowAcceleration(m_joint, NewtonUserJointCalculateRowZeroAccelaration(m_joint));
			NewtonUserJointSetRowMinimumFriction(m_joint, -m_coneFriction);
			NewtonUserJointSetRowMaximumFriction(m_joint, m_coneFriction);
		}
	}

	int GetStructuralDOF() const
	{
		return 3;
	}

	dAnimationJointRagdoll* m_owner;
};

dAnimationJointRagdoll::dAnimationJointRagdoll(const dMatrix& pinAndPivotInGlobalSpace, NewtonBody* const body, const dMatrix& bindMarix, dAnimationJoint* const parent)
	:dAnimationJoint(body, bindMarix, parent)
	,dAnimationContraint()
{
//	dJointDefinition::dJointLimit jointLimits(definition.m_jointLimits);
	dMatrix parentRollMatrix(dGetIdentityMatrix() * pinAndPivotInGlobalSpace);
	dRagDollMotor* const joint = new dRagDollMotor(this, pinAndPivotInGlobalSpace, parentRollMatrix, body, parent->GetBody());

	m_joint = joint;
	//dFloat friction = definition.m_friction * 0.25f;
	//joint->EnableCone(true);
	//joint->SetConeFriction(friction);
	//joint->SetConeLimits(jointLimits.m_coneAngle * dDegreeToRad);
	//
	//joint->EnableTwist(true);
	//joint->SetTwistFriction(friction);
	//joint->SetTwistLimits(jointLimits.m_minTwistAngle * dDegreeToRad, jointLimits.m_maxTwistAngle * dDegreeToRad);

	m_proxyJoint = this;
	Init(GetProxyBody(), parent->GetProxyBody());
}

dAnimationJointRagdoll::~dAnimationJointRagdoll()
{
}

void dAnimationJointRagdoll::RigidBodyToStates()
{
	dAnimationJoint::RigidBodyToStates();
	m_proxyBody.SetForce(dVector(0.0f));
	m_proxyBody.SetTorque(dVector(0.0f));
}

void dAnimationJointRagdoll::JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams)
{
	m_rowAccel = dVector (0.0f);
	NewtonImmediateModeConstraint descriptor;

	dRagDollMotor* const ragDollJoint = (dRagDollMotor*) m_joint;
	NewtonJoint* const newtonJoint = ragDollJoint->GetJoint();
	const int rows = NewtonUserJointSubmitImmediateModeConstraint(newtonJoint, &descriptor, constraintParams->m_timestep);
	const int fixdof = ragDollJoint->GetStructuralDOF();
	dAssert (rows >= fixdof);

	for (int i = 0; i < fixdof; i++) {
		constraintParams->m_jacobians[i].m_jacobian_J01.m_linear = dVector(descriptor.m_jacobian01[i][0], descriptor.m_jacobian01[i][1], descriptor.m_jacobian01[i][2], dFloat(0.0f));
		constraintParams->m_jacobians[i].m_jacobian_J01.m_angular = dVector(descriptor.m_jacobian01[i][3], descriptor.m_jacobian01[i][4], descriptor.m_jacobian01[i][5], dFloat(0.0f));
		constraintParams->m_jacobians[i].m_jacobian_J10.m_linear = dVector(descriptor.m_jacobian10[i][0], descriptor.m_jacobian10[i][1], descriptor.m_jacobian10[i][2], dFloat(0.0f));
		constraintParams->m_jacobians[i].m_jacobian_J10.m_angular = dVector(descriptor.m_jacobian10[i][3], descriptor.m_jacobian10[i][4], descriptor.m_jacobian10[i][5], dFloat(0.0f));
		constraintParams->m_jointAccel[i] = descriptor.m_jointAccel[i];
		constraintParams->m_jointLowFrictionCoef[i] = descriptor.m_minFriction[i];
		constraintParams->m_jointHighFrictionCoef[i] = descriptor.m_maxFriction[i];
		constraintParams->m_normalIndex[i] = 0;
	}

	for (int i = fixdof; i < rows; i++) {
		const int j = i - fixdof;
		m_jacobial01[j].m_linear = dVector(descriptor.m_jacobian01[i][0], descriptor.m_jacobian01[i][1], descriptor.m_jacobian01[i][2], dFloat(0.0f));
		m_jacobial01[j].m_angular = dVector(descriptor.m_jacobian01[i][3], descriptor.m_jacobian01[i][4], descriptor.m_jacobian01[i][5], dFloat(0.0f));
	
		m_jacobial10[j].m_linear = dVector(descriptor.m_jacobian10[i][0], descriptor.m_jacobian10[i][1], descriptor.m_jacobian10[i][2], dFloat(0.0f));
		m_jacobial10[j].m_angular = dVector(descriptor.m_jacobian10[i][3], descriptor.m_jacobian10[i][4], descriptor.m_jacobian10[i][5], dFloat(0.0f));
	}

	m_dof = fixdof;
	m_count = fixdof;
	constraintParams->m_count = fixdof;
}

void dAnimationJointRagdoll::UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const
{
	dAssert (0);
}
