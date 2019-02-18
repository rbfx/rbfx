/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgContact.h"
#include "dgCollision.h"
#include "dgCollisionInstance.h"
#include "dgCollisionCompound.h"
#include "dgWorldDynamicUpdate.h"
#include "dgCollisionDeformableMesh.h"
#include "dgCollisionCompoundFractured.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgBody::dgBody()
	:m_matrix (dgGetIdentityMatrix())
	,m_rotation(dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f))
	,m_invWorldInertiaMatrix(dgGetZeroMatrix())
	,m_mass(dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f))
	,m_invMass(dgFloat32 (0.0f))
	,m_veloc(dgFloat32 (0.0f))
	,m_omega(dgFloat32 (0.0f))
	,m_accel(dgFloat32 (0.0f))
	,m_alpha(dgFloat32 (0.0f))
	,m_minAABB(dgFloat32 (0.0f))
	,m_maxAABB(dgFloat32 (0.0f))
	,m_localCentreOfMass(dgFloat32 (0.0f))	
	,m_globalCentreOfMass(dgFloat32 (0.0f))	
	,m_impulseForce(dgFloat32 (0.0f))		
	,m_impulseTorque(dgFloat32 (0.0f))	
	,m_gyroTorque(dgFloat32 (0.0f))
	,m_gyroRotation(dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f))
	,m_criticalSectionLock(0)
	,m_flags(0)
	,m_userData(NULL)
	,m_world(NULL)
	,m_collision(NULL)
	,m_broadPhaseNode(NULL)
	,m_masterNode(NULL)
	,m_broadPhaseaggregateNode(NULL)
	,m_destructor(NULL)
	,m_matrixUpdate(NULL)
	,m_index(0)
	,m_uniqueID(0)
	,m_bodyGroupId(0)
	,m_rtti(m_baseBodyRTTI)
	,m_type(0)
	,m_serializedEnum(-1)
	,m_dynamicsLru(0)
	,m_genericLRUMark(0)
{
	m_autoSleep = true;
	m_collidable = true;
	m_transformIsDirty = true;
	m_collideWithLinkedBodies = true;
	m_invWorldInertiaMatrix[3][3] = dgFloat32 (1.0f);

	InitJointSet();
}

dgBody::dgBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionCashe, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber)
	:m_matrix (dgGetIdentityMatrix())
	,m_rotation(dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f))
	,m_invWorldInertiaMatrix(dgGetZeroMatrix())
	,m_mass(dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f), dgFloat32 (DG_INFINITE_MASS * 2.0f))
	,m_invMass(dgFloat32 (0.0f))
	,m_veloc(dgFloat32 (0.0f))
	,m_omega(dgFloat32 (0.0f))
	,m_accel(dgFloat32 (0.0f))
	,m_alpha(dgFloat32 (0.0f))
	,m_minAABB(dgFloat32 (0.0f))
	,m_maxAABB(dgFloat32 (0.0f))
	,m_localCentreOfMass(dgFloat32 (0.0f))	
	,m_globalCentreOfMass(dgFloat32 (0.0f))	
	,m_impulseForce(dgFloat32 (0.0f))		
	,m_impulseTorque(dgFloat32 (0.0f))		
	,m_gyroTorque(dgFloat32 (0.0f))
	,m_gyroRotation(dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f))
	,m_criticalSectionLock(0)
	,m_flags(0)
	,m_userData(NULL)
	,m_world(world)
	,m_collision(NULL)
	,m_broadPhaseNode(NULL)
	,m_masterNode(NULL)
	,m_broadPhaseaggregateNode(NULL)
	,m_destructor(NULL)
	,m_matrixUpdate(NULL)
	,m_index(0)
	,m_uniqueID(0)
	,m_bodyGroupId(0)
	,m_rtti(m_baseBodyRTTI)
	,m_type(0)
	,m_serializedEnum(-1)
	,m_dynamicsLru(0)
	,m_genericLRUMark(0)
{
	m_autoSleep = true;
	m_collidable = true;
	m_transformIsDirty = true;
	m_collideWithLinkedBodies = true;
	m_invWorldInertiaMatrix[3][3] = dgFloat32 (1.0f);

	serializeCallback (userData, &m_rotation, sizeof (m_rotation));
	serializeCallback (userData, &m_matrix, sizeof (m_matrix));
	serializeCallback (userData, &m_veloc, sizeof (m_veloc));
	serializeCallback (userData, &m_omega, sizeof (m_omega));
	serializeCallback (userData, &m_accel, sizeof (m_veloc));
	serializeCallback (userData, &m_alpha, sizeof (m_omega));
	serializeCallback (userData, &m_localCentreOfMass, sizeof (m_localCentreOfMass));
	serializeCallback (userData, &m_mass, sizeof (m_mass));
	serializeCallback (userData, &m_flags, sizeof (m_flags));
	serializeCallback (userData, &m_serializedEnum, sizeof(dgInt32));

	dgInt32 id;
	serializeCallback (userData, &id, sizeof (id));

	dgTree<const dgCollision*, dgInt32>::dgTreeNode* const node = collisionCashe->Find(id);
	dgAssert (node);

	const dgCollision* const collision = node->GetInfo();
	collision->AddRef();

	dgCollisionInstance* const instance = new (world->GetAllocator()) dgCollisionInstance (world, serializeCallback, userData, revisionNumber);
	instance->m_childShape = collision;
	m_collision = instance;

	InitJointSet();
}

