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



// based of the paper Hierarchical Approximate Convex Decomposition by Khaled Mamou 
// with his permission to adapt his algorithm to be more efficient.
// also making some addition to his heuristic for better convex clusters selections
// for the details http://kmamou.blogspot.com/


#define DG_CONCAVITY_SCALE				dgFloat64 (100.0f)
#define DG_CONCAVITY_PERIMETER_HANDICAP	dgFloat64 (0.5f)

class dgHACDEdge
{
	public:
	dgHACDEdge ()
		:m_mark(0)
		,m_proxyListNode(NULL)
		,m_backFaceHandicap(dgFloat64 (1.0))
	{
	}
	~dgHACDEdge ()
	{
	}

	dgInt32 m_mark;
	void* m_proxyListNode;
	dgFloat64 m_backFaceHandicap;
};

class dgHACDClusterFace
{
	public:
	dgHACDClusterFace()
		:m_edge(NULL)
		,m_area(dgFloat64(0.0f))
	{
	}
	~dgHACDClusterFace()
	{
	}

	dgEdge* m_edge;
	dgFloat64 m_area;
	dgBigVector m_normal;
};


class dgHACDCluster: public dgList<dgHACDClusterFace>
{
	public:
	dgHACDCluster ()
		:dgList<dgHACDClusterFace>(NULL)
		,m_color(0)
		,m_hierachicalClusterIndex(0)
		,m_area(dgFloat64 (0.0f))
		,m_concavity(dgFloat64 (0.0f))
	{
	}

	bool IsCoplanar(const dgBigPlane& plane, const dgMeshEffect& mesh, dgFloat64 tolerance) const
	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& info = node->GetInfo();
			dgEdge* ptr = info.m_edge;
			do {
				const dgBigVector& p = points[ptr->m_incidentVertex];
				dgFloat64 dist = fabs(plane.Evalue(p));
				if (dist > tolerance) {
					return false;
				}
				ptr = ptr->m_next;
			} while (ptr != info.m_edge);
		}
		return true;
	}


	dgInt32 m_color;
	dgInt32 m_hierachicalClusterIndex;
	dgFloat64 m_area;
	dgFloat64 m_concavity;
};


class dgHACDClusterGraph: public dgGraph<dgHACDCluster, dgHACDEdge> 
{
	public:
	class dgHACDConveHull: public dgConvexHull3d
	{
		class dgConvexHullRayCastData
		{
			public:
			dgFloat64 m_normalProjection;
			dgConvexHull3DFace* m_face;
		};

		public: 
		dgHACDConveHull (const dgHACDConveHull& hull)
			:dgConvexHull3d(hull)
			,m_mark(1)
		{
		}

		dgHACDConveHull (dgMemoryAllocator* const allocator, const dgBigVector* const points, dgInt32 count)
			:dgConvexHull3d(allocator, &points[0].m_x, sizeof (dgBigVector),count, dgFloat64 (0.0f))
			,m_mark(1)
		{
		}


		dgFloat64 CalculateTriangleConcavity(const dgBigVector& normal, dgInt32 i0, dgInt32 i1, dgInt32 i2, const dgBigVector* const points)
		{
			dgUnsigned32 head = 1;
			dgUnsigned32 tail = 0;
			dgBigVector pool[1<<8][3];

			pool[0][0] = points[i0];
			pool[0][1] = points[i1];
			pool[0][2] = points[i2];

			const dgFloat64 rayLength = dgFloat64(4.0f) * GetDiagonal();
			const dgBigVector step(normal.Scale(rayLength));

			dgFloat64 concavity = dgFloat32(0.0f);
			dgFloat64 minArea = dgFloat32(0.125f);
			dgFloat64 minArea2 = minArea * minArea * 0.5f;

			dgInt32 maxCount = 4;
			dgUnsigned32 mask = (sizeof (pool) / (3 * sizeof (pool[0][0]))) - 1;

			dgConvexHull3DFace* firstGuess = NULL;
			while ((tail != head) && (maxCount >= 0)) {
				maxCount --;
				dgBigVector p0(pool[tail][0]);
				dgBigVector p1(pool[tail][1]);
				dgBigVector p2(pool[tail][2]);
				p0.m_w = dgFloat32 (0.0f);
				p1.m_w = dgFloat32 (0.0f);
				p2.m_w = dgFloat32 (0.0f);

				tail = (tail + 1) & mask;

				dgBigVector q1((p0 + p1 + p2).Scale(dgFloat64(1.0f / 3.0f)));
				dgBigVector q0(q1 + step);

				dgFloat64 param = RayCast(q0, q1, &firstGuess);
				if (param > dgFloat64(1.0f)) {
					param = dgFloat64(1.0f);
				}
				dgBigVector dq(step.Scale(dgFloat32(1.0f) - param));
				dgAssert(dq.m_w == dgFloat32(0.0f));
				dgFloat64 lenght2 = sqrt (dq.DotProduct(dq).GetScalar());
				if (lenght2 > concavity) {
					concavity = lenght2;
				}

				if (((head + 1) & mask) != tail) {
					dgBigVector edge10(p1 - p0);
					dgBigVector edge20(p2 - p0);
					dgBigVector n(edge10.CrossProduct(edge20));
					dgAssert(n.m_w == dgFloat32(0.0f));
					dgFloat64 area2 = n.DotProduct(n).GetScalar();
					if (area2 > minArea2) {
						dgBigVector p01((p0 + p1).Scale(dgFloat64(0.5f)));
						dgBigVector p12((p1 + p2).Scale(dgFloat64(0.5f)));
						dgBigVector p20((p2 + p0).Scale(dgFloat64(0.5f)));

						pool[head][0] = p0;
						pool[head][1] = p01;
						pool[head][2] = p20;
						head = (head + 1) & mask;

						if (((head + 1) & mask) != tail) {
							pool[head][0] = p1;
							pool[head][1] = p12;
							pool[head][2] = p01;
							head = (head + 1) & mask;

							if (((head + 1) & mask) != tail)	{
								pool[head][0] = p2;
								pool[head][1] = p20;
								pool[head][2] = p12;
								head = (head + 1) & mask;
							}
						}
					}
				}
			}
			return concavity;
		}



