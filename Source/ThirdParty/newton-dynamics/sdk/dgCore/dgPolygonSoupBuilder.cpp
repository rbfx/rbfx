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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#include "dgStdafx.h"
#include "dgStack.h"
#include "dgMatrix.h"
#include "dgMemory.h"
#include "dgPolyhedra.h"
#include "dgPolygonSoupBuilder.h"

#define DG_POINTS_RUN (512 * 1024)



class dgPolygonSoupDatabaseBuilder::dgFaceInfo
{
	public:
	dgInt32 indexCount;
	dgInt32 indexStart;
};

class dgPolygonSoupDatabaseBuilder::dgFaceBucket: public dgList<dgFaceInfo>
{
	public: 
	dgFaceBucket (dgMemoryAllocator* const allocator)
		:dgList<dgFaceInfo>(allocator)
	{
	}
};

class dgPolygonSoupDatabaseBuilder::dgFaceMap: public dgTree<dgFaceBucket, dgInt32>
{
	public:
	dgFaceMap (dgMemoryAllocator* const allocator, dgPolygonSoupDatabaseBuilder& builder)
		:dgTree<dgFaceBucket, dgInt32>(allocator)
	{
		dgInt32 polygonIndex = 0;
		dgInt32 faceCount = builder.m_faceCount;
		const dgInt32* const faceVertexCounts = &builder.m_faceVertexCount[0];
		const dgInt32* const faceVertexIndex = &builder.m_vertexIndex[0];
		for (dgInt32 i = 0; i < faceCount; i ++) {
			dgInt32 count = faceVertexCounts[i];
			dgInt32 attribute = faceVertexIndex[polygonIndex + count - 1];
			
			dgTreeNode* node = Find(attribute);
			if (!node) {
				dgFaceBucket tmp (GetAllocator());
				node = Insert(tmp, attribute);
			}

			dgFaceBucket& bucket = node->GetInfo();
			dgFaceInfo& face = bucket.Append()->GetInfo();
			face.indexCount = count;
			face.indexStart = polygonIndex;
			polygonIndex += count;
		}
	}

};


class dgPolygonSoupDatabaseBuilder::dgPolySoupFilterAllocator: public dgPolyhedra
{
	public: 
	dgPolySoupFilterAllocator (dgMemoryAllocator* const allocator)
		:dgPolyhedra (allocator)
	{
	}

	~dgPolySoupFilterAllocator ()
	{
	}

	dgInt32 AddFilterFace (dgUnsigned32 count, dgInt32* const pool)
	{
		BeginFace();
		dgAssert (count);
		bool reduction = true;
		while (reduction && !AddFace (dgInt32 (count), pool)) {
			reduction = false;
			if (count >3) {
				for (dgUnsigned32 i = 0; i < count; i ++) {
					for (dgUnsigned32 j = i + 1; j < count; j ++) {
						if (pool[j] == pool[i]) {
							for (i = j; i < count - 1; i ++) {
								pool[i] =  pool[i + 1];
							}
							count --;
							i = count;
							reduction = true;
							break;
						}
					}
				}
			}
		}
		EndFace();
		return reduction ? dgInt32 (count) : 0;
	}
};


dgPolygonSoupDatabaseBuilder::dgPolygonSoupDatabaseBuilder (dgMemoryAllocator* const allocator)
	:m_faceVertexCount(allocator)
	,m_vertexIndex(allocator)
	,m_normalIndex(allocator)
	,m_vertexPoints(allocator)
	,m_normalPoints(allocator)
{
	m_run = DG_POINTS_RUN;
	m_faceCount = 0;
	m_indexCount = 0;
	m_vertexCount = 0;
	m_normalCount = 0;
	m_allocator = allocator;
}

dgPolygonSoupDatabaseBuilder::dgPolygonSoupDatabaseBuilder (const dgPolygonSoupDatabaseBuilder& source)
	:m_faceVertexCount(source.m_allocator)
	,m_vertexIndex(source.m_allocator)
	,m_normalIndex(source.m_allocator)
	,m_vertexPoints(source.m_allocator)
	,m_normalPoints(source.m_allocator)
{
	m_run = DG_POINTS_RUN;
	m_faceCount = source.m_faceCount;
	m_indexCount = source.m_indexCount;
	m_vertexCount = source.m_vertexCount;
	m_normalCount = source.m_normalCount;
	m_allocator = source.m_allocator;
	
	m_vertexIndex[m_indexCount-1] = 0;
	m_faceVertexCount[m_faceCount-1] = 0;
	m_vertexPoints[m_vertexCount-1].m_w = 0;

	memcpy (&m_vertexIndex[0], &source.m_vertexIndex[0], sizeof (dgInt32) * m_indexCount);
	memcpy (&m_faceVertexCount[0], &source.m_faceVertexCount[0], sizeof (dgInt32) * m_faceCount);
	memcpy (&m_vertexPoints[0], &source.m_vertexPoints[0], sizeof (dgBigVector) * m_vertexCount);

	if (m_normalCount) {
		m_normalIndex[m_faceCount-1] = 0;
		m_normalPoints[m_normalCount - 1].m_w = 0;

		memcpy (&m_normalIndex[0], &source.m_normalIndex[0], sizeof (dgInt32) * m_faceCount);
		memcpy (&m_normalPoints[0], &source.m_normalPoints[0], sizeof (dgBigVector) * m_normalCount);
	} else {
		m_normalIndex[0] = 0;
		m_normalPoints[0].m_w = 0;
	}
}


