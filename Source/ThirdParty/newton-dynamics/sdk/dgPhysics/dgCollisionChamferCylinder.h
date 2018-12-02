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

#if !defined(AFX_DGCOLLISIONCHAMFERCYLINDER_H__AS235640FER_H)
#define AFX_DGCOLLISIONCHAMFERCYLINDER_H__AS235640FER_H

#include "dgCollisionConvex.h"

#define DG_CHAMFERCYLINDER_SLICES         4
#define DG_CHAMFERCYLINDER_BRAKES		  8
#define DG_MAX_CHAMFERCYLINDER_DIR_COUNT  8


class dgCollisionChamferCylinder: public dgCollisionConvex  
{
	public:
	dgCollisionChamferCylinder(dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgFloat32 radius, dgFloat32 height);
	dgCollisionChamferCylinder(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionChamferCylinder();

	protected:
	void Init (dgFloat32 radius, dgFloat32 height);
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut)  const;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;

	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;
	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	static dgInt32 CalculateSignature (dgFloat32 radius, dgFloat32 height);
	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const;

	private:
	dgFloat32 m_height;
	dgFloat32 m_radius;

	dgVector m_vertex[DG_CHAMFERCYLINDER_BRAKES * (DG_CHAMFERCYLINDER_SLICES + 1)];
	static dgInt32 m_shapeRefCount;
	static dgConvexSimplexEdge m_edgeArray[];
	static dgVector m_shapesDirs[];
	static dgVector m_yzMask;

	friend class dgWorld;
};

#endif 

