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
#include "dgStack.h"
#include "dgTree.h"
#include "dgHeap.h"
#include "dgGoogol.h"
#include "dgConvexHull4d.h"
#include "dgSmallDeterminant.h"

#define DG_VERTEX_CLUMP_SIZE_4D		8 


dgConvexHull4d::dgNormalMap::dgNormalMap()
	:m_count(sizeof (m_normal) / sizeof (m_normal[0]))
{
	dgVector p0(dgFloat32( 1.0f), dgFloat32( 0.0f), dgFloat32( 0.0f), dgFloat32(0.0f));
	dgVector p1(dgFloat32(-1.0f), dgFloat32( 0.0f), dgFloat32( 0.0f), dgFloat32(0.0f));
	dgVector p2(dgFloat32( 0.0f), dgFloat32( 1.0f), dgFloat32( 0.0f), dgFloat32(0.0f));
	dgVector p3(dgFloat32( 0.0f), dgFloat32(-1.0f), dgFloat32( 0.0f), dgFloat32(0.0f));
	dgVector p4(dgFloat32( 0.0f), dgFloat32( 0.0f), dgFloat32( 1.0f), dgFloat32(0.0f));
	dgVector p5(dgFloat32( 0.0f), dgFloat32( 0.0f), dgFloat32(-1.0f), dgFloat32(0.0f));

	dgInt32 count = 0;
	dgInt32 subdivitions = 2;

	dgBigVector tmp[128];
	TessellateTriangle(subdivitions, p4, p0, p2, tmp, count);
	TessellateTriangle(subdivitions, p0, p5, p2, tmp, count);
	TessellateTriangle(subdivitions, p5, p1, p2, tmp, count);
	TessellateTriangle(subdivitions, p1, p4, p2, tmp, count);
	TessellateTriangle(subdivitions, p0, p4, p3, tmp, count);
	TessellateTriangle(subdivitions, p5, p0, p3, tmp, count);
	TessellateTriangle(subdivitions, p1, p5, p3, tmp, count);
	TessellateTriangle(subdivitions, p4, p1, p3, tmp, count);

	count = 0;
	for (dgInt32 j = 0; j < 8; j++) {
		dgFloat64 beta = (j - 4) * dgFloat64 (22.5f * dgDEG2RAD) + dgFloat64 (10.5f * dgDEG2RAD);
		dgFloat64 sinBeta = sin(beta);
		dgFloat64 cosBeta = cos(beta);

		dgFloat64 w = sinBeta;
		for (dgInt32 i = 0; i < 128; i ++) {
			dgFloat64 z = cosBeta * tmp[i].m_z;
			dgFloat64 y = cosBeta * tmp[i].m_y;
			dgFloat64 x = cosBeta * tmp[i].m_x;
			dgInt32 index = dgBitReversal(count, sizeof (m_normal) / sizeof (m_normal[0]));
			dgAssert (index < sizeof (m_normal) / sizeof (m_normal[0]));
			dgAssert (count < sizeof (m_normal) / sizeof (m_normal[0]));
			m_normal[index] = dgBigVector (x, y, z, w);
			count ++;
		}
	}
}

