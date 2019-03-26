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

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgCollisionUserMesh.h"


dgCollisionUserMesh::dgCollisionUserMesh(dgWorld* const world, const dgVector& boxP0, const dgVector& boxP1, const dgUserMeshCreation& data)
	:dgCollisionMesh (world, m_userMesh)
{
	m_rtti |= dgCollisionUserMesh_RTTI;

	m_userData = data.m_userData;
	m_getInfoCallback = data.m_getInfoCallback;
	m_faceInAABBCalback = data.m_faceInAABBCallback;
	m_rayHitCallback = data.m_rayHitCallback;
	m_collideCallback = data.m_collideCallback;
	m_destroyCallback = data.m_destroyCallback;
	m_serializeCallback = data.m_serializeCallback;
	m_getAABBOvelapTestCallback = data.m_getAABBOvelapTestCallback;

	SetCollisionBBox(boxP0, boxP1);
}

dgCollisionUserMesh::dgCollisionUserMesh (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionMesh (world, deserialization, userData, revisionNumber)
{
dgAssert (0);
	m_rtti |= dgCollisionUserMesh_RTTI;
	
/*
	dgAABBPolygonSoup::Deserialize (deserialization, userData);

	dgVector p0; 
	dgVector p1; 
	GetAABB (p0, p1);
	SetCollisionBBox(p0, p1);
*/
}

dgCollisionUserMesh::~dgCollisionUserMesh(void)
{
	if (m_destroyCallback) {
		m_destroyCallback (m_userData);
	}
}

void dgCollisionUserMesh::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);
	if (m_serializeCallback) {
		m_serializeCallback (m_userData, callback, userData);
	}
}


void dgCollisionUserMesh::GetVertexListIndexList (const dgVector& p0, const dgVector& p1, dgMeshVertexListIndexList &data) const
{
	if (m_faceInAABBCalback) {
		return m_faceInAABBCalback (m_userData, &p0[0], &p1[0], (const dgFloat32**) &data.m_veterxArray, &data.m_vertexCount, &data.m_vertexStrideInBytes,
							 data.m_indexList, data.m_maxIndexCount, data.m_userDataList);

	} else {
		data.m_triangleCount = 0;
	}
}

void dgCollisionUserMesh::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollision::GetCollisionInfo(info);
	if (m_getInfoCallback) {
		m_getInfoCallback (m_userData, info);
	}
}

bool dgCollisionUserMesh::AABBOvelapTest (const dgVector& boxP0, const dgVector& boxP1) const
{
	return (m_getAABBOvelapTestCallback && m_getAABBOvelapTestCallback (m_userData, boxP0, boxP1)) ? true : false;
}


dgFloat32 dgCollisionUserMesh::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body,	void* const userData, OnRayPrecastAction preFilter) const
{
	dgFloat32 param = dgFloat32 (1.2f);
	if (m_rayHitCallback) {
		dgCollisionMeshRayHitDesc data;
		data.m_localP0 = localP0;
		data.m_localP1 = localP1;
		data.m_userId = body->m_collision->GetUserDataID();
		data.m_userData = m_userData;
		data.m_altenateUserData = userData;
		if (body) {
			data.m_matrix = body->m_collision->GetGlobalMatrix();
		}

		dgFloat32 t = m_rayHitCallback (data);
		if ((t < dgFloat32 (1.0f)) && (t > dgFloat32 (0.0f))) {
			param = t;
			contactOut.m_normal = data.m_normal;
//			contactOut.m_userId = data.m_userId;
			contactOut.m_shapeId0 = dgInt64 (data.m_userId);
		} 
	}
	return param;
}


dgVector dgCollisionUserMesh::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (0);
	return dgVector::m_zero;
}

dgVector dgCollisionUserMesh::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert(0);
	return dgVector::m_zero;
}

