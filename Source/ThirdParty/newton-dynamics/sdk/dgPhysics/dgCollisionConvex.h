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

#ifndef _DG_CONVEX_COLLISION_H__
#define _DG_CONVEX_COLLISION_H__

#include "dgCollision.h"

#define DG_CLIP_MAX_COUNT				512
#define DG_CLIP_MAX_POINT_COUNT			64
#define D_MIN_CONVEX_SHAPE_SIZE			dgFloat32 (1.0f/128.0f)


DG_MSC_VECTOR_ALIGMENT
class dgCollisionConvex: public dgCollision
{
	public:
	class dgConvexSimplexEdge
	{
		public:
		dgConvexSimplexEdge* m_twin;
		dgConvexSimplexEdge* m_next;
		dgConvexSimplexEdge* m_prev;
		dgInt32 m_vertex;
	};
	
	virtual dgInt32 GetConvexVertexCount() const { return m_vertexCount;}
	virtual void CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const;
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;

	bool IntesectionTest (dgCollisionParamProxy& proxy) const;

	protected:
	dgCollisionConvex (dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgCollisionID id);
	dgCollisionConvex (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	~dgCollisionConvex ();

	virtual void SerializeLow(dgSerialize callback, void* const userData) const;

	virtual dgVector CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& plane, const dgCollisionInstance& parentScale) const;
	static void CalculateInertia (void *userData, int vertexCount, const dgFloat32* const FaceArray, int faceId);

	virtual dgFloat32 GetVolume () const;

	virtual dgFloat32 GetBoxMinRadius () const; 
	virtual dgFloat32 GetBoxMaxRadius () const;

	dgInt32 RayCastClosestFace (dgVector* tetrahedrum, const dgVector& origin, dgFloat32& pointDist) const;
	dgInt32 SimplifyClipPolygon (dgInt32 count, const dgVector& normal, dgVector* const polygon) const;

	dgVector CalculateVolumeIntegral (const dgPlane& plane) const; 
	
	void SetVolumeAndCG ();
	bool SanityCheck (dgPolyhedra& hull) const;
	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;

	virtual void MassProperties ();
	virtual dgMatrix CalculateInertiaAndCenterOfMass (const dgMatrix& m_alignMatrix, const dgVector& localScale, const dgMatrix& matrix) const;
	virtual dgFloat32 CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const;

	bool SanityCheck(dgInt32 count, const dgVector& normal, dgVector* const contactsOut) const;

	dgInt32 RectifyConvexSlice (dgInt32 count, const dgVector& normal, dgVector* const contactsOut) const;

	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const;
	virtual const dgConvexSimplexEdge** GetVertexToEdgeMapping() const {return NULL;}
	
	dgVector* m_vertex;
	dgConvexSimplexEdge* m_simplex;
	
	dgFloat32 m_boxMinRadius;
	dgFloat32 m_boxMaxRadius;
	dgFloat32 m_simplexVolume;
	
	dgUnsigned16 m_edgeCount;
	dgUnsigned16 m_vertexCount;
	
	public:	
	static dgVector m_unitCircle[6];	

	friend class dgWorld;
	friend class dgBroadPhase;
	friend class dgMinkowskiConv;
	friend class dgCollisionCompound;
	friend class dgCollisionConvexModifier;
} DG_GCC_VECTOR_ALIGMENT;



#endif //AFX_DGCONVEXCOLLISION_H__57E159CE_6B6F_42DE_891C_1F6C38EB9D29_H