		dgFloat64 FaceRayCast (const dgConvexHull3DFace* const face, const dgBigVector& origin, const dgBigVector& dist, dgFloat64& normalProjection) const
		{
			dgInt32 i0 = face->m_index[0];
			dgInt32 i1 = face->m_index[1];
			dgInt32 i2 = face->m_index[2];

			const dgBigVector& p0 = m_points[i0];
			dgBigVector normal ((m_points[i1] - p0).CrossProduct(m_points[i2] - p0));

			//dgFloat64 N = (origin - p0) % normal;
			dgAssert(normal.m_w == dgFloat32(0.0f));
			dgFloat64 N = normal.DotProduct(origin - p0).GetScalar();
			dgFloat64 D = normal.DotProduct(dist).GetScalar();

			if (fabs(D) < dgFloat64 (1.0e-16f)) { // 
				normalProjection = dgFloat32 (0.0);
				if (N > dgFloat64 (0.0f)) {
					return dgFloat32 (-1.0e30);
				} else {

					return dgFloat32 (1.0e30);
				}
			}
			normalProjection = D;
			return - N / D;
		}

		dgConvexHull3DFace* ClosestFaceVertexToPoint (const dgBigVector& point)
		{
			// note, for this function to be effective point should be an already close point to the Hull.
			// for example casting the point to the OBB or the AABB of the full is a good first guess. 
			dgConvexHull3DFace* closestFace = &GetFirst()->GetInfo();	
			dgInt8 pool[256 * (sizeof (dgConvexHull3DFace*) + sizeof (dgFloat64))];
			dgUpHeap<dgConvexHull3DFace*,dgFloat64> heap (pool, sizeof (pool));

			for (dgInt32 i = 0; i < 3; i ++) {
				dgBigVector dist (m_points[closestFace->m_index[i]] - point);
				dgAssert(dist.m_w == dgFloat32(0.0f));
				heap.Push(closestFace, dist.DotProduct(dist).GetScalar());
			}

			m_mark ++;	
			dgFloat64 minDist = heap.Value();
			while (heap.GetCount()) {
				dgConvexHull3DFace* const face = heap[0];	
				if (heap.Value() < minDist) {
					minDist = heap.Value();
					closestFace = face;
				}
				heap.Pop();
				//face->m_mark = m_mark;
				face->SetMark(m_mark);
				for (dgInt32 i = 0; i < 3; i ++) {
					dgConvexHull3DFace* twin = &face->GetTwin(i)->GetInfo();	
					if (twin->GetMark() != m_mark) {
						dgBigVector dist (m_points[twin->m_index[i]] - point);
						// use hysteresis to prevent stops at a local minimal, but at the same time fast descend
						dgAssert(dist.m_w == dgFloat32(0.0f));
						dgFloat64 dist2 = dist.DotProduct(dist).GetScalar();
						if (dist2 < (minDist * dgFloat64 (1.001f))) {
							heap.Push(twin, dist2);
						}
					}
				}
			}

			return closestFace;
		}


		// this version have input sensitive complexity (approximately  log2)
		// when casting parallel rays and using the last face as initial guess this version has const time complexity 
		dgFloat64 RayCast (const dgBigVector& localP0, const dgBigVector& localP1, dgConvexHull3DFace** firstFaceGuess)
		{
			dgConvexHull3DFace* face = &GetFirst()->GetInfo();
			if (firstFaceGuess && *firstFaceGuess) {
				face = *firstFaceGuess;
			} else {
				if (GetCount() > 32) {
					dgVector q0 (localP0);
					dgVector q1 (localP1);
					if (dgRayBoxClip (q0, q1, m_aabbP0, m_aabbP1)) {
						face = ClosestFaceVertexToPoint (q0);
					}
				}
			}

			m_mark ++;	
			face->SetMark (m_mark);
			dgInt8 pool[256 * (sizeof (dgConvexHullRayCastData) + sizeof (dgFloat64))];
			dgDownHeap<dgConvexHullRayCastData,dgFloat64> heap (pool, sizeof (pool));

			dgFloat64 t0 = dgFloat64 (-1.0e20);			//for the maximum entering segment parameter;
			dgFloat64 t1 = dgFloat64 ( 1.0e20);			//for the minimum leaving segment parameter;
			dgBigVector dS (localP1 - localP0);		// is the segment direction vector;
			dgConvexHullRayCastData data;
			data.m_face = face;
			dgFloat64 t = FaceRayCast (face, localP0, dS, data.m_normalProjection);
			if (data.m_normalProjection >= dgFloat32 (0.0f)) {
				t = dgFloat64 (-1.0e30);
			}

			heap.Push (data, t);
			while (heap.GetCount()) {
				dgConvexHullRayCastData data1 (heap[0]);
				t = heap.Value();
				dgConvexHull3DFace* const face1 = data1.m_face;
				dgFloat64 normalDistProjection = data1.m_normalProjection;
				heap.Pop();
				bool foundThisBestFace = true;
				if (normalDistProjection < dgFloat64 (0.0f)) {
					if (t > t0) {
						t0 = t;
					}
					if (t0 > t1) {
						return dgFloat64 (1.2f);
					}
				} else {
					foundThisBestFace = false;
				}

				for (dgInt32 i = 0; i < 3; i ++) {
					dgConvexHull3DFace* const face2 = &face1->GetTwin(i)->GetInfo();

					if (face2->GetMark() != m_mark) {
						face2->SetMark (m_mark);
						dgConvexHullRayCastData data2;
						data2.m_face = face2;
						t = FaceRayCast (face2, localP0, dS, data2.m_normalProjection);
						if (data2.m_normalProjection >= dgFloat32 (0.0)) {
							t = dgFloat64 (-1.0e30);
						} else if (t > t0) {
							foundThisBestFace = false;
						} else if (fabs (t - t0) < dgFloat64 (1.0e-10f)) {
							return dgConvexHull3d::RayCast (localP0, localP1);
						}
						if ((heap.GetCount() + 2)>= heap.GetMaxCount()) {
							// remove t values that are old and way outside interval [0.0, 1.0]  
							for (dgInt32 j = heap.GetCount() - 1; j >= 0; j--) {
								dgFloat64 val = heap.Value(j);
								if ((val < dgFloat64 (-100.0f)) || (val > dgFloat64 (100.0f))) {
									heap.Remove(j);
								}
							}
						}
						heap.Push (data2, t);
					}
				}
				if (foundThisBestFace) {
					if ((t0 >= dgFloat64 (0.0f)) && (t0 <= dgFloat64 (1.0f))) {
						if (firstFaceGuess) {
							*firstFaceGuess = face1;
						}
						return t0;
					}
					break;
				}
			}
			return dgFloat64 (1.2f);
		}