dgPolygonSoupDatabaseBuilder::~dgPolygonSoupDatabaseBuilder ()
{
}


void dgPolygonSoupDatabaseBuilder::Begin()
{
	m_run = DG_POINTS_RUN;
	m_faceCount = 0;
	m_indexCount = 0;
	m_vertexCount = 0;
	m_normalCount = 0;
}


void dgPolygonSoupDatabaseBuilder::AddMesh (const dgFloat32* const vertex, dgInt32 vertexCount, dgInt32 strideInBytes, dgInt32 faceCount,	
	const dgInt32* const faceArray, const dgInt32* const indexArray, const dgInt32* const faceMaterialId, const dgMatrix& worldMatrix) 
{
	dgInt32 faces[256];
	dgInt32 pool[2048];


	m_vertexPoints[m_vertexCount + vertexCount].m_x = dgFloat64 (0.0f);
	dgBigVector* const vertexPool = &m_vertexPoints[m_vertexCount];

	worldMatrix.TransformTriplex (&vertexPool[0].m_x, sizeof (dgBigVector), vertex, strideInBytes, vertexCount);
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		vertexPool[i].m_w = dgFloat64 (0.0f);
	}

	dgInt32 totalIndexCount = faceCount;
	for (dgInt32 i = 0; i < faceCount; i ++) {
		totalIndexCount += faceArray[i];
	}

	m_vertexIndex[m_indexCount + totalIndexCount] = 0;
	m_faceVertexCount[m_faceCount + faceCount] = 0;

	dgInt32 k = 0;
	for (dgInt32 i = 0; i < faceCount; i ++) {
		dgInt32 count = faceArray[i];
		for (dgInt32 j = 0; j < count; j ++) {
			dgInt32 index = indexArray[k];
			pool[j] = index + m_vertexCount;
			k ++;
		}

		dgInt32 convexFaces = 0;
		if (count == 3) {
			convexFaces = 1;
			dgBigVector p0 (m_vertexPoints[pool[2]]);
			for (dgInt32 j = 0; j < 3; j ++) {
				dgBigVector p1 (m_vertexPoints[pool[j]]);
				dgBigVector edge (p1 - p0);
				dgFloat64 mag2 = edge.DotProduct3(edge);
				if (mag2 < dgFloat32 (1.0e-6f)) {
					convexFaces = 0;
				}
				p0 = p1;
			}

			if (convexFaces) {
				dgBigVector edge0 (m_vertexPoints[pool[2]] - m_vertexPoints[pool[0]]);
				dgBigVector edge1 (m_vertexPoints[pool[1]] - m_vertexPoints[pool[0]]);
				dgBigVector normal (edge0.CrossProduct(edge1));
				dgFloat64 mag2 = normal.DotProduct3(normal);
				if (mag2 < dgFloat32 (1.0e-8f)) {
					convexFaces = 0;
				}
			}

			if (convexFaces) {
				faces[0] = 3;
			}

		} else {
			convexFaces = AddConvexFace (count, pool, faces);
		}

		dgInt32 indexAcc = 0;
		for (dgInt32 j = 0; j < convexFaces; j ++) {
			dgInt32 count1 = faces[j];
			m_vertexIndex[m_indexCount + count1] = faceMaterialId[i];
			for (dgInt32 m = 0; m < count1; m ++) {
				m_vertexIndex[m_indexCount + m] = pool[indexAcc + m];
			}
			indexAcc += count1;
			m_indexCount += (count1 + 1);
			m_faceVertexCount[m_faceCount] = count1 + 1;
			m_faceCount ++;
		}
	}
	m_vertexCount += vertexCount;
	m_run -= vertexCount;
	if (m_run <= 0) {
		PackArray();
	}
}

