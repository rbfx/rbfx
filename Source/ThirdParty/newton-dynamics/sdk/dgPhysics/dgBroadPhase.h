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

#ifndef __DG_BROADPHASE_H_
#define __DG_BROADPHASE_H_

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgBodyMasterList.h"

class dgBody;
class dgWorld;
class dgContact;
class dgCollision;
class dgDynamicBody;
class dgCollisionInstance;
class dgBroadPhaseAggregate;


#define DG_CACHE_DIST_TOL				dgFloat32 (1.0e-3f)
#define DG_BROADPHASE_MAX_STACK_DEPTH	256

class dgConvexCastReturnInfo
{
	public:
	dgFloat32 m_point[4];					// collision point in global space
	dgFloat32 m_normal[4];					// surface normal at collision point in global space
	//dgFloat32 m_normalOnHitPoint[4];		// surface normal at the surface of the hit body, 
											// is the same as the normal calculate by a raycast passing by the hit point in the direction of the cast
	dgInt64  m_contaID;	                // collision ID at contact point
	const dgBody* m_hitBody;				// body hit at contact point
	dgFloat32 m_penetration;                // contact penetration at collision point
};


DG_MSC_VECTOR_ALIGMENT
class dgBroadPhaseNode
{
	public:
	DG_CLASS_ALLOCATOR(allocator)
	dgBroadPhaseNode(dgBroadPhaseNode* const parent)
		:m_minBox(dgFloat32(-1.0e15f))
		,m_maxBox(dgFloat32(1.0e15f))
		,m_parent(parent)
		,m_surfaceArea(dgFloat32(1.0e20f))
		,m_criticalSectionLock(0)
	{
	}

	virtual ~dgBroadPhaseNode()
	{
	}

	virtual bool IsSegregatedRoot() const
	{
		return false;
	}

	virtual bool IsLeafNode() const
	{
		return false;
	}

	virtual bool IsAggregate() const
	{
		return false;
	}

	void SetAABB(const dgVector& minBox, const dgVector& maxBox)
	{
		dgAssert(minBox.m_x <= maxBox.m_x);
		dgAssert(minBox.m_y <= maxBox.m_y);
		dgAssert(minBox.m_z <= maxBox.m_z);

		dgVector p0(minBox * m_broadPhaseScale);
		dgVector p1(maxBox * m_broadPhaseScale + dgVector::m_one);

		m_minBox = p0.Floor() * m_broadInvPhaseScale;
		m_maxBox = p1.Floor() * m_broadInvPhaseScale;

		dgAssert(m_minBox.m_w == dgFloat32(0.0f));
		dgAssert(m_maxBox.m_w == dgFloat32(0.0f));

		dgVector side0(m_maxBox - m_minBox);
		m_surfaceArea = side0.DotProduct(side0.ShiftTripleRight()).m_x;
	}

	virtual dgBody* GetBody() const
	{
		return NULL;
	}

	virtual dgBroadPhaseNode* GetLeft() const
	{
		return NULL;
	}

	virtual dgBroadPhaseNode* GetRight() const
	{
		return NULL;
	}

	dgVector m_minBox;
	dgVector m_maxBox;
	dgBroadPhaseNode* m_parent;
	dgFloat32 m_surfaceArea;
	dgInt32 m_criticalSectionLock;

	static dgVector m_broadPhaseScale;
	static dgVector m_broadInvPhaseScale;
} DG_GCC_VECTOR_ALIGMENT;


class dgBroadPhaseBodyNode: public dgBroadPhaseNode
{
	public:
	dgBroadPhaseBodyNode(dgBody* const body)
		:dgBroadPhaseNode(NULL)
		,m_body(body)
		,m_updateNode(NULL)
	{
		SetAABB(body->m_minAABB, body->m_maxAABB);
		m_body->SetBroadPhase(this);
	}

	virtual ~dgBroadPhaseBodyNode()
	{
		m_body->SetBroadPhase(NULL);
	}

	virtual bool IsLeafNode() const
	{
		return true;
	}

	virtual dgBody* GetBody() const
	{
		return m_body;
	}

	dgBody* m_body;
	dgList<dgBroadPhaseNode*>::dgListNode* m_updateNode;
};

class dgBroadPhaseTreeNode: public dgBroadPhaseNode
{
	public:
	dgBroadPhaseTreeNode()
		:dgBroadPhaseNode(NULL)
		,m_left(NULL)
		,m_right(NULL)
		,m_fitnessNode(NULL)
	{
	}

