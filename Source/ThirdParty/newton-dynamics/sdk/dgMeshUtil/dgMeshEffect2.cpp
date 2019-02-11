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
#include "dgCollisionConvexHull.h"



class dgRayTrataAABBAccelerator: public dgMeshEffect::dgMeshBVH
{
	public:
	dgRayTrataAABBAccelerator(const dgMeshEffect* const tetraMesh)
		:dgMeshEffect::dgMeshBVH(tetraMesh)
	{
		Build();
	}

	dgMeshBVHNode* CreateLeafNode(dgEdge* const face, void* const userData)
	{
		dgMemoryAllocator* const allocator = m_mesh->GetAllocator();
		dgMeshBVHNode* const node = new (allocator)dgMeshBVHNode(m_mesh, face, userData);

		dgInt32 mark = m_mesh->GetLRU();

		dgVector p0(m_mesh->GetVertex(face->m_twin->m_prev->m_incidentVertex));
		dgVector p1(p0);
		dgEdge* faceEdge = face;
		do {
			dgEdge* twinFace = faceEdge->m_twin;
			do {
				twinFace->m_mark = mark;
				twinFace = twinFace->m_next;
			} while (twinFace != faceEdge->m_twin);

			dgVector point(m_mesh->GetVertex(faceEdge->m_incidentVertex));
			p0 = point.GetMin(p0);
			p1 = point.GetMax(p1);

			faceEdge = faceEdge->m_next;
		} while (faceEdge != face);

		dgVector padding(dgFloat32(0.01f));
		p0 -= padding;
		p1 += padding;
		node->SetBox(p0, p1);
		return node;
	}
};


// idea taken from paper: Fast Tetrahedral Meshes with Good Dihedral Angles, by Francois Labelle Jonathan Richard Shewchuk
// but quite different approach.
class dgTetraIsoSufaceStuffing
{
	public:
	enum dgVertexSign
	{
		m_onSuface,
		m_inside,
		m_outside,
	};

	class dgGridDimension 
	{
		public:
		dgBigVector m_origin;
		dgFloat64 m_cellSize;
		dgFloat64 m_diameter;
		dgInt32 m_gridSizeX;
		dgInt32 m_gridSizeY;
		dgInt32 m_gridSizeZ;
		dgInt32 m_innerSize;
		dgInt32 m_outerSize;
	};

	template<class T, dgInt32 size>
	class dgAssessor 
	{
		public:
		dgAssessor() 
			:m_count(0)
		{
		}

		dgInt32 GetCount() const 
		{
			return m_count; 
		}

		const T& operator[] (dgInt32 i) const
		{
			dgAssert(i >= 0);
			dgAssert(i < size);
			return m_elements[i];
		}

		T& operator[] (dgInt32 i)
		{
			dgAssert (i >= 0);
			dgAssert (i < size);
			return m_elements[i];
		}

		void PushBack (const T data)
		{
			dgAssert(m_count >= 0);
			dgAssert(m_count < size);
			m_elements[m_count] = data;
			m_count ++;
		}

		private:
		dgInt32 m_count;
		T m_elements[size];
	};

	typedef dgAssessor<dgInt32, 4> dgTetrahedra; 
	typedef dgAssessor<dgFloat32, 6> dgTetraEdgeCuts; 
	typedef dgAssessor<dgInt32, 32> dgTetraToVertexNode; 

	class dgNormalMap
	{
		public:
		dgNormalMap()
			:m_count(sizeof (m_normal)/sizeof (m_normal[0]))
		{
			dgVector p0(dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
			dgVector p1(dgFloat32(-1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
			dgVector p2(dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f));
			dgVector p3(dgFloat32(0.0f), dgFloat32(-1.0f), dgFloat32(0.0f), dgFloat32(0.0f));
			dgVector p4(dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f));
			dgVector p5(dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(-1.0f), dgFloat32(0.0f));

			dgInt32 count = 0;
			dgInt32 subdivitions = 1;
			TessellateTriangle(subdivitions, p4, p0, p2, count);
			TessellateTriangle(subdivitions, p0, p5, p2, count);
			TessellateTriangle(subdivitions, p5, p1, p2, count);
			TessellateTriangle(subdivitions, p1, p4, p2, count);
			TessellateTriangle(subdivitions, p0, p4, p3, count);
			TessellateTriangle(subdivitions, p5, p0, p3, count);
			TessellateTriangle(subdivitions, p1, p5, p3, count);
			TessellateTriangle(subdivitions, p4, p1, p3, count);
		}

		private:
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
				dgInt32 index = dgBitReversal(count, sizeof (m_normal) / sizeof (m_normal[0]));
				m_normal[index] = n;
				count++;
				dgAssert(count <= sizeof (m_normal) / sizeof (m_normal[0]));
			}
		}

