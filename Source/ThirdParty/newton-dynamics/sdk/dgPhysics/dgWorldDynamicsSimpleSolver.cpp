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
#include "dgConstraint.h"
#include "dgDynamicBody.h"
#include "dgDynamicBody.h"
#include "dgSkeletonContainer.h"
#include "dgCollisionInstance.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"

//#define DG_TEST_GYRO
#define DG_USE_SKEL

void dgWorldDynamicUpdate::ResolveClusterForces(dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const
{
	dgInt32 activeJoint = cluster->m_jointCount;
	if (activeJoint > 0) {
		activeJoint = SortClusters(cluster, timestep, threadID);
	}

	dgWorld* const world = (dgWorld*) this;
	dgJointInfo* const constraintArray = &world->m_jointsMemory[cluster->m_jointStart];

	if (!cluster->m_isContinueCollision) {
		if (activeJoint >= 1) {
			BuildJacobianMatrix(cluster, threadID, timestep);
			CalculateClusterReactionForces(cluster, threadID, timestep);
		} else if (cluster->m_jointCount == 0) {
			IntegrateExternalForce(cluster, timestep, threadID);
		} else {
			dgAssert((activeJoint == 0) && cluster->m_jointCount);
			dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];
			dgVector zero(dgVector::m_zero);
			for (dgInt32 i = 1; i < cluster->m_bodyCount; i++) {
				dgDynamicBody* const body = (dgDynamicBody*)bodyArray[i].m_body;
				body->m_accel = zero;
				body->m_alpha = zero;
			}
		}

		IntegrateVelocity (cluster, DG_SOLVER_MAX_ERROR, timestep, threadID); 
	} else {
		// calculate reaction forces and new velocities
		BuildJacobianMatrix (cluster, threadID, timestep);
		IntegrateReactionsForces (cluster, threadID, timestep);

		// see if the cluster goes to sleep
		bool isAutoSleep = true;
		bool stackSleeping = true;
		dgInt32 sleepCounter = 10000;

		const dgInt32 bodyCount = cluster->m_bodyCount;
		dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];

		const dgFloat32 forceDamp = DG_FREEZZING_VELOCITY_DRAG;
		dgFloat32 maxAccel = dgFloat32 (0.0f);
		dgFloat32 maxAlpha = dgFloat32 (0.0f);
		dgFloat32 maxSpeed = dgFloat32 (0.0f);
		dgFloat32 maxOmega = dgFloat32 (0.0f);

		const dgFloat32 speedFreeze = world->m_freezeSpeed2;
		const dgFloat32 accelFreeze = world->m_freezeAccel2;
		const dgVector forceDampVect (forceDamp, forceDamp, forceDamp, dgFloat32 (0.0f));
		for (dgInt32 i = 1; i < bodyCount; i ++) {
			dgDynamicBody* const body = (dgDynamicBody*) bodyArray[i].m_body;
			if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
				dgAssert (body->m_invMass.m_w);

				const dgFloat32 accel2 = body->m_accel.DotProduct(body->m_accel).GetScalar();
				const dgFloat32 alpha2 = body->m_alpha.DotProduct(body->m_alpha).GetScalar();
				const dgFloat32 speed2 = body->m_veloc.DotProduct(body->m_veloc).GetScalar();
				const dgFloat32 omega2 = body->m_omega.DotProduct(body->m_omega).GetScalar();

				maxAccel = dgMax (maxAccel, accel2);
				maxAlpha = dgMax (maxAlpha, alpha2);
				maxSpeed = dgMax (maxSpeed, speed2);
				maxOmega = dgMax (maxOmega, omega2);

				bool equilibrium = (accel2 < accelFreeze) && (alpha2 < accelFreeze) && (speed2 < speedFreeze) && (omega2 < speedFreeze);
				if (equilibrium) {
					dgVector veloc (body->m_veloc * forceDampVect);
					dgVector omega = body->m_omega * forceDampVect;
					body->m_veloc = (veloc.DotProduct(veloc) > m_velocTol) & veloc;
					body->m_omega = (omega.DotProduct(omega) > m_velocTol) & omega;

				}
				body->m_equilibrium = equilibrium ? 1 : 0;
				stackSleeping &= equilibrium;
				isAutoSleep &= body->m_autoSleep;

				sleepCounter = dgMin (sleepCounter, body->m_sleepingCounter);
			}
			// clear accel and angular acceleration
			body->m_accel = dgVector::m_zero;
			body->m_alpha = dgVector::m_zero;
		}

		if (isAutoSleep) {
			if (stackSleeping) {
				// the cluster went to sleep mode, 
				for (dgInt32 i = 1; i < bodyCount; i ++) {
					dgBody* const body = bodyArray[i].m_body;
					dgAssert (body->IsRTTIType (dgBody::m_dynamicBodyRTTI) || body->IsRTTIType (dgBody::m_kinematicBodyRTTI));
					body->m_accel = dgVector::m_zero;
					body->m_alpha = dgVector::m_zero;
					body->m_veloc = dgVector::m_zero;
					body->m_omega = dgVector::m_zero;
				}
			} else {
				// cluster is not sleeping but may be resting with small residual velocity for a long time
				// see if we can force to go to sleep
				if ((maxAccel > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAccel) ||
					(maxAlpha > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxAlpha) ||
					(maxSpeed > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxVeloc) ||
					(maxOmega > world->m_sleepTable[DG_SLEEP_ENTRIES - 1].m_maxOmega)) {
					for (dgInt32 i = 1; i < bodyCount; i ++) {
						dgDynamicBody* const body = (dgDynamicBody*) bodyArray[i].m_body;
						if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
							body->m_sleepingCounter = 0;
						}
					}
				} else {
					dgInt32 index = 0;
					for (dgInt32 i = 0; i < DG_SLEEP_ENTRIES; i ++) {
						if ((maxAccel <= world->m_sleepTable[i].m_maxAccel) &&
							(maxAlpha <= world->m_sleepTable[i].m_maxAlpha) &&
							(maxSpeed <= world->m_sleepTable[i].m_maxVeloc) &&
							(maxOmega <= world->m_sleepTable[i].m_maxOmega)) {
								index = i;
								break;
						}
					}

					dgInt32 timeScaleSleepCount = dgInt32 (dgFloat32 (60.0f) * sleepCounter * timestep);
					if (timeScaleSleepCount > world->m_sleepTable[index].m_steps) {
						// force cluster to sleep
						stackSleeping = true;
						for (dgInt32 i = 1; i < bodyCount; i ++) {
							dgBody* const body = bodyArray[i].m_body;
							dgAssert (body->IsRTTIType (dgBody::m_dynamicBodyRTTI) || body->IsRTTIType (dgBody::m_kinematicBodyRTTI));
							body->m_accel = dgVector::m_zero;
							body->m_alpha = dgVector::m_zero;
							body->m_veloc = dgVector::m_zero;
							body->m_omega = dgVector::m_zero;
							body->m_equilibrium = 1;
						}
					} else {
						sleepCounter ++;
						for (dgInt32 i = 1; i < bodyCount; i ++) {
							dgDynamicBody* const body = (dgDynamicBody*) bodyArray[i].m_body;
							if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
								body->m_sleepingCounter = sleepCounter;
							}
						}
					}
				}
			}
		} 


		if (!(isAutoSleep & stackSleeping)) {
			// cluster is not sleeping, need to integrate cluster velocity
			const dgUnsigned32 lru = world->GetBroadPhase()->m_lru;
			const dgInt32 jointCount = cluster->m_jointCount;

			dgFloat32 timeRemaining = timestep;
			const dgFloat32 timeTol = dgFloat32 (0.01f) * timestep;
			for (dgInt32 i = 0; (i < DG_MAX_CONTINUE_COLLISON_STEPS) && (timeRemaining > timeTol); i ++) {
				// calculate the closest time to impact 
				dgFloat32 timeToImpact = timeRemaining;
				for (dgInt32 j = 0; (j < jointCount) && (timeToImpact > timeTol); j ++) {
					dgContact* const contact = (dgContact*) constraintArray[j].m_joint;
					if (contact->GetId() == dgConstraint::m_contactConstraint) {
						dgDynamicBody* const body0 = (dgDynamicBody*)contact->m_body0;
						dgDynamicBody* const body1 = (dgDynamicBody*)contact->m_body1;
						if (body0->m_continueCollisionMode | body1->m_continueCollisionMode) {
							dgVector p;
							dgVector q;
							dgVector normal;
							timeToImpact = dgMin (timeToImpact, world->CalculateTimeToImpact (contact, timeToImpact, threadID, p, q, normal, dgFloat32 (-1.0f / 256.0f)));
						}
					}
				}

				if (timeToImpact > timeTol) {
					timeRemaining -= timeToImpact;
					for (dgInt32 j = 1; j < bodyCount; j ++) {
						dgDynamicBody* const body = (dgDynamicBody*) bodyArray[j].m_body;
						if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
							body->IntegrateVelocity(timeToImpact);
							body->UpdateWorlCollisionMatrix();
						}
					}
				} else {
					if (timeToImpact >= dgFloat32 (-1.0e-5f)) {
						for (dgInt32 j = 1; j < bodyCount; j++) {
							dgDynamicBody* const body = (dgDynamicBody*)bodyArray[j].m_body;
							if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
								body->IntegrateVelocity(timeToImpact);
								body->UpdateWorlCollisionMatrix();
							}
						}
					}

					CalculateClusterContacts (cluster, timeRemaining, lru, threadID);
					BuildJacobianMatrix (cluster, threadID, 0.0f);
					IntegrateReactionsForces (cluster, threadID, 0.0f);

					bool clusterReceding = true;
					const dgFloat32 step = timestep * dgFloat32 (1.0f / DG_MAX_CONTINUE_COLLISON_STEPS); 
					for (dgInt32 k = 0; (k < DG_MAX_CONTINUE_COLLISON_STEPS) && clusterReceding; k ++) {
						dgFloat32 smallTimeStep = dgMin (step, timeRemaining);
						timeRemaining -= smallTimeStep;
						for (dgInt32 j = 1; j < bodyCount; j ++) {
							dgDynamicBody* const body = (dgDynamicBody*) bodyArray[j].m_body;
							if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
								body->IntegrateVelocity (smallTimeStep);
								body->UpdateWorlCollisionMatrix();
							}
						}

						clusterReceding = false;
						if (timeRemaining > timeTol) {
							CalculateClusterContacts (cluster, timeRemaining, lru, threadID);

							bool isColliding = false;
							for (dgInt32 j = 0; (j < jointCount) && !isColliding; j ++) {
								dgContact* const contact = (dgContact*) constraintArray[j].m_joint;
								if (contact->GetId() == dgConstraint::m_contactConstraint) {

									const dgBody* const body0 = contact->m_body0;
									const dgBody* const body1 = contact->m_body1;

									const dgVector& veloc0 = body0->m_veloc;
									const dgVector& veloc1 = body1->m_veloc;

									const dgVector& omega0 = body0->m_omega;
									const dgVector& omega1 = body1->m_omega;

									const dgVector& com0 = body0->m_globalCentreOfMass;
									const dgVector& com1 = body1->m_globalCentreOfMass;
									
									for (dgList<dgContactMaterial>::dgListNode* node = contact->GetFirst(); node; node = node->GetNext()) {
										const dgContactMaterial* const contactMaterial = &node->GetInfo();
										dgVector vel0 (veloc0 + omega0.CrossProduct(contactMaterial->m_point - com0));
										dgVector vel1 (veloc1 + omega1.CrossProduct(contactMaterial->m_point - com1));
										dgVector vRel (vel0 - vel1);
										dgAssert (contactMaterial->m_normal.m_w == dgFloat32 (0.0f));
										dgFloat32 speed = vRel.DotProduct(contactMaterial->m_normal).m_w;
										isColliding |= (speed < dgFloat32 (0.0f));
									}
								}
							}
							clusterReceding = !isColliding;
						}
					}
				}
			}

			if (timeRemaining > dgFloat32 (0.0)) {
				for (dgInt32 j = 1; j < bodyCount; j ++) {
					dgDynamicBody* const body = (dgDynamicBody*) bodyArray[j].m_body;
					if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
						body->IntegrateVelocity(timeRemaining);
						body->UpdateCollisionMatrix (timeRemaining, threadID);
					}
				}
			} else {
				for (dgInt32 j = 1; j < bodyCount; j ++) {
					dgDynamicBody* const body = (dgDynamicBody*) bodyArray[j].m_body;
					if (body->IsRTTIType (dgBody::m_dynamicBodyRTTI)) {
						body->UpdateCollisionMatrix (timestep, threadID);
					}
				}
			}
		}
	}
}

