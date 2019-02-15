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
#include "dgMeshEffect.h"
#include "dgCollisionBVH.h"
#include "dgCollisionCompound.h"
#include "dgCollisionConvexHull.h"


dgMeshEffect::dgPointFormat::dgPointFormat(dgMemoryAllocator* const allocator)
	:m_layers(allocator)
	,m_vertex(allocator)
{
}

dgMeshEffect::dgPointFormat::dgPointFormat(const dgPointFormat& source)
	:m_layers(source.m_layers)
	,m_vertex(source.m_vertex)
{
}

dgMeshEffect::dgPointFormat::~dgPointFormat()
{
}

void dgMeshEffect::dgPointFormat::Clear()
{
	m_layers.Clear();
	m_vertex.Clear();
}

void dgMeshEffect::dgPointFormat::SetCount(dgInt32 count)
{
	m_layers.SetCount(count);
	m_vertex.SetCount(count);
}

dgInt32 dgMeshEffect::dgFormat::GetSortIndex (const dgChannel<dgBigVector, m_point>& points, dgFloat64& dist) const
{
	dgBigVector xc(dgFloat64(0.0f));
	dgBigVector x2c(dgFloat64(0.0f));
	dgBigVector minP(dgFloat64(1.0e10f));
	dgBigVector maxP(dgFloat64(-1.0e10f));
	for (dgInt32 i = 0; i < points.m_count; i++) {
		dgBigVector x(points[i]);
		xc += x;
		x2c += x * x;
		minP = minP.GetMin(x);
		maxP = maxP.GetMax(x);
	}

	dgBigVector del(maxP - minP);
	dgFloat64 minDist = dgMin(del.m_x, del.m_y, del.m_z);
	if (minDist < dgFloat64(1.0e-3f)) {
		minDist = dgFloat64(1.0e-3f);
	}

	dgInt32 firstSortAxis = 0;
	x2c = x2c.Scale(points.m_count) - xc * xc;
	if ((x2c.m_y >= x2c.m_x) && (x2c.m_y >= x2c.m_z)) {
		firstSortAxis = 1;
	}
	else if ((x2c.m_z >= x2c.m_x) && (x2c.m_z >= x2c.m_y)) {
		firstSortAxis = 2;
	}
	dist = minDist;
	return firstSortAxis;
}

dgInt32 dgMeshEffect::dgFormat::CompareVertex(const dgSortKey* const ptr0, const dgSortKey* const ptr1, void* const context)
{
	const VertexSortData* const sortContext = (VertexSortData*)context;
	const dgInt32 compIndex = sortContext->m_vertexSortIndex;
	const dgChannel<dgBigVector, m_point>& points = *sortContext->m_points;
	const dgFloat64 x0 = points[ptr0->m_vertexIndex][compIndex];
	const dgFloat64 x1 = points[ptr1->m_vertexIndex][compIndex];

	if (x0 < x1) {
		return -1;
	} else if (x0 > x1) {
		return 1;
	}
	return 0;
}

void dgMeshEffect::dgPointFormat::CompressData(dgInt32* const indexList)
{
	dgFloat64 minDist;
	const dgInt32 firstSortAxis = GetSortIndex(m_vertex, minDist);

	dgStack<dgFormat::dgSortKey> indirectListBuffer(m_vertex.m_count);
	dgFormat::dgSortKey* indirectList = &indirectListBuffer[0];
	for (dgInt32 i = 0; i < m_vertex.m_count; i++) {
		indirectList[i].m_mask = -1;
		indirectList[i].m_ordinal = i;
		indirectList[i].m_vertexIndex = i;
		indirectList[i].m_attibuteIndex = -1;
	}

	dgPointFormat tmpFormat(*this);
	VertexSortData sortContext;
	sortContext.m_points = &tmpFormat.m_vertex;
	//sortContext.m_points = &tmpFormat;
	sortContext.m_vertexSortIndex = firstSortAxis;
	dgSort(indirectList, m_vertex.m_count, dgFormat::CompareVertex, &sortContext);

	const dgFloat64 tolerance = dgMin (minDist, dgFloat64 (1.0e-12f));
	const dgFloat64 sweptWindow = dgFloat64(2.0f) * tolerance + dgFloat64 (1.0e-10f);

	dgInt32 newCount = 0;
	for (dgInt32 i = 0; i < tmpFormat.m_vertex.m_count; i++) {
		const dgInt32 ii = indirectList[i].m_mask;
		if (ii == -1) {
			const dgInt32 i0 = indirectList[i].m_ordinal;
			const dgInt32 iii = indirectList[i].m_vertexIndex;
			const dgBigVector& p = tmpFormat.m_vertex[iii];
			//const dgFloat64 swept = tmpFormat.m_vertex[iii][firstSortAxis] + sweptWindow;
			const dgFloat64 swept = p[firstSortAxis] + sweptWindow;
			for (dgInt32 j = i + 1; j < tmpFormat.m_vertex.m_count; j++) {

				const dgInt32 jj = indirectList[j].m_mask;
				if (jj == -1) {
					const dgInt32 j0 = indirectList[j].m_ordinal;
					const dgInt32 jjj = indirectList[j].m_vertexIndex;
					const dgBigVector& q = tmpFormat.m_vertex[jjj];
					//dgFloat64 val = tmpFormat.m_vertex[jjj][firstSortAxis];
					dgFloat64 val = q[firstSortAxis];
					if (val >= swept) {
						break;
					}

					bool test = true;
					if (iii != jjj) {
						//dgBigVector dp(tmpFormat.m_vertex[iii] - tmpFormat.m_vertex[jjj]);
						dgBigVector dp(p - q);
						for (dgInt32 k = 0; k < 3; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}
					if (test && tmpFormat.m_layers.m_count) {
						test &= (tmpFormat.m_layers[i0] == tmpFormat.m_layers[j0]);
					}
					// note, is ok weight duplicate to be ignored.

					if (test) {
						indirectList[j].m_mask = newCount;
					}
				}
			}

			//indirectList[newCount].m_attibuteIndex = indirectList[i].m_attibuteIndex;
			indirectList[newCount].m_vertexIndex = indirectList[i].m_vertexIndex;
			indirectList[i].m_mask = newCount;
			newCount++;
		}
	}

	Clear();
	for (dgInt32 i = 0; i < newCount; i++) {
		dgAssert (indirectList[i].m_attibuteIndex == -1);
		m_vertex.PushBack(tmpFormat.m_vertex[indirectList[i].m_vertexIndex]);
	}

	if (tmpFormat.m_layers.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_layers.PushBack(tmpFormat.m_layers[indirectList[i].m_vertexIndex]);
		}
	}

	for (dgInt32 i = 0; i < tmpFormat.m_vertex.m_count; i++) {
		dgInt32 i1 = indirectList[i].m_ordinal;
		dgInt32 index = indirectList[i].m_mask;
		indexList[i1] = index;
	}
}

dgMeshEffect::dgAttibutFormat::dgAttibutFormat(dgMemoryAllocator* const allocator)
	:m_pointChannel(allocator)
	,m_materialChannel(allocator)
	,m_normalChannel(allocator)
	,m_binormalChannel(allocator)
	,m_colorChannel(allocator)
	,m_uv0Channel(allocator)
	,m_uv1Channel(allocator)
{
}

dgMeshEffect::dgAttibutFormat::dgAttibutFormat(const dgAttibutFormat& source)
	:m_pointChannel(source.m_pointChannel)
	,m_materialChannel(source.m_materialChannel)
	,m_normalChannel(source.m_normalChannel)
	,m_binormalChannel(source.m_binormalChannel)
	,m_colorChannel(source.m_colorChannel)
	,m_uv0Channel(source.m_uv0Channel)
	,m_uv1Channel(source.m_uv1Channel)
{
}

dgMeshEffect::dgAttibutFormat::~dgAttibutFormat()
{
}


void dgMeshEffect::dgAttibutFormat::Clear()
{
	m_pointChannel.Clear();
	m_materialChannel.Clear();
	m_normalChannel.Clear();
	m_binormalChannel.Clear();
	m_colorChannel.Clear();
	m_uv0Channel.Clear();
	m_uv1Channel.Clear();
}

void dgMeshEffect::dgAttibutFormat::SetCount (dgInt32 count)
{
	m_pointChannel.SetCount(count);
	m_materialChannel.SetCount(count);
	m_normalChannel.SetCount(count);
	m_binormalChannel.SetCount(count);
	m_colorChannel.SetCount(count);
	m_uv0Channel.SetCount(count);
	m_uv1Channel.SetCount(count);
}

void dgMeshEffect::dgAttibutFormat::CopyFrom (const dgAttibutFormat& source)
{
	m_pointChannel.CopyFrom(source.m_pointChannel);
	m_materialChannel.CopyFrom(source.m_materialChannel);
	m_normalChannel.CopyFrom(source.m_normalChannel);
	m_binormalChannel.CopyFrom(source.m_binormalChannel);
	m_colorChannel.CopyFrom(source.m_colorChannel);
	m_uv0Channel.CopyFrom(source.m_uv0Channel);
	m_uv1Channel.CopyFrom(source.m_uv1Channel);
}

void dgMeshEffect::dgAttibutFormat::CompressData (const dgPointFormat& points, dgInt32* const indexList)
{
	dgFloat64 minDist;
	const dgInt32 firstSortAxis = GetSortIndex(points.m_vertex, minDist);

	dgStack<dgFormat::dgSortKey> indirectListBuffer(m_pointChannel.m_count);
	dgFormat::dgSortKey* indirectList = &indirectListBuffer[0];
	for (dgInt32 i = 0; i < m_pointChannel.m_count; i++) {
		indirectList[i].m_mask = -1;
		indirectList[i].m_ordinal = i;
		indirectList[i].m_attibuteIndex = i;
		indirectList[i].m_vertexIndex = m_pointChannel[i];
	}

	VertexSortData sortContext;
	sortContext.m_points = &points.m_vertex;
	sortContext.m_vertexSortIndex = firstSortAxis;
	dgSort (indirectList, m_pointChannel.m_count, dgFormat::CompareVertex, &sortContext);
	dgAttibutFormat tmpFormat (*this);
	Clear();

	const dgFloat64 tolerance = dgMin(minDist, dgFloat64(1.0e-12f));
	const dgFloat64 sweptWindow = dgFloat64(2.0f) * tolerance + dgFloat64(1.0e-10f);

	dgInt32 newCount = 0;
	for (dgInt32 i = 0; i < tmpFormat.m_pointChannel.m_count; i ++) {
		const dgInt32 ii = indirectList[i].m_mask;
		if (ii == -1) {
			const dgInt32 i0 = indirectList[i].m_ordinal;
			const dgInt32 iii = indirectList[i].m_vertexIndex;
			const dgFloat64 swept = points.m_vertex[iii][firstSortAxis] + sweptWindow;
			for (dgInt32 j = i + 1; j < tmpFormat.m_pointChannel.m_count; j++) {
				const dgInt32 jj = indirectList[j].m_mask;
				if (jj == -1) {
					const dgInt32 j0 = indirectList[j].m_ordinal;
					const dgInt32 jjj = indirectList[j].m_vertexIndex;;
					dgFloat64 val = points.m_vertex[jjj][firstSortAxis];
					if (val >= swept) {
						break;
					}

					bool test = true;
					if (iii != jjj) {
						dgBigVector dp (points.m_vertex[iii] - points.m_vertex[jjj]);
						for (dgInt32 k = 0; k < 3; k ++) {
							test &= (fabs (dp[k]) <= tolerance);
						}
					}
					if (test && points.m_layers.m_count) {
						test &= (points.m_layers[iii] == points.m_layers[jjj]);
					}

					if (test && tmpFormat.m_normalChannel.m_count) {
						dgVector n0(tmpFormat.m_normalChannel[i0].m_x, tmpFormat.m_normalChannel[i0].m_y, tmpFormat.m_normalChannel[i0].m_z, dgFloat32 (0.0f)); 
						dgVector n1(tmpFormat.m_normalChannel[j0].m_x, tmpFormat.m_normalChannel[j0].m_y, tmpFormat.m_normalChannel[j0].m_z, dgFloat32 (0.0f));  
						dgVector dp (n1 - n0);
						for (dgInt32 k = 0; k < 3; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}

					if (test && tmpFormat.m_binormalChannel.m_count) {
						dgVector n0(tmpFormat.m_binormalChannel[i0].m_x, tmpFormat.m_binormalChannel[i0].m_y, tmpFormat.m_binormalChannel[i0].m_z, dgFloat32(0.0f));
						dgVector n1(tmpFormat.m_binormalChannel[j0].m_x, tmpFormat.m_binormalChannel[j0].m_y, tmpFormat.m_binormalChannel[j0].m_z, dgFloat32(0.0f));
						dgVector dp(n1 - n0);
						for (dgInt32 k = 0; k < 3; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}

					if (test && tmpFormat.m_uv0Channel.m_count) {
						dgVector n0(tmpFormat.m_uv0Channel[i0].m_u, tmpFormat.m_uv0Channel[i0].m_v, dgFloat32(0.0f), dgFloat32(0.0f));
						dgVector n1(tmpFormat.m_uv0Channel[j0].m_u, tmpFormat.m_uv0Channel[j0].m_v, dgFloat32(0.0f), dgFloat32(0.0f));
						dgVector dp(n1 - n0);
						for (dgInt32 k = 0; k < 2; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}

					if (test && tmpFormat.m_uv1Channel.m_count) {
						dgVector n0(tmpFormat.m_uv1Channel[i0].m_u, tmpFormat.m_uv1Channel[i0].m_v, dgFloat32(0.0f), dgFloat32(0.0f));
						dgVector n1(tmpFormat.m_uv1Channel[j0].m_u, tmpFormat.m_uv1Channel[j0].m_v, dgFloat32(0.0f), dgFloat32(0.0f));
						dgVector dp(n1 - n0);
						for (dgInt32 k = 0; k < 2; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}

					if (test && tmpFormat.m_colorChannel.m_count) {
						dgVector dp(m_colorChannel[i0] - m_colorChannel[j0]);
						for (dgInt32 k = 0; k < 3; k++) {
							test &= (fabs(dp[k]) <= tolerance);
						}
					}

					if (test && tmpFormat.m_materialChannel.m_count) {
						test &= (tmpFormat.m_materialChannel[i0] == tmpFormat.m_materialChannel[j0]);
					}

					if (test) {
						indirectList[j].m_mask = newCount;
					}
				}
			}

			indirectList[newCount].m_attibuteIndex = indirectList[i].m_attibuteIndex;
			indirectList[newCount].m_vertexIndex = indirectList[i].m_vertexIndex;
			indirectList[i].m_mask = newCount;
			newCount++;
		}
	}

	for (dgInt32 i = 0; i < newCount; i ++) {
		m_pointChannel.PushBack(indirectList[i].m_vertexIndex);
	}

	if (tmpFormat.m_normalChannel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_normalChannel.PushBack(tmpFormat.m_normalChannel[indirectList[i].m_attibuteIndex]);
		}
	}

	if (tmpFormat.m_binormalChannel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_binormalChannel.PushBack(tmpFormat.m_binormalChannel[indirectList[i].m_attibuteIndex]);
		}
	}

	if (tmpFormat.m_uv0Channel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_uv0Channel.PushBack(tmpFormat.m_uv0Channel[indirectList[i].m_attibuteIndex]);
		}
	}

	if (tmpFormat.m_uv1Channel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_uv1Channel.PushBack(tmpFormat.m_uv1Channel[indirectList[i].m_attibuteIndex]);
		}
	}

	if (tmpFormat.m_colorChannel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_colorChannel.PushBack(tmpFormat.m_colorChannel[indirectList[i].m_attibuteIndex]);
		}
	}

	if (tmpFormat.m_materialChannel.m_count) {
		for (dgInt32 i = 0; i < newCount; i++) {
			m_materialChannel.PushBack(tmpFormat.m_materialChannel[indirectList[i].m_attibuteIndex]);
		}
	}

	for (dgInt32 i = 0; i < tmpFormat.m_pointChannel.m_count; i ++) {
		dgInt32 i1 = indirectList[i].m_ordinal;
		dgInt32 index = indirectList[i].m_mask;
		indexList[i1] = index;
	}
}

dgMeshEffect::dgMeshBVH::dgFitnessList::dgFitnessList (dgMemoryAllocator* const allocator)
	:dgTree <dgMeshBVHNode*, dgMeshBVHNode*>(allocator)
{
}

dgFloat64 dgMeshEffect::dgMeshBVH::dgFitnessList::TotalCost () const
{
	dgFloat64 cost = dgFloat32 (0.0f);
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		dgMeshBVHNode* const box = node->GetInfo();
		cost += box->m_area;
	}
	return cost;
}


dgMeshEffect::dgMeshBVH::dgMeshBVHNode::dgMeshBVHNode (const dgMeshEffect* const mesh, dgEdge* const face, void* const userData)
	:m_area(dgFloat32 (0.0f))
	,m_face (face)
	,m_userData(userData)
	,m_left (NULL)
	,m_right(NULL)
	,m_parent(NULL)
{
	dgBigVector p0(dgFloat32 ( 1.0e30f));
	dgBigVector p1(dgFloat32 (-1.0e30f));

	const dgBigVector* const points = (dgBigVector*) mesh->GetVertexPool();

	dgEdge* ptr = m_face;
	do {
		dgInt32 i = ptr->m_incidentVertex;
		const dgBigVector& p = points[i];
		p0 = p.GetMin(p0);
		p1 = p.GetMax(p1);

		ptr = ptr->m_next;
	} while (ptr != face);

	dgVector padding (dgFloat32 (1.0f / 32.0f));
	SetBox (p0 - padding, p1 + padding);
}

dgMeshEffect::dgMeshBVH::dgMeshBVHNode::dgMeshBVHNode (dgMeshBVHNode* const left, dgMeshBVHNode* const right)
	:m_area(dgFloat32 (0.0f))
	,m_face (NULL)
	,m_userData(NULL)
	,m_left (left)
	,m_right(right)
	,m_parent(NULL)
{
	m_left->m_parent = this;
	m_right->m_parent = this;

//	dgVector p0 (dgMin (left->m_p0.m_x, right->m_p0.m_x), dgMin (left->m_p0.m_y, right->m_p0.m_y), dgMin (left->m_p0.m_z, right->m_p0.m_z), dgFloat32 (0.0f));
//	dgVector p1 (dgMax (left->m_p1.m_x, right->m_p1.m_x), dgMax (left->m_p1.m_y, right->m_p1.m_y), dgMax (left->m_p1.m_z, right->m_p1.m_z), dgFloat32 (0.0f));
	dgVector p0 (left->m_p0.GetMin(right->m_p0));
	dgVector p1 (left->m_p1.GetMax(right->m_p1));
	SetBox(p0, p1);
}


