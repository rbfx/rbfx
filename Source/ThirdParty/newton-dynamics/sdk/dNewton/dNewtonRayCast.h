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

#ifndef _D_NEWTON_RAYCAST_H_
#define _D_NEWTON_RAYCAST_H_


#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"
#include "dNewtonMaterial.h"
#include "dNewtonCollision.h"

class dNewton;
class dNewtonBody;


class dNewtonRayCast: virtual public dNewtonAlloc, public dNewtonMaterial  
{
	public:
	CNEWTON_API dNewtonRayCast (dNewton* const world, dLong collisionMask);
	CNEWTON_API virtual ~dNewtonRayCast();

	CNEWTON_API virtual void CastRay (const dFloat* const p0, const dFloat* const p1, int threadIndex = 0);

	protected:
	virtual bool OnPrefilter (const dNewtonBody* const body, const dNewtonCollision* const shape) const 
	{
		return (shape->GetCollisionMask() & m_collisionMask) ? true : false;
	}
	CNEWTON_API virtual dFloat OnRayHit (const dNewtonBody* const body, const dNewtonCollision* const shape, const dFloat* const contact, const dFloat* const normal, dLong collisionID, dFloat intersectParam) = 0;
	
	private:
	CNEWTON_API static dFloat RayFilterCallback(const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam);
	CNEWTON_API static unsigned PrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);

	dNewton* m_world;
};

#endif