dgBody::~dgBody()
{
}

dgVector dgBody::GetAlpha() const
{
	return dgVector::m_zero;
}
dgVector dgBody::GetAccel() const
{
	return dgVector::m_zero;
}

void dgBody::SetAlpha(const dgVector& alpha)
{
}

void dgBody::SetAccel(const dgVector& accel)
{
}

void dgBody::AttachCollision (dgCollisionInstance* const collisionSrc)
{
	dgCollisionInstance* const instance = new (m_world->GetAllocator()) dgCollisionInstance (*collisionSrc);
	m_world->GetBroadPhase()->CollisionChange (this, instance);
	if (m_collision) {
		m_collision->Release();
	}
	m_collision = instance;
	m_equilibrium = 0;
}

void dgBody::Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData)
{
	serializeCallback (userData, &m_rotation, sizeof (m_rotation));
	serializeCallback (userData, &m_matrix, sizeof (m_matrix));
	serializeCallback (userData, &m_veloc, sizeof (m_veloc));
	serializeCallback (userData, &m_omega, sizeof (m_omega));
	serializeCallback (userData, &m_accel, sizeof (m_veloc));
	serializeCallback (userData, &m_alpha, sizeof (m_omega));
	serializeCallback (userData, &m_localCentreOfMass, sizeof (m_localCentreOfMass));
	serializeCallback(userData, &m_mass, sizeof (m_mass));
	serializeCallback (userData, &m_flags, sizeof (m_flags));
	serializeCallback(userData, &m_serializedEnum, sizeof(dgInt32));

	dgTree<dgInt32, const dgCollision*>::dgTreeNode* const node = collisionRemapId.Find(m_collision->GetChildShape());
	dgAssert (node);

	dgInt32 id = node->GetInfo();
	serializeCallback (userData, &id, sizeof (id));
	m_collision->Serialize(serializeCallback, userData, false);
}

void dgBody::UpdateLumpedMatrix()
{
	if (m_collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
		dgAssert(IsRTTIType(dgBody::m_dynamicBodyRTTI));
		dgCollisionLumpedMassParticles* const lumpedMass = (dgCollisionLumpedMassParticles*)m_collision->m_childShape;
		lumpedMass->SetOwnerAndMassPraperties((dgDynamicBody*) this); 
	}
}


void dgBody::SetMatrix(const dgMatrix& matrix)
{
	SetMatrixOriginAndRotation(matrix);

	if (!m_inCallback) {
		UpdateCollisionMatrix (dgFloat32 (0.0f), 0);
	}
}

void dgBody::SetMatrixNoSleep(const dgMatrix& matrix)
{
	SetMatrix(matrix);
}

void dgBody::SetMatrixResetSleep(const dgMatrix& matrix)
{
	m_sleeping = false;
	SetMatrix(matrix);
}

void dgBody::UpdateWorlCollisionMatrix() const
{
	m_collision->SetGlobalMatrix (m_collision->GetLocalMatrix() * m_matrix);
}