dgMeshEffect::dgMeshBVH::dgMeshBVHNode::~dgMeshBVHNode ()
{
	if (m_left) {
		delete m_left;
	}
	if (m_right) {
		delete m_right;
	}
}

void dgMeshEffect::dgMeshBVH::dgMeshBVHNode::SetBox (const dgVector& p0, const dgVector& p1)
{
	m_p0 = p0 & dgVector::m_triplexMask;
	m_p1 = p1 & dgVector::m_triplexMask;
	dgVector size (dgVector::m_half * (m_p1 - m_p0));
	dgVector size1(size.ShiftTripleLeft());
	dgAssert (size1.m_w == dgFloat32 (0.0f));
	m_area = size.DotProduct(size1).GetScalar();
}

dgMeshEffect::dgMeshBVH::dgMeshBVH (const dgMeshEffect* const mesh)
	:m_mesh(mesh)
	,m_rootNode(NULL)
	,m_fitness(m_mesh->GetAllocator())
{
}


dgMeshEffect::dgMeshBVH::~dgMeshBVH()
{
	Cleanup ();
}

dgMeshEffect* dgMeshEffect::CreateFromSerialization (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData)
{
	return new (allocator) dgMeshEffect(allocator, deserialization, userData);
}

void dgMeshEffect::Serialize (dgSerialize callback, void* const userData) const
{
	dgAssert(0);
/*
	dgInt32 faceCount = 0;
	dgTree<dgEdge*, dgEdge*>filter(GetAllocator());
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if (!filter.Find(face) && (face->m_incidentFace > 0)) {
			faceCount ++;
			dgEdge* edge = face; 
			do {
				filter.Insert(edge, edge);
				edge = edge->m_next;
			} while (edge != face);
		}
	}

	callback (userData, &faceCount, sizeof (dgInt32));
	callback (userData, &m_pointCount, sizeof (dgInt32));
	callback (userData, &m_atribCount, sizeof (dgInt32));
	callback (userData, &m_atribCount, sizeof (dgInt32));

	callback (userData, m_points, m_pointCount * sizeof (dgBigVector));
	callback (userData, m_attrib, m_atribCount * sizeof (dgVertexAtribute));

	filter.RemoveAll();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if (!filter.Find(face) && (face->m_incidentFace > 0)) {
			dgInt32 indices[1024];
			dgInt64 attibuteIndex[1024];
			dgInt32 vertexCount = 0;
			dgEdge* edge = face; 
			do {
				indices[vertexCount] = edge->m_incidentVertex;
				attibuteIndex[vertexCount] = edge->m_userData;
				vertexCount ++;
				filter.Insert(edge, edge);
				edge = edge->m_next;
			} while (edge != face);

			callback (userData, &vertexCount, sizeof (dgInt32));
			callback (userData, indices, vertexCount * sizeof (dgInt32));
			callback (userData, attibuteIndex, vertexCount * sizeof (dgInt64));
		}
	}
*/
}

void dgMeshEffect::dgMeshBVH::Build ()
{
	dgInt32 lru = m_mesh->IncLRU();
/*
	for (void* faceNode = m_mesh->GetFirstFace (); faceNode; faceNode = m_mesh->GetNextFace(faceNode)) {
		if (!m_mesh->IsFaceOpen(faceNode)) {
			dgEdge* const face = &((dgTreeNode*)faceNode)->GetInfo();
			if (face->m_mark != mark) {
				AddFaceNode(face, NULL);
			}
		}
	}
*/
	dgMeshEffect::Iterator iter(*m_mesh);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if (face->m_mark != lru) {
			AddFaceNode(face, NULL);
		}
	}
	ImproveNodeFitness ();
}

void dgMeshEffect::dgMeshBVH::Cleanup ()
{
	if (m_rootNode) {
		delete m_rootNode;
	}
}

dgFloat32 dgMeshEffect::dgMeshBVH::CalculateSurfaceArea (dgMeshBVHNode* const node0, dgMeshBVHNode* const node1, dgVector& minBox, dgVector& maxBox) const
{
	minBox = dgVector (dgMin (node0->m_p0.m_x, node1->m_p0.m_x), dgMin (node0->m_p0.m_y, node1->m_p0.m_y), dgMin (node0->m_p0.m_z, node1->m_p0.m_z), dgFloat32 (0.0f));
	maxBox = dgVector (dgMax (node0->m_p1.m_x, node1->m_p1.m_x), dgMax (node0->m_p1.m_y, node1->m_p1.m_y), dgMax (node0->m_p1.m_z, node1->m_p1.m_z), dgFloat32 (0.0f));		
	dgVector side0 ((maxBox - minBox) * dgVector::m_half);
	dgVector side1 (side0.ShiftTripleLeft());
	dgAssert (side1.m_w == dgFloat32 (0.0f));
	return side0.DotProduct(side1).GetScalar();
}

void dgMeshEffect::dgMeshBVH::ImproveNodeFitness (dgMeshBVHNode* const node)
{
	dgAssert (node->m_left);
	dgAssert (node->m_right);

	if (node->m_parent)	{
		if (node->m_parent->m_left == node) {
			dgFloat32 cost0 = node->m_area;

			dgVector cost1P0;
			dgVector cost1P1;		
			dgFloat32 cost1 = CalculateSurfaceArea (node->m_right, node->m_parent->m_right, cost1P0, cost1P1);

			dgVector cost2P0;
			dgVector cost2P1;		
			dgFloat32 cost2 = CalculateSurfaceArea (node->m_left, node->m_parent->m_right, cost2P0, cost2P1);

			if ((cost1 <= cost0) && (cost1 <= cost2)) {
				dgMeshBVHNode* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				} else {
					m_rootNode = node;
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_right->m_parent = parent;
				parent->m_left = node->m_right;
				node->m_right = parent;
				parent->m_p0 = cost1P0;
				parent->m_p1 = cost1P1;		
				parent->m_area = cost1;


			} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
				dgMeshBVHNode* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				} else {
					m_rootNode = node;
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_left->m_parent = parent;
				parent->m_left = node->m_left;
				node->m_left = parent;

				parent->m_p0 = cost2P0;
				parent->m_p1 = cost2P1;		
				parent->m_area = cost2;
			}
		} else {
			dgFloat32 cost0 = node->m_area;

			dgVector cost1P0;
			dgVector cost1P1;		
			dgFloat32 cost1 = CalculateSurfaceArea (node->m_left, node->m_parent->m_left, cost1P0, cost1P1);

			dgVector cost2P0;
			dgVector cost2P1;		
			dgFloat32 cost2 = CalculateSurfaceArea (node->m_right, node->m_parent->m_left, cost2P0, cost2P1);

			if ((cost1 <= cost0) && (cost1 <= cost2)) {
				dgMeshBVHNode* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				} else {
					m_rootNode = node;
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_left->m_parent = parent;
				parent->m_right = node->m_left;
				node->m_left = parent;

				parent->m_p0 = cost1P0;
				parent->m_p1 = cost1P1;		
				parent->m_area = cost1;

			} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
				dgMeshBVHNode* const parent = node->m_parent;
				node->m_p0 = parent->m_p0;
				node->m_p1 = parent->m_p1;
				node->m_area = parent->m_area; 

				if (parent->m_parent) {
					if (parent->m_parent->m_left == parent) {
						parent->m_parent->m_left = node;
					} else {
						dgAssert (parent->m_parent->m_right == parent);
						parent->m_parent->m_right = node;
					}
				} else {
					m_rootNode = node;
				}
				node->m_parent = parent->m_parent;
				parent->m_parent = node;
				node->m_right->m_parent = parent;
				parent->m_right = node->m_right;
				node->m_right = parent;

				parent->m_p0 = cost2P0;
				parent->m_p1 = cost2P1;		
				parent->m_area = cost2;
			}
		}
	}
	//	dgAssert (SanityCheck());
}


void dgMeshEffect::dgMeshBVH::ImproveNodeFitness ()
{
	dgFloat64 cost0 = m_fitness.TotalCost ();
	dgFloat64 cost1 = cost0;
	do {
		cost0 = cost1;
		dgFitnessList::Iterator iter (m_fitness);
		for (iter.Begin(); iter; iter ++) {
			dgFitnessList::dgTreeNode* const node = iter.GetNode();
			ImproveNodeFitness (node->GetInfo());
		}
		cost1 = m_fitness.TotalCost ();
	} while (cost1 < (dgFloat32 (0.95f)) * cost0);
}

/*
dgMeshEffect::dgMeshBVH::dgMeshBVHNode* dgMeshEffect::dgMeshBVH::CreateLeafNode (dgEdge* const face, void* const userData)
{
	dgMemoryAllocator* const allocator = m_mesh->GetAllocator();
	return new (allocator) dgMeshBVHNode (m_mesh, face, userData);
}
*/

dgMeshEffect::dgMeshBVH::dgMeshBVHNode* dgMeshEffect::dgMeshBVH::AddFaceNode (dgEdge* const face, void* const userData)
{
	dgMemoryAllocator* const allocator = m_mesh->GetAllocator();

	dgMeshBVHNode* const newNode = CreateLeafNode (face, userData);
	if (!m_rootNode) {
		m_rootNode = newNode;
	} else {

		dgVector p0;
		dgVector p1;		
		dgMeshBVHNode* sibling = m_rootNode;

		dgFloat32 surfaceArea = dgMeshBVH::CalculateSurfaceArea (newNode, sibling, p0, p1);
		while(sibling->m_left && sibling->m_right) {

			if (surfaceArea > sibling->m_area) {
				break;
			} 

			sibling->SetBox (p0, p1);

			dgVector leftP0;
			dgVector leftP1;		
			dgFloat32 leftSurfaceArea = CalculateSurfaceArea (newNode, sibling->m_left, leftP0, leftP1);

			dgVector rightP0;
			dgVector rightP1;		
			dgFloat32 rightSurfaceArea = CalculateSurfaceArea (newNode, sibling->m_right, rightP0, rightP1);

			if (leftSurfaceArea < rightSurfaceArea) {
				sibling = sibling->m_left;
				p0 = leftP0;
				p1 = leftP1;
				surfaceArea = leftSurfaceArea;
			} else {
				sibling = sibling->m_right;
				p0 = rightP0;
				p1 = rightP1;
				surfaceArea = rightSurfaceArea;
			}
		} 

		if (!sibling->m_parent) {
			m_rootNode = new (allocator) dgMeshBVHNode (sibling, newNode);
			m_fitness.Insert(m_rootNode, m_rootNode);
		} else {
			dgMeshBVHNode* const parent = sibling->m_parent;
			if (parent->m_left == sibling) {
				dgMeshBVHNode* const node = new (allocator) dgMeshBVHNode (sibling, newNode);
				m_fitness.Insert(node, node);
				parent->m_left = node;
				node->m_parent = parent;
			} else {
				dgAssert (parent->m_right == sibling); 
				dgMeshBVHNode* const node = new (allocator) dgMeshBVHNode (sibling, newNode);
				m_fitness.Insert(node, node);
				parent->m_right = node;
				node->m_parent = parent;
			}
		}
	}

	return newNode;
}


void dgMeshEffect::dgMeshBVH::RemoveNode (dgMeshBVHNode* const treeNode)
{
	if (!treeNode->m_parent) {
		delete (m_rootNode);
		m_rootNode = NULL;
	} else if (!treeNode->m_parent->m_parent) {
		dgMeshBVHNode* const root = m_rootNode;
		if (treeNode->m_parent->m_left == treeNode) {
			m_rootNode = treeNode->m_parent->m_right;
			treeNode->m_parent->m_right = NULL;
		} else {
			dgAssert (treeNode->m_parent->m_right == treeNode);
			m_rootNode = treeNode->m_parent->m_left;
			treeNode->m_parent->m_left= NULL;
		}
		m_rootNode->m_parent = NULL;
		dgAssert (m_fitness.Find(root));
		m_fitness.Remove(root);
		delete (root);

	} else {
		dgMeshBVHNode* const root = treeNode->m_parent->m_parent;
		if (treeNode->m_parent == root->m_left) {
			if (treeNode->m_parent->m_right == treeNode) {
				root->m_left = treeNode->m_parent->m_left;
				treeNode->m_parent->m_left = NULL;
			} else {
				dgAssert (treeNode->m_parent->m_left == treeNode);
				root->m_left = treeNode->m_parent->m_right;
				treeNode->m_parent->m_right = NULL;
			}
			root->m_left->m_parent = root;
		} else {
			if (treeNode->m_parent->m_right == treeNode) {
				root->m_right = treeNode->m_parent->m_left;
				treeNode->m_parent->m_left = NULL;
			} else {
				dgAssert (treeNode->m_parent->m_left == treeNode);
				root->m_right = treeNode->m_parent->m_right;
				treeNode->m_parent->m_right = NULL;
			}
			root->m_right->m_parent = root;
		}

		dgAssert (m_fitness.Find(treeNode->m_parent));
		m_fitness.Remove(treeNode->m_parent);
		delete (treeNode->m_parent);
	}

	//dgAssert (SanityCheck());
}

bool dgMeshEffect::dgMeshBVH::SanityCheck() const
{
	#ifdef _DEBUG
	dgAssert (m_mesh->Sanity ());

	if (!m_rootNode) {
		return false;
	}

	if ((!m_rootNode->m_left && m_rootNode->m_right) || (m_rootNode->m_left && !m_rootNode->m_right)) {
		return false;
	}

	if (m_rootNode->m_left && m_rootNode->m_right) {
		if (m_rootNode->m_left->m_parent != m_rootNode) {
			return false;
		}
		if (m_rootNode->m_right->m_parent != m_rootNode) {
			return false;
		}

		dgMeshBVHNode* stackPool[DG_MESH_EFFECT_BVH_STACK_DEPTH];

		dgInt32 stack = 2;
		stackPool[0] = m_rootNode->m_left;
		stackPool[1] = m_rootNode->m_right;
		while (stack) {
			stack --;
			dgMeshBVHNode* const node = stackPool[stack];

			if ((node->m_parent->m_left != node) && (node->m_parent->m_right != node)){
				return false;
			}

			if (node->m_left) {
				dgAssert (node->m_right);
				stackPool[stack] = node->m_left;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));

				stackPool[stack] = node->m_right;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));
			}
		}
	}
	#endif
	return true;
}

void dgMeshEffect::dgMeshBVH::GetOverlapNodes (dgList<dgMeshBVHNode*>& overlapNodes, const dgBigVector& p0, const dgBigVector& p1) const
{
	dgMeshBVHNode* stackPool[DG_MESH_EFFECT_BVH_STACK_DEPTH];

	dgInt32 stack = 1;
	stackPool[0] = m_rootNode;

	dgVector l0(p0);
	dgVector l1(p1);

	while (stack) {
		stack --;
		dgMeshBVHNode* const me = stackPool[stack];

		if (me && dgOverlapTest (me->m_p0, me->m_p1, l0, l1)) {

			if (!me->m_left) {
				dgAssert (!me->m_right);
				overlapNodes.Append(me);
			} else {
				dgAssert (me->m_left);
				dgAssert (me->m_right);
				stackPool[stack] = me->m_left;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));

				stackPool[stack] = me->m_right;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));
			}
		}
	}
}

/*
dgFloat64 dgMeshEffect::dgMeshBVH::VertexRayCast (const dgBigVector& p0, const dgBigVector& p1) const
{
	dgAssert (0);

	dgMeshBVHNode* stackPool[DG_MESH_EFFECT_BVH_STACK_DEPTH];

	dgInt32 stack = 1;
	stackPool[0] = m_rootNode;

	dgVector l0(p0);
	dgVector l1(p1);
	dgBigVector p1p0 (p1 - p0);
	dgFloat64 den = p1p0 % p1p0;

	const dgBigVector* const points = (dgBigVector*) m_mesh->GetVertexPool();
	while (stack) {
		stack --;
		dgMeshBVHNode* const me = stackPool[stack];

		if (me && dgOverlapTest (me->m_p0, me->m_p1, l0, l1)) {
			if (!me->m_left) {
				dgAssert (!me->m_right);

				dgEdge* ptr = me->m_face;
				do {
					dgInt32 index = ptr->m_incidentVertex;
					const dgBigVector& q0 = points[index];
					dgBigVector q0p0 (q0 - p0);
					dgFloat64 alpha = q0p0 % p1p0;
					if ((alpha > (DG_BOOLEAN_ZERO_TOLERANCE * den)) && (alpha < (den - DG_BOOLEAN_ZERO_TOLERANCE))) {
						dgBigVector dist (p0 + p1p0.Scale (alpha / den) - q0);
						dgFloat64 dist2 = dist % dist;
						if (dist2 < (DG_BOOLEAN_ZERO_TOLERANCE * DG_BOOLEAN_ZERO_TOLERANCE)) {
							return alpha / den;
						}
					}

					ptr = ptr->m_next;
				} while (ptr != me->m_face);
			} else {
				dgAssert (me->m_left);
				dgAssert (me->m_right);
				stackPool[stack] = me->m_left;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));

				stackPool[stack] = me->m_right;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));
			}
		}
	}
	return 1.2f;
}


bool dgMeshEffect::dgMeshBVH::RayRayIntersect (dgEdge* const edge, const dgMeshEffect* const otherMesh, dgEdge* const otherEdge, dgFloat64& param, dgFloat64& otherParam) const
{
	dgAssert (0);

	dgBigVector ray_p0 (m_mesh->m_points[edge->m_incidentVertex]);
	dgBigVector ray_p1 (m_mesh->m_points[edge->m_twin->m_incidentVertex]);

	dgBigVector ray_q0 (otherMesh->m_points[otherEdge->m_incidentVertex]);
	dgBigVector ray_q1 (otherMesh->m_points[otherEdge->m_twin->m_incidentVertex]);

	dgBigVector p1p0 (ray_p1 - ray_p0);
	dgBigVector q1q0 (ray_q1 - ray_q0);
	dgBigVector p0q0 (ray_p0 - ray_q0);

	dgFloat64 a = p1p0 % p1p0;        // always >= 0
	dgFloat64 c = q1q0 % q1q0;        // always >= 0
	dgFloat64 b = p1p0 % q1q0;

	dgFloat64 d = (p1p0 % p0q0);
	dgFloat64 e = (q1q0 % p0q0);
	dgFloat64 den = a * c - b * b;   // always >= 0
	// compute the line parameters of the two closest points
	if (den < DG_BOOLEAN_ZERO_TOLERANCE) { 
		// the lines are almost parallel
		return false;
	} else {         
		// get the closest points on the infinite lines
		dgFloat64 t = b * e - c * d;
		dgFloat64 s = a * e - b * d;

		if (t < (DG_BOOLEAN_ZERO_TOLERANCE * den) || (s < (DG_BOOLEAN_ZERO_TOLERANCE * den)) || (t > (den - DG_BOOLEAN_ZERO_TOLERANCE)) ||  (s > (den - DG_BOOLEAN_ZERO_TOLERANCE))) {
			return false;
		}
		//dgBigVector normal (p1p0 * q1q0);
		dgBigVector r0 (ray_p0 + p1p0.Scale (t / den));
		dgBigVector r1 (ray_q0 + q1q0.Scale (s / den));
		dgBigVector r1r0 (r1 - r0);
		dgFloat64 dist2 = r1r0 % r1r0;
		if (dist2 > (DG_BOOLEAN_ZERO_TOLERANCE * DG_BOOLEAN_ZERO_TOLERANCE)) {
			return false;
		}

		param = t / den;
		otherParam = s / den;
	}
	return true;
}
*/


