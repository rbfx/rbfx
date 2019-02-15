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
#include "dgObb.h"
#include "dgHeap.h"
#include "dgDebug.h"
#include "dgStack.h"
#include "dgPolyhedra.h"
#include "dgConvexHull3d.h"
#include "dgSmallDeterminant.h"


#define DG_LOCAL_BUFFER_SIZE  1024

class dgDiagonalEdge
{
	public:
	dgDiagonalEdge (dgEdge* const edge)
		:m_i0(edge->m_incidentVertex), m_i1(edge->m_twin->m_incidentVertex)
	{
	}
	dgInt32 m_i0;
	dgInt32 m_i1;
};


struct dgEdgeCollapseEdgeHandle
{
	dgEdgeCollapseEdgeHandle (dgEdge* const newEdge)
		:m_inList(false), m_edge(newEdge)
	{
	}

	dgEdgeCollapseEdgeHandle (const dgEdgeCollapseEdgeHandle &dataHandle)
		:m_inList(true), m_edge(dataHandle.m_edge)
	{
		dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *)IntToPointer (m_edge->m_userData);
		if (handle) {
			dgAssert (handle != this);
			handle->m_edge = NULL;
		}
		m_edge->m_userData = dgUnsigned64 (PointerToInt(this));
	}

	~dgEdgeCollapseEdgeHandle ()
	{
		if (m_inList) {
			if (m_edge) {
				dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *)IntToPointer (m_edge->m_userData);
				if (handle == this) {
					m_edge->m_userData = PointerToInt (NULL);
				}
			}
		}
		m_edge = NULL;
	}

	dgUnsigned32 m_inList;
	dgEdge* m_edge;
};


class dgVertexCollapseVertexMetric
{
	public:
	dgVertexCollapseVertexMetric (const dgBigPlane &plane) 
	{
		elem[0] = plane.m_x * plane.m_x;  
		elem[1] = plane.m_y * plane.m_y;  
		elem[2] = plane.m_z * plane.m_z;  
		elem[3] = plane.m_w * plane.m_w;  
		elem[4] = dgFloat64 (2.0) * plane.m_x * plane.m_y;  
		elem[5] = dgFloat64 (2.0) * plane.m_x * plane.m_z;  
		elem[6] = dgFloat64 (2.0) * plane.m_x * plane.m_w;  
		elem[7] = dgFloat64 (2.0) * plane.m_y * plane.m_z;  
		elem[8] = dgFloat64 (2.0) * plane.m_y * plane.m_w;  
		elem[9] = dgFloat64 (2.0) * plane.m_z * plane.m_w;  
	}

	void Clear ()
	{
		memset (elem, 0, 10 * sizeof (dgFloat64));
	}

	void Accumulate (const dgVertexCollapseVertexMetric& p) 
	{
		elem[0] += p.elem[0]; 
		elem[1] += p.elem[1]; 
		elem[2] += p.elem[2]; 
		elem[3] += p.elem[3]; 
		elem[4] += p.elem[4]; 
		elem[5] += p.elem[5]; 
		elem[6] += p.elem[6]; 
		elem[7] += p.elem[7]; 
		elem[8] += p.elem[8]; 
		elem[9] += p.elem[9]; 
	}

	void Accumulate (const dgBigPlane& plane) 
	{
		elem[0] += plane.m_x * plane.m_x;  
		elem[1] += plane.m_y * plane.m_y;  
		elem[2] += plane.m_z * plane.m_z;  
		elem[3] += plane.m_w * plane.m_w;  

		elem[4] += dgFloat64 (2.0f) * plane.m_x * plane.m_y;  
		elem[5] += dgFloat64 (2.0f) * plane.m_x * plane.m_z;  
		elem[7] += dgFloat64 (2.0f) * plane.m_y * plane.m_z;  

		elem[6] += dgFloat64 (2.0f) * plane.m_x * plane.m_w;  
		elem[8] += dgFloat64 (2.0f) * plane.m_y * plane.m_w;  
		elem[9] += dgFloat64 (2.0f) * plane.m_z * plane.m_w;  
	}


	dgFloat64 Evalue (const dgBigVector &p) const 
	{
		dgFloat64 acc = elem[0] * p.m_x * p.m_x + elem[1] * p.m_y * p.m_y + elem[2] * p.m_z * p.m_z + 
						elem[4] * p.m_x * p.m_y + elem[5] * p.m_x * p.m_z + elem[7] * p.m_y * p.m_z + 
						elem[6] * p.m_x + elem[8] * p.m_y + elem[9] * p.m_z + elem[3];  
		return fabs (acc);
	}

	dgFloat64 elem[10];
};



dgPolyhedra::dgPolyhedra (dgMemoryAllocator* const allocator)
	:dgTree <dgEdge, dgInt64>(allocator)
	,m_baseMark(0)
	,m_edgeMark(0)
	,m_faceSecuence(0)
{
}

dgPolyhedra::dgPolyhedra (const dgPolyhedra &polyhedra)
	:dgTree <dgEdge, dgInt64>(polyhedra.GetAllocator())
	,m_baseMark(0)
	,m_edgeMark(0)
	,m_faceSecuence(0)
{
	dgStack<dgInt32> indexPool (DG_LOCAL_BUFFER_SIZE * 16);
	dgStack<dgUnsigned64> userPool (DG_LOCAL_BUFFER_SIZE * 16);
	dgInt32* const index = &indexPool[0];
	dgUnsigned64* const user = &userPool[0];

	BeginFace ();
	Iterator iter(polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			continue;
		}

		if (!FindEdge(edge->m_incidentVertex, edge->m_twin->m_incidentVertex))	{
			dgInt32 indexCount = 0;
			dgEdge* ptr = edge;
			do {
				user[indexCount] = ptr->m_userData;
				index[indexCount] = ptr->m_incidentVertex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != edge);

			dgEdge* const face = AddFace (indexCount, index, (dgInt64*) user);
			ptr = face;
			do {
				ptr->m_incidentFace = edge->m_incidentFace;
				ptr = ptr->m_next;
			} while (ptr != face);
		}
	}
	EndFace();

	m_faceSecuence = polyhedra.m_faceSecuence;

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck());
#endif
}

dgPolyhedra::~dgPolyhedra ()
{
}


dgInt32 dgPolyhedra::GetFaceCount() const
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

		count ++;
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}
	return count;
}



dgEdge* dgPolyhedra::AddFace ( dgInt32 count, const dgInt32* const index, const dgInt64* const userdata)
{
	class IntersectionFilter
	{
		public:
		IntersectionFilter ()
		{
			m_count = 0;
		}

		bool Insert (dgInt64 value)
		{
			dgInt32 i = 0;				
			for (; i < m_count; i ++) {
				if (m_array[i] == value) {
					return false;
				}
			}
			m_array[i] = value;
			m_count ++;
			return true;
		}

		dgInt32 m_count;
		dgInt64 m_array[2048];
	};

	IntersectionFilter selfIntersectingFaceFilter;

	dgInt32 i0 = index[count-1];
	for (dgInt32 i = 0; i < count; i ++) {
		dgInt32 i1 = index[i];

		dgPairKey code0 (i0, i1);
		if (!selfIntersectingFaceFilter.Insert (code0.GetVal())) {
			return NULL;
		}

		dgPairKey code1 (i1, i0);
		if (!selfIntersectingFaceFilter.Insert (code1.GetVal())) {
			return NULL;
		}

		if (i0 == i1) {
			return NULL;
		}
		if (FindEdge (i0, i1)) {
			return NULL;
		}
		i0 = i1;
	}

	m_faceSecuence ++;

	i0 = index[count-1];
	dgInt32 i1 = index[0];
	dgUnsigned64 udata0 = 0;
	dgUnsigned64 udata1 = 0;
	if (userdata) {
		udata0 = dgUnsigned64 (userdata[count-1]);
		udata1 = dgUnsigned64 (userdata[0]);
	} 

	bool state;
	dgPairKey code (i0, i1);
	dgEdge tmpEdge (i0, m_faceSecuence, udata0);
	dgTreeNode* const node = Insert (tmpEdge, code.GetVal(), state); 
	dgAssert (!state);
	dgEdge* edge0 = &node->GetInfo();
	dgEdge* const first = edge0;

	for (dgInt32 i = 1; i < count; i ++) {
		i0 = i1;
		i1 = index[i];
		udata0 = udata1;
		udata1 = dgUnsigned64 (userdata ? userdata[i] : 0);

		dgPairKey code1 (i0, i1);
		dgEdge tmpEdge1 (i0, m_faceSecuence, udata0);
		dgTreeNode* const node1 = Insert (tmpEdge1, code1.GetVal(), state); 
		dgAssert (!state);

		dgEdge* const edge1 = &node1->GetInfo();
		edge0->m_next = edge1;
		edge1->m_prev = edge0;
		edge0 = edge1;
	}

	first->m_prev = edge0;
	edge0->m_next = first;

	return first->m_next;
}


bool dgPolyhedra::EndFace ()
{
	dgPolyhedra::Iterator iter (*this);

	// Connect all twin edge
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (!edge->m_twin) {
			edge->m_twin = FindEdge (edge->m_next->m_incidentVertex, edge->m_incidentVertex);
			if (edge->m_twin) {
				edge->m_twin->m_twin = edge; 
			}
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck());
#endif
	dgStack<dgEdge*> edgeArrayPool(GetCount() * 2 + 256);

	dgInt32 edgeCount = 0;
	dgEdge** const edgeArray = &edgeArrayPool[0];
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (!edge->m_twin) {
			bool state;
			dgPolyhedra::dgPairKey code (edge->m_next->m_incidentVertex, edge->m_incidentVertex);
			dgEdge tmpEdge (edge->m_next->m_incidentVertex, -1);
			tmpEdge.m_incidentFace = -1; 
			dgPolyhedra::dgTreeNode* const node = Insert (tmpEdge, code.GetVal(), state); 
			dgAssert (!state);
			edge->m_twin = &node->GetInfo();
			edge->m_twin->m_twin = edge; 
			edgeArray[edgeCount] = edge->m_twin;
			edgeCount ++;
		}
	}

	for (dgInt32 i = 0; i < edgeCount; i ++) {
		dgEdge* const edge = edgeArray[i];
		dgAssert (!edge->m_prev);
		dgEdge *ptr = edge->m_twin;
		for (; ptr->m_next; ptr = ptr->m_next->m_twin){}
		ptr->m_next = edge;
		edge->m_prev = ptr;
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif

	return true;
}


void dgPolyhedra::DeleteFace(dgEdge* const face)
{
	dgEdge* edgeList[DG_LOCAL_BUFFER_SIZE * 16];

	if (face->m_incidentFace > 0) {
		dgInt32 count = 0;
		dgEdge* ptr = face;
		do {
			ptr->m_incidentFace = -1;
			dgInt32 i = 0;
			for (; i < count; i ++) {
				if ((edgeList[i] == ptr) || (edgeList[i]->m_twin == ptr)) {
					break;
				}
			}
			if (i == count) {
				edgeList[count] = ptr;
				count ++;
			}
			ptr = ptr->m_next;
		} while (ptr != face);


		for (dgInt32 i = 0; i < count; i ++) {
			dgEdge* const ptr1 = edgeList[i];
			if (ptr1->m_twin->m_incidentFace < 0) {
				DeleteEdge (ptr1);
			}
		}
	}
}

dgBigVector dgPolyhedra::FaceNormal (const dgEdge* const face, const dgFloat64* const pool, dgInt32 strideInBytes) const
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));
	const dgEdge* edge = face;
	dgBigVector p0 (&pool[edge->m_incidentVertex * stride]);
	edge = edge->m_next;
	dgBigVector p1 (&pool[edge->m_incidentVertex * stride]);
	dgBigVector e1 (p1 - p0);

	dgBigVector normal (dgFloat32 (0.0f));
	for (edge = edge->m_next; edge != face; edge = edge->m_next) {
		dgBigVector p2 (&pool[edge->m_incidentVertex * stride]);
		dgBigVector e2 (p2 - p0);
		normal += e1.CrossProduct(e2);
		e1 = e2;
	} 
	dgAssert (normal.m_w == dgFloat32 (0.0f));
	return normal;
}


dgEdge* dgPolyhedra::AddHalfEdge (dgInt32 v0, dgInt32 v1)
{
	if (v0 != v1) {
		dgPairKey pairKey (v0, v1);
		dgEdge tmpEdge (v0, -1);

		dgTreeNode* node = Insert (tmpEdge, pairKey.GetVal()); 
		return node ? &node->GetInfo() : NULL;
	} else {
		return NULL;
	}
}


