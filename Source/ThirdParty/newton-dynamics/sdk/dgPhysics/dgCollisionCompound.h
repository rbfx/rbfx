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

#ifndef __DGCOLLISIONCOMPOUND_H__
#define __DGCOLLISIONCOMPOUND_H__


#include "dgCollision.h"

class dgFastRayTest;
class dgCollisionBVH;
class dgCollisionInstance;


#define DG_COMPOUND_STACK_DEPTH	256

class dgCollisionCompound: public dgCollision
{
	protected:
	enum dgNodeType
	{
		m_leaf,
		m_node,
	};

	public:

	class dgNodeBase;
	class dgTreeArray: public dgTree<dgNodeBase*, dgInt32>
	{
		public:
		dgTreeArray (dgMemoryAllocator* const allocator);
		void AddNode (dgNodeBase* const node, dgInt32 index, const dgCollisionInstance* const parent); 
	};

	DG_MSC_VECTOR_ALIGMENT
	class dgOOBBTestData
	{
		public:
		dgOOBBTestData (const dgMatrix& matrix);
		dgOOBBTestData (const dgMatrix& matrix, const dgVector& origin, const dgVector& size);

		DG_INLINE dgFloat32 UpdateSeparatingDistance(const dgVector& box0Min, const dgVector& box0Max, const dgVector& box1Min, const dgVector& box1Max) const;

		dgMatrix m_matrix;
		dgMatrix m_absMatrix;
		dgVector m_origin;
		dgVector m_size;
		dgVector m_localP0;
		dgVector m_localP1;
		dgVector m_aabbP0;
		dgVector m_aabbP1;

		dgVector m_crossAxis[9];
		dgVector m_crossAxisAbs[9];
		dgVector m_crossAxisDotAbs[9];
		dgVector m_extendsMinX[3];
		dgVector m_extendsMaxX[3];
		mutable dgFloat32 m_separatingDistance;
		static dgVector m_maxDist;
	} DG_GCC_VECTOR_ALIGMENT;


	DG_MSC_VECTOR_ALIGMENT
	class dgNodeBase
	{
		public:
		DG_CLASS_ALLOCATOR(allocator)
		dgNodeBase (); 
		dgNodeBase (const dgNodeBase& copyFrom);
		dgNodeBase (dgCollisionInstance* const instance);
		dgNodeBase (dgNodeBase* const left, dgNodeBase* const right);
		~dgNodeBase();

		void CalculateAABB();
		void SetBox (const dgVector& p0, const dgVector& p1);
		bool BoxTest (const dgOOBBTestData& data) const;
		bool BoxTest (const dgOOBBTestData& data, const dgNodeBase* const otherNode) const;
		dgFloat32 RayBoxDistance (const dgOOBBTestData& data, const dgFastRayTest& myRay, const dgFastRayTest& otherRay, const dgNodeBase* const otherNode) const;

		DG_INLINE dgCollisionInstance* GetShape() const 
		{
			return m_shape;
		}

		DG_INLINE dgInt32 BoxIntersect (const dgFastRayTest& ray, const dgVector& boxP0, const dgVector& boxP1) const
		{
			dgVector minBox (m_p0 - boxP1);
			dgVector maxBox (m_p1 - boxP0);
			return ray.BoxTest(minBox, maxBox);
		}

		dgVector m_p0;
		dgVector m_p1;
		dgVector m_size;
		dgVector m_origin;
		dgFloat32 m_area;
		dgInt32 m_type;
		dgNodeBase* m_left;
		dgNodeBase* m_right;
		dgNodeBase* m_parent;
		dgCollisionInstance* m_shape;
		dgTreeArray::dgTreeNode* m_myNode; 
	} DG_GCC_VECTOR_ALIGMENT;

	protected:
	class dgNodePairs
	{
		public:
		const void* m_treeNode;
		dgNodeBase* m_myNode;
		dgInt32 m_treeNodeIsLeaf;
	};

	class dgSpliteInfo;
	class dgHeapNodePair;

	public:
	dgCollisionCompound (dgWorld* const world);
	dgCollisionCompound (const dgCollisionCompound& source, const dgCollisionInstance* const myInstance);
	dgCollisionCompound (dgWorld* const world, dgDeserialize deserialization, void* const userData, const dgCollisionInstance* const myInstance, dgInt32 revisionNumber);

	void SetParent (const dgCollisionInstance* const myInstance);
	virtual ~dgCollisionCompound();