		public:
		dgBigVector m_normal[32];
		dgInt32 m_count;
	};

	class dgClosePointsAccelerator: public dgMeshEffect::dgMeshBVH
	{
		public:
		dgClosePointsAccelerator(const dgMeshEffect* const mesh)
			:dgMeshEffect::dgMeshBVH(mesh)
		{
			Build();
		}

		dgMeshBVHNode* CreateLeafNode(dgEdge* const face, void* const userData)
		{
			dgMemoryAllocator* const allocator = m_mesh->GetAllocator();
			dgMeshBVHNode* const node = new (allocator)dgMeshBVHNode(m_mesh, face, userData);

			dgInt32 mark = m_mesh->GetLRU();
			dgAssert(mark != face->m_mark);

			dgEdge* faceEdge = face;
			do {
				faceEdge->m_mark = mark;
				faceEdge = faceEdge->m_twin->m_next;
			} while (faceEdge != face);

			dgVector padding(dgFloat32(1.0f / 32.0f));
			dgVector p(m_mesh->GetVertex(face->m_incidentVertex));
			dgVector p0(p - padding);
			dgVector p1(p + padding);
			node->SetBox(p0, p1);
			return node;
		}

		bool DoesTetrahedrumHasInsidePoints (const dgArray<dgBigVector>& points, const dgTetrahedra& tetra) const 
		{
			dgBigVector box0(dgFloat64( 1.0e10f));
			dgBigVector box1(dgFloat64(-1.0e10f));
			for (dgInt32 i = 0; i < 4; i++) {
				box0 = box0.GetMin(points[tetra[i]]);
				box1 = box1.GetMax(points[tetra[i]]);
			}

			dgBigVector padding (dgFloat64 (0.01f));
			box0 -= padding;
			box1 += padding;

			dgList<dgMeshBVH::dgMeshBVHNode*> overlapNodes(m_mesh->GetAllocator());
			GetOverlapNodes(overlapNodes, box0, box1);
			if (overlapNodes.GetCount()) {
				dgBigVector p0(points[tetra[0]]);
				dgBigVector p1(points[tetra[1]]);
				dgBigVector p2(points[tetra[2]]);
				dgBigVector p3(points[tetra[3]]);

				for (dgList<dgMeshBVH::dgMeshBVHNode*>::dgListNode* node = overlapNodes.GetFirst(); node; node = node->GetNext()) {
					dgEdge* const edge = node->GetInfo()->m_face;
					dgBigVector point(m_mesh->GetVertex(edge->m_incidentVertex));
					dgBigVector closestPoint(dgPointToTetrahedrumDistance(point, p0, p1, p2, p3));
					dgBigVector error(closestPoint - point);
					dgFloat64 error2 = error.DotProduct3(error);
					if (error2 < dgFloat64(1.0e-8f)) {
						return true;
					}
				}
			}

			return false;
		}
	};

	class dgRayTraceAccelerator: public dgMeshEffect::dgMeshBVH
	{
		enum dgTraceType
		{
			m_pointSide,
			m_pointOnSurface,
		};

		class dgRayTraceBase
		{
			public:
			dgRayTraceBase(dgTraceType type)
				:m_type(type)
			{
			}
			dgTraceType m_type;
		};

		class dgRayTracePointSide: public dgRayTraceBase
		{
			public:
			dgRayTracePointSide()
				:dgRayTraceBase(m_pointSide)
				,m_hitCount(0)
				,m_rayIsDegenerate(true)
			{
			}
			dgInt32 m_hitCount;
			bool m_rayIsDegenerate;
		};

		class dgRayPointOnSurface: public dgRayTraceBase
		{
			public:
			dgRayPointOnSurface()
				:dgRayTraceBase(m_pointOnSurface)
				,m_param(2.0f)
			{
			}

			dgFloat32 m_param;
		};

		public:
		dgRayTraceAccelerator(const dgMeshEffect* const mesh, dgFloat64 diameter)
			:dgMeshEffect::dgMeshBVH(mesh)
			,m_normals()
			,m_diameter(diameter)
		{
			Build();
		}

		dgMeshBVHNode* CreateLeafNode(dgEdge* const face, void* const userData)
		{
			dgMemoryAllocator* const allocator = m_mesh->GetAllocator();
			dgMeshBVHNode* const node = new (allocator)dgMeshBVHNode(m_mesh, face, userData);
			dgInt32 mark = m_mesh->GetLRU();
			dgAssert(mark != face->m_mark);

			dgEdge* faceEdge = face;
			do {
				faceEdge->m_mark = mark;
				faceEdge = faceEdge->m_next;
			} while (faceEdge != face);
			return node;
		}

		dgFloat64 dgPointToRayDistance (const dgBigVector& point, const dgBigVector& ray_p0, const dgBigVector& ray_p1, dgRayTracePointSide* const rayType) const
		{
			dgBigVector dp(ray_p1 - ray_p0);
			dgFloat64 den = dp.DotProduct3(dp);
			dgFloat64 num = dp.DotProduct3(point - ray_p0);
			if ((num >= dgFloat64 (0.0f)) && (num <= den)) { 
				dgBigVector p (ray_p0 + dp.Scale (num / den));
				dgBigVector dist (point - p);
				if (dist.DotProduct3(dist) < dgFloat64 (1.0e-12f)) {
					rayType->m_rayIsDegenerate = true;
					return dgFloat64 (-2.0f);
				}
			}
			return dgFloat64 (2.0f);
		}

		dgFloat64 PointSideTest(const dgMeshBVHNode* const faceNode, const dgBigVector& point0, const dgBigVector& point1, dgRayTracePointSide* const rayType) const
		{
			const dgEdge* const edge = faceNode->m_face;
			const dgBigVector p0 (m_mesh->GetVertex(edge->m_incidentVertex));
			const dgBigVector p1 (m_mesh->GetVertex(edge->m_next->m_incidentVertex));
			const dgBigVector p2 (m_mesh->GetVertex(edge->m_next->m_next->m_incidentVertex));

			const dgBigVector e10(p1 - p0);
			const dgBigVector e20(p2 - p0);
			const dgFloat64 a00 = e10.DotProduct(e10).GetScalar();
			const dgFloat64 a11 = e20.DotProduct(e20).GetScalar();
			const dgFloat64 a01 = e10.DotProduct(e20).GetScalar();

			const dgFloat64 det = a00 * a11 - a01 * a01;
			dgAssert(det >= dgFloat32(0.0f));
			if (dgAbs(det) > dgFloat32(1.0e-24f)) {
				dgBigVector p0Point(point0 - p0);
				dgBigVector normal(e10.CrossProduct(e20));
				dgFloat64 t = -normal.DotProduct3(p0Point) / normal.DotProduct3(point1 - point0);
				if ((t > dgFloat64(0.0f)) && (t < dgFloat64(1.0f))) {
					dgBigVector point(point0 + (point1 - point0).Scale(t));
					dgBigVector variPoint(point - p0);	
					const dgFloat64 b0 = e10.DotProduct(variPoint).GetScalar();
					const dgFloat64 b1 = e20.DotProduct(variPoint).GetScalar();

					dgFloat64 beta = b1 * a00 - a01 * b0;
					dgFloat64 alpha = b0 * a11 - a01 * b1;

					if (beta <= dgFloat32(0.0f)) {
						return dgPointToRayDistance (point, p0, p1, rayType);
					} else if (alpha <= dgFloat32(0.0f)) {
						return dgPointToRayDistance (point, p0, p2, rayType);
					} else if ((alpha + beta) >= det) {
						return dgPointToRayDistance (point, p1, p2, rayType);
					}
					rayType->m_hitCount ++;
				}
			}
			return dgFloat64 (2.0f);
		}


		dgFloat64 PointSurfaceHit(const dgMeshBVHNode* const faceNode, const dgBigVector& point0, const dgBigVector& point1, dgRayPointOnSurface* const rayType) const
		{
			const dgEdge* const edge = faceNode->m_face;
			const dgBigVector p0(m_mesh->GetVertex(edge->m_incidentVertex));
			const dgBigVector p1(m_mesh->GetVertex(edge->m_next->m_incidentVertex));
			const dgBigVector p2(m_mesh->GetVertex(edge->m_next->m_next->m_incidentVertex));

			const dgBigVector e10(p1 - p0);
			const dgBigVector e20(p2 - p0);
			const dgFloat64 a00 = e10.DotProduct(e10).GetScalar();
			const dgFloat64 a11 = e20.DotProduct(e20).GetScalar();
			const dgFloat64 a01 = e10.DotProduct(e20).GetScalar();

			const dgFloat64 det = a00 * a11 - a01 * a01;
			dgAssert(det >= dgFloat32(0.0f));
			if (dgAbs(det) > dgFloat32(1.0e-24f)) {
				dgBigVector p0Point(point0 - p0);
				dgBigVector normal(e10.CrossProduct(e20));
				dgFloat64 t = -normal.DotProduct3(p0Point) / normal.DotProduct3(point1 - point0);
				if ((t > dgFloat64(0.0f)) && (t < dgFloat64(1.0f))) {
					dgBigVector point(point0 + (point1 - point0).Scale(t));
					dgBigVector variPoint(point - p0);
					const dgFloat64 b0 = e10.DotProduct(variPoint).GetScalar();
					const dgFloat64 b1 = e20.DotProduct(variPoint).GetScalar();

					dgFloat64 beta = b1 * a00 - a01 * b0;
					dgFloat64 alpha = b0 * a11 - a01 * b1;

					if (beta <= dgFloat32(0.0f)) {
						return dgFloat64(2.0f);
					} else if (alpha <= dgFloat32(0.0f)) {
						return dgFloat64(2.0f);
					} else if ((alpha + beta) >= det) {
						return dgFloat64(2.0f);
					}
					if (t < rayType->m_param) {
						rayType->m_param = dgFloat32 (t);
					}
				}
			}
			return dgFloat64(2.0f);
		}

		dgFloat64 RayFaceIntersect(const dgMeshBVHNode* const faceNode, const dgBigVector& point0, const dgBigVector& point1, void* const userData) const
		{
			dgRayTraceBase* const rayType = (dgRayTraceBase*)userData;
			
			switch (rayType->m_type)
			{
				case m_pointSide:
					return PointSideTest(faceNode, point0, point1, (dgRayTracePointSide*) rayType);

				case m_pointOnSurface:
					return PointSurfaceHit(faceNode, point0, point1, (dgRayPointOnSurface*) rayType);
			}

			dgAssert (0);
			return dgFloat64 (-1.0f);
		}

		dgVertexSign CalculateVertexSide (const dgBigVector& point0) const
		{
			dgRayTracePointSide hits;
			for (dgInt32 i = 0; (i < m_normals.m_count) && hits.m_rayIsDegenerate; i ++) {
				hits.m_hitCount = 0;
				hits.m_rayIsDegenerate = false;
				dgBigVector point1 (point0 + dgBigVector (m_normals.m_normal[i].Scale (m_diameter))); 
				FaceRayCast (point0, point1, &hits);
			}
			dgAssert (!hits.m_rayIsDegenerate);
			return (hits.m_hitCount & 1) ? m_inside : m_outside;
		}

		dgFloat32 CalculateEdgeCut (const dgBigVector& point0, const dgBigVector& point1) const
		{
			dgRayPointOnSurface pointOnSurface;
			FaceRayCast (point0, point1, &pointOnSurface);
			return pointOnSurface.m_param;
		}

		dgNormalMap m_normals;
		dgFloat64 m_diameter;
	};

	dgTetraIsoSufaceStuffing(const dgMeshEffect* const mesh, dgFloat64 cellSize)
		:m_points(mesh->GetAllocator())
		,m_tetraList(mesh->GetAllocator())
		,m_pointCount(0)
		,m_tetraCount(0)
	{
		dgArray<dgVertexSign> vertexSide(mesh->GetAllocator());
		dgArray<dgTetraToVertexNode> tetraGraph(mesh->GetAllocator());
		dgArray<dgTetraEdgeCuts> tetraEdgeCuts(mesh->GetAllocator());

		dgGridDimension gridDim (CalculateGridSize(mesh, cellSize));
		dgClosePointsAccelerator closePointaAccelerator (mesh);
		dgRayTraceAccelerator rayAccelerator (mesh, gridDim.m_diameter);
		
		PopulateGridPoints (gridDim);
		CalculateVertexSide (vertexSide, rayAccelerator);
		BuildTetraGraph (gridDim, vertexSide, closePointaAccelerator, tetraGraph);
		//CalculateEdgeCuts (tetraEdgeCuts, tetraGraph, vertexSide, rayAccelerator);
		//SnapClosePoints (tetraEdgeCuts, tetraGraph, vertexSide, rayAccelerator);
	}

	void CalculateEdgeCuts (dgArray<dgTetraEdgeCuts>& tetraEdgeCuts, const dgArray<dgTetraToVertexNode>& tetraGraph, const dgArray<dgVertexSign>& vertexSide, const dgRayTraceAccelerator& rayAccelerator)
	{
		tetraEdgeCuts.Resize(m_tetraCount);
		for (dgInt32 i = 0; i < m_tetraCount; i ++) {
			tetraEdgeCuts[i] = dgTetraEdgeCuts();
			for (dgInt32 j = 0; j < 6; j ++) {
				tetraEdgeCuts[i].PushBack(dgFloat32(-1.0f));
			}
		}

		for (dgInt32 i = 0; i < m_pointCount; i ++) {
			if (vertexSide[i] == m_outside) {
				const dgTetraToVertexNode& graphNode = tetraGraph[i];
				for (dgInt32 j = 0; j < graphNode.GetCount(); j ++) {
					dgTetraEdgeCuts& cuts = tetraEdgeCuts[graphNode[j]];
					const dgTetrahedra& tetra = m_tetraList[graphNode[j]];
					dgAssert ((tetra[0] == i) || (tetra[1] == i) || (tetra[2] == i) || (tetra[3] == i));
					
					dgInt32 index = 0;
					for (dgInt32 i0 = 0; i0 < 3; i0 ++) {
						const dgBigVector& p0 = m_points[tetra[i0]];
						for (dgInt32 i1 = i0 + 1; i1 < 4; i1 ++) {
							if ((tetra[i0] == i) && (vertexSide[tetra[i1]] == m_inside)) {
								const dgBigVector& p1 = m_points[tetra[i1]];
								dgFloat32 param = rayAccelerator.CalculateEdgeCut (p0, p1);
								cuts[index] = param;
							}
							index ++;
							dgAssert (index <= 6);
						}
					}
				}
			}
		}
	}

	void SnapClosePoints (dgArray<dgTetraEdgeCuts>& tetraEdgeCuts, const dgArray<dgTetraToVertexNode>& tetraGraph, const dgArray<dgVertexSign>& vertexSide, const dgRayTraceAccelerator& rayAccelerator)
	{
		for (dgInt32 i = 0; i < m_pointCount; i++) {
			if (vertexSide[i] == m_outside) {
/*
				const dgTetraToVertexNode& graphNode = tetraGraph[i];
				for (dgInt32 j = 0; j < graphNode.GetCount(); j++) {
					dgTetraEdgeCuts& cuts = tetraEdgeCuts[graphNode[j]];
					const dgTetrahedra& tetra = m_tetraList[graphNode[j]];
					dgAssert((tetra[0] == i) || (tetra[1] == i) || (tetra[2] == i) || (tetra[3] == i));

					dgInt32 index = 0;
					for (dgInt32 i0 = 0; i0 < 3; i0++) {
						const dgBigVector& p0 = m_points[tetra[i0]];
						for (dgInt32 i1 = i0 + 1; i1 < 4; i1++) {
							if ((tetra[i0] == i) && (vertexSide[tetra[i1]] == m_inside)) {
								const dgBigVector& p1 = m_points[tetra[i1]];
								dgFloat32 param = rayAccelerator.CalculateEdgeCut(p0, p1);
								cuts[index] = param;
							}
							index++;
							dgAssert(index <= 6);
						}
					}
				}
*/
			}
		}
	}

	void CalculateVertexSide (dgArray<dgVertexSign>& vertexSide, const dgRayTraceAccelerator& rayAccelerator)
	{
		vertexSide.Resize(m_pointCount);
		for (dgInt32 i = 0; i < m_pointCount; i ++) {
			vertexSide[i] = rayAccelerator.CalculateVertexSide (m_points[i]);
		}
	}

	dgGridDimension CalculateGridSize(const dgMeshEffect* const mesh, dgFloat64 cellsize) const
	{
		dgBigVector minBox;
		dgBigVector maxBox;
		mesh->CalculateAABB(minBox, maxBox);
		minBox -= (maxBox - minBox).Scale(dgFloat64(1.e-3f));
		maxBox += (maxBox - minBox).Scale(dgFloat64(1.e-3f));

		dgBigVector mMinInt((minBox.Scale(dgFloat64(1.0f) / cellsize)).Floor());
		dgBigVector mMaxInt((maxBox.Scale(dgFloat64(1.0f) / cellsize)).Floor() + dgBigVector::m_one);
		dgBigVector gridSize(mMaxInt - mMinInt + dgBigVector::m_one);

		dgBigVector size(maxBox - minBox);
		
		dgGridDimension gridDimension;
		gridDimension.m_origin = minBox; 
		gridDimension.m_cellSize = cellsize;
		gridDimension.m_diameter = sqrt (size.DotProduct3(size));
		gridDimension.m_gridSizeX = dgInt32(gridSize.m_x);
		gridDimension.m_gridSizeY = dgInt32(gridSize.m_y);
		gridDimension.m_gridSizeZ = dgInt32(gridSize.m_z);

		gridDimension.m_innerSize = gridDimension.m_gridSizeX * gridDimension.m_gridSizeY * gridDimension.m_gridSizeZ;
		gridDimension.m_outerSize = gridDimension.m_innerSize + (gridDimension.m_gridSizeX + 1) * (gridDimension.m_gridSizeY + 1) * (gridDimension.m_gridSizeZ + 1);
		return gridDimension;
	}

	void PopulateGridPoints(const dgGridDimension& gridDimension)
	{
		m_pointCount = 0;
		m_points.Resize(gridDimension.m_outerSize);

		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX; x++) {
					m_points[m_pointCount] = gridDimension.m_origin + dgBigVector(x * gridDimension.m_cellSize, y * gridDimension.m_cellSize, z * gridDimension.m_cellSize, dgFloat64(0.0f));
					m_pointCount++;
				}
			}
		}

		dgBigVector outerOrigin(gridDimension.m_origin - dgBigVector(gridDimension.m_cellSize * dgFloat64(0.5f)));
		outerOrigin.m_w = dgFloat64 (0.0f);
		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ + 1; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY + 1; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX + 1; x++) {
					m_points[m_pointCount] = outerOrigin + dgBigVector(x * gridDimension.m_cellSize, y * gridDimension.m_cellSize, z * gridDimension.m_cellSize, dgFloat64(0.0f));
					m_pointCount++;
				}
			}
		}
	}

	void AddTetra(dgArray<dgTetraToVertexNode>& graph, const dgTetrahedra& tetra, const dgArray<dgVertexSign>& vertexSigns, const dgClosePointsAccelerator& closePoint)
	{
		dgAssert(CalculateVolume(tetra) > dgFloat64(0.0f));
		bool hasInsizePoints = false;
		hasInsizePoints = hasInsizePoints || (vertexSigns[tetra[0]] == m_inside);
		hasInsizePoints = hasInsizePoints || (vertexSigns[tetra[1]] == m_inside);
		hasInsizePoints = hasInsizePoints || (vertexSigns[tetra[2]] == m_inside);
		hasInsizePoints = hasInsizePoints || (vertexSigns[tetra[3]] == m_inside);
		hasInsizePoints = hasInsizePoints || closePoint.DoesTetrahedrumHasInsidePoints(m_points, tetra);

		if (hasInsizePoints) {
			m_tetraList[m_tetraCount] = tetra;
			dgTetrahedra& tetraEntry = m_tetraList[m_tetraCount];
			for (dgInt32 i = 0; i < 4; i ++) {
				dgInt32 vertexIndex = tetra[i];
				tetraEntry.PushBack(vertexIndex);
				graph[vertexIndex].PushBack(m_tetraCount);
			}
			m_tetraCount ++;
		}
	}

	void BuildTetraGraph(const dgGridDimension& gridDimension, const dgArray<dgVertexSign>& vertexSigns, const dgClosePointsAccelerator& closePoint, dgArray<dgTetraToVertexNode>& graph)
	{
		graph.Resize(m_pointCount);
		for (dgInt32 i = 0; i < m_pointCount; i ++) {
			graph[i] = dgTetraToVertexNode();
		}

		dgDelaunayTetrahedralization delaunayTetrahedras(m_points.GetAllocator(), &m_points[0].m_x, m_pointCount, sizeof (dgBigVector), dgFloat32(0.0f));
		delaunayTetrahedras.RemoveUpperHull();

		for (dgDelaunayTetrahedralization::dgListNode* node = delaunayTetrahedras.GetFirst(); node; node = node->GetNext()) {
			dgTetrahedra stuffingTetra;
			dgConvexHull4dTetraherum& delaunayTetra = node->GetInfo();
			
			for (dgInt32 i = 0; i < 4; i ++) {
				stuffingTetra[i] = delaunayTetra.m_faces[0].m_index[i];
			}
			dgFloat64 volume = CalculateVolume(stuffingTetra);
			if (volume < dgFloat64 (0.0f)) {
				dgSwap(stuffingTetra[0], stuffingTetra[1]);
			}
			AddTetra(graph, stuffingTetra, vertexSigns, closePoint);
		}

		
/*
		const dgInt32 base = gridDimension.m_innerSize;
		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX - 1; x++) {
					dgTetrahedra tetra;
					tetra[0] = ((z * gridDimension.m_gridSizeY + y) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + y) * gridDimension.m_gridSizeX) + x + 1;
					tetra[2] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 0) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					tetra[3] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);

					tetra[0] = ((z * gridDimension.m_gridSizeY + y) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + y) * gridDimension.m_gridSizeX) + x + 1;
					tetra[3] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 0) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					tetra[2] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);
				}
			}
		}

		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY - 1; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX; x++) {
					dgTetrahedra tetra;
					tetra[0] = ((z * gridDimension.m_gridSizeY + (y + 0)) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + (y + 1)) * gridDimension.m_gridSizeX) + x;
					tetra[3] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					tetra[2] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);

					tetra[0] = ((z * gridDimension.m_gridSizeY + (y + 0)) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + (y + 1)) * gridDimension.m_gridSizeX) + x;
					tetra[2] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					tetra[3] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					AddTetra(graph, tetra, vertexSigns, closePoint);
				}
			}
		}

		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ - 1; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX; x++) {
					dgTetrahedra tetra;
					tetra[0] = (((z + 0) * gridDimension.m_gridSizeY + y + 0) * gridDimension.m_gridSizeX) + x;
					tetra[1] = (((z + 1) * gridDimension.m_gridSizeY + y + 0) * gridDimension.m_gridSizeX) + x;
					tetra[3] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 0) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					tetra[2] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					AddTetra(graph, tetra, vertexSigns, closePoint);

					tetra[0] = (((z + 0) * gridDimension.m_gridSizeY + y + 0) * gridDimension.m_gridSizeX) + x;
					tetra[1] = (((z + 1) * gridDimension.m_gridSizeY + y + 0) * gridDimension.m_gridSizeX) + x;
					tetra[2] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 0) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					tetra[3] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);
				}
			}
		}

		for (dgInt32 z = 0; z < gridDimension.m_gridSizeZ; z++) {
			for (dgInt32 y = 0; y < gridDimension.m_gridSizeY - 1; y++) {
				for (dgInt32 x = 0; x < gridDimension.m_gridSizeX; x++) {
					dgTetrahedra tetra;
					tetra[0] = ((z * gridDimension.m_gridSizeY + (y + 0)) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + (y + 1)) * gridDimension.m_gridSizeX) + x;
					tetra[2] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					tetra[3] = base + (((z + 1) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);

					tetra[0] = ((z * gridDimension.m_gridSizeY + (y + 0)) * gridDimension.m_gridSizeX) + x;
					tetra[1] = ((z * gridDimension.m_gridSizeY + (y + 1)) * gridDimension.m_gridSizeX) + x;
					tetra[3] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 0;
					tetra[2] = base + (((z + 0) * (gridDimension.m_gridSizeY + 1) + y + 1) * (gridDimension.m_gridSizeX + 1)) + x + 1;
					AddTetra(graph, tetra, vertexSigns, closePoint);
				}
			}
		}
*/
	}

	dgFloat64 CalculateVolume(const dgTetrahedra& tetra) const
	{
		const dgBigVector& p0 = m_points[tetra[0]];
		const dgBigVector& p1 = m_points[tetra[1]];
		const dgBigVector& p2 = m_points[tetra[2]];
		const dgBigVector& p3 = m_points[tetra[3]];
		dgBigVector p10(p1 - p0);
		dgBigVector p20(p2 - p0);
		dgBigVector p30(p3 - p0);
		return p10.DotProduct3(p20.CrossProduct(p30));
	}

	dgArray<dgBigVector> m_points;
	dgArray<dgTetrahedra> m_tetraList;
	dgInt32 m_pointCount;
	dgInt32 m_tetraCount;
};