dgFloat64 dgMeshEffect::dgMeshBVH::RayFaceIntersect (const dgMeshBVHNode* const faceNode, const dgBigVector& p0, const dgBigVector& p1, void* const userData) const
{
	dgAssert (0);
	return 0;
/*
	dgBigVector normal (m_mesh->FaceNormal(faceNode->m_face, m_mesh->GetVertexPool(), sizeof(dgBigVector)));

	dgBigVector diff (p1 - p0);

	dgFloat64 tOut = 2.0f;
	const dgBigVector* const points = (dgBigVector*) m_mesh->GetVertexPool();
	dgFloat64 dir = normal.DotProduct3(diff);
	if (dir < 0.0f) {
		dgEdge* ptr = faceNode->m_face;
		do {
			dgInt32 index0 = ptr->m_incidentVertex;
			dgInt32 index1 = ptr->m_next->m_incidentVertex;
			dgBigVector p0v0 (points[index0] - p0);
			dgBigVector p0v1 (points[index1] - p0);
			dgFloat64 alpha = p0v0.DotProduct3(diff.CrossProduct(p0v1));
			if (alpha <= 0.0f) {
				return 1.2f;
			}

			ptr = ptr->m_next;
		} while (ptr != faceNode->m_face);

		dgInt32 index0 = ptr->m_incidentVertex;
		dgBigVector p0v0 (points[index0] - p0);
		tOut = normal.DotProduct3(p0v0);
		dgFloat64 dist = normal.DotProduct3(diff);
		tOut = tOut / dist;

	} else if (doubleSidedFaces && (dir > 0.0f)) {
		dgEdge* ptr = faceNode->m_face;
		do {
			dgInt32 index0 = ptr->m_incidentVertex;
			dgInt32 index1 = ptr->m_prev->m_incidentVertex;
			dgBigVector p0v0 (points[index0] - p0);
			dgBigVector p0v1 (points[index1] - p0);
			dgFloat64 alpha = p0v0.DotProduct3(diff.CrossProduct(p0v1));
			if (alpha <= 0.0f) {
				return 1.2f;
			}

			ptr = ptr->m_prev;
		} while (ptr != faceNode->m_face);

		dgInt32 index0 = ptr->m_incidentVertex;
		dgBigVector p0v0 (points[index0] - p0);
		tOut = normal.DotProduct3(p0v0);
		dgFloat64 dist = normal.DotProduct3(diff);
		tOut = tOut / dist;
	}

	if (tOut < 1.e-12f) {
		tOut = 2.0f;
	} else if (tOut > (1.0 - 1.e-12f)) {
		tOut = 2.0f;
	}
	return tOut;
*/
}


void dgMeshEffect::dgMeshBVH::FaceRayCast (const dgBigVector& p0, const dgBigVector& p1, void* const userData) const
{
	dgMeshBVHNode* stackPool[DG_MESH_EFFECT_BVH_STACK_DEPTH];

	dgInt32 stack = 1;
	dgMeshBVHNode* node = NULL;

	stackPool[0] = m_rootNode;
	dgFloat64 maxParam = dgFloat32 (1.2f);

	dgVector l0(p0);
	dgVector l1(p1);
	l0 = l0 & dgVector::m_triplexMask;
	l1 = l1 & dgVector::m_triplexMask;
	dgFastRayTest ray (l0, l1);
	while (stack) {
		stack --;
		dgMeshBVHNode* const me = stackPool[stack];
		if (me && ray.BoxTest (me->m_p0, me->m_p1)) {
			if (!me->m_left) {
				dgAssert (!me->m_right);
				dgFloat64 param = RayFaceIntersect (me, p0, p1, userData);
				if (param < dgFloat64 (0.0f)) {
					break;
				}
				if (param < maxParam) {
					node = me;
					maxParam = param;
				}
			} else {
				dgAssert (me->m_left);
				dgAssert (me->m_right);
				stackPool[stack] = me->m_left;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));

				stackPool[stack] = me->m_right;
				stack++;
				dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgMeshBVHNode*)));
			}
		}
	}
}


dgMeshEffect::dgMeshEffect ()
	:dgPolyhedra(NULL)
	,m_points(NULL)
	,m_attrib(NULL)
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	Init();
}

dgMeshEffect::dgMeshEffect(dgMemoryAllocator* const allocator)
	:dgPolyhedra(allocator)
	,m_points(allocator)
	,m_attrib(allocator)
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	Init();
}

dgMeshEffect::dgMeshEffect (dgMemoryAllocator* const allocator, const dgMatrix& planeMatrix, dgFloat32 witdth, dgFloat32 breadth, dgInt32 material, const dgMatrix& textureMatrix0, const dgMatrix& textureMatrix1)
	:dgPolyhedra(allocator)
	,m_points(allocator)
	,m_attrib(allocator)
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	dgAssert (0);
	Init();
/*
	dgInt32 index[4];
	dgInt64 attrIndex[4];
	dgBigVector face[4];

//	Init();

	face[0] = dgBigVector (dgFloat32 (0.0f), -witdth, -breadth, dgFloat32 (0.0f));
	face[1] = dgBigVector (dgFloat32 (0.0f),  witdth, -breadth, dgFloat32 (0.0f));
	face[2] = dgBigVector (dgFloat32 (0.0f),  witdth,  breadth, dgFloat32 (0.0f));
	face[3] = dgBigVector (dgFloat32 (0.0f), -witdth,  breadth, dgFloat32 (0.0f));

	for (dgInt32 i = 0; i < 4; i ++) {
		dgBigVector uv0 (textureMatrix0.TransformVector(face[i]));
		dgBigVector uv1 (textureMatrix1.TransformVector(face[i]));

		m_points[i] = planeMatrix.TransformVector(face[i]);

		m_attrib[i].m_vertex.m_x = m_points[i].m_x;
		m_attrib[i].m_vertex.m_y = m_points[i].m_y;
		m_attrib[i].m_vertex.m_z = m_points[i].m_z;
		m_attrib[i].m_vertex.m_w = dgFloat64 (0.0f);

		m_attrib[i].m_normal_x = planeMatrix.m_front.m_x;
		m_attrib[i].m_normal_y = planeMatrix.m_front.m_y;
		m_attrib[i].m_normal_z = planeMatrix.m_front.m_z;
		
		m_attrib[i].m_u0 = uv0.m_y;
		m_attrib[i].m_v0 = uv0.m_z;

		m_attrib[i].m_u1 = uv1.m_y;
		m_attrib[i].m_v1 = uv1.m_z;

		m_attrib[i].m_material = material;

		index[i] = i;
		attrIndex[i] = i;
	}

	m_pointCount = 4;
	m_atribCount = 4;
	BeginFace();
	AddFace (4, index, attrIndex);
	EndFace();
*/
}


dgMeshEffect::dgMeshEffect(dgPolyhedra& mesh, const dgMeshEffect& source)
	:dgPolyhedra (mesh) 
	,m_points(source.m_points)
	,m_attrib(source.m_attrib)
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	Init();
}

dgMeshEffect::dgMeshEffect(const dgMeshEffect& source)
	:dgPolyhedra (source) 
	,m_points(source.m_points)
	,m_attrib(source.m_attrib)
	, m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	Init();
}

dgMeshEffect::dgMeshEffect(dgCollisionInstance* const collision)
	:dgPolyhedra (collision->GetAllocator()) 
	,m_points(collision->GetAllocator())
	,m_attrib(collision->GetAllocator())
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	class dgMeshEffectBuilder
	{
		public:
		dgMeshEffectBuilder (dgMemoryAllocator* const allocator)
			:m_brush(0)
			,m_faceCount(0)
			,m_vertexCount(0)
			,m_vertex(allocator)
			,m_faceIndexCount(allocator)
		{
		}

		static void GetShapeFromCollision (void* userData, dgInt32 vertexCount, const dgFloat32* faceVertex, dgInt32 id)
		{
			dgInt32 vertexIndex; 
			dgMeshEffectBuilder& builder = *((dgMeshEffectBuilder*)userData);

			builder.m_faceIndexCount[builder.m_faceCount] = vertexCount;
			builder.m_faceCount = builder.m_faceCount + 1;

			vertexIndex = builder.m_vertexCount; 
			dgFloat64 brush = dgFloat64 (builder.m_brush);
			for (dgInt32 i = 0; i < vertexCount; i ++) {
				builder.m_vertex[vertexIndex] = dgBigVector (faceVertex[i * 3 + 0], faceVertex[i * 3 + 1], faceVertex[i * 3 + 2], brush);
				vertexIndex ++;
			}

			builder.m_vertexCount = vertexIndex;
		}

		dgArray<dgBigVector> m_vertex;
		dgArray<dgInt32> m_faceIndexCount;
		dgInt32 m_brush;
		dgInt32 m_faceCount;
		dgInt32 m_vertexCount;
	};
	dgMeshEffectBuilder builder(GetAllocator());

	Init();
	if (collision->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionInfo collisionInfo;
		collision->GetCollisionInfo (&collisionInfo);

		dgInt32 brush = 0;
		dgMatrix matrix (collisionInfo.m_offsetMatrix);
		dgCollisionCompound* const compoundCollision = (dgCollisionCompound*) collision->GetChildShape();
		for (dgTree<dgCollisionCompound::dgNodeBase*, dgInt32>::dgTreeNode* node = compoundCollision->GetFirstNode(); node; node = compoundCollision->GetNextNode(node)) {
			builder.m_brush = brush;
			brush ++;
			dgCollisionInstance* const childShape = compoundCollision->GetCollisionFromNode(node);
			childShape->DebugCollision (matrix, (dgCollision::OnDebugCollisionMeshCallback) dgMeshEffectBuilder::GetShapeFromCollision, &builder);
		}

	} else {
		dgMatrix matrix (dgGetIdentityMatrix());
		collision->DebugCollision (matrix, (dgCollision::OnDebugCollisionMeshCallback) dgMeshEffectBuilder::GetShapeFromCollision, &builder);
	}

	dgStack<dgInt32>indexListBuffer (builder.m_vertexCount);
	dgInt32* const indexList = &indexListBuffer[0];
	dgVertexListToIndexList (&builder.m_vertex[0].m_x, sizeof (dgBigVector), 4, builder.m_vertexCount, &indexList[0], DG_VERTEXLIST_INDEXLIST_TOL);	
	
	dgMeshVertexFormat vertexFormat;

	vertexFormat.m_faceCount = builder.m_faceCount;
	vertexFormat.m_faceIndexCount = &builder.m_faceIndexCount[0];
//	vertexFormat.m_faceMaterial = materialIndex;

	vertexFormat.m_vertex.m_data = &builder.m_vertex[0].m_x;
	vertexFormat.m_vertex.m_strideInBytes = sizeof (dgBigVector);
	vertexFormat.m_vertex.m_indexList = &indexList[0];

	BuildFromIndexList (&vertexFormat);

    RepairTJoints();
	CalculateNormals(dgFloat32 (45.0f * dgDEG2RAD));
}

// create a convex hull
dgMeshEffect::dgMeshEffect(dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 count, dgInt32 strideInByte, dgFloat64 distTol)
	:dgPolyhedra(allocator)
	,m_points(allocator)
	,m_attrib(allocator)
{
	Init();
	if (count >= 4) {
		dgConvexHull3d convexHull(allocator, vertexCloud, strideInByte, count, distTol);
		if (convexHull.GetCount()) {
			dgStack<dgInt32> faceCountPool(convexHull.GetCount());
			dgStack<dgInt32> vertexIndexListPool(convexHull.GetCount() * 3);

			dgInt32 index = 0;
			dgMeshVertexFormat format;
			format.m_faceCount = convexHull.GetCount();
			format.m_faceIndexCount = &faceCountPool[0];
			format.m_vertex.m_indexList = &vertexIndexListPool[0];
			format.m_vertex.m_data = &convexHull.GetVertexPool()[0].m_x;
			format.m_vertex.m_strideInBytes = sizeof (dgBigVector);
			for (dgConvexHull3d::dgListNode* faceNode = convexHull.GetFirst(); faceNode; faceNode = faceNode->GetNext()) {
				dgConvexHull3DFace& face = faceNode->GetInfo();
				faceCountPool[index] = 3;
				vertexIndexListPool[index * 3 + 0] = face.m_index[0];
				vertexIndexListPool[index * 3 + 1] = face.m_index[1];
				vertexIndexListPool[index * 3 + 2] = face.m_index[2];
				index++;
			}
			BuildFromIndexList(&format);
			RepairTJoints();
		}
	}
}


dgMeshEffect::dgMeshEffect (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData)
	:dgPolyhedra (allocator) 
	,m_points(allocator)
	,m_attrib(allocator)
	,m_vertexBaseCount(-1)
	,m_constructionIndex(0)
{
	Init();
dgAssert (0);
/*
	dgInt32 faceCount;
	deserialization (userData, &faceCount, sizeof (dgInt32));
	deserialization (userData, &m_pointCount, sizeof (dgInt32));
	deserialization (userData, &m_atribCount, sizeof (dgInt32));
	deserialization (userData, &m_atribCount, sizeof (dgInt32));

	m_maxPointCount = m_pointCount;
	m_maxAtribCount = m_atribCount;

	m_points = (dgBigVector*) GetAllocator()->MallocLow(dgInt32 (m_pointCount * sizeof(dgBigVector)));
	m_attrib = (dgVertexAtribute*) GetAllocator()->MallocLow(dgInt32 (m_atribCount * sizeof(dgVertexAtribute)));

	deserialization (userData, m_points, m_pointCount * sizeof (dgBigVector));
	deserialization (userData, m_attrib, m_atribCount * sizeof (dgVertexAtribute));

	BeginFace();
	for (dgInt32 i = 0; i < faceCount; i ++) {
		dgInt32 vertexCount;
		dgInt32 face[1024];
		dgInt64 attrib[1024];
		deserialization (userData, &vertexCount, sizeof (dgInt32));
		deserialization (userData, face, vertexCount * sizeof (dgInt32));
		deserialization (userData, attrib, vertexCount * sizeof (dgInt64));
		AddFace (vertexCount, face, attrib);
	}
	EndFace();
*/
}


dgMeshEffect::~dgMeshEffect(void)
{
}

void dgMeshEffect::Init()
{
//	dgAssert ();
}

void dgMeshEffect::BeginFace()
{
	dgPolyhedra::BeginFace();
}

