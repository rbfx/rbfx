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


#ifndef _D_NEWTON_PLAYER_MANAGER_H_
#define _D_NEWTON_PLAYER_MANAGER_H_

#include "dStdAfxNewton.h"
#include "dNewtonKinematicBody.h"

class dNewtonPlayerManager: public dCustomPlayerControllerManager
{
	public:
	class dNewtonPlayer: public dNewtonKinematicBody
	{
		public:
		CNEWTON_API dNewtonPlayer (dNewtonPlayerManager* const manager, void* const userData, dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dFloat* const upDir, const dFloat* const frontDir, dLong collisionMask);
		CNEWTON_API ~dNewtonPlayer ();

		virtual void OnPlayerMove (dFloat timestep) = 0;

		CNEWTON_API dFloat GetPlayerHigh() const;
		CNEWTON_API void SetPlayerVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat headingAngle, const dFloat* const gravity, dFloat timestep);

		private:
		dCustomPlayerController* m_controller;

		friend class dNewtonPlayerManager;
	};

	CNEWTON_API dNewtonPlayerManager (dNewton* const world);
	CNEWTON_API virtual ~dNewtonPlayerManager ();

	CNEWTON_API dNewtonPlayer* GetFirstPlayer() const;
	CNEWTON_API dNewtonPlayer* GetNextPlayer(const dNewtonPlayer* const player) const;

	CNEWTON_API virtual void ApplyPlayerMove (dCustomPlayerController* const controller, dFloat timestep);
};




#endif
