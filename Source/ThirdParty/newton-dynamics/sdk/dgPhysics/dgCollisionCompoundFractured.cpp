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
#include "dgWorld.h"
#include "dgMeshEffect.h"
#include "dgDynamicBody.h"
#include "dgCollisionInstance.h"
#include "dgCollisionConvexHull.h"
#include "dgCollisionCompoundFractured.h"


#define DG_FRACTURE_AABB_GUARD_DISTANCE 1.0f
#define DG_FRACTURE_MAX_METERIAL_COUNT	256

dgCollisionCompoundFractured::dgConectivityGraph::dgConectivityGraph (dgMemoryAllocator* const allocator)
	:dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>(allocator)
{
}

dgCollisionCompoundFractured::dgConectivityGraph::dgConectivityGraph (const dgConectivityGraph& source)
	:dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>(source.GetAllocator())
{
	dgTree<dgListNode*, dgListNode*> filter(GetAllocator());   

	for (dgConectivityGraph::dgListNode* node = source.GetFirst(); node != source.GetLast(); node = node->GetNext() ) {
		dgListNode* const newNode = dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode();

		dgDebriNodeInfo& srcData = node->GetInfo().m_nodeData;
		dgDebriNodeInfo& data = newNode->GetInfo().m_nodeData;
		data.m_mesh = srcData.m_mesh;
		data.m_mesh->AddRef();

		filter.Insert(newNode, node);
	}

	for (dgListNode* node = source.GetFirst(); node != source.GetLast(); node = node->GetNext()) {
		dgListNode* const myNode = filter.Find(node)->GetInfo();
		for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = node->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dgListNode* const otherNode = filter.Find(edgeNode->GetInfo().m_node)->GetInfo();
			dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* const myEdgeNode = myNode->GetInfo().AddEdge (otherNode);
            myEdgeNode->GetInfo().m_edgeData.m_normal = edgeNode->GetInfo().m_edgeData.m_normal; 
		}
	}
}


dgCollisionCompoundFractured::dgConectivityGraph::~dgConectivityGraph ()
{
}

void dgCollisionCompoundFractured::dgConectivityGraph::Serialize(dgSerialize callback, void* const userData) const
{
	dgInt32 count = GetCount();

	dgInt32 index = 0;
	dgTree<dgInt32, dgListNode*> enumMap (GetAllocator());
	callback (userData, &count, dgInt32 (sizeof (dgInt32)));
	for (dgConectivityGraph::dgListNode* node = GetFirst(); node != GetLast(); node = node->GetNext() ) {
		enumMap.Insert(index, node);
		index ++;

		dgDebriNodeInfo& data = node->GetInfo().m_nodeData;
		dgInt32 nodeIndex = data.m_shapeNode->GetKey();
		callback (userData, &nodeIndex, dgInt32 (sizeof (dgInt32)));
		data.m_mesh->Serialize(callback, userData);
	}

	dgListNode* const lastNode = GetLast();
	dgDebriNodeInfo& data = lastNode->GetInfo().m_nodeData;
	data.m_mesh->Serialize(callback, userData);

	for (dgListNode* node = GetFirst(); node != GetLast(); node = node->GetNext()) {
		dgInt32 count1 = node->GetInfo().GetCount();
		callback (userData, &count1, dgInt32 (sizeof (dgInt32)));
		for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = node->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dgListNode* const otherNode = edgeNode->GetInfo().m_node;
			dgInt32 index1 = enumMap.Find(otherNode)->GetInfo();
			dgVector normal (edgeNode->GetInfo().m_edgeData.m_normal); 
			callback (userData, &index1, dgInt32 (sizeof (dgInt32)));
			callback (userData, &normal, dgInt32 (sizeof (dgVector)));
		}
	}
}

void dgCollisionCompoundFractured::dgConectivityGraph::Deserialize (dgCollisionCompoundFractured* const source, dgDeserialize callback, void* const userData)
{
	dgInt32 count = 0;
	callback (userData, &count, dgInt32 (sizeof (dgInt32)));

	dgTree<dgListNode*, dgInt32> enumMap (GetAllocator());
	for (dgInt32 i = 0; i < count - 1; i ++) {

		dgListNode* const node = dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode();
		enumMap.Insert(node, i);

		dgDebriNodeInfo& data = node->GetInfo().m_nodeData;
		dgInt32 nodeIndex;
		callback (userData, &nodeIndex, dgInt32 (sizeof (dgInt32)));
		data.m_shapeNode = source->FindNodeByIndex (nodeIndex);
		dgAssert (data.m_shapeNode);
		data.m_mesh = new (GetAllocator()) dgMesh (GetAllocator(), callback, userData);
	}

	dgListNode* const nodePtr = dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode();
	dgDebriNodeInfo& data = nodePtr->GetInfo().m_nodeData;
	data.m_mesh = new (GetAllocator()) dgMesh (GetAllocator(), callback, userData);

	for (dgListNode* node = GetFirst(); node != GetLast(); node = node->GetNext()) {
		dgInt32 count1;
		callback (userData, &count1, dgInt32 (sizeof (dgInt32)));

		for (dgInt32 i = 0; i < count1; i ++) {
			dgInt32 index;
			dgVector normal;
			callback (userData, &index, dgInt32 (sizeof (dgInt32)));
			callback (userData, &normal, dgInt32 (sizeof (dgVector)));

			dgListNode* const otherNode = enumMap.Find(index)->GetInfo();
			dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* const edgeNode = node->GetInfo().AddEdge (otherNode);
			edgeNode->GetInfo().m_edgeData.m_normal = normal;
		}
	}
}



class dgCollisionCompoundFractured::dgFractureBuilder: public dgTree<dgMeshEffect*, dgInt32>
{
	public:
    class dgPerimenterEdge
    {
        public:
        dgPerimenterEdge* m_next;
        dgPerimenterEdge* m_prev;
        const dgBigVector* m_vertex;
    };

	class dgFractureConectivity: public dgGraph<dgInt32, dgInt32>
	{
		public:
		dgFractureConectivity(dgMemoryAllocator* const allocator)
			:dgGraph<int, int> (allocator)
		{
		}
	};

	class dgConvexSolidVertices: public dgList<dgInt32>  
	{
		public:
		dgConvexSolidVertices(dgMemoryAllocator* const allocator)
			:dgList<dgInt32> (allocator)
		{
		}
	};

	class dgConvexSolidArray: public dgTree<dgConvexSolidVertices, dgInt32>  
	{
		public:
		dgConvexSolidArray(dgMemoryAllocator* const allocator)
			:dgTree<dgConvexSolidVertices, dgInt32> (allocator) 
		{
		}
	};


