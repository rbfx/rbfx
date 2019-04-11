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
#include "dgCollisionInstance.h"
#include "dgBroadPhaseAggregate.h"
#include "dgBroadPhaseSegregated.h"


class dgBroadPhaseSegregatedRootNode: public dgBroadPhaseTreeNode
{
	public:
	dgBroadPhaseSegregatedRootNode()
		:dgBroadPhaseTreeNode()
	{
	}

	virtual bool IsSegregatedRoot() const
	{
		return true;
	}

	void SetBox ()
	{
		if (m_right && m_left) {
			dgVector minBox (m_right->m_minBox.GetMin(m_left->m_minBox));
			dgVector maxBox (m_right->m_maxBox.GetMin(m_left->m_maxBox));
			SetAABB(minBox, maxBox);
		} else if (m_right) {
			SetAABB(m_right->m_minBox, m_right->m_maxBox);
		} else if (m_left) {
			SetAABB(m_left->m_minBox, m_left->m_maxBox);
		}
	}
};

dgBroadPhaseSegregated::dgBroadPhaseSegregated (dgWorld* const world)
	:dgBroadPhase(world)
	,m_staticEntropy(dgFloat32 (0.0f))
	,m_dynamicsEntropy(dgFloat32 (0.0f))
	,m_staticFitness(world->GetAllocator())
	,m_dynamicsFitness(world->GetAllocator())
	,m_staticNeedsUpdate(true)
{
	m_rootNode = new (world->GetAllocator()) dgBroadPhaseSegregatedRootNode();
}

dgBroadPhaseSegregated::~dgBroadPhaseSegregated ()
{
	delete m_rootNode;
}

dgInt32 dgBroadPhaseSegregated::GetType() const
{
	return dgWorld::m_broadphaseSegregated;
}

void dgBroadPhaseSegregated::CheckStaticDynamic(dgBody* const body, dgFloat32 mass1)
{
	dgBroadPhaseNode* const node = body->GetBroadPhase();
	if (node) {
		const dgFloat32 mass0 = body->GetMass().m_w;
		const bool m0 = (mass0 >= DG_INFINITE_MASS);
		const bool m1 = (mass1 >= DG_INFINITE_MASS);
		if (m0 ^ m1) {
			Remove(body);
			if (m1) {
				AddStaticBody (body);
			} else {
				AddDynamicBody (body);
			}
		}
	}
}

void dgBroadPhaseSegregated::AddStaticBody(dgBody* const body)
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	dgAssert(m_rootNode->IsSegregatedRoot());

	m_staticNeedsUpdate = true;
	dgBroadPhaseBodyNode* const bodyNode = new (m_world->GetAllocator()) dgBroadPhaseBodyNode(body);
	if (root->m_right) {
		dgBroadPhaseTreeNode* const node = InsertNode(root->m_right, bodyNode);
		node->m_fitnessNode = m_staticFitness.Append(node);
	} else {
		root->m_right = bodyNode;
		root->m_right->m_parent = root;
	}
}

void dgBroadPhaseSegregated::AddDynamicBody(dgBody* const body)
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	dgAssert(m_rootNode->IsSegregatedRoot());

	dgBroadPhaseBodyNode* const newNode = new (m_world->GetAllocator()) dgBroadPhaseBodyNode(body);
	if (root->m_left) {
		dgBroadPhaseTreeNode* const node = InsertNode(root->m_left, newNode);
		node->m_fitnessNode = m_dynamicsFitness.Append(node);
	} else {
		root->m_left = newNode;
		root->m_left->m_parent = root;
	}
	newNode->m_updateNode = m_updateList.Append(newNode);
}


void dgBroadPhaseSegregated::Add(dgBody* const body)
{
	dgAssert (!body->GetCollision()->IsType (dgCollision::dgCollisionNull_RTTI));
	dgAssert (((dgBroadPhaseSegregatedRootNode*)m_rootNode)->IsSegregatedRoot());

	if (body->GetCollision()->IsType(dgCollision::dgCollisionMesh_RTTI) || (body->GetInvMass().m_w == dgFloat32(0.0f))) {
		AddStaticBody(body);
	} else {
		AddDynamicBody(body);
	}
}

dgBroadPhaseAggregate* dgBroadPhaseSegregated::CreateAggregate()
{
	dgBroadPhaseAggregate* const aggregate = new (m_world->GetAllocator()) dgBroadPhaseAggregate(m_world->GetBroadPhase());
	LinkAggregate(aggregate);
	return aggregate;
}

