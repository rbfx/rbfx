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
#include "dgCollisionBVH.h"


dgCollisionBVH::dgCollisionBVH(dgWorld* const world)
	:dgCollisionMesh (world, m_boundingBoxHierachy), dgAABBPolygonSoup()
	,m_trianglesCount(0)
{
	m_rtti |= dgCollisionBVH_RTTI;
	m_builder = NULL;
	m_userRayCastCallback = NULL;
}

dgCollisionBVH::dgCollisionBVH (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionMesh (world, deserialization, userData, revisionNumber)
	,dgAABBPolygonSoup()
	,m_trianglesCount(0)
{
	dgAssert (m_rtti | dgCollisionBVH_RTTI);
	m_builder = NULL;;
	m_userRayCastCallback = NULL;

	dgAABBPolygonSoup::Deserialize (deserialization, userData, revisionNumber);

	dgVector p0; 
	dgVector p1; 
	GetAABB (p0, p1);
	SetCollisionBBox(p0, p1);

	deserialization(userData, &m_trianglesCount, sizeof (dgInt32));
}

dgCollisionBVH::~dgCollisionBVH(void)
{
}

void dgCollisionBVH::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);
	dgAABBPolygonSoup::Serialize ((dgSerialize) callback, userData);
	callback(userData, &m_trianglesCount, sizeof (dgInt32));
}

void dgCollisionBVH::BeginBuild()
{
	m_builder = new (m_allocator) dgPolygonSoupDatabaseBuilder(m_allocator);
	m_builder->Begin();
}

void dgCollisionBVH::AddFace(dgInt32 vertexCount, const dgFloat32* const vertexPtr, dgInt32 strideInBytes, dgInt32 faceAttribute)
{
	dgInt32 faceArray;
	dgInt32 indexList[256];

	faceArray = vertexCount;
	dgAssert (vertexCount < dgInt32 (sizeof (indexList) / sizeof (indexList[0])));
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		indexList[i] = i;
	}
	m_builder->AddMesh (vertexPtr, vertexCount, strideInBytes, 1, &faceArray, indexList, &faceAttribute, dgGetIdentityMatrix());
}

void dgCollisionBVH::SetCollisionRayCastCallback (dgCollisionBVHUserRayCastCallback rayCastCallback)
{
	m_userRayCastCallback = rayCastCallback;
}


void dgCollisionBVH::EndBuild(dgInt32 optimize)
{
	dgVector p0;
	dgVector p1;

	bool state = optimize ? true : false;

	m_builder->End(state);
	Create (*m_builder, state);
	CalculateAdjacendy();
	
	GetAABB (p0, p1);
	SetCollisionBBox (p0, p1);

	delete m_builder;
	m_builder = NULL;
		
	dgMeshVertexListIndexList data;
	data.m_indexList = NULL;
	data.m_userDataList = NULL;
	data.m_maxIndexCount = 1000000000;
	data.m_triangleCount = 0; 
	dgVector zero (dgFloat32 (0.0f));
	dgFastAABBInfo box (dgGetIdentityMatrix(), dgVector (dgFloat32 (1.0e15f)));
	ForAllSectors (box, zero, dgFloat32 (1.0f), GetTriangleCount, &data);
	m_trianglesCount = data.m_triangleCount;
}


void dgCollisionBVH::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollision::GetCollisionInfo(info);

	info->m_bvhCollision.m_vertexCount = GetVertexCount();
	info->m_bvhCollision.m_indexCount = m_trianglesCount * 3;
}

void dgCollisionBVH::ForEachFace (dgAABBIntersectCallback callback, void* const context) const
{
	dgVector p0 (-1.0e10f, -1.0e10f, -1.0e10f, 1.0f);
	dgVector p1 ( 1.0e10f,  1.0e10f,  1.0e10f, 1.0f);
	dgVector zero (dgFloat32 (0.0f));
	dgFastAABBInfo box (dgGetIdentityMatrix(), dgVector (dgFloat32 (1.0e15f)));
	//ForAllSectors (p0, p1, zero, dgFloat32 (1.0f), callback, context);
	ForAllSectors (box, zero, dgFloat32 (1.0f), callback, context);
}