bool dgMeshEffect::EndFace ()
{
	dgPolyhedra::EndFace();
	bool state = false;
	for (bool hasVertexCollision = true; hasVertexCollision;) {
		hasVertexCollision = false;
		const dgInt32 currentCount = m_points.m_vertex.m_count;
		dgStack<dgInt8> verterCollisionBuffer (currentCount);
		dgInt8* const verterCollision = &verterCollisionBuffer[0];
		memset (&verterCollision[0], 0, verterCollisionBuffer.GetSizeInBytes());

		Iterator iter (*this);
		dgInt32 mark = IncLRU();
		dgList<dgTreeNode*> collisionFound(GetAllocator());
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if (edge->m_mark != mark) {
				if ((edge->m_incidentVertex < currentCount) && (verterCollision[edge->m_incidentVertex] == 0)) {
					verterCollision[edge->m_incidentVertex] = 1;
				} else {
					hasVertexCollision = true;
					collisionFound.Append(iter.GetNode());
				}
				dgEdge* ptr = edge;
				do {
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
			}
		}

		dgAssert (!collisionFound.GetFirst() || Sanity());
		for (dgList<dgTreeNode*>::dgListNode* node = collisionFound.GetFirst(); node; node = node->GetNext()) {
			state = true;
			dgEdge* const edge = &node->GetInfo()->GetInfo();

			// this is a vertex collision
			m_points.m_vertex.PushBack (m_points.m_vertex[edge->m_incidentVertex]);
			if (m_points.m_layers.m_count) {
				m_points.m_layers.PushBack (m_points.m_layers[edge->m_incidentVertex]);
			}

			dgEdge* ptr = edge;
			do {
				if (ptr->m_incidentFace > 0) {
					//m_attrib.m_pointChannel[dgInt32 (ptr->m_userData)] = m_points.m_vertex.m_count - 1;
					dgInt32 index = dgInt32 (ptr->m_userData);
					m_attrib.m_pointChannel.PushBack(m_points.m_vertex.m_count - 1);
					if (m_attrib.m_materialChannel.m_count) {
						m_attrib.m_materialChannel.PushBack(m_attrib.m_materialChannel[index]);
					}
					if (m_attrib.m_normalChannel.m_count) {
						m_attrib.m_normalChannel.PushBack(m_attrib.m_normalChannel[index]);
					}
					if (m_attrib.m_binormalChannel.m_count) {
						m_attrib.m_binormalChannel.PushBack(m_attrib.m_binormalChannel[index]);
					}
					if (m_attrib.m_colorChannel.m_count) {
						m_attrib.m_colorChannel.PushBack(m_attrib.m_colorChannel[index]);
					}
					if (m_attrib.m_uv0Channel.m_count) {
						m_attrib.m_uv0Channel.PushBack(m_attrib.m_uv0Channel[index]);
					}
					if (m_attrib.m_uv1Channel.m_count) {
						m_attrib.m_uv1Channel.PushBack(m_attrib.m_uv1Channel[index]);
					}
					ptr->m_userData = m_attrib.m_pointChannel.m_count - 1;
				}

				dgTreeNode* const edgeNode = GetNodeFromInfo (*ptr);
				dgPairKey edgeKey (ptr->m_incidentVertex, ptr->m_twin->m_incidentVertex);
				ReplaceKey (edgeNode, edgeKey.GetVal());

				dgTreeNode* const twinNode = GetNodeFromInfo (*(ptr->m_twin));
				dgPairKey twinKey (ptr->m_twin->m_incidentVertex, ptr->m_incidentVertex);
				ReplaceKey (twinNode, twinKey.GetVal());

				ptr->m_incidentVertex = m_points.m_vertex.m_count - 1;

				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
		}
		dgAssert (!collisionFound.GetFirst() || Sanity());
	}

	return !state;
}


void dgMeshEffect::Trace () const
{
dgAssert (0);
/*
	for (dgInt32 i = 0; i < m_pointCount; i ++ ) {
		dgTrace (("%d-> %f %f %f\n", i, m_points[i].m_x, m_points[i].m_y, m_points[i].m_z));
	}


	dgTree<dgEdge*, dgEdge*>filter(GetAllocator());
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		if (!filter.Find(edge)) {
			dgEdge* ptr = edge;
			do {
				filter.Insert(edge, ptr);
				dgTrace (("%d ", ptr->m_incidentVertex));
				ptr = ptr->m_next;
			} while (ptr != edge);
			if (edge->m_incidentFace <= 0) {
				dgTrace (("open"));
			}
			dgTrace (("\n"));
		}
	}
	dgTrace (("\n"));
*/
};


void dgMeshEffect::SaveOFF (const char* const fileName) const
{
	FILE* const file = fopen (fileName, "wb");

	fprintf (file, "OFF\n");

	dgInt32 faceCount = 0;
	dgTree<dgEdge*, dgEdge*>filter(GetAllocator());
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if (!filter.Find(face) && (face->m_incidentFace > 0)) {
			faceCount ++;
			dgEdge* edge = face; 
			do {
				filter.Insert(edge, edge);
				edge = edge->m_next;
			} while (edge != face);
		}
	}
	fprintf (file, "%d %d 0\n", m_points.m_vertex.m_count, faceCount);

	for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i ++) {
		fprintf (file, "%f %f %f\n", m_points.m_vertex[i].m_x, m_points.m_vertex[i].m_y, m_points.m_vertex[i].m_z);
	}

	filter.RemoveAll();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if (!filter.Find(face) && (face->m_incidentFace > 0)) {
			dgInt32 indices[1024];
			dgInt32 vertexCount = 0;
			dgEdge* edge = face; 
			do {
				indices[vertexCount] = edge->m_incidentVertex;
				vertexCount ++;
				filter.Insert(edge, edge);
				edge = edge->m_next;
			} while (edge != face);

			fprintf (file, "%d", vertexCount);
			for (dgInt32 j = 0; j < vertexCount; j ++) {
				fprintf (file, " %d", indices[j]);
			}
			fprintf (file, "\n");
		}
	}
	fclose (file);
}

void dgMeshEffect::Triangulate  ()
{
/*
	dgInt32	index[DG_MESH_EFFECT_POINT_SPLITED];
	dgInt64	userData[DG_MESH_EFFECT_POINT_SPLITED];
	dgPolyhedra polygon(GetAllocator());
	polygon.BeginFace();
	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter1 (*this);
	for (iter1.Begin(); iter1; iter1 ++) {
		dgEdge* const face = &iter1.GetNode()->GetInfo();
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			dgEdge* ptr = face;
			dgInt32 indexCount = 0;
			do {
				index[indexCount] = dgInt32 (ptr->m_userData);
				userData[indexCount] = ptr->m_incidentVertex;
				ptr->m_mark = mark;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			polygon.AddFace(indexCount, index, userData);
		}
	}
	polygon.EndFace();

	dgPolyhedra leftOversOut(GetAllocator());
	polygon.Triangulate(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), &leftOversOut);
	dgAssert (leftOversOut.GetCount() == 0);

	SetLRU (0);
	RemoveAll();
	BeginFace();
	mark = polygon.IncLRU();
	dgPolyhedra::Iterator iter (polygon);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &iter.GetNode()->GetInfo();
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			dgEdge* ptr = face;
			dgInt32 indexCount = 0;
			do {
				ptr->m_mark = mark;
				index[indexCount] = dgInt32 (ptr->m_userData);
				userData[indexCount] = ptr->m_incidentVertex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			AddFace(indexCount, index, userData);
		}
	}
	EndFace();
*/
	UnpackPoints();
	dgPolyhedra leftOversOut(GetAllocator());
	dgPolyhedra::Triangulate(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), &leftOversOut);
	dgAssert(leftOversOut.GetCount() == 0);

	dgPolyhedra::Iterator iter(*this);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		edge->m_userData = (edge->m_incidentFace) > 0 ? edge->m_incidentVertex : 0;
	}
	PackPoints(dgFloat32(1.0e-24f));

	RepairTJoints ();
	dgAssert (Sanity ());
}

void dgMeshEffect::ConvertToPolygons ()
{
	UnpackPoints();
	dgPolyhedra leftOversOut(GetAllocator());
	dgPolyhedra::ConvexPartition(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), &leftOversOut);
	dgAssert(leftOversOut.GetCount() == 0);

	dgPolyhedra::Iterator iter(*this);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		edge->m_userData = (edge->m_incidentFace) > 0 ? edge->m_incidentVertex : 0;
	}
	PackPoints(dgFloat32 (1.0e-24f));

	RepairTJoints ();
	dgAssert (Sanity ());
}

void dgMeshEffect::RemoveUnusedVertices(dgInt32* const vertexMapResult)
{
	dgAssert (!vertexMapResult);
	UnpackAttibuteData();
	PackAttibuteData();
	UnpackPoints();
	PackPoints(dgFloat32 (1.0e-24f));
}


void dgMeshEffect::ApplyTransform (const dgMatrix& matrix)
{
	matrix.TransformTriplex(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), &m_points.m_vertex[0].m_x, sizeof (dgBigVector), m_points.m_vertex.m_count);

	dgMatrix invMatix(matrix.Inverse4x4());
	invMatix.m_posit = dgVector::m_wOne;
	dgMatrix rotation (invMatix.Transpose4X4());
	for (dgInt32 i = 0; i < m_attrib.m_normalChannel.m_count; i ++) {
		dgVector n (dgFloat32 (m_attrib.m_normalChannel[i].m_x), dgFloat32 (m_attrib.m_normalChannel[i].m_y), dgFloat32 (m_attrib.m_normalChannel[i].m_z), dgFloat32 (0.0f));
		n = rotation.RotateVector(n);
		dgAssert(n.m_w == dgFloat32(0.0f));
		dgAssert (n.DotProduct(n).GetScalar() > dgFloat32 (0.0f));
		n = n.Normalize(); 
		m_attrib.m_normalChannel[i].m_x = n.m_x;
		m_attrib.m_normalChannel[i].m_y = n.m_y;
		m_attrib.m_normalChannel[i].m_z = n.m_z;
	}

	for (dgInt32 i = 0; i < m_attrib.m_binormalChannel.m_count; i++) {
		dgVector n(dgFloat32(m_attrib.m_binormalChannel[i].m_x), dgFloat32(m_attrib.m_binormalChannel[i].m_y), dgFloat32(m_attrib.m_binormalChannel[i].m_z), dgFloat32(0.0f));
		n = rotation.RotateVector(n);
		dgAssert(n.m_w == dgFloat32(0.0f));
		dgAssert(n.DotProduct(n).GetScalar() > dgFloat32(0.0f));
		n = n.Normalize();
		m_attrib.m_binormalChannel[i].m_x = n.m_x;
		m_attrib.m_binormalChannel[i].m_y = n.m_y;
		m_attrib.m_binormalChannel[i].m_z = n.m_z;
	}
}

dgMatrix dgMeshEffect::CalculateOOBB (dgBigVector& size) const
{
	dgObb sphere (CalculateSphere (&m_points.m_vertex[0].m_x, sizeof (dgBigVector), NULL));
	size = sphere.m_size;
	size.m_w = 0.0f;

//	dgMatrix permuation (dgGetIdentityMatrix());
//	permuation[0][0] = dgFloat32 (0.0f);
//	permuation[0][1] = dgFloat32 (1.0f);
//	permuation[1][1] = dgFloat32 (0.0f);
//	permuation[1][2] = dgFloat32 (1.0f);
//	permuation[2][2] = dgFloat32 (0.0f);
//	permuation[2][0] = dgFloat32 (1.0f);
//	while ((size.m_x < size.m_y) || (size.m_x < size.m_z)) {
//		sphere = permuation * sphere;
//		size = permuation.UnrotateVector(size);
//	}

	return sphere;
}

void dgMeshEffect::CalculateAABB (dgBigVector& minBox, dgBigVector& maxBox) const
{
	dgBigVector minP ( dgFloat64 (1.0e15f),  dgFloat64 (1.0e15f),  dgFloat64 (1.0e15f), dgFloat64 (0.0f)); 
	dgBigVector maxP (-dgFloat64 (1.0e15f), -dgFloat64 (1.0e15f), -dgFloat64 (1.0e15f), dgFloat64 (0.0f)); 

	dgPolyhedra::Iterator iter (*this);
	const dgBigVector* const points = &m_points.m_vertex[0];
	for (iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		const dgBigVector& p (points[edge->m_incidentVertex]);

		minP.m_x = dgMin (p.m_x, minP.m_x); 
		minP.m_y = dgMin (p.m_y, minP.m_y); 
		minP.m_z = dgMin (p.m_z, minP.m_z); 

		maxP.m_x = dgMax (p.m_x, maxP.m_x); 
		maxP.m_y = dgMax (p.m_y, maxP.m_y); 
		maxP.m_z = dgMax (p.m_z, maxP.m_z); 
	}

	minBox = minP;
	maxBox = maxP;
}


void dgMeshEffect::BeginBuild ()
{
	m_points.Clear();
	m_attrib.Clear();
	RemoveAll();
	BeginFace();
	m_vertexBaseCount = -1;
	m_constructionIndex = 0;
}


void dgMeshEffect::BeginBuildFace ()
{
	m_constructionIndex = m_points.m_vertex.m_count;
}

void dgMeshEffect::AddPoint (dgFloat64 x, dgFloat64 y, dgFloat64 z)
{
	m_attrib.m_pointChannel.PushBack(m_points.m_vertex.m_count);
	m_points.m_vertex.PushBack(dgBigVector (QuantizeCordinade(x), QuantizeCordinade(y), QuantizeCordinade(z), dgFloat64(0.0f)));
}

void dgMeshEffect::AddLayer(dgInt32 layer)
{
	m_points.m_layers.PushBack(layer);
}

void dgMeshEffect::AddVertexColor(dgFloat32 x, dgFloat32 y, dgFloat32 z, dgFloat32 w)
{
	m_attrib.m_colorChannel.PushBack(dgVector (x, y, z, w));
}

void dgMeshEffect::AddNormal(dgFloat32 x, dgFloat32 y, dgFloat32 z)
{
	dgTriplex n;
	n.m_x = x;
	n.m_y = y;
	n.m_z = z;
	m_attrib.m_normalChannel.PushBack(n);
}

void dgMeshEffect::AddBinormal(dgFloat32 x, dgFloat32 y, dgFloat32 z)
{
	dgTriplex n;
	n.m_x = x;
	n.m_y = y;
	n.m_z = z;
	m_attrib.m_binormalChannel.PushBack(n);
}

void dgMeshEffect::AddUV0(dgFloat32 u, dgFloat32 v)
{
	dgAttibutFormat::dgUV uv;
	uv.m_u = u;
	uv.m_v = v;
	m_attrib.m_uv0Channel.PushBack(uv);
}

void dgMeshEffect::AddUV1(dgFloat32 u, dgFloat32 v)
{
	dgAttibutFormat::dgUV uv;
	uv.m_u = u;
	uv.m_v = v;
	m_attrib.m_uv1Channel.PushBack(uv);
}

void dgMeshEffect::AddMaterial (dgInt32 materialIndex)
{
	m_attrib.m_materialChannel.PushBack(materialIndex);
}


void dgMeshEffect::EndBuildFace ()
{
	dgInt32 count = m_points.m_vertex.m_count - m_constructionIndex;
	if (count > 3) {
		dgInt32 indexList[256];

		dgAssert(count < dgInt32(sizeof (indexList) / sizeof(indexList[0])));
		dgPolyhedra polygon(GetAllocator());
		
		dgPointFormat points(GetAllocator());
		dgAttibutFormat attibutes(GetAllocator());
		for (dgInt32 i = 0; i < count; i++) {
			indexList[i] = i;

			points.m_vertex.PushBack(m_points.m_vertex[m_constructionIndex + i]);
			if (m_points.m_layers.m_count) {
				points.m_layers.PushBack(m_points.m_layers[m_constructionIndex + i]);
			}

			if (m_attrib.m_materialChannel.m_count) {
				attibutes.m_materialChannel.PushBack(m_attrib.m_materialChannel[m_constructionIndex + i]);
			}

			if (m_attrib.m_normalChannel.m_count) {
				attibutes.m_normalChannel.PushBack(m_attrib.m_normalChannel[m_constructionIndex + i]);
			}

			if (m_attrib.m_binormalChannel.m_count) {
				attibutes.m_binormalChannel.PushBack(m_attrib.m_binormalChannel[m_constructionIndex + i]);
			}

			if (m_attrib.m_binormalChannel.m_count) {
				attibutes.m_colorChannel.PushBack(m_attrib.m_colorChannel[m_constructionIndex + i]);
			}

			if (m_attrib.m_uv0Channel.m_count) {
				attibutes.m_uv0Channel.PushBack(m_attrib.m_uv0Channel[m_constructionIndex + i]);
			}

			if (attibutes.m_uv1Channel.m_count) {
				attibutes.m_uv1Channel.PushBack(attibutes.m_uv1Channel[m_constructionIndex + i]);
			}
		}

		polygon.BeginFace();
		polygon.AddFace(count, indexList, NULL);
		polygon.EndFace();
		polygon.Triangulate(&points.m_vertex[0].m_x, sizeof (dgBigVector), NULL);

		m_points.SetCount (m_constructionIndex); 
		m_attrib.SetCount (m_constructionIndex);
		dgInt32 mark = polygon.IncLRU();
		dgPolyhedra::Iterator iter(polygon);
		for (iter.Begin(); iter; iter++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark < mark)) {
				dgInt32 i0 = edge->m_incidentVertex;
				dgInt32 i1 = edge->m_next->m_incidentVertex;
				dgInt32 i2 = edge->m_next->m_next->m_incidentVertex;
				edge->m_mark = mark;
				edge->m_next->m_mark = mark;
				edge->m_next->m_next->m_mark = mark;

				const dgBigVector& p0 = points.m_vertex[i0];
				const dgBigVector& p1 = points.m_vertex[i1];
				const dgBigVector& p2 = points.m_vertex[i2];

				dgBigVector e1(p1 - p0);
				dgBigVector e2(p2 - p0);
				dgBigVector n(e1.CrossProduct(e2));
				dgFloat64 mag3 = n.DotProduct3(n);
				if (mag3 >= dgFloat64(DG_MESH_EFFECT_PRECISION_SCALE_INV * DG_MESH_EFFECT_PRECISION_SCALE_INV)) {
					dgInt32 index[] = {i0, i1, i2};
					for (dgInt32 i = 0; i < 3; i ++) {
						dgInt32 j = index[i];
						AddPoint(points.m_vertex[j].m_x, points.m_vertex[j].m_y, points.m_vertex[j].m_z);
						if (points.m_layers.m_count) {
							AddLayer(points.m_layers[j]);
						}

						if (attibutes.m_materialChannel.m_count) {
							AddMaterial(attibutes.m_materialChannel[j]);
						}

						if (attibutes.m_normalChannel.m_count) {
							AddNormal(attibutes.m_normalChannel[j].m_x, attibutes.m_normalChannel[j].m_y, attibutes.m_normalChannel[j].m_z);
						}

						if (attibutes.m_binormalChannel.m_count) {
							AddBinormal(attibutes.m_binormalChannel[j].m_x, attibutes.m_binormalChannel[j].m_y, attibutes.m_binormalChannel[j].m_z);
						}

						if (attibutes.m_colorChannel.m_count) {
							AddVertexColor(attibutes.m_colorChannel[j].m_x, attibutes.m_colorChannel[j].m_y, attibutes.m_colorChannel[j].m_z, attibutes.m_colorChannel[j].m_w);
						}

						if (attibutes.m_uv0Channel.m_count) {
							AddUV0(attibutes.m_uv0Channel[j].m_u, attibutes.m_uv0Channel[j].m_v);
						}

						if (attibutes.m_uv1Channel.m_count) {
							AddUV1(attibutes.m_uv1Channel[j].m_u, attibutes.m_uv1Channel[j].m_v);
						}
					}
				}
			}
		}

	} else {
		const dgBigVector& p0 = m_points.m_vertex[m_constructionIndex + 0];
		const dgBigVector& p1 = m_points.m_vertex[m_constructionIndex + 1];
		const dgBigVector& p2 = m_points.m_vertex[m_constructionIndex + 2];

		dgBigVector e1(p1 - p0);
		dgBigVector e2(p2 - p0);
		dgBigVector n(e1.CrossProduct(e2));
		dgFloat64 mag3 = n.DotProduct3(n);
		if (mag3 < dgFloat64(DG_MESH_EFFECT_PRECISION_SCALE_INV * DG_MESH_EFFECT_PRECISION_SCALE_INV)) {
			m_attrib.SetCount (m_constructionIndex);
			m_points.SetCount (m_constructionIndex);
		}
	}
}