void dgBroadPhaseSegregated::LinkAggregate(dgBroadPhaseAggregate* const aggregate)
{
	dgAssert(m_rootNode->IsSegregatedRoot());
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;

	aggregate->m_broadPhase = this;
	if (root->m_left) {
		dgBroadPhaseTreeNode* const node = InsertNode(root->m_left, aggregate);
		node->m_fitnessNode = m_dynamicsFitness.Append(node);
	} else {
		root->m_left = aggregate;
		root->m_left->m_parent = m_rootNode;
	}
	aggregate->m_updateNode = m_updateList.Append(aggregate);
	aggregate->m_myAggregateNode = m_aggregateList.Append(aggregate);
}

void dgBroadPhaseSegregated::DestroyAggregate(dgBroadPhaseAggregate* const aggregate)
{
	m_updateList.Remove(aggregate->m_updateNode);
	m_aggregateList.Remove(aggregate->m_myAggregateNode);
	RemoveNode(aggregate);
}

void dgBroadPhaseSegregated::RemoveNode(dgBroadPhaseNode* const node)
{
	dgAssert (node->m_parent);

	if (node->m_parent->IsSegregatedRoot()) {
		dgBroadPhaseSegregatedRootNode* const parent = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
		dgAssert(parent == node->m_parent);

		if (parent->m_right == node) {
			m_staticNeedsUpdate = true;
			parent->m_right = NULL;
		} else {
			dgAssert(parent->m_left == node);
			parent->m_left = NULL;
		}
		node->m_parent = NULL;
		delete node;
	} else if (node->m_parent->IsAggregate()) {
		dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*)node->m_parent;
		dgBody* const body = node->GetBody();
		dgAssert(body);
		dgAssert(body->GetBroadPhaseAggregate() == aggregate);
		body->SetBroadPhaseAggregate(NULL);
		aggregate->m_root = NULL;
		node->m_parent = NULL;
		delete node;
	} else {
		dgBroadPhaseTreeNode* const parent = (dgBroadPhaseTreeNode*)node->m_parent;
		if (parent->m_parent->IsAggregate()) {
			dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*)parent->m_parent;
			if (parent->m_left == node) {
				dgAssert(parent->m_right);
				aggregate->m_root = parent->m_right;
				parent->m_right->m_parent = aggregate;
				parent->m_right = NULL;
			} else {
				dgAssert(parent->m_right == node);
				aggregate->m_root = parent->m_left;
				parent->m_left->m_parent = aggregate;
				parent->m_left = NULL;
			}
			parent->m_parent = NULL;

			if (parent->m_fitnessNode) {
				dgBody* const body = node->GetBody();
				if (body && body->GetBroadPhaseAggregate()) {
					body->GetBroadPhaseAggregate()->m_fitnessList.Remove(parent->m_fitnessNode);
					body->SetBroadPhaseAggregate(NULL);
				} else {
					m_dynamicsFitness.Remove(parent->m_fitnessNode);
				}
			}

			delete parent;

		} else if (parent->m_parent->IsSegregatedRoot()) {
			dgBroadPhaseSegregatedRootNode* const grandParent = (dgBroadPhaseSegregatedRootNode*) parent->m_parent;
			if (grandParent->m_right == parent) {
				m_staticNeedsUpdate = true;
				if (parent->m_right == node) {
					grandParent->m_right = parent->m_left;
					parent->m_left->m_parent = grandParent;
					parent->m_left = NULL;
					parent->m_parent = NULL;
				} else {
					grandParent->m_right = parent->m_right;
					parent->m_right->m_parent = grandParent;
					parent->m_right = NULL;
					parent->m_parent = NULL;
				}
				m_staticFitness.Remove(parent->m_fitnessNode);
				delete parent;
			} else {
				dgAssert (grandParent->m_left == parent);
				if (parent->m_right == node) {
					grandParent->m_left = parent->m_left;
					parent->m_left->m_parent = grandParent;
					parent->m_left = NULL;
					parent->m_parent = NULL;
				} else {
					grandParent->m_left = parent->m_right;
					parent->m_right->m_parent = grandParent;
					parent->m_right = NULL;
					parent->m_parent = NULL;
				}
				m_dynamicsFitness.Remove(parent->m_fitnessNode);
				delete parent;
			}
		} else {
			dgBroadPhaseTreeNode* const grandParent = (dgBroadPhaseTreeNode*)parent->m_parent;
			dgAssert (grandParent->GetLeft());
			dgAssert (grandParent->GetRight());
			if (grandParent->m_left == parent) {
				if (parent->m_right == node) {
					grandParent->m_left = parent->m_left;
					parent->m_left->m_parent = grandParent;
					parent->m_left = NULL;
					parent->m_parent = NULL;
				} else {
					grandParent->m_left = parent->m_right;
					parent->m_right->m_parent = grandParent;
					parent->m_right = NULL;
					parent->m_parent = NULL;
				}
			} else {
				if (parent->m_right == node) {
					grandParent->m_right = parent->m_left;
					parent->m_left->m_parent = grandParent;
					parent->m_left = NULL;
					parent->m_parent = NULL;
				} else {
					grandParent->m_right = parent->m_right;
					parent->m_right->m_parent = grandParent;
					parent->m_right = NULL;
					parent->m_parent = NULL;
				}
			}

			dgBody* const body = node->GetBody();
			if (body) {
				if (body->GetInvMass().m_w == dgFloat32(0.0f)) {
					m_staticNeedsUpdate = true;
					m_staticFitness.Remove(parent->m_fitnessNode);
				} else if (body->GetBroadPhaseAggregate()) {
					body->GetBroadPhaseAggregate()->m_fitnessList.Remove(parent->m_fitnessNode);
					body->SetBroadPhaseAggregate(NULL);
				} else {
					m_dynamicsFitness.Remove(parent->m_fitnessNode);
				}
			} else {
				dgAssert (node->IsAggregate());
				m_dynamicsFitness.Remove(parent->m_fitnessNode);
			}

			delete parent;
		}
	}
}

