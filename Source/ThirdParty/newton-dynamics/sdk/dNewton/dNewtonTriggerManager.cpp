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
#include "dNewtonTriggerManager.h"


dNewtonTriggerManager::dNewtonTriggerManager (dNewton* const world)
	:dCustomTriggerManager (world->GetNewton())
{
}

dNewtonTriggerManager::~dNewtonTriggerManager ()
{
}

dNewtonTriggerManager::dNewtonTrigger* dNewtonTriggerManager::GetFirstTrigger() const
{
	dAssert(0);
/*
	dListNode* const node = GetFirst();
	if (node) {
		return (dNewtonTriggerManager::dNewtonTrigger*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
*/
	return NULL;
}

dNewtonTriggerManager::dNewtonTrigger* dNewtonTriggerManager::GetNextTrigger(const dNewtonTrigger* const trigger) const
{
	dAssert(0);
/*
	dAssert (trigger);
	dAssert (GetNodeFromInfo(*trigger->m_controller));
	dListNode* const node = GetNodeFromInfo(*trigger->m_controller)->GetNext();
	if (node) {
		return (dNewtonTriggerManager::dNewtonTrigger*) NewtonBodyGetUserData (node->GetInfo().GetBody());
	}
*/
	return NULL;
}


dNewtonTriggerManager::dNewtonTrigger::dNewtonTrigger (dNewtonTriggerManager* const manager, const dNewtonCollision& convexShape, void* const userData, const dFloat* const matrix)
	:dNewtonKinematicBody(NULL)
{
//	m_controller = manager->CreateTrigger (matrix, convexShape.GetShape(), this);
//	NewtonBody* const body = m_controller->GetBody();
//	SetBody (body);
//	SetUserData (userData);
	dAssert(0);
}

dNewtonTriggerManager::dNewtonTrigger::~dNewtonTrigger ()
{
	dAssert(0);
/*
	NewtonBody* const body = m_controller->GetBody();	
	if (NewtonBodyGetDestructorCallback(body)) {
		SetBody(NULL);
		dNewtonTriggerManager* const manager = (dNewtonTriggerManager*)m_controller->GetManager();
		manager->DestroyController (m_controller);
	}
*/
}

void dNewtonTriggerManager::EventCallback (const dCustomTriggerController* const trigger, dTriggerEventType event, NewtonBody* const guess) const
{
	dAssert(0);
/*
	dNewtonTrigger* const callback = (dNewtonTrigger*) trigger->GetUserData();
	dNewtonBody* const guessBody = (dNewtonBody*) NewtonBodyGetUserData(guess);
	switch (event) 
	{
		case m_enterTrigger:
		{
			callback->OnEnter(guessBody);
			break;
		}

		case m_exitTrigger:
		{
			callback->OnExit(guessBody);
			break;
		}

		case m_inTrigger:
		{
			callback->OnInside(guessBody);
			break;
		}
	}
*/
}