void dgMeshEffect::LoadOffMesh(const char* const fileName)
{
	class ParceOFF
	{
		public:
		enum Token
		{
			m_off,
			m_value,
			m_end,
		};

		ParceOFF(FILE* const file)
			:m_file(file)
		{
		}

		Token GetToken(char* const buffer) const
		{
			while (!feof(m_file) && fscanf(m_file, "%s", buffer)) {
				if (buffer[0] == '#') {
					SkipLine();
				} else {
					if (!_stricmp(buffer, "OFF")) {
						return m_off;
					}
					return m_value;
				}
			}
			return m_end;
		}

		char* SkipLine() const
		{
			char tmp[1024];
			return fgets(tmp, sizeof (tmp), m_file);
		}

		dgInt32 GetInteger() const
		{
			char buffer[1024];
			GetToken(buffer);
			return atoi(buffer);
		}

		dgFloat64 GetFloat() const
		{
			char buffer[1024];
			GetToken(buffer);
			return atof(buffer);
		}

		FILE* m_file;
	};

	FILE* const file = fopen(fileName, "rb");
	if (file) {
		ParceOFF parcel(file);

		dgInt32 vertexCount = 0;
		dgInt32 faceCount = 0;
		//dgInt32 edgeCount = 0;

		char buffer[1024];
		bool stillData = true;
		while (stillData) {
			ParceOFF::Token token = parcel.GetToken(buffer);
			switch (token) 
			{
				case ParceOFF::m_off:
				{
					vertexCount = parcel.GetInteger();
					faceCount = parcel.GetInteger();
					//					edgeCount = parcel.GetInteger();
					parcel.SkipLine();

					dgArray<dgBigVector> points(GetAllocator());
					for (dgInt32 i = 0; i < vertexCount; i++) {
						dgFloat64 x = parcel.GetFloat();
						dgFloat64 y = parcel.GetFloat();
						dgFloat64 z = parcel.GetFloat();
						dgBigVector p(x, y, z, dgFloat32(0.0f));
						points[i] = p;
					}

					dgArray<dgInt32> indexList(GetAllocator());
					dgArray<dgInt32> faceVertex(GetAllocator());
					dgInt32 index = 0;
					for (dgInt32 i = 0; i < faceCount; i++) {
						const dgInt32 faceVertexCount = parcel.GetInteger();
						faceVertex[i] = faceVertexCount;
						for (dgInt32 j = 0; j < faceVertexCount; j++) {
							indexList[index] = parcel.GetInteger();
							index++;
						}
						parcel.SkipLine();
					}

					dgMeshVertexFormat vertexFormat;
					vertexFormat.m_faceCount = faceCount;
					vertexFormat.m_faceIndexCount = &faceVertex[0];

					vertexFormat.m_vertex.m_data = &points[0].m_x;
					vertexFormat.m_vertex.m_strideInBytes = sizeof (dgBigVector);
					vertexFormat.m_vertex.m_indexList = &indexList[0];
					BuildFromIndexList(&vertexFormat);

					CalculateNormals(30.0f * dgDEG2RAD);
					stillData = false;
					break;
				}

				default:;
			}
		}

		fclose(file);
	}
}