void dgMeshEffect::UnpackAttibuteData()
{
	dgAttibutFormat attibutes(m_attrib);
	m_attrib.Clear();

	Iterator iter(*this);
	dgInt32 attributeCount = 0;
	const dgInt32 lru = IncLRU();
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		if ((edge->m_incidentFace > 0) && (edge->m_mark != lru)) {
			dgEdge* ptr = edge;

			ptr = edge;
			do {
				ptr->m_mark = lru;
				m_attrib.m_pointChannel.PushBack(ptr->m_incidentVertex);

				if (attibutes.m_materialChannel.m_count) {
					m_attrib.m_materialChannel.PushBack(attibutes.m_materialChannel[dgInt32(ptr->m_userData)]);
				}

				if (attibutes.m_normalChannel.m_count) {
					m_attrib.m_normalChannel.PushBack(attibutes.m_normalChannel[dgInt32(ptr->m_userData)]);
				}

				if (attibutes.m_binormalChannel.m_count) {
					m_attrib.m_binormalChannel.PushBack(attibutes.m_binormalChannel[dgInt32(ptr->m_userData)]);
				}

				if (attibutes.m_colorChannel.m_count) {
					m_attrib.m_colorChannel.PushBack(attibutes.m_colorChannel[dgInt32(ptr->m_userData)]);
				}

				if (attibutes.m_uv0Channel.m_count) {
					m_attrib.m_uv0Channel.PushBack(attibutes.m_uv0Channel[dgInt32(ptr->m_userData)]);
				}

				if (attibutes.m_uv1Channel.m_count) {
					m_attrib.m_uv1Channel.PushBack(attibutes.m_uv1Channel[dgInt32(ptr->m_userData)]);
				}

				ptr->m_userData = attributeCount;
				attributeCount++;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
	dgAssert(m_attrib.m_pointChannel.m_count == attributeCount);
}

void dgMeshEffect::PackAttibuteData()
{
	dgStack<dgInt32>attrIndexBuffer(m_attrib.m_pointChannel.m_count);
	dgInt32* const attrIndexMap = &attrIndexBuffer[0];
	m_attrib.CompressData(m_points, &attrIndexMap[0]);

	Iterator iter(*this);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace > 0) {
			edge->m_userData = attrIndexMap[edge->m_userData];
		}
	}

	memset (attrIndexMap, -1, sizeof (dgInt32) * m_attrib.m_pointChannel.m_count);
	dgAttibutFormat tmpFormat (m_attrib);
	m_attrib.Clear();

	dgInt32 remapIndex = 0;
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace > 0)  {
			dgInt32 index = dgInt32(edge->m_userData);
			if (attrIndexMap[edge->m_userData] == -1) {
				attrIndexMap[index] = remapIndex;
				remapIndex ++;

				m_attrib.m_pointChannel.PushBack(tmpFormat.m_pointChannel[index]);
				if (tmpFormat.m_materialChannel.m_count) {
					m_attrib.m_materialChannel.PushBack(tmpFormat.m_materialChannel[index]);
				}
				if (tmpFormat.m_normalChannel.m_count) {
					m_attrib.m_normalChannel.PushBack(tmpFormat.m_normalChannel[index]);
				}
				if (tmpFormat.m_binormalChannel.m_count) {
					m_attrib.m_binormalChannel.PushBack(tmpFormat.m_binormalChannel[index]);
				}
				if (tmpFormat.m_uv0Channel.m_count) {
					m_attrib.m_uv0Channel.PushBack(tmpFormat.m_uv0Channel[index]);
				}
				if (tmpFormat.m_uv1Channel.m_count) {
					m_attrib.m_uv1Channel.PushBack(tmpFormat.m_uv1Channel[index]);
				}
				if (tmpFormat.m_colorChannel.m_count) {
					m_attrib.m_colorChannel.PushBack(tmpFormat.m_colorChannel[index]);
				}
			}
			edge->m_userData = attrIndexMap[index];
		}
	}
}

void dgMeshEffect::PackPoints (dgFloat64 tol)
{
	dgStack<dgInt32>vertexIndexMapBuffer(m_points.m_vertex.m_count);
	dgInt32* const vertexIndexMap = &vertexIndexMapBuffer[0];
	m_points.CompressData(&vertexIndexMap[0]);

	dgInt32	index[DG_MESH_EFFECT_POINT_SPLITED];
	dgInt64	userData[DG_MESH_EFFECT_POINT_SPLITED];
	dgPolyhedra polygon(GetAllocator());
	SwapInfo(polygon);
	dgAssert(GetCount() == 0);

	BeginFace();
	const dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter(polygon);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark != mark) && (edge->m_incidentFace > 0)) {
			dgEdge* ptr = edge;
			dgInt32 indexCount = 0;
			do {
				ptr->m_mark = mark;
				index[indexCount] = vertexIndexMap[ptr->m_incidentVertex];
				m_attrib.m_pointChannel[dgInt32 (ptr->m_userData)] = vertexIndexMap[ptr->m_incidentVertex];
				userData[indexCount] = ptr->m_userData;
				
				indexCount++;
				ptr = ptr->m_next;
			} while (ptr != edge);
			dgEdge* const face = AddFace(indexCount, index, userData);
			if (!face) {
				dgTrace (("skiping degeneraded face\n"));
				//dgAssert (0);
			}
		}
	}
	EndFace();
}

void dgMeshEffect::UnpackPoints()
{
	do {
		dgPointFormat points(m_points);
		m_points.Clear();
		for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i++) {
			dgInt32 index = m_attrib.m_pointChannel[i];
			m_points.m_vertex.PushBack(points.m_vertex[index]);
			if (points.m_layers.m_count) {
				m_points.m_layers.PushBack(points.m_layers[index]);
			}

			m_attrib.m_pointChannel[i] = i;
		}

		dgInt32	index[DG_MESH_EFFECT_POINT_SPLITED];
		dgInt64	userData[DG_MESH_EFFECT_POINT_SPLITED];
		dgPolyhedra polygon(GetAllocator());
		SwapInfo (polygon);
		dgAssert (GetCount() == 0);
		BeginFace();
		const dgInt32 mark = IncLRU();
		dgPolyhedra::Iterator iter(polygon);
		for (iter.Begin(); iter; iter++) {
			dgEdge* const face = &(*iter);
			if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
				dgEdge* ptr = face;
				dgInt32 indexCount = 0;
				do {
					ptr->m_mark = mark;
					index[indexCount] = dgInt32(ptr->m_userData);
					userData[indexCount] = ptr->m_userData;
					indexCount++;
					ptr = ptr->m_next;
				} while (ptr != face);
				AddFace(indexCount, index, userData);
			}
		}
	} while (!EndFace());

	dgAssert(m_points.m_vertex.m_count == m_attrib.m_pointChannel.m_count);
#ifdef _DEBUG
	for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i++) {
		dgAssert(m_attrib.m_pointChannel[i] == i);
	}
#endif

}


void dgMeshEffect::EndBuild (dgFloat64 tol, bool fixTjoint)
{
#ifdef _DEBUG
	for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i += 3) {
		dgBigVector p0 (m_points.m_vertex[i + 0]);
		dgBigVector p1 (m_points.m_vertex[i + 1]);
		dgBigVector p2 (m_points.m_vertex[i + 2]);
		dgBigVector e1 (p1 - p0);
		dgBigVector e2 (p2 - p0);
		dgBigVector n (e1.CrossProduct(e2));
		dgFloat64 mag2 = n.DotProduct3(n);
		dgAssert (mag2 > dgFloat32 (0.0f));
	}
#endif

	dgInt32 triangCount = m_points.m_vertex.m_count / 3;
	const dgInt32* const indexList = &m_attrib.m_pointChannel[0];
	for (dgInt32 i = 0; i < triangCount; i ++) {
		dgInt32 index[3];
		dgInt64 userdata[3];

		index[0] = indexList[i * 3 + 0];
		index[1] = indexList[i * 3 + 1];
		index[2] = indexList[i * 3 + 2];

		dgBigVector e1 (m_points.m_vertex[index[1]] - m_points.m_vertex[index[0]]);
		dgBigVector e2 (m_points.m_vertex[index[2]] - m_points.m_vertex[index[0]]);

		dgBigVector n (e1.CrossProduct(e2));
		dgFloat64 mag2 = n.DotProduct3(n);
		if (mag2 > dgFloat64 (1.0e-12f)) {
			userdata[0] = i * 3 + 0;
			userdata[1] = i * 3 + 1;
			userdata[2] = i * 3 + 2;
			dgEdge* const edge = AddFace (3, index, userdata);
			if (!edge) {
				dgAssert (0);
/*
				//dgAssert ((m_pointCount + 3) <= m_maxPointCount);
				m_points[m_pointCount + 0] = m_points[index[0]];
				m_points[m_pointCount + 1] = m_points[index[1]];
				m_points[m_pointCount + 2] = m_points[index[2]];

				index[0] = m_pointCount + 0;
				index[1] = m_pointCount + 1;
				index[2] = m_pointCount + 2;

				m_pointCount += 3;

				#ifdef _DEBUG
					dgEdge* test = AddFace (3, index, userdata);
					dgAssert (test);
				#else 
					AddFace (3, index, userdata);
				#endif
*/
			}
		}
	}
	EndFace();

	PackAttibuteData ();
	PackPoints (tol);

	if (fixTjoint) {
		RepairTJoints ();
	}

#ifdef _DEBUG
	dgPolyhedra::Iterator iter (*this);	
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if (face->m_incidentFace > 0) {
			dgBigVector p0 (m_points.m_vertex[face->m_incidentVertex]);
			dgBigVector p1 (m_points.m_vertex[face->m_next->m_incidentVertex]);
			dgBigVector p2 (m_points.m_vertex[face->m_next->m_next->m_incidentVertex]);
			dgBigVector e1 (p1 - p0);
			dgBigVector e2 (p2 - p0);
			dgBigVector n (e1.CrossProduct(e2));
			dgFloat64 mag2 = n.DotProduct3(n);
			dgAssert (mag2 >= dgFloat32 (0.0f));
		}
	}
#endif
}


void dgMeshEffect::OptimizePoints()
{

}

void dgMeshEffect::OptimizeAttibutes()
{
	UnpackAttibuteData ();
	PackAttibuteData();
}

void dgMeshEffect::BuildFromIndexList(const dgMeshVertexFormat* const format)
{
	BeginBuild();
	dgAssert (format->m_vertex.m_data);
	dgAssert (format->m_vertex.m_indexList);
	dgAssert (format->m_vertex.m_strideInBytes);

	// calculate vertex Count
	dgInt32 vertexCount = 0;
	dgInt32 maxAttribCount = 0;
	for (dgInt32 j = 0; j < format->m_faceCount; j++) {
		dgInt32 count = format->m_faceIndexCount[j];
		for (dgInt32 i = 0; i < count; i++) {
			vertexCount = dgMax(vertexCount, format->m_vertex.m_indexList[maxAttribCount + i] + 1);
		}
		maxAttribCount += count;
	}
	m_vertexBaseCount = vertexCount;

	dgInt32 layerIndex = 0;
	dgInt32 vertexStride = dgInt32(format->m_vertex.m_strideInBytes / sizeof (dgFloat64));
	const dgFloat64* const vertex = format->m_vertex.m_data;
	for (dgInt32 i = 0; i < vertexCount; i++) {
		dgInt32 index = i * vertexStride;
		m_points.m_layers.PushBack(layerIndex);
		dgBigVector p(vertex[index + 0], vertex[index + 1], vertex[index + 2], dgFloat64(0.0f));
		m_points.m_vertex.PushBack(p);
	}

	bool pendingFaces = true;
	dgInt32 layerBase = 0;
	dgInt32 attributeCount = 0;

	dgInt32 normalStride = dgInt32(format->m_normal.m_strideInBytes / sizeof (dgFloat32));
	dgInt32 binormalStride = dgInt32(format->m_binormal.m_strideInBytes / sizeof (dgFloat32));
	dgInt32 uv0Stride = dgInt32(format->m_uv0.m_strideInBytes / sizeof (dgFloat32));
	dgInt32 uv1Stride = dgInt32(format->m_uv1.m_strideInBytes / sizeof (dgFloat32));
	dgInt32 vertexColorStride = dgInt32(format->m_vertexColor.m_strideInBytes / sizeof (dgFloat32));

	dgStack<dgInt8> faceMark(format->m_faceCount);
	memset(&faceMark[0], 0, faceMark.GetSizeInBytes());
	const dgInt32* const vertexIndex = format->m_vertex.m_indexList;

	while (pendingFaces) {
		dgInt32 acc = 0;
		pendingFaces = false;
		dgInt32 vertexBank = layerIndex * vertexCount;
		for (dgInt32 j = 0; j < format->m_faceCount; j++) {
			dgInt32 indexCount = format->m_faceIndexCount[j];

			if (indexCount > 0) {
				dgInt32 index[256];
				dgInt64 userdata[256];
				dgAssert(indexCount >= 3);
				dgAssert(indexCount < dgInt32(sizeof (index) / sizeof (index[0])));

				if (!faceMark[j]) {
					for (int i = 0; i < indexCount; i++) {
						dgInt32 k = attributeCount + i;
						userdata[i] = k;
						index[i] = vertexIndex[acc + i] + vertexBank;
					}


					dgEdge* const edge = AddFace(indexCount, index, userdata);
					if (edge) {
						faceMark[j] = 1;
						for (int i = 0; i < indexCount; i++) {
							m_attrib.m_pointChannel.PushBack(index[i]);
						}

						if (format->m_faceMaterial) {
							dgInt32 materialIndex = format->m_faceMaterial[j];
							for (int i = 0; i < indexCount; i++) {
								m_attrib.m_materialChannel.PushBack(materialIndex);
							}
						}

						if (format->m_normal.m_data) {
							dgTriplex normal;
							for (int i = 0; i < indexCount; i++) {
								dgInt32 k = attributeCount + i;
								dgInt32 m = format->m_normal.m_indexList[k] * normalStride;
								normal.m_x = format->m_normal.m_data[m + 0];
								normal.m_y = format->m_normal.m_data[m + 1];
								normal.m_z = format->m_normal.m_data[m + 2];
								m_attrib.m_normalChannel.PushBack(normal);
							}
						}

						if (format->m_binormal.m_data) {
							dgTriplex normal;
							for (int i = 0; i < indexCount; i++) {
								dgInt32 k = attributeCount + i;
								dgInt32 m = format->m_binormal.m_indexList[k] * binormalStride;
								normal.m_x = format->m_binormal.m_data[m + 0];
								normal.m_y = format->m_binormal.m_data[m + 1];
								normal.m_z = format->m_binormal.m_data[m + 2];
								m_attrib.m_binormalChannel.PushBack(normal);
							}
						}

						if (format->m_vertexColor.m_data) {
							for (int i = 0; i < indexCount; i++) {
								dgInt32 k = attributeCount + i;
								dgInt32 m = format->m_vertexColor.m_indexList[k] * vertexColorStride;
								dgVector color(format->m_vertexColor.m_data[m + 0], format->m_vertexColor.m_data[m + 1], format->m_vertexColor.m_data[m + 2], format->m_vertexColor.m_data[m + 3]);
								m_attrib.m_colorChannel.PushBack(color);
							}
						}

						if (format->m_uv0.m_data) {
							dgAttibutFormat::dgUV uv;
							for (int i = 0; i < indexCount; i++) {
								dgInt32 k = attributeCount + i;
								dgInt32 m = format->m_uv0.m_indexList[k] * uv0Stride;
								uv.m_u = format->m_uv0.m_data[m + 0];
								uv.m_v = format->m_uv0.m_data[m + 1];
								m_attrib.m_uv0Channel.PushBack(uv);
							}
						}

						if (format->m_uv1.m_data) {
							dgAttibutFormat::dgUV uv;
							for (int i = 0; i < indexCount; i++) {
								dgInt32 k = attributeCount + i;
								dgInt32 m = format->m_uv1.m_indexList[k] * uv1Stride;
								uv.m_u = format->m_uv1.m_data[m + 0];
								uv.m_v = format->m_uv1.m_data[m + 1];
								m_attrib.m_uv1Channel.PushBack(uv);
							}
						}
						attributeCount += indexCount;

					} else {
						// check if the face is not degenerated
						bool degeneratedFace = false;
						for (int i = 0; i < indexCount - 1; i++) {
							for (int k = i + 1; k < indexCount; k++) {
								if (index[i] == index[k]) {
									degeneratedFace = true;
								}
							}
						}
						if (degeneratedFace) {
							faceMark[j] = 1;
						} else {
							pendingFaces = true;
						}
					}
				}
				acc += indexCount;
			}
		}

		if (pendingFaces) {
			//dgAssert (0);
			layerIndex++;
			layerBase += vertexCount;
			for (dgInt32 i = 0; i < vertexCount; i++) {
				m_points.m_layers.PushBack(layerIndex);
				dgInt32 index = i * vertexStride;
				dgBigVector p (vertex[index + 0], vertex[index + 1], vertex[index + 2], vertex[index + 3]);
				m_points.m_vertex.PushBack(p);
			}
		}
	}

	dgAssert (m_points.m_vertex.m_count == vertexCount * (layerIndex + 1));
	dgAssert (m_attrib.m_pointChannel.m_count == attributeCount);
	
	EndFace();
	PackAttibuteData();
}

dgInt32 dgMeshEffect::GetTotalFaceCount() const
{
	return GetFaceCount();
}

dgInt32 dgMeshEffect::GetTotalIndexCount() const
{
	Iterator iter (*this);
	dgInt32 count = 0;
	dgInt32 mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark == mark) {
			continue;
		}

		if (edge->m_incidentFace < 0) {
			continue;
		}

		dgEdge* ptr = edge;
		do {
			count ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}
	return count;
}

void dgMeshEffect::GetFaces (dgInt32* const facesIndex, dgInt32* const materials, void** const faceNodeList) const
{
	Iterator iter (*this);

	dgInt32 faces = 0;
	dgInt32 indexCount = 0;
	dgInt32 mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark == mark) {
			continue;
		}

		if (edge->m_incidentFace < 0) {
			continue;
		}

		dgInt32 faceCount = 0;
		dgEdge* ptr = edge;
		do {
//			indexList[indexCount] = dgInt32 (ptr->m_userData);
			faceNodeList[indexCount] = GetNodeFromInfo (*ptr);
			indexCount ++;
			faceCount ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);

		facesIndex[faces] = faceCount;
		//materials[faces] = dgFastInt(m_attrib[dgInt32 (edge->m_userData)].m_material);
		materials[faces] = m_attrib.m_materialChannel.m_count ? m_attrib.m_materialChannel[dgInt32(edge->m_userData)] : 0;
		faces ++;
	}
}

