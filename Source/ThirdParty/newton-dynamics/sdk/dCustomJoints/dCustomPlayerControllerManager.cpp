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


// NewtonCustomJoint.cpp: implementation of the NewtonCustomJoint class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomJoint.h"
#include "dCustomPlayerControllerManager.h"


#define D_DESCRETE_MOTION_STEPS				8
#define D_PLAYER_MAX_INTERGRATION_STEPS		8
#define D_PLAYER_MAX_SOLVER_ITERATIONS		16
#define D_PLAYER_CONTACT_SKIN_THICKNESS		0.025f


dCustomPlayerController::dCustomPlayerController()
{
}

dCustomPlayerController::~dCustomPlayerController()
{
	NewtonDestroyBody(m_body);
	NewtonDestroyCollision (m_castingShape);
}


void dCustomPlayerController::Init(dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dMatrix& localAxis)
{
	dAssert (stairStep >= 0.0f);
	dAssert (innerRadius >= 0.0f);
	dAssert (outerRadius >= innerRadius);
	dAssert (height >= stairStep);
	dAssert (localAxis[0].m_w == dFloat (0.0f));
	dAssert (localAxis[1].m_w == dFloat (0.0f));

	dCustomPlayerControllerManager* const manager = (dCustomPlayerControllerManager*) GetManager();
	NewtonWorld* const world = manager->GetWorld();

	SetRestrainingDistance (0.0f);

	m_outerRadio = outerRadius;
	m_innerRadio = innerRadius;
	m_height = height;
	m_stairStep = stairStep;
	SetClimbSlope(45.0f * dDegreeToRad);
	m_upVector = localAxis[0];
	m_frontVector = localAxis[1];

	m_groundPlane = dVector (0.0f);
	m_groundVelocity = dVector (0.0f);

	const int steps = 12;
	dVector convexPoints[2][steps];

	// create an inner thin cylinder
	dFloat shapeHigh = height;
	dAssert (shapeHigh > 0.0f);
	dVector p0 (0.0f, m_innerRadio, 0.0f, 0.0f);
	dVector p1 (shapeHigh, m_innerRadio, 0.0f, 0.0f);
	for (int i = 0; i < steps; i ++) {
		dMatrix rotation (dPitchMatrix (i * 2.0f * dPi / steps));
		convexPoints[0][i] = localAxis.RotateVector(rotation.RotateVector(p0));
		convexPoints[1][i] = localAxis.RotateVector(rotation.RotateVector(p1));
	}
	NewtonCollision* const supportShape = NewtonCreateConvexHull(world, steps * 2, &convexPoints[0][0].m_x, sizeof (dVector), 0.0f, 0, NULL); 

	// create the outer thick cylinder
	dMatrix outerShapeMatrix (localAxis);
	dFloat capsuleHigh = m_height - stairStep;
	dAssert (capsuleHigh > 0.0f);
	m_sphereCastOrigin = capsuleHigh * 0.5f + stairStep;
	outerShapeMatrix.m_posit = outerShapeMatrix[0].Scale(m_sphereCastOrigin);
	outerShapeMatrix.m_posit.m_w = 1.0f;
	NewtonCollision* const bodyCapsule = NewtonCreateCapsule(world, 0.25f, 0.25f, 0.5f, 0, &outerShapeMatrix[0][0]);
	NewtonCollisionSetScale(bodyCapsule, capsuleHigh, m_outerRadio * 4.0f, m_outerRadio * 4.0f);

	// compound collision player controller
	NewtonCollision* const playerShape = NewtonCreateCompoundCollision(world, 0);
	NewtonCompoundCollisionBeginAddRemove(playerShape);	
	NewtonCompoundCollisionAddSubCollision (playerShape, supportShape);
	NewtonCompoundCollisionAddSubCollision (playerShape, bodyCapsule);
	NewtonCompoundCollisionEndAddRemove (playerShape);	

	// create the kinematic body
	dMatrix locationMatrix (dGetIdentityMatrix());
	m_body = NewtonCreateKinematicBody(world, playerShape, &locationMatrix[0][0]);

	// players must have weight, otherwise they are infinitely strong when they collide
	NewtonCollision* const shape = NewtonBodyGetCollision(m_body);
	NewtonBodySetMassProperties(m_body, mass, shape);

	// make the body collidable with other dynamics bodies, by default
	NewtonBodySetCollidable (m_body, true);

	dFloat castHigh = capsuleHigh * 0.4f;
	dFloat castRadio = (m_innerRadio * 0.5f > 0.05f) ? m_innerRadio * 0.5f : 0.05f;

	dVector q0 (0.0f, castRadio, 0.0f, 0.0f);
	dVector q1 (castHigh, castRadio, 0.0f, 0.0f);
	for (int i = 0; i < steps; i ++) {
		dMatrix rotation (dPitchMatrix (i * 2.0f * dPi / steps));
		convexPoints[0][i] = localAxis.RotateVector(rotation.RotateVector(q0));
		convexPoints[1][i] = localAxis.RotateVector(rotation.RotateVector(q1));
	}
	m_castingShape = NewtonCreateConvexHull(world, steps * 2, &convexPoints[0][0].m_x, sizeof (dVector), 0.0f, 0, NULL); 

	m_supportShape = NewtonCompoundCollisionGetCollisionFromNode (shape, NewtonCompoundCollisionGetNodeByIndex (shape, 0));
	m_upperBodyShape = NewtonCompoundCollisionGetCollisionFromNode (shape, NewtonCompoundCollisionGetNodeByIndex (shape, 1));

	NewtonDestroyCollision (bodyCapsule);
	NewtonDestroyCollision (supportShape);
	NewtonDestroyCollision (playerShape);

	m_isJumping = false;
}