void dgPolyhedra::DeleteEdge (dgEdge* const edge)
{
	dgEdge *const twin = edge->m_twin;

	edge->m_prev->m_next = twin->m_next;
	twin->m_next->m_prev = edge->m_prev;
	edge->m_next->m_prev = twin->m_prev;
	twin->m_prev->m_next = edge->m_next;

	dgTreeNode *const nodeA = GetNodeFromInfo (*edge);
	dgTreeNode *const nodeB = GetNodeFromInfo (*twin);

	dgAssert (&nodeA->GetInfo() == edge);
	dgAssert (&nodeB->GetInfo() == twin);

	Remove (nodeA);
	Remove (nodeB);
}


dgEdge* dgPolyhedra::ConnectVertex (dgEdge* const e0, dgEdge* const e1)
{
	dgEdge* const edge = AddHalfEdge(e1->m_incidentVertex, e0->m_incidentVertex);
	dgEdge* const twin = AddHalfEdge(e0->m_incidentVertex, e1->m_incidentVertex);
	dgAssert ((edge && twin) || !(edge || twin));
	if (edge) {
		edge->m_twin = twin;
		twin->m_twin = edge;

		edge->m_incidentFace = e0->m_incidentFace;
		twin->m_incidentFace = e1->m_incidentFace;

		edge->m_userData = e1->m_userData;
		twin->m_userData = e0->m_userData;

		edge->m_next = e0;
		edge->m_prev = e1->m_prev;

		twin->m_next = e1;
		twin->m_prev = e0->m_prev;

		e0->m_prev->m_next = twin;
		e0->m_prev = edge;

		e1->m_prev->m_next = edge;
		e1->m_prev = twin;
	}

	return edge;
}

dgEdge* dgPolyhedra::SpliteEdge (dgInt32 newIndex,	dgEdge* const edge)
{
	dgEdge* const edge00 = edge->m_prev;
	dgEdge* const edge01 = edge->m_next;
	dgEdge* const twin00 = edge->m_twin->m_next;
	dgEdge* const twin01 = edge->m_twin->m_prev;

	dgInt32 i0 = edge->m_incidentVertex;
	dgInt32 i1 = edge->m_twin->m_incidentVertex;

	dgInt32 f0 = edge->m_incidentFace;
	dgInt32 f1 = edge->m_twin->m_incidentFace;

	DeleteEdge (edge);

	dgEdge* const edge0 = AddHalfEdge (i0, newIndex);
	dgEdge* const edge1 = AddHalfEdge (newIndex, i1);

	dgEdge* const twin0 = AddHalfEdge (newIndex, i0);
	dgEdge* const twin1 = AddHalfEdge (i1, newIndex);
	dgAssert (edge0);
	dgAssert (edge1);
	dgAssert (twin0);
	dgAssert (twin1);

	edge0->m_twin = twin0;
	twin0->m_twin = edge0;

	edge1->m_twin = twin1;
	twin1->m_twin = edge1;

	edge0->m_next = edge1;
	edge1->m_prev = edge0;

	twin1->m_next = twin0;
	twin0->m_prev = twin1;

	edge0->m_prev = edge00;
	edge00 ->m_next = edge0;

	edge1->m_next = edge01;
	edge01->m_prev = edge1;

	twin0->m_next = twin00;
	twin00->m_prev = twin0;

	twin1->m_prev = twin01;
	twin01->m_next = twin1;

	edge0->m_incidentFace = f0;
	edge1->m_incidentFace = f0;

	twin0->m_incidentFace = f1;
	twin1->m_incidentFace = f1;

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	//	dgAssert (SanityCheck ());
#endif

	return edge0;
}



bool dgPolyhedra::FlipEdge (dgEdge* const edge)
{
	//	dgTreeNode *node;
	if (edge->m_next->m_next->m_next != edge) {
		return false;
	}

	if (edge->m_twin->m_next->m_next->m_next != edge->m_twin) {
		return false;
	}

	if (FindEdge(edge->m_prev->m_incidentVertex, edge->m_twin->m_prev->m_incidentVertex)) {
		return false;
	}

	dgEdge *const prevEdge = edge->m_prev;
	dgEdge *const prevTwin = edge->m_twin->m_prev;

	dgPairKey edgeKey (prevTwin->m_incidentVertex, prevEdge->m_incidentVertex);
	dgPairKey twinKey (prevEdge->m_incidentVertex, prevTwin->m_incidentVertex);

	ReplaceKey (GetNodeFromInfo (*edge), edgeKey.GetVal());
	//	dgAssert (node);

	ReplaceKey (GetNodeFromInfo (*edge->m_twin), twinKey.GetVal());
	//	dgAssert (node);

	edge->m_incidentVertex = prevTwin->m_incidentVertex;
	edge->m_twin->m_incidentVertex = prevEdge->m_incidentVertex;

	edge->m_userData = prevTwin->m_userData;
	edge->m_twin->m_userData = prevEdge->m_userData;

	prevEdge->m_next = edge->m_twin->m_next;
	prevTwin->m_prev->m_prev = edge->m_prev;

	prevTwin->m_next = edge->m_next;
	prevEdge->m_prev->m_prev = edge->m_twin->m_prev;

	edge->m_prev = prevTwin->m_prev;
	edge->m_next = prevEdge;

	edge->m_twin->m_prev = prevEdge->m_prev;
	edge->m_twin->m_next = prevTwin;

	prevTwin->m_prev->m_next = edge;
	prevTwin->m_prev = edge->m_twin;

	prevEdge->m_prev->m_next = edge->m_twin;
	prevEdge->m_prev = edge;

	edge->m_next->m_incidentFace = edge->m_incidentFace;
	edge->m_prev->m_incidentFace = edge->m_incidentFace;

	edge->m_twin->m_next->m_incidentFace = edge->m_twin->m_incidentFace;
	edge->m_twin->m_prev->m_incidentFace = edge->m_twin->m_incidentFace;


#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif

	return true;
}



bool dgPolyhedra::GetConectedSurface (dgPolyhedra &polyhedra) const
{
	if (!GetCount()) {
		return false;
	}

	dgEdge* edge = NULL;
	Iterator iter(*this);
	for (iter.Begin (); iter; iter ++) {
		edge = &(*iter);
		if ((edge->m_mark < m_baseMark) && (edge->m_incidentFace > 0)) {
			break;
		}
	}

	if (!iter) {
		return false;
	}

	dgInt32 faceIndex[4096];
	dgInt64 faceDataIndex[4096];
	dgStack<dgEdge*> stackPool (GetCount()); 
	dgEdge** const stack = &stackPool[0];

	dgInt32 mark = IncLRU();

	polyhedra.BeginFace ();
	stack[0] = edge;
	dgInt32 index = 1;
	while (index) {
		index --;
		dgEdge* const edge1 = stack[index];
		dgAssert (edge1);
		if (edge1->m_mark == mark) {
			continue;
		}

		dgInt32 count = 0;
		dgEdge* ptr = edge1;
		do {
			dgAssert (ptr);
			ptr->m_mark = mark;
			faceIndex[count] = ptr->m_incidentVertex;
			faceDataIndex[count] = dgInt64 (ptr->m_userData);
			count ++;
			dgAssert (count <  dgInt32 ((sizeof (faceIndex)/sizeof(faceIndex[0]))));

			if ((ptr->m_twin->m_incidentFace > 0) && (ptr->m_twin->m_mark != mark)) {
				stack[index] = ptr->m_twin;
				index ++;
				dgAssert (index < GetCount());
			}

			ptr = ptr->m_next;
		} while (ptr != edge1);

		polyhedra.AddFace (count, &faceIndex[0], &faceDataIndex[0]);
	}

	polyhedra.EndFace ();

	return true;
}


void dgPolyhedra::ChangeEdgeIncidentVertex (dgEdge* const edge, dgInt32 newIndex)
{
	dgEdge* ptr = edge;
	do {
		dgTreeNode* node = GetNodeFromInfo(*ptr);
		dgPairKey Key0 (newIndex, ptr->m_twin->m_incidentVertex);
		ReplaceKey (node, Key0.GetVal());

		node = GetNodeFromInfo(*ptr->m_twin);
		dgPairKey Key1 (ptr->m_twin->m_incidentVertex, newIndex);
		ReplaceKey (node, Key1.GetVal());

		ptr->m_incidentVertex = newIndex;

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);
}


