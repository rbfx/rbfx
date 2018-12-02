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

#ifndef __dgPolyhedra__
#define __dgPolyhedra__

#include "dgStdafx.h"
#include "dgList.h"
#include "dgTree.h"
#include "dgHeap.h"
#include "dgDebug.h"




class dgEdge;
class dgPlane;
class dgObb;
class dgMatrix;
class dgPolyhedra;
class dgVertexCollapseVertexMetric;

typedef dgInt64 dgEdgeKey;


DG_MSC_VECTOR_ALIGMENT
class dgEdge
{
	public:
	dgEdge ();
	dgEdge (dgInt32 vertex, dgInt32 face, dgUnsigned64 userdata = 0);
	~dgEdge ();

	dgInt32 m_incidentVertex;
	dgInt32 m_incidentFace;
	dgUnsigned64 m_userData;
	dgEdge* m_next;
	dgEdge* m_prev;
	dgEdge* m_twin;
	dgInt32 m_mark;
} DG_GCC_VECTOR_ALIGMENT;


class dgPolyhedra: public dgTree <dgEdge, dgEdgeKey>
{
	public:
	class dgPairKey
	{
		public:
		dgPairKey()
		{
		}

		dgPairKey(dgInt64 key)
			:m_key(dgUnsigned64(key))
		{
		}

		dgPairKey(dgInt32 keyHigh, dgInt32 keyLow)
			:m_keyLow(dgUnsigned32 (keyLow))
			,m_keyHigh(dgUnsigned32 (keyHigh))
		{
		}

		dgInt64 GetVal() const
		{
			return dgInt64(m_key);
		}

		dgInt32 GetLowKey() const
		{
			return dgInt32(m_keyLow);
		}

		dgInt32 GetHighKey() const
		{
			return dgInt32(m_keyHigh);
		}

		bool operator<(const dgPairKey& key) const
		{
			return m_key < key.m_key;
		}

		bool operator>(const dgPairKey& key) const
		{
			return m_key > key.m_key;
		}


		private:
		union {
			dgUnsigned64 m_key;
			struct {
				dgUnsigned32 m_keyLow;
				dgUnsigned32 m_keyHigh;
			};
		};
	};

	dgPolyhedra (dgMemoryAllocator* const allocator);
	dgPolyhedra (const dgPolyhedra &polyhedra);
	virtual ~dgPolyhedra();

	virtual void BeginFace();
	dgEdge* AddFace (dgInt32 v0, dgInt32 v1, dgInt32 v2);
	dgEdge* AddFace (dgInt32 count, const dgInt32* const index);
	dgEdge* AddFace (dgInt32 count, const dgInt32* const index, const dgInt64* const userdata);
	virtual bool EndFace ();
	virtual void DeleteFace(dgEdge* const edge);

	dgInt32 GetFaceCount() const;
	dgInt32 GetEdgeCount() const;
	dgInt32 GetLastVertexIndex() const;

	dgInt32 IncLRU() const;
	dgInt32 GetLRU() const;
	void SetLRU(dgInt32 lru) const;

	dgEdge* FindEdge (dgInt32 v0, dgInt32 v1) const;
	dgTreeNode* FindEdgeNode (dgInt32 v0, dgInt32 v1) const;

	dgEdge* AddHalfEdge (dgInt32 v0, dgInt32 v1);
	void DeleteEdge (dgEdge* const edge);
	void DeleteEdge (dgInt32 v0, dgInt32 v1);

	dgEdge* ConnectVertex (dgEdge* const e0, dgEdge* const e1);
	
	bool FlipEdge (dgEdge* const edge);
	dgEdge* SpliteEdge (dgInt32 newIndex, dgEdge* const edge);
	dgBigVector FaceNormal (const dgEdge* const face, const dgFloat64* const vertex, dgInt32 strideInBytes) const;

	void BeginConectedSurface() const;
	bool GetConectedSurface (dgPolyhedra &polyhedra) const;
	void EndConectedSurface() const;

	dgObb CalculateSphere (const dgFloat64* const vertex, dgInt32 strideInBytes, const dgMatrix* const basis = NULL) const;
	void ChangeEdgeIncidentVertex (dgEdge* const edge, dgInt32 newIndex);	
	void DeleteDegenerateFaces (const dgFloat64* const pool, dgInt32 dstStrideInBytes, dgFloat64 minArea);

	bool Optimize (const dgFloat64* const pool, dgInt32 strideInBytes, dgReportProgress normalizedProgress, void* const reportProgressUserData, dgFloat64 tol, dgInt32 maxFaceCount = 1<<28);
	void Triangulate (const dgFloat64* const vertex, dgInt32 strideInBytes, dgPolyhedra* const leftOversOut);
	void ConvexPartition (const dgFloat64* const vertex, dgInt32 strideInBytes, dgPolyhedra* const leftOversOut);
	dgEdge* CollapseEdge(dgEdge* const edge);
	bool PolygonizeFace (dgEdge* const face, const dgFloat64* const pool, dgInt32 stride);
	bool TriangulateFace (dgEdge* const face, const dgFloat64* const pool, dgInt32 stride);

	private:
	void RefineTriangulation (const dgFloat64* const vertex, dgInt32 stride);
	void RefineTriangulation (const dgFloat64* const vertex, dgInt32 stride, const dgBigVector& normal, dgInt32 perimeterCount, dgEdge** const perimeter);
	void OptimizeTriangulation (const dgFloat64* const vertex, dgInt32 strideInBytes);
	void RemoveInteriorEdges (dgPolyhedra& polyhedraOut, const dgFloat64* const vertex, dgInt32 strideInBytes);
	void MarkAdjacentCoplanarFaces (dgPolyhedra& polyhedraOut, dgEdge* const face, const dgFloat64* const pool, dgInt32 strideInBytes);
	dgEdge* FindEarTip (dgEdge* const face, const dgFloat64* const pool, dgInt32 stride, dgDownHeap<dgEdge*, dgFloat64>& heap, const dgBigVector &normal) const;
	dgEdge* TriangulateFace (dgEdge* const face, const dgFloat64* const pool, dgInt32 stride, dgDownHeap<dgEdge*, dgFloat64>& heap, dgBigVector* const faceNormalOut);
		
