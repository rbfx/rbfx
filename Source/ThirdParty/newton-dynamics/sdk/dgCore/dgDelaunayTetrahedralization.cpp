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

#include "dgStdafx.h"
#include "dgSort.h"
#include "dgStack.h"
#include "dgGoogol.h"
#include "dgSmallDeterminant.h"
#include "dgDelaunayTetrahedralization.h"

dgDelaunayTetrahedralization::dgDelaunayTetrahedralization(dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 count, dgInt32 strideInByte, dgFloat64 distTol)
	:dgConvexHull4d(allocator)
{
	dgSetPrecisionDouble precision;
	dgStack<dgBigVector> pool(count + 2);

	dgBigVector* const points = &pool[0];
	dgInt32 stride = dgInt32 (strideInByte / sizeof (dgFloat64));
	for (dgInt32 i = 0; i < count; i ++) {
		dgFloat64 x = dgRoundToFloat (vertexCloud[i * stride + 0]);
		dgFloat64 y = dgRoundToFloat (vertexCloud[i * stride + 1]);
		dgFloat64 z = dgRoundToFloat (vertexCloud[i * stride + 2]);
		points[i] = dgBigVector (x, y, z, x * x + y * y + z * z);
	}

	dgInt32 oldCount = count;
	BuildHull (allocator, &pool[0].m_x, sizeof (dgBigVector), count, distTol);
	if (oldCount > m_count) {
		// the mesh is convex, need to add two steiners point to make tractable
		dgBigVector origin (dgFloat64 (0.0f));
		dgFloat64 maxW = dgFloat64 (-1.0e20f);
		for (dgInt32 i = 0; i < count; i++) {
			dgFloat64 x = dgRoundToFloat(vertexCloud[i * stride + 0]);
			dgFloat64 y = dgRoundToFloat(vertexCloud[i * stride + 1]);
			dgFloat64 z = dgRoundToFloat(vertexCloud[i * stride + 2]);
			points[i] = dgBigVector(x, y, z, x * x + y * y + z * z);
			origin += points[i];
			maxW = dgMax (points[i].m_w, maxW);
		}
		origin = origin.Scale (dgFloat64 (1.0f) / count);
		points[count + 0] = origin;
		points[count + 1] = origin;
		points[count + 0].m_w += dgFloat64 (1.0f);
		points[count + 1].m_w -= dgFloat64 (1.0f);
 		BuildHull (allocator, &pool[0].m_x, sizeof (dgBigVector), count + 2, distTol);
	}
	if (oldCount > m_count) {
		// this is probably a regular convex solid, which will have a zero volume hull
		// add the rest of the points by incremental insertion with small perturbation
		dgInt32 hullCount = m_count;
		
		for (dgInt32 i = 0; i < count; i ++) {
			bool inHull = false;
			const dgConvexHull4dVector* const hullPoints = &m_points[0];
			for (dgInt32 j = 0; j < hullCount; j ++) {
				if (hullPoints[j].m_index == i) {
					inHull = true;
					break;
				}			
			}
			if (!inHull) {
				dgBigVector q (points[i]);
				dgInt32 index = AddVertex(q);
				if (index == -1) {
					q.m_x += dgFloat64 (1.0e-3f);
					q.m_y += dgFloat64 (1.0e-3f);
					q.m_z += dgFloat64 (1.0e-3f);
					index = AddVertex(q);
					dgAssert (index != -1);
				}
				dgAssert (index != -1);
				m_points[index].m_index = i;
			}
		}
	}

	SortVertexArray ();
}

dgDelaunayTetrahedralization::~dgDelaunayTetrahedralization()
{
}

dgInt32 dgDelaunayTetrahedralization::AddVertex (const dgBigVector& vertex)
{
	dgSetPrecisionDouble precision;

	dgBigVector p (vertex);
	p.m_w = p.DotProduct3(p);
	dgInt32 index = dgConvexHull4d::AddVertex(p);

	return index;
}

dgInt32 dgDelaunayTetrahedralization::CompareVertexByIndex(const dgConvexHull4dVector* const  A, const dgConvexHull4dVector* const B, void* const context)
{
	if (A->m_index < B ->m_index) {
		return -1;
	} else if (A->m_index > B->m_index) {
		return 1;
	}
	return 0;
}

void dgDelaunayTetrahedralization::SortVertexArray ()
{
	dgConvexHull4dVector* const points = &m_points[0];
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {	
		dgConvexHull4dTetraherum* const tetra = &node->GetInfo();
		for (dgInt32 i = 0; i < 4; i ++) {
			dgConvexHull4dTetraherum::dgTetrahedrumFace& face = tetra->m_faces[i];
			for (dgInt32 j = 0; j < 4; j ++) {
				dgInt32 index = face.m_index[j];
				face.m_index[j] = points[index].m_index;
			}
		}
	}

	dgSort(points, m_count, CompareVertexByIndex);
}

void dgDelaunayTetrahedralization::RemoveUpperHull ()
{
	dgSetPrecisionDouble precision;

	dgListNode* nextNode = NULL;
	for (dgListNode* node = GetFirst(); node; node = nextNode) {
		nextNode = node->GetNext();

		dgConvexHull4dTetraherum* const tetra = &node->GetInfo();
		tetra->SetMark(0);
		dgFloat64 w = GetTetraVolume (tetra);
		if (w >= dgFloat64 (0.0f)) {
			DeleteFace(node);
		}
	}
}

void dgDelaunayTetrahedralization::DeleteFace (dgListNode* const node)
{
	dgConvexHull4dTetraherum* const tetra = &node->GetInfo();
	for (dgInt32 i = 0; i < 4; i ++) {
		dgListNode* const twinNode = tetra->m_faces[i].m_twin;
		if (twinNode) {
			dgConvexHull4dTetraherum* const twinTetra = &twinNode->GetInfo();
			for (dgInt32 j = 0; j < 4; j ++) {
				if (twinTetra->m_faces[j].m_twin == node) {
					twinTetra->m_faces[j].m_twin = NULL;
					break;
				}
			}
		}
	}
	dgConvexHull4d::DeleteFace (node);
}




