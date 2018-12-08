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

#if !defined(AFX_DGCOLLISIONCONVEXHULL_H__AS235640FER_H)
#define AFX_DGCOLLISIONCONVEXHULL_H__AS235640FER_H

#include "dgCollisionConvex.h"


class dgCollisionConvexHull: public dgCollisionConvex  
{
	public:
	class dgConvexBox;

	dgCollisionConvexHull(dgMemoryAllocator* const allocator, dgUnsigned32 signature);
	dgCollisionConvexHull(dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgInt32 count, dgInt32 strideInBytes, dgFloat32 tolerance, const dgFloat32* const vertexArray);
	dgCollisionConvexHull(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionConvexHull();

	dgInt32 GetFaceIndices (dgInt32 index, dgInt32* const indices) const;

	static dgInt32 CalculateSignature (dgInt32 vertexCount, const dgFloat32* const vertexArray, dgInt32 strideInBytes);

	protected:
	void BuildHull (dgInt32 count, dgInt32 strideInBytes, dgFloat32 tolerance, const dgFloat32* const vertexArray);
	bool Create (dgInt32 count, dgInt32 strideInBytes, const dgFloat32* const vertexArray, dgFloat32 tolerance);

	bool RemoveCoplanarEdge (dgPolyhedra& convex, const dgBigVector* const hullVertexArray) const;	
	dgBigVector FaceNormal (const dgEdge *face, const dgBigVector* const pool) const;
	bool CheckConvex (dgPolyhedra& polyhedra, const dgBigVector* hullVertexArray) const;

	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;

	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;
	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;

	virtual const dgConvexSimplexEdge** GetVertexToEdgeMapping() const {return m_vertexToEdgeMapping;}

//	virtual dgFloat32 RayCast (const dgVector& localQ0, const dgVector& localP1, dgContactPoint& contactOut, const dgBody* const body, void* const userData) const;

	dgInt32 m_faceCount;
	dgInt32 m_supportTreeCount;
	dgConvexSimplexEdge** m_faceArray;
	const dgConvexSimplexEdge** m_vertexToEdgeMapping;
	dgConvexBox* m_supportTree;

	friend class dgWorld;
	friend class dgCollisionConvex;
};

#endif 

