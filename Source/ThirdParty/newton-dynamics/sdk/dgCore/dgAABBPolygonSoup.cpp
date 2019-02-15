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
#include "dgHeap.h"
#include "dgStack.h"
#include "dgList.h"
#include "dgMatrix.h"
#include "dgAABBPolygonSoup.h"
#include "dgPolygonSoupBuilder.h"


#define DG_STACK_DEPTH 512


DG_MSC_VECTOR_ALIGMENT
class dgAABBPolygonSoup::dgNodeBuilder: public dgAABBPolygonSoup::dgNode
{
	public:
	dgNodeBuilder (const dgVector& p0, const dgVector& p1)
		:dgNode()
		,m_left (NULL)
		,m_right (NULL)
		,m_parent (NULL)
		,m_indexBox0(0)
		,m_indexBox1(0)
		,m_enumeration(-1)
		,m_faceIndex(0)
		,m_indexCount(0)
		,m_faceIndices(NULL)
	{
		SetBox (p0, p1);
	}

	dgNodeBuilder (const dgVector* const vertexArray, dgInt32 faceIndex, dgInt32 indexCount, const dgInt32* const indexArray)
		:dgNode()
		,m_left (NULL)
		,m_right (NULL)
		,m_parent (NULL)
		,m_indexBox0(0)
		,m_indexBox1(0)
		,m_enumeration(-1)
		,m_faceIndex(faceIndex)
		,m_indexCount(indexCount)
		,m_faceIndices(indexArray)
	{
		dgVector minP ( dgFloat32 (1.0e15f)); 
		dgVector maxP (-dgFloat32 (1.0e15f)); 
		for (dgInt32 i = 0; i < indexCount; i ++) {
			dgInt32 index = indexArray[i];
			const dgVector& p (vertexArray[index]);
			minP = p.GetMin(minP); 
			maxP = p.GetMax(maxP); 
		}
		minP -= dgVector (dgFloat32 (1.0e-3f));
		maxP += dgVector (dgFloat32 (1.0e-3f));
		minP = minP & dgVector::m_triplexMask;
		maxP = maxP & dgVector::m_triplexMask;
		SetBox (minP, maxP);
	}

	dgNodeBuilder (dgNodeBuilder* const left, dgNodeBuilder* const right)
		:dgNode()
		,m_left(left)
		,m_right(right)
		,m_parent(NULL)
		,m_indexBox0(0)
		,m_indexBox1(0)
		,m_enumeration(-1)
		,m_faceIndex(0)
		,m_indexCount(0)
		,m_faceIndices(NULL)
	{
		m_left->m_parent = this;
		m_right->m_parent = this;

		dgVector p0 (left->m_p0.GetMin(right->m_p0));
		dgVector p1 (left->m_p1.GetMax(right->m_p1));
		SetBox(p0, p1);
	}

	void SetBox (const dgVector& p0, const dgVector& p1)
	{
		m_p0 = p0;
		m_p1 = p1;
		m_size = m_p1 - m_p0;
		m_origin = (m_p1 + m_p0) * dgVector::m_half;
		m_area = m_size.DotProduct(m_size.ShiftTripleRight()).m_x;
	}

	DG_INLINE static dgFloat32 CalculateSurfaceArea (dgNodeBuilder* const node0, dgNodeBuilder* const node1, dgVector& minBox, dgVector& maxBox)
	{
		minBox = node0->m_p0.GetMin(node1->m_p0);
		maxBox = node0->m_p1.GetMax(node1->m_p1);

		dgVector side0 ((maxBox - minBox) * dgVector::m_half);
		//dgVector side1 (side0.m_y, side0.m_z, side0.m_x, dgFloat32 (0.0f));
		dgVector side1 (side0.ShiftTripleLeft());
		return side0.DotProduct(side1).GetScalar();
	}

	dgVector m_p0;
	dgVector m_p1;
	dgVector m_size;
	dgVector m_origin;
	dgFloat32 m_area;
	
	dgNodeBuilder* m_left;
	dgNodeBuilder* m_right;
	dgNodeBuilder* m_parent;
	dgInt32 m_indexBox0;
	dgInt32 m_indexBox1;
	dgInt32 m_enumeration;
	dgInt32 m_faceIndex;
	dgInt32 m_indexCount;
	const dgInt32* m_faceIndices;
} DG_GCC_VECTOR_ALIGMENT;



class dgAABBPolygonSoup::dgSpliteInfo
{
	public:
	dgSpliteInfo (dgNodeBuilder* const boxArray, dgInt32 boxCount)
	{
		dgVector minP ( dgFloat32 (1.0e15f)); 
		dgVector maxP (-dgFloat32 (1.0e15f)); 

		if (boxCount == 2) {
			m_axis = 1;
			for (dgInt32 i = 0; i < boxCount; i ++) {
				const dgNodeBuilder& box = boxArray[i];
				const dgVector& p0 = box.m_p0;
				const dgVector& p1 = box.m_p1;
				minP = minP.GetMin (p0); 
				maxP = maxP.GetMax (p1); 
			}

		} else {
			dgVector median (dgFloat32 (0.0f));
			dgVector varian (dgFloat32 (0.0f));
			for (dgInt32 i = 0; i < boxCount; i ++) {
				const dgNodeBuilder& box = boxArray[i];

				const dgVector& p0 = box.m_p0;
				const dgVector& p1 = box.m_p1;

				minP = minP.GetMin (p0); 
				maxP = maxP.GetMax (p1); 
				dgVector p (dgVector::m_half * (p0 + p1));

				median += p;
				varian += p * p;
			}

			varian = varian.Scale (dgFloat32 (boxCount)) - median * median;

			dgInt32 index = 0;
			dgFloat32 maxVarian = dgFloat32 (-1.0e10f);
			for (dgInt32 i = 0; i < 3; i ++) {
				if (varian[i] > maxVarian) {
					index = i;
					maxVarian = varian[i];
				}
			}

			dgVector center = median.Scale (dgFloat32 (1.0f) / dgFloat32 (boxCount));
			dgFloat32 test = center[index];
			dgInt32 i0 = 0;
			dgInt32 i1 = boxCount - 1;
			do {    
				for (; i0 <= i1; i0 ++) {
					const dgNodeBuilder& box = boxArray[i0];
					dgFloat32 val = (box.m_p0[index] + box.m_p1[index]) * dgFloat32 (0.5f);
					if (val > test) {
						break;
					}
				}

				for (; i1 >= i0; i1 --) {
					const dgNodeBuilder& box = boxArray[i1];
					dgFloat32 val = (box.m_p0[index] + box.m_p1[index]) * dgFloat32 (0.5f);
					if (val < test) {
						break;
					}
				}

				if (i0 < i1)	{
					dgSwap(boxArray[i0], boxArray[i1]);
					i0++; 
					i1--;
				}
			} while (i0 <= i1);

			if (i0 > 0){
				i0 --;
			}
			if ((i0 + 1) >= boxCount) {
				i0 = boxCount - 2;
			}

			m_axis = i0 + 1;
		}

		dgAssert (maxP.m_x - minP.m_x >= dgFloat32 (0.0f));
		dgAssert (maxP.m_y - minP.m_y >= dgFloat32 (0.0f));
		dgAssert (maxP.m_z - minP.m_z >= dgFloat32 (0.0f));
		m_p0 = minP;
		m_p1 = maxP;
	}