void dgBody::UpdateCollisionMatrix (dgFloat32 timestep, dgInt32 threadIndex)
{
	m_transformIsDirty = true;
	m_collision->SetGlobalMatrix (m_collision->GetLocalMatrix() * m_matrix);
	m_collision->CalcAABB (m_collision->GetGlobalMatrix(), m_minAABB, m_maxAABB);

	if (m_continueCollisionMode) {
		dgVector predictiveVeloc (PredictLinearVelocity(timestep));
		dgVector predictiveOmega (PredictAngularVelocity(timestep));
		dgMovingAABB (m_minAABB, m_maxAABB, predictiveVeloc, predictiveOmega, timestep, m_collision->GetBoxMaxRadius(), m_collision->GetBoxMinRadius());
	}

	if (m_broadPhaseNode) {
		dgAssert (m_world);
		if (!m_equilibrium) {
			m_world->GetBroadPhase()->UpdateBody (this, threadIndex);
		}
	}
}


dgFloat32 dgBody::RayCast (const dgLineBox& line, OnRayCastAction filter, OnRayPrecastAction preFilter, void* const userData, dgFloat32 maxT) const
{
	dgAssert (filter);
	dgVector l0 (line.m_l0);
	dgVector l1 (line.m_l0 + (line.m_l1 - line.m_l0).Scale (dgMin(maxT, dgFloat32 (1.0f))));
	if (dgRayBoxClip (l0, l1, m_minAABB, m_maxAABB)) {
//	if (1) {
//l0 = dgVector (-20.3125000f, 3.54991579f, 34.3441200f, 0.0f);
//l1 = dgVector (-19.6875000f, 3.54257250f, 35.2211456f, 0.0f);

		dgContactPoint contactOut;
		const dgMatrix& globalMatrix = m_collision->GetGlobalMatrix();
		dgVector localP0 (globalMatrix.UntransformVector (l0));
		dgVector localP1 (globalMatrix.UntransformVector (l1));
		dgVector p1p0 (localP1 - localP0);
		dgAssert (p1p0.m_w == dgFloat32 (0.0f));
		if (p1p0.DotProduct(p1p0).GetScalar() > dgFloat32 (1.0e-12f)) {
			dgFloat32 t = m_collision->RayCast (localP0, localP1, dgFloat32 (1.0f), contactOut, preFilter, this, userData);
			if (t < dgFloat32 (1.0f)) {
				dgAssert (localP0.m_w == dgFloat32 (0.0f));
				dgAssert (localP1.m_w == dgFloat32 (0.0f));
				dgVector p (globalMatrix.TransformVector(localP0 + (localP1 - localP0).Scale(t)));
				dgVector l1l0 (line.m_l1 - line.m_l0);
				dgAssert (l1l0.m_w == dgFloat32 (0.0f));
				t = l1l0.DotProduct(p - line.m_l0).GetScalar() / l1l0.DotProduct(l1l0).GetScalar();
				if (t < maxT) {
					dgAssert (t >= dgFloat32 (0.0f));
					dgAssert (t <= dgFloat32 (1.0f));
					contactOut.m_normal = globalMatrix.RotateVector (contactOut.m_normal);
					maxT = filter (this, contactOut.m_collision0, p, contactOut.m_normal, contactOut.m_shapeId0, userData, t);
				}
			}
		}
	} 
	return maxT;
}

void dgBody::IntegrateVelocity (dgFloat32 timestep)
{
	dgAssert (m_veloc.m_w == dgFloat32 (0.0f));
	dgAssert (m_omega.m_w == dgFloat32 (0.0f));
	m_globalCentreOfMass += m_veloc.Scale (timestep); 
	dgFloat32 omegaMag2 = m_omega.DotProduct(m_omega).GetScalar();
#ifdef _DEBUG
	const dgFloat32 err = dgFloat32(90.0f * dgDEG2RAD);
	const dgFloat32 err2 = err * err;
	const dgFloat32 step2 = omegaMag2 * timestep * timestep;
	if (step2 > err2) {
		dgTrace (("warning bodies %d w(%f %f %f) with very high angular velocity, may be unstable\n", m_uniqueID, m_omega.m_x, m_omega.m_y, m_omega.m_z));
	}
#endif

	// this is correct
	if (omegaMag2 > ((dgFloat32 (0.0125f) * dgDEG2RAD) * (dgFloat32 (0.0125f) * dgDEG2RAD))) {
		dgFloat32 invOmegaMag = dgRsqrt (omegaMag2);
		dgVector omegaAxis (m_omega.Scale (invOmegaMag));
		dgFloat32 omegaAngle = invOmegaMag * omegaMag2 * timestep;
		dgQuaternion rotation (omegaAxis, omegaAngle);
		m_rotation = m_rotation * rotation;
		m_rotation.Scale(dgRsqrt (m_rotation.DotProduct (m_rotation)));
		m_matrix = dgMatrix (m_rotation, m_matrix.m_posit);
	}

	m_matrix.m_posit = m_globalCentreOfMass - m_matrix.RotateVector(m_localCentreOfMass);
	dgAssert (m_matrix.TestOrthogonal());
}