	void RemoveHalfEdge (dgEdge* const edge);
	dgEdge* OptimizeCollapseEdge (dgEdge* const edge);
	bool IsOkToCollapse (const dgBigVector* const pool, dgEdge* const edge) const;
	dgFloat64 EdgePenalty (const dgBigVector* const pool, dgEdge* const edge, dgFloat64 dist) const;
	dgBigPlane EdgePlane (dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const pool) const;
	void CalculateAllMetrics (dgVertexCollapseVertexMetric* const table, const dgBigVector* const pool) const;
	void CalculateVertexMetrics (dgVertexCollapseVertexMetric* const table, const dgBigVector* const pool, dgEdge* const edge) const;
	dgEdge* BestEdgePolygonizeFace(const dgBigVector& normal, dgEdge* const edge, const dgFloat64* const pool, dgInt32 stride, const dgBigVector& point) const;

	static dgInt32 GetInteriorDiagonals (dgPolyhedra& polyhedra, dgEdge** const diagonals, dgInt32 maxCount);
	static dgBigPlane UnboundedLoopPlane (dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const pool);
	static void RemoveOuterColinearEdges(dgPolyhedra& flatFace, const dgFloat64* const vertex, dgInt32 stride);
	static void RemoveInteriorColinearEdges(dgPolyhedra& flatFace, const dgFloat64* const vertex, dgInt32 stride);
	static bool IsEssensialDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const dgFloat64* const pool,  dgInt32 stride);
	static bool IsEssensialPointDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const dgFloat64* const pool, dgInt32 stride);
	
	
	mutable dgInt32 m_baseMark;
	mutable dgInt32 m_edgeMark;
	mutable dgInt32 m_faceSecuence;
	friend class dgPolyhedraDescriptor;
};


DG_INLINE dgEdge::dgEdge ()
{
}

DG_INLINE dgEdge::dgEdge (dgInt32 vertex, dgInt32 face, dgUnsigned64 userdata)
	:m_incidentVertex(vertex)
	,m_incidentFace(face)
	,m_userData(userdata)
	,m_next(NULL)
	,m_prev(NULL)
	,m_twin(NULL)
	,m_mark(0)
{
}

DG_INLINE dgEdge::~dgEdge ()
{
}



DG_INLINE void dgPolyhedra::BeginFace ()
{
}

DG_INLINE dgEdge* dgPolyhedra::AddFace (dgInt32 count, const dgInt32* const index) 
{
	return AddFace (count, index, NULL);
}

DG_INLINE dgEdge* dgPolyhedra::AddFace (dgInt32 v0, dgInt32 v1, dgInt32 v2)
{
	dgInt32 vertex[3];

	vertex [0] = v0;
	vertex [1] = v1;
	vertex [2] = v2;
	return AddFace (3, vertex, NULL);
}

DG_INLINE dgInt32 dgPolyhedra::GetEdgeCount() const
{
#ifdef _DEBUG
	dgInt32 edgeCount = 0;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		edgeCount ++;
	}
	dgAssert (edgeCount == GetCount());;
#endif
	return GetCount();
}

DG_INLINE dgInt32 dgPolyhedra::GetLastVertexIndex() const
{
	dgInt32 maxVertexIndex = -1;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		const dgEdge* const edge = &(*iter);
		if (edge->m_incidentVertex > maxVertexIndex) {
			maxVertexIndex = edge->m_incidentVertex;
		}
	}
	return maxVertexIndex + 1;
}


DG_INLINE dgInt32 dgPolyhedra::IncLRU() const
{	
	m_edgeMark ++;
	dgAssert (m_edgeMark < 0x7fffffff);
	return m_edgeMark;
}

DG_INLINE dgInt32 dgPolyhedra::GetLRU() const
{
	return m_edgeMark;
}

DG_INLINE void dgPolyhedra::SetLRU(dgInt32 lru) const
{
	if (lru > m_edgeMark) {
		m_edgeMark = lru;
	}
}

DG_INLINE void dgPolyhedra::BeginConectedSurface() const
{
	m_baseMark = IncLRU();
}

DG_INLINE void dgPolyhedra::EndConectedSurface() const
{
}


DG_INLINE dgPolyhedra::dgTreeNode* dgPolyhedra::FindEdgeNode (dgInt32 i0, dgInt32 i1) const
{
	dgPairKey key (i0, i1);
	return Find (key.GetVal());
}


DG_INLINE dgEdge *dgPolyhedra::FindEdge (dgInt32 i0, dgInt32 i1) const
{
	//	dgTreeNode *node;
	//	dgPairKey key (i0, i1);
	//	node = Find (key.GetVal());
	//	return node ? &node->GetInfo() : NULL;
	dgTreeNode* const node = FindEdgeNode (i0, i1);
	return node ? &node->GetInfo() : NULL;
}

DG_INLINE void dgPolyhedra::DeleteEdge (dgInt32 v0, dgInt32 v1)
{
	dgPairKey pairKey (v0, v1);
	dgTreeNode* const node = Find(pairKey.GetVal());
	dgEdge* const edge = node ? &node->GetInfo() : NULL;
	if (!edge) {
		return;
	}
	DeleteEdge (edge);
}


#endif