	dgInt32 m_axis;
	dgVector m_p0;
	dgVector m_p1;
};



dgAABBPolygonSoup::dgAABBPolygonSoup ()
	:dgPolygonSoupDatabase()
	,m_nodesCount(0)
	,m_indexCount(0)
	,m_aabb(NULL)
	,m_indices(NULL)
{
}

dgAABBPolygonSoup::~dgAABBPolygonSoup ()
{
	if (m_aabb) {
		dgFreeStack (m_aabb);
		dgFreeStack (m_indices);
	}
}


void dgAABBPolygonSoup::ImproveNodeFitness (dgNodeBuilder* const node) const
{
	dgAssert (node->m_left);
	dgAssert (node->m_right);

	if (node->m_parent)	{
		dgAssert(node->m_parent->m_p0.m_w == dgFloat32(0.0f));
		dgAssert(node->m_parent->m_p1.m_w == dgFloat32(0.0f));

		if (node->m_parent->m_left == node) {
			dgFloat32 cost0 = node->m_area;

			dgVector cost1P0;
			dgVector cost1P1;		
			dgFloat32 cost1 = dgNodeBuilder::CalculateSurfaceArea (node->m_right, node->m_parent->m_right, cost1P0, cost1P1);

			dgVector cost2P0;
			dgVector cost2P1;		
			dgFloat32 cost2 = dgNodeBuilder::CalculateSurfaceArea (node->m_left, node->m_parent->m_right, cost2P0, cost2P1);

			if ((cost1 <= cost0) && (cost1 <= cost2)) {
				dgNodeBuilder* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 
				node->m_size = parent->m_size;
				node->m_origin = parent->m_origin;

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_right->m_parent = parent;
				parent->m_left = node->m_right;
				node->m_right = parent;
				parent->m_p0 = cost1P0;
				parent->m_p1 = cost1P1;		
				parent->m_area = cost1;
				parent->m_size = (parent->m_p1 - parent->m_p0) * dgVector::m_half;
				parent->m_origin = (parent->m_p1 + parent->m_p0) * dgVector::m_half;

			} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
				dgNodeBuilder* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 
				node->m_size = parent->m_size;
				node->m_origin = parent->m_origin;

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_left->m_parent = parent;
				parent->m_left = node->m_left;
				node->m_left = parent;

				parent->m_p0 = cost2P0;
				parent->m_p1 = cost2P1;		
				parent->m_area = cost2;
				parent->m_size = (parent->m_p1 - parent->m_p0) * dgVector::m_half;
				parent->m_origin = (parent->m_p1 + parent->m_p0) * dgVector::m_half;
			}
		} else {
			dgFloat32 cost0 = node->m_area;

			dgVector cost1P0;
			dgVector cost1P1;		
			dgFloat32 cost1 = dgNodeBuilder::CalculateSurfaceArea (node->m_left, node->m_parent->m_left, cost1P0, cost1P1);

			dgVector cost2P0;
			dgVector cost2P1;		
			dgFloat32 cost2 = dgNodeBuilder::CalculateSurfaceArea (node->m_right, node->m_parent->m_left, cost2P0, cost2P1);

			if ((cost1 <= cost0) && (cost1 <= cost2)) {
				dgNodeBuilder* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 
				node->m_size = parent->m_size;
				node->m_origin = parent->m_origin;

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_left->m_parent = parent;
				parent->m_right = node->m_left;
				node->m_left = parent;

				parent->m_p0 = cost1P0;
				parent->m_p1 = cost1P1;		
				parent->m_area = cost1;
				parent->m_size = (parent->m_p1 - parent->m_p0) * dgVector::m_half;
				parent->m_origin = (parent->m_p1 + parent->m_p0) * dgVector::m_half;

			} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
				dgNodeBuilder* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 
				node->m_size = parent->m_size;
				node->m_origin = parent->m_origin;

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_right->m_parent = parent;
				parent->m_right = node->m_right;
				node->m_right = parent;

				parent->m_p0 = cost2P0;
				parent->m_p1 = cost2P1;		
				parent->m_area = cost2;
				parent->m_size = (parent->m_p1 - parent->m_p0) * dgVector::m_half;
				parent->m_origin = (parent->m_p1 + parent->m_p0) * dgVector::m_half;
			}
		}
	} else {
		// in the future I can handle this but it is too much work for little payoff
	}
}

dgFloat32 dgAABBPolygonSoup::CalculateFaceMaxSize (const dgVector* const vertex, dgInt32 indexCount, const dgInt32* const indexArray) const
{
	dgFloat32 maxSize = dgFloat32 (0.0f);
	dgInt32 index = indexArray[indexCount - 1];
	dgVector p0 (vertex[index]);
	for (dgInt32 i = 0; i < indexCount; i ++) {
		dgInt32 index1 = indexArray[i];
		dgVector p1 (vertex[index1]);

		dgVector dir (p1 - p0);
		dgAssert (dir.m_w == dgFloat32 (0.0f));
		dir = dir.Normalize();

		dgFloat32 maxVal = dgFloat32 (-1.0e10f);
		dgFloat32 minVal = dgFloat32 ( 1.0e10f);
		for (dgInt32 j = 0; j < indexCount; j ++) {
			dgInt32 index2 = indexArray[j];
			dgVector q (vertex[index2]);
			dgFloat32 val = dir.DotProduct(q).GetScalar();
			minVal = dgMin(minVal, val);
			maxVal = dgMax(maxVal, val);
		}

		dgFloat32 size = maxVal - minVal;
		maxSize = dgMax(maxSize, size);
		p0 = p1;
	}

	return dgFloor (maxSize + dgFloat32 (1.0f));
}

void dgAABBPolygonSoup::GetAABB (dgVector& p0, dgVector& p1) const
{
	if (m_aabb) { 
		GetNodeAABB (m_aabb, p0, p1);
	} else {
		p0 = dgVector::m_zero;
		p1 = dgVector::m_zero;
	}
}