void dgBroadPhaseSegregated::UnlinkAggregate (dgBroadPhaseAggregate* const aggregate)
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	dgAssert (root && root->m_left);
	if (aggregate->m_parent == root) {
		root->m_left = NULL;
	} else {
		dgBroadPhaseTreeNode* const parent = (dgBroadPhaseTreeNode*)aggregate->m_parent;
		dgBroadPhaseTreeNode* const grandParent = (dgBroadPhaseTreeNode*)parent->m_parent;
		if (grandParent->m_left == parent) {
			if (parent->m_left == aggregate) {
				grandParent->m_left = parent->m_right;
				parent->m_right->m_parent = grandParent;
			} else {
				dgAssert(parent->m_right == aggregate);
				grandParent->m_left = parent->m_left;
				parent->m_left->m_parent = grandParent;
			}
		} else {
			dgAssert(grandParent->m_right == parent);
			if (parent->m_left == aggregate) {
				grandParent->m_right = parent->m_right;
				parent->m_right->m_parent = grandParent;
			} else {
				dgAssert(parent->m_right == aggregate);
				grandParent->m_right = parent->m_left;
				parent->m_left->m_parent = grandParent;
			}
		}
		parent->m_left = NULL;
		parent->m_right = NULL;
		parent->m_parent = NULL;
		delete parent;
	}
	aggregate->m_parent = NULL;
}

void dgBroadPhaseSegregated::Remove(dgBody* const body)
{
	if (body->GetBroadPhase()) {
		dgBroadPhaseBodyNode* const node = body->GetBroadPhase();
		if (node->m_updateNode) {
			m_updateList.Remove(node->m_updateNode);
		}
		RemoveNode(node);
	}
}

void dgBroadPhaseSegregated::ResetEntropy()
{
	m_staticNeedsUpdate = true;
	m_staticEntropy = dgFloat32(0.0f);
	m_dynamicsEntropy = dgFloat32(0.0f);
}


void dgBroadPhaseSegregated::InvalidateCache()
{
	ResetEntropy();
	m_staticNeedsUpdate = false;
	dgAssert (m_rootNode->IsSegregatedRoot());
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	ImproveFitness(m_staticFitness, m_staticEntropy, &root->m_right);
	ImproveFitness(m_dynamicsFitness, m_dynamicsEntropy, &root->m_left);
	root->SetBox ();
	m_contactCache.Flush();
}

void dgBroadPhaseSegregated::UpdateFitness()
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	if (m_staticNeedsUpdate) {
		m_staticNeedsUpdate = false;
		ImproveFitness(m_staticFitness, m_staticEntropy, &root->m_right);
	}
	ImproveFitness(m_dynamicsFitness, m_dynamicsEntropy, &root->m_left);
	root->SetBox ();
}

