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


#ifndef _D_NEWTON_TRIGGER_MANAGER_H_
#define _D_NEWTON_TRIGGER_MANAGER_H_

#include "dStdAfxNewton.h"
#include "dNewtonKinematicBody.h"

class dNewtonBody;

class dNewtonTriggerManager: public dCustomTriggerManager
{
	public:
	class dNewtonTrigger: public dNewtonKinematicBody
	{
		public:
		CNEWTON_API dNewtonTrigger (dNewtonTriggerManager* const manager, const dNewtonCollision& convexShape, void* const userData, const dFloat* const matrix);
		CNEWTON_API ~dNewtonTrigger ();

		virtual void OnEnter (dNewtonBody* const visitor) = 0;
		virtual void OnInside (dNewtonBody* const visitor) = 0;
		virtual void OnExit (dNewtonBody* const visitor) = 0;

		private:
		//dCustomTriggerController* m_controller;

		friend class dNewtonTriggerManager;
	};

	CNEWTON_API dNewtonTriggerManager (dNewton* const world);
	CNEWTON_API virtual ~dNewtonTriggerManager ();

	CNEWTON_API dNewtonTrigger* GetFirstTrigger() const;
	CNEWTON_API dNewtonTrigger* GetNextTrigger(const dNewtonTrigger* const trigger) const;
	CNEWTON_API virtual void EventCallback (const dCustomTriggerController* const trigger, dTriggerEventType event, NewtonBody* const guess) const;
};




#endif