void dgWorldDynamicUpdate::CalculateClusterContacts(dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 currLru, dgInt32 threadID) const
{
	dgWorld* const world = (dgWorld*) this;
	dgInt32 jointCount = cluster->m_jointCount;
	dgJointInfo* const constraintArrayPtr = &world->m_jointsMemory[0];
	dgJointInfo* const constraintArray = &constraintArrayPtr[cluster->m_jointStart];

	dgBroadPhase::dgPair pair;
	dgContactPoint contactArray[DG_MAX_CONTATCS];
	for (dgInt32 j = 0; (j < jointCount); j ++) {
		dgContact* const contact = (dgContact*) constraintArray[j].m_joint;
		if (contact->GetId() == dgConstraint::m_contactConstraint) {
			const dgContactMaterial* const material = contact->m_material;
			if (material->m_flags & dgContactMaterial::m_collisionEnable) {
				dgInt32 processContacts = 1;
				if (material->m_aabbOverlap) {
					//processContacts = material->m_aabbOverlap (*material, *contact->GetBody0(), *contact->GetBody1(), threadID);
					processContacts = material->m_aabbOverlap(*contact, timestep, threadID);
				}

				if (processContacts) {
					contact->m_maxDOF = 0;
					contact->m_broadphaseLru = currLru;
					pair.m_contact = contact;
					pair.m_cacheIsValid = false;
					pair.m_timestep = timestep;
					pair.m_contactBuffer = contactArray;
					world->CalculateContacts (&pair, threadID, false, false);
					if (pair.m_contactCount) {
						dgAssert (pair.m_contactCount <= (DG_CONSTRAINT_MAX_ROWS / 3));
						world->ProcessContacts (&pair, threadID);
					}
				}
			}
		}
	}
}



