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
#include "dgTree.h"
#include "dgStack.h"
#include "dgGoogol.h"
#include "dgConvexHull3d.h"
#include "dgSmallDeterminant.h"


#define DG_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE		8

#ifdef	DG_OLD_CONVEXHULL_3D
class dgConvexHull3d::dgNormalMap
{
	public:
	dgNormalMap()
		:m_count(sizeof(m_normal) / sizeof(m_normal[0]))
	{
		dgVector p0(dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
		dgVector p1(dgFloat32(-1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
		dgVector p2(dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f));
		dgVector p3(dgFloat32(0.0f), dgFloat32(-1.0f), dgFloat32(0.0f), dgFloat32(0.0f));
		dgVector p4(dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f));
		dgVector p5(dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(-1.0f), dgFloat32(0.0f));

		dgInt32 count = 0;
		dgInt32 subdivitions = 2;
		TessellateTriangle(subdivitions, p4, p0, p2, count);
		TessellateTriangle(subdivitions, p0, p5, p2, count);
		TessellateTriangle(subdivitions, p5, p1, p2, count);
		TessellateTriangle(subdivitions, p1, p4, p2, count);
		TessellateTriangle(subdivitions, p0, p4, p3, count);
		TessellateTriangle(subdivitions, p5, p0, p3, count);
		TessellateTriangle(subdivitions, p1, p5, p3, count);
		TessellateTriangle(subdivitions, p4, p1, p3, count);
	}

	static const dgNormalMap& GetNormaMap()
	{
		static dgNormalMap normalMap;
		return normalMap;
	}

	void TessellateTriangle(dgInt32 level, const dgVector& p0, const dgVector& p1, const dgVector& p2, dgInt32& count)
	{
		if (level) {
			dgAssert(dgAbs(p0.DotProduct3(p0) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
			dgAssert(dgAbs(p1.DotProduct3(p1) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
			dgAssert(dgAbs(p2.DotProduct3(p2) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
			dgVector p01(p0 + p1);
			dgVector p12(p1 + p2);
			dgVector p20(p2 + p0);

			p01 = p01.Scale(dgRsqrt(p01.DotProduct3(p01)));
			p12 = p12.Scale(dgRsqrt(p12.DotProduct3(p12)));
			p20 = p20.Scale(dgRsqrt(p20.DotProduct3(p20)));

			dgAssert(dgAbs(p01.DotProduct3(p01) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
			dgAssert(dgAbs(p12.DotProduct3(p12) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
			dgAssert(dgAbs(p20.DotProduct3(p20) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));

			TessellateTriangle(level - 1, p0, p01, p20, count);
			TessellateTriangle(level - 1, p1, p12, p01, count);
			TessellateTriangle(level - 1, p2, p20, p12, count);
			TessellateTriangle(level - 1, p01, p12, p20, count);
		} else {
			dgBigPlane n(p0, p1, p2);
			n = n.Scale(dgFloat64(1.0f) / sqrt(n.DotProduct3(n)));
			n.m_w = dgFloat64(0.0f);
			dgInt32 index = dgBitReversal(count, sizeof(m_normal) / sizeof(m_normal[0]));
			m_normal[index] = n;
			count++;
			dgAssert(count <= sizeof(m_normal) / sizeof(m_normal[0]));
		}
	}

	dgBigVector m_normal[128];
	dgInt32 m_count;
};
#endif

class dgConvexHull3DVertex: public dgBigVector
{
	public:
	dgInt32 m_mark;
};

class dgConvexHull3dAABBTreeNode
{
	public:
	dgBigVector m_box[2];
	dgConvexHull3dAABBTreeNode* m_left;
	dgConvexHull3dAABBTreeNode* m_right;
	dgConvexHull3dAABBTreeNode* m_parent;
};

class dgConvexHull3dPointCluster: public dgConvexHull3dAABBTreeNode
{
	public:
	dgInt32 m_count;
	dgInt32 m_indices[DG_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE];
};


dgConvexHull3DFace::dgConvexHull3DFace()
{
	m_mark = 0;
	m_twin[0] = NULL;
	m_twin[1] = NULL;
	m_twin[2] = NULL;
}

dgFloat64 dgConvexHull3DFace::Evalue (const dgBigVector* const pointArray, const dgBigVector& point) const
{
	const dgBigVector& p0 = pointArray[m_index[0]];
	const dgBigVector& p1 = pointArray[m_index[1]];
	const dgBigVector& p2 = pointArray[m_index[2]];

	dgFloat64 matrix[3][3];
	for (dgInt32 i = 0; i < 3; i ++) {
		matrix[0][i] = p2[i] - p0[i];
		matrix[1][i] = p1[i] - p0[i];
		matrix[2][i] = point[i] - p0[i];
	}

	dgFloat64 error;
	dgFloat64 det = Determinant3x3 (matrix, &error);

	// the code use double, however the threshold for accuracy test is the machine precision of a float.
	// by changing this to a smaller number, the code should run faster since many small test will be considered valid
	// the precision must be a power of two no smaller than the machine precision of a double, (1<<48)
	// float64(1<<30) can be a good value

	// dgFloat64 precision	= dgFloat64 (1.0f) / dgFloat64 (1<<30);
	dgFloat64 precision	 = dgFloat64 (1.0f) / dgFloat64 (1<<24);
	dgFloat64 errbound = error * precision;
	if (fabs(det) > errbound) {
		return det;
	}

	dgGoogol exactMatrix[3][3];
	for (dgInt32 i = 0; i < 3; i ++) {
		exactMatrix[0][i] = dgGoogol(p2[i]) - dgGoogol(p0[i]);
		exactMatrix[1][i] = dgGoogol(p1[i]) - dgGoogol(p0[i]);
		exactMatrix[2][i] = dgGoogol(point[i]) - dgGoogol(p0[i]);
	}
	return Determinant3x3(exactMatrix);
}

dgBigPlane dgConvexHull3DFace::GetPlaneEquation (const dgBigVector* const pointArray) const
{
	const dgBigVector& p0 = pointArray[m_index[0]];
	const dgBigVector& p1 = pointArray[m_index[1]];
	const dgBigVector& p2 = pointArray[m_index[2]];
	dgBigPlane plane (p0, p1, p2);
	plane = plane.Scale (1.0f / sqrt (plane.DotProduct3(plane)));
	return plane;
}


dgConvexHull3d::dgConvexHull3d (dgMemoryAllocator* const allocator)
	:dgList<dgConvexHull3DFace>(allocator)
	,m_count (0)
	,m_diag()
	,m_aabbP0(dgBigVector (dgFloat64 (0.0f)))
	,m_aabbP1(dgBigVector (dgFloat64 (0.0f)))
	,m_points(allocator)
{
}

dgConvexHull3d::dgConvexHull3d(const dgConvexHull3d& source)
	:dgList<dgConvexHull3DFace>(source.GetAllocator())
	,m_count (source.m_count)
	,m_diag(source.m_diag)
	,m_aabbP0 (source.m_aabbP0)
	,m_aabbP1 (source.m_aabbP1)
	,m_points(source.GetAllocator(), source.m_count)
{
	m_points[m_count-1].m_w = dgFloat64 (0.0f);
	for (int i = 0; i < m_count; i ++) {
		m_points[i] = source.m_points[i];
	}
	dgTree<dgListNode*, dgListNode*> map (GetAllocator());
	for(dgListNode* sourceNode = source.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext() ) {
		dgListNode* const node = Append();
		map.Insert(node, sourceNode);
	}

	for(dgListNode* sourceNode = source.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext() ) {
		dgListNode* const node = map.Find(sourceNode)->GetInfo();

		dgConvexHull3DFace& face = node->GetInfo();
		dgConvexHull3DFace& srcFace = sourceNode->GetInfo();

		face.m_mark = 0;
		for (dgInt32 i = 0; i < 3; i ++) {
			face.m_index[i] = srcFace.m_index[i];
			face.m_twin[i] = map.Find (srcFace.m_twin[i])->GetInfo();
		}
	}
}

dgConvexHull3d::dgConvexHull3d(dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount)
	:dgList<dgConvexHull3DFace>(allocator)
	,m_count (0)
	,m_diag()
	,m_aabbP0 (dgBigVector (dgFloat64 (0.0), dgFloat64 (0.0), dgFloat64 (0.0), dgFloat64 (0.0)))
	,m_aabbP1 (dgBigVector (dgFloat64 (0.0), dgFloat64 (0.0), dgFloat64 (0.0), dgFloat64 (0.0)))
	,m_points(allocator)
{
	BuildHull (vertexCloud, strideInBytes, count, distTol, maxVertexCount);
}

dgConvexHull3d::~dgConvexHull3d(void)
{
}


void dgConvexHull3d::BuildHull (const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount)
{
	dgSetPrecisionDouble precision;

	dgInt32 treeCount = count / (DG_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE>>1);
	if (treeCount < 4) {
		treeCount = 4;
	}
	treeCount *= 2;

	dgStack<dgConvexHull3DVertex> points (count);
	dgStack<dgConvexHull3dPointCluster> treePool (treeCount + 256);
	count = InitVertexArray(&points[0], vertexCloud, strideInBytes, count, &treePool[0], treePool.GetSizeInBytes());

#ifdef	DG_OLD_CONVEXHULL_3D
	if (m_count >= 4) {
		CalculateConvexHull3d (&treePool[0], &points[0], count, distTol, maxVertexCount);
	}
#else
	if (m_count >= 3) {
		if (CheckFlatSurface(&treePool[0], &points[0], count, distTol, maxVertexCount)) {
			CalculateConvexHull2d(&treePool[0], &points[0], count, distTol, maxVertexCount);
		} else {
			dgAssert(m_count == 4);
			CalculateConvexHull3d(&treePool[0], &points[0], count, distTol, maxVertexCount);
		}
	}
#endif
}

dgInt32 dgConvexHull3d::ConvexCompareVertex(const dgConvexHull3DVertex* const  A, const dgConvexHull3DVertex* const B, void* const context)
{
	for (dgInt32 i = 0; i < 3; i ++) {
		if ((*A)[i] < (*B)[i]) {
			return -1;
		} else if ((*A)[i] > (*B)[i]) {
			return 1;
		}
	}
	return 0;
}

dgConvexHull3dAABBTreeNode* dgConvexHull3d::BuildTree (dgConvexHull3dAABBTreeNode* const parent, dgConvexHull3DVertex* const points, dgInt32 count, dgInt32 baseIndex, dgInt8** memoryPool, dgInt32& maxMemSize) const
{
	dgConvexHull3dAABBTreeNode* tree = NULL;

	dgAssert (count);
	dgBigVector minP ( dgFloat32 (1.0e15f));
	dgBigVector maxP (-dgFloat32 (1.0e15f));
	if (count <= DG_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE) {

		dgConvexHull3dPointCluster* const clump = new (*memoryPool) dgConvexHull3dPointCluster;
		*memoryPool += sizeof (dgConvexHull3dPointCluster);
		maxMemSize -= sizeof (dgConvexHull3dPointCluster);
		dgAssert (maxMemSize >= 0);

		dgAssert (clump);
		clump->m_count = count;
		for (dgInt32 i = 0; i < count; i ++) {
			clump->m_indices[i] = i + baseIndex;

			const dgBigVector& p = points[i];
			dgAssert(p.m_w == dgFloat32(0.0f));
			minP = minP.GetMin(p);
			maxP = maxP.GetMax(p);
		}

		clump->m_left = NULL;
		clump->m_right = NULL;
		tree = clump;

	} else {
		dgBigVector median (dgFloat32 (0.0f));
		dgBigVector varian (dgFloat32 (0.0f));
		for (dgInt32 i = 0; i < count; i ++) {

			const dgBigVector& p = points[i];
			dgAssert(p.m_w == dgFloat32(0.0f));
			minP = minP.GetMin(p);
			maxP = maxP.GetMax(p);
			median += p;
			varian += p * p;
		}

		varian = varian.Scale(dgFloat32(count)) - median * median;
		dgInt32 index = 0;
		dgFloat64 maxVarian = dgFloat64 (-1.0e10f);
		for (dgInt32 i = 0; i < 3; i ++) {
			if (varian[i] > maxVarian) {
				index = i;
				maxVarian = varian[i];
			}
		}
		dgBigVector center (median.Scale (dgFloat64 (1.0f) / dgFloat64 (count)));

		dgFloat64 test = center[index];

		dgInt32 i0 = 0;
		dgInt32 i1 = count - 1;
		do {
			for (; i0 <= i1; i0 ++) {
				dgFloat64 val = points[i0][index];
				if (val > test) {
					break;
				}
			}

			for (; i1 >= i0; i1 --) {
				dgFloat64 val = points[i1][index];
				if (val < test) {
					break;
				}
			}

			if (i0 < i1)	{
				dgSwap(points[i0], points[i1]);
				i0++;
				i1--;
			}
		} while (i0 <= i1);

		if (i0 == 0){
			i0 = count / 2;
		}
		if (i0 >= (count - 1)){
			i0 = count / 2;
		}

		tree = new (*memoryPool) dgConvexHull3dAABBTreeNode;
		*memoryPool += sizeof (dgConvexHull3dAABBTreeNode);
		maxMemSize -= sizeof (dgConvexHull3dAABBTreeNode);
		dgAssert (maxMemSize >= 0);

		dgAssert (i0);
		dgAssert (count - i0);

		tree->m_left = BuildTree (tree, points, i0, baseIndex, memoryPool, maxMemSize);
		tree->m_right = BuildTree (tree, &points[i0], count - i0, i0 + baseIndex, memoryPool, maxMemSize);
	}

	dgAssert (tree);
	tree->m_parent = parent;
	tree->m_box[0] = minP - dgBigVector (dgFloat64 (1.0e-3f));
	tree->m_box[1] = maxP + dgBigVector (dgFloat64 (1.0e-3f));
	return tree;
}

dgInt32 dgConvexHull3d::GetUniquePoints(dgConvexHull3DVertex* const points, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, void* const memoryPool, dgInt32 maxMemSize)
{
	dgInt32 stride = dgInt32(strideInBytes / sizeof(dgFloat64));
	if (stride >= 4) {
		for (dgInt32 i = 0; i < count; i++) {
			dgInt32 index = i * stride;
			dgBigVector& vertex = points[i];
			vertex = dgBigVector(vertexCloud[index], vertexCloud[index + 1], vertexCloud[index + 2], vertexCloud[index + 3]);
			dgAssert(dgCheckVector(vertex));
			points[i].m_mark = 0;
		}
	} else {
		for (dgInt32 i = 0; i < count; i++) {
			dgInt32 index = i * stride;
			dgBigVector& vertex = points[i];
			vertex = dgBigVector(vertexCloud[index], vertexCloud[index + 1], vertexCloud[index + 2], dgFloat64(0.0f));
			dgAssert(dgCheckVector(vertex));
			points[i].m_mark = 0;
		}
	}

	dgSort(points, count, ConvexCompareVertex);

	dgInt32 indexCount = 0;
	for (int i = 1; i < count; i++) {
		for (; i < count; i++) {
			if (ConvexCompareVertex(&points[indexCount], &points[i], NULL)) {
				indexCount++;
				points[indexCount] = points[i];
				break;
			}
		}
	}
	count = indexCount + 1;
	return count;
}

dgInt32 dgConvexHull3d::InitVertexArray(dgConvexHull3DVertex* const points, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, void* const memoryPool, dgInt32 maxMemSize)
{
	count = GetUniquePoints(points, vertexCloud, strideInBytes, count, memoryPool, maxMemSize);
	if (count < 4) {
		m_count = 0;
		return count;
	}
	dgConvexHull3dAABBTreeNode* tree = BuildTree (NULL, points, count, 0, (dgInt8**) &memoryPool, maxMemSize);

	m_aabbP0 = tree->m_box[0];
	m_aabbP1 = tree->m_box[1];

	dgBigVector boxSize (tree->m_box[1] - tree->m_box[0]);
	m_diag = dgFloat32 (sqrt (boxSize.DotProduct3(boxSize)));

#ifdef DG_OLD_CONVEXHULL_3D
	const dgNormalMap& normalMap = dgNormalMap::GetNormaMap();

	dgInt32 index0 = SupportVertex (&tree, points, normalMap.m_normal[0]);
	m_points[0] = points[index0];
	points[index0].m_mark = 1;

	bool validTetrahedrum = false;
	dgBigVector e1 (dgFloat64 (0.0f), dgFloat64 (0.0f), dgFloat64 (0.0f), dgFloat64 (0.0f)) ;
	for (dgInt32 i = 1; i < normalMap.m_count; i ++) {
		dgInt32 index = SupportVertex (&tree, points, normalMap.m_normal[i]);
		dgAssert (index >= 0);

		e1 = points[index] - m_points[0];
		dgFloat64 error2 = e1.DotProduct3(e1);
		if (error2 > (dgFloat32 (1.0e-4f) * m_diag * m_diag)) {
			m_points[1] = points[index];
			points[index].m_mark = 1;
			validTetrahedrum = true;
			break;
		}
	}
	if (!validTetrahedrum) {
		m_count = 0;
		dgAssert (0);
		return count;
	}

	validTetrahedrum = false;
	dgBigVector e2(dgFloat32 (0.0f));
	dgBigVector normal (dgFloat32 (0.0f));
	for (dgInt32 i = 2; i < normalMap.m_count; i ++) {
		dgInt32 index = SupportVertex (&tree, points, normalMap.m_normal[i]);
		dgAssert (index >= 0);
		e2 = points[index] - m_points[0];
		normal = e1.CrossProduct(e2);
		dgFloat64 error2 = sqrt (normal.DotProduct3(normal));
		if (error2 > (dgFloat32 (1.0e-4f) * m_diag * m_diag)) {
			m_points[2] = points[index];
			points[index].m_mark = 1;
			validTetrahedrum = true;
			break;
		}
	}

	dgAssert(normal.m_w == dgFloat32(0.0f));
	if (!validTetrahedrum) {
		m_count = 0;
		dgAssert (0);
		return count;
	}

	// find the largest possible tetrahedron
	validTetrahedrum = false;
	dgBigVector e3(dgFloat32 (0.0f));

	index0 = SupportVertex (&tree, points, normal);
	e3 = points[index0] - m_points[0];
	dgFloat64 err2 = normal.DotProduct3(e3);
	if (fabs (err2) > (dgFloat64 (1.0e-6f) * m_diag * m_diag)) {
		// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
		m_points[3] = points[index0];
		points[index0].m_mark = 1;
		validTetrahedrum = true;
	}
	if (!validTetrahedrum) {
		dgVector n (normal.Scale(dgFloat64 (-1.0f)));
		dgInt32 index = SupportVertex (&tree, points, n);
		e3 = points[index] - m_points[0];
		dgFloat64 error2 = normal.DotProduct3(e3);
		if (fabs (error2) > (dgFloat64 (1.0e-6f) * m_diag * m_diag)) {
			// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
			m_points[3] = points[index];
			points[index].m_mark = 1;
			validTetrahedrum = true;
		}
	}
	if (!validTetrahedrum) {
		for (dgInt32 i = 3; i < normalMap.m_count; i ++) {
			dgInt32 index = SupportVertex (&tree, points, normalMap.m_normal[i]);
			dgAssert (index >= 0);

			//make sure the volume of the fist tetrahedral is no negative
			e3 = points[index] - m_points[0];
			dgFloat64 error2 = normal.DotProduct3(e3);
			if (fabs (error2) > (dgFloat64 (1.0e-6f) * m_diag * m_diag)) {
				// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
				m_points[3] = points[index];
				points[index].m_mark = 1;
				validTetrahedrum = true;
				break;
			}
		}
	}
	if (!validTetrahedrum) {
		// the points do not form a convex hull
		m_count = 0;
		//dgAssert (0);
		return count;
	}


	m_count = 4;
	dgFloat64 volume = TetrahedrumVolume (m_points[0], m_points[1], m_points[2], m_points[3]);
	if (volume > dgFloat64 (0.0f)) {
		dgSwap(m_points[2], m_points[3]);
	}
	dgAssert (TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]) < dgFloat64(0.0f));

	return count;
#else
	
	dgBigVector origin((m_aabbP1 + m_aabbP0).Scale (0.5f));

	dgBigVector dir(m_aabbP1 - m_aabbP0);
	dgAssert(dir.DotProduct3(dir) > dgFloat32(1.0e-4f));
	dir = dir.Normalize();
	dgInt32 index0 = SupportVertex(&tree, points, dir);
	m_points[0] = points[index0];
	points[index0].m_mark = 1;

	dir = origin - m_points[0];
	dgAssert(dir.DotProduct3(dir) > dgFloat32(1.0e-4f));
	dir = dir.Normalize();
	dgInt32 index1 = SupportVertex(&tree, points, dir);
	m_points[1] = points[index1];
	points[index1].m_mark = 1;

	dgBigVector e0(m_points[1] - m_points[0]);
	dgAssert(e0.DotProduct3(e0) > dgFloat32(1.0e-4f));
	dgFloat64 t = -e0.DotProduct3(origin - m_points[0]) / e0.DotProduct3(e0);
	dir = m_points[0] + e0.Scale(t) - origin;

	dgAssert(dir.DotProduct3(dir) > dgFloat32(1.0e-4f));
	dir = dir.Normalize();
	dgInt32 index2 = SupportVertex(&tree, points, dir);
	m_points[2] = points[index2];
	points[index2].m_mark = 1;

	dgBigVector e1 (m_points[2] - m_points[0]);
	dgBigVector normal (e1.CrossProduct(e0));
	dgFloat64 error2 = sqrt(normal.DotProduct3(normal));
	if (error2 < (dgFloat32(1.0e-4f) * m_diag * m_diag)) {
		dgAssert(0);
//		m_points[2] = points[index];
//		points[index].m_mark = 1;
//		validTetrahedrum = true;
//		break;
	}

	m_count = 3;
	return count;
#endif
}

dgFloat64 dgConvexHull3d::TetrahedrumVolume (const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3) const
{
	dgBigVector p1p0 (p1 - p0);
	dgBigVector p2p0 (p2 - p0);
	dgBigVector p3p0 (p3 - p0);
	return p3p0.DotProduct3(p1p0.CrossProduct(p2p0));
}

dgInt32 dgConvexHull3d::SupportVertex (dgConvexHull3dAABBTreeNode** const treePointer, const dgConvexHull3DVertex* const points, const dgBigVector& dir, const bool removeEntry) const
{
	#define DG_STACK_DEPTH_3D 64
	dgFloat64 aabbProjection[DG_STACK_DEPTH_3D];
	const dgConvexHull3dAABBTreeNode *stackPool[DG_STACK_DEPTH_3D];

	dgInt32 index = -1;
	dgInt32 stack = 1;
	stackPool[0] = *treePointer;
	aabbProjection[0] = dgFloat32 (1.0e20f);
	dgFloat64 maxProj = dgFloat64 (-1.0e20f);
	dgInt32 ix = (dir[0] > dgFloat64 (0.0f)) ? 1 : 0;
	dgInt32 iy = (dir[1] > dgFloat64 (0.0f)) ? 1 : 0;
	dgInt32 iz = (dir[2] > dgFloat64 (0.0f)) ? 1 : 0;
	while (stack) {
		stack--;
		dgFloat64 boxSupportValue = aabbProjection[stack];
		if (boxSupportValue > maxProj) {
			const dgConvexHull3dAABBTreeNode* const me = stackPool[stack];

			if (me->m_left && me->m_right) {
				dgBigVector leftSupportPoint (me->m_left->m_box[ix].m_x, me->m_left->m_box[iy].m_y, me->m_left->m_box[iz].m_z, dgFloat32 (0.0));
				dgFloat64 leftSupportDist = leftSupportPoint.DotProduct3(dir);

				dgBigVector rightSupportPoint (me->m_right->m_box[ix].m_x, me->m_right->m_box[iy].m_y, me->m_right->m_box[iz].m_z, dgFloat32 (0.0));
				dgFloat64 rightSupportDist = rightSupportPoint.DotProduct3(dir);


				if (rightSupportDist >= leftSupportDist) {
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_3D);
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_3D);
				} else {
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_3D);
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_3D);
				}

			} else {
				dgConvexHull3dPointCluster* const cluster = (dgConvexHull3dPointCluster*) me;
				for (dgInt32 i = 0; i < cluster->m_count; i ++) {
					const dgConvexHull3DVertex& p = points[cluster->m_indices[i]];
					dgAssert (p.m_x >= cluster->m_box[0].m_x);
					dgAssert (p.m_x <= cluster->m_box[1].m_x);
					dgAssert (p.m_y >= cluster->m_box[0].m_y);
					dgAssert (p.m_y <= cluster->m_box[1].m_y);
					dgAssert (p.m_z >= cluster->m_box[0].m_z);
					dgAssert (p.m_z <= cluster->m_box[1].m_z);
					if (!p.m_mark) {
						dgFloat64 dist = p.DotProduct3(dir);
						if (dist > maxProj) {
							maxProj = dist;
							index = cluster->m_indices[i];
						}
					} else if (removeEntry) {
						cluster->m_indices[i] = cluster->m_indices[cluster->m_count - 1];
						cluster->m_count = cluster->m_count - 1;
						i --;
					}
				}

				if (cluster->m_count == 0) {
					dgConvexHull3dAABBTreeNode* const parent = cluster->m_parent;
					if (parent) {
						dgConvexHull3dAABBTreeNode* const sibling = (parent->m_left != cluster) ? parent->m_left : parent->m_right;
						dgAssert (sibling != cluster);
						dgConvexHull3dAABBTreeNode* const grandParent = parent->m_parent;
						if (grandParent) {
							sibling->m_parent = grandParent;
							if (grandParent->m_right == parent) {
								grandParent->m_right = sibling;
							} else {
								grandParent->m_left = sibling;
							}
						} else {
							sibling->m_parent = NULL;
							*treePointer = sibling;
						}
					}
				}
			}
		}
	}

	dgAssert (index != -1);
	return index;
}

dgConvexHull3d::dgListNode* dgConvexHull3d::AddFace (dgInt32 i0, dgInt32 i1, dgInt32 i2)
{
	dgListNode* const node = Append();
	dgConvexHull3DFace& face = node->GetInfo();

	face.m_index[0] = i0;
	face.m_index[1] = i1;
	face.m_index[2] = i2;
	return node;
}

void dgConvexHull3d::DeleteFace (dgListNode* const node)
{
	Remove (node);
}

bool dgConvexHull3d::Sanity() const
{
/*
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		dgConvexHull3DFace* const face = &node->GetInfo();
		for (dgInt32 i = 0; i < 3; i ++) {
			dgListNode* const twinNode = face->m_twin[i];
			if (!twinNode) {
				return false;
			}

			dgInt32 count = 0;
			dgListNode* me = NULL;
			dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
			for (dgInt32 j = 0; j < 3; j ++) {
				if (twinFace->m_twin[j] == node) {
					count ++;
					me = twinFace->m_twin[j];
				}
			}
			if (count != 1) {
				return false;
			}
			if (me != node) {
				return false;
			}
		}
	}
*/
	return true;
}

bool dgConvexHull3d::CheckFlatSurface(dgConvexHull3dAABBTreeNode* tree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount)
{
	dgBigVector e0(m_points[1] - m_points[0]);
	dgBigVector e1(m_points[2] - m_points[0]);
	dgAssert(e0.DotProduct3(e0) > dgFloat32(1.0e-4f));
	dgAssert(e1.DotProduct3(e1) > dgFloat32(1.0e-4f));
	dgBigVector normal(e1.CrossProduct(e0));
	dgAssert(normal.m_w == dgFloat32(0.0f));
	dgAssert(normal.DotProduct3(normal) > dgFloat32(1.0e-6f));
	normal = normal.Normalize();

	dgInt32 index = SupportVertex(&tree, points, normal);
	m_points[3] = points[index];

	dgFloat64 volume = TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]);
	if (dgAbs(volume) < dgFloat32(1.0e-9f)) {
		normal = normal.Scale(dgFloat32(-1.0f));
		index = SupportVertex(&tree, points, normal);
		m_points[3] = points[index];
		volume = TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]);
		if (dgAbs(volume) < dgFloat32(1.0e-9f)) {
			return true;
		}
	}
	points[index].m_mark = 1;
	if (volume > dgFloat64(0.0f)) {
		dgSwap(m_points[2], m_points[3]);
	}
	dgAssert(TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]) < dgFloat64(0.0f));
	m_count = 4;
	return false;
}