void dgCollisionUserMesh::GetCollidingFacesContinue(dgPolygonMeshDesc* const data) const
{
	data->m_me = this;
	data->m_faceCount = 0;
	data->m_userData = m_userData;
	data->m_separationDistance = dgFloat32(0.0f);

	dgFastRayTest ray(dgVector(dgFloat32(0.0f)), data->m_boxDistanceTravelInMeshSpace);
	m_collideCallback(&data->m_p0, &ray);

	dgInt32 faceCount0 = 0;
	dgInt32 faceIndexCount0 = 0;
	dgInt32 faceIndexCount1 = 0;
	dgInt32 stride = data->m_vertexStrideInBytes / sizeof(dgFloat32);
	dgFloat32* const vertex = data->m_vertex;
	dgInt32* const address = data->m_meshData.m_globalFaceIndexStart;
	dgFloat32* const hitDistance = data->m_meshData.m_globalHitDistance;
	const dgInt32* const srcIndices = data->m_faceVertexIndex;
	dgInt32* const dstIndices = data->m_globalFaceVertexIndex;
	dgInt32* const faceIndexCountArray = data->m_faceIndexCount;

	for (dgInt32 i = 0; (i < data->m_faceCount) && (faceIndexCount0 < (DG_MAX_COLLIDING_INDICES - 32)); i++) {
		dgInt32 indexCount = faceIndexCountArray[i];
		const dgInt32* const indexArray = &srcIndices[faceIndexCount1];

		dgInt32 normalIndex = data->GetNormalIndex(indexArray, indexCount);
		dgVector faceNormal(&vertex[normalIndex * stride]);
		faceNormal = faceNormal & dgVector::m_triplexMask;
		dgFloat32 dist = data->PolygonBoxRayDistance(faceNormal, indexCount, indexArray, stride, vertex, ray);

		const dgInt32 faceIndexCount = data->GetFaceIndexCount(indexCount);
		if (dist < dgFloat32(1.0f)) {
			hitDistance[faceCount0] = dist;
			address[faceCount0] = faceIndexCount0;
			faceIndexCountArray[faceCount0] = indexCount;
			memcpy(&dstIndices[faceIndexCount0], indexArray, faceIndexCount * sizeof(dgInt32));
			faceCount0++;
			faceIndexCount0 += faceIndexCount;
		}
		faceIndexCount1 += faceIndexCount;
	}

	data->m_faceCount = 0;
	if (faceCount0) {
		data->m_faceCount = faceCount0;
		data->m_faceIndexStart = address;
		data->m_hitDistance = hitDistance;
		data->m_faceVertexIndex = dstIndices;

		if (GetDebugCollisionCallback()) {
			dgTriplex triplex[32];
			//const dgMatrix& matrix = data->m_polySoupInstance->GetGlobalMatrix();
			const dgVector scale = data->m_polySoupInstance->GetScale();
			dgMatrix matrix(data->m_polySoupInstance->GetLocalMatrix() * data->m_polySoupBody->GetMatrix());

			for (dgInt32 i = 0; i < data->m_faceCount; i++) {
				dgInt32 base = address[i];
				dgInt32 indexCount = faceIndexCountArray[i];
				const dgInt32* const vertexFaceIndex = &data->m_faceVertexIndex[base];
				for (dgInt32 j = 0; j < indexCount; j++) {
					dgInt32 index = vertexFaceIndex[j];
					dgVector q(&vertex[index * stride]);
					q = q & dgVector::m_triplexMask;
					//dgVector p(matrix.TransformVector(scale * dgVector(&vertex[index * stride])));
					dgVector p(matrix.TransformVector(scale * q));
					triplex[j].m_x = p.m_x;
					triplex[j].m_y = p.m_y;
					triplex[j].m_z = p.m_z;
				}
				dgInt32 faceId = data->GetFaceId(vertexFaceIndex, indexCount);
				GetDebugCollisionCallback() (data->m_polySoupBody, data->m_objBody, faceId, indexCount, &triplex[0].m_x, sizeof(dgTriplex));
			}
		}
	}
}