	dgBroadPhaseTreeNode(dgBroadPhaseNode* const sibling, dgBroadPhaseNode* const myNode)
		:dgBroadPhaseNode(sibling->m_parent)
		,m_left(sibling)
		,m_right(myNode)
		,m_fitnessNode(NULL)
	{
		if (m_parent) {
			dgBroadPhaseTreeNode* const myParent = (dgBroadPhaseTreeNode*)m_parent;
			if (myParent->m_left == sibling) {
				myParent->m_left = this;
			} else {
				dgAssert(myParent->m_right == sibling);
				myParent->m_right = this;
			}
		}

		sibling->m_parent = this;
		myNode->m_parent = this;

		dgBroadPhaseNode* const left = m_left;
		dgBroadPhaseNode* const right = m_right;

		m_minBox = left->m_minBox.GetMin(right->m_minBox);
		m_maxBox = left->m_maxBox.GetMax(right->m_maxBox);
		dgVector side0(m_maxBox - m_minBox);
		m_surfaceArea = side0.DotProduct(side0.ShiftTripleRight()).m_x;
	}

	virtual ~dgBroadPhaseTreeNode()
	{
		if (m_left) {
			delete m_left;
		}
		if (m_right) {
			delete m_right;
		}
	}
	
	virtual dgBroadPhaseNode* GetLeft() const
	{
		return m_left;
	}

	virtual dgBroadPhaseNode* GetRight() const
	{
		return m_right;
	}

	dgBroadPhaseNode* m_left;
	dgBroadPhaseNode* m_right;
	dgList<dgBroadPhaseTreeNode*>::dgListNode* m_fitnessNode;
} DG_GCC_VECTOR_ALIGMENT;

#define DG_CONTACT_CACHE_LINE_SIZE 4

class dgBroadPhase
{
	protected:

	class CacheEntryTag
	{
		public:
		DG_INLINE CacheEntryTag() {}
		DG_INLINE CacheEntryTag(dgUnsigned32 tag0, dgUnsigned32 tag1)
			:m_tagLow(dgMin(tag0, tag1))
			,m_tagHigh(dgMax(tag0, tag1))
		{
		}

		DG_INLINE dgUnsigned32 GetHash() const
		{
			return m_tagHigh * 31415821u + m_tagLow;
		}
	
		union
		{
			dgUnsigned64 m_tag;
			struct 
			{
				dgUnsigned32 m_tagLow;
				dgUnsigned32 m_tagHigh;
			};
		};
	};

	class dgContactCacheLine
	{
		public:
		DG_INLINE dgContactCacheLine()
		{
		}

		dgInt32 m_count;
		dgUnsigned32 m_key;
		dgContact* m_contact[DG_CONTACT_CACHE_LINE_SIZE];
		dgInt32 m_hashKey[DG_CONTACT_CACHE_LINE_SIZE];
		CacheEntryTag m_tags[DG_CONTACT_CACHE_LINE_SIZE];
	};

	class dgContactCache: public dgArray<dgContactCacheLine>
	{
		public:
		dgContactCache (dgMemoryAllocator* const allocator)
			:dgArray<dgContactCacheLine>(allocator)
			,m_count(1<<10)
		{
			Init();
		}

		void Flush()
		{
			Clear();
			Init();
		}

		void Init()
		{
			m_count = 1 << 10;
			ResizeIfNecessary(m_count);
			dgContactCacheLine* const cache = &(*this)[0];
			memset(cache, 0, m_count * sizeof(dgContactCacheLine));
		}

		DG_INLINE dgContact* FindContactJoint(const dgBody* const body0, const dgBody* const body1) const
		{
			CacheEntryTag tag(body0->m_uniqueID, body1->m_uniqueID);
			dgUnsigned32 hash = tag.GetHash();

			dgInt32 entry = hash & (m_count - 1);

			const dgContactCacheLine& cacheLine = (*this)[entry];
			for (dgInt32 i = 0; i < cacheLine.m_count; i++) {
				if (cacheLine.m_tags[i].m_tag == tag.m_tag) {
					return cacheLine.m_contact[i];
				}
			}
			return NULL;
		}