void dgPolyhedra::DeleteDegenerateFaces (const dgFloat64* const pool, dgInt32 strideInBytes, dgFloat64 area)
{
	if (!GetCount()) {
		return;
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	dgStack <dgPolyhedra::dgTreeNode*> faceArrayPool(GetCount() / 2 + 100);

	dgInt32 count = 0;
	dgPolyhedra::dgTreeNode** const faceArray = &faceArrayPool[0];
	dgInt32 mark = IncLRU();
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		if ((edge->m_mark != mark) && (edge->m_incidentFace > 0)) {
			faceArray[count] = iter.GetNode();
			count ++;
			dgEdge* ptr = edge;
			do	{
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	dgFloat64 area2 = area * area;
	area2 *= dgFloat64 (4.0f);

	for (dgInt32 i = 0; i < count; i ++) {
		dgPolyhedra::dgTreeNode* const faceNode = faceArray[i];
		dgEdge* const edge = &faceNode->GetInfo();

		dgBigVector normal (FaceNormal (edge, pool, strideInBytes));

		dgFloat64 faceArea = normal.DotProduct(normal).GetScalar();
		if (faceArea < area2) {
			DeleteFace (edge);
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark != mark) && (edge->m_incidentFace > 0)) {
			//dgAssert (edge->m_next->m_next->m_next == edge);
			dgEdge* ptr = edge;
			do	{
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);

			dgBigVector normal (FaceNormal (edge, pool, strideInBytes));

			dgFloat64 faceArea = normal.DotProduct(normal).GetScalar();
			dgAssert (faceArea >= area2);
		}
	}
	dgAssert (SanityCheck ());
#endif
}


dgBigPlane dgPolyhedra::UnboundedLoopPlane (dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const pool)
{
	const dgBigVector p0 = pool[i0];
	const dgBigVector p1 = pool[i1];
	const dgBigVector p2 = pool[i2];
	dgBigVector E0 (p1 - p0); 
	dgBigVector E1 (p2 - p0); 

	dgBigVector N ((E0.CrossProduct(E1)).CrossProduct(E0)); 
	dgFloat64 dist = - N.DotProduct3(p0);
	dgBigPlane plane (N, dist);

	dgFloat64 mag = sqrt (plane.DotProduct3(plane));
	if (mag < dgFloat64 (1.0e-12f)) {
		mag = dgFloat64 (1.0e-12f);
	}
	mag = dgFloat64 (10.0f) / mag;

	plane.m_x *= mag;
	plane.m_y *= mag;
	plane.m_z *= mag;
	plane.m_w *= mag;

	return plane;
}

dgEdge* dgPolyhedra::CollapseEdge(dgEdge* const edge)
{
	dgInt32 v0 = edge->m_incidentVertex;
	dgInt32 v1 = edge->m_twin->m_incidentVertex;

	dgEdge* retEdge = edge->m_twin->m_prev->m_twin;
	if (retEdge	== edge->m_twin->m_next) {
		return NULL;
	}
	if (retEdge	== edge->m_twin) {
		return NULL;
	}
	if (retEdge	== edge->m_next) {
		retEdge = edge->m_prev->m_twin;
		if (retEdge	== edge->m_twin->m_next) {
			return NULL;
		}
		if (retEdge	== edge->m_twin) {
			return NULL;
		}
	}

	dgEdge* lastEdge = NULL;
	dgEdge* firstEdge = NULL;
	if ((edge->m_incidentFace >= 0)	&& (edge->m_twin->m_incidentFace >= 0)) {	
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
	} else if (edge->m_twin->m_incidentFace >= 0) {
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
		lastEdge = edge;
	} else {
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next;
	}

	for (dgEdge* ptr = firstEdge; ptr != lastEdge; ptr = ptr->m_twin->m_next) {
		dgEdge* const badEdge = FindEdge (edge->m_twin->m_incidentVertex, ptr->m_twin->m_incidentVertex);
		if (badEdge) {
			return NULL;
		}
	} 

	dgEdge* const twin = edge->m_twin;
	if (twin->m_next == twin->m_prev->m_prev) {
		twin->m_prev->m_twin->m_twin = twin->m_next->m_twin;
		twin->m_next->m_twin->m_twin = twin->m_prev->m_twin;

		Remove (GetNodeFromInfo(*twin->m_prev));
		Remove (GetNodeFromInfo(*twin->m_next));
	} else {
		twin->m_next->m_userData = twin->m_userData;
		twin->m_next->m_prev = twin->m_prev;
		twin->m_prev->m_next = twin->m_next;
	}

	if (edge->m_next == edge->m_prev->m_prev) {
		edge->m_next->m_twin->m_twin = edge->m_prev->m_twin;
		edge->m_prev->m_twin->m_twin = edge->m_next->m_twin;
		Remove (GetNodeFromInfo(*edge->m_next));
		Remove (GetNodeFromInfo(*edge->m_prev));
	} else {
		edge->m_next->m_prev = edge->m_prev;
		edge->m_prev->m_next = edge->m_next;
	}

	dgAssert (twin->m_twin->m_incidentVertex == v0);
	dgAssert (edge->m_twin->m_incidentVertex == v1);
	Remove (GetNodeFromInfo(*twin));
	Remove (GetNodeFromInfo(*edge));

	dgEdge* ptr = retEdge;
	do {
		dgPolyhedra::dgPairKey pairKey (v0, ptr->m_twin->m_incidentVertex);

		dgPolyhedra::dgTreeNode* node = Find (pairKey.GetVal());
		if (node) {
			if (&node->GetInfo() == ptr) {
				dgPolyhedra::dgPairKey key (v1, ptr->m_twin->m_incidentVertex);
				ptr->m_incidentVertex = v1;
				node = ReplaceKey (node, key.GetVal());
				dgAssert (node);
			} 
		}

		dgPolyhedra::dgPairKey TwinKey (ptr->m_twin->m_incidentVertex, v0);
		node = Find (TwinKey.GetVal());
		if (node) {
			if (&node->GetInfo() == ptr->m_twin) {
				dgPolyhedra::dgPairKey key (ptr->m_twin->m_incidentVertex, v1);
				node = ReplaceKey (node, key.GetVal());
				dgAssert (node);
			}
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != retEdge);

	return retEdge;
}



void dgPolyhedra::RemoveHalfEdge (dgEdge* const edge)
{
	dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *) IntToPointer (edge->m_userData);
	if (handle) { 
		handle->m_edge = NULL;
	}

	dgPolyhedra::dgTreeNode* const node = GetNodeFromInfo(*edge);
	dgAssert (node);
	Remove (node);
}


dgEdge* dgPolyhedra::FindEarTip (dgEdge* const face, const dgFloat64* const pool, dgInt32 stride, dgDownHeap<dgEdge*, dgFloat64>& heap, const dgBigVector &normal) const
{
	dgEdge* ptr = face;
	dgBigVector p0 (&pool[ptr->m_prev->m_incidentVertex * stride]);
	dgBigVector p1 (&pool[ptr->m_incidentVertex * stride]);
	dgBigVector d0 (p1 - p0);
	dgFloat64 val = sqrt (d0.DotProduct3(d0));
	if (val < dgFloat64 (1.0e-10f)) {
		val = dgFloat64 (1.0e-10f);
	}
	d0 = d0.Scale (dgFloat64 (1.0f) / val);

	dgFloat64 minAngle = dgFloat32 (10.0f);
	do {
		dgBigVector p2 (&pool [ptr->m_next->m_incidentVertex * stride]);
		dgBigVector d1 (p2 - p1);
		dgFloat64 val1 = dgFloat64 (1.0f) / sqrt (d1.DotProduct3(d1));
		if (val1 < dgFloat64 (1.0e-10f)) {
			val1 = dgFloat64 (1.0e-10f);
		}
		d1 = d1.Scale (dgFloat32 (1.0f) / val1);
		dgBigVector n (d0.CrossProduct(d1));

		dgFloat64 angle = normal.DotProduct3(n);
		if (angle >= dgFloat64 (0.0f)) {
			heap.Push (ptr, angle);
		}

		if (angle < minAngle) {
			minAngle = angle;
		}

		d0 = d1;
		p1 = p2;
		ptr = ptr->m_next;
	} while (ptr != face);

	if (minAngle > dgFloat32 (0.1f)) {
		return heap[0];
	}

	dgEdge* ear = NULL;
	while (heap.GetCount()) {
		ear = heap[0];
		heap.Pop();

		if (FindEdge (ear->m_prev->m_incidentVertex, ear->m_next->m_incidentVertex)) {
			continue;
		}

		dgBigVector q0 (&pool [ear->m_prev->m_incidentVertex * stride]);
		dgBigVector q1 (&pool [ear->m_incidentVertex * stride]);
		dgBigVector q2 (&pool [ear->m_next->m_incidentVertex * stride]);

		dgBigVector p10 (q1 - q0);
		dgBigVector p21 (q2 - q1);
		dgBigVector p02 (q0 - q2);

		for (ptr = ear->m_next->m_next; ptr != ear->m_prev; ptr = ptr->m_next) {
			if (!((ptr->m_incidentVertex == ear->m_incidentVertex) || (ptr->m_incidentVertex == ear->m_prev->m_incidentVertex) || (ptr->m_incidentVertex == ear->m_next->m_incidentVertex))) { 
				dgBigVector p (&pool [ptr->m_incidentVertex * stride]);

				//dgFloat64 side = ((p - p0) * p10) % normal;
				dgFloat64 side = normal.DotProduct3((p - q0).CrossProduct(p10));
				if (side < dgFloat64 (0.05f)) {
					//side = ((p - p1) * p21) % normal;
					side = normal.DotProduct3((p - q1).CrossProduct(p21));
					if (side < dgFloat64 (0.05f)) {
						//side = ((p - p2) * p02) % normal;
						side = normal.DotProduct3((p - q2).CrossProduct(p02));
						if (side < dgFloat32 (0.05f)) {
							break;

						}
					}
				}
			}
		}

		if (ptr == ear->m_prev) {
			break;
		}
	}

	return ear;
}



dgEdge* dgPolyhedra::TriangulateFace (dgEdge* const faceIn, const dgFloat64* const pool, dgInt32 stride, dgDownHeap<dgEdge*, dgFloat64>& heap, dgBigVector* const faceNormalOut)
{
	dgEdge* face = faceIn;
	dgBigVector normal (FaceNormal (face, pool, dgInt32 (stride * sizeof (dgFloat64))));

	dgFloat64 dot = normal.DotProduct3(normal);
	if (dot < dgFloat64 (1.0e-12f)) {
		if (faceNormalOut) {
			*faceNormalOut = dgBigVector (dgFloat32 (0.0f)); 
		}
		return face;
	}
	normal = normal.Scale (dgFloat64 (1.0f) / sqrt (dot));
	if (faceNormalOut) {
		*faceNormalOut = normal;
	}

	while (face->m_next->m_next->m_next != face) {
		dgEdge* const ear = FindEarTip (face, pool, stride, heap, normal); 
		if (!ear) {
			return face;
		}
		if ((face == ear)	|| (face == ear->m_prev)) {
			face = ear->m_prev->m_prev;
		}
		dgEdge* const edge = AddHalfEdge (ear->m_next->m_incidentVertex, ear->m_prev->m_incidentVertex);
		if (!edge) {
			return face;
		}
		dgEdge* const twin = AddHalfEdge (ear->m_prev->m_incidentVertex, ear->m_next->m_incidentVertex);
		if (!twin) {
			return face;
		}
		dgAssert (twin);


		edge->m_mark = ear->m_mark;
		edge->m_userData = ear->m_next->m_userData;
		edge->m_incidentFace = ear->m_incidentFace;

		twin->m_mark = ear->m_mark;
		twin->m_userData = ear->m_prev->m_userData;
		twin->m_incidentFace = ear->m_incidentFace;

		edge->m_twin = twin;
		twin->m_twin = edge;

		twin->m_prev = ear->m_prev->m_prev;
		twin->m_next = ear->m_next;
		ear->m_prev->m_prev->m_next = twin;
		ear->m_next->m_prev = twin;

		edge->m_next = ear->m_prev;
		edge->m_prev = ear;
		ear->m_prev->m_prev = edge;
		ear->m_next = edge;

		heap.Flush ();
	}
	return NULL;
}


void dgPolyhedra::MarkAdjacentCoplanarFaces (dgPolyhedra& polyhedraOut, dgEdge* const face, const dgFloat64* const pool, dgInt32 strideInBytes)
{
	const dgFloat64 normalDeviation = dgFloat64 (0.9999f);
	const dgFloat64 distanceFromPlane = dgFloat64 (1.0f / 128.0f);

	dgInt32 faceIndex[DG_LOCAL_BUFFER_SIZE * 4];
	dgInt64 userIndex[DG_LOCAL_BUFFER_SIZE * 4];
	dgEdge* stack[DG_LOCAL_BUFFER_SIZE * 4];
	dgEdge* deleteEdge[DG_LOCAL_BUFFER_SIZE * 4];

	dgInt32 deleteCount = 1;
	deleteEdge[0] = face;
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));

	dgAssert (face->m_incidentFace > 0);

	dgBigVector normalAverage (FaceNormal (face, pool, strideInBytes));
	dgAssert (normalAverage.m_w == dgFloat32 (0.0f));
	dgFloat64 dot = normalAverage.DotProduct(normalAverage).GetScalar();
	if (dot > dgFloat64 (1.0e-12f)) {
		dgInt32 testPointsCount = 1;
		dot = dgFloat64 (1.0f) / sqrt (dot);
		dgBigVector normal (normalAverage.Scale (dot));

		dgBigVector averageTestPoint (&pool[face->m_incidentVertex * stride]);
		dgBigPlane testPlane(normal, - normal.DotProduct(averageTestPoint & dgBigVector::m_triplexMask).GetScalar());

		polyhedraOut.BeginFace();

		IncLRU();
		dgInt32 faceMark = IncLRU();

		dgInt32 faceIndexCount = 0;
		dgEdge* ptr = face;
		do {
			ptr->m_mark = faceMark;
			faceIndex[faceIndexCount] = ptr->m_incidentVertex;
			userIndex[faceIndexCount] = dgInt64 (ptr->m_userData);
			faceIndexCount ++;
			dgAssert (faceIndexCount < dgInt32 (sizeof (faceIndex) / sizeof (faceIndex[0])));
			ptr = ptr ->m_next;
		} while (ptr != face);
		polyhedraOut.AddFace(faceIndexCount, faceIndex, userIndex);

		dgInt32 index = 1;
		deleteCount = 0;
		stack[0] = face;
		while (index) {
			index --;
			dgEdge* const face1 = stack[index];
			deleteEdge[deleteCount] = face1;
			deleteCount ++;
			dgAssert (deleteCount < dgInt32 (sizeof (deleteEdge) / sizeof (deleteEdge[0])));
			dgAssert (face1->m_next->m_next->m_next == face1);

			dgEdge* edge = face1;
			do {
				dgEdge* const ptr1 = edge->m_twin;
				if (ptr1->m_incidentFace > 0) {
					if (ptr1->m_mark != faceMark) {
						dgEdge* ptr2 = ptr1;
						faceIndexCount = 0;
						do {
							ptr2->m_mark = faceMark;
							faceIndex[faceIndexCount] = ptr2->m_incidentVertex;
							userIndex[faceIndexCount] = dgInt64 (ptr2->m_userData);
							dgAssert (faceIndexCount < dgInt32 (sizeof (faceIndex) / sizeof (faceIndex[0])));
							faceIndexCount ++;
							ptr2 = ptr2 ->m_next;
						} while (ptr2 != ptr1);

						dgBigVector normal1 (FaceNormal (ptr1, pool, strideInBytes));
						dot = normal1.DotProduct(normal1).GetScalar();
						if (dot < dgFloat64 (1.0e-12f)) {
							deleteEdge[deleteCount] = ptr1;
							deleteCount ++;
							dgAssert (deleteCount < dgInt32 (sizeof (deleteEdge) / sizeof (deleteEdge[0])));
						} else {
							dgBigVector testNormal (normal1.Scale (dgFloat64 (1.0f) / sqrt (dot)));
							dgAssert (testNormal.m_w == dgFloat32 (0.0f));
							dot = normal.DotProduct(testNormal).GetScalar();
							if (dot >= normalDeviation) {
								dgBigVector testPoint (&pool[ptr1->m_prev->m_incidentVertex * stride]);
								dgFloat64 dist = fabs (testPlane.Evalue (testPoint));
								if (dist < distanceFromPlane) {
									testPointsCount ++;

									averageTestPoint += testPoint;
									testPoint = averageTestPoint.Scale (dgFloat64 (1.0f) / dgFloat64(testPointsCount));

									normalAverage += normal1;
									dgAssert (normalAverage.m_w == dgFloat32 (0.0f));
									testNormal = normalAverage.Scale (dgFloat64 (1.0f) / sqrt (normalAverage.DotProduct(normalAverage).GetScalar()));
									testPlane = dgBigPlane (testNormal, - testPoint.DotProduct (testNormal).GetScalar());

									polyhedraOut.AddFace(faceIndexCount, faceIndex, userIndex);
									stack[index] = ptr1;
									index ++;
									dgAssert (index < dgInt32 (sizeof (stack) / sizeof (stack[0])));
								}
							}
						}
					}
				}

				edge = edge->m_next;
			} while (edge != face1);
		}
		polyhedraOut.EndFace();
	}

	for (dgInt32 index = 0; index < deleteCount; index ++) {
		DeleteFace (deleteEdge[index]);
	}
}