void* dgMeshEffect::GetFirstVertex () const
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		dgInt32 mark = IncLRU();
		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_twin->m_next;
		} while (ptr != edge);
	}
	return node;
}

void* dgMeshEffect::GetNextVertex (const void* const vertex) const
{
	dgTreeNode* const node0 = (dgTreeNode*) vertex;
	dgInt32 mark = node0->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node0);
	for (iter ++; iter; iter ++) {
		dgTreeNode* node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			dgEdge* const edge = &node->GetInfo();
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			return node;
		}
	}
	return NULL;
}

dgInt32 dgMeshEffect::GetVertexIndex (const void* const vertex) const
{
	dgTreeNode* const node = (dgTreeNode*) vertex;
	dgEdge* const edge = &node->GetInfo();
	return edge->m_incidentVertex;
}

void* dgMeshEffect::GetFirstPoint () const
{
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		dgEdge* const edge = &node->GetInfo();
		if (edge->m_incidentFace > 0) {
			return node;
		}
	}
	return NULL;
}

void* dgMeshEffect::GetNextPoint (const void* const point) const
{
	Iterator iter (*this);
	iter.Set ((dgTreeNode*) point);
	for (iter ++; iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		dgEdge* const edge = &node->GetInfo();
		if (edge->m_incidentFace > 0) {
			return node;
		}
	}
	return NULL;
}

dgInt32 dgMeshEffect::GetPointIndex (const void* const point) const
{
	dgTreeNode* const node = (dgTreeNode*) point;
	dgEdge* const edge = &node->GetInfo();
	return int (edge->m_userData);
}


dgInt32 dgMeshEffect::GetVertexIndexFromPoint (const void* const point) const
{
	return GetVertexIndex (point);
}

dgEdge* dgMeshEffect::SpliteFace (dgInt32 v0, dgInt32 v1)
{
	if (!FindEdge(v0, v1)) {
		dgPolyhedra::dgPairKey key (v0, 0);
		dgTreeNode* const node = FindGreaterEqual(key.GetVal());
		if (node) {
			dgEdge* const edge = &node->GetInfo();
			dgEdge* edge0 = edge;
			do {
				if (edge0->m_incidentFace > 0) {
					for (dgEdge* edge1 = edge0->m_next->m_next; edge1 != edge0->m_prev; edge1 = edge1->m_next) {
						if (edge1->m_incidentVertex == v1) {
							return ConnectVertex (edge0, edge1);
						}
					};
				}
				edge0 = edge0->m_twin->m_next;
			} while (edge0 != edge);
		}
	}
	return NULL;
}

const dgEdge* dgMeshEffect::GetPolyhedraEdgeFromNode(const void* const edge) const
{
	dgTreeNode* const node = (dgTreeNode*)edge;
	return &node->GetInfo();
}

void* dgMeshEffect::GetFirstEdge () const
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		dgInt32 mark = IncLRU();

		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		edge->m_mark = mark;
		edge->m_twin->m_mark = mark;
	}
	return node;
}

void* dgMeshEffect::GetNextEdge (const void* const edge) const
{
	dgTreeNode* const node0 = (dgTreeNode*) edge;
	dgInt32 mark = node0->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node0);
	for (iter ++; iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			node->GetInfo().m_mark = mark;
			node->GetInfo().m_twin->m_mark = mark;
			return node;
		}
	}
	return NULL;
}


void dgMeshEffect::GetEdgeIndex (const void* const edge, dgInt32& v0, dgInt32& v1) const
{
	dgTreeNode* const node = (dgTreeNode*) edge;
	v0 = node->GetInfo().m_incidentVertex;
	v1 = node->GetInfo().m_twin->m_incidentVertex;
}

//void* dgMeshEffect::FindEdge (dgInt32 v0, dgInt32 v1) const
//{
//	return FindEdgeNode(v0, v1);
//}

//void dgMeshEffect::GetEdgeAttributeIndex (const void* edge, dgInt32& v0, dgInt32& v1) const
//{
//	dgTreeNode* node = (dgTreeNode*) edge;
//	v0 = int (node->GetInfo().m_userData);
//	v1 = int (node->GetInfo().m_twin->m_userData);
//}


void* dgMeshEffect::GetFirstFace () const
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		dgInt32 mark = IncLRU();
		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}

	return node;
}

void* dgMeshEffect::GetNextFace (const void* const face) const
{
	dgTreeNode* const node0 = (dgTreeNode*) face;
	dgInt32 mark = node0->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node0);
	for (iter ++; iter; iter ++) {
		dgTreeNode* node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			dgEdge* const edge = &node->GetInfo();
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
			return node;
		}
	}
	return NULL;
}


dgInt32 dgMeshEffect::IsFaceOpen (const void* const face) const
{
	dgTreeNode* const node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	return (edge->m_incidentFace > 0) ? 0 : 1;
}

dgInt32 dgMeshEffect::GetFaceMaterial (const void* const face) const
{
	dgTreeNode* const node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	return dgInt32 (m_attrib.m_materialChannel.m_count ? m_attrib.m_materialChannel[dgInt32 (edge->m_userData)] : 0);
}

void dgMeshEffect::SetFaceMaterial (const void* const face, int mateialID)
{
	if (m_attrib.m_materialChannel.m_count) {
		dgTreeNode* const node = (dgTreeNode*) face;
		dgEdge* const edge = &node->GetInfo();
		if (edge->m_incidentFace > 0) {
			dgEdge* ptr = edge;
			do {
				//dgVertexAtribute* const attrib = &m_attrib[ptr->m_userData];
				//attrib->m_material = dgFloat64 (mateialID);
				m_attrib.m_materialChannel[dgInt32 (edge->m_userData)] = mateialID;
				ptr = ptr->m_next;
			} while (ptr != edge) ;
		}
	}
}

dgInt32 dgMeshEffect::GetFaceIndexCount (const void* const face) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
	return count;
}

void dgMeshEffect::GetFaceIndex (const void* const face, dgInt32* const indices) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		indices[count] =  ptr->m_incidentVertex;
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
}

void dgMeshEffect::GetFaceAttributeIndex (const void* const face, dgInt32* const indices) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		indices[count] = int (ptr->m_userData);
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
}


dgBigVector dgMeshEffect::CalculateFaceNormal (const void* const face) const
{
	dgTreeNode* const node = (dgTreeNode*) face;
	dgEdge* const faceEdge = &node->GetInfo();
	dgBigVector normal (FaceNormal (faceEdge, &m_points.m_vertex[0].m_x, sizeof (dgBigVector)));
	dgAssert (normal.m_w == dgFloat32 (0.0f));
	//normal = normal.Scale (1.0f / sqrt (normal.DotProduct3(normal)));
	normal = normal.Normalize();
	return normal;
}

/*
dgInt32 GetTotalFaceCount() const;
{
	dgInt32 mark;
	dgInt32 count;
	dgInt32 materialCount;
	dgInt32 materials[256];
	dgInt32 streamIndexMap[256];
	dgIndexArray* array;

	count = 0;
	materialCount = 0;

	array = (dgIndexArray*) GetAllocator()->MallocLow (4 * sizeof (dgInt32) * GetCount() + sizeof (dgIndexArray) + 2048);
	array->m_indexList = (dgInt32*)&array[1];

	mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	memset(streamIndexMap, 0, sizeof (streamIndexMap));
	for(iter.Begin(); iter; iter ++){

		dgEdge* const edge;
		edge = &(*iter);
		if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark)) {
			dgEdge* ptr;
			dgInt32 hashValue;
			dgInt32 index0;
			dgInt32 index1;

			ptr = edge;
			ptr->m_mark = mark;
			index0 = dgInt32 (ptr->m_userData);

			ptr = ptr->m_next;
			ptr->m_mark = mark;
			index1 = dgInt32 (ptr->m_userData);

			ptr = ptr->m_next;
			do {
				ptr->m_mark = mark;

				array->m_indexList[count * 4 + 0] = index0;
				array->m_indexList[count * 4 + 1] = index1;
				array->m_indexList[count * 4 + 2] = dgInt32 (ptr->m_userData);
				array->m_indexList[count * 4 + 3] = m_attrib[dgInt32 (edge->m_userData)].m_material;
				index1 = dgInt32 (ptr->m_userData);

				hashValue = array->m_indexList[count * 4 + 3] & 0xff;
				streamIndexMap[hashValue] ++;
				materials[hashValue] = array->m_indexList[count * 4 + 3];
				count ++;

				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
}
*/

bool dgMeshEffect::HasNormalChannel() const
{
	return m_attrib.m_normalChannel.m_count != 0;
}

bool dgMeshEffect::HasBinormalChannel() const
{
	return m_attrib.m_binormalChannel.m_count != 0;
}

bool dgMeshEffect::HasUV0Channel() const
{
	return m_attrib.m_uv0Channel.m_count != 0;
}

bool dgMeshEffect::HasUV1Channel() const
{
	return m_attrib.m_uv1Channel.m_count != 0;
}

bool dgMeshEffect::HasVertexColorChannel() const
{
	return m_attrib.m_colorChannel.m_count != 0;
}

void dgMeshEffect::GetVertexChannel64(dgInt32 strideInByte, dgFloat64* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat64);
	for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i ++)	{
		const dgInt32 j = i * stride;
		const dgInt32 index = m_attrib.m_pointChannel[i];
		bufferOut[j + 0] = m_points.m_vertex[index].m_x;
		bufferOut[j + 1] = m_points.m_vertex[index].m_y;
		bufferOut[j + 2] = m_points.m_vertex[index].m_z;
	}
}

void dgMeshEffect::GetVertexChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i++) {
		const dgInt32 j = i * stride;
		const dgInt32 index = m_attrib.m_pointChannel[i];
		const dgBigVector& p = m_points.m_vertex[index];
		bufferOut[j + 0] = dgFloat32(p.m_x);
		bufferOut[j + 1] = dgFloat32(p.m_y);
		bufferOut[j + 2] = dgFloat32(p.m_z);
	}
}

/*
void dgMeshEffect::GetWeightBlendChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt8* const buffer = (dgInt8*)bufferOut;
	for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i++) {
		const dgInt32 j = i * strideInByte;
		dgFloat32* const ptr = (dgFloat32*)&buffer[j];

		const dgInt32 index = m_attrib.m_pointChannel[i];
		const dgFloat32* const p = &m_points.m_weights[index].m_weightBlends[0];
		ptr[0] = dgFloat32(p[0]);
		ptr[1] = dgFloat32(p[1]);
		ptr[2] = dgFloat32(p[2]);
		ptr[3] = dgFloat32(p[3]);
	}
}

void dgMeshEffect::GetWeightIndexChannel(dgInt32 strideInByte, dgInt32* const bufferOut) const
{
	dgInt8* const buffer = (dgInt8*)bufferOut;
	for (dgInt32 i = 0; i < m_attrib.m_pointChannel.m_count; i++) {
		const dgInt32 j = i * strideInByte;
		dgInt32* const ptr = (dgInt32*)&buffer[j];
		const dgInt32 index = m_attrib.m_pointChannel[i];
		const dgInt32* const p = &m_points.m_weights[index].m_controlIndex[0];
		ptr[0] = p[0];
		ptr[1] = p[1];
		ptr[2] = p[2];
		ptr[3] = p[3];
	}
}
*/

void dgMeshEffect::GetNormalChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_normalChannel.m_count; i++) {
		const dgInt32 j = i * stride;
		bufferOut[j + 0] = dgFloat32(m_attrib.m_normalChannel[i].m_x);
		bufferOut[j + 1] = dgFloat32(m_attrib.m_normalChannel[i].m_y);
		bufferOut[j + 2] = dgFloat32(m_attrib.m_normalChannel[i].m_z);
	}
}

void dgMeshEffect::GetBinormalChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_binormalChannel.m_count; i++) {
		const dgInt32 j = i * stride;
		bufferOut[j + 0] = dgFloat32(m_attrib.m_binormalChannel[i].m_x);
		bufferOut[j + 1] = dgFloat32(m_attrib.m_binormalChannel[i].m_y);
		bufferOut[j + 2] = dgFloat32(m_attrib.m_binormalChannel[i].m_z);
	}
}

void dgMeshEffect::GetUV0Channel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_uv0Channel.m_count; i++) {
		const dgInt32 j = i * stride;
		bufferOut[j + 0] = dgFloat32(m_attrib.m_uv0Channel[i].m_u);
		bufferOut[j + 1] = dgFloat32(m_attrib.m_uv0Channel[i].m_v);
	}
}

void dgMeshEffect::GetUV1Channel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_uv1Channel.m_count; i++) {
		const dgInt32 j = i * stride;
		bufferOut[j + 0] = dgFloat32(m_attrib.m_uv1Channel[i].m_u);
		bufferOut[j + 1] = dgFloat32(m_attrib.m_uv1Channel[i].m_v);
	}
}

void dgMeshEffect::GetVertexColorChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const
{
	dgInt32 stride = strideInByte / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < m_attrib.m_colorChannel.m_count; i++) {
		const dgInt32 j = i * stride;
		bufferOut[j + 0] = dgFloat32(m_attrib.m_colorChannel[i].m_x);
		bufferOut[j + 1] = dgFloat32(m_attrib.m_colorChannel[i].m_y);
		bufferOut[j + 2] = dgFloat32(m_attrib.m_colorChannel[i].m_z);
		bufferOut[j + 3] = dgFloat32(m_attrib.m_colorChannel[i].m_w);
	}
}

dgMeshEffect::dgIndexArray* dgMeshEffect::MaterialGeometryBegin()
{
	dgInt32 materials[256];
	dgInt32 streamIndexMap[256];

	dgInt32 count = 0;
	dgInt32 materialCount = 0;
	
	dgIndexArray* const array = (dgIndexArray*) GetAllocator()->MallocLow (dgInt32 (4 * sizeof (dgInt32) * GetCount() + sizeof (dgIndexArray) + 2048));
	array->m_indexList = (dgInt32*)&array[1];
	
	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	memset(streamIndexMap, 0, sizeof (streamIndexMap));
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark)) {
			dgEdge* ptr = edge;
			ptr->m_mark = mark;
			dgInt32 index0 = dgInt32 (ptr->m_userData);

			ptr = ptr->m_next;
			ptr->m_mark = mark;
			dgInt32 index1 = dgInt32 (ptr->m_userData);

			ptr = ptr->m_next;
			do {
				ptr->m_mark = mark;

				array->m_indexList[count * 4 + 0] = index0;
				array->m_indexList[count * 4 + 1] = index1;
				array->m_indexList[count * 4 + 2] = dgInt32 (ptr->m_userData);
				array->m_indexList[count * 4 + 3] = m_attrib.m_materialChannel.m_count ? dgInt32 (m_attrib.m_materialChannel[dgInt32 (edge->m_userData)]) : 0;
				index1 = dgInt32 (ptr->m_userData);

				dgInt32 hashValue = array->m_indexList[count * 4 + 3] & 0xff;
				streamIndexMap[hashValue] ++;
				materials[hashValue] = array->m_indexList[count * 4 + 3];
				count ++;

				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	array->m_indexCount = count;
	array->m_materialCount = materialCount;

	count = 0;
	for (dgInt32 i = 0; i < 256;i ++) {
		if (streamIndexMap[i]) {
			array->m_materials[count] = materials[i];
			array->m_materialsIndexCount[count] = streamIndexMap[i] * 3;
			count ++;
		}
	}

	array->m_materialCount = count;

	return array;
}

void dgMeshEffect::MaterialGeomteryEnd(dgIndexArray* const handle)
{
	GetAllocator()->FreeLow (handle);
}


dgInt32 dgMeshEffect::GetFirstMaterial (dgIndexArray* const handle) const
{
	return GetNextMaterial (handle, -1);
}

dgInt32 dgMeshEffect::GetNextMaterial (dgIndexArray* const handle, dgInt32 materialId) const
{
	materialId ++;
	if(materialId >= handle->m_materialCount) {
		materialId = -1;
	}
	return materialId;
}

void dgMeshEffect::GetMaterialGetIndexStream (dgIndexArray* const handle, dgInt32 materialHandle, dgInt32* const indexArray) const
{

	dgInt32 index = 0;
	dgInt32 textureID = handle->m_materials[materialHandle];
	for (dgInt32 j = 0; j < handle->m_indexCount; j ++) {
		if (handle->m_indexList[j * 4 + 3] == textureID) {
			indexArray[index + 0] = handle->m_indexList[j * 4 + 0];
			indexArray[index + 1] = handle->m_indexList[j * 4 + 1];
			indexArray[index + 2] = handle->m_indexList[j * 4 + 2];

			index += 3;
		}
	}
}

void dgMeshEffect::GetMaterialGetIndexStreamShort (dgIndexArray* const handle, dgInt32 materialHandle, dgInt16* const indexArray) const
{
	dgInt32 index = 0;
	dgInt32 textureID = handle->m_materials[materialHandle];
	for (dgInt32 j = 0; j < handle->m_indexCount; j ++) {
		if (handle->m_indexList[j * 4 + 3] == textureID) {
			indexArray[index + 0] = (dgInt16)handle->m_indexList[j * 4 + 0];
			indexArray[index + 1] = (dgInt16)handle->m_indexList[j * 4 + 1];
			indexArray[index + 2] = (dgInt16)handle->m_indexList[j * 4 + 2];
			index += 3;
		}
	}
}

dgCollisionInstance* dgMeshEffect::CreateCollisionTree(dgWorld* const world, dgInt32 shapeID) const
{
	dgCollisionBVH* const collision = new  (GetAllocator()) dgCollisionBVH (world);

	collision->BeginBuild();

	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgTreeNode* const faceNode = iter.GetNode();
		//dgEdge* const face = &(*iter);
		dgEdge* const face = &faceNode->GetInfo();
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			dgInt32 count = 0;
			dgVector polygon[256]; 
			dgEdge* ptr = face;
			do {
				//polygon[count] = dgVector (m_points[ptr->m_incidentVertex]);
				polygon[count] = GetVertex(ptr->m_incidentVertex);
				count ++;
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != face);
			//collision->AddFace(count, &polygon[0].m_x, sizeof (dgVector), dgInt32 (m_attrib[face->m_userData].m_material));
			collision->AddFace(count, &polygon[0].m_x, sizeof (dgVector), GetFaceMaterial(faceNode));
		}
	}
	collision->EndBuild(0);

	dgCollisionInstance* const instance = world->CreateInstance(collision, shapeID, dgGetIdentityMatrix());
	collision->Release();
	return instance;
}

