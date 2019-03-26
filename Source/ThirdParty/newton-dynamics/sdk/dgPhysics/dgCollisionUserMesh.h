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

#ifndef __DGCOLLISION_USERMESH__
#define __DGCOLLISION_USERMESH__

#include "dgCollision.h"
#include "dgCollisionMesh.h"



class dgCollisionUserMesh: public dgCollisionMesh
{
	public:
	typedef void (dgApi *OnUserMeshDestroyCallback) (void* const userData);
	typedef dgFloat32 (dgApi *OnUserMeshRayHitCallback) (dgCollisionMeshRayHitDesc& rayHitdata);
	typedef void (dgApi *OnUserMeshCollisionInfo) (void* userData, dgCollisionInfo* infoRecord);
	typedef void (dgApi *OnUserMeshCollideCallback) (void* const collideData, const void* const continueHandle);
	typedef dgInt32 (dgApi *OnUserMeshAABBOverlapTest) (void* const userData, const dgVector& boxP0, const dgVector& boxP1);
	typedef void (dgApi *OnUserMeshSerialize) (void* const userSerializeData, dgSerialize function, void* const serilalizeObject);
	typedef void (dgApi *OnUserMeshFacesInAABB) (void* userData, const dgFloat32* p0, const dgFloat32* p1, const dgFloat32** vertexArray, dgInt32* vertexCount, dgInt32* vertexStrideInBytes, const dgInt32* indexList, dgInt32 maxIndexCount, const dgInt32* faceAttribute);

	dgCollisionUserMesh(dgWorld* const world, const dgVector& boxP0, const dgVector& boxP1, const dgUserMeshCreation& data);
	dgCollisionUserMesh (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionUserMesh(void);

	void GetVertexListIndexList (const dgVector& p0, const dgVector& p1, dgMeshVertexListIndexList &data) const;

	bool AABBOvelapTest (const dgVector& boxP0, const dgVector& boxP1) const;

	private:
	void Serialize(dgSerialize callback, void* const userData) const;

	dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const {return point;}

	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;
	virtual void GetCollidingFaces (dgPolygonMeshDesc* const data) const;
	virtual void DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;

	void GetCollidingFacesContinue(dgPolygonMeshDesc* const data) const;
	void GetCollidingFacesDescrete(dgPolygonMeshDesc* const data) const;

	void* m_userData;
	OnUserMeshSerialize m_serializeCallback;
	OnUserMeshCollisionInfo m_getInfoCallback;
	OnUserMeshFacesInAABB m_faceInAABBCalback;
	OnUserMeshRayHitCallback m_rayHitCallback;
	OnUserMeshCollideCallback m_collideCallback;
	OnUserMeshDestroyCallback m_destroyCallback;
	OnUserMeshAABBOverlapTest m_getAABBOvelapTestCallback;
};

class dgUserMeshCreation
{
	public:
	void* m_userData;
	dgCollisionUserMesh::OnUserMeshSerialize m_serializeCallback;
	dgCollisionUserMesh::OnUserMeshCollideCallback m_collideCallback;
	dgCollisionUserMesh::OnUserMeshRayHitCallback m_rayHitCallback;
	dgCollisionUserMesh::OnUserMeshDestroyCallback m_destroyCallback;
	dgCollisionUserMesh::OnUserMeshCollisionInfo m_getInfoCallback;
	dgCollisionUserMesh::OnUserMeshFacesInAABB m_faceInAABBCallback;
	dgCollisionUserMesh::OnUserMeshAABBOverlapTest m_getAABBOvelapTestCallback;
};


#endif
