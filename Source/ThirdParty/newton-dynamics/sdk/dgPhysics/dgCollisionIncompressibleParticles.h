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


#ifndef __DGCOLLISION_INCOMPRESSIBLE_PARTICLES_H__
#define __DGCOLLISION_INCOMPRESSIBLE_PARTICLES_H__


#include "dgCollision.h"
#include "dgCollisionLumpedMassParticles.h"


class dgCollisionIncompressibleParticles: public dgCollisionLumpedMassParticles
{
	public:
	dgCollisionIncompressibleParticles (const dgCollisionIncompressibleParticles& source);
	dgCollisionIncompressibleParticles (dgWorld* const world, dgMeshEffect* const mesh);
	dgCollisionIncompressibleParticles (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);

	virtual ~dgCollisionIncompressibleParticles(void);
	virtual void CalculateAcceleration(dgFloat32 timestep);
};





#endif 