dgCollisionInstance* dgMeshEffect::CreateConvexCollision(dgWorld* const world, dgFloat64 tolerance, dgInt32 shapeID, const dgMatrix& srcMatrix) const
{
	dgStack<dgVector> poolPtr (m_points.m_vertex.m_count * 2); 
	dgVector* const pool = &poolPtr[0];

	dgBigVector minBox;
	dgBigVector maxBox;
	CalculateAABB (minBox, maxBox);
	//dgVector com ((minBox + maxBox).Scale (dgFloat32 (0.5f)));
	dgVector com ((minBox + maxBox) * dgVector::m_half);

	dgInt32 count = 0;
	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const vertex = &(*iter);
		if (vertex->m_mark != mark) {
			dgEdge* ptr = vertex;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != vertex);

			if (count < dgInt32 (poolPtr.GetElementsCount())) {
				const dgBigVector p (m_points.m_vertex[vertex->m_incidentVertex]);
				pool[count] = dgVector (p) - com;
				count ++;
			}
		}
	}

	dgMatrix matrix (srcMatrix);
	matrix.m_posit += matrix.RotateVector(com);
	matrix.m_posit.m_w = dgFloat32 (1.0f);

	dgUnsigned32 crc = dgCollisionConvexHull::CalculateSignature (count, &pool[0].m_x, sizeof (dgVector));
	dgCollisionConvexHull* const collision = new (GetAllocator()) dgCollisionConvexHull (GetAllocator(), crc, count, sizeof (dgVector), dgFloat32 (tolerance), &pool[0].m_x);
	if (!collision->GetConvexVertexCount()) {
		collision->Release();
		return NULL;
	}
	dgCollisionInstance* const instance = world->CreateInstance(collision, shapeID, matrix);
	collision->Release();
	return instance;
}


void dgMeshEffect::TransformMesh (const dgMatrix& matrix)
{
	dgAssert(0);
	/*
	dgMatrix normalMatrix (matrix);
	normalMatrix.m_posit = dgVector (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (1.0f));

	matrix.TransformTriplex (&m_points->m_x, sizeof (dgBigVector), &m_points->m_x, sizeof (dgBigVector), m_pointCount);
	matrix.TransformTriplex (&m_attrib[0].m_vertex.m_x, sizeof (dgVertexAtribute), &m_attrib[0].m_vertex.m_x, sizeof (dgVertexAtribute), m_atribCount);
	normalMatrix.TransformTriplex (&m_attrib[0].m_normal_x, sizeof (dgVertexAtribute), &m_attrib[0].m_normal_x, sizeof (dgVertexAtribute), m_atribCount);
*/
}

dgInt32 dgMeshEffect::AddInterpolatedHalfAttribute(dgEdge* const edge, dgInt32 midPoint)
{
	dgBigVector p0(m_points.m_vertex[edge->m_incidentVertex]);
	dgBigVector p2(m_points.m_vertex[edge->m_next->m_incidentVertex]);
	dgBigVector p1(m_points.m_vertex[midPoint]);
	dgBigVector p2p0(p2 - p0);
	dgFloat64 den = p2p0.DotProduct3(p2p0);
	dgFloat64 param = p2p0.DotProduct3(p1 - p0) / den;
	dgFloat64 t1 = param;
	dgFloat64 t0 = dgFloat64(1.0f) - t1;
	dgAssert(t1 >= dgFloat64(0.0f));
	dgAssert(t1 <= dgFloat64(1.0f));

	m_attrib.m_pointChannel.PushBack(midPoint);

	if (m_attrib.m_materialChannel.m_count) {
		m_attrib.m_materialChannel.PushBack(m_attrib.m_materialChannel[dgInt32(edge->m_userData)]);
	}
	if (m_attrib.m_normalChannel.m_count) {
		dgTriplex edgeNormal;
		dgTriplex edgeNormal0(m_attrib.m_normalChannel[dgInt32(edge->m_userData)]);
		dgTriplex edgeNormal1(m_attrib.m_normalChannel[dgInt32(edge->m_next->m_userData)]);
		edgeNormal.m_x = edgeNormal0.m_x * dgFloat32(t0) + edgeNormal1.m_x * dgFloat32(t1);
		edgeNormal.m_y = edgeNormal0.m_y * dgFloat32(t0) + edgeNormal1.m_y * dgFloat32(t1);
		edgeNormal.m_z = edgeNormal0.m_z * dgFloat32(t0) + edgeNormal1.m_z * dgFloat32(t1);
		m_attrib.m_normalChannel.PushBack(edgeNormal);
	}
	if (m_attrib.m_binormalChannel.m_count) {
		dgAssert(0);
	}

	if (m_attrib.m_uv0Channel.m_count) {
		dgAttibutFormat::dgUV edgeUV;
		dgAttibutFormat::dgUV edgeUV0(m_attrib.m_uv0Channel[dgInt32(edge->m_userData)]);
		dgAttibutFormat::dgUV edgeUV1(m_attrib.m_uv0Channel[dgInt32(edge->m_next->m_userData)]);
		edgeUV.m_u = edgeUV0.m_u * dgFloat32(t0) + edgeUV1.m_u * dgFloat32(t1);
		edgeUV.m_v = edgeUV0.m_v * dgFloat32(t0) + edgeUV1.m_v * dgFloat32(t1);
		m_attrib.m_uv0Channel.PushBack(edgeUV);
	}

	if (m_attrib.m_uv1Channel.m_count) {
		dgAssert(0);
	}

	if (m_attrib.m_colorChannel.m_count) {
		dgAssert(0);
	}
	return m_attrib.m_pointChannel.m_count - 1;
}

void dgMeshEffect::AddInterpolatedEdgeAttribute (dgEdge* const edge, dgFloat64 param)
{
	dgFloat64 t1 = param;
	dgFloat64 t0 = dgFloat64 (1.0f) - t1;
	dgAssert (t1 >= dgFloat64(0.0f));
	dgAssert (t1 <= dgFloat64(1.0f));

	const dgInt32 vertexIndex = m_points.m_vertex.m_count;
	m_points.m_vertex.PushBack(m_points.m_vertex[edge->m_incidentVertex].Scale(t0) + m_points.m_vertex[edge->m_next->m_incidentVertex].Scale(t1));
	if (m_points.m_layers.m_count) {
		m_points.m_layers.PushBack(m_points.m_layers[edge->m_incidentVertex]);
	}

	m_attrib.m_pointChannel.PushBack(vertexIndex);
	m_attrib.m_pointChannel.PushBack(vertexIndex);

	if (m_attrib.m_materialChannel.m_count) {
		m_attrib.m_materialChannel.PushBack(m_attrib.m_materialChannel[dgInt32(edge->m_userData)]);
		m_attrib.m_materialChannel.PushBack(m_attrib.m_materialChannel[dgInt32(edge->m_twin->m_userData)]);
	}
	if (m_attrib.m_normalChannel.m_count) {
		dgTriplex edgeNormal;
		dgTriplex edgeNormal0(m_attrib.m_normalChannel[dgInt32(edge->m_userData)]);
		dgTriplex edgeNormal1(m_attrib.m_normalChannel[dgInt32(edge->m_next->m_userData)]);
		edgeNormal.m_x = edgeNormal0.m_x * dgFloat32(t0) + edgeNormal1.m_x * dgFloat32(t1);
		edgeNormal.m_y = edgeNormal0.m_y * dgFloat32(t0) + edgeNormal1.m_y * dgFloat32(t1);
		edgeNormal.m_z = edgeNormal0.m_z * dgFloat32(t0) + edgeNormal1.m_z * dgFloat32(t1);
		m_attrib.m_normalChannel.PushBack(edgeNormal);
		
		dgTriplex twinNormal;
		dgTriplex twinNormal0(m_attrib.m_normalChannel[dgInt32(edge->m_twin->m_next->m_userData)]);
		dgTriplex twinNormal1(m_attrib.m_normalChannel[dgInt32(edge->m_twin->m_userData)]);
		twinNormal.m_x = twinNormal0.m_x * dgFloat32(t0) + twinNormal1.m_x * dgFloat32(t1);
		twinNormal.m_y = twinNormal0.m_y * dgFloat32(t0) + twinNormal1.m_y * dgFloat32(t1);
		twinNormal.m_z = twinNormal0.m_z * dgFloat32(t0) + twinNormal1.m_z * dgFloat32(t1);
		m_attrib.m_normalChannel.PushBack(twinNormal);
	}
	if (m_attrib.m_binormalChannel.m_count) {
		dgAssert(0);
	}

	if (m_attrib.m_uv0Channel.m_count) {
		dgAttibutFormat::dgUV edgeUV;
		dgAttibutFormat::dgUV edgeUV0(m_attrib.m_uv0Channel[dgInt32(edge->m_userData)]);
		dgAttibutFormat::dgUV edgeUV1(m_attrib.m_uv0Channel[dgInt32(edge->m_next->m_userData)]);
		edgeUV.m_u = edgeUV0.m_u * dgFloat32(t0) + edgeUV1.m_u * dgFloat32(t1);
		edgeUV.m_v = edgeUV0.m_v * dgFloat32(t0) + edgeUV1.m_v * dgFloat32(t1);
		m_attrib.m_uv0Channel.PushBack(edgeUV);

		dgAttibutFormat::dgUV twinUV;
		dgAttibutFormat::dgUV twinUV0(m_attrib.m_uv0Channel[dgInt32(edge->m_twin->m_next->m_userData)]);
		dgAttibutFormat::dgUV twinUV1(m_attrib.m_uv0Channel[dgInt32(edge->m_twin->m_userData)]);
		twinUV.m_u = twinUV0.m_u * dgFloat32(t0) + twinUV1.m_u * dgFloat32(t1);
		twinUV.m_v = twinUV0.m_v * dgFloat32(t0) + twinUV1.m_v * dgFloat32(t1);
		m_attrib.m_uv0Channel.PushBack(twinUV);
	}

	if (m_attrib.m_uv1Channel.m_count) {
		dgAssert(0);
	}

	if (m_attrib.m_colorChannel.m_count) {
		dgAssert(0);
	}
}

bool dgMeshEffect::Sanity () const
{
	#ifdef  _DEBUG
		dgMeshEffect::Iterator iter (*this);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			dgAssert(edge->m_twin);
			dgAssert(edge->m_next);
			dgAssert(edge->m_prev);
			dgAssert (edge->m_twin->m_twin == edge);
			dgAssert (edge->m_next->m_incidentVertex == edge->m_twin->m_incidentVertex);
			dgAssert (edge->m_incidentVertex == edge->m_twin->m_next->m_incidentVertex);
		}
	#endif
	return true;
}


dgEdge* dgMeshEffect::InsertEdgeVertex (dgEdge* const edge, dgFloat64 param)
{
	dgEdge* const twin = edge->m_twin;
	AddInterpolatedEdgeAttribute(edge, param);

	dgInt32 edgeAttrV0 = dgInt32 (edge->m_userData);
	dgInt32 twinAttrV0 = dgInt32 (twin->m_userData);

	dgEdge* const faceA0 = edge->m_next;
	dgEdge* const faceA1 = edge->m_prev;
	dgEdge* const faceB0 = twin->m_next;
	dgEdge* const faceB1 = twin->m_prev;
	SpliteEdge (m_points.m_vertex.m_count - 1, edge);

	faceA0->m_prev->m_userData = dgUnsigned64 (m_attrib.m_pointChannel.m_count - 2);
	faceA1->m_next->m_userData = dgUnsigned64 (edgeAttrV0);

	faceB0->m_prev->m_userData = dgUnsigned64 (m_attrib.m_pointChannel.m_count - 1);
	faceB1->m_next->m_userData = dgUnsigned64 (twinAttrV0);
	return faceA1->m_next;
}



//dgMeshEffect::dgVertexAtribute dgMeshEffect::InterpolateVertex (const dgBigVector& srcPoint, const dgEdge* const face) const
dgInt32 dgMeshEffect::InterpolateVertex (const dgBigVector& srcPoint, const dgEdge* const face) const
{
dgAssert(0);
return 0;
/*
	const dgBigVector point (srcPoint);

	dgVertexAtribute attribute;
	memset (&attribute, 0, sizeof (attribute));

//	dgBigVector normal (FaceNormal(face, &m_points[0].m_x, sizeof(dgBigVector)));
//	normal = normal.Scale (dgFloat64 (1.0f) / sqrt (normal % normal));
//	attribute.m_vertex = srcPoint;
//	attribute.m_normal_x = normal.m_x;
//	attribute.m_normal_y = normal.m_y;
//	attribute.m_normal_z = normal.m_z;

	dgFloat64 tol = dgFloat32 (1.0e-4f);
	for (dgInt32 i = 0; i < 4; i ++) {
		const dgEdge* ptr = face;
		const dgEdge* const edge0 = ptr;
		dgBigVector q0 (m_points[ptr->m_incidentVertex]);

		ptr = ptr->m_next;
		const dgEdge* edge1 = ptr;
		dgBigVector q1 (m_points[ptr->m_incidentVertex]);

		ptr = ptr->m_next;
		const dgEdge* edge2 = ptr;
		do {
			const dgBigVector q2 (m_points[ptr->m_incidentVertex]);

			dgBigVector p10 (q1 - q0);
			dgBigVector p20 (q2 - q0);

			dgFloat64 dot = p20.DotProduct3(p10);
			dgFloat64 mag1 = p10.DotProduct3(p10);
			dgFloat64 mag2 = p20.DotProduct3(p20);
			dgFloat64 collinear = dot * dot - mag2 * mag1;
			if (fabs (collinear) > dgFloat64 (1.0e-8f)) {
				dgBigVector p_p0 (point - q0);
				dgBigVector p_p1 (point - q1);
				dgBigVector p_p2 (point - q2);

				dgFloat64 alpha1 = p10.DotProduct3(p_p0);
				dgFloat64 alpha2 = p20.DotProduct3(p_p0);
				dgFloat64 alpha3 = p10.DotProduct3(p_p1);
				dgFloat64 alpha4 = p20.DotProduct3(p_p1);
				dgFloat64 alpha5 = p10.DotProduct3(p_p2);
				dgFloat64 alpha6 = p20.DotProduct3(p_p2);

				dgFloat64 vc = alpha1 * alpha4 - alpha3 * alpha2;
				dgFloat64 vb = alpha5 * alpha2 - alpha1 * alpha6;
				dgFloat64 va = alpha3 * alpha6 - alpha5 * alpha4;
				dgFloat64 den = va + vb + vc;
				dgFloat64 minError = den * (-tol);
				dgFloat64 maxError = den * (dgFloat32 (1.0f) + tol);
				if ((va > minError) && (vb > minError) && (vc > minError) && (va < maxError) && (vb < maxError) && (vc < maxError)) {
					edge2 = ptr;

					den = dgFloat64 (1.0f) / (va + vb + vc);

					dgFloat64 alpha0 = dgFloat32 (va * den);
					dgFloat64 alpha1 = dgFloat32 (vb * den);
					dgFloat64 alpha2 = dgFloat32 (vc * den);

					const dgVertexAtribute& attr0 = m_attrib[edge0->m_userData];
					const dgVertexAtribute& attr1 = m_attrib[edge1->m_userData];
					const dgVertexAtribute& attr2 = m_attrib[edge2->m_userData];
					dgBigVector normal (attr0.m_normal_x * alpha0 + attr1.m_normal_x * alpha1 + attr2.m_normal_x * alpha2,
										attr0.m_normal_y * alpha0 + attr1.m_normal_y * alpha1 + attr2.m_normal_y * alpha2,
										attr0.m_normal_z * alpha0 + attr1.m_normal_z * alpha1 + attr2.m_normal_z * alpha2, dgFloat32 (0.0f));
					//normal = normal.Scale (dgFloat64 (1.0f) / sqrt (normal.DotProduct3(normal)));
					normal = normal.Normalize();

		#ifdef _DEBUG
					dgBigVector testPoint (attr0.m_vertex.m_x * alpha0 + attr1.m_vertex.m_x * alpha1 + attr2.m_vertex.m_x * alpha2,
										   attr0.m_vertex.m_y * alpha0 + attr1.m_vertex.m_y * alpha1 + attr2.m_vertex.m_y * alpha2,
										   attr0.m_vertex.m_z * alpha0 + attr1.m_vertex.m_z * alpha1 + attr2.m_vertex.m_z * alpha2, dgFloat32 (0.0f));
					dgAssert (fabs (testPoint.m_x - point.m_x) < dgFloat32 (1.0e-2f));
					dgAssert (fabs (testPoint.m_y - point.m_y) < dgFloat32 (1.0e-2f));
					dgAssert (fabs (testPoint.m_z - point.m_z) < dgFloat32 (1.0e-2f));
		#endif


					attribute.m_vertex.m_x = point.m_x;
					attribute.m_vertex.m_y = point.m_y;
					attribute.m_vertex.m_z = point.m_z;
					attribute.m_vertex.m_w = point.m_w;
					attribute.m_normal_x = normal.m_x;
					attribute.m_normal_y = normal.m_y;
					attribute.m_normal_z = normal.m_z;
					attribute.m_u0 = attr0.m_u0 * alpha0 +  attr1.m_u0 * alpha1 + attr2.m_u0 * alpha2;
					attribute.m_v0 = attr0.m_v0 * alpha0 +  attr1.m_v0 * alpha1 + attr2.m_v0 * alpha2;
					attribute.m_u1 = attr0.m_u1 * alpha0 +  attr1.m_u1 * alpha1 + attr2.m_u1 * alpha2;
					attribute.m_v1 = attr0.m_v1 * alpha0 +  attr1.m_v1 * alpha1 + attr2.m_v1 * alpha2;

					attribute.m_material = attr0.m_material;
					dgAssert (attr0.m_material == attr1.m_material);
					dgAssert (attr0.m_material == attr2.m_material);
					return attribute; 
				}
			}
				
			q1 = q2;
			edge1 = ptr;

			ptr = ptr->m_next;
		} while (ptr != face);
		tol *= dgFloat64 (2.0f);
	}
	// this should never happens
	dgAssert (0);
	return attribute;
*/
}

bool dgMeshEffect::HasOpenEdges () const
{
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if (face->m_incidentFace < 0){
			return true;
		}
	}
	return false;
}