dgVector dgBody::CalculateInverseDynamicForce (const dgVector& desiredVeloc, dgFloat32 timestep) const
{
	dgAssert (0);
	return dgVector();
/*
	dgFloat32 massAccel = m_mass.m_w / timestep;
	if (m_world->m_solverMode){
		if (m_masterNode->GetInfo().GetCount() >= 2){
			massAccel *= (dgFloat32 (2.0f) * dgFloat32 (LINEAR_SOLVER_SUB_STEPS) / dgFloat32 (LINEAR_SOLVER_SUB_STEPS + 1));
		} 
	}
	return (desiredVeloc - m_veloc).Scale (massAccel);
*/
}


dgConstraint* dgBody::GetFirstJoint() const
{
	if (m_masterNode) {
		for (dgBodyMasterListRow::dgListNode* node = m_masterNode->GetInfo().GetFirst(); node; node = node->GetNext()) {
			dgConstraint* const joint = node->GetInfo().m_joint;
			if (joint && (joint->GetId() >= dgConstraint::m_unknownConstraint)) {
				return joint;
			}
		}
	}
	return NULL;
}

dgConstraint* dgBody::GetNextJoint(dgConstraint* const joint) const
{
	dgBodyMasterListRow::dgListNode* node = joint->GetLink0();
	if (joint->GetBody0() != this) {
		node = joint->GetLink1();
	}

	if (node->GetInfo().m_joint == joint) {
		for (node = node->GetNext(); node; node = node->GetNext()) {
			dgConstraint* const joint1 = node->GetInfo().m_joint;
			if (joint1->GetId() >= dgConstraint::m_unknownConstraint) {
				return joint1;
			}
		}
	}

	return NULL;
}


dgConstraint* dgBody::GetFirstContact() const
{
	if (m_masterNode) {
		for (dgBodyMasterListRow::dgListNode* node = m_masterNode->GetInfo().GetFirst(); node; node = node->GetNext()) {
			dgConstraint* const joint = node->GetInfo().m_joint;
			dgAssert (joint);
			if (joint && (joint->GetId() == dgConstraint::m_contactConstraint)) {
				return joint;
			}
		}
	}
	return NULL;
}

dgConstraint* dgBody::GetNextContact(dgConstraint* const joint) const
{
	dgBodyMasterListRow::dgListNode* node = joint->GetLink0();
	if (joint->GetBody0() != this) {
		node = joint->GetLink1();
	}

	if (node->GetInfo().m_joint == joint) {
		for (node = node->GetNext(); node; node = node->GetNext()) {
			dgConstraint* const joint1 = node->GetInfo().m_joint;
			dgAssert (joint1);
			if (joint1 && (joint1->GetId() == dgConstraint::m_contactConstraint)) {
				return joint1;
			}
		}
	}

	return NULL;
}

void dgBody::SetFreeze (bool state)
{
	if (state){
		Freeze();
	} else {
		Unfreeze();	
	}
}


void dgBody::Freeze ()
{
	if (GetInvMass().m_w > dgFloat32 (0.0f)) {
		if (!m_freeze) {
			m_freeze = true;
			for (dgBodyMasterListRow::dgListNode* node = m_masterNode->GetInfo().GetFirst(); node; node = node->GetNext()) {
				dgBody* const body = node->GetInfo().m_bodyNode;
				body->Freeze ();
			}
		}
	}
}

void dgBody::Unfreeze ()
{
	if (GetInvMass().m_w > dgFloat32 (0.0f)) {
// note this is in observation (to prevent bodies from not going to sleep  inside triggers	               
//		m_equilibrium = false;			
		if (m_freeze) {
			m_freeze = false;
			for (dgBodyMasterListRow::dgListNode* node = m_masterNode->GetInfo().GetFirst(); node; node = node->GetNext()) {
				dgBody* const body = node->GetInfo().m_bodyNode;
				body->Unfreeze ();
			}
		}
	}
}

