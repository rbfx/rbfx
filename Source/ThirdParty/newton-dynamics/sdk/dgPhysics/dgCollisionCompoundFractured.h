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

#ifndef DG_COLLISION_COMPOUND_FRACTURED_H
#define DG_COLLISION_COMPOUND_FRACTURED_H

#include "dgCollisionCompound.h"


class dgMeshEffect;



class dgCollisionCompoundFractured: public dgCollisionCompound
{
	class dgFractureBuilder;

	public:
	class dgFlatVertex
	{
		public:
		dgFloat32 m_point[10]; // 3 point, 3 normal. 2 uv0, 2 uv1
	};

	class dgFlatVertexArray: public dgArray<dgFlatVertex>
	{
		public:
		dgFlatVertexArray(dgMemoryAllocator* const allocator)
			:dgArray<dgFlatVertex> (allocator)
		{
			dgAssert(0);
			m_count = 0;
		}

		dgInt32 m_count;
	};

	class dgVertexBuffer: public dgRefCounter  
	{
		public: 
		DG_CLASS_ALLOCATOR(allocator)
		dgVertexBuffer (dgInt32 count, dgMemoryAllocator* allocator);
		dgVertexBuffer (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData);
		~dgVertexBuffer ();

		void Serialize(dgSerialize callback, void* const userData) const;
		const dgFloat32* GetVertexPositions() const {return m_vertex;};
		const dgFloat32* GetVertexNormals() const {return m_normal;};
		const dgFloat32* GetVertexUVs() const {return m_uv;};

		dgFloat32 *m_uv;
		dgFloat32 *m_vertex;
		dgFloat32 *m_normal;
		dgMemoryAllocator* m_allocator;
		dgInt32 m_vertexCount;
	};

	class dgSubMesh
	{
		public:
		dgSubMesh (dgMemoryAllocator* const allocator);
		~dgSubMesh ();
		void Serialize(dgSerialize callback, void* const userData) const;

		dgInt32 *m_indexes;
		dgMemoryAllocator* m_allocator;
		dgInt32 m_material;
		dgInt32 m_faceCount;
		dgInt32 m_materialOrdinal;
		bool m_visibleFaces;
	};

	class dgMesh: public dgList<dgSubMesh>, public dgRefCounter 
	{
		public:
		dgMesh(dgMemoryAllocator* const allocator);
		dgMesh (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData);
		~dgMesh();
		void Serialize (dgSerialize callback, void* const userData) const;
		dgSubMesh* AddgSubMesh (dgInt32 indexCount, dgInt32 material);

		dgInt32 m_vertexOffsetStart;
		dgInt32 m_vertexCount;
		bool m_isVisible;
	};

	class dgDebriNodeInfo
	{
		public:
		dgDebriNodeInfo ();
		~dgDebriNodeInfo ();

		dgMesh* m_mesh;
		dgTreeArray::dgTreeNode* m_shapeNode;
		dgInt32 m_lru;
	};


	class dgSharedNodeMesh
	{
		public:
		dgSharedNodeMesh ();
		~dgSharedNodeMesh ();
        dgVector m_normal;
	};

	class dgConectivityGraph: public dgGraph<dgDebriNodeInfo, dgSharedNodeMesh>
	{
		public:
		dgConectivityGraph (dgMemoryAllocator* const allocator);
		dgConectivityGraph (const dgConectivityGraph& source);
		~dgConectivityGraph ();

		dgListNode* AddNode (dgFlatVertexArray& vertexArray, dgMeshEffect* const factureVisualMesh, dgTreeArray::dgTreeNode* const collisionNode, dgInt32 interiorMaterialBase);
		void Serialize(dgSerialize callback, void* const userData) const;
		void Deserialize (dgCollisionCompoundFractured* const source, dgDeserialize callback, void* const userData);
	};

	class dgConectivityGraphMap: public dgTree<dgConectivityGraph::dgListNode*, const dgCollisionInstance*>
	{
		public:
		dgConectivityGraphMap (const dgConectivityGraphMap& source)
			:dgTree<dgConectivityGraph::dgListNode*, const dgCollisionInstance*>(source.GetAllocator())
		{
		}

		dgConectivityGraphMap (dgMemoryAllocator* const allocator)
			:dgTree<dgConectivityGraph::dgListNode*, const dgCollisionInstance*>(allocator)
		{
		}

		void Pupolate(const dgConectivityGraph& graph)
		{
			for (dgConectivityGraph::dgListNode* node = graph.GetFirst(); node != graph.GetLast(); node = node->GetNext() ) {
				dgDebriNodeInfo& nodeInfo = node->GetInfo().m_nodeData;
				Insert(node, nodeInfo.m_shapeNode->GetInfo()->GetShape());
			}
		}
	};