void dgAABBPolygonSoup::CalculateAdjacendy ()
{
	dgVector p0;
	dgVector p1;
	GetAABB (p0, p1);
	dgFastAABBInfo box (p0, p1);
	ForAllSectors (box, dgVector (dgFloat32 (0.0f)), dgFloat32 (1.0f), CalculateAllFaceEdgeNormals, this);

	dgStack<dgTriplex> pool ((m_indexCount / 2) - 1);
	const dgTriplex* const vertexArray = (dgTriplex*)GetLocalVertexPool();
	dgInt32 normalCount = 0;
	for (dgInt32 i = 0; i < m_nodesCount; i ++) {
		const dgNode* const node = &m_aabb[i];
		if (node->m_left.IsLeaf()) {
			dgInt32 vCount = dgInt32 (node->m_left.GetCount());
			if (vCount) {
				dgInt32 index = dgInt32 (node->m_left.GetIndex());
				dgInt32* const face = &m_indices[index];

				dgInt32 j0 = 2 * (vCount + 1) - 1;
				dgVector normal (&vertexArray[face[vCount + 1]].m_x);
				normal = normal & dgVector::m_triplexMask;
				dgAssert (dgAbs (normal.DotProduct(normal).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
				dgVector q0 (&vertexArray[face[vCount - 1]].m_x);
				q0 = q0 & dgVector::m_triplexMask;
				for (dgInt32 j = 0; j < vCount; j ++) {
					dgInt32 j1 = vCount + 2 + j;
					dgVector q1 (&vertexArray[face[j]].m_x);
					q1 = q1 & dgVector::m_triplexMask;
					if (face[j0] == -1) {
						dgVector e (q1 - q0);
						dgVector n (e.CrossProduct(normal));
						n = n.Scale(dgFloat32 (1.0f) / dgSqrt (n.DotProduct(n).GetScalar()));
						dgAssert (dgAbs (n.DotProduct(n).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
						pool[normalCount].m_x = n.m_x;
						pool[normalCount].m_y = n.m_y;
						pool[normalCount].m_z = n.m_z;
						face[j0] = -normalCount - 1;
						normalCount ++;
					}
					q0 = q1;
					j0 = j1;
				}
			}
		}

		if (node->m_right.IsLeaf()) {
			dgInt32 vCount = dgInt32 (node->m_right.GetCount());
			if (vCount) {
				dgInt32 index = dgInt32 (node->m_right.GetIndex());
				dgInt32* const face = &m_indices[index];

				dgInt32 j0 = 2 * (vCount + 1) - 1;
				dgVector normal (&vertexArray[face[vCount + 1]].m_x);
				normal = normal & dgVector::m_triplexMask;
				dgAssert (dgAbs (normal.DotProduct(normal).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
				dgVector q0 (&vertexArray[face[vCount - 1]].m_x);
				q0 = q0 & dgVector::m_triplexMask;
				for (dgInt32 j = 0; j < vCount; j ++) {
					dgInt32 j1 = vCount + 2 + j;
					dgVector q1 (&vertexArray[face[j]].m_x);
					q1 = q1 & dgVector::m_triplexMask;
					if (face[j0] == -1) {
						dgVector e (q1 - q0);
						dgVector n (e.CrossProduct(normal));
						n = n.Scale(dgFloat32 (1.0f) / dgSqrt (n.DotProduct(n).GetScalar()));
						dgAssert (dgAbs (n.DotProduct(n).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
						pool[normalCount].m_x = n.m_x;
						pool[normalCount].m_y = n.m_y;
						pool[normalCount].m_z = n.m_z;
						face[j0] = -normalCount - 1;
						normalCount ++;
					}
					q0 = q1;
					j0 = j1;
				}
			}
		}
	}

	if (normalCount) {
		dgStack<dgInt32> indexArray (normalCount);
		dgInt32 newNormalCount = dgVertexListToIndexList (&pool[0].m_x, sizeof (dgTriplex), sizeof (dgTriplex), 0, normalCount, &indexArray[0], dgFloat32 (1.0e-6f));

		dgInt32 oldCount = GetVertexCount();
		dgTriplex* const vertexArray1 = (dgTriplex*) dgMallocStack (sizeof (dgTriplex) * (oldCount + newNormalCount));
		memcpy (vertexArray1, GetLocalVertexPool(), sizeof (dgTriplex) * oldCount);
		memcpy (&vertexArray1[oldCount], &pool[0].m_x, sizeof (dgTriplex) * newNormalCount);
		dgFreeStack(GetLocalVertexPool());

		m_localVertex = &vertexArray1[0].m_x;
		m_vertexCount = oldCount + newNormalCount;

		for (dgInt32 i = 0; i < m_nodesCount; i ++) {
			const dgNode* const node = &m_aabb[i];
			if (node->m_left.IsLeaf()) {
				dgInt32 vCount = dgInt32 (node->m_left.GetCount());
				dgInt32 index = dgInt32 (node->m_left.GetIndex());
				dgInt32* const face = &m_indices[index];
				for (dgInt32 j = 0; j < vCount; j ++) {
					if (face[vCount + 2 + j] < 0) {
						dgInt32 k = -1 - face[vCount + 2 + j];
						face[vCount + 2 + j] = indexArray[k] + oldCount;
					}
					#ifdef _DEBUG	
						dgVector normal (&vertexArray1[face[vCount + 2 + j]].m_x);
						normal = normal & dgVector::m_triplexMask;
						dgAssert (dgAbs (normal.DotProduct(normal).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
					#endif
				}
			}

			if (node->m_right.IsLeaf()) {
				dgInt32 vCount = dgInt32 (node->m_right.GetCount());
				dgInt32 index = dgInt32 (node->m_right.GetIndex());
				dgInt32* const face = &m_indices[index];
				for (dgInt32 j = 0; j < vCount; j ++) {
					if (face[vCount + 2 + j] < 0) {
						dgInt32 k = -1 - face[vCount + 2 + j];
						face[vCount + 2 + j] = indexArray[k] + oldCount;
					}

					#ifdef _DEBUG	
						dgVector normal (&vertexArray1[face[vCount + 2 + j]].m_x);
						normal = normal & dgVector::m_triplexMask;
						dgAssert (dgAbs (normal.DotProduct(normal).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-6f));
					#endif
				}
			}
		}
	}
}

dgIntersectStatus dgAABBPolygonSoup::CalculateAllFaceEdgeNormals (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));

	AdjacentdFace adjacentFaces;
	adjacentFaces.m_count = indexCount;
	adjacentFaces.m_index = (dgInt32*) indexArray;

	dgVector n (&polygon[indexArray[indexCount + 1] * stride]);
	dgVector p (&polygon[indexArray[0] * stride]);
	n = n & dgVector::m_triplexMask;
	p = p & dgVector::m_triplexMask;
	adjacentFaces.m_normal = dgPlane (n, - n.DotProduct(p).GetScalar());

	dgAssert (indexCount < dgInt32 (sizeof (adjacentFaces.m_edgeMap) / sizeof (adjacentFaces.m_edgeMap[0])));

	dgInt32 edgeIndex = indexCount - 1;
	dgInt32 i0 = indexArray[indexCount - 1];
	dgVector p0 ( dgFloat32 (1.0e15f),  dgFloat32 (1.0e15f),  dgFloat32 (1.0e15f), dgFloat32 (0.0f)); 
	dgVector p1 (-dgFloat32 (1.0e15f), -dgFloat32 (1.0e15f), -dgFloat32 (1.0e15f), dgFloat32 (0.0f)); 
	for (dgInt32 i = 0; i < indexCount; i ++) {
		dgInt32 i1 = indexArray[i];
		dgInt32 index = i1 * stride;
		dgVector point (&polygon[index]);
		point = point & dgVector::m_triplexMask;
		p0 = p0.GetMin(point);
		p1 = p1.GetMax(point);
		adjacentFaces.m_edgeMap[edgeIndex] = (dgInt64 (i1) << 32) + i0;
		edgeIndex = i;
		i0 = i1;
	}

	dgFloat32 padding = dgFloat32 (1.0f/16.0f);
	p0.m_x -= padding;
	p0.m_y -= padding;
	p0.m_z -= padding;
	p1.m_x += padding;
	p1.m_y += padding;
	p1.m_z += padding;

	dgAABBPolygonSoup* const me = (dgAABBPolygonSoup*) context;
	dgFastAABBInfo box (p0, p1);
	me->ForAllSectors (box, dgVector (dgFloat32 (0.0f)), dgFloat32 (1.0f), CalculateDisjointedFaceEdgeNormals, &adjacentFaces);
	return t_ContinueSearh;
}


dgIntersectStatus dgAABBPolygonSoup::CalculateDisjointedFaceEdgeNormals (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance)
{
	#define DG_WELDING_TOL (1.0e-2f)
	#define DG_WELDING_TOL2 (DG_WELDING_TOL * DG_WELDING_TOL)

	const AdjacentdFace& adjacentFace = *((AdjacentdFace*)context);

	if (adjacentFace.m_index != indexArray) {	
		dgInt32 adjacentCount = adjacentFace.m_count;
		dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));

		dgInt32 j0 = adjacentCount - 1;
		dgInt32 indexJ0 = adjacentFace.m_index[adjacentCount - 1];
		for (dgInt32 j = 0; j < adjacentCount; j ++) {
			dgInt32 indexJ1 = adjacentFace.m_index[j];
			dgBigVector q0 (dgVector(&polygon[indexJ1 * stride]) & dgVector::m_triplexMask);
			dgBigVector q1 (dgVector(&polygon[indexJ0 * stride]) & dgVector::m_triplexMask);
			dgBigVector q1q0 (q1 - q0);
			dgFloat64 q1q0Mag2 = q1q0.DotProduct(q1q0).GetScalar();

			dgInt32 indexI0 = indexArray[indexCount - 1];
			for (dgInt32 i = 0; i < indexCount; i ++) {
				dgInt32 indexI1 = indexArray[i];
				dgBigVector p0 (dgVector(&polygon[indexI0 * stride]) & dgVector::m_triplexMask);
				dgBigVector p1 (dgVector(&polygon[indexI1 * stride]) & dgVector::m_triplexMask);
				dgBigVector p1p0 (p1 - p0);
				dgFloat64 dot = p1p0.DotProduct(q1q0).GetScalar();
				if (dot > 0.0f) {
					dgFloat64 q1p0Mag2 = p1p0.DotProduct(p1p0).GetScalar();
					if ((dot * dot) >= (q1p0Mag2 * q1q0Mag2 * dgFloat64(0.99995f))) {
						dgFloat64 x0 = q0.DotProduct(q1q0).GetScalar();
						dgFloat64 x1 = q1.DotProduct(q1q0).GetScalar();
						dgFloat64 y0 = p0.DotProduct(q1q0).GetScalar();
						dgFloat64 y1 = p1.DotProduct(q1q0).GetScalar();
						dgAssert (x1 > x0);
						dgAssert (y1 > y0);
						if (!((y0 >= x1) || (y1 <= x0))) {
							dgFloat64 t = q1q0.DotProduct(p0 - q0).GetScalar() / q1q0Mag2;
							dgAssert (q1q0.m_w == dgFloat32 (0.0f));
							dgBigVector q (q0 + q1q0.Scale(t));
							dgBigVector dist (p0 - q);
							dgAssert (dist.m_w == dgFloat32 (0.0f));
							dgFloat64 err2 = dist.DotProduct(dist).GetScalar();
							if (err2 < DG_WELDING_TOL2) {
								dgFloat32 maxDist = dgFloat32 (0.0f);
								for (dgInt32 k = 0; k < indexCount; k ++) {
									dgVector r (&polygon[indexArray[k] * stride]);
									r = r & dgVector::m_triplexMask;
									dgFloat32 dist1 = adjacentFace.m_normal.Evalue(r);
									if (dgAbs (dist1) > dgAbs (maxDist)) {
										maxDist = dist1;
									}
								}

								if (adjacentFace.m_index[j0 + adjacentCount + 2] == -1) {
									if (maxDist < -dgFloat32 (1.0e-3f)) {
										adjacentFace.m_index[j0 + adjacentCount + 2] = indexArray[indexCount + 1];
									} else {
										adjacentFace.m_index[j0 + adjacentCount + 2] = adjacentFace.m_index[adjacentCount + 1];
									}
								} else {
									if (maxDist < -dgFloat32 (1.0e-3f)) {
										dgBigVector n0 (adjacentFace.m_normal[0], adjacentFace.m_normal[1], adjacentFace.m_normal[2], dgFloat64(0.0f));
										dgBigVector n1 (dgVector(&polygon[adjacentFace.m_index[j0 + adjacentCount + 2] * stride]) & dgVector::m_triplexMask);
										dgBigVector n2 (dgVector(&polygon[indexArray[indexCount + 1] * stride]) & dgVector::m_triplexMask);

										dgBigVector tilt0 (n0.CrossProduct(n1)); 
										dgBigVector tilt1 (n0.CrossProduct(n2)); 
										dgFloat64 dist0 (q1q0.DotProduct(tilt0).GetScalar());
										dgFloat64 dist1 (q1q0.DotProduct(tilt1).GetScalar());
										if (dist0 < dist1) {
											adjacentFace.m_index[j0 + adjacentCount + 2] = indexArray[indexCount + 1];
										}
									} else {
										adjacentFace.m_index[j0 + adjacentCount + 2] = adjacentFace.m_index[adjacentCount + 1];
									}
								}
								break;
							}
						}
					}
				}
				indexI0 = indexI1;
			}
			j0 = j;
			indexJ0 = indexJ1;
		}
	}
	return t_ContinueSearh;
}



dgAABBPolygonSoup::dgNodeBuilder* dgAABBPolygonSoup::BuildTopDown (dgNodeBuilder* const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgNodeBuilder** const allocator) const
{
	dgAssert (firstBox >= 0);
	dgAssert (lastBox >= 0);

	if (lastBox == firstBox) {
		return &leafArray[firstBox];
	} else {
		dgSpliteInfo info (&leafArray[firstBox], lastBox - firstBox + 1);

		dgNodeBuilder* const parent = new (*allocator) dgNodeBuilder (info.m_p0, info.m_p1);
		*allocator = *allocator + 1;

		dgAssert (parent);
		parent->m_right = BuildTopDown (leafArray, firstBox + info.m_axis, lastBox, allocator);
		parent->m_right->m_parent = parent;

		parent->m_left = BuildTopDown (leafArray, firstBox, firstBox + info.m_axis - 1, allocator);
		parent->m_left->m_parent = parent;
		return parent;
	}
}





void dgAABBPolygonSoup::Create (const dgPolygonSoupDatabaseBuilder& builder, bool optimizedBuild)
{
	if (builder.m_faceCount == 0) {
		return;
	}
	dgAssert (builder.m_faceCount >= 1);
	m_strideInBytes = sizeof (dgTriplex);
	m_nodesCount = ((builder.m_faceCount - 1) < 1) ? 1 : builder.m_faceCount - 1;
	m_aabb = (dgNode*) dgMallocStack (sizeof (dgNode) * m_nodesCount);
	m_indexCount = builder.m_indexCount * 2 + builder.m_faceCount;
	if (builder.m_faceCount == 1) {
		m_indexCount *= 2;
	}
	m_indices = (dgInt32*) dgMallocStack (sizeof (dgInt32) * m_indexCount);
	dgStack<dgVector> tmpVertexArrayCont(builder.m_vertexCount + builder.m_normalCount + builder.m_faceCount * 2 + 4);

	dgVector* const tmpVertexArray = &tmpVertexArrayCont[0];
	for (dgInt32 i = 0; i < builder.m_vertexCount; i ++) {
		tmpVertexArray[i] = builder.m_vertexPoints[i];
	}

	for (dgInt32 i = 0; i < builder.m_normalCount; i ++) {
		tmpVertexArray[i + builder.m_vertexCount] = builder.m_normalPoints[i];
	}

	const dgInt32* const indices = &builder.m_vertexIndex[0];
	dgStack<dgNodeBuilder> constructor (builder.m_faceCount * 2 + 16); 

	dgInt32 polygonIndex = 0;
	dgInt32 allocatorIndex = 0;
	if (builder.m_faceCount == 1) {
		dgInt32 indexCount = builder.m_faceVertexCount[0] - 1;
		new (&constructor[allocatorIndex]) dgNodeBuilder (&tmpVertexArray[0], 0, indexCount, &indices[0]);
		allocatorIndex ++;
	}
	for (dgInt32 i = 0; i < builder.m_faceCount; i ++) {
		dgInt32 indexCount = builder.m_faceVertexCount[i] - 1;
		new (&constructor[allocatorIndex]) dgNodeBuilder (&tmpVertexArray[0], i, indexCount, &indices[polygonIndex]);
		allocatorIndex ++;
		polygonIndex += (indexCount + 1);
	}

	dgNodeBuilder* contructorAllocator = &constructor[allocatorIndex];
	dgNodeBuilder* root = BuildTopDown (&constructor[0], 0, allocatorIndex - 1, &contructorAllocator);

	dgAssert (root);
	if (root->m_left) {

		dgAssert (root->m_right);
		dgList<dgNodeBuilder*> list (builder.m_allocator);
		dgList<dgNodeBuilder*> stack (builder.m_allocator);
		stack.Append(root);
		while (stack.GetCount()) {
			dgList<dgNodeBuilder*>::dgListNode* const stackNode = stack.GetLast();
			dgNodeBuilder* const node = stackNode->GetInfo();
			stack.Remove(stackNode);

			if (node->m_left) {
				dgAssert (node->m_right);
				list.Append(node);
				stack.Append(node->m_right);
				stack.Append(node->m_left);
			} 
		}

		dgFloat64 newCost = dgFloat32 (1.0e20f);
		dgFloat64 prevCost = newCost;
		do {
			prevCost = newCost;
			for (dgList<dgNodeBuilder*>::dgListNode* listNode = list.GetFirst(); listNode; listNode = listNode->GetNext()) {
				dgNodeBuilder* const node = listNode->GetInfo();
				ImproveNodeFitness (node);
			}

			newCost = dgFloat32 (0.0f);
			for (dgList<dgNodeBuilder*>::dgListNode* listNode = list.GetFirst(); listNode; listNode = listNode->GetNext()) {
				dgNodeBuilder* const node = listNode->GetInfo();
				newCost += node->m_area;
			}
		} while (newCost < (prevCost * dgFloat32 (0.9999f)));

		root = list.GetLast()->GetInfo();
		while (root->m_parent) {
			root = root->m_parent;
		}
	}

	dgList<dgNodeBuilder*> list (builder.m_allocator);

	list.Append(root);
	dgInt32 nodeIndex = 0;
	while (list.GetCount())  {
		dgNodeBuilder* const node = list.GetFirst()->GetInfo();
		list.Remove(list.GetFirst());

		if (node->m_left) {
			node->m_enumeration = nodeIndex;
			nodeIndex ++;
			dgAssert (node->m_right);
			list.Append(node->m_left);
			list.Append(node->m_right);
		}
	}
	dgAssert(!list.GetCount());

	dgInt32 aabbBase = builder.m_vertexCount + builder.m_normalCount;


	dgVector* const aabbPoints = &tmpVertexArray[aabbBase];


	dgInt32 vertexIndex = 0;
	dgInt32 aabbNodeIndex = 0;
	list.Append(root);
	dgInt32 indexMap = 0;
	while (list.GetCount())  {

		dgNodeBuilder* const node = list.GetFirst()->GetInfo();
		list.Remove(list.GetFirst());

		if (node->m_enumeration >= 0) {
			dgAssert (node->m_left);
			dgAssert (node->m_right);
			dgNode& aabbNode = m_aabb[aabbNodeIndex];
			aabbNodeIndex ++;
			dgAssert (aabbNodeIndex <= m_nodesCount);

			if (node->m_parent) {
				if (node->m_parent->m_left == node) {
					m_aabb[node->m_parent->m_enumeration].m_left = dgNode::dgLeafNodePtr (dgUnsigned32 (&m_aabb[node->m_enumeration] - m_aabb));
				} else {
					dgAssert (node->m_parent->m_right == node);
					m_aabb[node->m_parent->m_enumeration].m_right = dgNode::dgLeafNodePtr (dgUnsigned32 (&m_aabb[node->m_enumeration] - m_aabb));
				}
			}

			aabbPoints[vertexIndex + 0] = node->m_p0;
			aabbPoints[vertexIndex + 1] = node->m_p1;

			aabbNode.m_indexBox0 = aabbBase + vertexIndex;
			aabbNode.m_indexBox1 = aabbBase + vertexIndex + 1;

			vertexIndex += 2;

		} else {
			dgAssert (!node->m_left);
			dgAssert (!node->m_right);

			if (node->m_parent) {
				if (node->m_parent->m_left == node) {
					m_aabb[node->m_parent->m_enumeration].m_left = dgNode::dgLeafNodePtr (dgUnsigned32(node->m_indexCount), dgUnsigned32(indexMap));
				} else {
					dgAssert (node->m_parent->m_right == node);
					m_aabb[node->m_parent->m_enumeration].m_right = dgNode::dgLeafNodePtr (dgUnsigned32(node->m_indexCount), dgUnsigned32(indexMap));
				}
			}

			// index format i0, i1, i2, ... , id, normal, e0Normal, e1Normal, e2Normal, ..., faceSize
			for (dgInt32 j = 0; j < node->m_indexCount; j ++) {
				m_indices[indexMap + j] = node->m_faceIndices[j];
				m_indices[indexMap + j + node->m_indexCount + 2] = -1;
			}

			// face attribute
			m_indices[indexMap + node->m_indexCount] = node->m_faceIndices[node->m_indexCount];
			// face normal
			m_indices[indexMap + node->m_indexCount + 1] = builder.m_vertexCount + builder.m_normalIndex[node->m_faceIndex];
			// face size
			m_indices[indexMap + node->m_indexCount * 2 + 2] = dgInt32 (CalculateFaceMaxSize (&tmpVertexArray[0], node->m_indexCount, node->m_faceIndices));

			indexMap += node->m_indexCount * 2 + 3;
		}

		if (node->m_left) {
			dgAssert (node->m_right);
			list.Append(node->m_left);
			list.Append(node->m_right);
		}
	}

	dgStack<dgInt32> indexArray (vertexIndex);
	dgInt32 aabbPointCount = dgVertexListToIndexList (&aabbPoints[0].m_x, sizeof (dgVector), sizeof (dgTriplex), 0, vertexIndex, &indexArray[0], dgFloat32 (1.0e-6f));

	m_vertexCount = aabbBase + aabbPointCount;
	m_localVertex = (dgFloat32*) dgMallocStack (sizeof (dgTriplex) * m_vertexCount);

	dgTriplex* const dstPoints = (dgTriplex*)m_localVertex;
	for (dgInt32 i = 0; i < m_vertexCount; i ++) {
		dstPoints[i].m_x = tmpVertexArray[i].m_x;
		dstPoints[i].m_y = tmpVertexArray[i].m_y;
		dstPoints[i].m_z = tmpVertexArray[i].m_z;
	}

	for (dgInt32 i = 0; i < m_nodesCount; i ++) {
		dgNode& box = m_aabb[i];

		dgInt32 j = box.m_indexBox0 - aabbBase;
		box.m_indexBox0 = indexArray[j] + aabbBase;

		j = box.m_indexBox1 - aabbBase;
		box.m_indexBox1 = indexArray[j] + aabbBase;
	}

	if (builder.m_faceCount == 1) {
		m_aabb[0].m_right = dgNode::dgLeafNodePtr (0, 0);
	}
//	CalculateAdjacendy();
}

void dgAABBPolygonSoup::Serialize (dgSerialize callback, void* const userData) const
{
	callback (userData, &m_vertexCount, sizeof (dgInt32));
	callback (userData, &m_indexCount, sizeof (dgInt32));
	callback (userData, &m_nodesCount, sizeof (dgInt32));
	callback (userData, &m_nodesCount, sizeof (dgInt32));
	if (m_aabb) {
		callback (userData,  m_localVertex, dgInt32 (sizeof (dgTriplex) * m_vertexCount));
		callback (userData,  m_indices, dgInt32 (sizeof (dgInt32) * m_indexCount));
		callback (userData, m_aabb, dgInt32 (sizeof (dgNode) * m_nodesCount));
	}
}

void dgAABBPolygonSoup::Deserialize (dgDeserialize callback, void* const userData, dgInt32 revisionNumber)
{
	m_strideInBytes = sizeof (dgTriplex);
	callback (userData, &m_vertexCount, sizeof (dgInt32));
	callback (userData, &m_indexCount, sizeof (dgInt32));
	callback (userData, &m_nodesCount, sizeof (dgInt32));
	callback (userData, &m_nodesCount, sizeof (dgInt32));

	if (m_vertexCount) {
		m_localVertex = (dgFloat32*) dgMallocStack (sizeof (dgTriplex) * m_vertexCount);
		m_indices = (dgInt32*) dgMallocStack (sizeof (dgInt32) * m_indexCount);
		m_aabb = (dgNode*) dgMallocStack (sizeof (dgNode) * m_nodesCount);

		callback (userData, m_localVertex, dgInt32 (sizeof (dgTriplex) * m_vertexCount));
		callback (userData, m_indices, dgInt32 (sizeof (dgInt32) * m_indexCount));
		callback (userData, m_aabb, dgInt32 (sizeof (dgNode) * m_nodesCount));
	} else {
		m_localVertex = NULL;
		m_indices = NULL;
		m_aabb = NULL;
	}
}


dgVector dgAABBPolygonSoup::ForAllSectorsSupportVectex (const dgVector& dir) const
{
	dgVector supportVertex (dgFloat32 (0.0f));
	if (m_aabb) {
		dgFloat32 aabbProjection[DG_STACK_DEPTH];
		const dgNode *stackPool[DG_STACK_DEPTH];

		dgInt32 stack = 1;
		stackPool[0] = m_aabb;
		aabbProjection[0] = dgFloat32 (1.0e10f);
		const dgTriplex* const boxArray = (dgTriplex*)m_localVertex;
		

		dgFloat32 maxProj = dgFloat32 (-1.0e20f); 
		dgInt32 ix = (dir[0] > dgFloat32 (0.0f)) ? 1 : 0;
		dgInt32 iy = (dir[1] > dgFloat32 (0.0f)) ? 1 : 0;
		dgInt32 iz = (dir[2] > dgFloat32 (0.0f)) ? 1 : 0;

		while (stack) {
			dgFloat32 boxSupportValue;

			stack--;
			boxSupportValue = aabbProjection[stack];
			if (boxSupportValue > maxProj) {
				dgFloat32 backSupportDist = dgFloat32 (0.0f);
				dgFloat32 frontSupportDist = dgFloat32 (0.0f);
				const dgNode* const me = stackPool[stack];
				if (me->m_left.IsLeaf()) {
					backSupportDist = dgFloat32 (-1.0e20f);
					dgInt32 index = dgInt32 (me->m_left.GetIndex());
					dgInt32 vCount = dgInt32 (me->m_left.GetCount());
					dgVector vertex (dgFloat32 (0.0f));
					for (dgInt32 j = 0; j < vCount; j ++) {
						dgInt32 i0 = m_indices[index + j] * dgInt32 (sizeof (dgTriplex) / sizeof (dgFloat32));
						dgVector p (&boxArray[i0].m_x);
						p = p & dgVector::m_triplexMask;
						dgFloat32 dist = p.DotProduct(dir).GetScalar();
						if (dist > backSupportDist) {
							backSupportDist = dist;
							vertex = p;
						}
					}

					if (backSupportDist > maxProj) {
						maxProj = backSupportDist;
						supportVertex = vertex; 
					}

				} else {
					dgVector box[2];
					const dgNode* const node = me->m_left.GetNode(m_aabb);
					box[0].m_x = boxArray[node->m_indexBox0].m_x;
					box[0].m_y = boxArray[node->m_indexBox0].m_y;
					box[0].m_z = boxArray[node->m_indexBox0].m_z;
					box[1].m_x = boxArray[node->m_indexBox1].m_x;
					box[1].m_y = boxArray[node->m_indexBox1].m_y;
					box[1].m_z = boxArray[node->m_indexBox1].m_z;

					dgVector supportPoint (box[ix].m_x, box[iy].m_y, box[iz].m_z, dgFloat32 (0.0));
					backSupportDist = supportPoint.DotProduct(dir).GetScalar();
				}

				if (me->m_right.IsLeaf()) {
					frontSupportDist = dgFloat32 (-1.0e20f);
					dgInt32 index = dgInt32 (me->m_right.GetIndex());
					dgInt32 vCount = dgInt32 (me->m_right.GetCount());
					dgVector vertex (dgFloat32 (0.0f));
					for (dgInt32 j = 0; j < vCount; j ++) {
						dgInt32 i0 = m_indices[index + j] * dgInt32 (sizeof (dgTriplex) / sizeof (dgFloat32));
						dgVector p (&boxArray[i0].m_x);
						p = p & dgVector::m_triplexMask;
						dgFloat32 dist = p.DotProduct(dir).GetScalar();
						if (dist > frontSupportDist) {
							frontSupportDist = dist;
							vertex = p;
						}
					}
					if (frontSupportDist > maxProj) {
						maxProj = frontSupportDist;
						supportVertex = vertex; 
					}

				} else {
					dgVector box[2];
					const dgNode* const node = me->m_right.GetNode(m_aabb);
					box[0].m_x = boxArray[node->m_indexBox0].m_x;
					box[0].m_y = boxArray[node->m_indexBox0].m_y;
					box[0].m_z = boxArray[node->m_indexBox0].m_z;
					box[1].m_x = boxArray[node->m_indexBox1].m_x;
					box[1].m_y = boxArray[node->m_indexBox1].m_y;
					box[1].m_z = boxArray[node->m_indexBox1].m_z;

					dgVector supportPoint (box[ix].m_x, box[iy].m_y, box[iz].m_z, dgFloat32 (0.0f));
					frontSupportDist = supportPoint.DotProduct(dir).GetScalar();
				}

				if (frontSupportDist >= backSupportDist) {
					if (!me->m_left.IsLeaf()) {
						aabbProjection[stack] = backSupportDist;
						stackPool[stack] = me->m_left.GetNode(m_aabb);
						stack++;
					}

					if (!me->m_right.IsLeaf()) {
						aabbProjection[stack] = frontSupportDist;
						stackPool[stack] = me->m_right.GetNode(m_aabb);
						stack++;
					}

				} else {

					if (!me->m_right.IsLeaf()) {
						aabbProjection[stack] = frontSupportDist;
						stackPool[stack] = me->m_right.GetNode(m_aabb);
						stack++;
					}

					if (!me->m_left.IsLeaf()) {
						aabbProjection[stack] = backSupportDist;
						stackPool[stack] = me->m_left.GetNode(m_aabb);
						stack++;
					}
				}
			}
		}
	}
	return supportVertex;
}


void dgAABBPolygonSoup::ForAllSectorsRayHit (const dgFastRayTest& raySrc, dgFloat32 maxParam, dgRayIntersectCallback callback, void* const context) const
{
	const dgNode *stackPool[DG_STACK_DEPTH];
	dgFloat32 distance[DG_STACK_DEPTH];
	dgFastRayTest ray (raySrc);

	dgInt32 stack = 1;
	const dgTriplex* const vertexArray = (dgTriplex*) m_localVertex;

	stackPool[0] = m_aabb;
	distance[0] = m_aabb->RayDistance(ray, vertexArray);
	while (stack) {
		stack --;
		dgFloat32 dist = distance[stack];
		if (dist > maxParam) {
			break;
		} else {
			const dgNode *const me = stackPool[stack];
			if (me->m_left.IsLeaf()) {
				dgInt32 vCount = dgInt32 (me->m_left.GetCount());
				if (vCount > 0) {
					dgInt32 index = dgInt32 (me->m_left.GetIndex());
					dgFloat32 param = callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), &m_indices[index], vCount);
					dgAssert (param >= dgFloat32 (0.0f));
					if (param < maxParam) {
						maxParam = param;
						if (maxParam == dgFloat32 (0.0f)) {
							break;
						}
					}
				}

			} else {
				const dgNode* const node = me->m_left.GetNode(m_aabb);
				dgFloat32 dist1 = node->RayDistance(ray, vertexArray);
				if (dist1 < maxParam) {
					dgInt32 j = stack;
					for ( ; j && (dist1 > distance[j - 1]); j --) {
						stackPool[j] = stackPool[j - 1];
						distance[j] = distance[j - 1];
					}
					dgAssert (stack < DG_STACK_DEPTH);
					stackPool[j] = node;
					distance[j] = dist1;
					stack++;
				}
			}

			if (me->m_right.IsLeaf()) {
				dgInt32 vCount = dgInt32 (me->m_right.GetCount());
				if (vCount > 0) {
					dgInt32 index = dgInt32 (me->m_right.GetIndex());
					dgFloat32 param = callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), &m_indices[index], vCount);
					dgAssert (param >= dgFloat32 (0.0f));
					if (param < maxParam) {
						maxParam = param;
						if (maxParam == dgFloat32 (0.0f)) {
							break;
						}
					}
				}

			} else {
				const dgNode* const node = me->m_right.GetNode(m_aabb);
				dgFloat32 dist1 = node->RayDistance(ray, vertexArray);
				if (dist1 < maxParam) {
					dgInt32 j = stack;
					for ( ; j && (dist1 > distance[j - 1]); j --) {
						stackPool[j] = stackPool[j - 1];
						distance[j] = distance[j - 1];
					}
					dgAssert (stack < DG_STACK_DEPTH);
					stackPool[j] = node;
					distance[j] = dist1;
					stack++;
				}
			}
		}
	}
}


void dgAABBPolygonSoup::ForAllSectors (const dgFastAABBInfo& obbAabbInfo, const dgVector& boxDistanceTravel, dgFloat32 m_maxT, dgAABBIntersectCallback callback, void* const context) const
{
	dgAssert (dgAbs(dgAbs(obbAabbInfo[0][0]) - obbAabbInfo.m_absDir[0][0]) < dgFloat32 (1.0e-4f));
	dgAssert (dgAbs(dgAbs(obbAabbInfo[1][1]) - obbAabbInfo.m_absDir[1][1]) < dgFloat32 (1.0e-4f));
	dgAssert (dgAbs(dgAbs(obbAabbInfo[2][2]) - obbAabbInfo.m_absDir[2][2]) < dgFloat32 (1.0e-4f));

	dgAssert (dgAbs(dgAbs(obbAabbInfo[0][1]) - obbAabbInfo.m_absDir[1][0]) < dgFloat32 (1.0e-4f));
	dgAssert (dgAbs(dgAbs(obbAabbInfo[0][2]) - obbAabbInfo.m_absDir[2][0]) < dgFloat32 (1.0e-4f));
	dgAssert (dgAbs(dgAbs(obbAabbInfo[1][2]) - obbAabbInfo.m_absDir[2][1]) < dgFloat32 (1.0e-4f));

	if (m_aabb) {
		dgFloat32 distance[DG_STACK_DEPTH];
		const dgNode* stackPool[DG_STACK_DEPTH];

		const dgInt32 stride = sizeof (dgTriplex) / sizeof (dgFloat32);
		const dgTriplex* const vertexArray = (dgTriplex*) m_localVertex;

		dgAssert (boxDistanceTravel.m_w == dgFloat32 (0.0f));
		if (boxDistanceTravel.DotProduct(boxDistanceTravel).GetScalar() < dgFloat32 (1.0e-8f)) {

			dgInt32 stack = 1;
			stackPool[0] = m_aabb;
			distance[0] = m_aabb->BoxPenetration(obbAabbInfo, vertexArray);
			if (distance[0] <= dgFloat32(0.0f)) {
				obbAabbInfo.m_separationDistance = dgMin(obbAabbInfo.m_separationDistance, -distance[0]);
			}
			while (stack) {
				stack --;
				dgFloat32 dist = distance[stack];
				if (dist > dgFloat32 (0.0f)) {
					const dgNode* const me = stackPool[stack];
					if (me->m_left.IsLeaf()) {
						dgInt32 index = dgInt32 (me->m_left.GetIndex());
						dgInt32 vCount = dgInt32 (me->m_left.GetCount());
						if (vCount > 0) {
							const dgInt32* const indices = &m_indices[index];
							dgInt32 normalIndex = indices[vCount + 1];
							dgVector faceNormal (&vertexArray[normalIndex].m_x);
							faceNormal = faceNormal & dgVector::m_triplexMask;
							dgFloat32 dist1 = obbAabbInfo.PolygonBoxDistance (faceNormal, vCount, indices, stride, &vertexArray[0].m_x);
							if (dist1 > dgFloat32 (0.0f)) {
								obbAabbInfo.m_separationDistance = dgFloat32(0.0f);
								dgAssert (vCount >= 3);
								if (callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), indices, vCount, dist1) == t_StopSearh) {
									return;
								}
							} else {
								obbAabbInfo.m_separationDistance = dgMin(obbAabbInfo.m_separationDistance, -dist1);
							}
						}

					} else {
						const dgNode* const node = me->m_left.GetNode(m_aabb);
						dgFloat32 dist1 = node->BoxPenetration(obbAabbInfo, vertexArray);
						if (dist1 > dgFloat32 (0.0f)) {
							dgInt32 j = stack;
							for ( ; j && (dist1 > distance[j - 1]); j --) {
								stackPool[j] = stackPool[j - 1];
								distance[j] = distance[j - 1];
							}
							dgAssert (stack < DG_STACK_DEPTH);
							stackPool[j] = node;
							distance[j] = dist1;
							stack++;
						} else {
							obbAabbInfo.m_separationDistance = dgMin(obbAabbInfo.m_separationDistance, -dist1);
						}
					}

					if (me->m_right.IsLeaf()) {
						dgInt32 index = dgInt32 (me->m_right.GetIndex());
						dgInt32 vCount = dgInt32 (me->m_right.GetCount());
						if (vCount > 0) {
							const dgInt32* const indices = &m_indices[index];
							dgInt32 normalIndex = indices[vCount + 1];
							dgVector faceNormal (&vertexArray[normalIndex].m_x);
							faceNormal = faceNormal & dgVector::m_triplexMask;
							dgFloat32 dist1 = obbAabbInfo.PolygonBoxDistance (faceNormal, vCount, indices, stride, &vertexArray[0].m_x);
							if (dist1 > dgFloat32 (0.0f)) {
								dgAssert (vCount >= 3);
								obbAabbInfo.m_separationDistance = dgFloat32(0.0f);
								if (callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), indices, vCount, dist1) == t_StopSearh) {
									return;
								}
							} else {
								obbAabbInfo.m_separationDistance = dgMin(obbAabbInfo.m_separationDistance, -dist1);
							}
						}

					} else {
						const dgNode* const node = me->m_right.GetNode(m_aabb);
						dgFloat32 dist1 = node->BoxPenetration(obbAabbInfo, vertexArray);
						if (dist1 > dgFloat32 (0.0f)) {
							dgInt32 j = stack;
							for ( ; j && (dist1 > distance[j - 1]); j --) {
								stackPool[j] = stackPool[j - 1];
								distance[j] = distance[j - 1];
							}
							dgAssert (stack < DG_STACK_DEPTH);
							stackPool[j] = node;
							distance[j] = dist1;
							stack++;
						} else {
							obbAabbInfo.m_separationDistance = dgMin(obbAabbInfo.m_separationDistance, -dist1);
						}
					}
				}
			}

		} else {
			dgFastRayTest ray (dgVector (dgFloat32 (0.0f)), boxDistanceTravel);
			dgFastRayTest obbRay (dgVector (dgFloat32 (0.0f)), obbAabbInfo.UnrotateVector(boxDistanceTravel));
			dgInt32 stack = 1;
			stackPool[0] = m_aabb;
			distance [0] = m_aabb->BoxIntersect (ray, obbRay, obbAabbInfo, vertexArray);

			while (stack) {
				stack --;
				const dgFloat32 dist = distance[stack];
				const dgNode* const me = stackPool[stack];
				if (dist < dgFloat32 (1.0f)) {

					if (me->m_left.IsLeaf()) {
						dgInt32 index = dgInt32 (me->m_left.GetIndex());
						dgInt32 vCount = dgInt32 (me->m_left.GetCount());
						if (vCount > 0) {
							const dgInt32* const indices = &m_indices[index];
							dgInt32 normalIndex = indices[vCount + 1];
							dgVector faceNormal (&vertexArray[normalIndex].m_x);
							faceNormal = faceNormal & dgVector::m_triplexMask;
							dgFloat32 hitDistance = obbAabbInfo.PolygonBoxRayDistance (faceNormal, vCount, indices, stride, &vertexArray[0].m_x, ray);
							if (hitDistance < dgFloat32 (1.0f)) {
								dgAssert (vCount >= 3);
								if (callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), indices, vCount, hitDistance) == t_StopSearh) {
									return;
								}
							}
						}

					} else {
						const dgNode* const node = me->m_left.GetNode(m_aabb);
						dgFloat32 dist1 = node->BoxIntersect (ray, obbRay, obbAabbInfo, vertexArray);
						if (dist1 < dgFloat32 (1.0f)) {
							dgInt32 j = stack;
							for ( ; j && (dist1 > distance[j - 1]); j --) {
								stackPool[j] = stackPool[j - 1];
								distance[j] = distance[j - 1];
							}
							dgAssert (stack < DG_STACK_DEPTH);
							stackPool[j] = node;
							distance[j] = dist1;
							stack++;
						}
					}

					if (me->m_right.IsLeaf()) {
						dgInt32 index = dgInt32 (me->m_right.GetIndex());
						dgInt32 vCount = dgInt32 (me->m_right.GetCount());
						if (vCount > 0) {
							const dgInt32* const indices = &m_indices[index];
							dgInt32 normalIndex = indices[vCount + 1];
							dgVector faceNormal (&vertexArray[normalIndex].m_x);
							faceNormal = faceNormal & dgVector::m_triplexMask;
							dgFloat32 hitDistance = obbAabbInfo.PolygonBoxRayDistance (faceNormal, vCount, indices, stride, &vertexArray[0].m_x, ray);
							if (hitDistance < dgFloat32 (1.0f)) {
								dgAssert (vCount >= 3);
								if (callback(context, &vertexArray[0].m_x, sizeof (dgTriplex), indices, vCount, hitDistance) == t_StopSearh) {
									return;
								}
							}
						}

					} else {
						const dgNode* const node = me->m_right.GetNode(m_aabb);
						dgFloat32 dist1 = node->BoxIntersect (ray, obbRay, obbAabbInfo, vertexArray);
						if (dist1 < dgFloat32 (1.0f)) {
							dgInt32 j = stack;
							for ( ; j && (dist1 > distance[j - 1]); j --) {
								stackPool[j] = stackPool[j - 1];
								distance[j] = distance[j - 1];
							}
							dgAssert (stack < DG_STACK_DEPTH);
							stackPool[j] = node;
							distance[j] = dist1;
							stack ++;
						}
					}
				}
			}
		}
	}
}