void dgWorldDynamicUpdate::IntegrateExternalForce(const dgBodyCluster* const cluster, dgFloat32 timestep, dgInt32 threadID) const
{
	dgWorld* const world = (dgWorld*) this;
	//dgBodyInfo* const bodyArrayPtr = (dgBodyInfo*) &world->m_bodiesMemory[0]; 
	//dgBodyInfo* const bodyArray = &bodyArrayPtr[cluster->m_bodyStart];
	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];

	dgAssert (timestep > dgFloat32 (0.0f));
	const dgInt32 bodyCount = cluster->m_bodyCount;
	for (dgInt32 i = 1; i < bodyCount; i ++) {
		dgDynamicBody* const body = (dgDynamicBody*) bodyArray[i].m_body;
		body->AddDampingAcceleration(timestep);
		body->IntegrateOpenLoopExternalForce(timestep);
	}
}

void dgWorldDynamicUpdate::CalculateNetAcceleration(dgBody* const body, const dgVector& invTimeStep, const dgVector& maxAccNorm2) const
{
	dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBodyRTTI));
	// the initial velocity and angular velocity were stored in m_accel and body->m_alpha for memory saving
	dgVector accel (invTimeStep * (body->m_veloc - body->m_accel));
	dgVector alpha (invTimeStep * (body->m_omega - body->m_alpha));
	dgVector accelTest((accel.DotProduct(accel) > maxAccNorm2) | (alpha.DotProduct(alpha) > maxAccNorm2));
	accel = accel & accelTest;
	alpha = alpha & accelTest;

	body->m_accel = accel;
	body->m_alpha = alpha;
}