		dgInt32 m_mark;
	};

	class dgHACDConvacityLookAheadTree
	{
		public:
		DG_CLASS_ALLOCATOR(allocator)

			dgHACDConvacityLookAheadTree (dgMemoryAllocator* const allocator, dgEdge* const face, dgFloat64 concavity)
			:m_concavity(concavity)	
			,m_faceList (allocator)
			,m_left (NULL)
			,m_right (NULL)
		{
			m_faceList.Append(face);
		}


		dgHACDConvacityLookAheadTree (dgMemoryAllocator* const allocator, dgHACDConvacityLookAheadTree* const leftChild, dgHACDConvacityLookAheadTree* const rightChild, dgFloat64 concavity)
			:m_concavity(concavity)	
			,m_faceList (allocator)
			,m_left (leftChild)
			,m_right (rightChild)
		{
			dgAssert (leftChild);
			dgAssert (rightChild);

			dgFloat64 concavityTest = m_concavity - dgFloat64 (1.0e-5f);
			if ((((m_left->m_faceList.GetCount() == 1) || (m_right->m_faceList.GetCount() == 1))) ||
				((concavityTest <= m_left->m_concavity) && (concavityTest <= m_right->m_concavity))) {
					//The the parent has lower concavity this mean that the two do no add more detail, 
					//the can be deleted and replaced the parent node
					// for example the two children can be two convex strips that are part of a larger convex piece
					// but each part has a non zero concavity, while the convex part has a lower concavity 
					m_faceList.Merge (m_left->m_faceList);
					m_faceList.Merge (m_right->m_faceList);

					delete m_left;
					delete m_right;
					m_left = NULL;
					m_right = NULL;
			} else {
				for (dgList<dgEdge*>::dgListNode* node = m_left->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
				for (dgList<dgEdge*>::dgListNode* node = m_right->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
			}
		}

		~dgHACDConvacityLookAheadTree ()
		{
			if (m_left) {
				dgAssert (m_right);
				delete m_left;
				delete m_right;
			}
		}

		dgInt32 GetNodesCount () const
		{
			dgInt32 count = 0;
			dgInt32 stack = 1;
			const dgHACDConvacityLookAheadTree* pool[1024];
			pool[0] = this;
			while (stack) {
				stack --;
				count ++;
				const dgHACDConvacityLookAheadTree* const root = pool[stack];
				if (root->m_left) {
					dgAssert (root->m_right);
					pool[stack] = root->m_left;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
					pool[stack] = root->m_right;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
			return count;
		}

		void ReduceByCount (dgInt32 count, dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64>& approximation)
		{
			if (count < 1) {
				count = 1;
			}

			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;
			approximation.Push(tmp, m_concavity);
			while ((approximation.GetCount() < count) && (approximation.Value() >= dgFloat32 (0.0f))) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				dgFloat64 concavity = approximation.Value();
				if (!worseCluster->m_left && (concavity >= dgFloat32 (0.0f))) {
					dgAssert (!worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster, concavity - dgFloat64 (1.0e10f));
				} else {
					dgAssert (worseCluster->m_left);
					dgAssert (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
				}
			}
		}


		void ReduceByConcavity (dgFloat64 concavity, dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64>& approximation)
		{
			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;

			approximation.Push(tmp, m_concavity);
			while (approximation.Value() > concavity) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				if (!worseCluster->m_left && approximation.Value() >= dgFloat32 (0.0f)) {
					approximation.Pop();
					approximation.Push(worseCluster, dgFloat32 (-1.0f));
				} else {
					dgAssert (worseCluster->m_left);
					dgAssert (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
				}
			}
		}

		dgFloat64 m_concavity; 
		dgList<dgEdge*> m_faceList;
		dgHACDConvacityLookAheadTree* m_left;
		dgHACDConvacityLookAheadTree* m_right;
	};

	class dgPairProxy
	{
		public:
		dgPairProxy()
			:m_nodeA(NULL)
			,m_nodeB(NULL)
			,m_hierachicalClusterIndexA(0)
			,m_hierachicalClusterIndexB(0)
			,m_area(dgFloat64(0.0f))
		{
		}

		~dgPairProxy()
		{
		}

		dgListNode* m_nodeA;
		dgListNode* m_nodeB;
		dgInt32 m_hierachicalClusterIndexA;
		dgInt32 m_hierachicalClusterIndexB;
		dgFloat64 m_area;
		dgFloat64 m_distanceConcavity;
	};


	class dgBackFaceFinder: public dgMeshEffect::dgMeshBVH
	{
		public:
		dgBackFaceFinder(dgMeshEffect* const mesh, dgHACDClusterGraph* const graph)
			:dgMeshEffect::dgMeshBVH(mesh)
			,m_clusterA(NULL)
			,m_graph(graph)
		{
			for (dgListNode* clusterNode = graph->GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {
				dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
				dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
				dgEdge* const edge = face.m_edge;
				AddFaceNode(edge, &cluster);
			}
		}

		//dgFloat64 RayFaceIntersect (const dgMeshBVHNode* const face, const dgBigVector& p0, const dgBigVector& p1, bool doublesided) const
		dgFloat64 RayFaceIntersect (const dgMeshBVHNode* const face, const dgBigVector& p0, const dgBigVector& p1, void* const userData) const
		{
			dgHACDCluster* const clusterFace = (dgHACDCluster*) face->m_userData;

			dgFloat64 param = dgFloat32 (100.0f);
			if (clusterFace->m_color != m_clusterA->m_color) {
				param = dgMeshEffect::dgMeshBVH::RayFaceIntersect (face, p1, p0, NULL);
				if ((param >= dgFloat32 (0.0f)) && (param <= dgFloat32(1.0f))) {
					param = dgFloat32 (1.0f) - param;
				}
			}
			return param;
		}

		void CastBackFace (dgListNode* const clusterNodeA, const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, dgFloat32 distanceThreshold)
		{
			dgBigVector origin ((p0 + p1 + p2).Scale (dgFloat32 (1.0f/3.0f)));

			dgFloat32 rayDistance = distanceThreshold * dgFloat32 (2.0f);


			m_clusterA = &clusterNodeA->GetInfo().m_nodeData;
			dgHACDClusterFace& faceA = m_clusterA->GetFirst()->GetInfo();
			dgBigVector end (origin - faceA.m_normal.Scale (rayDistance));

			dgAssert (0);
/*
			dgFloat64 paramOut;
			//dgMeshBVHNode* const node = FaceRayCast (origin, end, paramOut, false);

			dgMeshBVHNode* node;
			FaceRayCast (origin, end, paramOut, &node);

			if (node) {
				dgHACDCluster* const clusterB = (dgHACDCluster*) node->m_userData;
				dgAssert (clusterB->m_color != m_clusterA->m_color);
				dgFloat64 distance = rayDistance * paramOut;

				if (distance < distanceThreshold) {
					dgHACDClusterFace& faceB = clusterB->GetFirst()->GetInfo();
					dgEdge* const edgeB = faceB.m_edge;

					
					bool isAdjacent = false;
					dgEdge* ptrA = faceA.m_edge;
					do {
						dgEdge* ptrB = edgeB;
						do {
							if (ptrB->m_twin == ptrA) {
								ptrA = faceA.m_edge->m_prev;
								isAdjacent = true;
								break;
							}
							ptrB = ptrB->m_next;
						} while (ptrB != edgeB);

						ptrA = ptrA->m_next;
					} while (ptrA != faceA.m_edge);

					if (!isAdjacent) {
						isAdjacent = false;
						dgHACDClusterGraph::dgListNode* const clusterNodeB = m_graph->GetNodeFromNodeData (clusterB);
						for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNodeA->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
							if (edgeNode->GetInfo().m_node == clusterNodeB) {
								isAdjacent = true;
								break;
							}
						}

						if (!isAdjacent) {
							dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeAB = clusterNodeA->GetInfo().AddEdge (clusterNodeB);
							dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeBA = clusterNodeB->GetInfo().AddEdge (clusterNodeA);

							dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
							dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
							edgeAB.m_backFaceHandicap = DG_CONCAVITY_PERIMETER_HANDICAP;
							edgeBA.m_backFaceHandicap = DG_CONCAVITY_PERIMETER_HANDICAP;
						}
					}
				}
			}
*/
		}

		
		dgHACDCluster* m_clusterA;
		dgHACDClusterGraph* m_graph;
	};

    dgHACDClusterGraph(dgMeshEffect& mesh, dgFloat32 backFaceDistanceFactor, dgReportProgress reportProgressCallback, void* const reportProgressUserData)
		:dgGraph<dgHACDCluster, dgHACDEdge> (mesh.GetAllocator())
		,m_mark(0)
		,m_faceCount(0)
		,m_vertexMark(0)
		,m_progress(0)
		,m_concavityTreeIndex(0)
		,m_invFaceCount(dgFloat32 (1.0f))
		,m_diagonal(dgFloat64(1.0f))
		,m_vertexMarks(NULL)
		,m_vertexPool(NULL)
		,m_proxyList(mesh.GetAllocator())
		,m_concavityTreeArray(NULL)
		,m_convexProximation(mesh.GetAllocator())
		,m_priorityHeap (mesh.GetCount() * 2 + 2048, mesh.GetAllocator())
        ,m_reportProgressCallback(reportProgressCallback)
        ,m_reportProgressUserData(reportProgressUserData)
	{
		m_faceCount = mesh.GetTotalFaceCount();

		dgMemoryAllocator* const allocator = mesh.GetAllocator();
		m_invFaceCount = dgFloat32 (1.0f) / (m_faceCount);

		// init some auxiliary structures
		dgInt32 vertexCount = mesh.GetVertexCount();
		m_vertexMarks = (dgInt32*) dgMallocStack(vertexCount * sizeof(dgInt32));
		m_vertexPool = (dgBigVector*) dgMallocStack(vertexCount * sizeof(dgBigVector));
		memset(m_vertexMarks, 0, vertexCount * sizeof(dgInt32));

		m_concavityTreeIndex = m_faceCount + 1;
		m_concavityTreeArray = (dgHACDConvacityLookAheadTree**) dgMallocStack(2 * m_concavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));
		memset(m_concavityTreeArray, 0, 2 * m_concavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));

		// scan the mesh and and add a node for each face
		dgInt32 color = 1;
		dgMeshEffect::Iterator iter(mesh);

		dgInt32 meshMask = mesh.IncLRU();
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (iter.Begin(); iter; iter++) {
			dgEdge* const edge = &(*iter);
			if ((edge->m_mark != meshMask) && (edge->m_incidentFace > 0)) {

				dgListNode* const clusterNode = AddNode ();
				dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
				cluster.SetAllocator(mesh.GetAllocator());

				dgFloat64 perimeter = dgFloat64(0.0f);
				dgEdge* ptr = edge;
				do {
					dgBigVector p1p0(points[ptr->m_incidentVertex] - points[ptr->m_prev->m_incidentVertex]);
					dgAssert(p1p0.m_w == dgFloat32(0.0f));
					perimeter += sqrt(p1p0.DotProduct(p1p0).GetScalar());
					ptr->m_incidentFace = color;
					ptr->m_userData = dgUnsigned64 (clusterNode);
					ptr->m_mark = meshMask;
					ptr = ptr->m_next;
				} while (ptr != edge);

				dgBigVector normal (mesh.FaceNormal(edge, &points[0][0], sizeof(dgBigVector)));
				dgFloat64 mag = sqrt(normal.DotProduct(normal).GetScalar());

				cluster.m_color = color;
				cluster.m_hierachicalClusterIndex = color;
				cluster.m_area = dgFloat64(0.5f) * mag;
				cluster.m_concavity = CalculateConcavityMetric (dgFloat64 (0.0f), cluster.m_area, perimeter, 1, 0);

				dgHACDClusterFace& face = cluster.Append()->GetInfo();
				face.m_edge = edge;
				face.m_area = dgFloat64(0.5f) * mag;
				face.m_normal = normal.Scale(dgFloat64(1.0f) / mag);

				m_concavityTreeArray[color] = new (allocator) dgHACDConvacityLookAheadTree (allocator, edge, dgFloat64 (0.0f));

				color ++;
			}
		}

		// add all link adjacent faces links
		for (dgListNode* clusterNode = GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {

			dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
			dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
			dgEdge* const edge = face.m_edge;
			dgEdge* ptr = edge; 
			do {
				if (ptr->m_twin->m_incidentFace > 0) {
					dgAssert (ptr->m_twin->m_userData);
					dgListNode* const twinClusterNode = (dgListNode*) ptr->m_twin->m_userData;
					dgAssert (twinClusterNode);

					bool doubleEdge = false;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
						if (edgeNode->GetInfo().m_node == twinClusterNode) {
							doubleEdge = true;
							break;
						}
					}
					if (!doubleEdge) {
						clusterNode->GetInfo().AddEdge (twinClusterNode);
					}
				}
				ptr = ptr->m_next;
			} while (ptr != edge);
		}

		Trace();
		dgAssert (0);
/*
		// add links to back faces
		dgBigVector minAABB;
		dgBigVector maxAABB;
		mesh.CalculateAABB (minAABB, maxAABB);
		maxAABB -= minAABB;
		dgAssert(maxAABB.m_w == dgFloat32(0.0f));
		dgFloat32 rayDiagonalLength = dgFloat32 (sqrt (maxAABB.DotProduct(maxAABB).GetScalar()));
		m_diagonal = rayDiagonalLength;

		dgBackFaceFinder backFaces(&mesh, this);
		dgFloat32 distanceThreshold = rayDiagonalLength * backFaceDistanceFactor;
		dgAssert (distanceThreshold >= dgFloat32 (0.0f));
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {

			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
			dgEdge* const edgeA = faceA.m_edge;
			dgEdge* ptr = edgeA;

			dgBigVector p0 (points[ptr->m_incidentVertex]);
			dgBigVector p1 (points[ptr->m_next->m_incidentVertex]);
			ptr = ptr->m_next->m_next;
			do {
				dgBigVector p2 (points[ptr->m_incidentVertex]);
				dgBigVector p01 ((p0 + p1).Scale (dgFloat32 (0.5f)));
				dgBigVector p12 ((p1 + p2).Scale (dgFloat32 (0.5f)));
				dgBigVector p20 ((p2 + p0).Scale (dgFloat32 (0.5f)));

				backFaces.CastBackFace (clusterNodeA, p0, p01, p20, distanceThreshold);
				backFaces.CastBackFace (clusterNodeA, p1, p12, p01, distanceThreshold);
				backFaces.CastBackFace (clusterNodeA, p2, p20, p12, distanceThreshold);
				backFaces.CastBackFace (clusterNodeA, p01, p12, p20, distanceThreshold);

				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edgeA);
		}
*/
		Trace();
	}

	~dgHACDClusterGraph ()
	{
		for (dgInt32 i = 0; i < m_faceCount * 2; i ++) {
			if (m_concavityTreeArray[i]) {
				delete m_concavityTreeArray[i];
			}
		}

		dgFreeStack(m_concavityTreeArray);
		dgFreeStack(m_vertexPool);
		dgFreeStack(m_vertexMarks);
	}




	void Trace() const
	{
#if 0
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {
			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			//dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
			//dgEdge* const edgeA = faceA.m_edge;

			dgTrace (("cluster node: %d\n", clusterA.m_color));
			dgTrace (("            links: "));
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeA = clusterNodeA->GetInfo().GetFirst(); edgeNodeA; edgeNodeA = edgeNodeA->GetNext()) {
				dgListNode* const clusterNodeB = edgeNodeA->GetInfo().m_node;
				dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;
				dgTrace (("%d ", clusterB.m_color));
			}
			dgTrace (("\n"));
		}
		dgTrace (("\n"));
#endif
	}


	// you can insert callback here to print the progress as it collapse clusters
	bool ReportProgress ()
	{
		bool state = true;
		m_progress ++;
		if (m_reportProgressCallback) {
			dgFloat32 progress = dgFloat32(m_progress) * m_invFaceCount;
			state = m_reportProgressCallback (progress * dgFloat32 (0.5f) + 0.5f, m_reportProgressUserData);
		}
		return state;
	}

	dgMeshEffect* CreatePartitionMesh (dgMeshEffect& mesh, dgInt32 maxVertexPerHull)
	{
		dgMemoryAllocator* const allocator = mesh.GetAllocator();
		dgMeshEffect* const convexPartionMesh = new (allocator) dgMeshEffect(allocator);

		dgArray<dgBigVector> convexVertexBuffer(mesh.m_points.m_vertex, mesh.m_points.m_vertex.m_count);
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		dgInt32 layer = 0;
		convexPartionMesh->BeginBuild();
		for (dgList<dgHACDConvacityLookAheadTree*>::dgListNode* clusterNode = m_convexProximation.GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {
			dgHACDConvacityLookAheadTree* const cluster = clusterNode->GetInfo();

			dgInt32 vertexCount = 0;
			for (dgList<dgEdge*>::dgListNode* faceNode = cluster->m_faceList.GetFirst(); faceNode; faceNode = faceNode->GetNext()) {
				dgEdge* const edge = faceNode->GetInfo();
				dgEdge* ptr = edge;
				do {
					dgInt32 index = ptr->m_incidentVertex;
					convexVertexBuffer[vertexCount] = points[index];
					vertexCount++;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}

			//dgConvexHull3d convexHull(allocator, &convexVertexBuffer[0].m_x, sizeof(dgBigVector), vertexCount, 0.0, maxVertexPerHull);
			dgMeshEffect convexMesh(allocator, &convexVertexBuffer[0].m_x, vertexCount, sizeof(dgBigVector), dgFloat64(0.0f));
			if (convexMesh.GetCount()) {
				for (dgInt32 i = 0; i < convexMesh.m_points.m_vertex.m_count; i++) {
					convexMesh.m_points.m_layers[i] = layer;
				}
				convexPartionMesh->MergeFaces(&convexMesh);
				layer++;
			}
		}
		convexPartionMesh->EndBuild(1.0e-5f);

		m_progress = m_faceCount - 1;
		ReportProgress();
		return convexPartionMesh;
	}

	dgFloat64 ConcavityByFaceMedian (dgInt32 faceCountA, dgInt32 faceCountB) const
	{
		dgFloat64 faceCountCost = DG_CONCAVITY_SCALE * dgFloat64 (0.1f) * (faceCountA + faceCountB) * m_invFaceCount;
		return faceCountCost;
	}

	dgFloat64 CalculateConcavityMetric (dgFloat64 convexConcavity, dgFloat64 area, dgFloat64 perimeter, dgInt32 faceCountA, dgInt32 faceCountB) const 
	{
		dgFloat64 edgeCost = perimeter * perimeter / (dgFloat64(4.0f * dgPI) * area);
		return convexConcavity * DG_CONCAVITY_SCALE + edgeCost + ConcavityByFaceMedian (faceCountA, faceCountB);
	}

	void SubmitInitialEdgeCosts (dgMeshEffect& mesh) 
	{
		m_mark ++;
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {
			// call the progress callback
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgFloat64 weight = edgeAB.m_backFaceHandicap; 
				if (edgeAB.m_mark != m_mark) {
					edgeAB.m_mark = m_mark;
					dgListNode* const clusterNodeB = edgeNodeAB->GetInfo().m_node;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
						dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
						if (clusterNode == clusterNodeA) {
							dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
							edgeBA.m_mark = m_mark;
							dgAssert (!edgeAB.m_proxyListNode);
							dgAssert (!edgeBA.m_proxyListNode);

							dgAssert (edgeBA.m_backFaceHandicap == weight);
							dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB, weight * edgeBA.m_backFaceHandicap);
							edgeAB.m_proxyListNode = proxyNode;
							edgeBA.m_proxyListNode = proxyNode;
							break;
						}
					}
				}
			}
		}
	}