	dgFractureBuilder (dgMemoryAllocator* const allocator, dgMeshEffect* const solidMesh, dgInt32 pointcloudCount, const dgFloat32* const vertexCloud, dgInt32 strideInBytes, int materialId, const dgMatrix& textureProjectionMatrix)
		:dgTree<dgMeshEffect*, dgInt32>(allocator)
		,m_conectivity(allocator)
	{
		dgAssert (0);
/*
		dgStack<dgBigVector> buffer(pointcloudCount + 16);
		dgBigVector* const pool = &buffer[0];
		dgFloat64 quantizeFactor = dgFloat64 (16.0f);
		dgFloat64 invQuantizeFactor = dgFloat64 (1.0f) / quantizeFactor;
		dgInt32 stride = strideInBytes / sizeof (dgFloat32); 

		dgBigVector minAABB;
		dgBigVector maxAABB;
		solidMesh->CalculateAABB (minAABB, maxAABB);
		for (dgInt32 i = 0; i < pointcloudCount; i ++) {
			dgFloat64 x = vertexCloud[i * stride + 0];
			dgFloat64 y	= vertexCloud[i * stride + 1];
			dgFloat64 z	= vertexCloud[i * stride + 2];
			x = floor (x * quantizeFactor) * invQuantizeFactor;
			y = floor (y * quantizeFactor) * invQuantizeFactor;
			z = floor (z * quantizeFactor) * invQuantizeFactor;
			pool[i] = dgBigVector (x, y, z, dgFloat64 (0.0f));
		}

		// add the bbox as a barrier
		int count = pointcloudCount;
		pool[count + 0] = dgBigVector ( minAABB.m_x, minAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 1] = dgBigVector ( maxAABB.m_x, minAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 2] = dgBigVector ( minAABB.m_x, maxAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 3] = dgBigVector ( maxAABB.m_x, maxAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 4] = dgBigVector ( minAABB.m_x, minAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 5] = dgBigVector ( maxAABB.m_x, minAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 6] = dgBigVector ( minAABB.m_x, maxAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 7] = dgBigVector ( maxAABB.m_x, maxAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		count += 8;

		dgStack<dgInt32> indexList(count);
		count = dgVertexListToIndexList(&pool[0].m_x, sizeof (dgBigVector), 3, count, &indexList[0], dgFloat64 (5.0e-2f));	
		dgAssert (count >= 8);

		dgFloat64 maxSize = dgMax(maxAABB.m_x - minAABB.m_x, maxAABB.m_y - minAABB.m_y, maxAABB.m_z - minAABB.m_z);
		minAABB -= dgBigVector (maxSize, maxSize, maxSize, dgFloat64 (0.0f));
		maxAABB += dgBigVector (maxSize, maxSize, maxSize, dgFloat64 (0.0f));

		// add the a guard zone, so that we do not have to clip
		dgInt32 guadVertexKey = count;
		pool[count + 0] = dgBigVector ( minAABB.m_x, minAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 1] = dgBigVector ( maxAABB.m_x, minAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 2] = dgBigVector ( minAABB.m_x, maxAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 3] = dgBigVector ( maxAABB.m_x, maxAABB.m_y, minAABB.m_z, dgFloat64 (0.0f));
		pool[count + 4] = dgBigVector ( minAABB.m_x, minAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 5] = dgBigVector ( maxAABB.m_x, minAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 6] = dgBigVector ( minAABB.m_x, maxAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		pool[count + 7] = dgBigVector ( maxAABB.m_x, maxAABB.m_y, maxAABB.m_z, dgFloat64 (0.0f));
		count += 8; 

		dgDelaunayTetrahedralization delaunayTetrahedras (allocator, &pool[0].m_x, count, sizeof (dgBigVector), dgFloat32 (0.0f));
		delaunayTetrahedras.RemoveUpperHull ();

		dgInt32 tetraCount = delaunayTetrahedras.GetCount();
		dgStack<dgBigVector> voronoiPoints(tetraCount + 32);
		dgStack<dgDelaunayTetrahedralization::dgListNode*> tetradrumNode(tetraCount);

		dgConvexSolidArray delanayNodes (allocator);	
		dgTree<dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*>, dgConvexSolidArray::dgTreeNode*> adjacentCell(allocator);

		dgInt32 index = 0;
		const dgHullVector* const delanayPoints = delaunayTetrahedras.GetHullVertexArray();
		for (dgDelaunayTetrahedralization::dgListNode* node = delaunayTetrahedras.GetFirst(); node; node = node->GetNext()) {
			dgConvexHull4dTetraherum& tetra = node->GetInfo();
			voronoiPoints[index] = tetra.CircumSphereCenter (delanayPoints);
			tetradrumNode[index] = node;

			dgConvexSolidArray::dgTreeNode* array[4];
			for (dgInt32 i = 0; i < 4; i ++) {
				dgConvexSolidArray::dgTreeNode* header = delanayNodes.Find(tetra.m_faces[0].m_index[i]);
				if (!header) {
					dgConvexSolidVertices list (allocator);
					header = delanayNodes.Insert(list, tetra.m_faces[0].m_index[i]);
				}
				header->GetInfo().Append (index);
				array[i] = header;
			}

			for (dgInt32 i = 0; i < 4; i ++) {
				dgTree<dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*>, dgConvexSolidArray::dgTreeNode*>::dgTreeNode* header = adjacentCell.Find (array[i]);
				if (!header) {
					dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*> list (allocator);
					header = adjacentCell.Insert(list, array[i]);
				}

				for (dgInt32 j = 0; j < 4; j ++) {
					if (i != j) {
						header->GetInfo().Insert(array[j], array[j]);
					}
				}
			}

			index ++;
		}

		dgConvexSolidArray::Iterator iter (delanayNodes);
		dgFloat32 normalAngleInRadians = 30.0f * dgDEG2RAD;

		dgTree<dgFractureConectivity::dgListNode*, dgConvexSolidArray::dgTreeNode*> graphMap(allocator);
		for (iter.Begin(); iter; iter ++) {

			dgConvexSolidArray::dgTreeNode* const nodeNode = iter.GetNode();
			const dgList<dgInt32>& list = nodeNode->GetInfo();
			dgInt32 key = nodeNode->GetKey();
			if (key < guadVertexKey) {
				dgBigVector pointArray[512];
				dgInt32 indexArray[512];

				dgInt32 count = 0;
				for (dgList<dgInt32>::dgListNode* ptr = list.GetFirst(); ptr; ptr = ptr->GetNext()) {
					dgInt32 i = ptr->GetInfo();
					pointArray[count] = voronoiPoints[i];
					count ++;
					dgAssert (count < dgInt32 (sizeof (pointArray) / sizeof (pointArray[0])));
				}

				count = dgVertexListToIndexList(&pointArray[0].m_x, sizeof (dgBigVector), 3, count, &indexArray[0], dgFloat64 (1.0e-3f));	
				if (count >= 4) {
					dgMeshEffect* const convexMesh = new (allocator) dgMeshEffect (allocator, &pointArray[0].m_x, count, sizeof (dgBigVector), dgFloat64 (0.0f));
					if (convexMesh->GetCount()) {
						convexMesh->AddRef();
						Insert (convexMesh, key);

						convexMesh->CalculateNormals(normalAngleInRadians);
						convexMesh->UniformBoxMapping (materialId, textureProjectionMatrix);

						dgInt32 vCount = convexMesh->GetVertexCount();
						for (dgInt32 i = 0; i < vCount; i ++) {
							dgBigVector& point = convexMesh->GetVertex (i);
							point.m_w = key;
						}
						dgInt32 aCount = convexMesh->GetPropertiesCount();
						for (dgInt32 i = 0; i < aCount; i ++) {
							dgMeshEffect::dgVertexAtribute& attib = convexMesh->GetAttribute (i);
							attib.m_vertex.m_w = key;
						}

						dgFractureConectivity::dgListNode* const node = m_conectivity.AddNode ();
						node->GetInfo().m_nodeData = key;
						graphMap.Insert(node, nodeNode);
					}
					convexMesh->Release();
				}
			}
		}


		dgTree<dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*>, dgConvexSolidArray::dgTreeNode*>::Iterator adjacentIter (adjacentCell);
		for (adjacentIter.Begin(); adjacentIter; adjacentIter++) {
			dgConvexSolidArray::dgTreeNode* const solidTree0 = adjacentIter.GetKey();
			
			dgTree<dgFractureConectivity::dgListNode*, dgConvexSolidArray::dgTreeNode*>::dgTreeNode* const mapNode0 = graphMap.Find(solidTree0);
			if (mapNode0) {
				dgFractureConectivity::dgListNode* const node0 = mapNode0->GetInfo();

				const dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*>& cell = adjacentIter.GetNode()->GetInfo();
				dgTree<dgConvexSolidArray::dgTreeNode*, dgConvexSolidArray::dgTreeNode*>::Iterator iter (cell);
				for (iter.Begin(); iter; iter ++) {
					dgConvexSolidArray::dgTreeNode* const solidTree1 = iter.GetNode()->GetInfo();
					dgTree<dgFractureConectivity::dgListNode*, dgConvexSolidArray::dgTreeNode*>::dgTreeNode* const mapNode1 = graphMap.Find(solidTree1);
					if (mapNode1) {
						dgFractureConectivity::dgListNode* const node1 = mapNode1->GetInfo();
						if (!IsPairConnected (node0, node1)) {
							node0->GetInfo().AddEdge(node1);
							node1->GetInfo().AddEdge(node0);
						}
					}
				}
			}
		}

		dgAssert (SanityCheck());
        ClipFractureParts (solidMesh);

		for (dgFractureConectivity::dgListNode* node = m_conectivity.GetFirst(); node; node = node->GetNext()) {
			dgInt32 index0 = node->GetInfo().m_nodeData;

			dgGraphNode<dgInt32, dgInt32>::dgListNode* nextEdge;
			for (dgGraphNode<dgInt32, dgInt32>::dgListNode* edgeNode = node->GetInfo().GetFirst(); edgeNode; edgeNode = nextEdge) {
				nextEdge = edgeNode->GetNext();
				dgFractureConectivity::dgListNode* const otherNode = edgeNode->GetInfo().m_node; 
				dgInt32 index1 = otherNode->GetInfo().m_nodeData;
				if (!AreSolidNeigborg (index0, index1)) {
					node->GetInfo().DeleteEdge(edgeNode);
				}
			}
		}

#if 0
for (dgFractureConectivity::dgListNode* node = m_conectivity.GetFirst(); node; node = node->GetNext()) {
	dgInt32 index = node->GetInfo().m_nodeData;
	dgTrace (("node %d: ", index));
	for (dgGraphNode<int, int>::dgListNode* edge = node->GetInfo().GetFirst(); edge; edge = edge->GetNext()) {
		dgFractureConectivity::dgListNode* const otherNode = edge->GetInfo().m_node;
		dgInt32 index1 = otherNode->GetInfo().m_nodeData;
		dgTrace (("%d ", index1));
	}
	dgTrace (("\n"));
}
#endif
*/
		dgAssert (SanityCheck());
	}

    ~dgFractureBuilder()
    {
        Iterator iter (*this);
        for (iter.Begin(); iter; iter ++) {
            dgMeshEffect* const mesh = iter.GetNode()->GetInfo();
            mesh->Release();
        }
    }


	bool IsPairConnected (dgFractureConectivity::dgListNode* const nodeA, dgFractureConectivity::dgListNode* const nodeB) const
	{
		for (dgGraphNode<int, int>::dgListNode* edgeNodeAB = nodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
			dgFractureConectivity::dgListNode* const otherNode = edgeNodeAB->GetInfo().m_node;
			if (otherNode == nodeB)
				return true;
		}
		return false;
	}