dgIntersectStatus dgCollisionBVH::CollectVertexListIndexList (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	dgMeshVertexListIndexList& data = (*(dgMeshVertexListIndexList*) context);

	if ((data.m_triangleCount + indexCount - 2) * 3 > data.m_maxIndexCount) {
		return t_StopSearh;
	}

	dgInt32 k = data.m_triangleCount;
	dgInt32 j = data.m_triangleCount * 3;
	dgInt32 index0 = indexArray[0];
	dgInt32 index1 = indexArray[1];
	dgInt32 attribute = indexArray[indexCount];
	for (dgInt32 i = 2; i < indexCount; i ++) {
		dgInt32 index2 = indexArray[i];
		data.m_indexList[j + 0] = index0;
		data.m_indexList[j + 1] = index1;
		data.m_indexList[j + 2] = index2;
		data.m_userDataList[k] = attribute;
		index1 = index2; 
		k ++;
		j += 3;
	}

	dgAssert (j <= data.m_maxIndexCount);
	data.m_triangleCount = k;
	dgAssert ((data.m_triangleCount * 3) <= data.m_maxIndexCount);

	return t_ContinueSearh;
}



dgIntersectStatus dgCollisionBVH::GetTriangleCount (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	dgMeshVertexListIndexList& data = (*(dgMeshVertexListIndexList*) context);

	if ((data.m_triangleCount + indexCount - 2) * 3 > data.m_maxIndexCount) {
		return t_StopSearh;
	}

	data.m_triangleCount += (indexCount - 2);
	dgAssert ((data.m_triangleCount * 3) <= data.m_maxIndexCount);
	return t_ContinueSearh;
}


void dgCollisionBVH::GetVertexListIndexList (const dgVector& p0, const dgVector& p1, dgMeshVertexListIndexList &data) const
{
	dgFastAABBInfo box (p0, p1);
	ForAllSectors (box, dgVector (dgFloat32 (0.0f)), dgFloat32 (1.0f), CollectVertexListIndexList, &data);

	data.m_veterxArray = GetLocalVertexPool(); 
	data.m_vertexCount = GetVertexCount(); 
	data.m_vertexStrideInBytes = GetStrideInBytes(); 

}



dgFloat32 dgCollisionBVH::RayHit (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount)
{
	dgBVHRay& me = *((dgBVHRay*) context);
	dgVector normal (&polygon[indexArray[indexCount + 1] * (strideInBytes / sizeof (dgFloat32))]);
	normal = normal & dgVector::m_triplexMask;
	dgFloat32 t = me.PolygonIntersect (normal, me.m_t, polygon, strideInBytes, indexArray, indexCount);
	if (t <= (me.m_t * dgFloat32 (1.0001f))) {
//		if ((t * dgFloat32 (1.0001f)) >= me.m_t) {
//			dgFloat32 dist0;
//			dgFloat32 dist1;
//			dist0 = me.m_diff % normal;
//			dist1 = me.m_diff % me.m_normal;
//			if (dist0 < dist1) {
//				me.m_t = t;
//				me.m_normal = normal;
//				me.m_id = me.m_me->GetTagId(indexArray, indexCount);
//			} else {
//				t = me.m_t;
//			}
//		} else {
			me.m_t = t;
			me.m_normal = normal;
			me.m_id = me.m_me->GetTagId(indexArray, indexCount);
//		}
	}
	return t;
}