dCustomPlayerControllerManager::dCustomPlayerControllerManager(NewtonWorld* const world)
	:dCustomControllerManager<dCustomPlayerController> (world, PLAYER_PLUGIN_NAME)
{

}

dCustomPlayerControllerManager::~dCustomPlayerControllerManager()
{
}

dCustomPlayerController* dCustomPlayerControllerManager::CreatePlayer (dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dMatrix& localAxis)
{
	dCustomPlayerController* const controller = CreateController ();
	controller->Init (mass, outerRadius, innerRadius, height, stairStep, localAxis);	
	return controller;
}




void dCustomPlayerController::SetPlayerOrigin (dFloat originHeight)
{
/*
	NewtonCollision* const playerShape = NewtonBodyGetCollision(m_body);
	NewtonCompoundCollisionBeginAddRemove(playerShape);	

		dMatrix supportShapeMatrix (dGetIdentityMatrix());
		supportShapeMatrix[0] = m_upVector;
		supportShapeMatrix[1] = m_frontVector;
		supportShapeMatrix[2] = supportShapeMatrix[0] * supportShapeMatrix[1];
		supportShapeMatrix.m_posit = supportShapeMatrix[0].Scale(m_height * 0.5f - originHigh);
		supportShapeMatrix.m_posit.m_w = 1.0f;
		NewtonCollisionSetMatrix (m_supportShape, &supportShapeMatrix[0][0]);

		dMatrix collisionShapeMatrix (supportShapeMatrix);
		dFloat cylinderHeight = m_height - m_stairStep;
		dAssert (cylinderHeight > 0.0f);
		collisionShapeMatrix.m_posit = collisionShapeMatrix[0].Scale(cylinderHeight * 0.5f + m_stairStep - originHigh);
		collisionShapeMatrix.m_posit.m_w = 1.0f;
		NewtonCollisionSetMatrix (m_upperBodyShape, &collisionShapeMatrix[0][0]);

	NewtonCompoundCollisionEndAddRemove (playerShape);	

	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat mass;
	NewtonBodyGetMass(m_body, &mass, &Ixx, &Iyy, &Izz);
	NewtonBodySetMassProperties(m_body, mass, playerShape);
*/
	originHeight = dClamp (originHeight, dFloat(0.0f), m_height);
	dVector origin (m_upVector.Scale (originHeight));
	NewtonBodySetCentreOfMass (m_body, &origin[0]);
}


/*
dVector planes[PLAYER_CONTROLLER_MAX_CONTACTS];
for (int i = 0; i < contactCount; i ++) {
	planes[i] = dVector (info[i].m_normal);
	dVector p (matrix.m_posit + planes[i].Scale (radio - D_PLAYER_DISTANCE_CONTACT));
	planes[i].m_w = - (planes[i] % p);
}
int planeCount = contactCount;
for (int i = 0; i < (planeCount - 1); i ++) {
	for (int k = i + 1; k < planeCount; k ++) {
		dVector diff (planes[i] - planes[k]);
		dFloat dist = planes[i].m_w - planes[k].m_w;
		dFloat erro2 = diff % diff + dist * dist;
		if (erro2 < 1.0e-3f) {
			planes[k] = planes[planeCount - 1];
			k --;
			planeCount --;
		}
	}
}

for (int i = 0; i < planeCount; i ++) {
	dFloat t = planes[i] % matrix.m_posit + planes[i].m_w; 
	matrix.m_posit = matrix.m_posit - planes[i].Scale (t);
	matrix.m_posit -= planes[i].Scale (radio);
}
*/



