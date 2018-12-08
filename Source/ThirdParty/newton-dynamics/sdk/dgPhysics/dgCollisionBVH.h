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

#ifndef __DGCOLLISION_BVH__
#define __DGCOLLISION_BVH__

#include "dgCollision.h"
#include "dgCollisionMesh.h"

class dgCollisionBVH;

typedef dgFloat32 (*dgCollisionBVHUserRayCastCallback) (const dgBody* const body, const dgCollisionBVH* const heightFieldCollision, dgFloat32 interception, dgFloat32* normal, dgInt32 faceId, void* usedData);

class dgCollisionBVH: public dgCollisionMesh, public dgAABBPolygonSoup
{
	public:
	DG_MSC_VECTOR_ALIGMENT 
	class dgBVHRay: public dgFastRayTest 
	{
		public:
		dgBVHRay(const dgVector& l0, const dgVector& l1)
			:dgFastRayTest (l0, l1)
		{
		}

		dgMatrix m_matrix;
		dgVector m_normal;
		dgUnsigned32 m_id;
		dgFloat32 m_t;
		void* m_userData;
		const dgBody* m_myBody;
		const dgCollisionBVH* m_me;
	} DG_GCC_VECTOR_ALIGMENT;

	dgCollisionBVH(dgWorld* const world);
	dgCollisionBVH (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionBVH(void);

	void BeginBuild();
	void AddFace (dgInt32 vertexCount, const dgFloat32* const vertexPtr, dgInt32 strideInBytes, dgInt32 faceAttribute);
	void EndBuild(dgInt32 optimize);

	void SetCollisionRayCastCallback (dgCollisionBVHUserRayCastCallback rayCastCallback);
	dgCollisionBVHUserRayCastCallback GetDebugRayCastCallback() const { return m_userRayCastCallback;} 
	void GetVertexListIndexList (const dgVector& p0, const dgVector& p1, dgMeshVertexListIndexList &data) const;

	void ForEachFace (dgAABBIntersectCallback callback, void* const context) const;

	private:
	static dgFloat32 RayHit (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount);
	static dgFloat32 RayHitUser (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount);
	static dgIntersectStatus GetPolygon (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);
	static dgIntersectStatus ShowDebugPolygon (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);
	static dgIntersectStatus GetTriangleCount (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);
	static dgIntersectStatus CollectVertexListIndexList (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);

	void Serialize(dgSerialize callback, void* const userData) const;
	virtual dgVector SupportVertex (const dgVector& dir) const;

	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;
	virtual void GetCollidingFaces (dgPolygonMeshDesc* const data) const;
	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;

	virtual void GetLocalAABB (const dgVector& p0, const dgVector& p1, dgVector& boxP0, dgVector& boxP1) const;
	virtual void DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;

	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const {return point;}

	dgPolygonSoupDatabaseBuilder* m_builder;
	dgCollisionBVHUserRayCastCallback m_userRayCastCallback;

	dgInt32 m_trianglesCount;
	friend class dgCollisionCompound;
	friend class dgCollisionDeformableMesh;
};


#endif