void dgPolygonSoupDatabaseBuilder::PackArray()
{
	dgStack<dgInt32> indexMapPool (m_vertexCount);
	dgInt32* const indexMap = &indexMapPool[0];
	m_vertexCount = dgVertexListToIndexList (&m_vertexPoints[0].m_x, sizeof (dgBigVector), 3, m_vertexCount, &indexMap[0], dgFloat32 (1.0e-6f));

	dgInt32 k = 0;
	for (dgInt32 i = 0; i < m_faceCount; i ++) {
		dgInt32 count = m_faceVertexCount[i] - 1;
		for (dgInt32 j = 0; j < count; j ++) {
			dgInt32 index = m_vertexIndex[k];
			index = indexMap[index];
			m_vertexIndex[k] = index;
			k ++;
		}
		k ++;
	}

	m_run = DG_POINTS_RUN;
}

void dgPolygonSoupDatabaseBuilder::Finalize()
{
	if (m_faceCount) {
		dgStack<dgInt32> indexMapPool (m_indexCount + m_vertexCount);

		dgInt32* const indexMap = &indexMapPool[0];
		m_vertexCount = dgVertexListToIndexList (&m_vertexPoints[0].m_x, sizeof (dgBigVector), 3, m_vertexCount, &indexMap[0], dgFloat32 (1.0e-4f));

		dgInt32 k = 0;
		for (dgInt32 i = 0; i < m_faceCount; i ++) {
			dgInt32 count = m_faceVertexCount[i] - 1;
			for (dgInt32 j = 0; j < count; j ++) {
				dgInt32 index = m_vertexIndex[k];
				index = indexMap[index];
				m_vertexIndex[k] = index;
				k ++;
			}
			k ++;
		}
		OptimizeByIndividualFaces();
	}
}



void dgPolygonSoupDatabaseBuilder::FinalizeAndOptimize()
{
	Finalize();
	dgPolyhedra polyhedra(m_allocator);
	dgPolygonSoupDatabaseBuilder source(*this);
	dgPolygonSoupDatabaseBuilder leftOver(m_allocator);
	dgInt32 tmpIndexPool[1024];
	dgVector tmpVertexPool[1024];
	
	Begin();
	leftOver.Begin();
	polyhedra.BeginFace ();
	dgInt32 faceIndexNumber = 0;
	dgInt32 attribute = m_vertexIndex[0];

	for (dgInt32 i = 0; i < source.m_faceCount; i ++) {
		dgInt32 indexCount = source.m_faceVertexCount[i];
		dgAssert (indexCount < 1024);

		dgEdge* const face = polyhedra.AddFace(indexCount - 1, &source.m_vertexIndex[faceIndexNumber]);
		if (!face) {
			for (dgInt32 j = 0; j < indexCount - 1; j ++) {
				dgInt32 index = source.m_vertexIndex[faceIndexNumber + j];
				tmpVertexPool[j] = source.m_vertexPoints[index];
				tmpIndexPool[j] = j;
			}
			dgInt32 faceArray = indexCount - 1;
			leftOver.AddMesh (&tmpVertexPool[0].m_x, indexCount, sizeof (tmpVertexPool[0]), 1, &faceArray, tmpIndexPool, &attribute, dgGetIdentityMatrix());
		} else {
			// set the attribute
			dgEdge* ptr = face;
			do {
				ptr->m_userData = dgUnsigned64 (attribute);
				ptr = ptr->m_next;
			} while (ptr != face);
		}
		faceIndexNumber += indexCount; 
	} 
	polyhedra.EndFace();

	dgPolyhedra facesLeft(m_allocator);
	facesLeft.BeginFace();
	polyhedra.ConvexPartition (&source.m_vertexPoints[0].m_x, source.m_vertexPoints.GetElementSize(), &facesLeft);
	facesLeft.EndFace();

	dgInt32 mark = polyhedra.IncLRU();
	dgPolyhedra::Iterator iter (polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			continue;
		}
		if (edge->m_mark == mark) {
			continue;
		}

		dgEdge* ptr = edge;
		dgInt32 indexCount = 0;
		do {
			ptr->m_mark = mark;
			tmpVertexPool[indexCount] = source.m_vertexPoints[ptr->m_incidentVertex];
			tmpIndexPool[indexCount] = indexCount;
			indexCount ++;
			ptr = ptr->m_next;
		} while (ptr != edge);

		if (indexCount >= 3) {
			AddMesh (&tmpVertexPool[0].m_x, indexCount, sizeof (tmpVertexPool[0]), 1, &indexCount, tmpIndexPool, &attribute, dgGetIdentityMatrix());
		}
	}


	mark = facesLeft.IncLRU();
	dgPolyhedra::Iterator iter1 (facesLeft);
	for (iter1.Begin(); iter1; iter1 ++) {
		dgEdge* const edge = &(*iter1);
		if (edge->m_incidentFace < 0) {
			continue;
		}
		if (edge->m_mark == mark) {
			continue;
		}

		dgEdge* ptr = edge;
		dgInt32 indexCount = 0;
		do {
			ptr->m_mark = mark;
			tmpVertexPool[indexCount] = source.m_vertexPoints[ptr->m_incidentVertex];
			tmpIndexPool[indexCount] = indexCount;
			indexCount ++;
			ptr = ptr->m_next;
		} while (ptr != edge);
		if (indexCount >= 3) {
			AddMesh (&tmpVertexPool[0].m_x, indexCount, sizeof (dgVector), 1, &indexCount, tmpIndexPool, &attribute, dgGetIdentityMatrix());
		}
	}

	faceIndexNumber = 0;
	for (dgInt32 i = 0; i < leftOver.m_faceCount; i ++) {
		dgInt32 indexCount = leftOver.m_faceVertexCount[i] - 1;
		for (dgInt32 j = 0; j < indexCount; j ++) {
			dgInt32 index = leftOver.m_vertexIndex[faceIndexNumber + j];
			tmpVertexPool[j] = leftOver.m_vertexPoints[index];
			tmpIndexPool[j] = j;
		}
		dgInt32 faceArray = indexCount;
		AddMesh (&tmpVertexPool[0].m_x, indexCount, sizeof (tmpVertexPool[0]), 1, &faceArray, tmpIndexPool, &attribute, dgGetIdentityMatrix());

		faceIndexNumber += (indexCount + 1); 
	}

	Finalize();
}