int dCustomPlayerControllerManager::ProcessContacts (const dCustomPlayerController* const controller, NewtonWorldConvexCastReturnInfo* const contacts, int contactCount) const 
{
	//	for (int i = 0; i < contactCount; i ++) {
	//		const dgVector normal = contacts[i].m_normal;
	//		const dgVector position = contacts[i].m_normal;
	//	}
	return contactCount;
}

dVector dCustomPlayerController::CalculateDesiredOmega (dFloat headingAngle, dFloat timestep) const
{
	dQuaternion playerRotation;
	dQuaternion targetRotation (m_upVector, headingAngle);
	NewtonBodyGetRotation(m_body, &playerRotation.m_x);
	return playerRotation.CalcAverageOmega (targetRotation, 0.5f / timestep);
}

dVector dCustomPlayerController::CalculateDesiredVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, const dVector& gravity, dFloat timestep) const
{
	dMatrix matrix;
	NewtonBodyGetMatrix(m_body, &matrix[0][0]);
	dVector updir (matrix.RotateVector(m_upVector));
	dVector frontDir (matrix.RotateVector(m_frontVector));
	dVector rightDir (frontDir.CrossProduct(updir));

	dVector veloc (0.0f);
	if ((verticalSpeed <= 0.0f) && (m_groundPlane.DotProduct3(m_groundPlane)) > 0.0f) {
		// plane is supported by a ground plane, apply the player input velocity
		if (m_groundPlane.DotProduct3(updir) >= m_maxSlope) {
			// player is in a legal slope, he is in full control of his movement
			dVector bodyVeloc(0.0f);
			NewtonBodyGetVelocity(m_body, &bodyVeloc[0]);
			veloc = updir.Scale(bodyVeloc.DotProduct3(updir)) + gravity.Scale (timestep) + frontDir.Scale (forwardSpeed) + rightDir.Scale (lateralSpeed) + updir.Scale(verticalSpeed);
			veloc += (m_groundVelocity - updir.Scale (updir.DotProduct3(m_groundVelocity)));

			dFloat speedLimitMag2 = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + m_groundVelocity.DotProduct3(m_groundVelocity) + 0.1f;
			dFloat speedMag2 = veloc.DotProduct3(veloc);
			if (speedMag2 > speedLimitMag2) {
				veloc = veloc.Scale (dSqrt (speedLimitMag2 / speedMag2));
			}

			dFloat normalVeloc = m_groundPlane.DotProduct3(veloc - m_groundVelocity);
			if (normalVeloc < 0.0f) {
				veloc -= m_groundPlane.Scale (normalVeloc);
			}
		} else {
			// player is in an illegal ramp, he slides down hill an loses control of his movement 
			NewtonBodyGetVelocity(m_body, &veloc[0]);
			veloc += updir.Scale(verticalSpeed);
			veloc += gravity.Scale (timestep);
			dFloat normalVeloc = m_groundPlane.DotProduct3(veloc - m_groundVelocity);
			if (normalVeloc < 0.0f) {
				veloc -= m_groundPlane.Scale (normalVeloc);
			}
		}
	} else {
		// player is on free fall, only apply the gravity
		NewtonBodyGetVelocity(m_body, &veloc[0]);
		veloc += updir.Scale(verticalSpeed);
		veloc += gravity.Scale (timestep);
	}
	return veloc;
}


void dCustomPlayerController::SetPlayerVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat headingAngle, const dVector& gravity, dFloat timestep)
{
	dVector omega (CalculateDesiredOmega (headingAngle, timestep));
	dVector veloc (CalculateDesiredVelocity (forwardSpeed, lateralSpeed, verticalSpeed, gravity, timestep));			

	NewtonBodySetOmega(m_body, &omega[0]);
	NewtonBodySetVelocity(m_body, &veloc[0]);

	if ((verticalSpeed > 0.0f)) {
		m_isJumping = true;
	}
}

void dCustomPlayerController::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dAssert(0);
}