		DG_INLINE bool AddContactJoint(dgContact* const joint)
		{
			// note this function is not thread safe
			CacheEntryTag tag(joint->GetBody0()->m_uniqueID, joint->GetBody1()->m_uniqueID);
			dgUnsigned32 hash = tag.GetHash();

			dgInt32 entry = hash & (m_count - 1);
			dgContactCacheLine* cacheLine = &(*this)[entry];

			for (dgInt32 i = cacheLine->m_count - 1; i >= 0; i--) {
				if (cacheLine->m_tags[i].m_tag == tag.m_tag) {
					return false;
				}
			}

			while (cacheLine->m_count == 4) {
				Rehash();
				entry = hash & (m_count - 1);
				cacheLine = &(*this)[entry];
			}
			if (cacheLine->m_count == 0) {
				cacheLine->m_key = hash;
			}

			const dgInt32 index = cacheLine->m_count;
			cacheLine->m_count++;
			cacheLine->m_tags[index] = tag;
			cacheLine->m_hashKey[index] = hash;
			cacheLine->m_contact[index] = joint;
			return true;
		}

		DG_INLINE void RemoveContactJoint(dgContact* const joint)
		{
			CacheEntryTag tag(joint->GetBody0()->m_uniqueID, joint->GetBody1()->m_uniqueID);
			dgUnsigned32 hash = tag.GetHash();

			dgInt32 entry = hash & (m_count - 1);
			dgContactCacheLine* const cacheLine = &(*this)[entry];
			for (dgInt32 i = cacheLine->m_count - 1; i >= 0 ; i--) {
				if (cacheLine->m_tags[i].m_tag == tag.m_tag) {
					cacheLine->m_count--;
					const dgInt32 index = cacheLine->m_count;
					cacheLine->m_tags[i] = cacheLine->m_tags[index];
					cacheLine->m_hashKey[i] = cacheLine->m_hashKey[index];
					cacheLine->m_contact[i] = cacheLine->m_contact[index];
					break;
				}
			}
		}

		private:
		void Rehash()
		{
			const dgInt32 newCount = m_count * 2;
			ResizeIfNecessary(newCount);
			dgContactCacheLine* const cache0 = &(*this)[0];
			dgContactCacheLine* const cache1 = &cache0[m_count];
	
			const dgInt32 mask = newCount - 1;
			for (dgInt32 i = 0; i < m_count; i++) {
				dgContactCacheLine* const src = &cache0[i];
				dgContactCacheLine* const dst = &cache1[i];
				dst->m_count = 0;
				for (dgInt32 j = src->m_count - 1; j >= 0; j--) {
					dgInt32 entry = src->m_hashKey[j] & mask;
					if (entry >= m_count) {
						const dgInt32 dstIndex = dst->m_count;
						dst->m_count++;
						dst->m_tags[dstIndex] = src->m_tags[j];
						dst->m_hashKey[dstIndex] = src->m_hashKey[j];
						dst->m_contact[dstIndex] = src->m_contact[j];

						src->m_count--;
						const dgInt32 srcIndex = src->m_count;
						src->m_tags[j] = src->m_tags[srcIndex];
						src->m_hashKey[j] = src->m_hashKey[srcIndex];
						src->m_contact[j] = src->m_contact[srcIndex];
					}
				}
			}
			m_count = newCount;
		}

		dgInt32 m_count;
	};

	class dgSpliteInfo;
	class dgBroadphaseSyncDescriptor
	{
		public:
		dgBroadphaseSyncDescriptor(dgFloat32 timestep, dgWorld* const world)
			:m_world(world)
			,m_timestep(timestep)
			,m_atomicIndex(0)
			,m_atomicDynamicsCount(0)
			,m_atomicPendingBodiesCount(0)
			,m_fullScan(false)
		{
		}

		dgWorld* m_world;
		dgFloat32 m_timestep;
		dgInt32 m_atomicIndex;
		dgInt32 m_atomicDynamicsCount;
		dgInt32 m_atomicPendingBodiesCount;
		bool m_fullScan;
	};
	
	class dgFitnessList: public dgList <dgBroadPhaseTreeNode*>
	{
		public:
		dgFitnessList(dgMemoryAllocator* const allocator)
			:dgList <dgBroadPhaseTreeNode*>(allocator)
			,m_index(0)
			,m_prevCost(dgFloat32 (0.0f))
		{
		}

		dgFloat64 TotalCost() const
		{
			dgFloat64 cost = dgFloat32(0.0f);
			for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
				dgBroadPhaseNode* const box = node->GetInfo();
				cost += box->m_surfaceArea;
			}
			return cost;
		}

