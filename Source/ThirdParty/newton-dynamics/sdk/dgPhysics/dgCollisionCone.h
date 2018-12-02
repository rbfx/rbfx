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

#if !defined(AFX_DGCOLLISIONCONE_H__AS235640FER_H)
#define AFX_DGCOLLISIONCONE_H__AS235640FER_H

#include "dgCollisionConvex.h"

#define DG_CONE_SEGMENTS 12


class dgCollisionCone: public dgCollisionConvex  
{
	public:
	dgCollisionCone (dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgFloat32 radius, dgFloat32 height);
	dgCollisionCone(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionCone();

	private:
	void Init (dgFloat32 radius, dgFloat32 height);
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const;

	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut) const;

	virtual void MassProperties ();
	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;
	
	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	static dgInt32 CalculateSignature (dgFloat32 radius, dgFloat32 height);

	dgVector m_profile[3];
	dgFloat32 m_height;
	dgFloat32 m_radius;
	dgVector m_vertex[DG_CONE_SEGMENTS + 1];
	static dgInt32 m_shapeRefCount;
	static dgConvexSimplexEdge m_edgeArray[];

	friend class dgWorld;
};

#endif 

