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

#ifndef _DG_COLLISION_CAPSULE_H_
#define _DG_COLLISION_CAPSULE_H_


#include "dgCollisionConvex.h"

class dgCollisionCapsule: public dgCollisionConvex  
{
	public:
	dgCollisionCapsule (dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height);
	dgCollisionCapsule(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionCapsule();

	private:
	void Init (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height);

	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;

	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;

	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const;

	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;

	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual dgFloat32 CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const;

	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const;
	static dgInt32 CalculateSignature (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height);

	void TesselateTriangle(dgInt32 level, const dgVector& p0, const dgVector& p1, const dgVector& p2, dgInt32& count, dgVector* ouput) const;

	dgVector m_p0;
	dgVector m_p1;
	dgVector m_transform;
	dgFloat32 m_height;
	dgFloat32 m_radio0;
	dgFloat32 m_radio1;

	friend class dgWorld;
};

#endif 