void dgBody::SetMassMatrix(dgFloat32 mass, const dgMatrix& inertia)
{
	dgFloat32 Ixx = inertia[0][0];
	dgFloat32 Iyy = inertia[1][1];
	dgFloat32 Izz = inertia[2][2];
	mass = dgAbs (mass);
	if (m_collision->IsType(dgCollision::dgCollisionMesh_RTTI) || m_collision->IsType(dgCollision::dgCollisionScene_RTTI)) {
		mass = DG_INFINITE_MASS * 2.0f;
	}

	if (m_collision->IsType(dgCollision::dgCollisionCompound_RTTI)) {
		const dgCollision* const childShape = m_collision->GetChildShape();
		if ((childShape->m_inertia.m_x < dgFloat32 (1.0e-5f)) || (childShape->m_inertia.m_y < dgFloat32 (1.0e-5f)) || (childShape->m_inertia.m_z < dgFloat32 (1.0e-5f))){
			mass = DG_INFINITE_MASS * 2.0f;
		}
	}

	if (mass < DG_MINIMUM_MASS) {
		mass = DG_INFINITE_MASS * 2.0f;
	}

	//dgAssert (m_masterNode);
	m_world->GetBroadPhase()->CheckStaticDynamic(this, mass);

	if (mass >= DG_INFINITE_MASS) {
		if (m_masterNode) {
			if (m_invMass.m_w != dgFloat32 (0.0f)) {
				dgBodyMasterList& masterList (*m_world);
				if (masterList.GetFirst() != m_masterNode) {
					masterList.InsertAfter (masterList.GetFirst(), m_masterNode);
				}
			}
		}

		m_mass.m_x = DG_INFINITE_MASS;
		m_mass.m_y = DG_INFINITE_MASS;
		m_mass.m_z = DG_INFINITE_MASS;
		m_mass.m_w = DG_INFINITE_MASS;
		m_invMass.m_x = dgFloat32(0.0f);
		m_invMass.m_y = dgFloat32(0.0f);
		m_invMass.m_z = dgFloat32(0.0f);
		m_invMass.m_w = dgFloat32(0.0f);

	} else {
		Ixx = dgAbs (Ixx);
		Iyy = dgAbs (Iyy);
		Izz = dgAbs (Izz);

		dgFloat32 Ixx1 = dgClamp (Ixx, dgFloat32 (0.001f) * mass, dgFloat32 (1000.0f) * mass);
		dgFloat32 Iyy1 = dgClamp (Iyy, dgFloat32 (0.001f) * mass, dgFloat32 (1000.0f) * mass);
		dgFloat32 Izz1 = dgClamp (Izz, dgFloat32 (0.001f) * mass, dgFloat32 (1000.0f) * mass);

		dgAssert (Ixx > dgFloat32 (0.0f));
		dgAssert (Iyy > dgFloat32 (0.0f));
		dgAssert (Izz > dgFloat32 (0.0f));

		if (m_masterNode) {
			if (m_invMass.m_w == dgFloat32 (0.0f)) {
				dgBodyMasterList& masterList(*m_world);
				masterList.RotateToEnd(m_masterNode);
			}
		}

		m_mass.m_x = Ixx1;
		m_mass.m_y = Iyy1;
		m_mass.m_z = Izz1;
		m_mass.m_w = mass;

		m_invMass.m_x = dgFloat32 (1.0f) / Ixx1;
		m_invMass.m_y = dgFloat32 (1.0f) / Iyy1;
		m_invMass.m_z = dgFloat32 (1.0f) / Izz1;
		m_invMass.m_w = dgFloat32 (1.0f) / mass;
	}


//#ifdef _DEBUG
#if 0
	dgBodyMasterList& me = *m_world;
	for (dgBodyMasterList::dgListNode* refNode = me.GetFirst(); refNode; refNode = refNode->GetNext()) {
		dgBody* const body0 = refNode->GetInfo().GetBody();
		dgVector invMass (body0->GetInvMass());
		if (invMass.m_w != 0.0f) {
			for (; refNode; refNode = refNode->GetNext()) {
				dgBody* const body1 = refNode->GetInfo().GetBody();
				dgVector invMass1 (body1->GetInvMass());
				dgAssert (invMass1.m_w != 0.0f);
			}
			break;
		}
	}
#endif
//	UpdateLumpedMatrix();
}

