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

#ifndef __DG_AABB_POLYGON_SOUP_H_
#define __DG_AABB_POLYGON_SOUP_H_

#include "dgStdafx.h"
#include "dgIntersections.h"
#include "dgPolygonSoupDatabase.h"


class dgPolygonSoupDatabaseBuilder;


class dgAABBPolygonSoup: public dgPolygonSoupDatabase
{
	public:
	class dgNode
	{
		public:
		enum dgNodeType
		{
			m_binary = 0,
			m_leaf,
		};

		class dgLeafNodePtr
		{
			#define DG_INDEX_COUNT_BITS 6

			public:
			DG_INLINE dgLeafNodePtr ()
			{
				dgAssert (0);
			}

			DG_INLINE dgLeafNodePtr (dgUnsigned32 node)
			{
				m_node = node;
				dgAssert (!IsLeaf());
			}

			DG_INLINE dgUnsigned32 IsLeaf () const 
			{
				return m_node & 0x80000000;
			}

			DG_INLINE dgUnsigned32 GetCount() const 
			{
				dgAssert (IsLeaf());
				return (m_node & (~0x80000000)) >> (32 - DG_INDEX_COUNT_BITS - 1);
			}

			DG_INLINE dgUnsigned32 GetIndex() const 
			{
				dgAssert (IsLeaf());
				return m_node & (~(-(1 << (32 - DG_INDEX_COUNT_BITS - 1))));
			}


			DG_INLINE dgLeafNodePtr (dgUnsigned32 faceIndexCount, dgUnsigned32 faceIndexStart)
			{
				dgAssert (faceIndexCount < (1<<DG_INDEX_COUNT_BITS));
				m_node = 0x80000000 | (faceIndexCount << (32 - DG_INDEX_COUNT_BITS - 1)) | faceIndexStart;
			}

			DG_INLINE dgNode* GetNode (const void* const root) const
			{
				return ((dgNode*) root) + m_node;
			}

			union {
				dgUnsigned32 m_node;
			};
		};


		dgNode ()
			:m_indexBox0(0)
			,m_indexBox1(0)
			,m_left(0)
			,m_right(0)
		{
		}

		DG_INLINE dgFloat32 RayDistance (const dgFastRayTest& ray, const dgTriplex* const vertexArray) const
		{
			dgVector minBox (&vertexArray[m_indexBox0].m_x);
			dgVector maxBox (&vertexArray[m_indexBox1].m_x);
			minBox = minBox & dgVector::m_triplexMask;
			maxBox = maxBox & dgVector::m_triplexMask;
			return ray.BoxIntersect(minBox, maxBox);
		}

		DG_INLINE dgFloat32 BoxPenetration (const dgFastAABBInfo& obb, const dgTriplex* const vertexArray) const
		{
			dgVector p0 (&vertexArray[m_indexBox0].m_x);
			dgVector p1 (&vertexArray[m_indexBox1].m_x);
			p0 = p0 & dgVector::m_triplexMask;
			p1 = p1 & dgVector::m_triplexMask;
			dgVector minBox (p0 - obb.m_p1);
			dgVector maxBox (p1 - obb.m_p0);
			dgAssert(maxBox.m_x >= minBox.m_x);
			dgAssert(maxBox.m_y >= minBox.m_y);
			dgAssert(maxBox.m_z >= minBox.m_z);

			dgVector mask ((minBox * maxBox) < dgVector::m_zero);
			dgVector dist (maxBox.GetMin (minBox.Abs()) & mask);
			dist = dist.GetMin(dist.ShiftTripleRight());
			dist = dist.GetMin(dist.ShiftTripleRight());

			if (dist.GetScalar() > dgFloat32 (0.0f)) {
				dgVector origin (dgVector::m_half * (p1 + p0));
				dgVector size (dgVector::m_half * (p1 - p0));

				origin = obb.UntransformVector(origin);
				size = obb.m_absDir.RotateVector(size);
				dgVector q0 (origin - size);
				dgVector q1 (origin + size);
				dgVector minBox1 (q0 - obb.m_size);
				dgVector maxBox1 (q1 + obb.m_size);
				dgAssert(maxBox1.m_x >= minBox1.m_x);
				dgAssert(maxBox1.m_y >= minBox1.m_y);
				dgAssert(maxBox1.m_z >= minBox1.m_z);
				dgVector mask1 ((minBox1 * maxBox1) < dgVector::m_zero);
				dgVector dist1 (maxBox1.GetMin (minBox1.Abs()) & mask1);
				dist1 = dist1.GetMin(dist1.ShiftTripleRight());
				dist1 = dist1.GetMin(dist1.ShiftTripleRight());
				dist = dist.GetMin(dist1);
			} else {
				dgVector p1p0((minBox.Abs()).GetMin(maxBox.Abs()).AndNot(mask));
				dist = p1p0.DotProduct(p1p0);
				dist = dist.Sqrt() * dgVector::m_negOne;
			}
			return	dist.GetScalar();
		}