dgFloat32 dgCollisionBVH::RayHitUser (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount)
{
	dgAssert (0);
	dgFloat32 t = dgFloat32 (1.2f);
	dgBVHRay& me = *((dgBVHRay*) context);
	dgVector normal (&polygon[indexArray[indexCount + 1] * (strideInBytes / sizeof (dgFloat32))]);
	normal = normal & dgVector::m_triplexMask;
dgAssert (0);
	t = me.PolygonIntersect (normal, me.m_t, polygon, strideInBytes, indexArray, indexCount);
	if (t < dgFloat32 (1.0f)) {
		if (t < me.m_t) {
			me.m_t = t;
			me.m_normal = normal;
			me.m_id = me.m_me->GetTagId(indexArray, indexCount);
		}
		normal = me.m_matrix.RotateVector(normal);
		t = me.m_me->GetDebugRayCastCallback() (me.m_myBody, me.m_me, t, &normal[0], dgInt32 (me.m_me->GetTagId(indexArray, indexCount)), me.m_userData);
	}
	return t;
}



dgFloat32 dgCollisionBVH::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	dgBVHRay ray (localP0, localP1);
	ray.m_t = dgMin(maxT, dgFloat32 (1.0f));
	ray.m_me = this;
	ray.m_userData = userData;
	if (!m_userRayCastCallback) {
		ForAllSectorsRayHit (ray, maxT, RayHit, &ray);
		if (ray.m_t < maxT) {
			maxT = ray.m_t; 
			dgAssert (ray.m_normal.m_w == dgFloat32 (0.0f));
			dgAssert (ray.m_normal.DotProduct(ray.m_normal).GetScalar() > dgFloat32(0.0f));
			//contactOut.m_normal = ray.m_normal.Scale (dgRsqrt (ray.m_normal.DotProduct(ray.m_normal).GetScalar() + dgFloat32 (1.0e-8f)));
			contactOut.m_normal = ray.m_normal.Normalize();
			//contactOut.m_userId = ray.m_id;
			contactOut.m_shapeId0 = ray.m_id;
			contactOut.m_shapeId1 = ray.m_id;
		}
	} else {
		if (body) {
			//ray.m_matrix = body->m_collisionWorldMatrix;
			ray.m_matrix = body->m_collision->GetGlobalMatrix();
		}
		
		ForAllSectorsRayHit (ray, maxT, RayHitUser, &ray);
		if (ray.m_t < dgFloat32 (1.0f)) {
			maxT = ray.m_t; 
			dgAssert(ray.m_normal.m_w == dgFloat32(0.0f));
			dgAssert(ray.m_normal.DotProduct(ray.m_normal).GetScalar() > dgFloat32(0.0f));
			//contactOut.m_normal = ray.m_normal.Scale (dgRsqrt (ray.m_normal.DotProduct(ray.m_normal).GetScalar() + dgFloat32 (1.0e-8f)));
			contactOut.m_normal = ray.m_normal.Normalize();
			//contactOut.m_userId = ray.m_id;
			contactOut.m_shapeId0 = ray.m_id;
			contactOut.m_shapeId1 = ray.m_id;
		}
	}
	return maxT;
}

dgIntersectStatus dgCollisionBVH::GetPolygon (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	dgPolygonMeshDesc& data = (*(dgPolygonMeshDesc*) context);
	if (data.m_faceCount >= DG_MAX_COLLIDING_FACES) {
		dgTrace (("buffer Over float, try using a lower resolution mesh for collision\n"));
		return t_StopSearh;
	}
	if ((data.m_globalIndexCount + indexCount * 2 + 3) >= DG_MAX_COLLIDING_INDICES) {
		dgTrace (("buffer Over float, try using a lower resolution mesh for collision\n"));
		return t_StopSearh;
	}

	if (data.m_me->GetDebugCollisionCallback()) { 
		dgTriplex triplex[128];
		dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));
		const dgVector scale = data.m_polySoupInstance->GetScale();
		dgMatrix matrix (data.m_polySoupInstance->GetLocalMatrix() * data.m_polySoupBody->GetMatrix());
		for (dgInt32 i = 0; i < indexCount; i ++ ) {
			dgVector p (matrix.TransformVector(scale * (dgVector(&polygon[indexArray[i] * stride]) & dgVector::m_triplexMask))); 
			triplex[i].m_x = p.m_x;
			triplex[i].m_y = p.m_y;
			triplex[i].m_z = p.m_z;
		}
		if (data.m_polySoupBody) {
			data.m_me->GetDebugCollisionCallback() (data.m_polySoupBody, data.m_objBody, indexArray[indexCount], indexCount, &triplex[0].m_x, sizeof (dgTriplex));
		}
	}

	dgAssert (data.m_vertex == polygon);
	dgInt32 count = indexCount * 2 + 3;

	data.m_faceIndexCount[data.m_faceCount] = indexCount;