void dgMeshEffect::LoadTetraMesh (const char* const filename)
{
	FILE* const file = fopen(filename, "rb");
	if (file) {
		dgInt32 vertexCount;
		size_t ret = fscanf(file, "%d", &vertexCount);
		dgArray<dgBigVector> points(GetAllocator());
		for (dgInt32 i = 0; i < vertexCount; i ++) {
			float x;
			float y;
			float z;
			ret = fscanf(file, "%f %f %f", &x, &y, &z);
			points[i] = dgBigVector (x, y, z, dgFloat32 (0.0f));
		}
		
		BeginBuild();
		dgInt32 tetras;
		ret = fscanf(file, "%d", &tetras);
		dgMemoryAllocator* const allocator = GetAllocator();
		for (dgInt32 layers = 0; layers < tetras; layers ++) {
			dgInt32 tetra[4];
			ret = fscanf(file, "%d %d %d %d", &tetra[0], &tetra[1], &tetra[2], &tetra[3]);
			ret = 0; 
			dgBigVector pointArray[4];
			for (dgInt32 i = 0; i < 4; i++) {
				dgInt32 index = tetra[i];
				pointArray[i] = points[index];
			}

			dgMeshEffect convexMesh(allocator, &pointArray[0].m_x, 4, sizeof (dgBigVector), dgFloat64(0.0f));

			dgAssert(convexMesh.GetCount());
			convexMesh.CalculateNormals(dgFloat32(30.0f * dgDEG2RAD));
			for (dgInt32 i = 0; i < convexMesh.m_points.m_vertex.m_count; i++) {
				convexMesh.m_points.m_layers[i] = layers;
			}
			MergeFaces(&convexMesh);
		}
		EndBuild(dgFloat64(1.0e-8f), false);
		fclose(file);
	}
}