void dgBroadPhaseSegregated::ForEachBodyInAABB(const dgVector& minBox, const dgVector& maxBox, OnBodiesInAABB callback, void* const userData) const
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	const dgBroadPhaseNode* stackPool[DG_BROADPHASE_MAX_STACK_DEPTH];

	dgInt32 stack = 0;
	if (root->m_left) {
		stackPool[stack] = root->m_left;
		stack++;
	}

	if (root->m_right) {
		stackPool[stack] = root->m_right;
		stack++;
	}
	dgBroadPhase::ForEachBodyInAABB(stackPool, stack, minBox, maxBox, callback, userData);
}

void dgBroadPhaseSegregated::RayCast(const dgVector& l0, const dgVector& l1, OnRayCastAction filter, OnRayPrecastAction prefilter, void* const userData) const
{
	dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
	if (filter && (root->m_left || root->m_right)) {
		dgVector segment(l1 - l0);
		dgAssert (segment.m_w == dgFloat32 (0.0f));
		dgFloat32 dist2 = segment.DotProduct(segment).GetScalar();
		if (dist2 > dgFloat32(1.0e-8f)) {

			dgFloat32 distance[DG_BROADPHASE_MAX_STACK_DEPTH];
			const dgBroadPhaseNode* stackPool[DG_BROADPHASE_MAX_STACK_DEPTH];

			dgFastRayTest ray(l0, l1);

			dgInt32 stack = 0;
			if (root->m_left) {
				stackPool[stack] = root->m_left;
				distance[stack] = ray.BoxIntersect(root->m_left->m_minBox, root->m_left->m_maxBox);
				stack++;
			}
			if (root->m_right) {
				stackPool[stack] = root->m_right;
				distance[stack] = ray.BoxIntersect(root->m_right->m_minBox, root->m_right->m_maxBox);
				stack++;
			}
			if (stack == 2) {
				if (distance[0] < distance[1]) {
					dgSwap(distance[0], distance[1]);
					dgSwap(stackPool[0], stackPool[1]);
				}
			}

			dgBroadPhase::RayCast(stackPool, distance, stack, l0, l1, ray, filter, prefilter, userData);
		}
	}
}

