/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_DYNAMIC_BODY_H_
#define _D_NEWTON_DYNAMIC_BODY_H_

#include "dStdAfxNewton.h"
#include "dNewtonBody.h"


class dNewton;
class dNewtonCollision;

class dNewtonDynamicBody: public dNewtonBody
{
	public:
	CNEWTON_API dNewtonDynamicBody(dNewton* const world, dFloat mass, const dNewtonCollision* const collision, void* const userData, const dFloat* const matrix, dNewtonBody* const parent);
	CNEWTON_API virtual ~dNewtonDynamicBody();

	CNEWTON_API void GetPointVeloc (const dFloat* const point, dFloat* const veloc) const;
	CNEWTON_API void ApplyImpulseToDesiredPointVeloc (const dFloat* const point, const dFloat* const desiredveloc, dFloat timestep);

	CNEWTON_API virtual void OnForceAndTorque (dFloat timestep, int threadIndex) = 0;


	protected:
	CNEWTON_API dNewtonDynamicBody(dNewtonBody* const parent);
	CNEWTON_API virtual void SetBody (NewtonBody* const body);

	private: 
	CNEWTON_API static void OnForceAndTorque (const NewtonBody* body, dFloat timestep, int threadIndex);
};

#endif