	bool ArePlaneCoplanar (dgMeshEffect* const meshA, void* faceA, const dgBigVector& planeA, dgMeshEffect* const meshB, void* faceB, const dgBigVector& planeB) const
	{
		if ((planeA.DotProduct(planeB & dgBigVector::m_triplexMask).GetScalar() < dgFloat64(-1.0 + 1.0e-6f)) && ((fabs(planeA.m_w + planeB.m_w) < dgFloat64(1.0e-6f)))) {
			const dgBigVector* const pointsA = (dgBigVector*)meshA->GetVertexPool();
			const dgBigVector* const pointsB = (dgBigVector*)meshB->GetVertexPool();

			dgInt32 indexA[128];
			dgInt32 indexB[128];

			dgInt32 indexCountA = meshA->GetFaceIndexCount(faceA);
			dgInt32 indexCountB = meshB->GetFaceIndexCount(faceB);
			dgAssert(indexCountA < sizeof (indexA) / sizeof(indexA[0]));
			dgAssert(indexCountB < sizeof (indexB) / sizeof(indexB[0]));

			meshA->GetFaceIndex(faceA, indexA);
			meshA->GetFaceIndex(faceB, indexB);

			//dgTrace (("faceA:\n"));
			dgPerimenterEdge subdivision[256];
			dgAssert((2 * (indexCountA + indexCountB)) < dgInt32(sizeof (subdivision) / sizeof (subdivision[0])));
			for (dgInt32 i = 1; i < indexCountB; i++) {
				subdivision[i].m_vertex = &pointsB[indexB[i]];
				subdivision[i].m_prev = &subdivision[i - 1];
				subdivision[i].m_next = &subdivision[i + 1];

				//dgTrace (("%f %f %f\n", pointsB[indexB[i]].m_x, pointsB[indexB[i]].m_y, pointsB[indexB[i]].m_z));
			}
			subdivision[0].m_vertex = &pointsB[indexB[0]];
			subdivision[0].m_next = &subdivision[1];
			subdivision[0].m_prev = &subdivision[indexCountB - 1];

			subdivision[indexCountB - 1].m_next = &subdivision[0];

			dgInt32 edgeIndex = indexCountB;

			dgBigVector outputPool[128];
			dgPerimenterEdge* edgeClipped[2];
			dgBigVector* output = &outputPool[0];
			edgeClipped[0] = NULL;
			edgeClipped[1] = NULL;

			//dgTrace (("faceB:\n"));
			dgPerimenterEdge* poly = &subdivision[0];
			dgInt32 i0 = indexCountA - 1;
			for (dgInt32 i1 = 0; i1 < indexCountA; i1++) {
				const dgBigVector& q0 = pointsA[indexA[i0]];
				const dgBigVector& q1 = pointsA[indexA[i1]];
				dgBigVector n(planeA.CrossProduct(q1 - q0));
				dgAssert(n.m_w == dgFloat32(0.0f));
				dgBigPlane plane(n, -n.DotProduct(q0).GetScalar());
				i0 = i1;
				//dgTrace (("%f %f %f\n", q0.m_x, q0.m_y, q0.m_z));

				dgInt32 count = 0;
				dgPerimenterEdge* tmp = poly;
				dgInt32 isInside = 0;
				dgFloat64 test0 = plane.Evalue(*tmp->m_vertex);
				do {
					dgFloat64 test1 = plane.Evalue(*tmp->m_next->m_vertex);
					if (test0 >= dgFloat32(0.0f)) {
						isInside |= 1;
						if (test1 < dgFloat32(0.0f)) {
							const dgBigVector& p0 = *tmp->m_vertex;
							const dgBigVector& p1 = *tmp->m_next->m_vertex;

							dgBigVector dp(p1 - p0);
							dgAssert(dp.m_w == dgFloat32(0.0f));
							dgFloat64 den = plane.DotProduct(dp).GetScalar();
							if (fabs(den) < dgFloat32(1.0e-24f)) {
								den = (den >= dgFloat32(0.0f)) ? dgFloat32(1.0e-24f) : dgFloat32(-1.0e-24f);
							}

							den = test0 / den;
							if (den >= dgFloat32(0.0f)) {
								den = dgFloat32(0.0f);
							} else if (den <= -1.0f) {
								den = dgFloat32(-1.0f);
							}
							output[0] = p0 - dp.Scale(den);
							edgeClipped[0] = tmp;
							count++;
						}
					} else if (test1 >= dgFloat32(0.0f)) {
						const dgBigVector& p0 = *tmp->m_vertex;
						const dgBigVector& p1 = *tmp->m_next->m_vertex;
						isInside |= 1;
						dgBigVector dp(p1 - p0);
						dgAssert(dp.m_w == dgFloat32(0.0f));
						dgFloat64 den = plane.DotProduct(dp).GetScalar();
						if (fabs(den) < dgFloat32(1.0e-24f)) {
							den = (den >= dgFloat32(0.0f)) ? dgFloat32(1.0e-24f) : dgFloat32(-1.0e-24f);
						}
						den = test0 / den;
						if (den >= dgFloat32(0.0f)) {
							den = dgFloat32(0.0f);
						} else if (den <= -1.0f) {
							den = dgFloat32(-1.0f);
						}
						output[1] = p0 - dp.Scale(den);
						edgeClipped[1] = tmp;
						count++;
					}
					test0 = test1;
					tmp = tmp->m_next;
				} while ((tmp != poly) && count < 2);

				if (!isInside) {
					return false;
				}

				if (count == 2) {
					dgPerimenterEdge* const newEdge = &subdivision[edgeIndex];
					newEdge->m_next = edgeClipped[1];
					newEdge->m_prev = edgeClipped[0];
					edgeClipped[0]->m_next = newEdge;
					edgeClipped[1]->m_prev = newEdge;

					newEdge->m_vertex = &output[0];
					edgeClipped[1]->m_vertex = &output[1];
					poly = newEdge;

					output += 2;
					edgeIndex++;
					dgAssert(edgeIndex < dgInt32(sizeof (subdivision) / sizeof (subdivision[0])));
				}
			}
			//dgTrace (("\n"));
			dgAssert(poly);
			dgBigVector area(dgFloat32(0.0f));
			dgBigVector r0(*poly->m_vertex);
			dgBigVector r1(*poly->m_next->m_vertex);
			dgBigVector r1r0(r1 - r0);
			dgPerimenterEdge* polyPtr = poly->m_next->m_next;
			do {
				dgBigVector r2(*polyPtr->m_vertex);
				dgBigVector r2r0(r2 - r0);
				area += r2r0.CrossProduct(r1r0);
				r1r0 = r2r0;
				polyPtr = polyPtr->m_next;
			} while (polyPtr != poly);
			dgAssert(area.m_w == dgFloat32(0.0f));
			return fabs(area.DotProduct(planeA).GetScalar()) > dgFloat32(1.0e-5f);
		}

		return false;
	}

    bool AreSolidNeigborg (int indexA, int indexB) const
    {
		dgMeshEffect* const meshA = Find(indexA)->GetInfo();
		dgMeshEffect* const meshB = Find(indexB)->GetInfo();

		const dgBigVector* const pointsA = (dgBigVector*)meshA->GetVertexPool();
		const dgBigVector* const pointsB = (dgBigVector*)meshB->GetVertexPool();

		dgBigVector planeB_array[512];

		dgInt32 planeB_Count = 0;
		for (void* faceB = meshB->GetFirstFace(); faceB; faceB = meshB->GetNextFace(faceB)) {
			if (!meshB->IsFaceOpen(faceB)) {
				dgInt32 vertexIndexB = meshB->GetVertexIndex(faceB);
				dgBigVector planeB(meshB->CalculateFaceNormal(faceB));
				planeB.m_w = -planeB.DotProduct(pointsB[vertexIndexB]).GetScalar();
				planeB_array[planeB_Count] = planeB;
				planeB_Count++;
				dgAssert(planeB_Count < sizeof (planeB_array) / sizeof (planeB_array[0]));
			}
		}

		for (void* faceA = meshA->GetFirstFace(); faceA; faceA = meshA->GetNextFace(faceA)) {
			if (!meshA->IsFaceOpen(faceA)) {
				dgInt32 vertexIndexA = meshA->GetVertexIndex(faceA);
				dgBigVector planeA(meshA->CalculateFaceNormal(faceA));
				planeA.m_w = -planeA.DotProduct(pointsA[vertexIndexA]).GetScalar();

				dgInt32 index = 0;
				for (void* faceB = meshB->GetFirstFace(); faceB; faceB = meshB->GetNextFace(faceB)) {
					if (!meshB->IsFaceOpen(faceB)) {
						if (ArePlaneCoplanar(meshA, faceA, planeA, meshB, faceB, planeB_array[index])) {
							return true;
						}
						index++;
					}
				}
			}
		}
		return false;
    }

    void ClipFractureParts (dgMeshEffect* const solidMesh)
    {
		dgFractureBuilder::dgFractureConectivity::dgListNode* nextNode;
        for (dgFractureConectivity::dgListNode* node = m_conectivity.GetFirst(); node; node = nextNode) {
			nextNode = node->GetNext();
            dgInt32 index = node->GetInfo().m_nodeData;
            dgTreeNode* const fractureNode = Find(index);
            dgAssert (fractureNode);
            dgMeshEffect* const voronoiConvex = fractureNode->GetInfo();
            dgMeshEffect* const fracturePiece = solidMesh->ConvexMeshIntersection (voronoiConvex);
            if (fracturePiece) {
                voronoiConvex->Release();    
                fractureNode->GetInfo() = fracturePiece;
            } else {
				m_conectivity.DeleteNode(node);
			}
        }
    }

	bool SanityCheck() const
	{
		for (dgFractureConectivity::dgListNode* rootNode = m_conectivity.GetFirst(); rootNode; rootNode = rootNode->GetNext() ) {
			dgTree<dgFractureConectivity::dgListNode*, dgFractureConectivity::dgListNode*> filter(GetAllocator());
			for (dgGraphNode<int, int>::dgListNode* edgeNode = rootNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
				dgFractureConectivity::dgListNode* node = edgeNode->GetInfo().m_node;
				dgAssert (!filter.Find(node));
				filter.Insert(node, node);
			}
		}
		return true;
	}

	dgFractureConectivity m_conectivity;
};



dgCollisionCompoundFractured::dgDebriNodeInfo::dgDebriNodeInfo ()
	:m_mesh(NULL)
	,m_shapeNode(NULL)
	,m_lru(0)
{
}

dgCollisionCompoundFractured::dgDebriNodeInfo::~dgDebriNodeInfo ()
{
	if (m_mesh) {
		m_mesh->Release();
	}
}


dgCollisionCompoundFractured::dgVertexBuffer::dgVertexBuffer(dgInt32 vertsCount, dgMemoryAllocator* allocator)
	:m_allocator(allocator)
	,m_vertexCount(vertsCount)
{
	m_uv = (dgFloat32 *) m_allocator->Malloc (2 * vertsCount * dgInt32 (sizeof (dgFloat32))); 
	m_vertex = (dgFloat32 *) m_allocator->Malloc (3 * vertsCount * dgInt32 (sizeof (dgFloat32))); 
	m_normal = (dgFloat32 *) m_allocator->Malloc (3 * vertsCount * dgInt32 (sizeof (dgFloat32))); 
}

dgCollisionCompoundFractured::dgVertexBuffer::~dgVertexBuffer ()
{
	m_allocator->Free (m_normal);
	m_allocator->Free (m_vertex);
	m_allocator->Free (m_uv);
}