	public:
	typedef void (*OnEmitNewCompundFractureCallBack) (dgBody* const body);
	typedef void (*OnEmitFractureChunkCallBack) (dgBody* const body, dgConectivityGraph::dgListNode* const chunkMeshNode, const dgCollisionInstance* const myInstance);
	typedef void (*OnReconstructFractureMainMeshCallBack) (dgBody* const body, dgConectivityGraph::dgListNode* const mainMeshNode, const dgCollisionInstance* const myInstance);

	dgCollisionCompoundFractured (const dgCollisionCompoundFractured& source, const dgCollisionInstance* const myInstance);
	dgCollisionCompoundFractured (dgCollisionCompoundFractured& source, const dgList<dgConectivityGraph::dgListNode*>& island);
	dgCollisionCompoundFractured (dgWorld* const world, dgMeshEffect* const solidMesh, dgInt32 fracturePhysicsMaterialID, int pointcloudCount, const dgFloat32* const vertexCloud, int strideInBytes, int materialID, const dgMatrix& offsetMatrix,
								 OnEmitFractureChunkCallBack emitFracturedChunk, OnEmitNewCompundFractureCallBack emitNewCompoundFactured, OnReconstructFractureMainMeshCallBack reconstructMainMesh);

	dgCollisionCompoundFractured (dgWorld* const world, dgDeserialize deserialization, void* const userData, const dgCollisionInstance* const myInstance, dgInt32 revisionNumber);
	virtual ~dgCollisionCompoundFractured(void);

	void SetCallbacks(OnEmitFractureChunkCallBack emitFracturedChunk, OnEmitNewCompundFractureCallBack emitNewCompoundFactured, OnReconstructFractureMainMeshCallBack reconstructMainMesh);

	dgConectivityGraph::dgListNode* GetMainMesh() const;
	dgConectivityGraph::dgListNode* GetFirstMesh() const;
	dgConectivityGraph::dgListNode* GetNextMesh(dgConectivityGraph::dgListNode* const mesh) const;

	dgInt32 GetVertecCount(dgConectivityGraph::dgListNode* const node) const;
	const dgFloat32* GetVertexPositions (dgConectivityGraph::dgListNode* const node) const;
	const dgFloat32* GetVertexNormal (dgConectivityGraph::dgListNode* const node) const;
	const dgFloat32* GetVertexUVs (dgConectivityGraph::dgListNode* const node) const;
	dgInt32 GetSegmentIndexStream (dgConectivityGraph::dgListNode* const node, dgMesh::dgListNode* const segment, dgInt32* const index) const;

	void SetImpulseStrength(dgFloat32 impulseStrength);
	dgFloat32 GetImpulseStrength() const;

	void SetImpulsePropgationFactor(dgFloat32 factor);
	dgFloat32 GetSetImpulsePropgationFactor() const;

	virtual void BeginAddRemove ();
	virtual void RemoveCollision (dgTreeArray::dgTreeNode* const node);
	virtual void EndAddRemove (bool flushCache = true);
	bool IsNodeSaseToDetach (dgTreeArray::dgTreeNode* const node) const;

	int GetFirstNiegborghArray (dgTreeArray::dgTreeNode* const node, dgTreeArray::dgTreeNode** const nodesArray, int maxCount) const;

	dgCollisionCompoundFractured* PlaneClip (const dgVector& plane);

	private:
	void BuildMainMeshSubMehes() const;
	dgVector GetObbSize() const;

	virtual void Serialize(dgSerialize callback, void* const userData) const;
    virtual void CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const;
	dgInt32 CalculateContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;

	void ColorDisjoinChunksIsland ();
	bool SpawnChunks (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const rootNode, dgFloat32 impulseStimate2, dgFloat32 impulseStimateCut2);
	void SpawnDisjointChunks (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const rootNode, dgFloat32 impulseStimate2, dgFloat32 impulseStimateCut2);

	void SpawnSingleChunk (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const chunkNode);
	void SpawnComplexChunk (dgBody* const myBody, const dgCollisionInstance* const myInstance, dgConectivityGraph::dgListNode* const chunkNode);
    bool CanChunk (dgConectivityGraph::dgListNode* const node) const;
	
	bool SanityCheck() const;
	
	inline bool IsAbovePlane (dgConectivityGraph::dgListNode* const node, const dgVector& plane) const;
	inline bool IsBelowPlane (dgConectivityGraph::dgListNode* const node, const dgVector& plane) const;
	inline dgConectivityGraph::dgListNode* FirstAcrossPlane (dgConectivityGraph::dgListNode* const node, const dgVector& plane) const;

	dgConectivityGraph m_conectivity;
	dgConectivityGraphMap m_conectivityMap;
	dgVertexBuffer* m_vertexBuffer;
	dgFloat32 m_impulseStrengthPerUnitMass;
	dgFloat32 m_impulseAbsortionFactor;
	dgFloat32 m_density;
	dgInt32 m_lru;
	dgInt32 m_materialCount;
	OnEmitFractureChunkCallBack m_emitFracturedChunk;
	OnEmitNewCompundFractureCallBack m_emitFracturedCompound;
	OnReconstructFractureMainMeshCallBack m_reconstructMainMesh;
};
#endif