void dgPolyhedra::RefineTriangulation (const dgFloat64* const vertex, dgInt32 stride, const dgBigVector& normal, dgInt32 perimeterCount, dgEdge** const perimeter)
{
	dgList<dgDiagonalEdge> dignonals(GetAllocator());

	for (dgInt32 i = 1; i <= perimeterCount; i ++) {
		dgEdge* const last = perimeter[i - 1];
		for (dgEdge* ptr = perimeter[i]->m_prev; ptr != last; ptr = ptr->m_twin->m_prev) {
			dgList<dgDiagonalEdge>::dgListNode* node = dignonals.GetFirst();
			for (; node; node = node->GetNext()) {
				const dgDiagonalEdge& key = node->GetInfo();
				if (((key.m_i0 == ptr->m_incidentVertex) && (key.m_i1 == ptr->m_twin->m_incidentVertex)) ||
					((key.m_i1 == ptr->m_incidentVertex) && (key.m_i0 == ptr->m_twin->m_incidentVertex))) {
						break;
				}
			}
			if (!node) {
				dgDiagonalEdge key (ptr);
				dignonals.Append(key);
			}
		}
	}

	dgEdge* const face = perimeter[0];
	dgInt32 i0 = face->m_incidentVertex * stride;
	dgInt32 i1 = face->m_next->m_incidentVertex * stride;
	dgBigVector p0 (vertex[i0], vertex[i0 + 1], vertex[i0 + 2], dgFloat32 (0.0f));
	dgBigVector p1 (vertex[i1], vertex[i1 + 1], vertex[i1 + 2], dgFloat32 (0.0f));

	dgBigVector p1p0 (p1 - p0);
	dgFloat64 mag2 = p1p0.DotProduct(p1p0).GetScalar();
	for (dgEdge* ptr = face->m_next->m_next; mag2 < dgFloat32 (1.0e-12f); ptr = ptr->m_next) {
		dgInt32 i2 = ptr->m_incidentVertex * stride;
		dgBigVector p2 (vertex[i2], vertex[i2 + 1], vertex[i2 + 2], dgFloat32 (0.0f));
		p1p0 = p2 - p0;
		mag2 = p1p0.DotProduct(p1p0).GetScalar();
	}

	dgAssert (p1p0.m_w == dgFloat32 (0.0f));
	dgMatrix matrix (dgGetIdentityMatrix());
	matrix.m_posit = p0;
	matrix.m_front = dgVector (p1p0.Scale (dgFloat64 (1.0f) / sqrt (mag2)));
	matrix.m_right = dgVector (normal.Scale (dgFloat64 (1.0f) / sqrt (normal.DotProduct(normal).GetScalar())));
	matrix.m_up = matrix.m_right.CrossProduct(matrix.m_front);
	matrix = matrix.Inverse();
	dgAssert (matrix.m_posit.m_w == dgFloat32 (1.0f));
//	matrix.m_posit.m_w = dgFloat32 (1.0f);

	dgInt32 maxCount = dignonals.GetCount() * dignonals.GetCount();
	while (dignonals.GetCount() && maxCount) {
		maxCount --;
		dgList<dgDiagonalEdge>::dgListNode* const node = dignonals.GetFirst();
		dgDiagonalEdge key (node->GetInfo());
		dignonals.Remove(node);
		dgEdge* const edge = FindEdge(key.m_i0, key.m_i1);
		if (edge) {
			dgInt32 k0 = edge->m_incidentVertex * stride;
			dgInt32 k1 = edge->m_next->m_incidentVertex * stride;
			dgInt32 k2 = edge->m_next->m_next->m_incidentVertex * stride;
			dgInt32 k3 = edge->m_twin->m_prev->m_incidentVertex * stride;

			dgBigVector q0 (vertex[k0], vertex[k0 + 1], vertex[k0 + 2], dgFloat64 (1.0f));
			dgBigVector q1 (vertex[k1], vertex[k1 + 1], vertex[k1 + 2], dgFloat64 (1.0f));
			dgBigVector q2 (vertex[k2], vertex[k2 + 1], vertex[k2 + 2], dgFloat64 (1.0f));
			dgBigVector q3 (vertex[k3], vertex[k3 + 1], vertex[k3 + 2], dgFloat64 (1.0f));

			q0 = matrix.TransformVector(q0);
			q1 = matrix.TransformVector(q1);
			q2 = matrix.TransformVector(q2);
			q3 = matrix.TransformVector(q3);

			dgFloat64 circleTest[3][3];
			circleTest[0][0] = q0[0] - q3[0];
			circleTest[0][1] = q0[1] - q3[1];
			circleTest[0][2] = circleTest[0][0] * circleTest[0][0] + circleTest[0][1] * circleTest[0][1];

			circleTest[1][0] = q1[0] - q3[0];
			circleTest[1][1] = q1[1] - q3[1];
			circleTest[1][2] = circleTest[1][0] * circleTest[1][0] + circleTest[1][1] * circleTest[1][1];

			circleTest[2][0] = q2[0] - q3[0];
			circleTest[2][1] = q2[1] - q3[1];
			circleTest[2][2] = circleTest[2][0] * circleTest[2][0] + circleTest[2][1] * circleTest[2][1];

			dgFloat64 error;
			dgFloat64 det = Determinant3x3 (circleTest, &error);
			if (det < dgFloat32 (0.0f)) {
				dgEdge* frontFace0 = edge->m_prev;
				dgEdge* backFace0 = edge->m_twin->m_prev;

				FlipEdge(edge);

				if (perimeterCount > 4) {
					dgEdge* backFace1 = backFace0->m_next;
					dgEdge* frontFace1 = frontFace0->m_next;
					for (dgInt32 i = 0; i < perimeterCount; i ++) {
						if (frontFace0 == perimeter[i]) {
							frontFace0 = NULL;
						}
						if (frontFace1 == perimeter[i]) {
							frontFace1 = NULL;
						}

						if (backFace0 == perimeter[i]) {
							backFace0 = NULL;
						}
						if (backFace1 == perimeter[i]) {
							backFace1 = NULL;
						}
					}

					if (backFace0 && (backFace0->m_incidentFace > 0) && (backFace0->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key0 (backFace0);
						dignonals.Append(key0);
					}
					if (backFace1 && (backFace1->m_incidentFace > 0) && (backFace1->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key1 (backFace1);
						dignonals.Append(key1);
					}

					if (frontFace0 && (frontFace0->m_incidentFace > 0) && (frontFace0->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key0 (frontFace0);
						dignonals.Append(key0);
					}

					if (frontFace1 && (frontFace1->m_incidentFace > 0) && (frontFace1->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key1 (frontFace1);
						dignonals.Append(key1);
					}
				}
			}
		}
	}
}

void dgPolyhedra::RefineTriangulation (const dgFloat64* const vertex, dgInt32 stride)
{
	if (GetCount() <= 6) {
		return;
	}

	dgInt32 mark = IncLRU();
	dgInt32 loopCount = 0;
	
	dgPolyhedra::Iterator iter (*this);
	dgEdge* edgePerimeters[DG_LOCAL_BUFFER_SIZE * 16];
	dgInt32 perimeterCount = 0;
	dgTree<dgEdge*, dgInt32> filter (GetAllocator());
	for (iter.Begin(); iter && (loopCount <= 1) ; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace < 0) && (edge->m_mark != mark)){
			loopCount ++;
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				if (!filter.Insert(ptr, ptr->m_incidentVertex)) {
					loopCount = 2;
					break;
				}
				edgePerimeters[perimeterCount] = ptr->m_twin;
				perimeterCount ++;
				dgAssert (perimeterCount < dgInt32 (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
				ptr = ptr->m_prev;
			} while (ptr != edge);
		}
	}

	if (loopCount == 1) {
		#ifdef _DEBUG
		for (dgInt32 i = 0; i < perimeterCount; i ++) {
			for (dgInt32 j = i + 1; j < perimeterCount; j ++) {
				dgAssert (edgePerimeters[i]->m_incidentVertex != edgePerimeters[j]->m_incidentVertex);
			}
		}
		#endif

		dgAssert (perimeterCount);
		dgAssert (perimeterCount < dgInt32 (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
		edgePerimeters[perimeterCount] = edgePerimeters[0];

		dgBigVector normal (FaceNormal(edgePerimeters[0], vertex, dgInt32 (stride * sizeof (dgFloat64))));
		if (normal.DotProduct(normal).GetScalar() > dgFloat32 (1.0e-12f)) {
			RefineTriangulation (vertex, stride, normal, perimeterCount, edgePerimeters);
		}
	}
}


void dgPolyhedra::OptimizeTriangulation (const dgFloat64* const vertex, dgInt32 strideInBytes)
{
	dgInt32 polygon[DG_LOCAL_BUFFER_SIZE * 8];
	dgInt64 userData[DG_LOCAL_BUFFER_SIZE * 8];
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));

	dgPolyhedra leftOver(GetAllocator());
	dgPolyhedra buildConvex(GetAllocator());

	buildConvex.BeginFace();
	dgPolyhedra::Iterator iter (*this);

	for (iter.Begin(); iter; ) {
		dgEdge* const edge = &(*iter);
		iter++;

		if (edge->m_incidentFace > 0) {
			dgPolyhedra flatFace(GetAllocator());
			MarkAdjacentCoplanarFaces (flatFace, edge, vertex, strideInBytes);
			//dgAssert (flatFace.GetCount());

			if (flatFace.GetCount()) {
				flatFace.RefineTriangulation (vertex, stride);

				dgInt32 mark = flatFace.IncLRU();
				dgPolyhedra::Iterator iter1 (flatFace);
				for (iter1.Begin(); iter1; iter1 ++) {
					dgEdge* const edge1 = &(*iter1);
					if (edge1->m_mark != mark) {
						if (edge1->m_incidentFace > 0) {
							dgEdge* ptr = edge1;
							dgInt32 vertexCount = 0;
							do {
								polygon[vertexCount] = ptr->m_incidentVertex;				
								userData[vertexCount] = dgInt64 (ptr->m_userData);
								vertexCount ++;
								dgAssert (vertexCount < dgInt32 (sizeof (polygon) / sizeof (polygon[0])));
								ptr->m_mark = mark;
								ptr = ptr->m_next;
							} while (ptr != edge1);
							if (vertexCount >= 3) {
								buildConvex.AddFace (vertexCount, polygon, userData);
							}
						}
					}
				}
			}
			iter.Begin();
		}
	}
	buildConvex.EndFace();
	dgAssert (GetCount() == 0);
	SwapInfo(buildConvex);
}


void dgPolyhedra::Triangulate (const dgFloat64* const vertex, dgInt32 strideInBytes, dgPolyhedra* const leftOver)
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));

	dgInt32 count = GetCount() / 2;
	dgStack<char> memPool (dgInt32 ((count + 512) * (2 * sizeof (dgFloat64)))); 
	dgDownHeap<dgEdge*, dgFloat64> heap(&memPool[0], memPool.GetSizeInBytes());

	dgInt32 mark = IncLRU();
	Iterator iter (*this);
	for (iter.Begin(); iter; ) {
		dgEdge* const thisEdge = &(*iter);
		iter ++;

		if (thisEdge->m_mark == mark) {
			continue;
		}
		if (thisEdge->m_incidentFace < 0) {
			continue;
		}

		count = 0;
		dgEdge* ptr = thisEdge;
		do {
			count ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != thisEdge);

		if (count > 3) {
			dgEdge* const edge = TriangulateFace (thisEdge, vertex, stride, heap, NULL);
			heap.Flush ();

			if (edge) {
				dgAssert (edge->m_incidentFace > 0);

				if (leftOver) {
					dgInt32* const index = (dgInt32 *) &heap[0];
					dgInt64* const data = (dgInt64 *)&index[count];
					dgInt32 i = 0;
					dgEdge* ptr1 = edge;
					do {
						index[i] = ptr1->m_incidentVertex;
						data[i] = dgInt64 (ptr1->m_userData);
						i ++;
						ptr1 = ptr1->m_next;
					} while (ptr1 != edge);
					leftOver->AddFace(i, index, data);

				} else {
					dgTrace (("Deleting face:"));					
					ptr = edge;
					do {
						dgTrace (("%d ", ptr->m_incidentVertex));
					} while (ptr != edge);
					dgTrace (("\n"));					
				}

				DeleteFace (edge);
				iter.Begin();
			}
		}
	}

	OptimizeTriangulation (vertex, strideInBytes);

	mark = IncLRU();
	m_faceSecuence = 1;
	for (iter.Begin(); iter; iter ++) {
		dgEdge* edge = &(*iter);
		if (edge->m_mark == mark) {
			continue;
		}
		if (edge->m_incidentFace < 0) {
			continue;
		}
		dgAssert (edge == edge->m_next->m_next->m_next);

		for (dgInt32 i = 0; i < 3; i ++) { 
			edge->m_incidentFace = m_faceSecuence; 
			edge->m_mark = mark;
			edge = edge->m_next;
		}
		m_faceSecuence ++;
	}
}