dgFloat64 dgMeshEffect::CalculateVolume () const
{
	dgAssert (0);
	return 0;
	/*

	dgPolyhedraMassProperties localData;

	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgInt32 count;
		dgEdge* ptr;
		dgEdge* face;
		dgVector points[256];
		
		face = &(*iter);
		if ((face->m_incidentFace > 0) && (face->m_mark != mark)) {
			count = 0;
			ptr = face;
			do {
				points[count] = m_points[ptr->m_incidentVertex];
				count ++;
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != face);
			localData.AddCGFace (count, points);
		}
	}

	dgFloat32 volume;
	dgVector p0;
	dgVector p1;
	dgVector com;
	dgVector inertia;
	dgVector crossInertia;
	volume = localData.MassProperties (com, inertia, crossInertia);
	return volume;
*/
}


dgMeshEffect* dgMeshEffect::GetNextLayer (dgInt32 mark)
{
	Iterator iter(*this);
	dgEdge* edge = NULL;
	for (iter.Begin (); iter; iter ++) {
		edge = &(*iter);
		if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
			break;
		}
	}

	if (!edge) {
		return NULL;
	}

	const dgInt32 layer = m_points.m_layers.m_count ? m_points.m_layers[edge->m_incidentVertex] : 0;
	dgPolyhedra polyhedra(GetAllocator());

	polyhedra.BeginFace ();
	for (iter.Begin (); iter; iter ++) {
		dgEdge* const edge1 = &(*iter);
		if ((edge1->m_mark < mark) && (edge1->m_incidentFace > 0)) {
			const dgInt32 thislayer = m_points.m_layers.m_count  ? m_points.m_layers[edge1->m_incidentVertex] : 0;
			if (thislayer == layer) {
				dgEdge* ptr = edge1;
				dgInt32 count = 0;
				dgInt32 faceIndex[256];
				dgInt64 faceDataIndex[256];
				do {
					ptr->m_mark = mark;
					faceIndex[count] = ptr->m_incidentVertex;
					faceDataIndex[count] = ptr->m_userData;
					count ++;
					dgAssert (count < dgInt32 (sizeof (faceIndex)/ sizeof(faceIndex[0])));
					ptr = ptr->m_next;
				} while (ptr != edge1);
				polyhedra.AddFace (count, &faceIndex[0], &faceDataIndex[0]);
			}
		}
	}
	polyhedra.EndFace ();

	dgMeshEffect* solid = NULL;
	if (polyhedra.GetCount()) {
		solid = new (GetAllocator()) dgMeshEffect(polyhedra, *this);
		solid->SetLRU(mark);
	}
	return solid;
}



void dgMeshEffect::MergeFaces (const dgMeshEffect* const source)
{
	dgInt32 mark = source->IncLRU();
	dgPolyhedra::Iterator iter (*source);
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace > 0) && (edge->m_mark < mark)) {
			BeginBuildFace ();
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				dgInt32 vIndex = ptr->m_incidentVertex;
				dgInt32 aIndex = dgInt32 (ptr->m_userData);
				AddPoint (source->m_points.m_vertex[vIndex].m_x, source->m_points.m_vertex[vIndex].m_y, source->m_points.m_vertex[vIndex].m_z);
				if (source->m_points.m_layers.m_count) {
					AddLayer (source->m_points.m_layers[vIndex]);
				}

				if (source->m_attrib.m_materialChannel.m_count) {
					AddMaterial (source->m_attrib.m_materialChannel[aIndex]);
				}
				if (source->m_attrib.m_colorChannel.m_count) {
					AddVertexColor(source->m_attrib.m_colorChannel[aIndex].m_x, source->m_attrib.m_colorChannel[aIndex].m_y, source->m_attrib.m_colorChannel[aIndex].m_z, source->m_attrib.m_colorChannel[aIndex].m_w);
				}
				if (source->m_attrib.m_normalChannel.m_count) {
					AddNormal(source->m_attrib.m_normalChannel[aIndex].m_x, source->m_attrib.m_normalChannel[aIndex].m_y, source->m_attrib.m_normalChannel[aIndex].m_z);
				}
				if (source->m_attrib.m_binormalChannel.m_count) {
					AddBinormal(source->m_attrib.m_binormalChannel[aIndex].m_x, source->m_attrib.m_binormalChannel[aIndex].m_y, source->m_attrib.m_binormalChannel[aIndex].m_z);
				}
				if (source->m_attrib.m_uv0Channel.m_count) {
					AddUV0(source->m_attrib.m_uv0Channel[aIndex].m_u, source->m_attrib.m_uv0Channel[aIndex].m_v);
				}
				if (source->m_attrib.m_uv1Channel.m_count) {
					AddUV1(source->m_attrib.m_uv1Channel[aIndex].m_u, source->m_attrib.m_uv1Channel[aIndex].m_v);
				}
				ptr = ptr->m_next;
			} while (ptr != edge);
			EndBuildFace ();
		}
	}
}

bool dgMeshEffect::SeparateDuplicateLoops (dgEdge* const face)
{
	for (dgEdge* ptr0 = face; ptr0 != face->m_prev; ptr0 = ptr0->m_next) {
		dgInt32 index = ptr0->m_incidentVertex;

		dgEdge* ptr1 = ptr0->m_next; 
		do {
			if (ptr1->m_incidentVertex == index) {
				dgEdge* const ptr00 = ptr0->m_prev;
				dgEdge* const ptr11 = ptr1->m_prev;

				ptr00->m_next = ptr1;
				ptr1->m_prev = ptr00;

				ptr11->m_next = ptr0;
				ptr0->m_prev = ptr11;

				return true;
			}

			ptr1 = ptr1->m_next;
		} while (ptr1 != face);
	}
	return false;
}

void dgMeshEffect::RepairTJoints ()
{
	dgAssert (Sanity ());

	// delete edge of zero length
	bool dirty = true;
	while (dirty) {
		dgFloat64 tol = dgFloat64 (1.0e-5f);
		dgFloat64 tol2 = tol * tol;
		dirty = false;
		dgPolyhedra::Iterator iter (*this);
		for (iter.Begin(); iter; ) {
			dgEdge* const edge = &(*iter);
			iter ++;
			const dgBigVector& p0 = m_points.m_vertex[edge->m_incidentVertex];
			const dgBigVector& p1 = m_points.m_vertex[edge->m_twin->m_incidentVertex];
			dgBigVector dist (p1 - p0);
			dgFloat64 mag2 = dist.DotProduct3(dist);
			if (mag2 < tol2) {
				bool move = true;
				while (move) {
					move = false;
					dgEdge* ptr = edge->m_twin;
					do {
						if ((&(*iter) == ptr) || (&(*iter) == ptr->m_twin)) {
							move = true;
							iter ++;
						}
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge->m_twin);

					ptr = edge;
					do {
						if ((&(*iter) == ptr) || (&(*iter) == ptr->m_twin)) {
							move = true;
							iter ++;
						}
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);
				}
				
				dgEdge* const collapsedEdge = CollapseEdge(edge);
				if (collapsedEdge) {
					dgAssert (0);
					dirty = true;
					dgBigVector q (m_points.m_vertex[collapsedEdge->m_incidentVertex]);
					dgEdge* ptr = collapsedEdge;
					do {
						if (ptr->m_incidentFace > 0) {
							dgAssert (0);
							//m_attrib[ptr->m_userData].m_vertex = q;
						}
						ptr = ptr->m_twin->m_next;
					} while (ptr != collapsedEdge);
				}
			}
		}
	}
	dgAssert (Sanity ());

	// repair straight open edges
	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark) != mark && (edge->m_incidentFace < 0)) {

			while (SeparateDuplicateLoops (edge));
			dgEdge* ptr = edge; 
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	dgAssert (Sanity ());
	DeleteDegenerateFaces(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), dgFloat64 (1.0e-7f));
	dgAssert (Sanity ());


	// delete straight line edges
	dirty = true;
	while (dirty) {
		dgFloat64 tol = dgFloat64(1.0 - 1.0e-8);
		dgFloat64 tol2 = tol * tol;

		dirty = false;
		dgAssert (Sanity ());

		dgPolyhedra::Iterator iter1 (*this);
		for (iter1.Begin(); iter1; ) {
			dgEdge* const edge = &(*iter1);
			iter1 ++;

			const dgBigVector& p0 = m_points.m_vertex[edge->m_incidentVertex];
			const dgBigVector& p1 = m_points.m_vertex[edge->m_next->m_incidentVertex];
			const dgBigVector& p2 = m_points.m_vertex[edge->m_next->m_next->m_incidentVertex];

			dgBigVector A (p1 - p0);
			dgBigVector B (p2 - p1);
			dgFloat64 ab = A.DotProduct3(B);
			if (ab >= 0.0f) {
				dgFloat64 aa = A.DotProduct3(A);
				dgFloat64 bb = B.DotProduct3(B);

				dgFloat64 magab2 = ab * ab;
				dgFloat64 magaabb = aa * bb * tol2;
				if (magab2 >= magaabb) {
					if ((edge->m_incidentFace > 0) && (edge->m_twin->m_incidentFace > 0)) {
						if (edge->m_twin->m_prev == edge->m_next->m_twin) {
							dgEdge* const newEdge = AddHalfEdge(edge->m_incidentVertex, edge->m_next->m_next->m_incidentVertex);
							if (newEdge) {
								dirty = true;
								dgEdge* const newTwin = AddHalfEdge(edge->m_next->m_next->m_incidentVertex, edge->m_incidentVertex);
								dgAssert (newEdge);
								dgAssert (newTwin);

								newEdge->m_twin = newTwin;
								newTwin->m_twin = newEdge;

								newEdge->m_userData = edge->m_userData;
								newTwin->m_userData = edge->m_twin->m_prev->m_userData;

								newEdge->m_incidentFace = edge->m_incidentFace;
								newTwin->m_incidentFace = edge->m_twin->m_incidentFace;

								dgEdge* const nextEdge = edge->m_next;

								nextEdge->m_twin->m_prev->m_next = newTwin;
								newTwin->m_prev = nextEdge->m_twin->m_prev;

								edge->m_twin->m_next->m_prev = newTwin;
								newTwin->m_next = edge->m_twin->m_next;

								nextEdge->m_next->m_prev = newEdge;
								newEdge->m_next = nextEdge->m_next;

								edge->m_prev->m_next = newEdge;
								newEdge->m_prev = edge->m_prev;

								while ((&(*iter1) == edge->m_twin) || (&(*iter1) == nextEdge) || (&(*iter1) == nextEdge->m_twin)) {
									iter1 ++;
								}

								nextEdge->m_twin->m_prev = nextEdge;
								nextEdge->m_twin->m_next = nextEdge;
								nextEdge->m_prev = nextEdge->m_twin;
								nextEdge->m_next = nextEdge->m_twin;

								edge->m_twin->m_prev = edge;
								edge->m_twin->m_next = edge;
								edge->m_prev = edge->m_twin;
								edge->m_next = edge->m_twin;

								DeleteEdge(edge);
								DeleteEdge(nextEdge);
								//dgAssert (Sanity ());

							} else if (edge->m_next->m_next->m_next == edge) {
								dgAssert (0);
/*
								dirty = true;
								dgEdge* const openEdge = edge;
								dgEdge* const nextEdge = openEdge->m_next;
								dgEdge* const deletedEdge = openEdge->m_prev;
								while ((&(*iter) == deletedEdge) || (&(*iter) == deletedEdge->m_twin)) {
									iter ++;
								}

								openEdge->m_userData = deletedEdge->m_twin->m_userData;

								dgBigVector p2p0 (p2 - p0);
								dgFloat64 den = p2p0.DotProduct3(p2p0);
								dgFloat64 param1 = p2p0.DotProduct3(p1 - p0) / den;
								dgVertexAtribute attib1 = AddInterpolateEdgeAttibute (deletedEdge->m_twin, param1);
								AddAtribute(attib1);
								openEdge->m_next->m_userData = m_atribCount  - 1;

								openEdge->m_incidentFace = deletedEdge->m_twin->m_incidentFace;
								openEdge->m_next->m_incidentFace = deletedEdge->m_twin->m_incidentFace;

								deletedEdge->m_twin->m_prev->m_next = openEdge;
								openEdge->m_prev = deletedEdge->m_twin->m_prev;

								deletedEdge->m_twin->m_next->m_prev = nextEdge;
								nextEdge->m_next = deletedEdge->m_twin->m_next;

								deletedEdge->m_twin->m_next = deletedEdge;
								deletedEdge->m_twin->m_prev = deletedEdge;
								deletedEdge->m_next = deletedEdge->m_twin;
								deletedEdge->m_prev = deletedEdge->m_twin;
								DeleteEdge(deletedEdge);
								//dgAssert (Sanity ());
*/
							}
						}
					} else if (FindEdge(edge->m_incidentVertex, edge->m_next->m_next->m_incidentVertex)) {
						dgEdge* const openEdge = edge;
						dgAssert (openEdge->m_incidentFace <= 0);
						dgEdge* const nextEdge = openEdge->m_next;
						dgEdge* const deletedEdge = openEdge->m_prev;
						if (deletedEdge == openEdge->m_next->m_next) {
							dirty = true;
							while ((&(*iter1) == deletedEdge) || (&(*iter1) == deletedEdge->m_twin)) {
								iter1 ++;
							}

							dgAssert (deletedEdge->m_twin->m_incidentFace > 0);
							openEdge->m_incidentFace = deletedEdge->m_twin->m_incidentFace;
							openEdge->m_next->m_incidentFace = deletedEdge->m_twin->m_incidentFace;

							dgInt32 attibuteIndex = AddInterpolatedHalfAttribute(deletedEdge->m_twin, nextEdge->m_incidentVertex);
							openEdge->m_next->m_userData = attibuteIndex;
							openEdge->m_userData = deletedEdge->m_twin->m_userData;
							
							deletedEdge->m_twin->m_prev->m_next = openEdge;
							openEdge->m_prev = deletedEdge->m_twin->m_prev;

							deletedEdge->m_twin->m_next->m_prev = nextEdge;
							nextEdge->m_next = deletedEdge->m_twin->m_next;

							deletedEdge->m_twin->m_next = deletedEdge;
							deletedEdge->m_twin->m_prev = deletedEdge;
							deletedEdge->m_next = deletedEdge->m_twin;
							deletedEdge->m_prev = deletedEdge->m_twin;
							DeleteEdge(deletedEdge);
							dgAssert (Sanity ());
						}

					} else {
						dgEdge* const openEdge = (edge->m_incidentFace <= 0) ? edge : edge->m_twin;
						dgAssert (openEdge->m_incidentFace <= 0);

						const dgBigVector& p3 = m_points.m_vertex[openEdge->m_next->m_next->m_next->m_incidentVertex];

						dgBigVector A0 (p3 - p2);
						dgBigVector B0 (p2 - p1);
						dgFloat64 ab0 = A0.DotProduct3(B0);
						if (ab0 >= 0.0) {
							dgFloat64 aa0 = A0.DotProduct3(A0);
							dgFloat64 bb0 = B0.DotProduct3(B0);

							dgFloat64 ab0ab0 = ab0 * ab0;
							dgFloat64 aa0bb0 = aa0 * bb0 * tol2;
							if (ab0ab0 >= aa0bb0) {
								if (openEdge->m_next->m_next->m_next->m_next != openEdge) {
									const dgBigVector& p4 = m_points.m_vertex[openEdge->m_prev->m_incidentVertex];
									dgBigVector A1 (p1 - p0);
									dgBigVector B1 (p1 - p4);
									dgFloat64 ab1 = A1.DotProduct3(B1);
									if (ab1 < 0.0f) {
										dgFloat64 ab1ab1 = ab1 * ab1;
										dgFloat64 aa1bb1 = aa0 * bb0 * tol2;
										if (ab1ab1 >= aa1bb1) {
											dgEdge* const newFace = ConnectVertex (openEdge->m_prev, openEdge->m_next);
											dirty |= newFace ? true : false;
										}
									}
									//dgAssert (Sanity ());
								} else if (openEdge->m_prev->m_twin->m_incidentFace > 0) {
									dirty = true;
									
									dgEdge* const deletedEdge = openEdge->m_prev;
									while ((&(*iter1) == deletedEdge) || (&(*iter1) == deletedEdge->m_twin)) {
										iter1 ++;
									}

									openEdge->m_incidentFace = deletedEdge->m_twin->m_incidentFace;
									openEdge->m_next->m_incidentFace = deletedEdge->m_twin->m_incidentFace;
									openEdge->m_next->m_next->m_incidentFace = deletedEdge->m_twin->m_incidentFace;

									dgInt32 attibuteIndex0 = AddInterpolatedHalfAttribute(deletedEdge->m_twin, openEdge->m_next->m_incidentVertex);
									dgInt32 attibuteIndex1 = AddInterpolatedHalfAttribute(deletedEdge->m_twin, openEdge->m_next->m_next->m_incidentVertex);
									
									openEdge->m_userData = deletedEdge->m_twin->m_userData;
									openEdge->m_next->m_userData = attibuteIndex0;
									openEdge->m_next->m_next->m_userData = attibuteIndex1;


									deletedEdge->m_twin->m_prev->m_next = openEdge;
									openEdge->m_prev = deletedEdge->m_twin->m_prev;

									deletedEdge->m_twin->m_next->m_prev = deletedEdge->m_prev;
									deletedEdge->m_prev->m_next = deletedEdge->m_twin->m_next;
									
									deletedEdge->m_twin->m_next = deletedEdge;
									deletedEdge->m_twin->m_prev = deletedEdge;
									deletedEdge->m_next = deletedEdge->m_twin;
									deletedEdge->m_prev = deletedEdge->m_twin;
									DeleteEdge(deletedEdge);
									//dgAssert (Sanity ());
								}
							}
						}
					}
				}
			}
		}
	}
	dgAssert (Sanity ());

	DeleteDegenerateFaces(&m_points.m_vertex[0].m_x, sizeof (dgBigVector), dgFloat64 (1.0e-7f));
/*
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		if (edge->m_incidentFace > 0) {
			dgBigVector p0 (m_points[edge->m_incidentVertex]);
			m_attrib[edge->m_userData].m_vertex.m_x = p0.m_x;
			m_attrib[edge->m_userData].m_vertex.m_y = p0.m_y;
			m_attrib[edge->m_userData].m_vertex.m_z = p0.m_z;
		}
	}
*/
	dgAssert (Sanity ());
}