void dgBody::SetMassProperties (dgFloat32 mass, const dgCollisionInstance* const collision)
{
	// using general central theorem, to extract the Inertia relative to the center of mass 
	dgMatrix inertia (collision->CalculateInertia());

	dgVector origin (inertia.m_posit);
		for (dgInt32 i = 0; i < 3; i ++) {
		inertia[i][i] = (inertia[i][i] + origin[i] * origin[i]) * mass;
		for (dgInt32 j = i + 1; j < 3; j ++) {
			dgFloat32 crossIJ = origin[i] * origin[j];
			inertia[i][j] = (inertia[i][j] + crossIJ) * mass;
			inertia[j][i] = (inertia[j][i] + crossIJ) * mass;
		}
	}

	// although the engine fully supports asymmetric inertia, I will ignore cross inertia for now
	//SetMassMatrix(mass, inertia[0][0], inertia[1][1], inertia[2][2]);
	SetCentreOfMass(origin);
	SetMassMatrix(mass, inertia);
}

dgMatrix dgBody::CalculateLocalInertiaMatrix () const
{
	dgMatrix inertia (dgGetIdentityMatrix());
	inertia[0][0] = m_mass.m_x;
	inertia[1][1] = m_mass.m_y;
	inertia[2][2] = m_mass.m_z;
	return inertia;
}

dgMatrix dgBody::CalculateInertiaMatrix () const
{
#if 0
	dgMatrix tmp (m_matrix.Transpose4X4());
	dgVector mass (m_mass & dgVector::m_triplexMask);
	tmp[0] = tmp[0] * mass;
	tmp[1] = tmp[1] * mass;
	tmp[2] = tmp[2] * mass;
	return dgMatrix (m_matrix.RotateVector(tmp[0]), m_matrix.RotateVector(tmp[1]), m_matrix.RotateVector(tmp[2]), dgVector::m_wOne);
#else
	const dgVector Ixx(m_mass[0]);
	const dgVector Iyy(m_mass[1]);
	const dgVector Izz(m_mass[2]);
	return dgMatrix (m_matrix.m_front.Scale(m_matrix.m_front[0]) * Ixx +
					 m_matrix.m_up.Scale(m_matrix.m_up[0])		  * Iyy +
					 m_matrix.m_right.Scale(m_matrix.m_right[0]) * Izz,

					 m_matrix.m_front.Scale(m_matrix.m_front[1]) * Ixx +
					 m_matrix.m_up.Scale(m_matrix.m_up[1])       * Iyy +
					 m_matrix.m_right.Scale(m_matrix.m_right[1]) * Izz,

					 m_matrix.m_front.Scale(m_matrix.m_front[2]) * Ixx +
					 m_matrix.m_up.Scale(m_matrix.m_up[2])       * Iyy +
					 m_matrix.m_right.Scale(m_matrix.m_right[2]) * Izz,
					 dgVector::m_wOne);
#endif
}

dgMatrix dgBody::CalculateInvInertiaMatrix () const
{
#if 0
	dgMatrix tmp (m_matrix.Transpose4X4());
	tmp[0] = tmp[0] * m_invMass;
	tmp[1] = tmp[1] * m_invMass;
	tmp[2] = tmp[2] * m_invMass;
	return dgMatrix (m_matrix.RotateVector(tmp[0]), m_matrix.RotateVector(tmp[1]), m_matrix.RotateVector(tmp[2]), dgVector::m_wOne);
#else
	const dgVector invIxx(m_invMass[0]);
	const dgVector invIyy(m_invMass[1]);
	const dgVector invIzz(m_invMass[2]);
	return dgMatrix(m_matrix.m_front.Scale(m_matrix.m_front[0]) * invIxx +
					m_matrix.m_up.Scale(m_matrix.m_up[0])		 * invIyy +
					m_matrix.m_right.Scale(m_matrix.m_right[0]) * invIzz,

					m_matrix.m_front.Scale(m_matrix.m_front[1]) * invIxx +
					m_matrix.m_up.Scale(m_matrix.m_up[1])		 * invIyy +
					m_matrix.m_right.Scale(m_matrix.m_right[1]) * invIzz,

					m_matrix.m_front.Scale(m_matrix.m_front[2]) * invIxx +
					m_matrix.m_up.Scale(m_matrix.m_up[2])		 * invIyy +
					m_matrix.m_right.Scale(m_matrix.m_right[2]) * invIzz,
					dgVector::m_wOne);
#endif
}