void dgConvexHull3d::CalculateConvexHull2d(dgConvexHull3dAABBTreeNode* tree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount)
{

}

void dgConvexHull3d::CalculateConvexHull3d (dgConvexHull3dAABBTreeNode* vertexTree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount)
{
	distTol = dgAbs (distTol) * m_diag;
	dgListNode* const f0Node = AddFace (0, 1, 2);
	dgListNode* const f1Node = AddFace (0, 2, 3);
	dgListNode* const f2Node = AddFace (2, 1, 3);
	dgListNode* const f3Node = AddFace (1, 0, 3);

	dgConvexHull3DFace* const f0 = &f0Node->GetInfo();
	dgConvexHull3DFace* const f1 = &f1Node->GetInfo();
	dgConvexHull3DFace* const f2 = &f2Node->GetInfo();
	dgConvexHull3DFace* const f3 = &f3Node->GetInfo();

	f0->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node;
	f0->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node;
	f0->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node;

	f1->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node;
	f1->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node;
	f1->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node;

	f2->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node;
	f2->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node;
	f2->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node;

	f3->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node;
	f3->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node;
	f3->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node;

	dgList<dgListNode*> boundaryFaces(GetAllocator());

	boundaryFaces.Append(f0Node);
	boundaryFaces.Append(f1Node);
	boundaryFaces.Append(f2Node);
	boundaryFaces.Append(f3Node);
	count -= 4;
	maxVertexCount -= 4;
	dgInt32 currentIndex = 4;


	dgStack<dgListNode*> stackPool(1024 + m_count);
	dgStack<dgListNode*> coneListPool(1024 + m_count);
	dgStack<dgListNode*> deleteListPool(1024 + m_count);

	dgListNode** const stack = &stackPool[0];
	dgListNode** const coneList = &stackPool[0];
	dgListNode** const deleteList = &deleteListPool[0];

	while (boundaryFaces.GetCount() && count && (maxVertexCount > 0)) {
		// my definition of the optimal convex hull of a given vertex count,
		// is the convex hull formed by a subset of the input vertex that minimizes the volume difference
		// between the perfect hull formed from all input vertex and the hull of the sub set of vertex.
		// When using a priority heap this algorithms will generate the an optimal of a fix vertex count.
		// Since all Newton's tools do not have a limit on the point count of a convex hull, I can use either a stack or a queue.
		// a stack maximize construction speed, a Queue tend to maximize the volume of the generated Hull approaching a perfect Hull.
		// For now we use a queue.
		// For general hulls it does not make a difference if we use a stack, queue, or a priority heap.
		// perfect optimal hull only apply for when build hull of a limited vertex count.
		//
		// Also when building Hulls of a limited vertex count, this function runs in constant time.
		// yes that is correct, it does not makes a difference if you build a N point hull from 100 vertex
		// or from 100000 vertex input array.

		#if 0
			// using stack (faster)
			dgListNode* const faceNode = boundaryFaces.GetFirst()->GetInfo();
		#else
			// using a queue (some what slower by better hull when reduced vertex count is desired)
			dgListNode* const faceNode = boundaryFaces.GetLast()->GetInfo();
		#endif

		dgConvexHull3DFace* const face = &faceNode->GetInfo();
		dgBigPlane planeEquation (face->GetPlaneEquation (&m_points[0]));

		dgInt32 index = SupportVertex (&vertexTree, points, planeEquation);
		const dgBigVector& p = points[index];
		dgFloat64 dist = planeEquation.Evalue(p);

		if ((dist >= distTol) && (face->Evalue(&m_points[0], p) > dgFloat64(0.0f))) {
			dgAssert (Sanity());

			dgAssert (faceNode);
			stack[0] = faceNode;

			dgInt32 stackIndex = 1;
			dgInt32 deletedCount = 0;

			while (stackIndex) {
				stackIndex --;
				dgListNode* const node1 = stack[stackIndex];
				dgConvexHull3DFace* const face1 = &node1->GetInfo();

				if (!face1->m_mark && (face1->Evalue(&m_points[0], p) > dgFloat64(0.0f))) {
					#ifdef _DEBUG
					for (dgInt32 i = 0; i < deletedCount; i ++) {
						dgAssert (deleteList[i] != node1);
					}
					#endif

					deleteList[deletedCount] = node1;
					deletedCount ++;
					dgAssert (deletedCount < dgInt32 (deleteListPool.GetElementsCount()));
					face1->m_mark = 1;
					for (dgInt32 i = 0; i < 3; i ++) {
						dgListNode* const twinNode = (dgListNode*)face1->m_twin[i];
						dgAssert (twinNode);
						dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
						if (!twinFace->m_mark) {
							stack[stackIndex] = twinNode;
							stackIndex ++;
							dgAssert (stackIndex < dgInt32 (stackPool.GetElementsCount()));
						}
					}
				}
			}

			m_points[currentIndex] = points[index];
			points[index].m_mark = 1;

			dgInt32 newCount = 0;
			for (dgInt32 i = 0; i < deletedCount; i ++) {
				dgListNode* const node1 = deleteList[i];
				dgConvexHull3DFace* const face1 = &node1->GetInfo();
				dgAssert (face1->m_mark == 1);
				for (dgInt32 j0 = 0; j0 < 3; j0 ++) {
					dgListNode* const twinNode = face1->m_twin[j0];
					dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
					if (!twinFace->m_mark) {
						dgInt32 j1 = (j0 == 2) ? 0 : j0 + 1;
						dgListNode* const newNode = AddFace (currentIndex, face1->m_index[j0], face1->m_index[j1]);
						boundaryFaces.Addtop(newNode);

						dgConvexHull3DFace* const newFace = &newNode->GetInfo();
						newFace->m_twin[1] = twinNode;
						for (dgInt32 k = 0; k < 3; k ++) {
							if (twinFace->m_twin[k] == node1) {
								twinFace->m_twin[k] = newNode;
							}
						}
						coneList[newCount] = newNode;
						newCount ++;
						dgAssert (newCount < dgInt32 (coneListPool.GetElementsCount()));
					}
				}
			}

			for (dgInt32 i = 0; i < newCount - 1; i ++) {
				dgListNode* const nodeA = coneList[i];
				dgConvexHull3DFace* const faceA = &nodeA->GetInfo();
				dgAssert (faceA->m_mark == 0);
				for (dgInt32 j = i + 1; j < newCount; j ++) {
					dgListNode* const nodeB = coneList[j];
					dgConvexHull3DFace* const faceB = &nodeB->GetInfo();
					dgAssert (faceB->m_mark == 0);
					if (faceA->m_index[2] == faceB->m_index[1]) {
						faceA->m_twin[2] = nodeB;
						faceB->m_twin[0] = nodeA;
						break;
					}
				}

				for (dgInt32 j = i + 1; j < newCount; j ++) {
					dgListNode* const nodeB = coneList[j];
					dgConvexHull3DFace* const faceB = &nodeB->GetInfo();
					dgAssert (faceB->m_mark == 0);
					if (faceA->m_index[1] == faceB->m_index[2]) {
						faceA->m_twin[0] = nodeB;
						faceB->m_twin[2] = nodeA;
						break;
					}
				}
			}

			for (dgInt32 i = 0; i < deletedCount; i ++) {
				dgListNode* const node = deleteList[i];
				boundaryFaces.Remove (node);
				DeleteFace (node);
			}

			maxVertexCount --;
			currentIndex ++;
			count --;
		} else {
			boundaryFaces.Remove (faceNode);
		}
	}
	m_count = currentIndex;
}