dFloat dCustomPlayerController::CalculateContactKinematics(const dVector& veloc, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
{
	dVector contactVeloc(0.0f) ;
	if (contactInfo->m_hitBody) {
		NewtonBodyGetPointVelocity (contactInfo->m_hitBody, contactInfo->m_point, &contactVeloc[0]);
	}

	const dFloat restitution = 0.0f;
	dVector normal (contactInfo->m_normal);
	dFloat reboundVelocMag = - (veloc - contactVeloc).DotProduct3(normal) * (1.0f + restitution);
	return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f; 
}


void dCustomPlayerController::UpdateGroundPlane (dMatrix& matrix, const dMatrix& castMatrix, const dVector& dst, int threadIndex)
{
	dCustomPlayerControllerManager* const manager = (dCustomPlayerControllerManager*) GetManager();
	NewtonWorld* const world = manager->GetWorld();
	NewtonWorldConvexCastReturnInfo info;
	dCustomControllerConvexRayFilter filter(m_body);

	dFloat param = 10.0f;
	int count = NewtonWorldConvexCast (world, &castMatrix[0][0], &dst[0], m_castingShape, &param, &filter, dCustomControllerConvexCastPreFilter::Prefilter, &info, 1, threadIndex);

	m_groundPlane = dVector (0.0f);
	m_groundVelocity = dVector (0.0f);

	if (count && (param <= 1.0f)) {
		m_isJumping = false;
		dVector supportPoint (castMatrix.m_posit + (dst - castMatrix.m_posit).Scale (param));
		m_groundPlane = dVector (info.m_normal[0], info.m_normal[1], info.m_normal[2], 0.0f);
		m_groundPlane.m_w = - supportPoint.DotProduct3(m_groundPlane);
		NewtonBodyGetPointVelocity (info.m_hitBody, &supportPoint.m_x, &m_groundVelocity[0]);
		matrix.m_posit = supportPoint;
		matrix.m_posit.m_w = 1.0f;
	}
}


void dCustomPlayerController::PostUpdate(dFloat timestep, int threadIndex)
{
	dMatrix matrix; 
	dQuaternion bodyRotation;
	dVector veloc(0.0f); 
	dVector omega(0.0f);  

	dCustomPlayerControllerManager* const manager = (dCustomPlayerControllerManager*) GetManager();
	NewtonWorld* const world = manager->GetWorld();

	// apply the player motion, by calculation the desired plane linear and angular velocity
	manager->ApplyPlayerMove (this, timestep);

	// get the body motion state 
	NewtonBodyGetMatrix(m_body, &matrix[0][0]);
	NewtonBodyGetVelocity(m_body, &veloc[0]);
	NewtonBodyGetOmega(m_body, &omega[0]);

	// integrate body angular velocity
	NewtonBodyGetRotation (m_body, &bodyRotation.m_x); 
	bodyRotation = bodyRotation.IntegrateOmega(omega, timestep);
	matrix = dMatrix (bodyRotation, matrix.m_posit);

	// integrate linear velocity
	dFloat normalizedTimeLeft = 1.0f; 
	dFloat step = timestep * dSqrt (veloc.DotProduct3(veloc)) ;
	dFloat descreteTimeStep = timestep * (1.0f / D_DESCRETE_MOTION_STEPS);
	int prevContactCount = 0;
	dCustomControllerConvexCastPreFilter castFilterData (m_body);
	NewtonWorldConvexCastReturnInfo prevInfo[PLAYER_CONTROLLER_MAX_CONTACTS];

	dVector updir (matrix.RotateVector(m_upVector));

	dVector scale(0.0f);
	NewtonCollisionGetScale (m_upperBodyShape, &scale.m_x, &scale.m_y, &scale.m_z);
	//const dFloat radio = m_outerRadio * 4.0f;
	const dFloat radio = (m_outerRadio + m_restrainingDistance) * 4.0f;
	NewtonCollisionSetScale (m_upperBodyShape, m_height - m_stairStep, radio, radio);


	NewtonWorldConvexCastReturnInfo upConstratint;
	memset (&upConstratint, 0, sizeof (upConstratint));
	upConstratint.m_normal[0] = m_upVector.m_x;
	upConstratint.m_normal[1] = m_upVector.m_y;
	upConstratint.m_normal[2] = m_upVector.m_z;
	upConstratint.m_normal[3] = m_upVector.m_w;

	for (int j = 0; (j < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > 1.0e-5f); j ++ ) {
		if (veloc.DotProduct3(veloc) < 1.0e-6f) {
			break;
		}

		dFloat timetoImpact;
		NewtonWorldConvexCastReturnInfo info[PLAYER_CONTROLLER_MAX_CONTACTS];
		dVector destPosit (matrix.m_posit + veloc.Scale (timestep));
		int contactCount = NewtonWorldConvexCast (world, &matrix[0][0], &destPosit[0], m_upperBodyShape, &timetoImpact, &castFilterData, dCustomControllerConvexCastPreFilter::Prefilter, info, sizeof (info) / sizeof (info[0]), threadIndex);
		if (contactCount) {
			contactCount = manager->ProcessContacts (this, info, contactCount);
		}

		if (contactCount) {
			matrix.m_posit += veloc.Scale (timetoImpact * timestep);
			if (timetoImpact > 0.0f) {
				matrix.m_posit -= veloc.Scale (D_PLAYER_CONTACT_SKIN_THICKNESS / dSqrt (veloc.DotProduct3(veloc))) ; 
			}

			normalizedTimeLeft -= timetoImpact;

			dFloat speed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
			dFloat bounceSpeed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
			dVector bounceNormal[PLAYER_CONTROLLER_MAX_CONTACTS * 2];

			for (int i = 1; i < contactCount; i ++) {
				dVector n0 (info[i-1].m_normal);
				for (int k = 0; k < i; k ++) {
					dVector n1 (info[k].m_normal);
					if (n0.DotProduct3(n1) > 0.9999f) {
						info[i] = info[contactCount - 1];
						i --;
						contactCount --;
						break;
					}
				}
			}

			int count = 0;
			if (!m_isJumping) {
				upConstratint.m_point[0] = matrix.m_posit.m_x;
				upConstratint.m_point[1] = matrix.m_posit.m_y;
				upConstratint.m_point[2] = matrix.m_posit.m_z;
				upConstratint.m_point[3] = matrix.m_posit.m_w;

				speed[count] = 0.0f;
				bounceNormal[count] = dVector (upConstratint.m_normal);
				bounceSpeed[count] = CalculateContactKinematics(veloc, &upConstratint);
				count ++;
			}

			for (int i = 0; i < contactCount; i ++) {
				speed[count] = 0.0f;
				bounceNormal[count] = dVector (info[i].m_normal);
				bounceSpeed[count] = CalculateContactKinematics(veloc, &info[i]);
				count ++;
			}

			for (int i = 0; i < prevContactCount; i ++) {
				speed[count] = 0.0f;
				bounceNormal[count] = dVector (prevInfo[i].m_normal);
				bounceSpeed[count] = CalculateContactKinematics(veloc, &prevInfo[i]);
				count ++;
			}

			dFloat residual = 10.0f;
			dVector auxBounceVeloc (0.0f);
			for (int i = 0; (i < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residual > 1.0e-3f); i ++) {
				residual = 0.0f;
				for (int k = 0; k < count; k ++) {
					dVector normal (bounceNormal[k]);
					dFloat v = bounceSpeed[k] - normal.DotProduct3(auxBounceVeloc);
					dFloat x = speed[k] + v;
					if (x < 0.0f) {
						v = 0.0f;
						x = 0.0f;
					}

					if (dAbs (v) > residual) {
						residual = dAbs (v);
					}

					auxBounceVeloc += normal.Scale (x - speed[k]);
					speed[k] = x;
				}
			}

			dVector velocStep (0.0f);
			for (int i = 0; i < count; i ++) {
				dVector normal (bounceNormal[i]);
				velocStep += normal.Scale (speed[i]);
			}
			veloc += velocStep;

			dFloat velocMag2 = velocStep.DotProduct3(velocStep);
			if (velocMag2 < 1.0e-6f) {
				dFloat advanceTime = dMin (descreteTimeStep, normalizedTimeLeft * timestep);
				matrix.m_posit += veloc.Scale (advanceTime);
				normalizedTimeLeft -= advanceTime / timestep;
			}

			prevContactCount = contactCount;
			memcpy (prevInfo, info, prevContactCount * sizeof (NewtonWorldConvexCastReturnInfo));

		} else {
			matrix.m_posit = destPosit;
			matrix.m_posit.m_w = 1.0f;
			break;
		}
	}
	NewtonCollisionSetScale (m_upperBodyShape, scale.m_x, scale.m_y, scale.m_z);

	// determine if player is standing on some plane
	dMatrix supportMatrix (matrix);
	supportMatrix.m_posit += updir.Scale (m_sphereCastOrigin);
	if (m_isJumping) {
		dVector dst (matrix.m_posit);
		UpdateGroundPlane (matrix, supportMatrix, dst, threadIndex);
	} else {
		step = dAbs (updir.DotProduct3(veloc.Scale (timestep)));
		dFloat castDist = (m_groundPlane.DotProduct3(m_groundPlane) > 0.0f) ? m_stairStep : step;
		dVector dst (matrix.m_posit - updir.Scale (castDist * 2.0f));
		UpdateGroundPlane (matrix, supportMatrix, dst, threadIndex);
	}

	// set player velocity, position and orientation
	NewtonBodySetVelocity(m_body, &veloc[0]);
	NewtonBodySetMatrix (m_body, &matrix[0][0]);
}