void dgWorldDynamicUpdate::IntegrateReactionsForces(const dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const
{
	if (cluster->m_jointCount == 0) {
		IntegrateExternalForce(cluster, timestep, threadID);
	} else {
		CalculateClusterReactionForces(cluster, threadID, timestep);
	}
}


dgFloat32 dgWorldDynamicUpdate::CalculateJointForce_3_13(const dgJointInfo* const jointInfo, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide) const
{
	dgVector accNorm(dgVector::m_zero);
	const dgInt32 m0 = jointInfo->m_m0;
	const dgInt32 m1 = jointInfo->m_m1;
	const dgBody* const body0 = bodyArray[m0].m_body;
	const dgBody* const body1 = bodyArray[m1].m_body;

	if (!(body0->m_resting & body1->m_resting)) {
		dgFloat32 normalForce[DG_CONSTRAINT_MAX_ROWS + 1];
		dgVector linearM0(internalForces[m0].m_linear);
		dgVector angularM0(internalForces[m0].m_angular);
		dgVector linearM1(internalForces[m1].m_linear);
		dgVector angularM1(internalForces[m1].m_angular);

		const dgVector preconditioner0(jointInfo->m_preconditioner0);
		const dgVector preconditioner1(jointInfo->m_preconditioner1);

		const dgInt32 index = jointInfo->m_pairStart;
		const dgInt32 rowsCount = jointInfo->m_pairCount;

		normalForce[0] = dgFloat32 (1.0f);
		dgVector firstPass(dgVector::m_one);
		dgVector maxAccel(dgVector::m_three);
		const dgFloat32 restAcceleration = DG_SOLVER_MAX_ERROR * DG_SOLVER_MAX_ERROR * dgFloat32(4.0f);
		for (dgInt32 i = 0; (i < 4) && (maxAccel.GetScalar() > restAcceleration); i++) {
			maxAccel = dgFloat32(0.0f);
			for (dgInt32 j = 0; j < rowsCount; j++) {
				dgRightHandSide* const rhs = &rightHandSide[index + j];
				const dgLeftHandSide* const row = &matrixRow[index + j];

				dgAssert(row->m_Jt.m_jacobianM0.m_linear.m_w == dgFloat32(0.0f));
				dgAssert(row->m_Jt.m_jacobianM0.m_angular.m_w == dgFloat32(0.0f));
				dgAssert(row->m_Jt.m_jacobianM1.m_linear.m_w == dgFloat32(0.0f));
				dgAssert(row->m_Jt.m_jacobianM1.m_angular.m_w == dgFloat32(0.0f));

				dgVector diag(row->m_JMinv.m_jacobianM0.m_linear * linearM0 + row->m_JMinv.m_jacobianM0.m_angular * angularM0 +
							  row->m_JMinv.m_jacobianM1.m_linear * linearM1 + row->m_JMinv.m_jacobianM1.m_angular * angularM1);

				dgVector accel(rhs->m_coordenateAccel - rhs->m_force * rhs->m_diagDamp - (diag.AddHorizontal()).GetScalar());
				dgVector force(rhs->m_force + rhs->m_invJinvMJt * accel.GetScalar());

				dgAssert (rhs->m_normalForceIndex >= -1);
				dgAssert (rhs->m_normalForceIndex <= rowsCount);
				dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;

				dgFloat32 frictionNormal = normalForce[frictionIndex];
				dgVector lowerFrictionForce(frictionNormal * rhs->m_lowerBoundFrictionCoefficent);
				dgVector upperFrictionForce(frictionNormal * rhs->m_upperBoundFrictionCoefficent);

				accel = accel.AndNot((force > upperFrictionForce) | (force < lowerFrictionForce));
				force = force.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

				maxAccel = maxAccel.GetMax(accel.Abs());
				dgAssert(maxAccel.m_x >= dgAbs(accel.m_x));

				accNorm = accNorm.GetMax(maxAccel * firstPass);

				dgVector deltaForce(force - dgVector(rhs->m_force));
				rhs->m_force = force.GetScalar();
				normalForce[j + 1] = force.GetScalar();

				dgVector deltaforce0(preconditioner0 * deltaForce);
				dgVector deltaforce1(preconditioner1 * deltaForce);

				linearM0 += row->m_Jt.m_jacobianM0.m_linear * deltaforce0;
				angularM0 += row->m_Jt.m_jacobianM0.m_angular * deltaforce0;
				linearM1 += row->m_Jt.m_jacobianM1.m_linear * deltaforce1;
				angularM1 += row->m_Jt.m_jacobianM1.m_angular * deltaforce1;
			}
			firstPass = dgVector::m_zero;
		}

		for (dgInt32 i = 0; i < rowsCount; i++) {
			//dgLeftHandSide* const row = &matrixRow[index + i];
			dgRightHandSide* const rhs = &rightHandSide[index + i];
			rhs->m_maxImpact = dgMax(dgAbs(rhs->m_force), rhs->m_maxImpact);
		}

		internalForces[m0].m_linear = linearM0;
		internalForces[m0].m_angular = angularM0;
		internalForces[m1].m_linear = linearM1;
		internalForces[m1].m_angular = angularM1;
	}

	return accNorm.GetScalar() * accNorm.GetScalar();
}

dgFloat32 dgWorldDynamicUpdate::CalculateJointForce(const dgJointInfo* const jointInfo, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide) const
{
	dgVector accNorm(dgVector::m_zero);
	dgFloat32 normalForce[DG_CONSTRAINT_MAX_ROWS + 4];

	const dgInt32 m0 = jointInfo->m_m0;
	const dgInt32 m1 = jointInfo->m_m1;
	const dgBody* const body0 = bodyArray[m0].m_body;
	const dgBody* const body1 = bodyArray[m1].m_body;

	if (!(body0->m_resting & body1->m_resting)) {
		dgInt32 rowsCount = jointInfo->m_pairCount;

		dgVector linearM0(internalForces[m0].m_linear);
		dgVector angularM0(internalForces[m0].m_angular);
		dgVector linearM1(internalForces[m1].m_linear);
		dgVector angularM1(internalForces[m1].m_angular);

		const dgVector preconditioner0(jointInfo->m_preconditioner0);
		const dgVector preconditioner1(jointInfo->m_preconditioner1);

		normalForce[0] = dgFloat32(1.0f);
		const dgInt32 rowStart = jointInfo->m_pairStart;
		for (dgInt32 j = 0; j < rowsCount; j++) {
			dgRightHandSide* const rhs = &rightHandSide[rowStart + j];
			const dgLeftHandSide* const row = &matrixRow[rowStart + j];
			dgVector a (row->m_JMinv.m_jacobianM0.m_linear * linearM0);
			a = a.MulAdd(row->m_JMinv.m_jacobianM0.m_angular, angularM0);
			a = a.MulAdd(row->m_JMinv.m_jacobianM1.m_linear, linearM1);
			a = a.MulAdd(row->m_JMinv.m_jacobianM1.m_angular, angularM1);
			a = dgVector(rhs->m_coordenateAccel + rhs->m_gyroAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();

			dgVector f(rhs->m_force + rhs->m_invJinvMJt * a.GetScalar());
			dgAssert(rhs->m_normalForceIndex >= -1);
			dgAssert(rhs->m_normalForceIndex <= rowsCount);
			dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;

			dgFloat32 frictionNormal = normalForce[frictionIndex];
			dgVector lowerFrictionForce(frictionNormal * rhs->m_lowerBoundFrictionCoefficent);
			dgVector upperFrictionForce(frictionNormal * rhs->m_upperBoundFrictionCoefficent);

			a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
			f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

			//accNorm += a * a;
			accNorm = accNorm.MulAdd(a, a);
			dgVector deltaForce(f - dgVector(rhs->m_force));

			rhs->m_force = f.GetScalar();
			normalForce[j + 1] = f.GetScalar();

			dgVector deltaforce0(preconditioner0 * deltaForce);
			dgVector deltaforce1(preconditioner1 * deltaForce);
			linearM0 = linearM0.MulAdd(row->m_Jt.m_jacobianM0.m_linear, deltaforce0);
			angularM0 = angularM0.MulAdd(row->m_Jt.m_jacobianM0.m_angular, deltaforce0);
			linearM1 = linearM1.MulAdd(row->m_Jt.m_jacobianM1.m_linear, deltaforce1);
			angularM1 = angularM1.MulAdd(row->m_Jt.m_jacobianM1.m_angular, deltaforce1);
		}

		dgVector maxAccel(accNorm);
		const dgFloat32 tol = dgFloat32(0.5f);
		const dgFloat32 tol2 = tol * tol;
		for (dgInt32 i = 0; (i < 4) && (maxAccel.GetScalar() > tol2); i++) {
			maxAccel = dgVector::m_zero;
			for (dgInt32 j = 0; j < rowsCount; j++) {
				dgRightHandSide* const rhs = &rightHandSide[rowStart + j];
				const dgLeftHandSide* const row = &matrixRow[rowStart + j];
				dgVector a(row->m_JMinv.m_jacobianM0.m_linear * linearM0);
				a = a.MulAdd(row->m_JMinv.m_jacobianM0.m_angular, angularM0);
				a = a.MulAdd(row->m_JMinv.m_jacobianM1.m_linear, linearM1);
				a = a.MulAdd(row->m_JMinv.m_jacobianM1.m_angular, angularM1);
				a = dgVector(rhs->m_coordenateAccel + rhs->m_gyroAccel - rhs->m_force * rhs->m_diagDamp) - a.AddHorizontal();

				dgVector f(rhs->m_force + rhs->m_invJinvMJt * a.GetScalar());
				dgAssert(rhs->m_normalForceIndex >= -1);
				dgAssert(rhs->m_normalForceIndex <= rowsCount);
				dgInt32 frictionIndex = rhs->m_normalForceIndex + 1;

				dgFloat32 frictionNormal = normalForce[frictionIndex];
				dgVector lowerFrictionForce(frictionNormal * rhs->m_lowerBoundFrictionCoefficent);
				dgVector upperFrictionForce(frictionNormal * rhs->m_upperBoundFrictionCoefficent);

				a = a.AndNot((f > upperFrictionForce) | (f < lowerFrictionForce));
				f = f.GetMax(lowerFrictionForce).GetMin(upperFrictionForce);

				//maxAccel += a * a;
				maxAccel = maxAccel.MulAdd (a, a);

				dgVector deltaForce(f - dgVector(rhs->m_force));

				rhs->m_force = f.GetScalar();
				normalForce[j + 1] = f.GetScalar();

				dgVector deltaforce0(preconditioner0 * deltaForce);
				dgVector deltaforce1(preconditioner1 * deltaForce);
				linearM0 = linearM0.MulAdd(row->m_Jt.m_jacobianM0.m_linear, deltaforce0);
				angularM0 = angularM0.MulAdd(row->m_Jt.m_jacobianM0.m_angular, deltaforce0);
				linearM1 = linearM1.MulAdd(row->m_Jt.m_jacobianM1.m_linear, deltaforce1);
				angularM1 = angularM1.MulAdd(row->m_Jt.m_jacobianM1.m_angular, deltaforce1);
			}
		}

		for (dgInt32 i = 0; i < rowsCount; i++) {
			dgRightHandSide* const rhs = &rightHandSide[rowStart + i];
			rhs->m_maxImpact = dgMax(dgAbs(rhs->m_force), rhs->m_maxImpact);
		}

		internalForces[m0].m_linear = linearM0;
		internalForces[m0].m_angular = angularM0;
		internalForces[m1].m_linear = linearM1;
		internalForces[m1].m_angular = angularM1;
	}
	return accNorm.GetScalar();
}


dgJacobian dgWorldDynamicUpdate::IntegrateForceAndToque(dgDynamicBody* const body, const dgVector& force, const dgVector& torque, const dgVector& timestep) const
{
	dgJacobian velocStep;
	
#ifdef DG_TEST_GYRO
	dgVector dtHalf(timestep * dgVector::m_half);
	dgMatrix matrix(body->m_gyroRotation, dgVector::m_wOne);
	dgVector localOmega(matrix.UnrotateVector(body->m_omega));
	dgVector localTorque(matrix.UnrotateVector(torque));

	// and solving for alpha we get the angular acceleration at t + dt
	// calculate gradient at a full time step
	dgVector gradientStep(localTorque * timestep);

	// derivative at half time step. (similar to midpoint Euler so that it does not loses too much energy)
	dgVector dw(localOmega * dtHalf);

	dgVector inertia (body->m_mass);
	dgFloat32 jacobianMatrix[3][3];

	jacobianMatrix[0][0] = inertia[0];
	jacobianMatrix[0][1] = (inertia[2] - inertia[1]) * dw[2];
	jacobianMatrix[0][2] = (inertia[2] - inertia[1]) * dw[1];

	jacobianMatrix[1][0] = (inertia[0] - inertia[2]) * dw[2];
	jacobianMatrix[1][1] = inertia[1];
	jacobianMatrix[1][2] = (inertia[0] - inertia[2]) * dw[0];

	jacobianMatrix[2][0] = (inertia[1] - inertia[0]) * dw[1];
	jacobianMatrix[2][1] = (inertia[1] - inertia[0]) * dw[0];
	jacobianMatrix[2][2] = inertia[2];

	dgAssert(jacobianMatrix[0][0] > dgFloat32(0.0f));
	dgFloat32 den = dgFloat32(1.0f) / jacobianMatrix[0][0];
	dgFloat32 scale = jacobianMatrix[1][0] * den;
	jacobianMatrix[1][0] -= jacobianMatrix[0][0] * scale;
	jacobianMatrix[1][1] -= jacobianMatrix[0][1] * scale;
	jacobianMatrix[1][2] -= jacobianMatrix[0][2] * scale;
	gradientStep[1] -= gradientStep[0] * scale;

	scale = jacobianMatrix[2][0] * den;
	jacobianMatrix[2][0] -= jacobianMatrix[0][0] * scale;
	jacobianMatrix[2][1] -= jacobianMatrix[0][1] * scale;
	jacobianMatrix[2][2] -= jacobianMatrix[0][2] * scale;
	gradientStep[2] -= gradientStep[0] * scale;

	dgAssert(jacobianMatrix[1][1] > dgFloat32(0.0f));
	scale = jacobianMatrix[2][1] / jacobianMatrix[1][1];
	jacobianMatrix[2][1] -= jacobianMatrix[1][1] * scale;
	jacobianMatrix[2][2] -= jacobianMatrix[1][2] * scale;
	gradientStep[2] -= gradientStep[1] * scale;

	dgAssert(jacobianMatrix[2][2] > dgFloat32(0.0f));
	gradientStep[2] = gradientStep[2] / jacobianMatrix[2][2];
	gradientStep[1] = (gradientStep[1] - jacobianMatrix[1][2] * gradientStep[2]) / jacobianMatrix[1][1];
	gradientStep[0] = (gradientStep[0] - jacobianMatrix[0][1] * gradientStep[1] - jacobianMatrix[0][2] * gradientStep[2]) / jacobianMatrix[0][0];

	dgVector omega (matrix.RotateVector(localOmega + gradientStep));
	dgAssert(omega.m_w == dgFloat32(0.0f));

	// integrate rotation here
	dgFloat32 omegaMag2 = omega.DotProduct(omega).GetScalar() + dgFloat32(1.0e-12f);
	dgFloat32 invOmegaMag = dgRsqrt(omegaMag2);
	dgVector omegaAxis(omega.Scale(invOmegaMag));
	dgFloat32 omegaAngle = invOmegaMag * omegaMag2 * timestep.GetScalar();
	dgQuaternion deltaRotation(omegaAxis, omegaAngle);
	body->m_gyroRotation = body->m_gyroRotation * deltaRotation;
	dgAssert((body->m_gyroRotation.DotProduct(body->m_gyroRotation) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f));

	// calculate new gyro torque
	matrix = dgMatrix (body->m_gyroRotation, dgVector::m_wOne);
	localOmega = matrix.UnrotateVector(omega);
	dgVector angularMomentum(inertia * localOmega);
	body->m_gyroTorque = matrix.RotateVector(localOmega.CrossProduct(angularMomentum));
	
	velocStep.m_angular = matrix.RotateVector(gradientStep);
#else
	//dgVector externTorque(torque - body->m_gyroToque);
	dgVector externTorque(torque);
	velocStep.m_angular = body->m_invWorldInertiaMatrix.RotateVector(externTorque) * timestep;
#endif
	velocStep.m_linear = force.Scale(body->m_invMass.m_w) * timestep;
	return velocStep;
}

void dgWorldDynamicUpdate::CalculateClusterReactionForces(const dgBodyCluster* const cluster, dgInt32 threadID, dgFloat32 timestep) const
{
	dgWorld* const world = (dgWorld*) this;
	const dgInt32 bodyCount = cluster->m_bodyCount;
	const dgInt32 jointCount = cluster->m_jointCount;

	dgJacobian* const internalForces = &m_solverMemory.m_internalForcesBuffer[cluster->m_bodyStart];
	dgBodyInfo* const bodyArray = &world->m_bodiesMemory[cluster->m_bodyStart];
	dgJointInfo* const constraintArray = &world->m_jointsMemory[cluster->m_jointStart];

	dgRightHandSide* const rightHandSide = &m_solverMemory.m_righHandSizeBuffer[cluster->m_rowStart];
	const dgLeftHandSide* const leftHandSide = &m_solverMemory.m_leftHandSizeBuffer[cluster->m_rowStart];

	const dgInt32 derivativesEvaluationsRK4 = 4;
	dgFloat32 invTimestep = (timestep > dgFloat32(0.0f)) ? dgFloat32(1.0f) / timestep : dgFloat32(0.0f);
	dgFloat32 invStepRK = (dgFloat32(1.0f) / dgFloat32(derivativesEvaluationsRK4));
	dgFloat32 timestepRK = timestep * invStepRK;
	dgFloat32 invTimestepRK = invTimestep * dgFloat32(derivativesEvaluationsRK4);
	dgAssert(bodyArray[0].m_body == world->m_sentinelBody);

	dgVector speedFreeze2(world->m_freezeSpeed2 * dgFloat32(0.1f));
	dgVector freezeOmega2(world->m_freezeOmega2 * dgFloat32(0.1f));

	dgJointAccelerationDecriptor joindDesc;
	joindDesc.m_timeStep = timestepRK;
	joindDesc.m_invTimeStep = invTimestepRK;
	joindDesc.m_firstPassCoefFlag = dgFloat32(0.0f);

	dgInt32 skeletonCount = 0;
	dgSkeletonContainer* skeletonArray[DG_MAX_SKELETON_JOINT_COUNT];
#ifdef DG_USE_SKEL
	dgInt32 lru = dgAtomicExchangeAndAdd(&dgSkeletonContainer::m_lruMarker, 1);
	for (dgInt32 i = 1; i < bodyCount; i++) {
		dgDynamicBody* const body = (dgDynamicBody*)bodyArray[i].m_body;
		dgSkeletonContainer* const container = body->GetSkeleton();
		if (container && (container->m_lru != lru)) {
			container->m_lru = lru;
			skeletonArray[skeletonCount] = container;
			container->InitMassMatrix(constraintArray, leftHandSide, rightHandSide);
			skeletonCount++;
			dgAssert(skeletonCount < dgInt32(sizeof(skeletonArray) / sizeof(skeletonArray[0])));
		}
	}
#endif

	const dgInt32 passes = world->m_solverIterations;
	for (dgInt32 step = 0; step < derivativesEvaluationsRK4; step++) {

		for (dgInt32 i = 0; i < jointCount; i++) {
			dgJointInfo* const jointInfo = &constraintArray[i];
			dgConstraint* const constraint = jointInfo->m_joint;
			const dgInt32 pairStart = jointInfo->m_pairStart;

			joindDesc.m_rowsCount = jointInfo->m_pairCount;
			joindDesc.m_leftHandSide = &leftHandSide[pairStart];
			joindDesc.m_rightHandSide = &rightHandSide[pairStart];
			constraint->JointAccelerations(&joindDesc);
			
			const dgVector& gyroTorque0 = constraint->m_body0->m_gyroTorque;
			const dgVector& gyroTorque1 = constraint->m_body1->m_gyroTorque;
			for (dgInt32 j = 0; j < jointInfo->m_pairCount; j++) {
				dgRightHandSide* const rhs = &rightHandSide[pairStart + j];
				const dgLeftHandSide* const row = &leftHandSide[pairStart + j];

				dgVector gyroaccel(row->m_JMinv.m_jacobianM0.m_angular * gyroTorque0 + row->m_JMinv.m_jacobianM1.m_angular * gyroTorque1);
				rhs->m_gyroAccel = gyroaccel.AddHorizontal().GetScalar();
#ifndef DG_TEST_GYRO
				rhs->m_gyroAccel = dgFloat32 (0.0f);
#endif
			}
		}
		joindDesc.m_firstPassCoefFlag = dgFloat32(1.0f);

		dgFloat32 maxAccNorm = DG_SOLVER_MAX_ERROR * DG_SOLVER_MAX_ERROR;
		dgFloat32 accNorm = maxAccNorm * dgFloat32(2.0f);

		for (dgInt32 i = 0; (i < passes) && (accNorm > maxAccNorm); i++) {
			accNorm = dgFloat32(0.0f);
			for (dgInt32 j = 0; j < jointCount; j++) {
				dgJointInfo* const jointInfo = &constraintArray[j];
#ifdef DG_USE_SKEL
				if (!jointInfo->m_joint->IsSkeleton()) 
#endif
				{
					//dgFloat32 accel2 = CalculateJointForce_3_13(jointInfo, bodyArray, internalForces, leftHandSide);
					dgFloat32 accel2 = CalculateJointForce(jointInfo, bodyArray, internalForces, leftHandSide, rightHandSide);
					accNorm += accel2;
				}
			}
			for (dgInt32 j = 0; j < skeletonCount; j++) {
				skeletonArray[j]->CalculateJointForce(constraintArray, bodyArray, internalForces);
			}
		}

		if (timestepRK != dgFloat32(0.0f)) {
			dgVector timestep4(timestepRK);
			for (dgInt32 i = 1; i < bodyCount; i++) {
				dgDynamicBody* const body = (dgDynamicBody*)bodyArray[i].m_body;
				dgAssert(body->m_index == i);
				if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
					const dgVector force(internalForces[i].m_linear + body->m_externalForce);
					const dgVector torque(internalForces[i].m_angular + body->m_externalTorque - body->m_gyroTorque);
					dgJacobian velocStep(IntegrateForceAndToque(body, force, torque, timestep4));
					if (!body->m_resting) {
						body->m_veloc += velocStep.m_linear;
						body->m_omega += velocStep.m_angular;
					} else {
						const dgVector velocStep2(velocStep.m_linear.DotProduct(velocStep.m_linear));
						const dgVector omegaStep2(velocStep.m_angular.DotProduct(velocStep.m_angular));
						const dgVector test(((velocStep2 > speedFreeze2) | (omegaStep2 > speedFreeze2)) & dgVector::m_negOne);
						const dgInt32 equilibrium = test.GetSignMask() ? 0 : 1;
						body->m_resting &= equilibrium;
					}

					dgAssert(body->m_veloc.m_w == dgFloat32(0.0f));
					dgAssert(body->m_omega.m_w == dgFloat32(0.0f));
				}
			}
		} else {
			for (dgInt32 i = 1; i < bodyCount; i++) {
				dgDynamicBody* const body = (dgDynamicBody*)bodyArray[i].m_body;
				const dgVector& linearMomentum = internalForces[i].m_linear;
				const dgVector& angularMomentum = internalForces[i].m_angular;

				body->m_veloc += linearMomentum.Scale(body->m_invMass.m_w);
				body->m_omega += body->m_invWorldInertiaMatrix.RotateVector(angularMomentum);
			}
		}
	}

	dgInt32 hasJointFeeback = 0;
	if (timestepRK != dgFloat32(0.0f)) {
		for (dgInt32 i = 0; i < jointCount; i++) {
			dgJointInfo* const jointInfo = &constraintArray[i];
			dgConstraint* const constraint = jointInfo->m_joint;

			const dgInt32 first = jointInfo->m_pairStart;
			const dgInt32 count = jointInfo->m_pairCount;

			for (dgInt32 j = 0; j < count; j++) {
				dgRightHandSide* const rhs = &rightHandSide[j + first];
				//const dgLeftHandSide* const row = &leftHandSide[j + first];
				dgAssert(dgCheckFloat(rhs->m_force));
				rhs->m_jointFeebackForce->m_force = rhs->m_force;
				rhs->m_jointFeebackForce->m_impact = rhs->m_maxImpact * timestepRK;
			}
			hasJointFeeback |= (constraint->m_updaFeedbackCallback ? 1 : 0);
		}

		const dgVector invTime(invTimestep);
		const dgVector maxAccNorm2(DG_SOLVER_MAX_ERROR * DG_SOLVER_MAX_ERROR);

		for (dgInt32 i = 1; i < bodyCount; i++) {
			dgBody* const body = bodyArray[i].m_body;
			CalculateNetAcceleration(body, invTime, maxAccNorm2);
		}

		if (hasJointFeeback) {
			for (dgInt32 i = 0; i < jointCount; i++) {
				if (constraintArray[i].m_joint->m_updaFeedbackCallback) {
					constraintArray[i].m_joint->m_updaFeedbackCallback(*constraintArray[i].m_joint, timestep, threadID);
				}
			}
		}
	} else {
		for (dgInt32 i = 1; i < bodyCount; i++) {
			dgBody* const body = bodyArray[i].m_body;
			dgAssert(body->IsRTTIType(dgBody::m_dynamicBodyRTTI) || body->IsRTTIType(dgBody::m_kinematicBodyRTTI));
			body->m_accel = dgVector::m_zero;
			body->m_alpha = dgVector::m_zero;
		}
	}
}