		dgInt32 m_index;
		dgFloat64 m_prevCost;
	};

	public:
	enum dgContactCode
	{
		m_close,
		m_persist,
		m_separated,
	};

	class dgPair
	{
    	public:
		dgContact* m_contact;
		dgContactPoint* m_contactBuffer;
		dgFloat32 m_timestep;
		dgInt32 m_contactCount : 16;
		dgInt32 m_cacheIsValid : 1;
		dgInt32 m_flipContacts : 1;
	};

	dgBroadPhase(dgWorld* const world);
	virtual ~dgBroadPhase();

	DG_INLINE dgUnsigned32 GetLRU() const
	{
		return m_lru;
	}

	DG_INLINE dgFloat32 CalculateSurfaceArea(const dgBroadPhaseNode* const node0, const dgBroadPhaseNode* const node1, dgVector& minBox, dgVector& maxBox) const
	{
		minBox = node0->m_minBox.GetMin(node1->m_minBox);
		maxBox = node0->m_maxBox.GetMax(node1->m_maxBox);
		dgVector side0(maxBox - minBox);
		return side0.DotProduct(side0.ShiftTripleRight()).GetScalar();
	}

	dgWorld* GetWorld() const { return m_world;}

	virtual dgInt32 GetType() const = 0;
	
	virtual void Add(dgBody* const body) = 0;
	virtual void Remove(dgBody* const body) = 0;

	virtual void ResetEntropy() = 0;
	virtual void UpdateFitness() = 0;
	virtual void InvalidateCache() = 0;
	virtual dgBroadPhaseAggregate* CreateAggregate() = 0;
	virtual void DestroyAggregate(dgBroadPhaseAggregate* const aggregate) = 0;

	virtual void CheckStaticDynamic(dgBody* const body, dgFloat32 mass) = 0;
	virtual void ForEachBodyInAABB (const dgVector& minBox, const dgVector& maxBox, OnBodiesInAABB callback, void* const userData) const = 0;
	virtual void RayCast (const dgVector& p0, const dgVector& p1, OnRayCastAction filter, OnRayPrecastAction prefilter, void* const userData) const = 0;
	virtual dgInt32 Collide(dgCollisionInstance* const shape, const dgMatrix& matrix, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const = 0;
	virtual dgInt32 ConvexCast (dgCollisionInstance* const shape, const dgMatrix& matrix, const dgVector& target, dgFloat32* const param, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const = 0;
	virtual void FindCollidingPairs (dgBroadphaseSyncDescriptor* const descriptor, dgList<dgBroadPhaseNode*>::dgListNode* const node, dgInt32 threadID) = 0;

	void DeleteDeadContacts();
	void AttachNewContacts(dgContactList::dgListNode* const lastNode);
	void UpdateBody(dgBody* const body, dgInt32 threadIndex);
	void AddInternallyGeneratedBody(dgBody* const body)
	{
		m_generatedBodies.Append(body);
	}

	void UpdateContacts(dgFloat32 timestep);
	void CollisionChange (dgBody* const body, dgCollisionInstance* const collisionSrc);

	void MoveNodes (dgBroadPhase* const dest);

	protected:
	virtual void LinkAggregate (dgBroadPhaseAggregate* const aggregate) = 0; 
	virtual void UnlinkAggregate (dgBroadPhaseAggregate* const aggregate) = 0; 

	bool DoNeedUpdate(dgBodyMasterList::dgListNode* const node) const;
	dgFloat64 CalculateEntropy (dgFitnessList& fitness, dgBroadPhaseNode** const root);
	dgBroadPhaseTreeNode* InsertNode (dgBroadPhaseNode* const root, dgBroadPhaseNode* const node);

	void RotateLeft(dgBroadPhaseTreeNode* const node, dgBroadPhaseNode** const root);
	void RotateRight(dgBroadPhaseTreeNode* const node, dgBroadPhaseNode** const root);
	void ImproveNodeFitness(dgBroadPhaseTreeNode* const node, dgBroadPhaseNode** const root);
	void ImproveFitness(dgFitnessList& fitness, dgFloat64& oldEntropy, dgBroadPhaseNode** const root);

	void CalculatePairContacts (dgPair* const pair, dgInt32 threadID);
	bool ValidateContactCache(dgContact* const contact, dgFloat32 timestep) const;
	void AddPair (dgContact* const contact, dgFloat32 timestep, dgInt32 threadIndex);
	void AddPair (dgBody* const body0, dgBody* const body1, dgFloat32 timestep, dgInt32 threadID);	

	bool TestOverlaping(const dgBody* const body0, const dgBody* const body1, dgFloat32 timestep) const;

	void ForEachBodyInAABB (const dgBroadPhaseNode** stackPool, dgInt32 stack, const dgVector& minBox, const dgVector& maxBox, OnBodiesInAABB callback, void* const userData) const;
	void RayCast (const dgBroadPhaseNode** stackPool, dgFloat32* const distance, dgInt32 stack, const dgVector& l0, const dgVector& l1, dgFastRayTest& ray, OnRayCastAction filter, OnRayPrecastAction prefilter, void* const userData) const;

	dgInt32 ConvexCast (const dgBroadPhaseNode** stackPool, dgFloat32* const distance, dgInt32 stack, const dgVector& velocA, const dgVector& velocB, dgFastRayTest& ray,  
						dgCollisionInstance* const shape, const dgMatrix& matrix, const dgVector& target, dgFloat32* const param, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const;

	dgInt32 Collide(const dgBroadPhaseNode** stackPool, dgInt32* const overlap, dgInt32 stack, const dgVector& p0, const dgVector& p1, 
		            dgCollisionInstance* const shape, const dgMatrix& matrix, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const;

	void SleepingState (dgBroadphaseSyncDescriptor* const descriptor, dgBodyMasterList::dgListNode* node, dgInt32 threadID);
	void ApplyForceAndtorque (dgBroadphaseSyncDescriptor* const descriptor, dgBodyMasterList::dgListNode* node, dgInt32 threadID);
	
	void UpdateAggregateEntropy (dgBroadphaseSyncDescriptor* const descriptor, dgList<dgBroadPhaseAggregate*>::dgListNode* node, dgInt32 threadID);

	dgBroadPhaseNode* BuildTopDown(dgBroadPhaseNode** const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgFitnessList::dgListNode** const nextNode);
	dgBroadPhaseNode* BuildTopDownBig(dgBroadPhaseNode** const leafArray, dgInt32 firstBox, dgInt32 lastBox, dgFitnessList::dgListNode** const nextNode);

	void KinematicBodyActivation (dgContact* const contatJoint) const;
	
	void FindGeneratedBodiesCollidingPairs (dgBroadphaseSyncDescriptor* const descriptor, dgInt32 threadID);
	void UpdateSoftBodyContacts(dgBroadphaseSyncDescriptor* const descriptor, dgFloat32 timeStep, dgInt32 threadID);
	void UpdateRigidBodyContacts (dgBroadphaseSyncDescriptor* const descriptor, dgContactList::dgListNode* const node, dgFloat32 timeStep, dgInt32 threadID);
	void SubmitPairs (dgBroadPhaseNode* const body, dgBroadPhaseNode* const node, dgFloat32 timestep, dgInt32 threaCount, dgInt32 threadID);
	void AddNewContacts(dgBroadphaseSyncDescriptor* const descriptor, dgContactList::dgListNode* const nodeConstactNode, dgInt32 threadID);
	bool SanityCheck() const;
		
	static void SleepingStateKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void ForceAndToqueKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void CollidingPairsKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void AddNewContactsKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void UpdateAggregateEntropyKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void AddGeneratedBodiesContactsKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void UpdateRigidBodyContactKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static void UpdateSoftBodyContactKernel(void* const descriptor, void* const worldContext, dgInt32 threadID);
	static dgInt32 CompareNodes(const dgBroadPhaseNode* const nodeA, const dgBroadPhaseNode* const nodeB, void* const notUsed);

	class dgPendingCollisionSoftBodies
	{
		public:
		dgBody* m_body0;
		dgBody* m_body1;
	};

	dgWorld* m_world;
	dgBroadPhaseNode* m_rootNode;
	dgList<dgBody*> m_generatedBodies;
	dgList<dgBroadPhaseNode*> m_updateList;
	dgList<dgBroadPhaseAggregate*> m_aggregateList;
	dgUnsigned32 m_lru;
	dgContactCache m_contactCache;
	dgArray<dgPendingCollisionSoftBodies> m_pendingSoftBodyCollisions;
	dgInt32 m_pendingSoftBodyPairsCount;
	dgInt32 m_contacJointLock;
	dgInt32 m_criticalSectionLock;

	static dgVector m_velocTol;
	static dgVector m_linearContactError2;
	static dgVector m_angularContactError2;

	friend class dgBody;
	friend class dgWorld;
	friend class dgWorldDynamicUpdate;
	friend class dgBroadPhaseAggregate;
	friend class dgCollisionCompoundFractured;
};


#endif