void dgPolyhedra::RemoveOuterColinearEdges (dgPolyhedra& flatFace, const dgFloat64* const vertex, dgInt32 stride)
{
	dgEdge* edgePerimeters[DG_LOCAL_BUFFER_SIZE];

	dgInt32 perimeterCount = 0;
	dgInt32 mark = flatFace.IncLRU();
	dgPolyhedra::Iterator iter (flatFace);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace < 0) && (edge->m_mark != mark)) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
			edgePerimeters[perimeterCount] = edge;
			perimeterCount ++;
			dgAssert (perimeterCount < dgInt32 (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
		}
	}

	for (dgInt32 i = 0; i < perimeterCount; i ++) {
		dgEdge* edge = edgePerimeters[i];
		dgEdge* ptr = edge;
		dgBigVector p0 (&vertex[ptr->m_incidentVertex * stride]);
		dgBigVector p1 (&vertex[ptr->m_next->m_incidentVertex * stride]);
		dgBigVector e0 (p1 - p0) ;
		e0 = e0.Scale (dgRsqrt (e0.DotProduct3(e0) + dgFloat32 (1.0e-12f)));
		dgInt32 ignoreTest = 1;
		do {
			ignoreTest = 0;
			dgBigVector p2 (&vertex[ptr->m_next->m_next->m_incidentVertex * stride]);
			dgBigVector e1 (p2 - p1);
			e1 = e1.Scale (dgRsqrt (e1.DotProduct3(e1) + dgFloat32 (1.0e-12f)));
			dgFloat64 dot = e1.DotProduct3(e0);
			if (dot > dgFloat32 (dgFloat32 (0.9999f))) {

				for (dgEdge* interiorEdge = ptr->m_next->m_twin->m_next; interiorEdge != ptr->m_twin; interiorEdge = ptr->m_next->m_twin->m_next) {
					flatFace.DeleteEdge (interiorEdge);
				} 

				if (ptr->m_twin->m_next->m_next->m_next == ptr->m_twin) {
					dgAssert (ptr->m_twin->m_next->m_incidentFace > 0);
					flatFace.DeleteEdge (ptr->m_twin->m_next);
				}

				dgAssert (ptr->m_next->m_twin->m_next->m_twin == ptr);
				edge = ptr->m_next;

				if (!flatFace.FindEdge (ptr->m_incidentVertex, edge->m_twin->m_incidentVertex) && 
					!flatFace.FindEdge (edge->m_twin->m_incidentVertex, ptr->m_incidentVertex)) {
						ptr->m_twin->m_prev = edge->m_twin->m_prev;
						edge->m_twin->m_prev->m_next = ptr->m_twin;

						edge->m_next->m_prev = ptr;
						ptr->m_next = edge->m_next;

						edge->m_next = edge->m_twin;
						edge->m_prev = edge->m_twin;
						edge->m_twin->m_next = edge;
						edge->m_twin->m_prev = edge;
						flatFace.DeleteEdge (edge);								
						flatFace.ChangeEdgeIncidentVertex (ptr->m_twin, ptr->m_next->m_incidentVertex);

						e1 = e0;
						p1 = p2;
						edge = ptr;
						ignoreTest = 1;
						continue;
				}
			}

			e0 = e1;
			p1 = p2;
			ptr = ptr->m_next;
		} while ((ptr != edge) || ignoreTest);
	}
}


void dgPolyhedra::RemoveInteriorColinearEdges(dgPolyhedra& flatFace, const dgFloat64* const vertex, dgInt32 stride)
{
	bool foundEdge = true;
	while (foundEdge) {
		foundEdge = false;
		dgPolyhedra::Iterator iter(flatFace);
		for (iter.Begin(); iter; iter++) {
			dgEdge* const edge = &(*iter);
			if ((edge->m_incidentFace > 0) && (edge->m_twin->m_incidentFace > 0)) {
				if (edge->m_twin->m_next->m_twin->m_next == edge) {
					dgBigVector p0(&vertex[edge->m_prev->m_incidentVertex * stride]);
					dgBigVector p1(&vertex[edge->m_incidentVertex * stride]);
					dgBigVector p2(&vertex[edge->m_next->m_incidentVertex * stride]);
					
					dgBigVector e0(p1 - p0);
					dgBigVector e1(p2 - p1);
					e0 = e0.Scale(dgRsqrt(e0.DotProduct3(e0) + dgFloat32(1.0e-12f)));
					e1 = e1.Scale(dgRsqrt(e1.DotProduct3(e1) + dgFloat32(1.0e-12f)));
					dgFloat64 dot = e1.DotProduct3(e0);
					if (dot > dgFloat32(0.9999f)) {
						dgInt32 v = edge->m_twin->m_incidentVertex;
						dgEdge* const nextEdge = edge->m_twin->m_next;
						edge->m_next->m_prev = edge->m_prev;
						edge->m_prev->m_next = edge->m_next;
						edge->m_twin->m_next->m_prev = edge->m_twin->m_prev;
						edge->m_twin->m_prev->m_next = edge->m_twin->m_next;

						edge->m_next = edge->m_twin;
						edge->m_prev = edge->m_twin;
						edge->m_twin->m_next = edge;
						edge->m_twin->m_prev = edge;
						flatFace.DeleteEdge(edge);
						flatFace.ChangeEdgeIncidentVertex(nextEdge, v);
						foundEdge = true;
						break;
					}
				}
			}
		}
	}
}


dgInt32 dgPolyhedra::GetInteriorDiagonals (dgPolyhedra& polyhedra, dgEdge** const diagonals, dgInt32 maxCount)
{
	dgInt32 count = 0;
	dgInt32 mark = polyhedra.IncLRU();
	dgPolyhedra::Iterator iter (polyhedra);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) { 
			if (edge->m_incidentFace > 0) {
				if (edge->m_twin->m_incidentFace > 0) {
					edge->m_twin->m_mark = mark;
					if (count < maxCount){
						diagonals[count] = edge;
						count ++;
					}
					dgAssert (count <= maxCount);
				}
			}
		}
		edge->m_mark = mark;
	}

	return count;
}

bool dgPolyhedra::IsEssensialPointDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const dgFloat64* const pool, dgInt32 stride)
{
	if (diagonal->m_twin->m_next->m_twin->m_next != diagonal) {
		dgBigVector p0 (&pool[diagonal->m_incidentVertex * stride]);
		dgBigVector p1 (&pool[diagonal->m_twin->m_next->m_twin->m_incidentVertex * stride]);
		dgBigVector p2 (&pool[diagonal->m_prev->m_incidentVertex * stride]);

		dgBigVector e1 (p1 - p0);
		dgFloat64 dot = e1.DotProduct3(e1);
		if (dot < dgFloat64 (1.0e-12f)) {
			return false;
		}
		e1 = e1.Scale (dgFloat64 (1.0f) / sqrt(dot));

		dgBigVector e2 (p2 - p0);
		dot = e2.DotProduct3(e2);
		if (dot < dgFloat64 (1.0e-12f)) {
			return false;
		}
		e2 = e2.Scale (dgFloat64 (1.0f) / sqrt(dot));

		dgBigVector n1 (e1.CrossProduct(e2)); 

		dot = normal.DotProduct3(n1);
		if (dot >= dgFloat64 (0.0f)) {
			return false;
		}
	}
	return true;
}


bool dgPolyhedra::IsEssensialDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const dgFloat64* const pool,  dgInt32 stride)
{
	return IsEssensialPointDiagonal (diagonal, normal, pool, stride) || IsEssensialPointDiagonal (diagonal->m_twin, normal, pool, stride); 
}

dgObb dgPolyhedra::CalculateSphere (const dgFloat64* const vertex, dgInt32 strideInBytes, const dgMatrix* const basis) const
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));	

	dgInt32 vertexCount = 0;
	dgInt32 mark = IncLRU();
	dgPolyhedra::Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			vertexCount ++;
		}
	}
	dgAssert (vertexCount);

	mark = IncLRU();
	dgInt32 vertexCountIndex = 0;
	dgStack<dgBigVector> pool (vertexCount);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			dgInt32 incidentVertex = edge->m_incidentVertex * stride;
			pool[vertexCountIndex] = dgBigVector (vertex[incidentVertex + 0], vertex[incidentVertex + 1], vertex[incidentVertex + 2], dgFloat32 (0.0f));
			vertexCountIndex ++;
		}
	}
	dgAssert (vertexCountIndex <= vertexCount);

	dgMatrix axis (dgGetIdentityMatrix());
	dgObb sphere (axis);
	dgConvexHull3d convexHull (GetAllocator(), &pool[0].m_x, sizeof (dgBigVector), vertexCountIndex, 0.0f);
	if (convexHull.GetCount()) {
		dgStack<dgInt32> triangleList (convexHull.GetCount() * 3); 				
		dgInt32 trianglesCount = 0;
		for (dgConvexHull3d::dgListNode* node = convexHull.GetFirst(); node; node = node->GetNext()) {
			dgConvexHull3DFace* const face = &node->GetInfo();
			triangleList[trianglesCount * 3 + 0] = face->m_index[0];
			triangleList[trianglesCount * 3 + 1] = face->m_index[1];
			triangleList[trianglesCount * 3 + 2] = face->m_index[2];
			trianglesCount ++;
			dgAssert ((trianglesCount * 3) <= triangleList.GetElementsCount());
		}

		dgVector* const dst = (dgVector*) &pool[0].m_x;
		for (dgInt32 i = 0; i < convexHull.GetVertexCount(); i ++) {
			dst[i] = convexHull.GetVertex(i);
		}
		sphere.SetDimensions (&dst[0].m_x, sizeof (dgVector), &triangleList[0], trianglesCount * 3, NULL);

	} else if (vertexCountIndex >= 3) {
		dgStack<dgInt32> triangleList (GetCount() * 3 * 2); 
		dgInt32 mark1 = IncLRU();
		dgInt32 trianglesCount = 0;
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			if (edge->m_mark != mark1) {
				dgEdge* ptr = edge;
				do {
					ptr->m_mark = mark1;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				ptr = edge->m_next->m_next;
				do {
					triangleList[trianglesCount * 3 + 0] = edge->m_incidentVertex;
					triangleList[trianglesCount * 3 + 1] = ptr->m_prev->m_incidentVertex;
					triangleList[trianglesCount * 3 + 2] = ptr->m_incidentVertex;
					trianglesCount ++;
					dgAssert ((trianglesCount * 3) <= triangleList.GetElementsCount());
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				dgVector* const dst = (dgVector*) &pool[0].m_x;
				for (dgInt32 i = 0; i < vertexCountIndex; i ++) {
					dst[i] = pool[i];
				}
				sphere.SetDimensions (&dst[0].m_x, sizeof (dgVector), &triangleList[0], trianglesCount * 3, NULL);
			}
		}
	}
	return sphere;
}