	virtual void BeginAddRemove ();
	virtual dgTreeArray::dgTreeNode* AddCollision (dgCollisionInstance* const part);
	virtual void RemoveCollision (dgTreeArray::dgTreeNode* const node);
	virtual void SetCollisionMatrix (dgTreeArray::dgTreeNode* const node, const dgMatrix& matrix);
	virtual void EndAddRemove (bool flushCache = true);

	void ApplyScale (const dgVector& scale);
	void GetAABB (dgVector& p0, dgVector& p1) const;

	dgInt32 GetNodeIndex(dgTreeArray::dgTreeNode* const node) const;
	dgTreeArray::dgTreeNode* FindNodeByIndex (dgInt32 index) const;

	dgTreeArray::dgTreeNode* GetFirstNode () const;
	dgTreeArray::dgTreeNode* GetNextNode (dgTreeArray::dgTreeNode* const node) const;
	dgCollisionInstance* GetCollisionFromNode (dgTreeArray::dgTreeNode* const node) const;

	protected:
	void RemoveCollision (dgNodeBase* const node);
	virtual dgFloat32 GetVolume () const;
	virtual dgFloat32 GetBoxMinRadius () const; 
	virtual dgFloat32 GetBoxMaxRadius () const;
	
	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const {return point;}

	virtual void CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const;
	
	static void CalculateInertia (void* userData, int vertexCount, const dgFloat32* const FaceArray, int faceId);

	virtual void MassProperties ();
	dgMatrix CalculateInertiaAndCenterOfMass (const dgMatrix& m_alignMatrix, const dgVector& localScale, const dgMatrix& matrix) const;
	dgFloat32 CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const;
	virtual dgVector CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& plane, const dgCollisionInstance& parentScale) const;

	virtual void DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;

	virtual dgInt32 CalculateSignature () const;
	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1);
	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;
	virtual dgInt32 CalculateContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;

	dgInt32 CalculateContactsToSingle (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToSingleContinue (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToCompound (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToCompoundContinue (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToCollisionTree (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToCollisionTreeContinue (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsToHeightField (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateContactsUserDefinedCollision (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	dgInt32 ClosestDistance (dgCollisionParamProxy& proxy) const;
	dgInt32 ClosestDistanceToConvex (dgCollisionParamProxy& proxy) const;
	dgInt32 ClosestDistanceToCompound (dgCollisionParamProxy& proxy) const;

#ifdef _DEBUG
	dgVector InternalSupportVertex (const dgVector& dir) const;
#endif
	
	dgNodeBase* BuildTopDown (dgNodeBase** const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgList<dgNodeBase*>::dgListNode** const nextNode);
	dgNodeBase* BuildTopDownBig (dgNodeBase** const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgList<dgNodeBase*>::dgListNode** const nextNode);

	dgFloat64 CalculateEntropy (dgList<dgNodeBase*>& list);

	void ImproveNodeFitness (dgNodeBase* const node) const;
	DG_INLINE dgFloat32 CalculateSurfaceArea (dgNodeBase* const node0, dgNodeBase* const node1, dgVector& minBox, dgVector& maxBox) const;

	dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const;

	void PushNode (const dgMatrix& matrix, dgUpHeap<dgHeapNodePair, dgFloat32>& heap, dgNodeBase* const myNode, dgNodeBase* const otehrNode) const;

	static dgInt32 CompareNodes (const dgNodeBase* const nodeA, const dgNodeBase* const nodeB, void* notUsed);


	dgWorld* m_world;	
	dgNodeBase* m_root;
	const dgCollisionInstance* m_myInstance;
	dgTreeArray m_array;
	dgFloat64 m_treeEntropy;
	dgFloat32 m_boxMinRadius;
	dgFloat32 m_boxMaxRadius;
	dgInt32 m_idIndex;
	dgInt32 m_criticalSectionLock;

	static dgVector m_padding;
	friend class dgBody;
	friend class dgWorld;
	friend class dgCollisionScene;
};

DG_INLINE dgFloat32 dgCollisionCompound::dgOOBBTestData::UpdateSeparatingDistance(const dgVector& box0Min, const dgVector& box0Max, const dgVector& box1Min, const dgVector& box1Max) const
{
	dgVector minBox(box0Min - box1Max);
	dgVector maxBox(box0Max - box1Min);
	dgVector mask((minBox * maxBox) < dgVector::m_zero);
	dgVector dist((mask & m_maxDist) | ((maxBox.Abs()).GetMin(minBox.Abs())).AndNot(mask));
//	dgVector dist(((maxBox.Abs()).GetMin(minBox.Abs())).AndNot(mask));
	dist = dist.GetMin(dist.ShiftTripleRight());
	dist = dist.GetMin(dist.ShiftTripleRight());
	return dist.GetScalar();
}


#endif 