	dgInt32 CopyVertexToPool(const dgMeshEffect& mesh, const dgHACDCluster& cluster, dgInt32 start)
	{
		dgInt32 count = start;

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				dgInt32 index = edge->m_incidentVertex;
				if (m_vertexMarks[index] != m_vertexMark) {
					m_vertexMarks[index] = m_vertexMark;
					m_vertexPool[count] = points[index];
					count++;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
		return count;
	}


	void MarkInteriorClusterEdges (dgMeshEffect& mesh, dgInt32 mark, const dgHACDCluster& cluster, dgInt32 colorA, dgInt32 colorB) const
	{
		dgAssert (colorA != colorB);
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if ((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB)) {
					edge->m_mark = mark;
					edge->m_twin->m_mark = mark;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
	}

	dgFloat64 CalculateClusterPerimeter (dgMeshEffect& mesh, dgInt32 mark, const dgHACDCluster& cluster, dgInt32 colorA, dgInt32 colorB) const
	{
		dgAssert (colorA != colorB);
		dgFloat64 perimeter = dgFloat64 (0.0f);
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if (!((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB))) {
					dgBigVector p1p0(points[edge->m_twin->m_incidentVertex] - points[edge->m_incidentVertex]);
					dgAssert(p1p0.m_w == dgFloat32(0.0f));
					perimeter += sqrt(p1p0.DotProduct(p1p0).GetScalar());
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}

		return perimeter;
	}

	void HeapCollectGarbage () 
	{
		if ((m_priorityHeap.GetCount() + 20) > m_priorityHeap.GetMaxCount()) {
			for (dgInt32 i = m_priorityHeap.GetCount() - 1; i >= 0; i--) {
				dgList<dgPairProxy>::dgListNode* const emptyNode = m_priorityHeap[i];
				dgPairProxy& emptyPair = emptyNode->GetInfo();
				if ((emptyPair.m_nodeA == NULL) && (emptyPair.m_nodeB == NULL)) {
					m_priorityHeap.Remove(i);
				}
			}
		}
	}


	dgFloat64 CalculateConcavity(dgHACDConveHull& hull, const dgMeshEffect& mesh, const dgHACDCluster& cluster)
	{
		dgFloat64 concavity = dgFloat32(0.0f);

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			dgInt32 i0 = edge->m_incidentVertex;
			dgInt32 i1 = edge->m_next->m_incidentVertex;
			for (dgEdge* ptr = edge->m_next->m_next; ptr != edge; ptr = ptr->m_next) {
				dgInt32 i2 = ptr->m_incidentVertex;
				dgFloat64 val = hull.CalculateTriangleConcavity(clusterFace.m_normal, i0, i1, i2, points);
				if (val > concavity) {
					concavity = val;
				}
				i1 = i2;
			}
		}

		return concavity;
	}

	dgFloat64 CalculateConcavity (dgHACDConveHull& hull, dgMeshEffect& mesh, dgHACDCluster& clusterA, dgHACDCluster& clusterB)
	{
		return dgMax(CalculateConcavity(hull, mesh, clusterA), CalculateConcavity(hull, mesh, clusterB));
	}


	dgList<dgPairProxy>::dgListNode* SubmitEdgeCost (dgMeshEffect& mesh, dgListNode* const clusterNodeA, dgListNode* const clusterNodeB, dgFloat64 perimeterHandicap)
	{
		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		bool flatStrip = true;
		dgFloat64 tol = dgFloat64 (1.0e-5f) * m_diagonal;
		dgHACDClusterFace& clusterFaceA = clusterA.GetFirst()->GetInfo();
		dgAssert(clusterFaceA.m_normal.m_w == dgFloat32(0.0f));
		dgBigPlane plane(clusterFaceA.m_normal, - points[clusterFaceA.m_edge->m_incidentVertex].DotProduct(clusterFaceA.m_normal).GetScalar());

		if (clusterA.GetCount() > 1) {
			flatStrip = clusterA.IsCoplanar(plane, mesh, tol);
		}

		if (flatStrip) {
			flatStrip = clusterB.IsCoplanar(plane, mesh, tol);
		}

		dgList<dgPairProxy>::dgListNode* pairNode = NULL;
		if (!flatStrip) {
			m_vertexMark ++;
			dgInt32 vertexCount = CopyVertexToPool(mesh, clusterA, 0);
			vertexCount = CopyVertexToPool(mesh, clusterB, vertexCount);

			dgHACDConveHull convexHull(mesh.GetAllocator(), m_vertexPool, vertexCount);

			if (convexHull.GetVertexCount()) {
				dgInt32 mark = mesh.IncLRU();
				MarkInteriorClusterEdges (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color);
				MarkInteriorClusterEdges (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);

				dgFloat64 area = clusterA.m_area + clusterB.m_area;
				dgFloat64 perimeter = CalculateClusterPerimeter (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color) +
									  CalculateClusterPerimeter (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);
				dgFloat64 concavity = CalculateConcavity (convexHull, mesh, clusterA, clusterB);

				if (concavity < dgFloat64(1.0e-3f)) {
					concavity = dgFloat64(0.0f);
				}

				// see if the heap will overflow
				HeapCollectGarbage ();

				// add a new pair to the heap
				dgList<dgPairProxy>::dgListNode* pairNode1 = m_proxyList.Append();
				dgPairProxy& pair = pairNode1->GetInfo();
				pair.m_nodeA = clusterNodeA;
				pair.m_nodeB = clusterNodeB;
				pair.m_distanceConcavity = concavity;
				pair.m_hierachicalClusterIndexA = clusterA.m_hierachicalClusterIndex;
				pair.m_hierachicalClusterIndexB = clusterB.m_hierachicalClusterIndex;

				pair.m_area = area;
				dgFloat64 cost = CalculateConcavityMetric (concavity, area * perimeterHandicap, perimeter * perimeterHandicap, clusterA.GetCount(), clusterB.GetCount());
				m_priorityHeap.Push(pairNode1, cost);

				return pairNode1;
			}
		}
		return pairNode;
	}


	bool CollapseEdge (dgList<dgPairProxy>::dgListNode* const pairNode, dgMeshEffect& mesh, dgFloat64 concavity)
	{
		dgListNode* adjacentNodes[1024];
		dgPairProxy& pair = pairNode->GetInfo();

		dgMemoryAllocator* const allocator = mesh.GetAllocator();

		bool continueColapsing = true;
		dgAssert((pair.m_nodeA && pair.m_nodeB) || (!pair.m_nodeA && !pair.m_nodeB));
		if (pair.m_nodeA && pair.m_nodeB && continueColapsing) {
			// call the progress callback
			continueColapsing = ReportProgress();

			dgListNode* const clusterNodeA = pair.m_nodeA;
			dgListNode* const clusterNodeB = pair.m_nodeB;
			dgAssert (clusterNodeA != clusterNodeB);

			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;

			dgAssert (&clusterA != &clusterB);
			dgAssert(clusterA.m_color != clusterB.m_color);

			dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexA];
			dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexB];
			dgAssert (leftTree);
			dgAssert (rightTree);
			m_concavityTreeArray[pair.m_hierachicalClusterIndexA] = NULL;
			m_concavityTreeArray[pair.m_hierachicalClusterIndexB] = NULL;
			dgAssert (m_concavityTreeIndex < (2 * (m_faceCount + 1)));

			dgFloat64 treeConcavity = pair.m_distanceConcavity;
//			 dgAssert (treeConcavity < 0.1);
			m_concavityTreeArray[m_concavityTreeIndex] = new (allocator) dgHACDConvacityLookAheadTree (allocator, leftTree, rightTree, treeConcavity);
			clusterA.m_hierachicalClusterIndex = m_concavityTreeIndex;
			clusterB.m_hierachicalClusterIndex = m_concavityTreeIndex;
			m_concavityTreeIndex ++;

			// merge two clusters
			while (clusterB.GetCount()) {

				dgHACDCluster::dgListNode* const nodeB = clusterB.GetFirst();
				clusterB.Unlink(nodeB);
	
				// now color code all faces of the merged cluster
				dgHACDClusterFace& faceB = nodeB->GetInfo();
				dgEdge* ptr = faceB.m_edge;
				do {
					ptr->m_incidentFace = clusterA.m_color;
					ptr = ptr->m_next;
				} while (ptr != faceB.m_edge);
				clusterA.Append(nodeB);
			}
			clusterA.m_area = pair.m_area;
			clusterA.m_concavity = concavity;

			// invalidate all proxies that are still in the heap
			dgInt32 adjacentCount = 1;
			adjacentNodes[0] = clusterNodeA;
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeAB.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					dgAssert ((edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeA) || (edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeB));
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeAB.m_proxyListNode = NULL;
				}

