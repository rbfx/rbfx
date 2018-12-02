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

#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonBody.h"
#include "dNewtonCollision.h"
#include "dNewtonVehicleManager.h"


dNewtonVehicleManager::dNewtonVehicleManager (dNewton* const world)
	:dCustomVehicleControllerManager (world->GetNewton(), 0, NULL)
{
	dAssert (0);
}

dNewtonVehicleManager::~dNewtonVehicleManager ()
{
}

dNewton* dNewtonVehicleManager::GetWorld() const 
{
	NewtonWorld* const workd = dCustomVehicleControllerManager::GetWorld();

	return (dNewton*) NewtonWorldGetUserData(workd);
}

/*
dNewtonVehicleManager::dNewtonVehicle* dNewtonVehicleManager::GetFirstVehicle() const
{
	CustomListNode* const node = GetFirst();
	if (node) {
		return (dNewtonVehicleManager::dNewtonVehicle*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
	return NULL;
}

dNewtonVehicleManager::dNewtonVehicle* dNewtonVehicleManager::GetNextVehicle(const dNewtonVehicle* const Vehicle) const
{
	dAssert (Vehicle);
	dAssert (FindNodeFromInfo(*Vehicle->m_controller));
	CustomListNode* const node = GetNodeFromInfo(*Vehicle->m_controller)->GetNext();
	if (node) {
		return (dNewtonVehicleManager::dNewtonVehicle*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
	return NULL;
}



void dNewtonVehicleManager::ApplyVehicleMove (CustomVehicleController* const controller, dFloat timestep)
{
	dNewtonVehicle* const body = (dNewtonVehicle*) NewtonBodyGetUserData(controller->GetBody());
	body->OnVehicleMove (timestep);
}
*/

//dNewtonVehicleManager::dNewtonVehicle::dNewtonVehicle (dNewtonVehicleManager* const manager, void* const userData, dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dFloat* const upDir, const dFloat* const frontDir, dLong collisionMask)
dNewtonVehicleManager::dNewtonVehicle::dNewtonVehicle (dNewtonVehicleManager* const manager, const dNewtonCollision& collisionShape, void* const userData, const dFloat* const location)
	:dNewtonDynamicBody(NULL)
{
/*
	dMatrix VehicleAxis; 
	VehicleAxis[0] = dVector (upDir); // the y axis is the character up vector
	VehicleAxis[1] = dVector (frontDir); // the x axis is the character front direction
	VehicleAxis[2] = VehicleAxis[0] * VehicleAxis[1];
	VehicleAxis[3] = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	m_controller = manager->CreateVehicle(mass, outerRadius, innerRadius, height, stairStep, VehicleAxis);

	NewtonBody* const body = m_controller->GetBody();

	SetBody (body);
	SetUserData (userData);
	
	NewtonCollision* const collision = NewtonBodyGetCollision(body);
	new dNewtonCollisionCompound (collision, collisionMask);

	for (void* node = NewtonCompoundCollisionGetFirstNode (collision); node; node = NewtonCompoundCollisionGetNextNode (collision, node)) {
		NewtonCollision* const childShape = NewtonCompoundCollisionGetCollisionFromNode (collision, node);

		switch (NewtonCollisionGetType (childShape)) 
		{
			case SERIALIZE_ID_CAPSULE:
			{
				new dNewtonCollisionCapsule (childShape, collisionMask);
				break;
			}

			case SERIALIZE_ID_CYLINDER:
			{
				new dNewtonCollisionCylinder (childShape, collisionMask);
				break;
			}

			case SERIALIZE_ID_CONVEXHULL:
			{
				new dNewtonCollisionConvexHull (childShape, collisionMask);
				break;
			}

			default: 
				dAssert (0);
		}
	}
*/
}


dNewtonVehicleManager::dNewtonVehicle::~dNewtonVehicle ()
{
	NewtonBody* const body = m_controller->GetBody();	
	if (NewtonBodyGetDestructorCallback(body)) {
		SetBody(NULL);
		dNewtonVehicleManager* const manager = (dNewtonVehicleManager*)m_controller->GetManager();
		manager->DestroyController (m_controller);
	}
}

/*
void dNewtonVehicleManager::dNewtonVehicle::SetVehicleVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat headingAngle, const dFloat* const gravity, dFloat timestep)
{
	m_controller->SetVehicleVelocity (forwardSpeed, lateralSpeed, verticalSpeed, headingAngle, dVector(gravity), timestep);
}

dFloat dNewtonVehicleManager::dNewtonVehicle::GetVehicleHigh() const
{
	return m_controller->GetHigh();
}

*/