void dgPolygonSoupDatabaseBuilder::OptimizeByIndividualFaces()
{
	dgInt32* const faceArray = &m_faceVertexCount[0];
	dgInt32* const indexArray = &m_vertexIndex[0];

	dgInt32* const oldFaceArray = &m_faceVertexCount[0];
	dgInt32* const oldIndexArray = &m_vertexIndex[0];

	dgInt32 polygonIndex = 0;
	dgInt32 newFaceCount = 0;
	dgInt32 newIndexCount = 0;
	for (dgInt32 i = 0; i < m_faceCount; i ++) {
		dgInt32 oldCount = oldFaceArray[i];
		dgInt32 count = FilterFace (oldCount - 1, &oldIndexArray[polygonIndex]);
		if (count) {
			faceArray[newFaceCount] = count + 1;
			for (dgInt32 j = 0; j < count; j ++) {
				indexArray[newIndexCount + j] = oldIndexArray[polygonIndex + j];
			}
			indexArray[newIndexCount + count] = oldIndexArray[polygonIndex + oldCount - 1];
			newFaceCount ++;
			newIndexCount += (count + 1);
		}
		polygonIndex += oldCount;
	}
	dgAssert (polygonIndex == m_indexCount);
	m_faceCount = newFaceCount;
	m_indexCount = newIndexCount;
}


void dgPolygonSoupDatabaseBuilder::End(bool optimize)
{
	if (optimize) {
		dgPolygonSoupDatabaseBuilder copy (*this);
		dgFaceMap faceMap (m_allocator, copy);

		Begin();
		dgFaceMap::Iterator iter (faceMap);
		for (iter.Begin(); iter; iter ++) {
			const dgFaceBucket& bucket = iter.GetNode()->GetInfo();
			Optimize(iter.GetNode()->GetKey(), bucket, copy);
		}
	}
	Finalize();

	// build the normal array and adjacency array
	// calculate all face the normals
	dgInt32 indexCount = 0;
	m_normalPoints[m_faceCount].m_x = dgFloat64 (0.0f);
	for (dgInt32 i = 0; i < m_faceCount; i ++) {
		dgInt32 faceIndexCount = m_faceVertexCount[i];

		const dgInt32* const ptr = &m_vertexIndex[indexCount];
		dgBigVector v0 (&m_vertexPoints[ptr[0]].m_x);
		dgBigVector v1 (&m_vertexPoints[ptr[1]].m_x);
		dgBigVector e0 (v1 - v0);
		dgBigVector normal0 (dgBigVector::m_zero);
		for (dgInt32 j = 2; j < faceIndexCount - 1; j ++) {
			dgBigVector v2 (&m_vertexPoints[ptr[j]].m_x);
			dgBigVector e1 (v2 - v0);
			normal0 += e0.CrossProduct(e1);
			e0 = e1;
		}
		dgBigVector normal (normal0.Normalize());

		m_normalPoints[i].m_x = normal.m_x;
		m_normalPoints[i].m_y = normal.m_y;
		m_normalPoints[i].m_z = normal.m_z;
		m_normalPoints[i].m_w = dgFloat32 (0.0f);
		indexCount += faceIndexCount;
	}
	// compress normals array
	m_normalIndex[m_faceCount] = 0;
	m_normalCount = dgVertexListToIndexList(&m_normalPoints[0].m_x, sizeof (dgBigVector), 3, m_faceCount, &m_normalIndex[0], dgFloat32 (1.0e-6f));
}