				adjacentNodes[adjacentCount] = edgeNodeAB->GetInfo().m_node;
				adjacentCount ++;
				dgAssert (adjacentCount < sizeof (adjacentNodes)/ sizeof (adjacentNodes[0]));
			}

			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
				dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeBA.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeBA.m_proxyListNode = NULL;
				}

				bool alreadyLinked = false;
				dgListNode* const node = edgeNodeBA->GetInfo().m_node;
				for (dgInt32 i = 0; i < adjacentCount; i ++) {
					if (node == adjacentNodes[i]) {
						alreadyLinked = true;
						break;
					}
				}
				if (!alreadyLinked) {
					clusterNodeA->GetInfo().AddEdge (node);
					node->GetInfo().AddEdge (clusterNodeA);
				}
			}
			DeleteNode (clusterNodeB);

			// submit all new costs for each edge connecting this new node to any other node 
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgFloat64 weigh = edgeAB.m_backFaceHandicap;
				dgListNode* const clusterNodeB1 = edgeNodeAB->GetInfo().m_node;
				for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB1->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
					dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
					if (clusterNode == clusterNodeA) {
						dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
						dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB1, weigh * edgeBA.m_backFaceHandicap);
						if (proxyNode) {
							edgeBA.m_proxyListNode = proxyNode;
							edgeAB.m_proxyListNode = proxyNode;
						}
						break;
					}
				}
			}
		}
		m_proxyList.Remove(pairNode);

		return continueColapsing;
	}

	bool CollapseClusters (dgMeshEffect& mesh, dgFloat64 maxConcavity___, dgInt32 maxClustesCount)
	{
		bool collapseEdgeState = true;
		while (m_priorityHeap.GetCount() && collapseEdgeState) {
			dgFloat64 concavity =  m_priorityHeap.Value();
			dgList<dgPairProxy>::dgListNode* const pairNode = m_priorityHeap[0];
			m_priorityHeap.Pop();
			collapseEdgeState = CollapseEdge (pairNode, mesh, concavity);
		}

		if (collapseEdgeState) {
			dgInt32 treeCounts = 0;
			for (dgInt32 i = 0; i < m_concavityTreeIndex; i ++) {
				if (m_concavityTreeArray[i]) {
					m_concavityTreeArray[treeCounts] = m_concavityTreeArray[i];
					m_concavityTreeArray[i] = NULL;
					treeCounts ++;
				}
			}

			if (treeCounts > 1) {
				for (dgInt32 i = 0; i < treeCounts; i ++) {
					dgAssert (m_concavityTreeArray[i]);
					if (m_concavityTreeArray[i]->m_faceList.GetCount()==1) {
						delete m_concavityTreeArray[i];
						m_concavityTreeArray[i] = m_concavityTreeArray[treeCounts-1];
						m_concavityTreeArray[treeCounts-1]= NULL;
						treeCounts --;
						i--;
					}
				}


				dgFloat32 largeConcacvity = 10000;
				while (treeCounts > 1)	 {
					dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[treeCounts-1];
					dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[treeCounts-2];
					m_concavityTreeArray[treeCounts-1] = NULL;
					m_concavityTreeArray[treeCounts-2] = new (mesh.GetAllocator()) dgHACDConvacityLookAheadTree (mesh.GetAllocator(), leftTree, rightTree, largeConcacvity);
					largeConcacvity *= 2;
					treeCounts --;
				}

			}

			dgHACDConvacityLookAheadTree* const tree = m_concavityTreeArray[0];
			if (tree) {
				dgDownHeap<dgHACDConvacityLookAheadTree*, dgFloat64> approximation(maxClustesCount * 2, mesh.GetAllocator());

				tree->ReduceByCount (maxClustesCount, approximation);
				//tree->ReduceByConcavity (maxConcavity, approximation);

//while ((approximation.Value() + dgFloat64 (1.0e10f)) > 1.0e-5) {
//approximation.Pop();
//}

				while (approximation.GetCount()) {
					m_convexProximation.Append(approximation[0]);
					approximation.Pop();
				}
			}
		}
		return collapseEdgeState;
	}


	dgInt32 m_mark;
	dgInt32 m_faceCount;
	dgInt32 m_vertexMark;
	dgInt32 m_progress;
	dgInt32 m_concavityTreeIndex;
	dgFloat32 m_invFaceCount;
	dgFloat64 m_diagonal;
	dgInt32* m_vertexMarks;
	dgBigVector* m_vertexPool;
	dgList<dgPairProxy> m_proxyList;
	dgHACDConvacityLookAheadTree** m_concavityTreeArray;	
	dgList<dgHACDConvacityLookAheadTree*> m_convexProximation;
	dgUpHeap<dgList<dgPairProxy>::dgListNode*, dgFloat64> m_priorityHeap;
	dgReportProgress m_reportProgressCallback;
    void* m_reportProgressUserData;
};

