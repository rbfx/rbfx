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

#ifndef _DG_COLLISION_CONVEX_POLYGON_H_
#define _DG_COLLISION_CONVEX_POLYGON_H_

#include "dgCollisionConvexHull.h"


#define DG_CONVEX_POLYGON_MAX_VERTEX_COUNT	64

DG_MSC_VECTOR_ALIGMENT 
class dgCollisionConvexPolygon: public dgCollisionConvex	
{
	public:
	class dgClippedFaceEdge
	{
		public:
		dgClippedFaceEdge* m_next;
		dgClippedFaceEdge* m_twin;
		dgInt32 m_incidentNormal;
		dgInt32 m_incidentVertex;
	};

	public:
	dgCollisionConvexPolygon (dgMemoryAllocator* const allocator);
	~dgCollisionConvexPolygon ();

	virtual dgInt32 CalculateSignature () const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;
	
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;

	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const;

	virtual dgFloat32 GetVolume () const;
	virtual dgFloat32 GetBoxMinRadius () const; 
	virtual dgFloat32 GetBoxMaxRadius () const;
	
	void BeamClipping (const dgVector& origin, dgFloat32 size);

	dgVector CalculateGlobalNormal (const dgCollisionInstance* const parentMesh, const dgVector& localNormal) const;
	dgInt32 CalculateContactToConvexHullDescrete(const dgWorld* const world, const dgCollisionInstance* const parentMesh, dgCollisionParamProxy& proxy);
	dgInt32 CalculateContactToConvexHullContinue (const dgWorld* const world, const dgCollisionInstance* const parentMesh, dgCollisionParamProxy& proxy);

	dgVector m_normal;
	dgVector m_localPoly[DG_CONVEX_POLYGON_MAX_VERTEX_COUNT];
	dgInt32 m_clippEdgeNormal[DG_CONVEX_POLYGON_MAX_VERTEX_COUNT];
	dgInt32 m_count;
	dgInt32 m_paddedCount;
	dgInt32 m_faceId;
	dgInt32 m_stride;
	dgInt32 m_faceNormalIndex;
	dgFloat32 m_faceClipSize; 
	const dgFloat32* m_vertex;
	const dgInt32* m_vertexIndex;
	const dgInt32* m_adjacentFaceEdgeNormalIndex;
} DG_GCC_VECTOR_ALIGMENT;

#endif