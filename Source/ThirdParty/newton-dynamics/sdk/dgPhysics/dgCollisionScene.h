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


#ifndef _DGCOLLISIONSCENE_H_
#define _DGCOLLISIONSCENE_H_


#include "dgCollision.h"
#include "dgCollisionCompound.h"


class dgCollisionScene: public dgCollisionCompound  
{
	public:
	dgCollisionScene(dgWorld* const world);
	dgCollisionScene (const dgCollisionScene& source, const dgCollisionInstance* const myInstance);
	dgCollisionScene(dgWorld* const world, dgDeserialize deserialization, void* const userData, const dgCollisionInstance* const myInstance, dgInt32 revisionNumber);
	virtual ~dgCollisionScene();

	void CollidePair (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	void CollideCompoundPair (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	virtual void MassProperties ();
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	dgFloat32 GetBoxMinRadius () const;
	dgFloat32 GetBoxMaxRadius () const;
};

#endif