void dgCollisionUserMesh::GetCollidingFacesDescrete(dgPolygonMeshDesc* const data) const
{
	data->m_me = this;
	data->m_faceCount = 0;
	data->m_userData = m_userData;
	data->m_separationDistance = dgFloat32(0.0f);

	m_collideCallback(&data->m_p0, NULL);

	dgInt32 faceCount0 = 0;
	dgInt32 faceIndexCount0 = 0;
	dgInt32 faceIndexCount1 = 0;
	dgInt32 stride = data->m_vertexStrideInBytes / sizeof(dgFloat32);
	dgFloat32* const vertex = data->m_vertex;
	dgInt32* const address = data->m_meshData.m_globalFaceIndexStart;
	dgFloat32* const hitDistance = data->m_meshData.m_globalHitDistance;
	const dgInt32* const srcIndices = data->m_faceVertexIndex;
	dgInt32* const dstIndices = data->m_globalFaceVertexIndex;
	dgInt32* const faceIndexCountArray = data->m_faceIndexCount;

	for (dgInt32 i = 0; (i < data->m_faceCount) && (faceIndexCount0 < (DG_MAX_COLLIDING_INDICES - 32)); i++) {
		dgInt32 indexCount = faceIndexCountArray[i];
		const dgInt32* const indexArray = &srcIndices[faceIndexCount1];

		dgInt32 normalIndex = data->GetNormalIndex(indexArray, indexCount);
		dgVector faceNormal(&vertex[normalIndex * stride]);
		faceNormal = faceNormal & dgVector::m_triplexMask;
		dgFloat32 dist = data->PolygonBoxDistance(faceNormal, indexCount, indexArray, stride, vertex);

		const dgInt32 faceIndexCount = data->GetFaceIndexCount(indexCount);
		if (dist > dgFloat32(0.0f)) {
			hitDistance[faceCount0] = dist;
			address[faceCount0] = faceIndexCount0;
			faceIndexCountArray[faceCount0] = indexCount;
			memcpy(&dstIndices[faceIndexCount0], indexArray, faceIndexCount * sizeof(dgInt32));
			faceCount0++;
			faceIndexCount0 += faceIndexCount;
		}
		faceIndexCount1 += faceIndexCount;
	}

	data->m_faceCount = 0;
	if (faceCount0) {
		data->m_faceCount = faceCount0;
		data->m_faceIndexStart = address;
		data->m_hitDistance = hitDistance;
		data->m_faceVertexIndex = dstIndices;

		if (GetDebugCollisionCallback()) {
			dgTriplex triplex[32];
			//const dgMatrix& matrix = data->m_polySoupInstance->GetGlobalMatrix();
			const dgVector scale = data->m_polySoupInstance->GetScale();
			dgMatrix matrix(data->m_polySoupInstance->GetLocalMatrix() * data->m_polySoupBody->GetMatrix());

			for (dgInt32 i = 0; i < data->m_faceCount; i++) {
				dgInt32 base = address[i];
				dgInt32 indexCount = faceIndexCountArray[i];
				const dgInt32* const vertexFaceIndex = &data->m_faceVertexIndex[base];
				for (dgInt32 j = 0; j < indexCount; j++) {
					dgInt32 index = vertexFaceIndex[j];
					dgVector q(&vertex[index * stride]);
					q = q & dgVector::m_triplexMask; 
					//dgVector p(matrix.TransformVector(scale * dgVector(&vertex[index * stride])));
					dgVector p(matrix.TransformVector(scale * q));
					triplex[j].m_x = p.m_x;
					triplex[j].m_y = p.m_y;
					triplex[j].m_z = p.m_z;
				}
				dgInt32 faceId = data->GetFaceId(vertexFaceIndex, indexCount);
				GetDebugCollisionCallback() (data->m_polySoupBody, data->m_objBody, faceId, indexCount, &triplex[0].m_x, sizeof(dgTriplex));
			}
		}
	}
}

void dgCollisionUserMesh::GetCollidingFaces (dgPolygonMeshDesc* const data) const
{
	if (m_collideCallback) {
		if (data->m_doContinuesCollisionTest) {
			GetCollidingFacesContinue(data);
		} else {
			GetCollidingFacesDescrete(data);
		}
	}
}

void dgCollisionUserMesh::DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
/*
	dgCollisionUserMeshShowPolyContext context;

	context.m_matrix = matrixPtr;
	context.m_userData = userData;;
	context.m_callback = callback;

	dgVector p0 (dgFloat32 (-1.0e20f), dgFloat32 (-1.0e20f), dgFloat32 (-1.0e20f), dgFloat32 (0.0f));
	dgVector p1 (dgFloat32 ( 1.0e20f), dgFloat32 ( 1.0e20f), dgFloat32 ( 1.0e20f), dgFloat32 (0.0f));
	ForAllSectors (p0, p1, ShowDebugPolygon, &context);
*/
}