dgMeshEffect* dgMeshEffect::CreateVoronoiConvexDecomposition (dgMemoryAllocator* const allocator, dgInt32 pointCount, dgInt32 pointStrideInBytes, const dgFloat32* const pointCloud, dgInt32 materialId, const dgMatrix& textureProjectionMatrix)
{
	dgStack<dgBigVector> buffer(pointCount + 16);
	dgBigVector* const pool = &buffer[0];
	dgInt32 count = 0;
	dgFloat64 quantizeFactor = dgFloat64 (16.0f);
	dgFloat64 invQuantizeFactor = dgFloat64 (1.0f) / quantizeFactor;
	dgInt32 stride = pointStrideInBytes / sizeof (dgFloat32); 

	dgBigVector pMin (dgFloat32 (1.0e10f), dgFloat32 (1.0e10f), dgFloat32 (1.0e10f), dgFloat32 (0.0f));
	dgBigVector pMax (dgFloat32 (-1.0e10f), dgFloat32 (-1.0e10f), dgFloat32 (-1.0e10f), dgFloat32 (0.0f));
	for (dgInt32 i = 0; i < pointCount; i ++) {
		dgFloat64 x = pointCloud[i * stride + 0];
		dgFloat64 y	= pointCloud[i * stride + 1];
		dgFloat64 z	= pointCloud[i * stride + 2];
		x = floor (x * quantizeFactor) * invQuantizeFactor;
		y = floor (y * quantizeFactor) * invQuantizeFactor;
		z = floor (z * quantizeFactor) * invQuantizeFactor;
		dgBigVector p (x, y, z, dgFloat64 (0.0f));
		pMin = dgBigVector (dgMin (x, pMin.m_x), dgMin (y, pMin.m_y), dgMin (z, pMin.m_z), dgFloat64 (0.0f));
		pMax = dgBigVector (dgMax (x, pMax.m_x), dgMax (y, pMax.m_y), dgMax (z, pMax.m_z), dgFloat64 (0.0f));
		pool[count] = p;
		count ++;
	}
	// add the bbox as a barrier
	pool[count + 0] = dgBigVector ( pMin.m_x, pMin.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 1] = dgBigVector ( pMax.m_x, pMin.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 2] = dgBigVector ( pMin.m_x, pMax.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 3] = dgBigVector ( pMax.m_x, pMax.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 4] = dgBigVector ( pMin.m_x, pMin.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 5] = dgBigVector ( pMax.m_x, pMin.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 6] = dgBigVector ( pMin.m_x, pMax.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 7] = dgBigVector ( pMax.m_x, pMax.m_y, pMax.m_z, dgFloat64 (0.0f));
	count += 8;

	dgStack<dgInt32> indexList(count);
	count = dgVertexListToIndexList(&pool[0].m_x, sizeof (dgBigVector), 3, count, &indexList[0], dgFloat64 (5.0e-2f));	
	dgAssert (count >= 8);

	dgFloat64 maxSize = dgMax(pMax.m_x - pMin.m_x, pMax.m_y - pMin.m_y, pMax.m_z - pMin.m_z);
	pMin -= dgBigVector (maxSize, maxSize, maxSize, dgFloat64 (0.0f));
	pMax += dgBigVector (maxSize, maxSize, maxSize, dgFloat64 (0.0f));

	// add the a guard zone, so that we do no have to clip
	dgInt32 guardVertexKey = count;
	pool[count + 0] = dgBigVector ( pMin.m_x, pMin.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 1] = dgBigVector ( pMax.m_x, pMin.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 2] = dgBigVector ( pMin.m_x, pMax.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 3] = dgBigVector ( pMax.m_x, pMax.m_y, pMin.m_z, dgFloat64 (0.0f));
	pool[count + 4] = dgBigVector ( pMin.m_x, pMin.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 5] = dgBigVector ( pMax.m_x, pMin.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 6] = dgBigVector ( pMin.m_x, pMax.m_y, pMax.m_z, dgFloat64 (0.0f));
	pool[count + 7] = dgBigVector ( pMax.m_x, pMax.m_y, pMax.m_z, dgFloat64 (0.0f));
	count += 8; 

	dgDelaunayTetrahedralization delaunayTetrahedras (allocator, &pool[0].m_x, count, sizeof (dgBigVector), dgFloat32 (0.0f));
	delaunayTetrahedras.RemoveUpperHull ();

//	delaunayTetrahedras.Save("xxx0.txt");
	dgInt32 tetraCount = delaunayTetrahedras.GetCount();
	dgStack<dgBigVector> voronoiPoints(tetraCount + 32);
	dgStack<dgDelaunayTetrahedralization::dgListNode*> tetradrumNode(tetraCount);
	dgTree<dgList<dgInt32>, dgInt32> delaunayNodes (allocator);	

	dgInt32 index = 0;
	const dgConvexHull4dVector* const convexHulPoints = delaunayTetrahedras.GetHullVertexArray();
	for (dgDelaunayTetrahedralization::dgListNode* node = delaunayTetrahedras.GetFirst(); node; node = node->GetNext()) {
		dgConvexHull4dTetraherum& tetra = node->GetInfo();
		voronoiPoints[index] = tetra.CircumSphereCenter (convexHulPoints);
		tetradrumNode[index] = node;

		for (dgInt32 i = 0; i < 4; i ++) {
			dgTree<dgList<dgInt32>, dgInt32>::dgTreeNode* header = delaunayNodes.Find(tetra.m_faces[0].m_index[i]);
			if (!header) {
				dgList<dgInt32> list (allocator);
				header = delaunayNodes.Insert(list, tetra.m_faces[0].m_index[i]);
			}
			header->GetInfo().Append (index);
		}
		index ++;
	}

	const dgFloat32 normalAngleInRadians = dgFloat32 (30.0f * dgDEG2RAD);
	dgMeshEffect* const voronoiPartition = new (allocator) dgMeshEffect (allocator);
	voronoiPartition->BeginBuild();
	dgInt32 layer = 0;
	dgTree<dgList<dgInt32>, dgInt32>::Iterator iter (delaunayNodes);
	for (iter.Begin(); iter; iter ++) {
		dgTree<dgList<dgInt32>, dgInt32>::dgTreeNode* const nodeNode = iter.GetNode();
		const dgList<dgInt32>& list = nodeNode->GetInfo();
		dgInt32 key = nodeNode->GetKey();

		if (key < guardVertexKey) {
			dgBigVector pointArray[512];
			dgInt32 indexArray[512];
			
			dgInt32 count1 = 0;
			for (dgList<dgInt32>::dgListNode* ptr = list.GetFirst(); ptr; ptr = ptr->GetNext()) {
				dgInt32 i = ptr->GetInfo();
				pointArray[count1] = voronoiPoints[i];
				count1 ++;
				dgAssert (count1 < dgInt32 (sizeof (pointArray) / sizeof (pointArray[0])));
			}

			count1 = dgVertexListToIndexList(&pointArray[0].m_x, sizeof (dgBigVector), 3, count1, &indexArray[0], dgFloat64 (1.0e-3f));	
			if (count1 >= 4) {
				dgMeshEffect convexMesh (allocator, &pointArray[0].m_x, count1, sizeof (dgBigVector), dgFloat64 (0.0f));
				if (convexMesh.GetCount()) {
					convexMesh.CalculateNormals(normalAngleInRadians);
					convexMesh.UniformBoxMapping (materialId, textureProjectionMatrix);
					for (dgInt32 i = 0; i < convexMesh.m_points.m_vertex.m_count; i ++) {
						convexMesh.m_points.m_layers[i] = layer;
					}
					voronoiPartition->MergeFaces(&convexMesh);
					layer ++;
				}
			}
		}
	}
	voronoiPartition->EndBuild(dgFloat64 (1.0e-8f), false);
	//voronoiPartition->SaveOFF("xxx0.off");
	return voronoiPartition;
}


