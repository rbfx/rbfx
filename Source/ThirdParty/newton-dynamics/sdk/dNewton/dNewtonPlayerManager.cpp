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
#include "dNewtonPlayerManager.h"


dNewtonPlayerManager::dNewtonPlayerManager (dNewton* const world)
	:dCustomPlayerControllerManager (world->GetNewton())
{
}

dNewtonPlayerManager::~dNewtonPlayerManager ()
{
}

dNewtonPlayerManager::dNewtonPlayer* dNewtonPlayerManager::GetFirstPlayer() const
{
	dListNode* const node = GetFirst();
	if (node) {
		return (dNewtonPlayerManager::dNewtonPlayer*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
	return NULL;
}

dNewtonPlayerManager::dNewtonPlayer* dNewtonPlayerManager::GetNextPlayer(const dNewtonPlayer* const player) const
{
	dAssert (player);
	dAssert (GetNodeFromInfo(*player->m_controller));
	dListNode* const node = GetNodeFromInfo(*player->m_controller)->GetNext();
	if (node) {
		return (dNewtonPlayerManager::dNewtonPlayer*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
	return NULL;
}



void dNewtonPlayerManager::ApplyPlayerMove (dCustomPlayerController* const controller, dFloat timestep)
{
	dNewtonPlayer* const body = (dNewtonPlayer*) NewtonBodyGetUserData(controller->GetBody());
	body->OnPlayerMove (timestep);
}


dNewtonPlayerManager::dNewtonPlayer::dNewtonPlayer (dNewtonPlayerManager* const manager, void* const userData, dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dFloat* const upDir, const dFloat* const frontDir, dLong collisionMask)
	:dNewtonKinematicBody(NULL)
{
	dMatrix playerAxis; 
	playerAxis[0] = dVector (upDir); // the y axis is the character up vector
	playerAxis[1] = dVector (frontDir); // the x axis is the character front direction
	playerAxis[2] = playerAxis[0].CrossProduct(playerAxis[1]);
	playerAxis[3] = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	m_controller = manager->CreatePlayer(mass, outerRadius, innerRadius, height, stairStep, playerAxis);

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

}

dNewtonPlayerManager::dNewtonPlayer::~dNewtonPlayer ()
{
	NewtonBody* const body = m_controller->GetBody();	
	if (NewtonBodyGetDestructorCallback(body)) {
		SetBody(NULL);
		dNewtonPlayerManager* const manager = (dNewtonPlayerManager*)m_controller->GetManager();
		manager->DestroyController (m_controller);
	}
}


void dNewtonPlayerManager::dNewtonPlayer::SetPlayerVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat headingAngle, const dFloat* const gravity, dFloat timestep)
{
	m_controller->SetPlayerVelocity (forwardSpeed, lateralSpeed, verticalSpeed, headingAngle, dVector(gravity), timestep);
}

dFloat dNewtonPlayerManager::dNewtonPlayer::GetPlayerHigh() const
{
	return m_controller->GetHigh();
}