		DG_INLINE dgFloat32 BoxIntersect (const dgFastRayTest& ray, const dgFastRayTest& obbRay, const dgFastAABBInfo& obb, const dgTriplex* const vertexArray) const
		{
			dgVector p0 (&vertexArray[m_indexBox0].m_x);
			dgVector p1 (&vertexArray[m_indexBox1].m_x);
			p0 = p0 & dgVector::m_triplexMask;
			p1 = p1 & dgVector::m_triplexMask;

			dgVector minBox (p0 - obb.m_p1);
			dgVector maxBox (p1 - obb.m_p0);
			dgFloat32 dist = ray.BoxIntersect(minBox, maxBox);
			if (dist < dgFloat32 (1.0f)) {
				dgVector origin (dgVector::m_half * (p1 + p0));
				dgVector size (dgVector::m_half * (p1 - p0));

				origin = obb.UntransformVector(origin);
				size = obb.m_absDir.RotateVector(size);
				dgVector q0 (origin - size);
				dgVector q1 (origin + size);

				dgVector minBox1 (q0 - obb.m_size);
				dgVector maxBox1 (q1 + obb.m_size);
				dgFloat32 dist1 = obbRay.BoxIntersect(minBox1, maxBox1);
				dist = (dist1  > dgFloat32 (1.0f)) ? dist1 : dgMax (dist1, dist);
			}
			return dist;
		}

		dgInt32 m_indexBox0;
		dgInt32 m_indexBox1;
		dgLeafNodePtr m_left;
		dgLeafNodePtr m_right;
	};

	class dgSpliteInfo;
	class dgNodeBuilder;

	virtual void GetAABB (dgVector& p0, dgVector& p1) const;
	virtual void Serialize (dgSerialize callback, void* const userData) const;
	virtual void Deserialize (dgDeserialize callback, void* const userData, dgInt32 revisionNumber);

	protected:
	dgAABBPolygonSoup ();
	virtual ~dgAABBPolygonSoup ();

	void Create (const dgPolygonSoupDatabaseBuilder& builder, bool optimizedBuild);
	void CalculateAdjacendy ();
	virtual void ForAllSectorsRayHit (const dgFastRayTest& ray, dgFloat32 maxT, dgRayIntersectCallback callback, void* const context) const;
	virtual void ForAllSectors (const dgFastAABBInfo& obbAabb, const dgVector& boxDistanceTravel, dgFloat32 m_maxT, dgAABBIntersectCallback callback, void* const context) const;
	

	DG_INLINE void* GetRootNode() const 
	{
		return m_aabb;
	}

	DG_INLINE void* GetBackNode(const void* const root) const 
	{
		dgNode* const node = (dgNode*) root;
		return node->m_left.IsLeaf() ? NULL : node->m_left.GetNode(m_aabb);
	}

	DG_INLINE void* GetFrontNode(const void* const root) const 
	{
		dgNode* const node = (dgNode*) root;
		return node->m_right.IsLeaf() ? NULL : node->m_right.GetNode(m_aabb);
	}

	DG_INLINE void GetNodeAABB(const void* const root, dgVector& p0, dgVector& p1) const 
	{
		const dgNode* const node = (dgNode*)root;
		p0 = dgVector (&((dgTriplex*)m_localVertex)[node->m_indexBox0].m_x);
		p1 = dgVector (&((dgTriplex*)m_localVertex)[node->m_indexBox1].m_x);
		p0 = p0 & dgVector::m_triplexMask;
		p1 = p1 & dgVector::m_triplexMask;
	}

	virtual dgVector ForAllSectorsSupportVectex (const dgVector& dir) const;

	private:
	dgNodeBuilder* BuildTopDown (dgNodeBuilder* const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgNodeBuilder** const allocator) const;
	dgFloat32 CalculateFaceMaxSize (const dgVector* const vertex, dgInt32 indexCount, const dgInt32* const indexArray) const;
//	static dgIntersectStatus CalculateManifoldFaceEdgeNormals (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount);
	static dgIntersectStatus CalculateDisjointedFaceEdgeNormals (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);
	static dgIntersectStatus CalculateAllFaceEdgeNormals (void* const context, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);
	void ImproveNodeFitness (dgNodeBuilder* const node) const;

	dgInt32 m_nodesCount;
	dgInt32 m_indexCount;
	dgNode* m_aabb;
	dgInt32* m_indices;
};


#endif