void dgBody::InvalidateCache ()
{
	m_sleeping = false;
	m_equilibrium = false;
	m_genericLRUMark = 0;
	dgMatrix matrix (m_matrix);
	SetMatrixOriginAndRotation(matrix);
}


void dgBody::AddImpulse (const dgVector& pointDeltaVeloc, const dgVector& pointPosit, dgFloat32 timestep)
{
	dgMatrix invInertia (CalculateInvInertiaMatrix());

	// get contact matrix
	dgMatrix tmp;
	dgVector globalContact (pointPosit - m_globalCentreOfMass);

	tmp[0][0] = dgFloat32 (0.0f);
	tmp[0][1] = + globalContact[2];
	tmp[0][2] = - globalContact[1];
	tmp[0][3] = dgFloat32 (0.0f);

	tmp[1][0] = -globalContact[2];
	tmp[1][1] = dgFloat32 (0.0f);
	tmp[1][2] = +globalContact[0];
	tmp[1][3] = dgFloat32 (0.0f);

	tmp[2][0] = +globalContact[1];
	tmp[2][1] = -globalContact[0];
	tmp[2][2] = dgFloat32 (0.0f);
	tmp[2][3] = dgFloat32 (0.0f);

	tmp[3][0] = dgFloat32 (0.0f);
	tmp[3][1] = dgFloat32 (0.0f);
	tmp[3][2] = dgFloat32 (0.0f);
	tmp[3][3] = dgFloat32 (1.0f);

	dgMatrix contactMatrix (tmp * invInertia * tmp);
	for (dgInt32 i = 0; i < 3; i ++) {
		for (dgInt32 j = 0; j < 3; j ++) {
			contactMatrix[i][j] *= -dgFloat32 (1.0f);
		}
	}
	contactMatrix[0][0] += m_invMass.m_w;	
	contactMatrix[1][1] += m_invMass.m_w;	
	contactMatrix[2][2] += m_invMass.m_w;	

	contactMatrix = contactMatrix.Symetric3by3Inverse ();

	// change of momentum
	dgVector changeOfMomentum (contactMatrix.RotateVector (pointDeltaVeloc));

	m_impulseForce += changeOfMomentum.Scale(1.0f / timestep);
	m_impulseTorque += globalContact.CrossProduct(m_impulseForce);

	m_sleeping	= false;
	m_equilibrium = false;
	Unfreeze ();
}

void dgBody::ApplyImpulsePair (const dgVector& linearImpulseIn, const dgVector& angularImpulseIn, dgFloat32 timestep)
{
	m_impulseForce += linearImpulseIn.Scale(1.0f / timestep);
	m_impulseTorque += angularImpulseIn.Scale(1.0f / timestep);

	m_sleeping	= false;
	m_equilibrium = false;
	Unfreeze ();
}

void dgBody::ApplyImpulsesAtPoint (dgInt32 count, dgInt32 strideInBytes, const dgFloat32* const impulseArray, const dgFloat32* const pointArray, dgFloat32 timestep)
{
	dgInt32 stride = strideInBytes / sizeof (dgFloat32);

	dgMatrix inertia (CalculateInertiaMatrix());

	dgVector impulse (dgFloat32 (0.0f));
	dgVector angularImpulse (dgFloat32 (0.0f));

	dgVector com (m_globalCentreOfMass);
	for (dgInt32 i = 0; i < count; i ++) {
		dgInt32 index = i * stride;
		dgVector r (pointArray[index], pointArray[index + 1], pointArray[index + 2], dgFloat32 (0.0f));
		dgVector L (impulseArray[index], impulseArray[index + 1], impulseArray[index + 2], dgFloat32 (0.0f));
		dgVector Q ((r - com).CrossProduct(L));

		impulse += L;
		angularImpulse += Q;
	}

	m_impulseForce += impulse.Scale(1.0f / timestep);
	m_impulseTorque += angularImpulse.Scale(1.0f / timestep);

	m_sleeping	= false;
	m_equilibrium = false;
	Unfreeze ();
}