dgMeshEffect* dgMeshEffect::CreateTetrahedraIsoSurface() const
{
/*
dgMeshEffect xxxx  (GetAllocator());
xxxx.BeginBuild();

xxxx.BeginBuildFace ();
xxxx.AddPoint (0.0, 0.0, -1.0);
xxxx.AddLayer (0);

xxxx.AddPoint (1.0, 0.0, 0.0);
xxxx.AddLayer (0);

xxxx.AddPoint (0.0, 0.0, 1.0);
xxxx.AddLayer (0);
xxxx.EndBuildFace ();

xxxx.BeginBuildFace ();
xxxx.AddPoint (0.0, 0.0, -1.0);
xxxx.AddLayer (1);

xxxx.AddPoint (0.0, 0.0, 1.0);
xxxx.AddLayer (1);

xxxx.AddPoint (-1.0, 0.0, 0.0);
xxxx.AddLayer (1);

xxxx.EndBuildFace ();
xxxx.EndBuild(dgFloat64(1.0e-8f), false);
*/


	dgTetraIsoSufaceStuffing tetraIsoStuffing (this, dgFloat64(0.125f));

	dgMeshEffect* delaunayPartition = NULL;
	if (tetraIsoStuffing.m_tetraCount) {
		dgMemoryAllocator* const allocator = GetAllocator();
		delaunayPartition = new (allocator) dgMeshEffect (allocator);
		delaunayPartition->BeginBuild();
		dgInt32 layer = 0;
		dgBigVector pointArray[4];
		for (dgInt32 j = 0; j < tetraIsoStuffing.m_tetraCount; j ++) {
			dgTetraIsoSufaceStuffing::dgTetrahedra& tetra = tetraIsoStuffing.m_tetraList[j];
			for (dgInt32 i = 0; i < 4; i ++) {
				dgInt32 index = tetra[i];
				pointArray[i] = tetraIsoStuffing.m_points[index];
			}
			dgMeshEffect convexMesh(allocator, &pointArray[0].m_x, 4, sizeof (dgBigVector), dgFloat64(0.0f));
			//dgAssert (convexMesh.GetCount());
			//convexMesh.CalculateNormals(dgFloat32 (30.0f * dgDEG2RAD));
			for (dgInt32 i = 0; i < convexMesh.m_points.m_vertex.m_count; i++) {
				convexMesh.m_points.m_layers[i] = layer;
			}
			delaunayPartition->MergeFaces(&convexMesh);
			layer++;
		}
		delaunayPartition->EndBuild(dgFloat64(1.0e-8f), false);
	}

	return delaunayPartition;
}