dgInt32 dgBroadPhaseSegregated::ConvexCast(dgCollisionInstance* const shape, const dgMatrix& matrix, const dgVector& target, dgFloat32* const param, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const
{
	dgInt32 totalCount = 0;
	if (m_rootNode) {
		dgVector boxP0;
		dgVector boxP1;
		dgAssert(matrix.TestOrthogonal());
		shape->CalcAABB(matrix, boxP0, boxP1);

		dgFloat32 distance[DG_BROADPHASE_MAX_STACK_DEPTH];
		const dgBroadPhaseNode* stackPool[DG_BROADPHASE_MAX_STACK_DEPTH];

		dgVector velocA((target - matrix.m_posit) & dgVector::m_triplexMask);
		dgVector velocB(dgFloat32(0.0f));
		dgFastRayTest ray(dgVector(dgFloat32(0.0f)), velocA);

		dgInt32 stack = 0;
		dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
		if (root->m_left) {
			dgVector minBox(root->m_left->m_minBox - boxP1);
			dgVector maxBox(root->m_left->m_maxBox - boxP0);
			stackPool[stack] = root->m_left;
			distance[stack] = ray.BoxIntersect(minBox, maxBox);
			stack++;
		}
		if (root->m_right) {
			dgVector minBox(root->m_right->m_minBox - boxP1);
			dgVector maxBox(root->m_right->m_maxBox - boxP0);

			stackPool[stack] = root->m_right;
			distance[stack] = ray.BoxIntersect(minBox, maxBox);
			stack++;
		}
		if (stack == 2) {
			if (distance[0] < distance[1]) {
				dgSwap(distance[0], distance[1]);
				dgSwap(stackPool[0], stackPool[1]);
			}
		}

		*param = dgFloat32 (1.0f);
		totalCount = dgBroadPhase::ConvexCast(stackPool, distance, 2, velocA, velocB, ray, shape, matrix, target, param, prefilter, userData, info, maxContacts, threadIndex);
	}
	return totalCount;
}

dgInt32 dgBroadPhaseSegregated::Collide(dgCollisionInstance* const shape, const dgMatrix& matrix, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const
{
	dgInt32 totalCount = 0;
	if (m_rootNode) {
		dgVector boxP0;
		dgVector boxP1;
		dgAssert(matrix.TestOrthogonal());
		shape->CalcAABB(matrix, boxP0, boxP1);

		dgInt32 overlaped[DG_BROADPHASE_MAX_STACK_DEPTH];
		const dgBroadPhaseNode* stackPool[DG_BROADPHASE_MAX_STACK_DEPTH];

		dgInt32 stack = 0;
		dgBroadPhaseSegregatedRootNode* const root = (dgBroadPhaseSegregatedRootNode*)m_rootNode;
		if (dgOverlapTest(m_rootNode->m_minBox, m_rootNode->m_maxBox, boxP0, boxP1)) {
			if (root->m_left) {
				stackPool[stack] = root->m_left;
				overlaped[stack] = dgOverlapTest(root->m_left->m_minBox, root->m_left->m_maxBox, boxP0, boxP1);
				stack ++;
			}
			if (root->m_left) {
				stackPool[stack] = root->m_right;
				overlaped[stack] = dgOverlapTest(root->m_right->m_minBox, root->m_right->m_maxBox, boxP0, boxP1);
				stack++;
			}
			totalCount = dgBroadPhase::Collide(stackPool, overlaped, 1, boxP0, boxP1, shape, matrix, prefilter, userData, info, maxContacts, threadIndex);
		}
	}

	return totalCount;
}

void dgBroadPhaseSegregated::FindCollidingPairs (dgBroadphaseSyncDescriptor* const descriptor, dgList<dgBroadPhaseNode*>::dgListNode* const nodePtr, dgInt32 threadID)
{
	DG_TRACKTIME();
	const dgFloat32 timestep = descriptor->m_timestep;

	dgList<dgBroadPhaseNode*>::dgListNode* node = nodePtr;
	const dgInt32 threadCount = descriptor->m_world->GetThreadCount();

	if (descriptor->m_fullScan) {
		while (node) {
			dgBroadPhaseNode* const broadPhaseNode = node->GetInfo();
			dgAssert(broadPhaseNode->IsLeafNode());
			dgAssert(!broadPhaseNode->GetBody() || (broadPhaseNode->GetBody()->GetBroadPhase() == broadPhaseNode));

			if (broadPhaseNode->IsAggregate()) {
				((dgBroadPhaseAggregate*)broadPhaseNode)->SubmitSelfPairs(timestep, threadID);
			}

			for (dgBroadPhaseNode* ptr = broadPhaseNode; ptr->m_parent; ptr = ptr->m_parent) {
				dgBroadPhaseTreeNode* const parent = (dgBroadPhaseTreeNode*)ptr->m_parent;
				dgAssert(!parent->IsLeafNode());
				dgBroadPhaseNode* const sibling = parent->m_right;
				if (sibling && (sibling != ptr)) {
					SubmitPairs(broadPhaseNode, sibling, timestep, 0, threadID);
				}
			}

			for (dgInt32 i = 0; i < threadCount; i++) {
				dgBroadPhaseNode* const info = node ? node->GetInfo() : NULL;
				node = (info && ((info->GetBody() && (info->GetBody()->GetInvMass().m_w != dgFloat32(0.0f))) || info->IsAggregate())) ? node->GetNext() : NULL;
			}
		}	

	} else {
		const dgBodyInfo* const bodyArray = &m_world->m_bodiesMemory[0];
		const dgInt32 bodyCount = descriptor->m_atomicPendingBodiesCount;
		dgInt32* const atomicIndex = &descriptor->m_atomicIndex;

		for (dgInt32 i = dgAtomicExchangeAndAdd(atomicIndex, 1); i < bodyCount; i = dgAtomicExchangeAndAdd(atomicIndex, 1)) {
			dgBroadPhaseNode* const broadPhaseNode = bodyArray[i].m_body->GetBroadPhase();

			dgAssert(broadPhaseNode->IsLeafNode());
			dgAssert(!broadPhaseNode->GetBody() || (broadPhaseNode->GetBody()->GetBroadPhase() == broadPhaseNode));

			for (dgBroadPhaseNode* ptr = broadPhaseNode; ptr->m_parent; ptr = ptr->m_parent) {
				dgBroadPhaseTreeNode* const parent = (dgBroadPhaseTreeNode*)ptr->m_parent;
				if (!parent->IsAggregate()) {
					dgAssert(!parent->IsLeafNode());
					dgBroadPhaseNode* const rightSibling = parent->m_right;
					if (rightSibling && (rightSibling != ptr)) {
						SubmitPairs(broadPhaseNode, rightSibling, timestep, 0, threadID);
					}

					dgBroadPhaseNode* const leftSibling = parent->m_left;
					if (leftSibling && (leftSibling != ptr)) {
						SubmitPairs(broadPhaseNode, leftSibling, timestep, threadCount, threadID);
					}
				}
			}
		}
	}
}