void dgPolygonSoupDatabaseBuilder::Optimize(dgInt32 faceId, const dgFaceBucket& faceBucket, const dgPolygonSoupDatabaseBuilder& source)
{
	#define DG_MESH_PARTITION_SIZE (1024 * 4)

	const dgInt32* const indexArray = &source.m_vertexIndex[0];
	const dgBigVector* const points = &source.m_vertexPoints[0];

	dgVector face[256];
	dgInt32 faceIndex[256];
	if (faceBucket.GetCount() >= DG_MESH_PARTITION_SIZE) {
		dgStack<dgFaceBucket::dgListNode*> array(faceBucket.GetCount());
		dgInt32 count = 0;
		for (dgFaceBucket::dgListNode* node = faceBucket.GetFirst(); node; node = node->GetNext()) {
			array[count] = node;
			count ++;
		}

		dgInt32 stack = 1;
		dgInt32 segments[32][2];
			
		segments[0][0] = 0;
		segments[0][1] = count;
	
		while (stack) {
			stack --;
			dgInt32 faceStart = segments[stack][0];
			dgInt32 faceCount = segments[stack][1];

			if (faceCount <= DG_MESH_PARTITION_SIZE) {

				dgPolygonSoupDatabaseBuilder tmpBuilder (m_allocator);
				for (dgInt32 i = 0; i < faceCount; i ++) {
					const dgFaceInfo& faceInfo = array[faceStart + i]->GetInfo();

					dgInt32 count1 = faceInfo.indexCount - 1;
					dgInt32 start1 = faceInfo.indexStart;
					dgAssert (faceId == indexArray[start1 + count1]);
					for (dgInt32 j = 0; j < count1; j ++) {
						dgInt32 index = indexArray[start1 + j];
						face[j] = points[index];
						faceIndex[j] = j;
					}
					dgInt32 faceIndexCount = count1;
					tmpBuilder.AddMesh (&face[0].m_x, count1, sizeof (dgVector), 1, &faceIndexCount, &faceIndex[0], &faceId, dgGetIdentityMatrix()); 
				}
				tmpBuilder.FinalizeAndOptimize ();

				dgInt32 faceIndexNumber = 0;
				for (dgInt32 i = 0; i < tmpBuilder.m_faceCount; i ++) {
					dgInt32 indexCount = tmpBuilder.m_faceVertexCount[i] - 1;
					for (dgInt32 j = 0; j < indexCount; j ++) {
						dgInt32 index = tmpBuilder.m_vertexIndex[faceIndexNumber + j];
						face[j] = tmpBuilder.m_vertexPoints[index];
						faceIndex[j] = j;
					}
					dgInt32 faceArray = indexCount;
					AddMesh (&face[0].m_x, indexCount, sizeof (dgVector), 1, &faceArray, faceIndex, &faceId, dgGetIdentityMatrix());

					faceIndexNumber += (indexCount + 1); 
				}

			} else {
				dgBigVector median (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
				dgBigVector varian (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
				for (dgInt32 i = 0; i < faceCount; i ++) {
					const dgFaceInfo& faceInfo = array[faceStart + i]->GetInfo();
					dgInt32 count1 = faceInfo.indexCount - 1;
					dgInt32 start1 = faceInfo.indexStart;
					dgBigVector p0 (dgFloat32 ( 1.0e10f), dgFloat32 ( 1.0e10f), dgFloat32 ( 1.0e10f), dgFloat32 (0.0f));
					dgBigVector p1 (dgFloat32 (-1.0e10f), dgFloat32 (-1.0e10f), dgFloat32 (-1.0e10f), dgFloat32 (0.0f));
					for (dgInt32 j = 0; j < count1; j ++) {
						dgInt32 index = indexArray[start1 + j];
						const dgBigVector& p = points[index];
						dgAssert(p.m_w == dgFloat32(0.0f));
						p0 = p0.GetMin(p);
						p1 = p1.GetMax(p);
					}
					dgBigVector p ((p0 + p1).Scale (0.5f));
					median += p;
					varian += p * p;
				}

				varian = varian.Scale (dgFloat32 (faceCount)) - median * median;

				dgInt32 axis = 0;
				dgFloat32 maxVarian = dgFloat32 (-1.0e10f);
				for (dgInt32 i = 0; i < 3; i ++) {
					if (varian[i] > maxVarian) {
						axis = i;
						maxVarian = dgFloat32 (varian[i]);
					}
				}
				dgBigVector center = median.Scale (dgFloat32 (1.0f) / dgFloat32 (faceCount));
				dgFloat64 axisVal = center[axis];

				dgInt32 leftCount = 0;
				dgInt32 lastFace = faceCount;

				for (dgInt32 i = 0; i < lastFace; i ++) {
					dgInt32 side = 0;
					const dgFaceInfo& faceInfo = array[faceStart + i]->GetInfo();

					dgInt32 start1 = faceInfo.indexStart;
					dgInt32 count1 = faceInfo.indexCount - 1;
					for (dgInt32 j = 0; j < count1; j ++) {
						dgInt32 index = indexArray[start1 + j];
						const dgBigVector& p = points[index];
						if (p[axis] > axisVal) {
							side = 1;
							break;
						}
					}

					if (side) {
						dgSwap (array[faceStart + i], array[faceStart + lastFace - 1]);
						lastFace --;
						i --;
					} else {
						leftCount ++;
					}
				}
				dgAssert (leftCount);
				dgAssert (leftCount < faceCount);

				segments[stack][0] = faceStart;
				segments[stack][1] = leftCount;
				stack ++;

				segments[stack][0] = faceStart + leftCount;
				segments[stack][1] = faceCount - leftCount;
				stack ++;
			}
		}
	
	} else {
		dgPolygonSoupDatabaseBuilder tmpBuilder (m_allocator);
		for (dgFaceBucket::dgListNode* node = faceBucket.GetFirst(); node; node = node->GetNext()) {
			const dgFaceInfo& faceInfo = node->GetInfo();

			dgInt32 count = faceInfo.indexCount - 1;
			dgInt32 start = faceInfo.indexStart;
			dgAssert (faceId == indexArray[start + count]);
			for (dgInt32 j = 0; j < count; j ++) {
				dgInt32 index = indexArray[start + j];
				face[j] = points[index];
				faceIndex[j] = j;
			}
			dgInt32 faceIndexCount = count;
			tmpBuilder.AddMesh (&face[0].m_x, count, sizeof (dgVector), 1, &faceIndexCount, &faceIndex[0], &faceId, dgGetIdentityMatrix()); 
		}
		tmpBuilder.FinalizeAndOptimize ();

		dgInt32 faceIndexNumber = 0;
		for (dgInt32 i = 0; i < tmpBuilder.m_faceCount; i ++) {
			dgInt32 indexCount = tmpBuilder.m_faceVertexCount[i] - 1;
			for (dgInt32 j = 0; j < indexCount; j ++) {
				dgInt32 index = tmpBuilder.m_vertexIndex[faceIndexNumber + j];
				face[j] = tmpBuilder.m_vertexPoints[index];
				faceIndex[j] = j;
			}
			dgInt32 faceArray = indexCount;
			AddMesh (&face[0].m_x, indexCount, sizeof (dgVector), 1, &faceArray, faceIndex, &faceId, dgGetIdentityMatrix());

			faceIndexNumber += (indexCount + 1); 
		}
	}
}


dgInt32 dgPolygonSoupDatabaseBuilder::FilterFace (dgInt32 count, dgInt32* const pool)
{
	if (count == 3) {
		dgBigVector p0 (m_vertexPoints[pool[2]]);
		for (dgInt32 i = 0; i < 3; i ++) {
			dgBigVector p1 (m_vertexPoints[pool[i]]);
			dgBigVector edge (p1 - p0);
			dgFloat64 mag2 = edge.DotProduct3(edge);
			if (mag2 < dgFloat32 (1.0e-6f)) {
				count = 0;
			}
			p0 = p1;
		}

		if (count == 3) {
			dgBigVector edge0 (m_vertexPoints[pool[2]] - m_vertexPoints[pool[0]]);
			dgBigVector edge1 (m_vertexPoints[pool[1]] - m_vertexPoints[pool[0]]);
			dgBigVector normal (edge0.CrossProduct(edge1));
			dgFloat64 mag2 = normal.DotProduct3(normal);
			if (mag2 < dgFloat32 (1.0e-8f)) {
				count = 0;
			}
		}
	} else {
		dgPolySoupFilterAllocator polyhedra(m_allocator);
		count = polyhedra.AddFilterFace (dgUnsigned32 (count), pool);

		if (!count) {
			return 0;
		}

		dgEdge* edge = &polyhedra.GetRoot()->GetInfo();
		if (edge->m_incidentFace < 0) {
			edge = edge->m_twin;
		}

		bool flag = true;
		while (flag) {
			flag = false;
			if (count >= 3) {
				dgEdge* ptr = edge;

				dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
				do {
					dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
					dgBigVector e0 (p1 - p0);
					dgFloat64 mag2 = e0.DotProduct3(e0);
					if (mag2 < dgFloat32 (1.0e-6f)) {
						count --;
						flag = true;
						edge = ptr->m_next;
						ptr->m_prev->m_next = ptr->m_next;
						ptr->m_next->m_prev = ptr->m_prev;
						ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
						ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
						break;
					}
					p0 = p1;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}
		if (count >= 3) {
			flag = true;
			dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));

			dgAssert (normal.DotProduct3(normal) > dgFloat32 (1.0e-10f)); 
			normal = normal.Scale (dgFloat64 (1.0f) / sqrt (normal.DotProduct3(normal) + dgFloat32 (1.0e-24f)));

			while (flag) {
				flag = false;
				if (count >= 3) {
					dgEdge* ptr = edge;

					dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
					dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
					dgBigVector e0 (p1 - p0);
					e0 = e0.Scale (dgFloat64 (1.0f) / sqrt (e0.DotProduct3(e0) + dgFloat32(1.0e-24f)));
					do {
						dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
						dgBigVector e1 (p2 - p1);

						e1 = e1.Scale (dgFloat64 (1.0f) / sqrt (e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));
						dgFloat64 mag2 = e1.DotProduct3(e0);
						if (mag2 > dgFloat32 (0.9999f)) {
							count --;
							flag = true;
							edge = ptr->m_next;
							ptr->m_prev->m_next = ptr->m_next;
							ptr->m_next->m_prev = ptr->m_prev;
							ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
							ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
							break;
						}

						dgBigVector n (e0.CrossProduct(e1));
						mag2 = n.DotProduct3(normal);
						if (mag2 < dgFloat32 (1.0e-5f)) {
							count --;
							flag = true;
							edge = ptr->m_next;
							ptr->m_prev->m_next = ptr->m_next;
							ptr->m_next->m_prev = ptr->m_prev;
							ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
							ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
							break;
						}

						e0 = e1;
						p1 = p2;
						ptr = ptr->m_next;
					} while (ptr != edge);
				}
			}
		}

		dgEdge* first = edge;
		if (count >= 3) {
			dgFloat64 best = dgFloat32 (2.0f);
			dgEdge* ptr = edge;

			dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
			dgBigVector e0 (p1 - p0);
			e0 = e0.Scale (dgFloat64 (1.0f) / sqrt (e0.DotProduct3(e0) + dgFloat32(1.0e-24f)));
			do {
				dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_next->m_incidentVertex].m_x);
				dgBigVector e1 (p2 - p1);

				e1 = e1.Scale (dgFloat64 (1.0f) / sqrt (e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));
				dgFloat64 mag2 = fabs (e1.DotProduct3(e0));
				if (mag2 < best) {
					best = mag2;
					first = ptr;
				}

				e0 = e1;
				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edge);

			count = 0;
			ptr = first;
			do {
				pool[count] = ptr->m_incidentVertex;
				count ++;
				ptr = ptr->m_next;
			} while (ptr != first);
		}

	#ifdef _DEBUG
		if (count >= 3) {
			dgInt32 j0 = count - 2;  
			dgInt32 j1 = count - 1;  
			dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));
			for (dgInt32 j2 = 0; j2 < count; j2 ++) { 
				dgBigVector p0 (&m_vertexPoints[pool[j0]].m_x);
				dgBigVector p1 (&m_vertexPoints[pool[j1]].m_x);
				dgBigVector p2 (&m_vertexPoints[pool[j2]].m_x);
				dgBigVector e0 ((p0 - p1));
				dgBigVector e1 ((p2 - p1));

				dgBigVector n (e1.CrossProduct(e0));
				dgAssert (n.DotProduct3(normal) > dgFloat32 (0.0f));
				j0 = j1;
				j1 = j2;
			}
		}
	#endif
	}

	return (count >= 3) ? count : 0;
}