dgCollisionCompoundFractured::dgVertexBuffer::dgVertexBuffer (dgMemoryAllocator* const allocator, dgDeserialize callback, void* const userData)
{
	m_allocator = allocator;
	callback (userData, &m_vertexCount, dgInt32 (sizeof (dgInt32)));

	m_uv = (dgFloat32 *) m_allocator->Malloc (2 * m_vertexCount * dgInt32 (sizeof (dgFloat32))); 
	m_vertex = (dgFloat32 *) m_allocator->Malloc (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))); 
	m_normal = (dgFloat32 *) m_allocator->Malloc (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))); 

	callback (userData, m_vertex, size_t (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
	callback (userData, m_normal, size_t (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
	callback (userData, m_uv, size_t (2 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
}

void dgCollisionCompoundFractured::dgVertexBuffer::Serialize(dgSerialize callback, void* const userData) const
{
	callback (userData, &m_vertexCount, dgInt32 (sizeof (dgInt32)));
	callback (userData, m_vertex, size_t (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
	callback (userData, m_normal, size_t (3 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
	callback (userData, m_uv, size_t (2 * m_vertexCount * dgInt32 (sizeof (dgFloat32))));
}


dgCollisionCompoundFractured::dgSubMesh::dgSubMesh (dgMemoryAllocator* const allocator)
	:m_indexes(NULL)
	,m_allocator(allocator)
	,m_material(0)
	,m_faceCount(0)
	,m_materialOrdinal(0)
	,m_visibleFaces(true)
{
}

dgCollisionCompoundFractured::dgSubMesh::~dgSubMesh ()
{
	if (m_indexes) {
		m_allocator->Free (m_indexes);
	}
}

void dgCollisionCompoundFractured::dgSubMesh::Serialize(dgSerialize callback, void* const userData) const
{
	dgInt32 visible = m_visibleFaces ? 1 : 0;
	callback (userData, &m_material, dgInt32 (sizeof (dgInt32)));
	callback (userData, &m_materialOrdinal, dgInt32 (sizeof (dgInt32)));
	callback (userData, &m_faceCount, dgInt32 (sizeof (dgInt32)));
	callback (userData, &visible, dgInt32 (sizeof (dgInt32)));
	callback (userData, m_indexes, size_t (3 * m_faceCount * dgInt32 (sizeof (dgInt32))));
}

dgCollisionCompoundFractured::dgSharedNodeMesh::dgSharedNodeMesh ()
{
}

dgCollisionCompoundFractured::dgSharedNodeMesh::~dgSharedNodeMesh ()
{
}


dgCollisionCompoundFractured::dgMesh::dgMesh(dgMemoryAllocator* const allocator)
	:dgList<dgSubMesh>(allocator)
	,m_vertexOffsetStart(0)
	,m_vertexCount(0)
	,m_isVisible(false)
{
}

dgCollisionCompoundFractured::dgMesh::~dgMesh()
{
}

dgCollisionCompoundFractured::dgMesh::dgMesh (dgMemoryAllocator* const allocator, dgDeserialize callback, void* const userData)
	:dgList<dgSubMesh>(allocator), dgRefCounter ()
{
	dgInt32 count;
	dgInt32 isVisible;
	callback (userData, &m_vertexOffsetStart, dgInt32 (sizeof (m_vertexOffsetStart)));
	callback (userData, &m_vertexCount, dgInt32 (sizeof (m_vertexCount)));
	callback (userData, &isVisible, dgInt32 (sizeof (m_vertexCount)));
	callback (userData, &count, dgInt32 (sizeof (dgInt32)));
	m_isVisible = isVisible ? true : false;

	for (dgInt32 i = 0; i < count; i ++) {
		dgInt32 visible;
		dgInt32 material;
		dgInt32 faceCount;
		dgInt32 materialOrdinal;

		callback (userData, &material, dgInt32 (sizeof (dgInt32)));
		callback (userData, &materialOrdinal, dgInt32 (sizeof (dgInt32)));
		callback (userData, &faceCount, dgInt32 (sizeof (dgInt32)));
		callback (userData, &visible, dgInt32 (sizeof (dgInt32)));

		dgSubMesh* const subMesh = AddgSubMesh (faceCount * 3, material);
		subMesh->m_faceCount = faceCount;
		subMesh->m_material = material;
		subMesh->m_materialOrdinal = materialOrdinal;
		subMesh->m_visibleFaces = visible ? true : false;
		callback (userData, subMesh->m_indexes, size_t (3 * faceCount * dgInt32 (sizeof (dgInt32))));
	}
}

void dgCollisionCompoundFractured::dgMesh::Serialize(dgSerialize callback, void* const userData) const
{
	dgInt32 count = GetCount();
	dgInt32 visible = m_isVisible ? 1 : 0;

	callback (userData, &m_vertexOffsetStart, dgInt32 (sizeof (m_vertexOffsetStart)));
	callback (userData, &m_vertexCount, dgInt32 (sizeof (m_vertexCount)));
	callback (userData, &visible, dgInt32 (sizeof (m_vertexCount)));
	callback (userData, &count, dgInt32 (sizeof (dgInt32)));
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		dgSubMesh& subMesh = node->GetInfo();
		subMesh.Serialize(callback, userData);
	}
}

dgCollisionCompoundFractured::dgSubMesh* dgCollisionCompoundFractured::dgMesh::AddgSubMesh(dgInt32 indexCount, dgInt32 material)
{
	dgSubMesh tmp (GetAllocator());
	dgSubMesh& subMesh = Append(tmp)->GetInfo();

	subMesh.m_visibleFaces = true;

	subMesh.m_material = material;
	subMesh.m_faceCount = indexCount / 3;

	subMesh.m_indexes = (dgInt32 *) subMesh.m_allocator->Malloc (indexCount * dgInt32 (sizeof (dgInt32))); 
	return &subMesh;
}


 dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* dgCollisionCompoundFractured::dgConectivityGraph::AddNode (dgFlatVertexArray& vertexArray, dgMeshEffect* const factureVisualMesh, dgTreeArray::dgTreeNode* const collisionNode, dgInt32 interiorMaterialBase)
{
	dgListNode* const node = dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode ();
	dgDebriNodeInfo& data = node->GetInfo().m_nodeData;

	data.m_mesh = new (GetAllocator()) dgMesh(GetAllocator());
	data.m_shapeNode = collisionNode;

	dgInt32 vertexCount = factureVisualMesh->GetPropertiesCount();

	dgStack<dgVector>vertex (vertexCount);
	dgStack<dgVector>normal (vertexCount);
	dgStack<dgVector>uv0 (vertexCount);
	dgStack<dgVector>uv1 (vertexCount);
	dgAssert (0);
//	factureVisualMesh->GetVertexStreams (sizeof (dgVector), &vertex[0].m_x, sizeof (dgVector), &normal[0].m_x, sizeof (dgVector), &uv0[0].m_x, sizeof (dgVector), &uv1[0].m_x);
		
	// extract the materials index array for mesh
	dgInt32 baseVertexCount = vertexArray.m_count;
	data.m_mesh->m_vertexOffsetStart = baseVertexCount;

	vertexArray[baseVertexCount + vertexCount].m_point[0] = 0.0f;
	for (dgInt32 i = 0; i < vertexCount; i ++) {
		dgInt32 j = baseVertexCount + i;
		vertexArray[j].m_point[0] = vertex[i].m_x;
		vertexArray[j].m_point[1] = vertex[i].m_y;
		vertexArray[j].m_point[2] = vertex[i].m_z;
		vertexArray[j].m_point[3] = normal[i].m_x;
		vertexArray[j].m_point[4] = normal[i].m_y;
		vertexArray[j].m_point[5] = normal[i].m_z;
		vertexArray[j].m_point[6] = uv0[i].m_x;
		vertexArray[j].m_point[7] = uv0[i].m_y;
		vertexArray[j].m_point[8] = uv1[i].m_x;
		vertexArray[j].m_point[9] = uv1[i].m_y;
	}


	dgStack<dgInt32> indexBuffer (vertexCount);
	data.m_mesh->m_vertexCount = dgVertexListToIndexList (&vertexArray[baseVertexCount].m_point[0], sizeof (dgFlatVertex), sizeof (dgFlatVertex), 0, vertexCount, &indexBuffer[0], dgFloat32 (1.0e-6f));
	vertexArray.m_count += data.m_mesh->m_vertexCount;

	dgMeshEffect::dgIndexArray* const geometryHandle = factureVisualMesh->MaterialGeometryBegin();
	for (dgInt32 handle = factureVisualMesh->GetFirstMaterial (geometryHandle); handle != -1; handle = factureVisualMesh->GetNextMaterial (geometryHandle, handle)) {
		dgInt32 material = factureVisualMesh->GetMaterialID (geometryHandle, handle);

		bool isVisible = (material < interiorMaterialBase) ? true : false;
		data.m_mesh->m_isVisible |= isVisible;

		dgInt32 indexCount = factureVisualMesh->GetMaterialIndexCount (geometryHandle, handle);
		dgSubMesh* const segment = data.m_mesh->AddgSubMesh(indexCount, isVisible ? material : material - interiorMaterialBase);

		segment->m_visibleFaces = isVisible;

		factureVisualMesh->GetMaterialGetIndexStream (geometryHandle, handle, segment->m_indexes);

		for (dgInt32 i = 0; i < indexCount; i++) {
			dgInt32 j = segment->m_indexes[i];
			segment->m_indexes[i] = indexBuffer[j];
		}
	}
	factureVisualMesh->MaterialGeomteryEnd(geometryHandle);
	return node;
}
 

dgCollisionCompoundFractured::dgCollisionCompoundFractured (const dgCollisionCompoundFractured& source, const dgCollisionInstance* const myInstance)
    :dgCollisionCompound(source, myInstance)
	,m_conectivity(source.m_conectivity)
	,m_conectivityMap (source.m_conectivityMap)
	,m_vertexBuffer(source.m_vertexBuffer)
	,m_impulseStrengthPerUnitMass(source.m_impulseStrengthPerUnitMass)
	,m_impulseAbsortionFactor(source.m_impulseAbsortionFactor)
	,m_density(dgFloat32 (-1.0f))
	,m_lru(0)
	,m_materialCount(source.m_materialCount)
	,m_emitFracturedChunk(source.m_emitFracturedChunk)
	,m_emitFracturedCompound(source.m_emitFracturedCompound)
	,m_reconstructMainMesh(source.m_reconstructMainMesh)
{
	m_rtti |= dgCollisionCompoundBreakable_RTTI;

	m_vertexBuffer->AddRef();

	dgTree<dgInt32, dgTreeArray::dgTreeNode*> nodeMap(m_allocator);
	dgTreeArray::Iterator iter (source.m_array);
	for (iter.Begin(); iter; iter ++) {
		nodeMap.Insert(iter.GetKey(), iter.GetNode());
	}

	dgConectivityGraph::dgListNode* myNode = m_conectivity.GetFirst();
	for (dgConectivityGraph::dgListNode* node = source.m_conectivity.GetFirst(); node != source.m_conectivity.GetLast(); myNode = myNode->GetNext(), node = node->GetNext() ) {
		dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
		dgTreeArray::dgTreeNode* const nodeBase = nodeInfo.m_shapeNode;

		dgAssert (nodeMap.Find(nodeBase));
		dgInt32 index = nodeMap.Find(nodeBase)->GetInfo();

		dgAssert (m_array.Find(index));
		dgDebriNodeInfo& myNodeInfo = myNode->GetInfo().m_nodeData;
		myNodeInfo.m_shapeNode = m_array.Find(index);
	}

	dgMesh* const mainMesh = new (m_world->GetAllocator()) dgMesh(m_world->GetAllocator());
	dgConectivityGraph::dgListNode* const mainNode = m_conectivity.dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode ();
	dgDebriNodeInfo& mainNodeData = mainNode->GetInfo().m_nodeData;
	mainNodeData.m_mesh = mainMesh;

	BuildMainMeshSubMehes();
	m_conectivityMap.Pupolate(m_conectivity);

	m_density = -dgFloat32 (1.0f) / GetVolume();
	dgAssert (SanityCheck());
}

dgCollisionCompoundFractured::dgCollisionCompoundFractured (dgCollisionCompoundFractured& source, const dgList<dgConectivityGraph::dgListNode*>& island)
	:dgCollisionCompound(source.m_world, NULL)
	,m_conectivity(source.GetAllocator())
	,m_conectivityMap (source.GetAllocator())
	,m_vertexBuffer(source.m_vertexBuffer)
	,m_impulseStrengthPerUnitMass(source.m_impulseStrengthPerUnitMass)
	,m_impulseAbsortionFactor(source.m_impulseAbsortionFactor)
	,m_density(source.m_density)
	,m_lru(0)
	,m_materialCount(source.m_materialCount)
	,m_emitFracturedChunk(source.m_emitFracturedChunk)
	,m_emitFracturedCompound(source.m_emitFracturedCompound)
	,m_reconstructMainMesh(source.m_reconstructMainMesh)
{
	m_collisionId = m_compoundFracturedCollision;
	m_rtti |= dgCollisionCompoundBreakable_RTTI;

	m_vertexBuffer->AddRef();

	dgCollisionCompound::BeginAddRemove ();
	for (dgList<dgConectivityGraph::dgListNode*>::dgListNode* node = island.GetFirst(); node; node = node->GetNext()) {
		dgConectivityGraph::dgListNode* const chunkNode = node->GetInfo();
		dgDebriNodeInfo& nodeInfo = chunkNode->GetInfo().m_nodeData;

		dgCollisionInstance* const chunkCollision = nodeInfo.m_shapeNode->GetInfo()->GetShape();
		dgTreeArray::dgTreeNode* const treeNode = dgCollisionCompound::AddCollision (chunkCollision); 

		source.m_conectivityMap.Remove (chunkCollision);
		source.dgCollisionCompound::RemoveCollision (nodeInfo.m_shapeNode);
		source.m_conectivity.Unlink(chunkNode);

		m_conectivity.Append(chunkNode);
		nodeInfo.m_shapeNode = treeNode;
	}
	dgCollisionCompound::EndAddRemove(false);

	dgMesh* const mainMesh = new (m_world->GetAllocator()) dgMesh(m_world->GetAllocator());
	dgConectivityGraph::dgListNode* const mainNode = m_conectivity.dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode ();
	dgDebriNodeInfo& mainNodeData = mainNode->GetInfo().m_nodeData;
	mainNodeData.m_mesh = mainMesh;

	BuildMainMeshSubMehes();
	m_conectivityMap.Pupolate(m_conectivity);

	m_density = -dgFloat32 (1.0f) / GetVolume();
	dgAssert (SanityCheck());
}

dgCollisionCompoundFractured::dgCollisionCompoundFractured (dgWorld* const world, dgDeserialize deserialization, void* const userData, const dgCollisionInstance* const myInstance, dgInt32 revisionNumber)
	:dgCollisionCompound (world, deserialization, userData, myInstance, revisionNumber)
	,m_conectivity (world->GetAllocator())
	,m_conectivityMap (world->GetAllocator())
{
	m_conectivity.Deserialize(this, deserialization, userData);
	m_vertexBuffer = new (m_world->GetAllocator()) dgVertexBuffer(m_world->GetAllocator(), deserialization, userData);

	deserialization (userData, &m_impulseStrengthPerUnitMass, sizeof (m_impulseStrengthPerUnitMass));
	deserialization (userData, &m_impulseAbsortionFactor, sizeof (m_impulseAbsortionFactor));
	deserialization (userData, &m_density, sizeof (m_density));
	deserialization (userData, &m_materialCount, sizeof (m_materialCount));
}


dgCollisionCompoundFractured::dgCollisionCompoundFractured (
	dgWorld* const world, dgMeshEffect* const solidMesh, int fracturePhysicsMaterialID, int pointcloudCount, const dgFloat32* const vertexCloud, int strideInBytes, int materialID, const dgMatrix& offsetMatrix, 
	OnEmitFractureChunkCallBack emitFracturedChunk, OnEmitNewCompundFractureCallBack emitNewCompoundFactured, OnReconstructFractureMainMeshCallBack reconstructMainMesh)
	:dgCollisionCompound (world)
	,m_conectivity(world->GetAllocator())
	,m_conectivityMap (world->GetAllocator())
	,m_vertexBuffer(NULL)
	,m_impulseStrengthPerUnitMass(10.0f)
	,m_impulseAbsortionFactor(0.5f)
	,m_density(dgFloat32 (-1.0f))
	,m_lru(0)
	,m_materialCount(0)
	,m_emitFracturedChunk(emitFracturedChunk) 
	,m_emitFracturedCompound(emitNewCompoundFactured)
	,m_reconstructMainMesh(reconstructMainMesh)
{
	m_collisionId = m_compoundFracturedCollision;
	m_rtti |= dgCollisionCompoundBreakable_RTTI;

	dgInt32 interiorMaterialBase = 1000;
	dgFractureBuilder fractureBuilder (GetAllocator(), solidMesh, pointcloudCount, vertexCloud, strideInBytes, materialID + interiorMaterialBase, offsetMatrix);

	dgFlatVertexArray vertexArray(m_world->GetAllocator());
	dgTree<dgConectivityGraph::dgListNode*, dgInt32> conectinyMap(GetAllocator());
	
    dgCollisionCompound::BeginAddRemove ();
    for (dgFractureBuilder::dgFractureConectivity::dgListNode* node = fractureBuilder.m_conectivity.GetFirst(); node; node = node->GetNext()) {
        dgInt32 index = node->GetInfo().m_nodeData;
        dgFractureBuilder::dgTreeNode* const fractureNode = fractureBuilder.Find(index);
        dgAssert (fractureNode);
        dgMeshEffect* const fractureSubMesh = fractureNode->GetInfo();
        dgCollisionInstance* const collisionInstance = fractureSubMesh->CreateConvexCollision(world, dgFloat32 (1.0e-5f), fracturePhysicsMaterialID);
		dgTreeArray::dgTreeNode* const shapeNode = AddCollision (collisionInstance);
		dgConectivityGraph::dgListNode* const conectivityNode = m_conectivity.AddNode(vertexArray, fractureSubMesh, shapeNode, interiorMaterialBase);
		conectinyMap.Insert(conectivityNode, index);
		collisionInstance->Release();
	}
    dgCollisionCompound::EndAddRemove ();

	for (dgFractureBuilder::dgFractureConectivity::dgListNode* node = fractureBuilder.m_conectivity.GetFirst(); node; node = node->GetNext()) {
		dgInt32 index0 = node->GetInfo().m_nodeData;
		dgConectivityGraph::dgListNode* const conectivityNode0 = conectinyMap.Find(index0)->GetInfo();
		for (dgGraphNode<int, int>::dgListNode* edge = node->GetInfo().GetFirst(); edge; edge = edge->GetNext()) {
			dgFractureBuilder::dgFractureConectivity::dgListNode* const otherNode = edge->GetInfo().m_node;
			dgInt32 index1 = otherNode->GetInfo().m_nodeData;
			dgConectivityGraph::dgListNode* const conectivityNode1 = conectinyMap.Find(index1)->GetInfo();
			dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* const edgeNode = conectivityNode0->GetInfo().AddEdge(conectivityNode1);

            dgDebriNodeInfo& nodeInfo0 = conectivityNode0->GetInfo().m_nodeData;
            dgDebriNodeInfo& nodeInfo1 = conectivityNode1->GetInfo().m_nodeData;
            dgCollisionInstance* const chunkCollision0 = nodeInfo0.m_shapeNode->GetInfo()->GetShape();
            dgCollisionInstance* const chunkCollision1 = nodeInfo1.m_shapeNode->GetInfo()->GetShape();

            dgMatrix inertia0 (chunkCollision0->CalculateInertia());
            dgMatrix inertia1 (chunkCollision1->CalculateInertia());
            dgVector normal (inertia1.m_posit - inertia0.m_posit);
			normal = normal.Normalize();
            edgeNode->GetInfo().m_edgeData.m_normal = normal; 
		}
	}

	m_vertexBuffer = new (m_world->GetAllocator()) dgVertexBuffer(vertexArray.m_count, m_world->GetAllocator());
	for (dgInt32 i = 0; i < vertexArray.m_count; i ++) {
		m_vertexBuffer->m_vertex[i * 3 + 0] = vertexArray[i].m_point[0];
		m_vertexBuffer->m_vertex[i * 3 + 1] = vertexArray[i].m_point[1];
		m_vertexBuffer->m_vertex[i * 3 + 2] = vertexArray[i].m_point[2];
		m_vertexBuffer->m_normal[i * 3 + 0] = vertexArray[i].m_point[3];
		m_vertexBuffer->m_normal[i * 3 + 1] = vertexArray[i].m_point[4];
		m_vertexBuffer->m_normal[i * 3 + 2] = vertexArray[i].m_point[5];
		m_vertexBuffer->m_uv[i * 2 + 0] = vertexArray[i].m_point[6];
		m_vertexBuffer->m_uv[i * 2 + 1] = vertexArray[i].m_point[7];
	}

	dgTree<dgInt32, dgInt32> materailOrdinalMap(GetAllocator());
	for (dgConectivityGraph::dgListNode* node = m_conectivity.GetFirst(); node; node = node->GetNext()) {
		dgDebriNodeInfo& data = node->GetInfo().m_nodeData;
		for (dgMesh::dgListNode* meshSegment = data.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
			dgSubMesh* const subMesh = &meshSegment->GetInfo();
			dgTree<dgInt32, dgInt32>::dgTreeNode* node1 = materailOrdinalMap.Find(subMesh->m_material);
			if (!node1) {
				node1 = materailOrdinalMap.Insert(m_materialCount, subMesh->m_material);
				m_materialCount ++;
				dgAssert (m_materialCount < DG_FRACTURE_MAX_METERIAL_COUNT);
			}

			dgInt32 materialIndex = node1->GetInfo();
			subMesh->m_materialOrdinal = materialIndex;
		}
	}

	dgMesh* const mainMesh = new (m_world->GetAllocator()) dgMesh(m_world->GetAllocator());
	dgConectivityGraph::dgListNode* const mainNode = m_conectivity.dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>::AddNode ();
	dgDebriNodeInfo& mainNodeData = mainNode->GetInfo().m_nodeData;
	mainNodeData.m_mesh = mainMesh;

	BuildMainMeshSubMehes();
	m_conectivityMap.Pupolate(m_conectivity);
	m_density = -dgFloat32 (1.0f) / GetVolume();

	dgAssert (SanityCheck());
}

dgCollisionCompoundFractured::~dgCollisionCompoundFractured(void)
{
    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
    }
}

void dgCollisionCompoundFractured::Serialize(dgSerialize callback, void* const userData) const
{
	dgCollisionCompound::Serialize(callback, userData);
	m_conectivity.Serialize(callback, userData);
	m_vertexBuffer->Serialize(callback, userData);

	callback (userData, &m_impulseStrengthPerUnitMass, sizeof (m_impulseStrengthPerUnitMass));
	callback (userData, &m_impulseAbsortionFactor, sizeof (m_impulseAbsortionFactor));
	callback (userData, &m_density, sizeof (m_density));
	callback (userData, &m_materialCount, sizeof (m_materialCount));
}

void dgCollisionCompoundFractured::SetCallbacks(OnEmitFractureChunkCallBack emitFracturedChunk, OnEmitNewCompundFractureCallBack emitNewCompoundFactured, OnReconstructFractureMainMeshCallBack reconstructMainMesh)
{
	m_emitFracturedChunk = emitFracturedChunk; 
	m_reconstructMainMesh = reconstructMainMesh;
	m_emitFracturedCompound = emitNewCompoundFactured;
}

void dgCollisionCompoundFractured::BuildMainMeshSubMehes() const
{
	dgConectivityGraph::dgListNode* const mainNode = m_conectivity.GetLast();
	dgMesh* const mainMesh = mainNode->GetInfo().m_nodeData.m_mesh;

	mainMesh->RemoveAll();

	dgAssert (mainMesh->m_vertexOffsetStart == 0);
	mainMesh->m_vertexCount = m_vertexBuffer->m_vertexCount;

	dgInt32 histogram[DG_FRACTURE_MAX_METERIAL_COUNT];
	dgInt32 materials[DG_FRACTURE_MAX_METERIAL_COUNT];
	memset (histogram, 0, m_materialCount * sizeof (dgInt32));

	for (dgConectivityGraph::dgListNode* node = m_conectivity.GetFirst(); node != mainNode; node = node->GetNext() ) {
		dgDebriNodeInfo& data = node->GetInfo().m_nodeData;
		for (dgMesh::dgListNode* meshSegment = data.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
			dgSubMesh* const subMesh = &meshSegment->GetInfo();
			if (subMesh->m_visibleFaces) {
				dgInt32 index = subMesh->m_materialOrdinal;
				histogram[index] += subMesh->m_faceCount;
				materials[index] = subMesh->m_material;
			}
		}
	}

	dgSubMesh* mainSubMeshes[DG_FRACTURE_MAX_METERIAL_COUNT];
	for (dgInt32 i = 0; i < m_materialCount; i ++) {
		if (histogram[i]) {
			mainSubMeshes[i] = mainMesh->AddgSubMesh (histogram[i] * 3, materials[i]);
		}
	}

	dgInt32 faceIndexIndexOffset[DG_FRACTURE_MAX_METERIAL_COUNT];
	memset (faceIndexIndexOffset, 0, m_materialCount * sizeof (dgInt32));

	for (dgConectivityGraph::dgListNode* node = m_conectivity.GetFirst(); node != mainNode; node = node->GetNext() ) {
		dgDebriNodeInfo& data = node->GetInfo().m_nodeData;
		const dgInt32 vertexOffsetStart = data.m_mesh->m_vertexOffsetStart;
		for (dgMesh::dgListNode* meshSegment = data.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
			dgSubMesh* const subMesh = &meshSegment->GetInfo();
			if (subMesh->m_visibleFaces) {
				dgInt32 index = subMesh->m_materialOrdinal;
				dgInt32 base = faceIndexIndexOffset[index];
				dgSubMesh* const mainSubMesh = mainSubMeshes[index];
				const dgInt32 count = 3 * subMesh->m_faceCount;
				for (dgInt32 i = 0; i < count; i ++) {
					mainSubMesh->m_indexes[base + i] = subMesh->m_indexes[i] + vertexOffsetStart;
				}
				faceIndexIndexOffset[index] += count;
				dgAssert (faceIndexIndexOffset[index] <= histogram[index] * 3);
			}
		}
	}
}

bool dgCollisionCompoundFractured::SanityCheck() const
{
	for (dgConectivityGraph::dgListNode* rootNode = m_conectivity.GetFirst(); rootNode; rootNode = rootNode->GetNext() ) {
		dgTree<dgConectivityGraph::dgListNode*, dgConectivityGraph::dgListNode*> filter(GetAllocator());
		for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = rootNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dgConectivityGraph::dgListNode* const node = edgeNode->GetInfo().m_node;
			dgAssert (!filter.Find(node));
			filter.Insert(node, node);
		}
	}

	return true;
}


dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* dgCollisionCompoundFractured::GetMainMesh() const 
{
	return m_conectivity.GetLast();
}

dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* dgCollisionCompoundFractured::GetFirstMesh() const 
{
	return m_conectivity.GetFirst();
}

dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* dgCollisionCompoundFractured::GetNextMesh (dgConectivityGraph::dgListNode* const mesNodeh) const
{
	return (mesNodeh->GetNext() != m_conectivity.GetLast()) ? mesNodeh->GetNext() : NULL;
}

dgInt32 dgCollisionCompoundFractured::GetVertecCount(dgConectivityGraph::dgListNode* const node) const 
{
	dgMesh* const mesh = node->GetInfo().m_nodeData.m_mesh;
	return mesh->m_vertexCount;
}

const dgFloat32* dgCollisionCompoundFractured::GetVertexPositions (dgConectivityGraph::dgListNode* const node) const 
{
	dgMesh* const mesh = node->GetInfo().m_nodeData.m_mesh;
	return m_vertexBuffer->GetVertexPositions() + mesh->m_vertexOffsetStart * 3;
}


const dgFloat32* dgCollisionCompoundFractured::GetVertexNormal (dgConectivityGraph::dgListNode* const node) const 
{
	dgMesh* const mesh = node->GetInfo().m_nodeData.m_mesh;
	return m_vertexBuffer->GetVertexNormals() + mesh->m_vertexOffsetStart * 3;
}


const dgFloat32* dgCollisionCompoundFractured::GetVertexUVs (dgConectivityGraph::dgListNode* const node) const 
{
	dgMesh* const mesh = node->GetInfo().m_nodeData.m_mesh;
	return m_vertexBuffer->GetVertexUVs() + mesh->m_vertexOffsetStart * 2;
}

dgInt32 dgCollisionCompoundFractured::GetSegmentIndexStream (dgConectivityGraph::dgListNode* const node___, dgMesh::dgListNode* const subMeshNode, dgInt32* const index) const
{
	dgSubMesh* const subMesh = &subMeshNode->GetInfo();
	const dgInt32* const indexes = subMesh->m_indexes;
	memcpy (index, indexes, 3 * subMesh->m_faceCount * sizeof(dgInt32));
	return subMesh->m_faceCount * 3;
}


void dgCollisionCompoundFractured::SetImpulseStrength(dgFloat32 impulseStrength)
{
	m_impulseStrengthPerUnitMass = dgMax (impulseStrength, dgFloat32 (0.0f), impulseStrength) ;
}

dgFloat32 dgCollisionCompoundFractured::GetImpulseStrength() const
{
	return m_impulseStrengthPerUnitMass;
}

void dgCollisionCompoundFractured::SetImpulsePropgationFactor(dgFloat32 factor)
{
	m_impulseAbsortionFactor = dgClamp(factor, dgFloat32 (0.0f), dgFloat32 (1.0f));
}

dgFloat32 dgCollisionCompoundFractured::GetSetImpulsePropgationFactor() const
{
	return m_impulseAbsortionFactor;
}



dgVector dgCollisionCompoundFractured::GetObbSize() const
{
	return dgCollisionCompound::GetObbSize() + (dgVector (DG_FRACTURE_AABB_GUARD_DISTANCE) & dgVector::m_triplexMask);
}


void dgCollisionCompoundFractured::CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const
{
	dgCollisionCompound::CalcAABB (matrix, p0, p1);
	p0 -= (dgVector (DG_FRACTURE_AABB_GUARD_DISTANCE) & dgVector::m_triplexMask);
	p1 += (dgVector (DG_FRACTURE_AABB_GUARD_DISTANCE) & dgVector::m_triplexMask);
}


dgInt32 dgCollisionCompoundFractured::CalculateContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const
{
	dgAssert (0);
	return 0;
/*
	dgBroadPhase* const broaphaPhase = m_world->GetBroadPhase();
	dgInt32 count = dgCollisionCompound::CalculateContacts (pair, proxy);
	
	if (!count && broaphaPhase->m_recursiveChunks && m_emitFracturedChunk && m_root) {
		dgContact* const constraint = pair->m_contact;
		dgBody* const myBody = constraint->GetBody0();
		dgBody* const otherBody = constraint->GetBody1();
		dgVector relVeloc (otherBody->GetVelocity() - myBody->GetVelocity());
		dgAssert (relVeloc.m_w == dgFloat32 (0.0f));
		dgVector impulseStimate (relVeloc.Scale (dgFloat32 (1.0f) / (myBody->GetInvMass().m_w + otherBody->GetInvMass().m_w))); 
		dgFloat32 impulseStimate2 = impulseStimate.DotProduct(impulseStimate).m_w;
		dgFloat32 impulseStrength = m_impulseStrengthPerUnitMass * myBody->GetMass().m_w;

		if (impulseStimate2 > (impulseStrength * impulseStrength)) {
			dgCollisionInstance* const myInstance = myBody->GetCollision();
			dgCollisionInstance* const otherInstance = otherBody->GetCollision();
			dgAssert (myInstance->GetChildShape() == this);
			dgContactPoint contactOut;
			dgAssert (0);
//			dgFloat32 dist = ConvexRayCast (otherInstance, otherInstance->GetGlobalMatrix(), relVeloc, proxy.m_timestep, contactOut, myBody, myInstance, NULL, proxy.m_threadIndex);
			dgFloat32 dist = 0;
			if (dist < proxy.m_timestep) {
				dgAssert (m_conectivityMap.Find(contactOut.m_collision0));
				dgConectivityGraph::dgListNode* const rootNode = m_conectivityMap.Find(contactOut.m_collision0)->GetInfo();
				dgDebriNodeInfo& nodeInfo = rootNode->GetInfo().m_nodeData;
				nodeInfo.m_lru = 1;
				dgCollisionCompoundFractured* const me = (dgCollisionCompoundFractured*)this;

				m_world->GlobalLock(true);
                if (me->SpawnChunks (myBody, myInstance, rootNode, impulseStimate2, impulseStrength * impulseStrength)) {
					me->SpawnDisjointChunks (myBody, myInstance, rootNode, impulseStimate2, impulseStrength * impulseStrength);

					BuildMainMeshSubMehes();
					m_reconstructMainMesh (myBody, m_conectivity.GetLast(), myInstance);
					if (m_root) {
						me->MassProperties ();
						dgFloat32 mass = m_centerOfMass.m_w * m_density;
						myBody->SetMassProperties(mass, myInstance);
					} else {
						myBody->SetMassProperties(dgFloat32 (0.0f), myInstance);
					}
				}
				m_world->GlobalUnlock();
			}
		}
	}
	return count;
*/
}

void dgCollisionCompoundFractured::BeginAddRemove ()
{
	dgCollisionCompound::BeginAddRemove ();
}

void dgCollisionCompoundFractured::EndAddRemove (bool flushCache)
{
	dgCollisionCompound::EndAddRemove (flushCache);
	BuildMainMeshSubMehes();
}

void dgCollisionCompoundFractured::RemoveCollision (dgTreeArray::dgTreeNode* const node)
{
	dgConectivityGraphMap::dgTreeNode* const mapNode = m_conectivityMap.Find(node->GetInfo()->GetShape());
	dgAssert (mapNode);

	dgConectivityGraph::dgListNode* const chunkNode = mapNode->GetInfo();

	for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = chunkNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
		dgConectivityGraph::dgListNode* const node1 = edgeNode->GetInfo().m_node;
		dgDebriNodeInfo& childNodeInfo = node1->GetInfo().m_nodeData;
		childNodeInfo.m_mesh->m_isVisible = true;
		for (dgMesh::dgListNode* meshSegment = childNodeInfo.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
			dgSubMesh* const subMesh = &meshSegment->GetInfo();
			subMesh->m_visibleFaces = true;
		}
	}

	dgDebriNodeInfo& nodeInfo = chunkNode->GetInfo().m_nodeData;
	dgCollisionInstance* const chunkCollision = nodeInfo.m_shapeNode->GetInfo()->GetShape();

	m_conectivityMap.Remove (chunkCollision);
	m_conectivity.DeleteNode(chunkNode);
	dgCollisionCompound::RemoveCollision (node);
}

bool dgCollisionCompoundFractured::IsNodeSaseToDetach(dgTreeArray::dgTreeNode* const node) const
{
	dgConectivityGraphMap::dgTreeNode* const mapNode = m_conectivityMap.Find(node->GetInfo()->GetShape());
	dgAssert (mapNode);
	return mapNode ? CanChunk (mapNode->GetInfo()) : false;
}

int dgCollisionCompoundFractured::GetFirstNiegborghArray (dgTreeArray::dgTreeNode* const node, dgTreeArray::dgTreeNode** const nodesArray, int maxCount) const
{
	int count = 0;
	dgConectivityGraphMap::dgTreeNode* const mapNode = m_conectivityMap.Find(node->GetInfo()->GetShape());
	dgAssert (mapNode);

	dgConectivityGraph::dgListNode* const chunkNode = mapNode->GetInfo();
	for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = chunkNode->GetInfo().GetFirst(); edgeNode && (count < maxCount); edgeNode = edgeNode->GetNext()) {
		dgConectivityGraph::dgListNode* const node1 = edgeNode->GetInfo().m_node;
		dgDebriNodeInfo& childNodeInfo = node1->GetInfo().m_nodeData;
		nodesArray[count] = childNodeInfo.m_shapeNode;
		count ++;
	}

	return count;
}

bool dgCollisionCompoundFractured::CanChunk (dgConectivityGraph::dgListNode* const chunkNode) const
{
	dgVector directionsMap[32];
	dgInt32 count = 0;
	for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = chunkNode->GetInfo().GetFirst(); edgeNode && (count < dgInt32 (sizeof (directionsMap)/sizeof (directionsMap[0]))); edgeNode = edgeNode->GetNext()) {
		directionsMap[count] = edgeNode->GetInfo().m_edgeData.m_normal;
		count ++;
	}
	
	dgVector error (dgFloat32 (1.0e-3f));
	dgVector himespherePlane (directionsMap[0]);
	for (dgInt32 i = 1; i < count; i ++) {
		dgVector projection (himespherePlane.DotProduct(directionsMap[i]));
		dgInt32 sign = (projection < error).GetSignMask();
		if (sign & 0x0f) {
//			dgFloat32 val;
//			projection.StoreScalar(&val);
			dgFloat32 val = projection.GetScalar();
			dgAssert (val > dgFloat32 (-1.0f));
			dgFloat32 angle = dgAcos (val) - dgFloat32 (90.0f * dgDEG2RAD) + dgFloat32 (15.0f * dgDEG2RAD);
			dgVector axis (himespherePlane.CrossProduct(directionsMap[i]));
			axis = axis.Normalize();
			dgQuaternion rot (axis, angle);
			himespherePlane = dgMatrix (rot, dgVector::m_wOne).RotateVector(himespherePlane);

			for (dgInt32 j = 0; j < i; j ++) {
				dgVector projection1 (himespherePlane.DotProduct(directionsMap[j]));
				dgInt32 sign1 = (projection1 < error).GetSignMask();
				if (sign1 & 0x0f) {
					return false;
				}
			}
		}
	}
	return true;
}




bool dgCollisionCompoundFractured::IsBelowPlane (dgConectivityGraph::dgListNode* const node, const dgVector& plane) const
{
	dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
	dgCollisionInstance* const instance = nodeInfo.m_shapeNode->GetInfo()->GetShape();

	dgVector dir (plane & dgVector::m_triplexMask);

	const dgMatrix& matrix = instance->GetLocalMatrix(); 
	dgVector support (matrix.TransformVector(instance->SupportVertex(matrix.UnrotateVector(dir))));

	dgFloat32 dist = (support.DotProduct(plane)).GetScalar();
	return dist < dgFloat32 (0.0f);
}

bool dgCollisionCompoundFractured::IsAbovePlane (dgConectivityGraph::dgListNode* const node, const dgVector& plane) const
{
	dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
	dgCollisionInstance* const instance = nodeInfo.m_shapeNode->GetInfo()->GetShape();

	dgVector dir (dgVector::m_negOne * (plane & dgVector::m_triplexMask));

	const dgMatrix& matrix = instance->GetLocalMatrix(); 
	dgVector support (matrix.TransformVector(instance->SupportVertex(matrix.UnrotateVector(dir))));

	dgFloat32 dist = (support.DotProduct(plane)).GetScalar();
	return dist > dgFloat32 (0.0f);
}

dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* dgCollisionCompoundFractured::FirstAcrossPlane (dgConectivityGraph::dgListNode* const nodeBelowPlane, const dgVector& plane) const
{
	dgDebriNodeInfo& nodeInfo = nodeBelowPlane->GetInfo().m_nodeData;
	dgCollisionInstance* const instance = nodeInfo.m_shapeNode->GetInfo()->GetShape();

	dgVector dir (plane & dgVector::m_triplexMask);

	const dgMatrix& matrix = instance->GetLocalMatrix(); 
	dgVector support (matrix.TransformVector(instance->SupportVertex(matrix.UnrotateVector(dir))));

	dgFloat32 dist = (support.DotProduct(plane)).GetScalar();
	dgAssert (dist < dgFloat32 (0.0f));

	dgConectivityGraph::dgListNode* startNode = nodeBelowPlane;
	for (bool foundBetterNode = true; foundBetterNode; ) {
		foundBetterNode = false;
		for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = startNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dgConectivityGraph::dgListNode* const node = edgeNode->GetInfo().m_node;
			dgDebriNodeInfo& neighborgInfo = node->GetInfo().m_nodeData;
			dgCollisionInstance* const instance1 = neighborgInfo.m_shapeNode->GetInfo()->GetShape();

			const dgMatrix& matrix1 = instance1->GetLocalMatrix(); 
			dgVector support1 (matrix1.TransformVector(instance1->SupportVertex(matrix1.UnrotateVector(dir))));
			dgFloat32 dist1 = (support1.DotProduct(plane)).GetScalar();
			if (dist1 > dist) {
				dist = dist1;
				foundBetterNode = true;
				startNode = node;
				if (dist > dgFloat32 (0.0f)) {
					foundBetterNode = false;
					return startNode;
				}
			}
		}
	}

	return NULL;
}

dgCollisionCompoundFractured* dgCollisionCompoundFractured::PlaneClip (const dgVector& plane)
{
	dgConectivityGraph::dgListNode* startNode = m_conectivity.GetFirst();
	if (IsAbovePlane (startNode, plane)) {
		startNode = FirstAcrossPlane (startNode, plane * dgVector::m_negOne);
	} else if (IsBelowPlane (startNode, plane)) {
		startNode = FirstAcrossPlane (startNode, plane);
	}

	if (startNode) {
		dgVector posgDir (plane & dgVector::m_triplexMask);
		dgVector negDir (posgDir * dgVector::m_negOne);
		dgTree<dgConectivityGraph::dgListNode*, dgConectivityGraph::dgListNode*> upperSide (GetAllocator());

		dgInt32 stack = 1;
		dgConectivityGraph::dgListNode* pool[512];
		pool[0] = startNode;
		m_lru += 2;
		while (stack) {
			stack --;
			dgConectivityGraph::dgListNode* const planeNode = pool[stack];
			dgDebriNodeInfo& nodeInfo = planeNode->GetInfo().m_nodeData;
	
			if (nodeInfo.m_lru != m_lru) {

				nodeInfo.m_lru = m_lru;

				dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* nextEdge;
				for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = planeNode->GetInfo().GetFirst(); edgeNode; edgeNode = nextEdge) {
					nextEdge = edgeNode->GetNext();

					dgConectivityGraph::dgListNode* const node = edgeNode->GetInfo().m_node;
					dgDebriNodeInfo& neighborgInfo = node->GetInfo().m_nodeData;
					if (neighborgInfo.m_lru != m_lru) {
						dgCollisionInstance* const instance = neighborgInfo.m_shapeNode->GetInfo()->GetShape();
						const dgMatrix& matrix = instance->GetLocalMatrix(); 

						dgVector support (matrix.TransformVector(instance->SupportVertex(matrix.UnrotateVector(negDir))));
						dgFloat32 dist = (support.DotProduct(plane)).GetScalar();
						if (dist > dgFloat32 (0.0f)) {
							upperSide.Insert(node, node);
							planeNode->GetInfo().DeleteEdge (edgeNode);
							node->GetInfo().m_nodeData.m_mesh->m_isVisible = true;
							planeNode->GetInfo().m_nodeData.m_mesh->m_isVisible = true;

							for (dgMesh::dgListNode* meshSegment = node->GetInfo().m_nodeData.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
								dgSubMesh* const subMesh = &meshSegment->GetInfo();
								subMesh->m_visibleFaces = true;
							}

							for (dgMesh::dgListNode* meshSegment = planeNode->GetInfo().m_nodeData.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
								dgSubMesh* const subMesh = &meshSegment->GetInfo();
								subMesh->m_visibleFaces = true;
							}


						} else {
							dgVector support1 (matrix.TransformVector(instance->SupportVertex(matrix.UnrotateVector(posgDir))));
							dgFloat32 dist1 = (support1.DotProduct(plane)).GetScalar();
							if (dist1 > dgFloat32 (0.0f)) {
								pool[stack] = node;
								stack ++;
								dgAssert (stack < sizeof (pool) / sizeof (pool[0]));
							}
						}
					}
				}
			}
		}
	}

	return NULL;
}

bool dgCollisionCompoundFractured::SpawnChunks (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const rootNode, dgFloat32 impulseStimate2, dgFloat32 impulseStimateCut2)
{
	dgFloat32 strengthPool[512];
	dgConectivityGraph::dgListNode* pool[512];

	dgVector massMatrix (myBody->GetMass());
	if (m_density < dgFloat32 (0.0f)) {
		m_density = dgAbs (massMatrix.m_w * m_density);
	}

	dgFloat32 attenuation = m_impulseAbsortionFactor;
	m_lru ++;
	pool[0] = rootNode;
	strengthPool[0] = impulseStimate2;

	dgInt32 stack = 1;
	bool spawned = false;
	while (stack) {
		stack --;
		dgFloat32 strenght = strengthPool[stack] * attenuation;
		dgConectivityGraph::dgListNode* const chunkNode = pool[stack];

		if ((strenght > impulseStimateCut2) && CanChunk (chunkNode)) {
			spawned = true;
			dgDebriNodeInfo& nodeInfo = chunkNode->GetInfo().m_nodeData;

			nodeInfo.m_mesh->m_isVisible = true;
			for (dgMesh::dgListNode* meshSegment = nodeInfo.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
				dgSubMesh* const subMesh = &meshSegment->GetInfo();
				subMesh->m_visibleFaces = true;
			}

			for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = chunkNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
				dgConectivityGraph::dgListNode* const node = edgeNode->GetInfo().m_node;
				dgDebriNodeInfo& childNodeInfo = node->GetInfo().m_nodeData;
				childNodeInfo.m_mesh->m_isVisible = true;
				for (dgMesh::dgListNode* meshSegment = childNodeInfo.m_mesh->GetFirst(); meshSegment; meshSegment = meshSegment->GetNext()) {
					dgSubMesh* const subMesh = &meshSegment->GetInfo();
					subMesh->m_visibleFaces = true;
				}
			}

			for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = chunkNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
				dgConectivityGraph::dgListNode* const node = edgeNode->GetInfo().m_node;
				dgDebriNodeInfo& childNodeInfo = node->GetInfo().m_nodeData;
				if (childNodeInfo.m_lru != m_lru) {
					childNodeInfo.m_lru = m_lru;
					strengthPool[stack] = strenght;
					pool[stack] = node;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
			SpawnSingleChunk (myBody, myInstance, chunkNode);
		}
	}
	return spawned;
}

void dgCollisionCompoundFractured::ColorDisjoinChunksIsland ()
{
	dgInt32 stack = 1;
	dgConectivityGraph::dgListNode* pool[512];

	dgInt32 stackMark = m_lru + 1;
	m_lru += 2;
	pool[0] = m_conectivity.GetFirst();
	stack = 1;
	while (stack) {
		stack --;
		dgConectivityGraph::dgListNode* const node = pool[stack];
		dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
		if (nodeInfo.m_lru <= stackMark) {
			nodeInfo.m_lru = m_lru;
			for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = node->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
				dgConectivityGraph::dgListNode* const node1 = edgeNode->GetInfo().m_node;
				dgDebriNodeInfo& childNodeInfo1 = node1->GetInfo().m_nodeData;
				if (childNodeInfo1.m_lru < stackMark) {
					childNodeInfo1.m_lru = stackMark;
					pool[stack] = node1;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
		}
	}
}

void dgCollisionCompoundFractured::SpawnDisjointChunks (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const rootNode, dgFloat32 impulseStimate2, dgFloat32 impulseStimateCut2)
{
	if (m_conectivity.GetFirst() != m_conectivity.GetLast()) {
		ColorDisjoinChunksIsland ();
		dgConectivityGraph::dgListNode* nextNode;
		for (dgConectivityGraph::dgListNode* node = m_conectivity.GetFirst(); node != m_conectivity.GetLast(); node = nextNode) {
			nextNode = node->GetNext();
			dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
			if (nodeInfo.m_lru != m_lru) {
				if (node->GetInfo().GetCount() == 0) {
					SpawnSingleChunk (myBody, myInstance, node);
				} else {
					do {
						nextNode = nextNode->GetPrev();
					} while (nextNode && (nextNode->GetInfo().m_nodeData.m_lru != m_lru));
					dgAssert (nextNode);
					SpawnComplexChunk (myBody, myInstance, node);
				}
			}
		}
	}

}


void dgCollisionCompoundFractured::SpawnSingleChunk (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const chunkNode)
{
	const dgMatrix& matrix = myBody->GetMatrix();
	const dgVector& veloc = myBody->GetVelocity();
	const dgVector& omega = myBody->GetOmega();
	dgVector com (matrix.TransformVector(myBody->GetCentreOfMass()));

	dgDebriNodeInfo& nodeInfo = chunkNode->GetInfo().m_nodeData;
	dgCollisionInstance* const chunkCollision = nodeInfo.m_shapeNode->GetInfo()->GetShape();
	dgDynamicBody* const chunkBody = m_world->CreateDynamicBody (chunkCollision, matrix);
	chunkBody->SetMassProperties(chunkCollision->GetVolume() * m_density, chunkBody->GetCollision());
	m_world->GetBroadPhase()->AddInternallyGeneratedBody(chunkBody);

	// calculate debris initial velocity
	dgVector chunkOrigin (matrix.TransformVector(chunkCollision->GetLocalMatrix().m_posit));
	dgVector chunkVeloc (veloc + omega.CrossProduct(chunkOrigin - com));

	chunkBody->SetOmega(omega);
	chunkBody->SetVelocity(chunkVeloc);
	chunkBody->SetGroupID(chunkCollision->GetUserDataID());

	m_emitFracturedChunk(chunkBody, chunkNode, myInstance);

	m_conectivityMap.Remove (chunkCollision);
	dgCollisionCompound::RemoveCollision (nodeInfo.m_shapeNode);
	m_conectivity.DeleteNode(chunkNode);
}

void dgCollisionCompoundFractured::SpawnComplexChunk (dgBody* const myBody, const dgCollisionInstance* const parentInstance, dgConectivityGraph::dgListNode* const chunkNode)
{
	dgInt32 stack = 1;
	dgConectivityGraph::dgListNode* pool[512];

	dgList<dgConectivityGraph::dgListNode*> islanList (GetAllocator());
	dgInt32 stackMark = m_lru - 1;
	pool[0] = chunkNode;
	stack = 1;
	while (stack) {
		stack --;
		dgConectivityGraph::dgListNode* const node = pool[stack];
		dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
		if (nodeInfo.m_lru <= stackMark) {
			islanList.Append(node);

			nodeInfo.m_lru = m_lru;
			for (dgGraphNode<dgDebriNodeInfo, dgSharedNodeMesh>::dgListNode* edgeNode = node->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
				dgConectivityGraph::dgListNode* const node1 = edgeNode->GetInfo().m_node;
				dgDebriNodeInfo& childNodeInfo1 = node1->GetInfo().m_nodeData;
				if (childNodeInfo1.m_lru < stackMark) {
					childNodeInfo1.m_lru = stackMark;
					pool[stack] = node1;
					stack ++;
					dgAssert (stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
		}
	}

	const dgMatrix& matrix = myBody->GetMatrix();
	const dgVector& veloc = myBody->GetVelocity();
	const dgVector& omega = myBody->GetOmega();
	dgVector com (matrix.TransformVector(myBody->GetCentreOfMass()));

	dgCollisionCompoundFractured* const childStructureCollision = new (GetAllocator()) dgCollisionCompoundFractured (*this, islanList);
	dgCollisionInstance* const childStructureInstance = m_world->CreateInstance (childStructureCollision, parentInstance->GetUserDataID(), parentInstance->GetLocalMatrix()); 
	childStructureCollision->m_myInstance = childStructureInstance;
	childStructureCollision->Release();

	dgDynamicBody* const chunkBody = m_world->CreateDynamicBody (childStructureInstance, matrix);
	chunkBody->SetMassProperties(childStructureInstance->GetVolume() * m_density, chunkBody->GetCollision());
	m_world->GetBroadPhase()->AddInternallyGeneratedBody(chunkBody);

	// calculate debris initial velocity
	dgVector chunkOrigin (matrix.TransformVector(childStructureInstance->GetLocalMatrix().m_posit));
	dgVector chunkVeloc (veloc + omega.CrossProduct(chunkOrigin - com));

	chunkBody->SetOmega(omega);
	chunkBody->SetVelocity(chunkVeloc);
	chunkBody->SetGroupID(childStructureInstance->GetUserDataID());

	m_emitFracturedCompound (chunkBody);
	childStructureInstance->Release();
}