dgBigPlane dgPolyhedra::EdgePlane (dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const pool) const
{
	const dgBigVector& p0 = pool[i0];
	const dgBigVector& p1 = pool[i1];
	const dgBigVector& p2 = pool[i2];

	dgBigPlane plane (p0, p1, p2);
	dgFloat64 mag = sqrt (plane.DotProduct(plane & dgBigPlane::m_triplexMask).GetScalar());
	if (mag < dgFloat64 (1.0e-12f)) {
		mag = dgFloat64 (1.0e-12f);
	}
	mag = dgFloat64 (1.0f) / mag;

	plane.m_x *= mag;
	plane.m_y *= mag;
	plane.m_z *= mag;
	plane.m_w *= mag;

	return plane;
}


void dgPolyhedra::CalculateVertexMetrics (dgVertexCollapseVertexMetric* const table, const dgBigVector* const pool, dgEdge* const edge) const
{
	dgInt32 i0 = edge->m_incidentVertex;

	table[i0].Clear ();
	dgEdge* ptr = edge;
	do {

		if (ptr->m_incidentFace > 0) {
			dgInt32 i1 = ptr->m_next->m_incidentVertex;
			dgInt32 i2 = ptr->m_prev->m_incidentVertex;
			dgBigPlane constrainPlane (EdgePlane (i0, i1, i2, pool));
			table[i0].Accumulate (constrainPlane);

		} else {
			dgInt32 i1 = ptr->m_twin->m_incidentVertex;
			dgInt32 i2 = ptr->m_twin->m_prev->m_incidentVertex;
			dgBigPlane constrainPlane (UnboundedLoopPlane (i0, i1, i2, pool));
			table[i0].Accumulate (constrainPlane);

			i1 = ptr->m_prev->m_incidentVertex;
			i2 = ptr->m_prev->m_twin->m_prev->m_incidentVertex;
			constrainPlane = UnboundedLoopPlane (i0, i1, i2, pool);
			table[i0].Accumulate (constrainPlane);
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);
}


void dgPolyhedra::CalculateAllMetrics (dgVertexCollapseVertexMetric* const table, const dgBigVector* const pool) const
{
	dgInt32 edgeMark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		dgAssert (edge);
		if (edge->m_mark != edgeMark) {

			if (edge->m_incidentFace > 0) {
				dgInt32 i0 = edge->m_incidentVertex;
				dgInt32 i1 = edge->m_next->m_incidentVertex;
				dgInt32 i2 = edge->m_prev->m_incidentVertex;

				dgBigPlane constrainPlane (EdgePlane (i0, i1, i2, pool));
				dgVertexCollapseVertexMetric tmp (constrainPlane);

				dgEdge* ptr = edge;
				do {
					ptr->m_mark = edgeMark;
					i0 = ptr->m_incidentVertex;
					table[i0].Accumulate(tmp);

					ptr = ptr->m_next;
				} while (ptr != edge);

			} else {
				dgAssert (edge->m_twin->m_incidentFace > 0);
				dgInt32 i0 = edge->m_twin->m_incidentVertex;
				dgInt32 i1 = edge->m_twin->m_next->m_incidentVertex;
				dgInt32 i2 = edge->m_twin->m_prev->m_incidentVertex;

				edge->m_mark = edgeMark;
				dgBigPlane constrainPlane (UnboundedLoopPlane (i0, i1, i2, pool));
				dgVertexCollapseVertexMetric tmp (constrainPlane);

				i0 = edge->m_incidentVertex;
				table[i0].Accumulate(tmp);

				i0 = edge->m_twin->m_incidentVertex;
				table[i0].Accumulate(tmp);
			}
		}
	}
}

bool dgPolyhedra::IsOkToCollapse (const dgBigVector* const pool, dgEdge* const edge) const
{
	const dgBigVector& q = pool[edge->m_incidentVertex];
	const dgBigVector& p = pool[edge->m_twin->m_incidentVertex];
	for (dgEdge* triangle = edge->m_prev->m_twin; triangle != edge->m_twin->m_next; triangle = triangle->m_prev->m_twin) {
		if (triangle->m_incidentFace > 0) {
			dgAssert ((edge->m_incidentFace < 0) || (edge->m_incidentVertex == edge->m_next->m_next->m_next->m_incidentVertex));

			dgBigVector originalArea ((pool[triangle->m_next->m_incidentVertex] - q).CrossProduct(pool[triangle->m_prev->m_incidentVertex] - q));
			dgBigVector newArea ((pool[triangle->m_next->m_incidentVertex] - p).CrossProduct(pool[triangle->m_prev->m_incidentVertex] - p));

			dgFloat64 projectedArea = newArea.DotProduct3(originalArea);
			if (projectedArea <= dgFloat64 (0.0f)) {
				return false;
			}

			dgFloat64 mag20 = newArea.DotProduct3(newArea);
			dgFloat64 mag21 = originalArea.DotProduct3(originalArea);
			if ((projectedArea * projectedArea)  < (mag20 * mag21 * dgFloat64 (1.0e-10f)))  {
				return false;
			}
		}
	}

	return true;
}


dgEdge* dgPolyhedra::OptimizeCollapseEdge (dgEdge* const edge)
{
	dgInt32 v0 = edge->m_incidentVertex;
	dgInt32 v1 = edge->m_twin->m_incidentVertex;

#ifdef _DEBUG
	dgPolyhedra::dgPairKey TwinKey (v1, v0);
	dgPolyhedra::dgTreeNode* const node = Find (TwinKey.GetVal());
	dgEdge* const twin1 = node ? &node->GetInfo() : NULL;
	dgAssert (twin1);
	dgAssert (edge->m_twin == twin1);
	dgAssert (twin1->m_twin == edge);
	dgAssert (edge->m_incidentFace != 0);
	dgAssert (twin1->m_incidentFace != 0);
	dgAssert ((edge->m_incidentFace < 0) || (edge->m_incidentVertex == edge->m_next->m_next->m_next->m_incidentVertex));
	dgAssert ((edge->m_twin->m_incidentFace < 0) || (edge->m_twin->m_incidentVertex == edge->m_twin->m_next->m_next->m_next->m_incidentVertex));
#endif

	dgEdge* retEdge = edge->m_twin->m_prev->m_twin;
	if (retEdge	== edge->m_twin->m_next) {
		return NULL;
	}
	if (retEdge	== edge->m_twin) {
		return NULL;
	}
	if (retEdge	== edge->m_next) {
		retEdge = edge->m_prev->m_twin;
		if (retEdge	== edge->m_twin->m_next) {
			return NULL;
		}
		if (retEdge	== edge->m_twin) {
			return NULL;
		}
	}

	dgEdge* lastEdge = NULL;
	dgEdge* firstEdge = NULL;
	if ((edge->m_incidentFace >= 0)	&& (edge->m_twin->m_incidentFace >= 0)) {	
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
	} else if (edge->m_twin->m_incidentFace >= 0) {
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
		lastEdge = edge;
	} else {
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next;
	}

	for (dgEdge* ptr = firstEdge; ptr != lastEdge; ptr = ptr->m_twin->m_next) {
		dgEdge* badEdge = FindEdge (edge->m_twin->m_incidentVertex, ptr->m_twin->m_incidentVertex);
		if (badEdge) {
			return NULL;
		}
	} 

	dgEdge* const twin = edge->m_twin;
	if (twin->m_next == twin->m_prev->m_prev) {
		twin->m_prev->m_twin->m_twin = twin->m_next->m_twin;
		twin->m_next->m_twin->m_twin = twin->m_prev->m_twin;

		RemoveHalfEdge (twin->m_prev);
		RemoveHalfEdge (twin->m_next);
	} else {
		twin->m_next->m_prev = twin->m_prev;
		twin->m_prev->m_next = twin->m_next;
	}

	if (edge->m_next == edge->m_prev->m_prev) {
		edge->m_next->m_twin->m_twin = edge->m_prev->m_twin;
		edge->m_prev->m_twin->m_twin = edge->m_next->m_twin;
		RemoveHalfEdge (edge->m_next);
		RemoveHalfEdge (edge->m_prev);
	} else {
		edge->m_next->m_prev = edge->m_prev;
		edge->m_prev->m_next = edge->m_next;
	}

	dgAssert (twin->m_twin->m_incidentVertex == v0);
	dgAssert (edge->m_twin->m_incidentVertex == v1);
	RemoveHalfEdge (twin);
	RemoveHalfEdge (edge);

	dgEdge* remapPtr = retEdge;
	do {
		dgPolyhedra::dgPairKey pairKey (v0, remapPtr->m_twin->m_incidentVertex);
		dgPolyhedra::dgTreeNode* const pairEdgeNode = Find (pairKey.GetVal());
		if (pairEdgeNode) {
			if (&pairEdgeNode->GetInfo() == remapPtr) {
				dgPolyhedra::dgPairKey key (v1, remapPtr->m_twin->m_incidentVertex);
				remapPtr->m_incidentVertex = v1;
				ReplaceKey (pairEdgeNode, key.GetVal());
			} 
		}

		dgPolyhedra::dgPairKey twinKey1 (remapPtr->m_twin->m_incidentVertex, v0);
		dgPolyhedra::dgTreeNode* const pairTwinNode = Find (twinKey1.GetVal());
		if (pairTwinNode) {
			if (&pairTwinNode->GetInfo() == remapPtr->m_twin) {
				dgPolyhedra::dgPairKey key (remapPtr->m_twin->m_incidentVertex, v1);
				ReplaceKey (pairTwinNode, key.GetVal());
			}
		}

		remapPtr = remapPtr->m_twin->m_next;
	} while (remapPtr != retEdge);

	return retEdge;
}