void dgConvexHull4d::dgNormalMap::TessellateTriangle(dgInt32 level, const dgVector& p0, const dgVector& p1, const dgVector& p2, dgBigVector* const buffer, dgInt32& count)
{
	if (level) {
		dgAssert(dgAbs(p0.DotProduct3(p0) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
		dgAssert(dgAbs(p1.DotProduct3(p1) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
		dgAssert(dgAbs(p2.DotProduct3(p2) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
		dgVector p01(p0 + p1);
		dgVector p12(p1 + p2);
		dgVector p20(p2 + p0);

		dgAssert (p01.m_w == dgFloat32 (0.0f));
		dgAssert (p12.m_w == dgFloat32 (0.0f));
		dgAssert (p20.m_w == dgFloat32 (0.0f));
		p01 = p01.Scale(dgRsqrt(p01.DotProduct3(p01)));
		p12 = p12.Scale(dgRsqrt(p12.DotProduct3(p12)));
		p20 = p20.Scale(dgRsqrt(p20.DotProduct3(p20)));

		dgAssert(dgAbs(p01.DotProduct3(p01) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
		dgAssert(dgAbs(p12.DotProduct3(p12) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
		dgAssert(dgAbs(p20.DotProduct3(p20) - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));

		TessellateTriangle(level - 1, p0, p01, p20, buffer, count);
		TessellateTriangle(level - 1, p1, p12, p01, buffer, count);
		TessellateTriangle(level - 1, p2, p20, p12, buffer, count);
		TessellateTriangle(level - 1, p01, p12, p20, buffer, count);
	} else {
		dgBigPlane n(p0, p1, p2);
		n = n.Scale(dgFloat64(1.0f) / sqrt(n.DotProduct3(n)));
		n.m_w = dgFloat64(0.0f);
		dgInt32 index = dgBitReversal(count, 128);
		buffer[index] = n;
		dgAssert(count < 128);
		count++;
	}
}


class dgConvexHull4dAABBTreeNode
{
	public:
	#ifdef _DEBUG
	dgConvexHull4dAABBTreeNode()
	{
		static dgInt32 id = 0;
		m_id = id;
		id ++;
	}
	dgInt32 m_id;
	#endif

		dgBigVector m_box[2];
		dgConvexHull4dAABBTreeNode* m_left;
		dgConvexHull4dAABBTreeNode* m_right;
		dgConvexHull4dAABBTreeNode* m_parent;
};

class dgConvexHull4dPointCluster: public dgConvexHull4dAABBTreeNode
{
	public:
	dgInt32 m_count;
	dgInt32 m_indices[DG_VERTEX_CLUMP_SIZE_4D];
};


dgConvexHull4dTetraherum::dgTetrahedrumPlane::dgTetrahedrumPlane (const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3)
	:dgBigVector ((p1 - p0).CrossProduct (p2 - p0, p3 - p0))
{
	dgBigVector& me = *this;
	dgFloat64 invMag2 = dgFloat32 (0.0f);
	dgFloat64 val = me.DotProduct(me).m_x;
	if (val > dgFloat64 (1.0e-38)) {
		invMag2 = dgFloat64 (1.0f) / sqrt (val);
	} else {
		invMag2 = dgFloat32 (0.0f);
	}

	me.m_x *= invMag2;
	me.m_y *= invMag2;
	me.m_z *= invMag2;
	me.m_w *= invMag2;
	m_dist = - me.DotProduct(p0).m_x;
}

dgFloat64 dgConvexHull4dTetraherum::dgTetrahedrumPlane::Evalue (const dgBigVector& point) const
{
	const dgBigVector& me = *this;
	return me.DotProduct(point).m_x + m_dist;
}


dgConvexHull4dTetraherum::dgConvexHull4dTetraherum()
{
#ifdef _DEBUG
	static dgInt32 debugID;
	m_debugID = debugID;
	debugID ++;
#endif
	static dgInt32 m_monotonicID;

	m_uniqueID = m_monotonicID;
	m_monotonicID ++;
}

void dgConvexHull4dTetraherum::Init (const dgConvexHull4dVector* const points, dgInt32 v0, dgInt32 v1, dgInt32 v2, dgInt32 v3)
{
	m_faces[0].m_index[0] = v0;
	m_faces[0].m_index[1] = v1;
	m_faces[0].m_index[2] = v2;
	m_faces[0].m_index[3] = v3;

	m_faces[1].m_index[0] = v3;
	m_faces[1].m_index[1] = v0;
	m_faces[1].m_index[2] = v2;
	m_faces[1].m_index[3] = v1;

	m_faces[2].m_index[0] = v3;
	m_faces[2].m_index[1] = v2;
	m_faces[2].m_index[2] = v1;
	m_faces[2].m_index[3] = v0;

	m_faces[3].m_index[0] = v3;
	m_faces[3].m_index[1] = v1;
	m_faces[3].m_index[2] = v0;
	m_faces[3].m_index[3] = v2;

	SetMark (0); 
	for (dgInt32 i = 0; i < 4; i ++) {
		m_faces[i].m_twin = NULL;
	}

#ifdef _DEBUG
	dgBigVector p1p0 (points[v1] - points[v0]);
	dgBigVector p2p0 (points[v2] - points[v0]);
	dgBigVector p3p0 (points[v3] - points[v0]);
	dgBigVector normal (p1p0.CrossProduct(p2p0, p3p0));
	dgFloat64 volume = normal.DotProduct(normal).m_x;
	dgAssert (volume > dgFloat64 (0.0f));
#endif
}


dgFloat64 dgConvexHull4dTetraherum::Evalue (const dgConvexHull4dVector* const pointArray, const dgBigVector& point) const
{
	const dgBigVector &p0 = pointArray[m_faces[0].m_index[0]];
	const dgBigVector &p1 = pointArray[m_faces[0].m_index[1]];
	const dgBigVector &p2 = pointArray[m_faces[0].m_index[2]];
	const dgBigVector &p3 = pointArray[m_faces[0].m_index[3]];

	dgFloat64 matrix[4][4];
	for (dgInt32 i = 0; i < 4; i ++) {
		matrix[0][i] = p1[i] - p0[i];
		matrix[1][i] = p2[i] - p0[i];
		matrix[2][i] = p3[i] - p0[i];
		matrix[3][i] = point[i] - p0[i];
	}

	dgFloat64 error;
	dgFloat64 det = Determinant4x4 (matrix, &error);
	dgFloat64 precision  = dgFloat64 (1.0f) / dgFloat64 (1<<24);
	dgFloat64 errbound = error * precision; 
	if (fabs(det) > errbound) {
		return det;
	}

	dgGoogol exactMatrix[4][4];
	for (dgInt32 i = 0; i < 4; i ++) {
		exactMatrix[0][i] = dgGoogol(p1[i]) - dgGoogol(p0[i]);
		exactMatrix[1][i] = dgGoogol(p2[i]) - dgGoogol(p0[i]);
		exactMatrix[2][i] = dgGoogol(p3[i]) - dgGoogol(p0[i]);
		exactMatrix[3][i] = dgGoogol(point[i]) - dgGoogol(p0[i]);
	}
	return Determinant4x4(exactMatrix);
}

dgFloat64 dgConvexHull4dTetraherum::GetTetraVolume(const dgConvexHull4dVector* const points) const
{
	const dgBigVector &p0 = points[m_faces[0].m_index[0]];
	const dgBigVector &p1 = points[m_faces[0].m_index[1]];
	const dgBigVector &p2 = points[m_faces[0].m_index[2]];
	const dgBigVector &p3 = points[m_faces[0].m_index[3]];

	dgFloat64 matrix[3][3];
	for (dgInt32 i = 0; i < 3; i++) {
		matrix[0][i] = p2[i] - p0[i];
		matrix[1][i] = p1[i] - p0[i];
		matrix[2][i] = p3[i] - p0[i];
	}

	dgFloat64 error;
	dgFloat64 det = Determinant3x3(matrix, &error);


	dgFloat64 precision = dgFloat64(1.0f) / dgFloat64(1 << 24);
	dgFloat64 errbound = error * precision;
	if (fabs(det) > errbound) {
		return det;
	}

	dgGoogol exactMatrix[3][3];
	for (dgInt32 i = 0; i < 3; i++) {
		exactMatrix[0][i] = dgGoogol(p2[i]) - dgGoogol(p0[i]);
		exactMatrix[1][i] = dgGoogol(p1[i]) - dgGoogol(p0[i]);
		exactMatrix[2][i] = dgGoogol(p3[i]) - dgGoogol(p0[i]);
	}
	return Determinant3x3(exactMatrix);
}


dgBigVector dgConvexHull4dTetraherum::CircumSphereCenter (const dgConvexHull4dVector* const pointArray) const
{
	dgGoogol matrix[4][4];

	dgBigVector points[4];
	points[0] = pointArray[m_faces[0].m_index[0]];
	points[1] = pointArray[m_faces[0].m_index[1]];
	points[2] = pointArray[m_faces[0].m_index[2]];
	points[3] = pointArray[m_faces[0].m_index[3]];

	for (dgInt32 i = 0; i < 4; i ++) {
		for (dgInt32 j = 0; j < 3; j ++) {
			matrix[i][j] = dgGoogol (points[i][j]);
		}
		matrix[i][3] = dgGoogol (1.0f);
	}
	dgGoogol det (Determinant4x4(matrix));
	dgFloat64 invDen = dgFloat64 (1.0f) / (dgFloat64(det) * dgFloat64 (2.0f));

	dgBigVector centerOut;
	dgFloat64 sign = dgFloat64 (1.0f);
	for (dgInt32 k = 0; k < 3; k ++) {
		for (dgInt32 i = 0; i < 4; i ++) {
			matrix[i][0] = dgGoogol (points[i][3]);
			for (dgInt32 j = 0; j < 2; j ++) {
				dgInt32 j1 = (j < k) ? j : j + 1; 
				matrix[i][j + 1] = dgGoogol (points[i][j1]);
			}
			matrix[i][3] = dgGoogol (1.0f);
		}
		dgGoogol det1 (Determinant4x4(matrix));
		dgFloat64 val = dgFloat64 (det1) * sign;
		sign *= dgFloat64 (-1.0f);
		centerOut[k] = val * invDen; 
	}
	centerOut[3] = dgFloat32 (0.0f);
	return centerOut;
}

dgConvexHull4dTetraherum::dgTetrahedrumPlane dgConvexHull4dTetraherum::GetPlaneEquation (const dgConvexHull4dVector* const points) const
{
	const dgBigVector &p0 = points[m_faces[0].m_index[0]];
	const dgBigVector &p1 = points[m_faces[0].m_index[1]];
	const dgBigVector &p2 = points[m_faces[0].m_index[2]];
	const dgBigVector &p3 = points[m_faces[0].m_index[3]];
	return dgTetrahedrumPlane (p0, p1, p2, p3);
}

dgConvexHull4d::dgConvexHull4d (dgMemoryAllocator* const allocator)
	:dgList<dgConvexHull4dTetraherum>(allocator), m_mark(0), m_count (0), m_diag(), m_points(allocator) 
{
}


dgConvexHull4d::dgConvexHull4d (dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol)
	:dgList<dgConvexHull4dTetraherum>(allocator), m_mark(0), m_count (0), m_diag(), m_points(allocator) 
{
	BuildHull (allocator, vertexCloud, strideInBytes, count, distTol);
}


dgConvexHull4d::~dgConvexHull4d(void)
{
}

const dgConvexHull4d::dgNormalMap& dgConvexHull4d::GetNormaMap()
{
	static dgNormalMap normalMap;
	return normalMap;
}


void dgConvexHull4d::Save (const char* const filename) const
{
	FILE* const file = fopen (filename, "wb");
	int index = 0;
//	fprintf (file, "final\n");
	for (dgListNode* nodePtr = GetFirst(); nodePtr; nodePtr = nodePtr->GetNext()) {
		fprintf (file, "tetra %d\n", index);
		index ++;
		const dgConvexHull4dTetraherum& face = nodePtr->GetInfo();
		const dgBigVector& p0 = m_points[face.m_faces[0].m_index[0]];
		const dgBigVector& p1 = m_points[face.m_faces[0].m_index[1]];
		const dgBigVector& p2 = m_points[face.m_faces[0].m_index[2]];
		const dgBigVector& p3 = m_points[face.m_faces[0].m_index[3]];
		fprintf (file, "p0(%f %f %f %f)\n", p0[0], p0[1], p0[2], p0[3]);
		fprintf (file, "p1(%f %f %f %f)\n", p1[0], p1[1], p1[2], p1[3]);
		fprintf (file, "p2(%f %f %f %f)\n", p2[0], p2[1], p2[2], p2[3]);
		fprintf (file, "p3(%f %f %f %f)\n", p3[0], p3[1], p3[2], p3[3]);
	}
	fprintf (file, "\n");

	fclose (file);
}


dgInt32 dgConvexHull4d::SupportVertex (dgConvexHull4dAABBTreeNode** const treePointer, const dgConvexHull4dVector* const points, const dgBigVector& dir, const bool removeEntry) const
{
	#define DG_STACK_DEPTH_4D	64
	dgFloat64 aabbProjection[DG_STACK_DEPTH_4D];
	const dgConvexHull4dAABBTreeNode *stackPool[DG_STACK_DEPTH_4D];

	dgInt32 index = -1;
	dgInt32 stack = 1;
	stackPool[0] = *treePointer;
	aabbProjection[0] = dgFloat32 (1.0e20f);
	dgFloat64 maxProj = dgFloat64 (-1.0e20f); 
	dgInt32 ix = (dir[0] > dgFloat64 (0.0f)) ? 1 : 0;
	dgInt32 iy = (dir[1] > dgFloat64 (0.0f)) ? 1 : 0;
	dgInt32 iz = (dir[2] > dgFloat64 (0.0f)) ? 1 : 0;
	dgInt32 iw = (dir[3] > dgFloat64 (0.0f)) ? 1 : 0;
	while (stack) {
		stack--;
		dgFloat64 boxSupportValue = aabbProjection[stack];
		if (boxSupportValue > maxProj) {
			const dgConvexHull4dAABBTreeNode* const me = stackPool[stack];

			if (me->m_left && me->m_right) {
				dgBigVector leftSupportPoint (me->m_left->m_box[ix].m_x, me->m_left->m_box[iy].m_y, me->m_left->m_box[iz].m_z, me->m_left->m_box[iw].m_w);
				dgFloat64 leftSupportDist = leftSupportPoint.DotProduct(dir).m_x;

				dgBigVector rightSupportPoint (me->m_right->m_box[ix].m_x, me->m_right->m_box[iy].m_y, me->m_right->m_box[iz].m_z, me->m_right->m_box[iw].m_w);
				dgFloat64 rightSupportDist = rightSupportPoint.DotProduct(dir).m_x;

				if (rightSupportDist >= leftSupportDist) {
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_4D);
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_4D);
				} else {
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_4D);
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < DG_STACK_DEPTH_4D);
				}

			} else {
				dgConvexHull4dPointCluster* const cluster = (dgConvexHull4dPointCluster*) me;
				for (dgInt32 i = 0; i < cluster->m_count; i ++) {
					const dgConvexHull4dVector& p = points[cluster->m_indices[i]];
					dgAssert (p.m_x >= cluster->m_box[0].m_x);
					dgAssert (p.m_x <= cluster->m_box[1].m_x);
					dgAssert (p.m_y >= cluster->m_box[0].m_y);
					dgAssert (p.m_y <= cluster->m_box[1].m_y);
					dgAssert (p.m_z >= cluster->m_box[0].m_z);
					dgAssert (p.m_z <= cluster->m_box[1].m_z);
					dgAssert (p.m_w >= cluster->m_box[0].m_w);
					dgAssert (p.m_w <= cluster->m_box[1].m_w);
					if (!p.m_mark) {
						dgFloat64 dist = p.DotProduct(dir).m_x;
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
					dgConvexHull4dAABBTreeNode* const parent = cluster->m_parent;
					if (parent) {	
						dgConvexHull4dAABBTreeNode* const sibling = (parent->m_left != cluster) ? parent->m_left : parent->m_right;
						dgAssert (sibling != cluster);
						dgConvexHull4dAABBTreeNode* const grandParent = parent->m_parent;
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


dgInt32 dgConvexHull4d::ConvexCompareVertex(const dgConvexHull4dVector* const  A, const dgConvexHull4dVector* const B, void* const context)
{
	for (dgInt32 i = 0; i < 4; i ++) {
		if ((*A)[i] < (*B)[i]) {
			return -1;
		} else if ((*A)[i] > (*B)[i]) {
			return 1;
		}
	}
	return 0;
}


dgConvexHull4dAABBTreeNode* dgConvexHull4d::BuildTree (dgConvexHull4dAABBTreeNode* const parent, dgConvexHull4dVector* const points, dgInt32 count, dgInt32 baseIndex, dgInt8** memoryPool, dgInt32& maxMemSize) const
{
	dgConvexHull4dAABBTreeNode* tree = NULL;

	dgAssert (count);
	dgBigVector minP ( dgFloat32 (1.0e15f)); 
	dgBigVector maxP (-dgFloat32 (1.0e15f)); 
	if (count <= DG_VERTEX_CLUMP_SIZE_4D) {

		dgConvexHull4dPointCluster* const clump = new (*memoryPool) dgConvexHull4dPointCluster;
		*memoryPool += sizeof (dgConvexHull4dPointCluster);
		maxMemSize -= sizeof (dgConvexHull4dPointCluster);
		dgAssert (maxMemSize >= 0);

		dgAssert (clump);
		clump->m_count = count;
		for (dgInt32 i = 0; i < count; i ++) {
			clump->m_indices[i] = i + baseIndex;

			const dgBigVector& p = points[i];
			minP.m_x = dgMin (p.m_x, minP.m_x); 
			minP.m_y = dgMin (p.m_y, minP.m_y); 
			minP.m_z = dgMin (p.m_z, minP.m_z); 
			minP.m_w = dgMin (p.m_w, minP.m_w); 

			maxP.m_x = dgMax (p.m_x, maxP.m_x); 
			maxP.m_y = dgMax (p.m_y, maxP.m_y); 
			maxP.m_z = dgMax (p.m_z, maxP.m_z); 
			maxP.m_w = dgMax (p.m_w, maxP.m_w); 
		}

		clump->m_left = NULL;
		clump->m_right = NULL;
		tree = clump;

	} else {
		dgBigVector median (dgFloat32 (0.0f));
		dgBigVector varian (dgFloat32 (0.0f));
		for (dgInt32 i = 0; i < count; i ++) {

			const dgBigVector& p = points[i];
			minP.m_x = dgMin (p.m_x, minP.m_x); 
			minP.m_y = dgMin (p.m_y, minP.m_y); 
			minP.m_z = dgMin (p.m_z, minP.m_z); 
			minP.m_w = dgMin (p.m_w, minP.m_w); 

			maxP.m_x = dgMax (p.m_x, maxP.m_x); 
			maxP.m_y = dgMax (p.m_y, maxP.m_y); 
			maxP.m_z = dgMax (p.m_z, maxP.m_z); 
			maxP.m_w = dgMax (p.m_w, maxP.m_w); 

			median = median + p;
			varian = varian + p * p;
		}

		varian = varian.Scale (dgFloat32 (count)) - median * median;

		dgInt32 index = 0;
		dgFloat64 maxVarian = dgFloat64 (-1.0e10f);
		for (dgInt32 i = 0; i < 4; i ++) {
			if (varian[i] > maxVarian) {
				index = i;
				maxVarian = varian[i];
			}
		}
		dgBigVector center = median.Scale (dgFloat64 (1.0f) / dgFloat64 (count));

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

		tree = new (*memoryPool) dgConvexHull4dAABBTreeNode;
		*memoryPool += sizeof (dgConvexHull4dAABBTreeNode);
		maxMemSize -= sizeof (dgConvexHull4dAABBTreeNode);
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

dgInt32 dgConvexHull4d::InitVertexArray(dgConvexHull4dVector* const points, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, void* const memoryPool, dgInt32 maxMemSize)
{
	dgInt32 stride = dgInt32(strideInBytes / sizeof (dgFloat64));
	for (dgInt32 i = 0; i < count; i ++) {
		points[i] = dgBigVector (vertexCloud[i * stride + 0], vertexCloud[i * stride + 1], vertexCloud[i * stride + 2], vertexCloud[i * stride + 3]);
		points[i].m_index = i;
		points[i].m_mark = 0;
	}

	dgSort(points, count, ConvexCompareVertex);
	dgInt32 indexCount = 0;
	for (int i = 1; i < count; i ++) {
		for (; i < count; i ++) {
			if (ConvexCompareVertex (&points[indexCount], &points[i], NULL)) {
				indexCount ++;
				points[indexCount] = points[i];
				break;
			}
		}
	}
	count = indexCount + 1;
	if (count < 4) {
		m_count = 0;
		return count;
	}

	dgConvexHull4dAABBTreeNode* tree = BuildTree (NULL, points, count, 0, (dgInt8**) &memoryPool, maxMemSize);

	dgBigVector boxSize (tree->m_box[1] - tree->m_box[0]);	
	m_diag = dgFloat32 (sqrt (boxSize.DotProduct(boxSize).m_x));

	dgInt32 marks[4];
	m_points.Resize(count);
	bool validTetrahedrum = false;
	dgConvexHull4dVector* const convexPoints = &m_points[0]; 
	const dgFloat64 testVol = dgFloat32 (1.0e-6f) * m_diag * m_diag * m_diag;

	const dgNormalMap& normalMap = GetNormaMap();
	for (dgInt32 i = 0; !validTetrahedrum && (i < normalMap.m_count); i++) {
		dgInt32 index = SupportVertex(&tree, points, normalMap.m_normal[i], false);
		convexPoints[0] = points[index];
		marks[0] = index;
		for (dgInt32 j = i + 1; !validTetrahedrum && (j < normalMap.m_count); j++) {
			dgInt32 index1 = SupportVertex(&tree, points, normalMap.m_normal[j], false);
			convexPoints[1] = points[index1];
			dgBigVector p10(convexPoints[1] - convexPoints[0]);
			if (p10.DotProduct(p10).GetScalar() >(dgFloat32(1.0e-3f) * m_diag)) {
				marks[1] = index1;
				for (dgInt32 k = j + 1; !validTetrahedrum && (k < normalMap.m_count); k++) {
					dgInt32 index2 = SupportVertex(&tree, points, normalMap.m_normal[k], false);
					convexPoints[2] = points[index2];
					dgBigVector p20(convexPoints[2] - convexPoints[0]);
					dgBigVector p21(convexPoints[2] - convexPoints[1]);
					bool test = p20.DotProduct(p20).GetScalar() > (dgFloat32(1.0e-3f) * m_diag);
					test = test && (p21.DotProduct(p21).GetScalar() > (dgFloat32(1.0e-3f) * m_diag));
					if (test) {
						marks[2] = index2;
						for (dgInt32 l = k + 1; !validTetrahedrum && (l < normalMap.m_count); l++) {
							dgInt32 index3 = SupportVertex(&tree, points, normalMap.m_normal[l], false);
							convexPoints[3] = points[index3];
							dgBigVector p30(convexPoints[3] - convexPoints[0]);
							dgBigVector plane(p10.CrossProduct(p20, p30));
							dgFloat64 volume = plane.DotProduct(plane).GetScalar();
							if (volume > testVol) {
								validTetrahedrum = true;
								marks[3] = index3;
							}
						}
					}
				}
			}
		}
	}

	m_count = 4;
	if (!validTetrahedrum) {
		m_count = 0;
	}

	if (validTetrahedrum) {
		for (dgInt32 i = 0; i < 4; i ++) {
			points[marks[i]].m_mark = 1;
		}
	}

	return count;
}


dgConvexHull4d::dgListNode* dgConvexHull4d::AddFace (dgInt32 i0, dgInt32 i1, dgInt32 i2, dgInt32 i3)
{
	dgListNode* const node = Append();
	dgConvexHull4dTetraherum& face = node->GetInfo();
	face.Init (&m_points[0], i0, i1, i2, i3);
	return node;
}


void dgConvexHull4d::DeleteFace (dgListNode* const node) 
{
	Remove (node);
}


bool dgConvexHull4d::Sanity() const
{
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		dgConvexHull4dTetraherum* const tetra = &node->GetInfo();

		for (dgInt32 i = 0; i < 4; i ++) {
			dgConvexHull4dTetraherum::dgTetrahedrumFace* const face = &tetra->m_faces[i];
			dgListNode* const twinNode = face->m_twin;
			if (!twinNode) {
				return false;
			}
		}
	}

/*
	dgList<dgListNode*> tetraList(GetAllocator());
	const dgHullVector* const points = &m_points[0];
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		dgConvexHull4dTetraherum* const tetra = &node->GetInfo();
		const dgBigVector &p0 = points[tetra->m_faces[0].m_index[0]];
		const dgBigVector &p1 = points[tetra->m_faces[0].m_index[1]];
		const dgBigVector &p2 = points[tetra->m_faces[0].m_index[2]];
		const dgBigVector &p3 = points[tetra->m_faces[0].m_index[3]];

		dgBigVector p1p0 (p1.Sub4(p0));
		dgBigVector p2p0 (p2.Sub4(p0));
		dgBigVector p3p0 (p3.Sub4(p0));
		dgBigVector normal (p1p0.CrossProduct (p2p0, p3p0));

		if (normal.m_w < dgFloat64 (0.0f)) {
			tetraList.Append(node);
		}
	}

	for (dgList<dgListNode*>::dgListNode* node0 = tetraList.GetFirst(); node0; node0 = node0->GetNext()) {
		dgListNode* const tetraNode0 = node0->GetInfo();
		dgConvexHull4dTetraherum* const tetra0 = &tetraNode0->GetInfo();

		dgInt32 index0[4];
		index0[0] = tetra0->m_faces[0].m_index[0];
		index0[1] = tetra0->m_faces[0].m_index[1];
		index0[2] = tetra0->m_faces[0].m_index[2];
		index0[3] = tetra0->m_faces[0].m_index[3];

		const dgBigVector &p0 = points[index0[0]];
		const dgBigVector &p1 = points[index0[1]];
		const dgBigVector &p2 = points[index0[2]];
		const dgBigVector &p3 = points[index0[3]];
		for (dgList<dgListNode*>::dgListNode* node1 = node0->GetNext(); node1; node1 = node1->GetNext()) {
			dgListNode* const tetraNode1 = node1->GetInfo();
			dgConvexHull4dTetraherum* const tetra1 = &tetraNode1->GetInfo();

			dgInt32 index1[4];
			index1[0] = tetra1->m_faces[0].m_index[0];
			index1[1] = tetra1->m_faces[0].m_index[1];
			index1[2] = tetra1->m_faces[0].m_index[2];
			index1[3] = tetra1->m_faces[0].m_index[3];

			for (dgInt32 i = 0; i < 4; i ++) {
				dgInt32 count = 0;
				dgInt32 k = index1[i];
				for (dgInt32 j = 0; j < 4; j ++) {
					count += (k == index0[j]);
				}
				if (!count){
//					const dgBigVector &p = points[k];
//					dgFloat64 size = -insphere(&p0.m_x, &p1.m_x, &p2.m_x, &p3.m_x, &p.m_x);
//					if (size < dgFloat64 (0.0f)) {
//						return false;
//					}
				}
			}
		}
	}
*/

	return true;
}


void dgConvexHull4d::LinkSibling (dgListNode* node0, dgListNode* node1)	const
{
	dgConvexHull4dTetraherum* const tetra0 = &node0->GetInfo();
	dgConvexHull4dTetraherum* const tetra1 = &node1->GetInfo();
	for (int i = 0; i < 4; i ++) {
		dgConvexHull4dTetraherum::dgTetrahedrumFace* const face0 = &tetra0->m_faces[i];
		if (!face0->m_twin) {
			dgInt32 i0 = face0->m_index[0];
			dgInt32 i1 = face0->m_index[1];
			dgInt32 i2 = face0->m_index[2];
			for (int j = 0; j < 4; j ++) {
				dgConvexHull4dTetraherum::dgTetrahedrumFace* const face1 = &tetra1->m_faces[j];
				if (!face1->m_twin) {
					dgInt32 j2 = face1->m_index[0];
					dgInt32 j1 = face1->m_index[1];
					dgInt32 j0 = face1->m_index[2];

					if (((i0 == j0) && (i1 == j1) && (i2 == j2)) || ((i1 == j0) && (i2 == j1) && (i0 == j2)) || ((i2 == j0) && (i0 == j1) && (i1 == j2))) {
						face0->m_twin = node1;
						face1->m_twin = node0;
						return;
					}
				}
			}
		}
	}
}


void dgConvexHull4d::InsertNewVertex(dgInt32 vertexIndex, dgListNode* const frontFace, dgList<dgListNode*>& deletedFaces, dgList<dgListNode*>& newFaces)
{
	dgAssert (Sanity());
	dgList<dgListNode*> stack(GetAllocator());
	
	dgInt32 mark = IncMark();
	stack.Append(frontFace);
	dgConvexHull4dVector* const hullVertexArray = &m_points[0];
	const dgBigVector& p = hullVertexArray[vertexIndex];
	while (stack.GetCount()) {
		dgList<dgListNode*>::dgListNode* const stackNode = stack.GetLast();
		dgListNode* const node = stackNode->GetInfo();
		stack.Remove(stackNode);
		dgConvexHull4dTetraherum* const face = &node->GetInfo();
		if ((face->GetMark() != mark) && (face->Evalue(hullVertexArray, p) > dgFloat64(0.0f))) { 
#ifdef _DEBUG
			for (dgList<dgListNode*>::dgListNode* deleteNode = deletedFaces.GetFirst(); deleteNode; deleteNode = deleteNode->GetNext()) {
				dgAssert (deleteNode->GetInfo() != node);
			}
#endif
			deletedFaces.Append(node);

			face->SetMark(mark);
			for (dgInt32 i = 0; i < 4; i ++) {
				dgListNode* const twinNode = (dgListNode*)face->m_faces[i].m_twin;
				dgAssert (twinNode);
				dgConvexHull4dTetraherum* const twinFace = &twinNode->GetInfo();

				if (twinFace->GetMark() != mark) {
					stack.Append(twinNode);
				}
			}
		}
	}

    dgTree<dgListNode*, dgInt32> perimeter(GetAllocator());
	for (dgList<dgListNode*>::dgListNode* deleteNode = deletedFaces.GetFirst(); deleteNode; deleteNode = deleteNode->GetNext()) {
		dgListNode* const deleteTetraNode = deleteNode->GetInfo();
		dgConvexHull4dTetraherum* const deletedTetra = &deleteTetraNode->GetInfo();
		dgAssert (deletedTetra->GetMark() == mark);
		for (dgInt32 i = 0; i < 4; i ++) {
			dgListNode* const twinNode = deletedTetra->m_faces[i].m_twin;
			dgConvexHull4dTetraherum* const twinTetra = &twinNode->GetInfo();

			if (twinTetra->GetMark() != mark) {
				if (!perimeter.Find(twinTetra->m_uniqueID)) {
					perimeter.Insert(twinNode, twinTetra->m_uniqueID);
				}
			}
			deletedTetra->m_faces[i].m_twin = NULL;
		}
	}

	dgList<dgListNode*> coneList(GetAllocator());
	dgTree<dgListNode*, dgInt32>::Iterator iter (perimeter);
	for (iter.Begin(); iter; iter ++) {
		dgListNode* const perimeterNode = iter.GetNode()->GetInfo();
		dgConvexHull4dTetraherum* const perimeterTetra = &perimeterNode->GetInfo();
		for (dgInt32 i = 0; i < 4; i ++) {
			dgConvexHull4dTetraherum::dgTetrahedrumFace* const perimeterFace = &perimeterTetra->m_faces[i];

			if (perimeterFace->m_twin->GetInfo().GetMark() == mark) {
				dgListNode* const newNode = AddFace (vertexIndex, perimeterFace->m_index[0], perimeterFace->m_index[1], perimeterFace->m_index[2]);
				newFaces.Addtop(newNode);

				dgConvexHull4dTetraherum* const newTetra = &newNode->GetInfo();
				newTetra->m_faces[2].m_twin = perimeterNode;
				perimeterFace->m_twin = newNode;
				coneList.Append (newNode);
			}
		}
	}

	for (dgList<dgListNode*>::dgListNode* coneNode = coneList.GetFirst(); coneNode->GetNext(); coneNode = coneNode->GetNext()) {
		dgListNode* const coneNodeA = coneNode->GetInfo();
		for (dgList<dgListNode*>::dgListNode* nextConeNode = coneNode->GetNext(); nextConeNode; nextConeNode = nextConeNode->GetNext()) {
			dgListNode* const coneNodeB = nextConeNode->GetInfo();
			LinkSibling (coneNodeA, coneNodeB);
		}
	}
}

dgConvexHull4d::dgListNode* dgConvexHull4d::FindFacingNode(const dgBigVector& vertex)
{
	const dgConvexHull4dVector* const hullVertexArray = &m_points[0];

	dgListNode* bestNode = GetFirst();
	dgConvexHull4dTetraherum* const tetra = &bestNode->GetInfo();
	dgConvexHull4dTetraherum::dgTetrahedrumPlane plane (tetra->GetPlaneEquation (hullVertexArray));
	dgFloat64 dist = plane.Evalue(vertex);
	dgInt32 mark = IncMark();
	tetra->SetMark(mark);

	dgInt8 buffer[1024 * 2 * sizeof (dgFloat64)];
	dgDownHeap<dgListNode*, dgFloat64> heap (buffer, sizeof (buffer));

	heap.Push(bestNode, dist);
	dgInt32 maxCount = heap.GetMaxCount() - 1;
	dgInt32 releafCount = maxCount >> 3;
	while (heap.GetCount()) {
		dgListNode* const node1 = heap[0];
		dgFloat64 dist1 = heap.Value();
		if (dist1 > dgFloat64 (1.0e-5f)) {
			return node1;
		}
		heap.Pop();
		dgConvexHull4dTetraherum* const tetra1 = &node1->GetInfo();
		for (dgInt32 i = 0; i < 4; i ++) {
			dgListNode* neigborghNode = tetra1->m_faces[i].m_twin;
			dgConvexHull4dTetraherum* const neighborgh = &neigborghNode->GetInfo();
			if (neighborgh->GetMark() != mark) {
				neighborgh->SetMark(mark);
				if (heap.GetCount() >= maxCount) {
					for (dgInt32 j = 0; j < releafCount; j ++) {
						heap.Remove(heap.GetCount() - 1);
					}
				}
				dgConvexHull4dTetraherum::dgTetrahedrumPlane plane1 (neighborgh->GetPlaneEquation (hullVertexArray));
				heap.Push(neigborghNode, plane1.Evalue(vertex));
			}
		}
	}

	for (dgListNode* node1 = GetFirst(); node1; node1 = node1->GetNext()) {
		dgConvexHull4dTetraherum* const tetra1 = &node1->GetInfo();
		dgFloat64 dist1 = tetra1->Evalue(hullVertexArray, vertex);
		if (dist1 > dgFloat64(0.0f)) {
			return node1;
		}
	}


	return NULL;
}

dgInt32 dgConvexHull4d::AddVertex (const dgBigVector& vertex)
{
	dgSetPrecisionDouble precision;
	dgInt32 index = -1;
	dgListNode* const faceNode = FindFacingNode(vertex);
	if (faceNode) {
		index = m_count;
		m_points[index] = vertex;
		m_points[index].m_index = index;
		m_count ++;

		dgList<dgListNode*> newFaces(GetAllocator());
		dgList<dgListNode*> deleteList(GetAllocator());
		
		InsertNewVertex(index, faceNode, deleteList, newFaces);
		for (dgList<dgListNode*>::dgListNode* deleteNode = deleteList.GetFirst(); deleteNode; deleteNode = deleteNode->GetNext()) {
			dgListNode* const node = deleteNode->GetInfo();
			DeleteFace (node); 
		}
	}
	return index;
}


void dgConvexHull4d::BuildHull (dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol)
{
	dgInt32 treeCount = count / (DG_VERTEX_CLUMP_SIZE_4D>>1); 
	if (treeCount < 4) {
		treeCount = 4;
	}
	treeCount *= 2;

	dgStack<dgConvexHull4dVector> points (count);
	dgStack<dgConvexHull4dPointCluster> treePool (treeCount + 256);

	count = InitVertexArray(&points[0], vertexCloud, strideInBytes, count, &treePool[0], treePool.GetSizeInBytes());
	if (m_count >= 4) {
		CalculateConvexHull (&treePool[0], &points[0], count, distTol);
	}
}


void dgConvexHull4d::CalculateConvexHull (dgConvexHull4dAABBTreeNode* vertexTree, dgConvexHull4dVector* const points, dgInt32 count, dgFloat64 distTol)
{
	distTol = fabs (distTol) * m_diag;
	dgListNode* const nodes0 = AddFace (0, 1, 2, 3);
	dgListNode* const nodes1 = AddFace (0, 1, 3, 2);

	dgList<dgListNode*> boundaryFaces(GetAllocator());
	boundaryFaces.Append(nodes0);
	boundaryFaces.Append(nodes1);

	LinkSibling (nodes0, nodes1);
	LinkSibling (nodes0, nodes1);
	LinkSibling (nodes0, nodes1);
	LinkSibling (nodes0, nodes1);

	IncMark();
	count -= 4;
	dgInt32 currentIndex = 4;
	while (boundaryFaces.GetCount() && count) {
		dgConvexHull4dVector* const hullVertexArray = &m_points[0]; 
		dgListNode* const faceNode = boundaryFaces.GetFirst()->GetInfo();
		dgConvexHull4dTetraherum* const face = &faceNode->GetInfo();
		dgConvexHull4dTetraherum::dgTetrahedrumPlane planeEquation (face->GetPlaneEquation (hullVertexArray));

		dgInt32 index = SupportVertex (&vertexTree, points, planeEquation);

		const dgConvexHull4dVector& p = points[index];
		dgFloat64 dist = planeEquation.Evalue(p);
		if ((dist > distTol) && (face->Evalue(hullVertexArray, p) > dgFloat64(0.0f))) {

			m_points[currentIndex] = p;
			points[index].m_mark = 1;

			dgList<dgListNode*> deleteList(GetAllocator());
			InsertNewVertex(currentIndex, faceNode, deleteList, boundaryFaces);

			for (dgList<dgListNode*>::dgListNode* deleteNode = deleteList.GetFirst(); deleteNode; deleteNode = deleteNode->GetNext()) {
				dgListNode* const node = deleteNode->GetInfo();
				boundaryFaces.Remove (node);
				DeleteFace (node); 
			}

			currentIndex ++;
			count --;
		} else {
			boundaryFaces.Remove (faceNode);
		}
	}
	m_count = currentIndex;
}
