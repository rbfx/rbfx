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

#ifndef _DG_CONTACT_SOLVER_H__
#define _DG_CONTACT_SOLVER_H__

#include "dgCollision.h"
#include "dgCollisionInstance.h"

DG_MSC_VECTOR_ALIGMENT
class dgMinkFace
{
	public:
	dgPlane m_plane;
	dgMinkFace* m_twin[3];
	dgInt16 m_vertex[3];
	dgInt8 m_mark;
	dgInt8 m_alive;
} DG_GCC_VECTOR_ALIGMENT;

#define DG_SEPARATION_PLANES_ITERATIONS	8
#define DG_CONVEX_MINK_STACK_SIZE		64
#define DG_CONNICS_CONTATS_ITERATIONS	32
#define DG_CONVEX_MINK_MAX_FACES		512
#define DG_CONVEX_MINK_MAX_POINTS		256
#define DG_MAX_EDGE_COUNT				2048
#define DG_PENETRATION_TOL				dgFloat32 (1.0f / 1024.0f)
#define DG_MINK_VERTEX_ERR				(dgFloat32 (1.0e-3f))
#define DG_MINK_VERTEX_ERR2				(DG_MINK_VERTEX_ERR * DG_MINK_VERTEX_ERR)


class dgCollisionParamProxy;

DG_MSC_VECTOR_ALIGMENT
class dgContactSolver: public dgDownHeap<dgMinkFace *, dgFloat32>  
{
	public: 
	dgContactSolver(dgCollisionParamProxy* const proxy);
	dgContactSolver(dgCollisionInstance* const instance0);

	bool CalculateClosestPoints();
	dgInt32 CalculateConvexCastContacts();
	dgInt32 CalculateConvexToConvexContacts();
	dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut);

	const dgVector& GetNormal() const {return m_normal;}
	const dgVector& GetPoint0() const {return m_closestPoint0;}
	const dgVector& GetPoint1() const {return m_closestPoint1;}
	
	private:
	class dgPerimenterEdge
	{
		public:
		const dgVector* m_vertex;
		dgPerimenterEdge* m_next;
		dgPerimenterEdge* m_prev;
	};

	class dgFaceFreeList
	{
		public:
		dgFaceFreeList* m_next;
	};

	DG_INLINE dgMinkFace* NewFace();
	DG_INLINE void PushFace(dgMinkFace* const face);
	DG_INLINE void DeleteFace(dgMinkFace* const face);
	DG_INLINE dgMinkFace* AddFace(dgInt32 v0, dgInt32 v1, dgInt32 v2);
	DG_INLINE void SupportVertex(const dgVector& dir, dgInt32 vertexIndex);
	
	DG_INLINE void TranslateSimplex(const dgVector& step);
	
	DG_INLINE void CalculateContactFromFeacture(dgInt32 featureType);
	DG_INLINE dgPerimenterEdge* ReduceContacts(dgPerimenterEdge* poly, dgInt32 maxCount) const;
//	DG_INLINE void ReduceDegeneratedTriangle();
	DG_INLINE dgBigVector ReduceLine(dgInt32& indexOut);
	DG_INLINE dgBigVector ReduceTriangle (dgInt32& indexOut);
	DG_INLINE dgBigVector ReduceTetrahedrum (dgInt32& indexOut);

	bool SanityCheck() const;
	dgInt32 ConvexPolygonsIntersection(const dgVector& normal, dgInt32 count1, dgVector* const shape1, dgInt32 count2, dgVector* const shape2, dgVector* const contactOut, dgInt32 maxContacts) const;
	dgInt32 ConvexPolygonToLineIntersection(const dgVector& normal, dgInt32 count1, dgVector* const shape1, dgInt32 count2, dgVector* const shape2, dgVector* const contactOut, dgVector* const mem) const;
	dgInt32 CalculateContacts (const dgVector& point0, const dgVector& point1, const dgVector& normal);
	dgInt32 CalculateClosestSimplex ();
	dgInt32 CalculateIntersectingPlane(dgInt32 count);

	dgVector m_normal;
	dgVector m_closestPoint0;
	dgVector m_closestPoint1;
	dgCollisionParamProxy* m_proxy;
	dgCollisionInstance* m_instance0;
	dgCollisionInstance* m_instance1;
	
	dgFaceFreeList* m_freeFace; 
	dgInt32 m_vertexIndex;
	dgInt32 m_faceIndex;

	dgVector m_hullDiff[DG_CONVEX_MINK_MAX_POINTS];
	dgVector m_hullSum[DG_CONVEX_MINK_MAX_POINTS];
	dgMinkFace* m_faceStack[DG_CONVEX_MINK_STACK_SIZE];
	dgMinkFace* m_coneFaceList[DG_CONVEX_MINK_STACK_SIZE];
	dgMinkFace* m_deletedFaceList[DG_CONVEX_MINK_STACK_SIZE];
	dgMinkFace m_facePool[DG_CONVEX_MINK_MAX_FACES];
	dgInt8 m_heapBuffer[DG_CONVEX_MINK_MAX_FACES * (sizeof (dgFloat32) + sizeof (dgMinkFace *))];

	static dgVector m_hullDirs[14]; 
	static dgInt32 m_rayCastSimplex[4][4];
}DG_GCC_VECTOR_ALIGMENT;


#endif 