//	data.m_faceIndexStart[data.m_faceCount] = data.m_faceCount ? (data.m_faceIndexStart[data.m_faceCount - 1] + data.m_faceIndexCount[data.m_faceCount - 1]) : 0;
	data.m_faceIndexStart[data.m_faceCount] = data.m_globalIndexCount;
	data.m_hitDistance[data.m_faceCount] = hitDistance;
	data.m_faceCount ++;
	dgInt32* const dst = &data.m_faceVertexIndex[data.m_globalIndexCount];

	//the docks say memcpy is an intrinsic function but as usual this is another Microsoft lied
	//memcpy (dst, indexArray, sizeof (dgInt32) * count);
	for (dgInt32 i = 0; i < count; i ++) {
		dst[i] = indexArray[i];
	}
	
	data.m_globalIndexCount += count;
	
	return t_ContinueSearh;
}

void dgCollisionBVH::GetCollidingFaces (dgPolygonMeshDesc* const data) const
{
	data->m_me = this;
	data->m_vertex = GetLocalVertexPool();
	data->m_vertexStrideInBytes = GetStrideInBytes();

	data->m_faceCount = 0;
	data->m_globalIndexCount = 0;
	data->m_faceIndexCount = data->m_meshData.m_globalFaceIndexCount;
	data->m_faceIndexStart = data->m_meshData.m_globalFaceIndexStart;
	data->m_faceVertexIndex = data->m_globalFaceVertexIndex;
	data->m_hitDistance = data->m_meshData.m_globalHitDistance;
	ForAllSectors (*data, data->m_boxDistanceTravelInMeshSpace, data->m_maxT, GetPolygon, data);
}


dgVector dgCollisionBVH::SupportVertex (const dgVector& dir) const
{
	return ForAllSectorsSupportVectex (dir);
}

dgVector dgCollisionBVH::SupportVertex(const dgVector& dir, dgInt32* const vertexIndex) const
{
	return ForAllSectorsSupportVectex(dir);
}

dgVector dgCollisionBVH::SupportVertexSpecial(const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert(0);
	return SupportVertex(dir, vertexIndex);
}



void dgCollisionBVH::GetLocalAABB (const dgVector& p0, const dgVector& p1, dgVector& boxP0, dgVector& boxP1) const
{
	dgAssert (0);
}


struct dgCollisionBVHShowPolyContext
{
	dgMatrix m_matrix;
	void* m_userData;
	dgCollision::OnDebugCollisionMeshCallback m_callback;
};

dgIntersectStatus dgCollisionBVH::ShowDebugPolygon (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	dgTriplex triplex[128];
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));

	dgCollisionBVHShowPolyContext& data = *(dgCollisionBVHShowPolyContext*) context;
	for (dgInt32 i = 0; i < indexCount; i ++ ) {
		dgVector p (&polygon[indexArray[i] * stride]);
		p = p & dgVector::m_triplexMask;
		p = data.m_matrix.TransformVector(p);
		triplex[i].m_x = p.m_x;
		triplex[i].m_y = p.m_y;
		triplex[i].m_z = p.m_z;
	}
	data.m_callback (data.m_userData, indexCount, &triplex[0].m_x, indexArray[-1]);

	return t_ContinueSearh;
}



void dgCollisionBVH::DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgCollisionBVHShowPolyContext context;

	context.m_matrix = matrixPtr;
	context.m_userData = userData;;
	context.m_callback = callback;

	dgFastAABBInfo box (dgGetIdentityMatrix(), dgVector (1.0e15f));
	ForAllSectors (box, dgVector(dgFloat32 (0.0f)), dgFloat32 (1.0f), ShowDebugPolygon, &context);
}