dgFloat64 dgPolyhedra::EdgePenalty (const dgBigVector* const pool, dgEdge* const edge, dgFloat64 dist) const
{
	dgInt32 i0 = edge->m_incidentVertex;
	dgInt32 i1 = edge->m_next->m_incidentVertex;

	dgFloat32 maxPenalty = dgFloat32 (1.0e14f);

	const dgBigVector& p0 = pool[i0];
	const dgBigVector& p1 = pool[i1];
	dgBigVector dp (p1 - p0);

	dgFloat64 dot = dp.DotProduct3(dp);
	if (dot < dgFloat64(1.0e-6f)) {
		return dist * maxPenalty;
	}

	if ((edge->m_incidentFace > 0) && (edge->m_twin->m_incidentFace > 0)) {
		dgBigVector edgeNormal (FaceNormal (edge, &pool[0].m_x, sizeof (dgBigVector)));
		dgBigVector twinNormal (FaceNormal (edge->m_twin, &pool[0].m_x, sizeof (dgBigVector)));

		dgFloat64 mag0 = edgeNormal.DotProduct3(edgeNormal);
		dgFloat64 mag1 = twinNormal.DotProduct3(twinNormal);
		if ((mag0 < dgFloat64 (1.0e-24f)) || (mag1 < dgFloat64 (1.0e-24f))) {
			return dist * maxPenalty;
		}

		edgeNormal = edgeNormal.Scale (dgFloat64 (1.0f) / sqrt(mag0));
		twinNormal = twinNormal.Scale (dgFloat64 (1.0f) / sqrt(mag1));

		dot = edgeNormal.DotProduct3(twinNormal);
		if (dot < dgFloat64 (-0.9f)) {
			return dist * maxPenalty;
		}

		dgEdge* ptr = edge;
		do {
			if ((ptr->m_incidentFace <= 0) || (ptr->m_twin->m_incidentFace <= 0)){
				dgEdge* const adj = edge->m_twin;
				ptr = edge;
				do {
					if ((ptr->m_incidentFace <= 0) || (ptr->m_twin->m_incidentFace <= 0)){
						return dist * maxPenalty;
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != adj);
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != edge);
	}

	dgInt32 faceA = edge->m_incidentFace;
	dgInt32 faceB = edge->m_twin->m_incidentFace;

	i0 = edge->m_twin->m_incidentVertex;
	dgBigVector p (pool[i0].m_x, pool[i0].m_y, pool[i0].m_z, dgFloat32 (0.0f));

	bool penalty = false;
	dgEdge* ptr = edge;
	do {
		dgEdge* const adj = ptr->m_twin;

		dgInt32 face = adj->m_incidentFace;
		if ((face != faceB) && (face != faceA) && (face >= 0) && (adj->m_next->m_incidentFace == face) && (adj->m_prev->m_incidentFace == face)){

			dgInt32 k0 = adj->m_next->m_incidentVertex;
			const dgBigVector& q0 = pool[k0];

			dgInt32 k1 = adj->m_incidentVertex;
			const dgBigVector& q1 = pool[k1];

			dgInt32 k2 = adj->m_prev->m_incidentVertex;
			const dgBigVector& q2 = pool[k2];

			dgBigVector n0 ((q1 - q0).CrossProduct(q2 - q0));
			dgBigVector n1 ((q1 - p).CrossProduct(q2 - p));
			dgFloat64 project = n0.DotProduct3(n1);
			if (project < dgFloat64 (0.0f)) {
				penalty = true;
				break;
			}
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);

	dgFloat64 aspect = dgFloat32 (0.0f);
	if (!penalty) {
		dgInt32 k0 = edge->m_twin->m_incidentVertex;
		dgBigVector q0 (pool[k0]);

		aspect = dgFloat32 (1.0f);
		for (dgEdge* ptr1 = edge->m_twin->m_next->m_twin->m_next; ptr1 != edge; ptr1 = ptr1->m_twin->m_next) {
			if (ptr1->m_incidentFace > 0) {
				dgInt32 k1 = ptr1->m_next->m_incidentVertex;
				const dgBigVector& q1 = pool[k1];

				dgInt32 k2 = ptr1->m_prev->m_incidentVertex;
				const dgBigVector& q2 = pool[k2];

				dgBigVector e0 (q1 - q0);
				dgBigVector e1 (q2 - q1);
				dgBigVector e2 (q0 - q2);

				dgFloat64 mag0 = e0.DotProduct3(e0);
				dgFloat64 mag1 = e1.DotProduct3(e1);
				dgFloat64 mag2 = e2.DotProduct3(e2);
				dgFloat64 maxMag = dgMax (mag0, mag1, mag2);
				dgFloat64 minMag = dgMin (mag0, mag1, mag2);
				dgFloat64 ratio = minMag / maxMag;

				if (ratio < aspect) {
					aspect = ratio;
				}
			}
		}
		aspect = dgFloat32 (1.0f) - aspect;
	}
	return aspect * aspect * dist;
}


bool dgPolyhedra::Optimize (const dgFloat64* const array, dgInt32 strideInBytes, dgReportProgress normalizedProgress, void* const reportProgressUserData, dgFloat64 tol, dgInt32 maxFaceCount)
{
	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif

	dgFloat32 progressDen = dgFloat32 (1.0f / GetEdgeCount());
	dgInt32 edgeCount = GetEdgeCount() * 4 + DG_LOCAL_BUFFER_SIZE * 16;
	dgInt32 maxVertexIndex = GetLastVertexIndex();
	
	dgStack<dgBigVector> vertexPool (maxVertexIndex); 
	dgStack<dgVertexCollapseVertexMetric> vertexMetrics (maxVertexIndex + 512); 

	dgList <dgEdgeCollapseEdgeHandle> edgeHandleList(GetAllocator());
	dgStack<char> heapPool (2 * edgeCount * dgInt32 (sizeof (dgFloat64) + sizeof (dgEdgeCollapseEdgeHandle*) + sizeof (dgInt32))); 
	dgUpHeap<dgList <dgEdgeCollapseEdgeHandle>::dgListNode* , dgFloat64> bigHeapArray(&heapPool[0], heapPool.GetSizeInBytes());

	for (dgInt32 i = 0; i < maxVertexIndex; i ++) {
		vertexPool[i].m_x = array[i * stride + 0];
		vertexPool[i].m_y = array[i * stride + 1];
		vertexPool[i].m_z = array[i * stride + 2];
		vertexPool[i].m_w= dgFloat64 (0.0f);
	}

	memset (&vertexMetrics[0], 0, maxVertexIndex * sizeof (dgVertexCollapseVertexMetric));
	CalculateAllMetrics (&vertexMetrics[0], &vertexPool[0]);

	const dgFloat64 maxCost = dgFloat32 (1.0e-3f);
	dgFloat64 tol2 = tol * tol;
	dgFloat64 distTol = dgMax (tol2, dgFloat64 (1.0e-12f));
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		edge->m_userData = 0;
		dgInt32 index0 = edge->m_incidentVertex;
		dgInt32 index1 = edge->m_twin->m_incidentVertex;

		dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
		const dgBigVector& p = vertexPool[index1];
		dgFloat64 faceCost = metric.Evalue (p); 
		dgFloat64 edgePenalty = EdgePenalty (&vertexPool[0], edge, distTol);
		dgAssert (edgePenalty >= dgFloat32 (0.0f));
		dgEdgeCollapseEdgeHandle handle (edge);
		dgList <dgEdgeCollapseEdgeHandle>::dgListNode* handleNodePtr = edgeHandleList.Addtop (handle);
		bigHeapArray.Push (handleNodePtr, faceCost + edgePenalty);
	}

	bool progress = true;
	dgInt32 interPasses = 0;
	dgInt32 faceCount = GetFaceCount();
	while (bigHeapArray.GetCount() && (bigHeapArray.Value() < maxCost) && ((bigHeapArray.Value() < tol2) || (faceCount > maxFaceCount)) && progress ) {
		dgList <dgEdgeCollapseEdgeHandle>::dgListNode* const handleNodePtr = bigHeapArray[0];

		dgEdge* edge = handleNodePtr->GetInfo().m_edge;
		bigHeapArray.Pop();
		edgeHandleList.Remove (handleNodePtr);

		if (edge) {
			if (IsOkToCollapse (&vertexPool[0], edge)) {

				if (normalizedProgress) {
				interPasses ++;
				faceCount -= 2;
				if (interPasses >= 400) {
					interPasses = 0;
					faceCount = GetFaceCount();
						progress = normalizedProgress(dgFloat32 (1.0f) - GetEdgeCount() * progressDen, reportProgressUserData);
					}
				}

				if (bigHeapArray.GetCount() > (bigHeapArray.GetMaxCount() - 100)) {
					for(dgInt32 i = bigHeapArray.GetCount() - 1; i >= 0; i --) {
						dgList <dgEdgeCollapseEdgeHandle>::dgListNode* const emptyHandle = bigHeapArray[i];
						if (!emptyHandle->GetInfo().m_edge) {
							bigHeapArray.Remove(i);
							edgeHandleList.Remove (emptyHandle);
						}
					}
				}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
				dgAssert (SanityCheck ());
#endif

				edge = OptimizeCollapseEdge(edge);

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
				dgAssert (SanityCheck ());
#endif
				if (edge) {
					// Update vertex metrics
					CalculateVertexMetrics (&vertexMetrics[0], &vertexPool[0], edge);

					// Update metrics for all surrounding vertex
					dgEdge* ptr = edge;
					do {
						CalculateVertexMetrics (&vertexMetrics[0], &vertexPool[0], ptr->m_twin);
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);

					// calculate edge cost of all incident edges
					dgInt32 mark = IncLRU();
					ptr = edge;
					do {
						dgAssert (ptr->m_mark != mark);
						ptr->m_mark = mark;

						dgInt32 index0 = ptr->m_incidentVertex;
						dgInt32 index1 = ptr->m_twin->m_incidentVertex;

						dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
						const dgBigVector& p = vertexPool[index1];

						dgFloat64 faceCost = metric.Evalue (p); 
						dgFloat64 edgePenalty = EdgePenalty (&vertexPool[0], ptr, distTol);
						dgAssert (edgePenalty >= dgFloat32 (0.0f));
						dgEdgeCollapseEdgeHandle handle (ptr);
						dgList <dgEdgeCollapseEdgeHandle>::dgListNode* handleNodePtr1 = edgeHandleList.Addtop (handle);
						bigHeapArray.Push (handleNodePtr1, faceCost + edgePenalty);

						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);


					// calculate edge cost of all incident edges to a surrounding vertex
					ptr = edge;
					do {
						dgEdge* const incidentEdge = ptr->m_twin;		

						dgEdge* ptr1 = incidentEdge;
						do {
							dgInt32 index0 = ptr1->m_incidentVertex;
							dgInt32 index1 = ptr1->m_twin->m_incidentVertex;

							if (ptr1->m_mark != mark) {
								ptr1->m_mark = mark;
								dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
								const dgBigVector& p = vertexPool[index1];

								dgFloat64 faceCost = metric.Evalue (p); 
								dgFloat64 edgePenalty = EdgePenalty (&vertexPool[0], ptr1, distTol);
								dgAssert (edgePenalty >= dgFloat32 (0.0f));
								dgEdgeCollapseEdgeHandle handle (ptr1);
								dgList <dgEdgeCollapseEdgeHandle>::dgListNode* handleNodePtr1 = edgeHandleList.Addtop (handle);
								bigHeapArray.Push (handleNodePtr1, faceCost + edgePenalty);
							}

							if (ptr1->m_twin->m_mark != mark) {
								ptr1->m_twin->m_mark = mark;
								dgVertexCollapseVertexMetric &metric = vertexMetrics[index1];
								const dgBigVector& p = vertexPool[index0];
								dgFloat64 faceCost = metric.Evalue (p); 
								dgFloat64 edgePenalty = EdgePenalty (&vertexPool[0], ptr1->m_twin, distTol);
								dgAssert (edgePenalty >= dgFloat32 (0.0f));
								dgEdgeCollapseEdgeHandle handle (ptr1->m_twin);
								dgList <dgEdgeCollapseEdgeHandle>::dgListNode* handleNodePtr1 = edgeHandleList.Addtop (handle);
								bigHeapArray.Push (handleNodePtr1, faceCost + edgePenalty);
							}

							ptr1 = ptr1->m_twin->m_next;
						} while (ptr1 != incidentEdge);

						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);
				}
			}
		}
	}

	if (normalizedProgress && progress) {
		progress = normalizedProgress(dgFloat32 (1.0f), reportProgressUserData);
	}
	return progress;
}

bool dgPolyhedra::TriangulateFace(dgEdge* const face, const dgFloat64* const pool, dgInt32 strideInBytes)
{
	if (face->m_next->m_next->m_next != face) {
		dgInt32 mark = IncLRU();
		dgEdge* ptr = face;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != face);
		char memPool[DG_LOCAL_BUFFER_SIZE * (sizeof (dgEdge*)+sizeof (dgFloat64))];
		dgDownHeap<dgEdge*, dgFloat64> heap(&memPool[0], sizeof (memPool));

		dgInt32 stride = dgInt32(strideInBytes / sizeof (dgFloat64));
		dgEdge* const edge = TriangulateFace(face, pool, stride, heap, NULL);
		dgAssert(!edge);
		return !edge;
	}
	return true;
}


dgEdge* dgPolyhedra::BestEdgePolygonizeFace(const dgBigVector& normal, dgEdge* const edge, const dgFloat64* const pool, dgInt32 stride, const dgBigVector& point) const
{
	dgBigVector p0(&pool[edge->m_incidentVertex * stride]);
	dgBigVector r(point - p0);
	dgEdge* e0 = edge;
	do {
		dgBigVector p1(&pool[e0->m_twin->m_incidentVertex * stride]);
		dgBigVector p2(&pool[e0->m_prev->m_incidentVertex * stride]);
		//dgFloat64 test0 = (normal * (p1 - p0)) % r;
		//dgFloat64 test1 = ((p2 - p0) * normal) % r;
		dgFloat64 test0 = r.DotProduct3(normal.CrossProduct(p1 - p0));
		dgFloat64 test1 = r.DotProduct3((p2 - p0).CrossProduct(normal));
		
		if ((test0 > 0.0f) && (test1 > 0.0f)) {
			break;
		}
		e0 = e0->m_prev->m_twin;
	} while (e0 != edge);
	return e0;
}


bool dgPolyhedra::PolygonizeFace(dgEdge* const face, const dgFloat64* const pool, dgInt32 strideInBytes)
{
	dgPolyhedra flatFace(GetAllocator());
	dgEdge* array[DG_LOCAL_BUFFER_SIZE];

	dgInt32 count = 0;		
	dgEdge* edge = face;
	do {
		dgEdge* const perimeter = flatFace.AddHalfEdge (edge->m_incidentVertex, edge->m_twin->m_incidentVertex);
		dgAssert (perimeter);
		perimeter->m_userData = edge->m_userData;
		perimeter->m_incidentFace = 1;
		perimeter->m_twin = NULL;
		perimeter->m_prev = NULL;
		perimeter->m_next = NULL;

		array[count] = perimeter;
		count++;
		dgAssert(count <= DG_LOCAL_BUFFER_SIZE);
		edge = edge->m_next;
	} while (edge != face);


	dgInt32 i0 = count - 1;
	for(dgInt32 i = 0; i < count; i ++) {
		dgEdge* const edge1 = array[i];
		dgEdge* const prev1 = array[i0];

		edge1->m_prev = prev1;
		prev1->m_next = edge1;
		i0 = i;
	} 

	for(dgInt32 i = 0; i < count; i ++) {
		dgEdge* const edge1 = array[i];
		dgEdge* const twin1 = flatFace.FindEdge (edge1->m_next->m_incidentVertex, edge1->m_incidentVertex);
		if (twin1) {
			twin1->m_twin = edge1;
			edge1->m_twin = twin1;
		} else {
			dgEdge* const perimeter = flatFace.AddHalfEdge (edge1->m_next->m_incidentVertex, edge1->m_incidentVertex);
			perimeter->m_twin = edge1;
			edge1->m_twin = perimeter;
			perimeter->m_incidentFace = -1;
			perimeter->m_prev = NULL;
			perimeter->m_next = NULL;
		}
	}

	for (dgInt32 i = 0; i < count; i++) {
		dgEdge* const edge1 = array[i];
		dgEdge* const twin1 = edge1->m_twin;
		if (!twin1->m_next) {
			dgEdge* next = edge1->m_prev->m_twin;
			while (next->m_prev) {
				next = next->m_prev->m_twin;
			}
			twin1->m_next = next;	
			next->m_prev = next;
		}
	}

	dgBigVector normal (flatFace.FaceNormal(array[0], pool, strideInBytes));
	if (flatFace.TriangulateFace(array[0], pool, strideInBytes)) {
		dgInt32 stride = dgInt32(strideInBytes / sizeof (dgFloat64));
		flatFace.RefineTriangulation(pool, stride);

		//RemoveOuterColinearEdges(*this, vertex, stride);
		dgInt32 polygon[DG_LOCAL_BUFFER_SIZE];
		dgEdge* diagonalsPool[DG_LOCAL_BUFFER_SIZE];

		dgInt32 diagonalCount = GetInteriorDiagonals(flatFace, diagonalsPool, sizeof (diagonalsPool) / sizeof (diagonalsPool[0]));

		if (diagonalCount) {
			dgEdge* edge1 = &flatFace.GetRoot()->GetInfo();
			if (edge1->m_incidentFace < 0) {
				edge1 = edge1->m_twin;
			}

			dgAssert(edge1->m_incidentFace > 0);

			dgBigVector normal1(flatFace.FaceNormal(edge1, pool, strideInBytes));
			normal1 = normal1.Scale(dgFloat64(1.0f) / sqrt(normal1.DotProduct3(normal1)));

			edge1 = NULL;
			dgPolyhedra::Iterator iter0(flatFace);
			for (iter0.Begin(); iter0; iter0++) {
				edge1 = &(*iter0);
				if (edge1->m_incidentFace < 0) {
					break;
				}
			}
			dgAssert(edge1);

			dgInt32 isConvex = 1;
			dgEdge* ptr = edge1;
			dgInt32 mark = flatFace.IncLRU();

			dgBigVector normal2(normal1);
			dgBigVector p0(&pool[ptr->m_prev->m_incidentVertex * stride]);
			dgBigVector p1(&pool[ptr->m_incidentVertex * stride]);
			dgBigVector e0(p1 - p0);
			e0 = e0.Scale(dgFloat64(1.0f) / sqrt(e0.DotProduct3(e0) + dgFloat64(1.0e-24f)));
			do {
				dgBigVector p2(&pool[ptr->m_next->m_incidentVertex * stride]);
				dgBigVector e1(p2 - p1);
				e1 = e1.Scale(dgFloat64(1.0f) / sqrt(e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));
				dgFloat64 dot = normal2.DotProduct3(e0.CrossProduct(e1));
				
				if (dot > dgFloat32(5.0e-3f)) {
					isConvex = 0;
					break;
				}
				ptr->m_mark = mark;
				e0 = e1;
				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edge1);

			if (isConvex) {
				dgPolyhedra::Iterator iter(flatFace);
				for (iter.Begin(); iter; iter++) {
					ptr = &(*iter);
					if (ptr->m_incidentFace < 0) {
						if (ptr->m_mark < mark) {
							isConvex = 0;
							break;
						}
					}
				}
			}

			if (isConvex) {
				if (diagonalCount > 2) {
					dgInt32 count1 = 0;
					ptr = edge1;
					do {
						polygon[count1] = ptr->m_incidentVertex;
						count1++;
						dgAssert(count1 < dgInt32(sizeof (polygon) / sizeof (polygon[0])));
						ptr = ptr->m_next;
					} while (ptr != edge1);

					for (dgInt32 i = 0; i < count1 - 1; i++) {
						for (dgInt32 j = i + 1; j < count1; j++) {
							if (polygon[i] == polygon[j]) {
								i = count1;
								isConvex = 0;
								break;
							}
						}
					}
				}
			}

			if (isConvex) {
				for (dgInt32 j = 0; j < diagonalCount; j++) {
					dgEdge* const diagonal = diagonalsPool[j];
					flatFace.DeleteEdge(diagonal);
				}
			} else {
				for (dgInt32 j = 0; j < diagonalCount; j++) {
					dgEdge* const diagonal = diagonalsPool[j];
					if (!IsEssensialDiagonal(diagonal, normal1, pool, stride)) {
						flatFace.DeleteEdge(diagonal);
					}
				}
			}
		}

		dgInt32 mark = flatFace.IncLRU();
		dgPolyhedra::Iterator iter0(flatFace);
		for (iter0.Begin(); iter0; iter0++) {
			dgEdge* const edge1 = &(*iter0);
			if ((edge1->m_mark != mark) && (edge1->m_incidentFace > 0)) {
				edge1->m_mark = mark;
				edge1->m_twin->m_mark = mark;
				if (!FindEdge(edge1->m_incidentVertex, edge1->m_twin->m_incidentVertex)) {
					dgPairKey key0 (edge1->m_incidentVertex, 0);
					dgPairKey key1 (edge1->m_twin->m_incidentVertex, 0);
					dgTreeNode* const node0 = FindGreater (key0.GetVal());
					dgTreeNode* const node1 = FindGreater (key1.GetVal());
					dgAssert (node0);
					dgAssert (node1);
					dgEdge* e0 = &node0->GetInfo();
					dgEdge* e1 = &node1->GetInfo();

					dgBigVector p0 (&pool[e0->m_incidentVertex * stride]);
					dgBigVector p1 (&pool[e1->m_incidentVertex * stride]);
					e0 = BestEdgePolygonizeFace (normal, e0, pool, stride, p1);
					e1 = BestEdgePolygonizeFace (normal, e1, pool, stride, p0);
					ConnectVertex (e0, e1);
				}
			}
		}
	}

	return true;
}

void dgPolyhedra::RemoveInteriorEdges (dgPolyhedra& buildConvex, const dgFloat64* const vertex, dgInt32 strideInBytes)
{
	dgInt32 polygon[DG_LOCAL_BUFFER_SIZE * 8];
	dgEdge* diagonalsPool[DG_LOCAL_BUFFER_SIZE * 8];

	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat64));

	buildConvex.BeginFace();
	dgPolyhedra::Iterator iter(*this);
	for (iter.Begin(); iter;) {
		dgEdge* edge = &(*iter);
		iter++;
		if (edge->m_incidentFace > 0) {

			dgPolyhedra flatFace(GetAllocator());
			MarkAdjacentCoplanarFaces(flatFace, edge, vertex, strideInBytes);
			if (flatFace.GetCount()) {
				flatFace.RefineTriangulation(vertex, stride);
				RemoveOuterColinearEdges(flatFace, vertex, stride);
				RemoveInteriorColinearEdges(flatFace, vertex, stride);

				dgInt32 diagonalCount = GetInteriorDiagonals(flatFace, diagonalsPool, sizeof(diagonalsPool) / sizeof(diagonalsPool[0]));
				if (diagonalCount) {
					edge = &flatFace.GetRoot()->GetInfo();
					if (edge->m_incidentFace < 0) {
						edge = edge->m_twin;
					}
					dgAssert(edge->m_incidentFace > 0);

					dgBigVector normal(FaceNormal(edge, vertex, strideInBytes));
					normal = normal.Scale(dgFloat64(1.0f) / sqrt(normal.DotProduct3(normal)));

					edge = NULL;
					dgPolyhedra::Iterator iter1(flatFace);
					for (iter1.Begin(); iter1; iter1++) {
						edge = &(*iter1);
						if (edge->m_incidentFace < 0) {
							break;
						}
					}
					dgAssert(edge);

					dgInt32 isConvex = 1;
					dgEdge* ptr = edge;
					dgInt32 mark = flatFace.IncLRU();

					dgBigVector normal2(normal);
					dgBigVector p0(&vertex[ptr->m_prev->m_incidentVertex * stride]);
					dgBigVector p1(&vertex[ptr->m_incidentVertex * stride]);
					dgBigVector e0(p1 - p0);
					e0 = e0.Scale(dgFloat64(1.0f) / sqrt(e0.DotProduct3(e0) + dgFloat64(1.0e-24f)));
					do {
						dgBigVector p2(&vertex[ptr->m_next->m_incidentVertex * stride]);
						dgBigVector e1(p2 - p1);
						e1 = e1.Scale(dgFloat64(1.0f) / sqrt(e1.DotProduct3(e1) + dgFloat32(1.0e-24f)));
						dgFloat64 dot = normal2.DotProduct3(e0.CrossProduct(e1));

						if (dot > dgFloat32(5.0e-3f)) {
							isConvex = 0;
							break;
						}
						ptr->m_mark = mark;
						e0 = e1;
						p1 = p2;
						ptr = ptr->m_next;
					} while (ptr != edge);

					if (isConvex) {
						dgPolyhedra::Iterator iter2(flatFace);
						for (iter2.Begin(); iter2; iter2++) {
							ptr = &(*iter2);
							if (ptr->m_incidentFace < 0) {
								if (ptr->m_mark < mark) {
									isConvex = 0;
									break;
								}
							}
						}
					}

					if (isConvex) {
						if (diagonalCount > 2) {
							dgInt32 count = 0;
							ptr = edge;
							do {
								polygon[count] = ptr->m_incidentVertex;
								count++;
								dgAssert(count < dgInt32(sizeof(polygon) / sizeof(polygon[0])));
								ptr = ptr->m_next;
							} while (ptr != edge);

							for (dgInt32 i = 0; i < count - 1; i++) {
								for (dgInt32 j = i + 1; j < count; j++) {
									if (polygon[i] == polygon[j]) {
										i = count;
										isConvex = 0;
										break;
									}
								}
							}
						}
					}

					if (isConvex) {
						for (dgInt32 j = 0; j < diagonalCount; j++) {
							dgEdge* const diagonal = diagonalsPool[j];
							flatFace.DeleteEdge(diagonal);
						}
					}
					else {
						for (dgInt32 j = 0; j < diagonalCount; j++) {
							dgEdge* const diagonal = diagonalsPool[j];
							if (!IsEssensialDiagonal(diagonal, normal, vertex, stride)) {
								flatFace.DeleteEdge(diagonal);
							}
						}
					}
				}

				dgInt32 mark = flatFace.IncLRU();
				dgPolyhedra::Iterator iter1(flatFace);
				for (iter1.Begin(); iter1; iter1++) {
					dgEdge* const edge1 = &(*iter1);
					if (edge1->m_mark != mark) {
						if (edge1->m_incidentFace > 0) {
							dgEdge* ptr = edge1;
							dgInt32 diagonalCount1 = 0;
							do {
								polygon[diagonalCount1] = ptr->m_incidentVertex;
								diagonalCount1++;
								dgAssert(diagonalCount1 < dgInt32(sizeof(polygon) / sizeof(polygon[0])));
								ptr->m_mark = mark;
								ptr = ptr->m_next;
							} while (ptr != edge1);
							if (diagonalCount1 >= 3) {
								buildConvex.AddFace(diagonalCount1, polygon);
							}
						}
					}
				}
			}
			iter.Begin();
		}
	}

	buildConvex.EndFace();
	dgAssert(GetCount() == 0);
}

void dgPolyhedra::ConvexPartition (const dgFloat64* const vertex, dgInt32 strideInBytes, dgPolyhedra* const leftOversOut)
{
	if (GetCount()) {
		Triangulate (vertex, strideInBytes, leftOversOut);
		DeleteDegenerateFaces (vertex, strideInBytes, dgFloat32 (1.0e-5f));
		Optimize (vertex, strideInBytes, NULL, NULL, dgFloat32 (1.0e-3f));
		DeleteDegenerateFaces (vertex, strideInBytes, dgFloat32 (1.0e-5f));

		if (GetCount()) {
			dgPolyhedra buildConvex(GetAllocator());
			RemoveInteriorEdges (buildConvex, vertex, strideInBytes);
			SwapInfo(buildConvex);
		}
	}
}