dgInt32 dgPolygonSoupDatabaseBuilder::AddConvexFace (dgInt32 count, dgInt32* const pool, dgInt32* const facesArray)
{
	dgPolySoupFilterAllocator polyhedra(m_allocator);

	count = polyhedra.AddFilterFace(dgUnsigned32 (count), pool);

	dgEdge* edge = &polyhedra.GetRoot()->GetInfo();
	if (edge->m_incidentFace < 0) {
		edge = edge->m_twin;
	}

	
	dgInt32 isconvex = 1;
	dgInt32 facesCount = 0;

	dgInt32 flag = 1;
	while (flag) {
		flag = 0;
		if (count >= 3) {
			dgEdge* ptr = edge;

			dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			do {
				dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				dgFloat64 mag2 = e0.DotProduct3(e0);
				if (mag2 < dgFloat32 (1.0e-6f)) {
					count --;
					flag = 1;
					edge = ptr->m_next;
					ptr->m_prev->m_next = ptr->m_next;
					ptr->m_next->m_prev = ptr->m_prev;
					ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
					ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
					break;
				}
				p0 = p1;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
	if (count >= 3) {
		flag = 1;

		while (flag) {
			flag = 0;
			if (count >= 3) {
				dgEdge* ptr = edge;

				dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
				dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				e0 = e0.Scale (dgFloat64 (1.0f) / sqrt (e0.DotProduct3(e0) + dgFloat32(1.0e-24f)));
				do {
					dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
					dgBigVector e1 (p2 - p1);

					e1 = e1.Scale (dgFloat64 (1.0f) / sqrt (e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));
					dgFloat64 mag2 = e1.DotProduct3(e0);
					if (mag2 > dgFloat32 (0.9999f)) {
						count --;
						flag = 1;
						edge = ptr->m_next;
						ptr->m_prev->m_next = ptr->m_next;
						ptr->m_next->m_prev = ptr->m_prev;
						ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
						ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
						break;
					}

					e0 = e1;
					p1 = p2;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}

		dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));
		dgFloat64 mag2 = normal.DotProduct3(normal);
		if (mag2 < dgFloat32 (1.0e-8f)) {
			return 0;
		}
		normal = normal.Scale (dgFloat64 (1.0f) / sqrt (mag2));

		if (count >= 3) {
			dgEdge* ptr = edge;
			dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
			dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			dgBigVector e0 (p1 - p0);
			e0 = e0.Scale (dgFloat64 (1.0f) / sqrt (e0.DotProduct3(e0) + dgFloat32(1.0e-24f)));
			do {
				dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
				dgBigVector e1 (p2 - p1);

				e1 = e1.Scale (dgFloat64 (1.0f) / sqrt (e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));

				dgBigVector n (e0.CrossProduct(e1));
				dgFloat64 magnitud2 = n.DotProduct3(normal);
				if (magnitud2 < dgFloat32 (1.0e-5f)) {
					isconvex = 0;
					break;
				}

				e0 = e1;
				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	if (isconvex) {
		dgEdge* const first = edge;
		if (count >= 3) {
			count = 0;
			dgEdge* ptr = first;
			do {
				pool[count] = ptr->m_incidentVertex;
				count ++;
				ptr = ptr->m_next;
			} while (ptr != first);
			facesArray[facesCount] = count;
			facesCount = 1;
		}
	} else {
		dgPolyhedra leftOver(m_allocator);
		dgPolyhedra polyhedra2(m_allocator);
		dgEdge* ptr = edge;
		count = 0;
		do {
			pool[count] = ptr->m_incidentVertex;
			count ++;
			ptr = ptr->m_next;
		} while (ptr != edge);


		polyhedra2.BeginFace();
		polyhedra2.AddFace (count, pool);
		polyhedra2.EndFace();
		leftOver.BeginFace();
		polyhedra2.ConvexPartition (&m_vertexPoints[0].m_x, m_vertexPoints.GetElementSize(), &leftOver);
		leftOver.EndFace();

#if _DEBUG
		if (leftOver.GetCount()) {
			dgTrace (("warning: %d faces with more that a one shared edge\n", leftOver.GetCount()));
			dgTrace (("         this mesh is not a manifold and may lead to collision malfunctions\n"));
		}
#endif

		dgInt32 mark = polyhedra2.IncLRU();
		dgInt32 index = 0;
		dgPolyhedra::Iterator iter (polyhedra2);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge1 = &(*iter);
			if (edge1->m_incidentFace < 0) {
				continue;
			}
			if (edge1->m_mark == mark) {
				continue;
			}

			ptr = edge1;
			count = 0;
			do {
				ptr->m_mark = mark;
				pool[index] = ptr->m_incidentVertex;
				index ++;
				count ++;
				ptr = ptr->m_next;
			} while (ptr != edge1);

			facesArray[facesCount] = count;
			facesCount ++;
		}
	}

	return facesCount;
}