dgMeshEffect* dgMeshEffect::CreateConvexApproximation(dgFloat32 maxConcavity, dgFloat32 backFaceDistanceFactor, dgInt32 maxHullsCount, dgInt32 maxVertexPerHull, dgReportProgress reportProgressCallback, void* const progressReportUserData) const
{
	//	dgMeshEffect triangleMesh(*this);
	if (maxHullsCount <= 1) {
		maxHullsCount = 1;
	}
	if (maxConcavity <= dgFloat32 (1.0e-5f)) {
		maxConcavity = dgFloat32 (1.0e-5f);
	}

	if (maxVertexPerHull < 4) {
		maxVertexPerHull = 4;
	}
	backFaceDistanceFactor = dgClamp(backFaceDistanceFactor, dgFloat32 (1.0e-6f), dgFloat32 (1.0f));

	dgMeshEffect* partition = NULL;

	// make a copy of the mesh
	dgMeshEffect mesh(*this);
	mesh.m_attrib.m_materialChannel.Clear();
	mesh.m_attrib.m_normalChannel.Clear();
	mesh.m_attrib.m_binormalChannel.Clear();
	mesh.m_attrib.m_colorChannel.Clear();
	mesh.m_attrib.m_uv0Channel.Clear();
	mesh.m_attrib.m_uv1Channel.Clear();
	mesh.Triangulate ();

	mesh.UnpackAttibuteData();
	mesh.PackAttibuteData();
	mesh.UnpackPoints();
	bool state = mesh.Optimize (&mesh.m_points.m_vertex[0].m_x, sizeof (dgBigVector), reportProgressCallback, progressReportUserData, dgFloat32 (1.0e-3f), 1500);
	// optimized override userdata
	dgPolyhedra::Iterator iter(mesh);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &iter.GetNode()->GetInfo();
		if (edge->m_incidentFace > 0) {
			edge->m_userData = edge->m_incidentVertex;
		}
	}
	mesh.PackPoints(dgFloat32 (1.0e-24f));
	if (state) {
		mesh.DeleteDegenerateFaces (&mesh.m_points.m_vertex[0].m_x, sizeof (dgBigVector), dgFloat32 (1.0e-12f));
		mesh.RepairTJoints();
		mesh.ConvertToPolygons();
		//mesh.SaveOFF ("xxxxxx.off");

		// create a general connectivity graph    
		dgHACDClusterGraph graph (mesh, backFaceDistanceFactor, reportProgressCallback, progressReportUserData);

		// calculate initial edge costs
		graph.SubmitInitialEdgeCosts (mesh);

		// collapse the graph
		if (graph.CollapseClusters (mesh, maxConcavity, maxHullsCount)) {
			// Create Partition Mesh
			partition = graph.CreatePartitionMesh (mesh, maxVertexPerHull);
		}
	}

	return partition;
}
