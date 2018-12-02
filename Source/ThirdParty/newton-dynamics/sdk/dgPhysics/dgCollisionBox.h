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

#ifndef _DG_BOX_H_
#define _DG_BOX_H_


#include "dgCollisionConvex.h"


class dgCollisionBox: public dgCollisionConvex
{
	public:

	dgCollisionBox(dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgFloat32 size_x, dgFloat32 size_y, dgFloat32 size_z);
	dgCollisionBox(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionBox();

	protected:
	void Init (dgFloat32 size_x, dgFloat32 size_y, dgFloat32 size_z);
	virtual void CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecial(const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint(const dgVector& point, const dgVector& dir) const;
	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const;
	virtual const dgConvexSimplexEdge** GetVertexToEdgeMapping() const;
	
	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual void MassProperties ();

	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	static dgInt32 CalculateSignature (dgFloat32 dx, dgFloat32 dy, dgFloat32 dz);

	dgVector m_size[2];
	dgVector m_vertex[8];
	static dgInt32 m_initSimplex;
	static dgInt32 m_faces[][4];
	static dgVector m_indexMark;
	static dgConvexSimplexEdge m_edgeArray[];
	static dgConvexSimplexEdge* m_edgeEdgeMap[];
	static dgConvexSimplexEdge* m_vertexToEdgeMap[];
	friend class dgWorld;
};

#endif 