void dgMeshEffect::CreateTetrahedraLinearBlendSkinWeightsChannel (const dgMeshEffect* const tetrahedraMesh)
{
dgAssert(0);
/*
	dgRayTrataAABBAccelerator accelerator (tetrahedraMesh);
	m_points.m_weights.Clear();
	m_points.m_weights.Reserve(m_points.m_vertex.m_count);

	dgBigVector padding (dgFloat64(1.0f / 32.0f));
	for (dgInt32 i = 0; i < m_points.m_weights.m_count; i ++) {
		dgBigVector p (m_points.m_vertex[i]);
		dgBigVector p0 (p - padding);
		dgBigVector p1 (p + padding);
		dgList<dgMeshBVH::dgMeshBVHNode*> overlapNodes (GetAllocator());
		accelerator.GetOverlapNodes (overlapNodes, p0, p1);
		dgAssert (overlapNodes.GetCount());

		bool weightFound = false;
		for (dgList<dgMeshBVH::dgMeshBVHNode*>::dgListNode* node = overlapNodes.GetFirst(); node; node = node->GetNext()) {
			dgEdge* const edge = node->GetInfo()->m_face;

			dgInt32 i0 = edge->m_incidentVertex;
			dgInt32 i1 = edge->m_next->m_incidentVertex;
			dgInt32 i2 = edge->m_prev->m_incidentVertex;
			dgInt32 i3 = edge->m_twin->m_prev->m_incidentVertex;
			dgBigVector q0 (tetrahedraMesh->m_points.m_vertex[i0]);
			dgBigVector q1 (tetrahedraMesh->m_points.m_vertex[i1]);
			dgBigVector q2 (tetrahedraMesh->m_points.m_vertex[i2]);
			dgBigVector q3 (tetrahedraMesh->m_points.m_vertex[i3]);

			const dgBigVector e10(q1 - q0);
			const dgBigVector e20(q2 - q0);
			const dgBigVector e30(q3 - q0);

			dgAssert (e10.DotProduct(e10).GetScalar() > dgFloat32 (0.0f));
			const dgFloat64 d0 = sqrt(e10.DotProduct(e10).GetScalar());
			const dgFloat64 invd0 = dgFloat64(1.0f) / d0;
			const dgFloat64 l10 = e20.DotProduct(e10).GetScalar() * invd0;
			const dgFloat64 l20 = e30.DotProduct(e10).GetScalar() * invd0;

			dgAssert ((e20.DotProduct(e20).GetScalar() - l10 * l10) > dgFloat32 (0.0f));
			const dgFloat64 desc11 = e20.DotProduct(e20).GetScalar() - l10 * l10;

			const dgFloat64 d1 = sqrt(desc11);
			const dgFloat64 invd1 = dgFloat64(1.0f) / d1;
			const dgFloat64 l21 = (e30.DotProduct(e20).GetScalar() - l20 * l10) * invd1;
			dgAssert (e30.DotProduct(e30).GetScalar() - l20 * l20 - l21 * l21 > dgFloat32 (0.0f));
			const dgFloat64 desc22 = e30.DotProduct(e30).GetScalar() - l20 * l20 - l21 * l21;

			dgBigVector p0Point(p - q0);
			const dgFloat64 d2 = sqrt(desc22);
			const dgFloat64 invd2 = dgFloat64(1.0f) / d2;

			const dgFloat64 b0 = e10.DotProduct(p0Point).GetScalar();
			const dgFloat64 b1 = e20.DotProduct(p0Point).GetScalar();
			const dgFloat64 b2 = e30.DotProduct(p0Point).GetScalar();

			dgFloat64 u1 = b0 * invd0;
			dgFloat64 u2 = (b1 - l10 * u1) * invd1;
			dgFloat64 u3 = (b2 - l20 * u1 - l21 * u2) * invd2;

			u3 = u3 * invd2;
			u2 = (u2 - l21 * u3) * invd1;
			u1 = (u1 - l10 * u2 - l20 * u3) * invd0;
			if ((u1 >= dgFloat64(0.0f)) && (u2 >= dgFloat64(0.0f)) && (u3 >= dgFloat64(0.0f)) && ((u1 + u2 + u3) <= dgFloat64(1.0f))) {
				dgBigVector r0 (q0 + e10.Scale(u1) + e20.Scale(u2) + e30.Scale(u3));

				dgFloat64 u0 = dgFloat64 (1.0f) - u1 - u2 - u3;
				dgBigVector r1 (q0.Scale (u0) + q1.Scale (u1) + q2.Scale (u2) + q3.Scale (u3));
				dgWeights& weighSet = m_points.m_weights[i];

				weighSet.m_controlIndex[0] = i0;
				weighSet.m_weightBlends[0] = dgFloat32(u0);

				weighSet.m_controlIndex[1] = i1;
				weighSet.m_weightBlends[1] = dgFloat32(u1);

				weighSet.m_controlIndex[2] = i2;
				weighSet.m_weightBlends[2] = dgFloat32(u2);

				weighSet.m_controlIndex[3] = i3;
				weighSet.m_weightBlends[3] = dgFloat32(u3);

				weightFound = true;
				break;
			}
		}
		dgAssert (weightFound);
		if (!weightFound) {
			dgTrace (("%d %f %f %f\n", i, p.m_x, p.m_y, p.m_z));
		}

		overlapNodes.RemoveAll();
	}
*/
}