void dgConvexHull3d::CalculateVolumeAndSurfaceArea (dgFloat64& volume, dgFloat64& surfaceArea) const
{
	dgFloat64 areaAcc = dgFloat32 (0.0f);
	dgFloat64  volumeAcc = dgFloat32 (0.0f);
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dgConvexHull3DFace* const face = &node->GetInfo();
		dgInt32 i0 = face->m_index[0];
		dgInt32 i1 = face->m_index[1];
		dgInt32 i2 = face->m_index[2];
		const dgBigVector& p0 = m_points[i0];
		const dgBigVector& p1 = m_points[i1];
		const dgBigVector& p2 = m_points[i2];
		dgBigVector normal ((p1 - p0).CrossProduct(p2 - p0));
		dgFloat64 area = sqrt (normal.DotProduct3(normal));
		areaAcc += area;
		//volumeAcc += (p0 * p1) % p2;
		volumeAcc += p2.DotProduct3(p0.CrossProduct(p1));
	}
	dgAssert (volumeAcc >= dgFloat64 (0.0f));
	volume = volumeAcc * dgFloat64 (1.0f/6.0f);
	surfaceArea = areaAcc * dgFloat64 (0.5f);
}

// this code has linear time complexity on the number of faces
dgFloat64 dgConvexHull3d::RayCast (const dgBigVector& localP0, const dgBigVector& localP1) const
{
	dgFloat64 interset = dgFloat32 (1.2f);
	dgFloat64 tE = dgFloat64 (0.0f);	// for the maximum entering segment parameter;
	dgFloat64 tL = dgFloat64 (1.0f);	// for the minimum leaving segment parameter;
	dgBigVector dS (localP1 - localP0); // is the segment direction vector;
	dgInt32 hasHit = 0;

	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dgConvexHull3DFace* const face = &node->GetInfo();

		dgInt32 i0 = face->m_index[0];
		dgInt32 i1 = face->m_index[1];
		dgInt32 i2 = face->m_index[2];

		const dgBigVector& p0 = m_points[i0];
		dgBigVector normal ((m_points[i1] - p0).CrossProduct(m_points[i2] - p0));

		//dgFloat64 N = -((localP0 - p0) % normal);
		dgFloat64 N = -normal.DotProduct3(localP0 - p0);
		dgFloat64 D =  normal.DotProduct3(dS);

		if (fabs(D) < dgFloat64 (1.0e-12f)) { //
			if (N < dgFloat64 (0.0f)) {
				return dgFloat64 (1.2f);
			} else {
				continue;
			}
		}

		dgFloat64 t = N / D;
		if (D < dgFloat64 (0.0f)) {
			if (t > tE) {
				tE = t;
				hasHit = 1;
			}
			if (tE > tL) {
				return dgFloat64 (1.2f);
			}
		} else {
			dgAssert (D >= dgFloat64 (0.0f));
			tL = dgMin (tL, t);
			if (tL < tE) {
				return dgFloat64 (1.2f);
			}
		}
	}

	if (hasHit) {
		interset = tE;
	}

	return interset;
}

void dgConvexHull3d::Save (const char* const filename) const
{
	FILE* const file = fopen(filename, "wb");
	int index = 0;
//	fprintf(file, "final\n");
	for (dgListNode* nodePtr = GetFirst(); nodePtr; nodePtr = nodePtr->GetNext()) {
		fprintf(file, "triangle %d\n", index);
		index++;
		const dgConvexHull3DFace& face = nodePtr->GetInfo();
		const dgBigVector& p0 = m_points[face.m_index[0]];
		const dgBigVector& p1 = m_points[face.m_index[1]];
		const dgBigVector& p2 = m_points[face.m_index[2]];

		fprintf(file, "p0(%f %f %f)\n", p0[0], p0[1], p0[2]);
		fprintf(file, "p1(%f %f %f)\n", p1[0], p1[1], p1[2]);
		fprintf(file, "p2(%f %f %f)\n", p2[0], p2[1], p2[2]);
	}
	fprintf(file, "\n");

	fclose(file);
}