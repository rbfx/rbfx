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
#include "dNewtonCollision.h"
#include "dNewtonDynamicBody.h"


dNewtonDynamicBody::dNewtonDynamicBody(dNewtonBody* const parent)
	:dNewtonBody(m_dynamic, parent)
{
}

dNewtonDynamicBody::dNewtonDynamicBody (dNewton* const dWorld, dFloat mass, const dNewtonCollision* const collision, void* const userData, const dFloat* const matrix, dNewtonBody* const parent)
	:dNewtonBody(dWorld, mass, collision, userData, matrix, m_dynamic, parent)
{
	NewtonBodySetForceAndTorqueCallback(m_body, OnForceAndTorque);
}

dNewtonDynamicBody::~dNewtonDynamicBody()
{
}



void dNewtonDynamicBody::GetPointVeloc (const dFloat* const point, dFloat* const veloc) const
{
	NewtonBodyGetPointVelocity (m_body, &point[0], &veloc[0]);
}

void dNewtonDynamicBody::SetBody (NewtonBody* const body)
{
	dNewtonBody::SetBody(body);
	NewtonBodySetForceAndTorqueCallback(m_body, OnForceAndTorque);
}

void dNewtonDynamicBody::ApplyImpulseToDesiredPointVeloc (const dFloat* const point, const dFloat* const desiredveloc, dFloat timestep)
{
	NewtonBodyAddImpulse (m_body, &desiredveloc[0], &point[0], timestep);
}


void dNewtonDynamicBody::OnForceAndTorque (const NewtonBody* body, dFloat timestep, int threadIndex)
{
	dNewtonDynamicBody* const me = (dNewtonDynamicBody*) NewtonBodyGetUserData(body);
	dAssert (me);
	me->OnForceAndTorque (timestep, threadIndex);
}

