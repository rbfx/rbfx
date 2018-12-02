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
#include "dgBroadPhaseAggregate.h"


dgBroadPhaseAggregate::dgBroadPhaseAggregate(dgBroadPhase* const broadPhase)
	:dgBroadPhaseNode(NULL)
	,m_root(NULL)
	,m_broadPhase(broadPhase)
	,m_updateNode(NULL)
	,m_myAggregateNode(NULL)
	,m_fitnessList(broadPhase->m_world->GetAllocator())
	,m_treeEntropy(dgFloat32(0.0f))
	,m_isInEquilibrium(false)
	,m_isSelfCollidable(true)
{
	m_minBox = dgVector(dgFloat32(0.0f));
	m_maxBox = dgVector(dgFloat32(0.0f));
	m_surfaceArea = dgFloat32(0.0f);
}

dgBroadPhaseAggregate::~dgBroadPhaseAggregate()
{
	if (m_root) {
		dgBody* buffer[2040];

		dgBroadPhaseNode* pool[DG_BROADPHASE_MAX_STACK_DEPTH];
		pool[0] = m_root;
		dgInt32 stack = 1;

		dgInt32 count = 0;
		while (stack) {
			stack--;
			dgBroadPhaseNode* const rootNode = pool[stack];
			if (rootNode->IsLeafNode()) {
				buffer[count] = rootNode->GetBody();;
				count ++;
			} else {
				dgBroadPhaseTreeNode* const tmpNode = (dgBroadPhaseTreeNode*)rootNode;
				dgAssert(tmpNode->m_left);
				dgAssert(tmpNode->m_right);

				pool[stack] = tmpNode->m_left;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
				pool[stack] = tmpNode->m_right;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
			}
		}

		for (dgInt32 i = 0; i < count; i ++) {
			RemoveBody(buffer[i]);
		}
		dgAssert (!m_root);
	}
}

void dgBroadPhaseAggregate::AddBody(dgBody* const body)
{
	dgAssert(body->GetBroadPhase());
	m_broadPhase->Remove(body);

	dgBroadPhaseBodyNode* const newNode = new (m_broadPhase->GetWorld()->GetAllocator()) dgBroadPhaseBodyNode(body);
	if (!m_root) {
		m_root = newNode;
		newNode->m_parent = this;
	} else {
		dgBroadPhaseTreeNode* const tmp = m_broadPhase->InsertNode(m_root, newNode);
		dgList<dgBroadPhaseTreeNode*>::dgListNode* const link = m_fitnessList.Append(tmp);
		tmp->m_fitnessNode = link;
	}
	body->m_broadPhaseaggregateNode = this;
	SetAABB (m_root->m_minBox, m_root->m_maxBox);
	for (dgBroadPhaseNode* ptr = this; ptr->m_parent; ptr = ptr->m_parent) {
		if (dgBoxInclusionTest(ptr->m_minBox, ptr->m_maxBox, ptr->m_parent->m_minBox, ptr->m_parent->m_maxBox)) {
			break;
		}
		dgVector minBox;
		dgVector maxBox;
		dgFloat32 area;
		area = m_broadPhase->CalculateSurfaceArea(ptr->m_parent, ptr, minBox, maxBox);
		ptr->m_parent->m_minBox = minBox;
		ptr->m_parent->m_maxBox = maxBox;
		ptr->m_parent->m_surfaceArea = area;
	}
}

void dgBroadPhaseAggregate::RemoveBody(dgBody* const body)
{
	dgAssert(body->GetBroadPhase());
	m_broadPhase->Remove(body);
	m_broadPhase->Add(body);
}


void dgBroadPhaseAggregate::ImproveEntropy()
{
	if (m_root) {
		if (m_root->IsLeafNode()) {
			dgAssert (m_root->GetBody());
			m_isInEquilibrium = m_root->GetBody()->m_equilibrium;
		} else if (!m_isInEquilibrium) {

			bool equlibrium = true;
			dgFloat64 entropy = dgFloat32(0.0f);
			for (dgList<dgBroadPhaseTreeNode*>::dgListNode* ptr = m_fitnessList.GetFirst(); ptr; ptr = ptr->GetNext()) {
				dgBroadPhaseTreeNode* const tmpNode = ptr->GetInfo();
				entropy += tmpNode->m_surfaceArea;
				const dgBody* const leftBody = tmpNode->m_left->GetBody();
				const dgBody* const rightBody = tmpNode->m_right->GetBody();
				equlibrium &= (!leftBody || leftBody->m_equilibrium) ? true : false;
				equlibrium &= (!rightBody ||rightBody->m_equilibrium) ? true : false;
			}

			m_isInEquilibrium = equlibrium;
			if (!m_isInEquilibrium && ((entropy > m_treeEntropy * dgFloat32(2.0f)) || (entropy < m_treeEntropy * dgFloat32(0.5f)))) {
				m_root->m_parent = NULL;
				dgFloat64 cost0 = entropy;
				dgFloat64 cost1 = cost0;
				do {
					cost0 = cost1;
					for (dgList<dgBroadPhaseTreeNode*>::dgListNode* ptr = m_fitnessList.GetFirst(); ptr; ptr = ptr->GetNext()) {
						dgBroadPhaseTreeNode* const tmpNode = ptr->GetInfo();
						m_broadPhase->ImproveNodeFitness(tmpNode, &m_root);
					}
					cost1 = dgFloat32(0.0f);
					for (dgList<dgBroadPhaseTreeNode*>::dgListNode* ptr = m_fitnessList.GetFirst(); ptr; ptr = ptr->GetNext()) {
						dgBroadPhaseTreeNode* const tmpNode = ptr->GetInfo();
						cost1 += tmpNode->m_surfaceArea;
					}
				} while (cost1 < (dgFloat32(0.99f)) * cost0);

				m_treeEntropy = cost1;
				m_root->m_parent = this;
				m_minBox = m_root->m_minBox;
				m_maxBox = m_root->m_maxBox;
				m_surfaceArea = m_root->m_surfaceArea;
			}
		}
	}
}

void dgBroadPhaseAggregate::SummitPairs(dgBroadPhaseAggregate* const aggregate, dgFloat32 timestep, dgInt32 threadID) const
{
	if (m_root && aggregate->m_root && !(m_isInEquilibrium & aggregate->m_isInEquilibrium)) {
		SubmitSelfPairs(m_root, aggregate->m_root, timestep, threadID);
	}
}

void dgBroadPhaseAggregate::SubmitSelfPairs(dgFloat32 timestep, dgInt32 threadID) const
{
	if (m_root && !m_root->IsLeafNode()) {
		if (!m_isInEquilibrium & m_isSelfCollidable) {
			SubmitSelfPairs(m_root->GetLeft(), m_root->GetRight(), timestep, threadID);
		}
	}
}

void dgBroadPhaseAggregate::SummitPairs(dgBody* const body, dgFloat32 timestep, dgInt32 threadID) const
{
	if (m_root) {
		if (m_root->IsLeafNode()) {
			dgAssert (m_root->GetBody());
			m_broadPhase->AddPair(body, m_root->GetBody(), timestep, threadID);
		} else if (!(m_isInEquilibrium & body->m_equilibrium)) {
			dgBroadPhaseNode* pool[DG_BROADPHASE_MAX_STACK_DEPTH/2];
			pool[0] = m_root;
			dgInt32 stack = 1;

			const dgVector& boxP0 = body->m_minAABB;
			const dgVector& boxP1 = body->m_maxAABB;

			while (stack) {
				stack--;
				dgBroadPhaseNode* const rootNode = pool[stack];
				if (dgOverlapTest(rootNode->m_minBox, rootNode->m_maxBox, boxP0, boxP1)) {
					if (rootNode->IsLeafNode()) {
						dgBody* const body1 = rootNode->GetBody();
						dgAssert (body1);
						m_broadPhase->AddPair(body, body1, timestep, threadID);
					} else {
						dgBroadPhaseTreeNode* const tmpNode = (dgBroadPhaseTreeNode*)rootNode;
						dgAssert(tmpNode->m_left);
						dgAssert(tmpNode->m_right);

						pool[stack] = tmpNode->m_left;
						stack++;
						dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

						pool[stack] = tmpNode->m_right;
						stack++;
						dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
					}
				}
			}
		}
	}
}

void dgBroadPhaseAggregate::SubmitSelfPairs(dgBroadPhaseNode* const node0, dgBroadPhaseNode* const node1, dgFloat32 timestep, dgInt32 threadID) const
{
/*
	dgInt32 stack = 1;
	dgBroadPhaseNode* pool[DG_BROADPHASE_MAX_STACK_DEPTH][2];

	pool[0][0] = node0;
	pool[0][1] = node1;
	while (stack) {
		stack--;
		dgBroadPhaseNode* const root0 = pool[stack][0];
		dgBroadPhaseNode* const root1 = pool[stack][1];
		if (dgOverlapTest(root0->m_minBox, root0->m_maxBox, root1->m_minBox, root1->m_maxBox)) {
			if (root0->IsLeafNode()) {
				if (root1->IsLeafNode()) {
					dgBody* const body0 = root0->GetBody();
					dgBody* const body1 = root1->GetBody();
					dgAssert(body0);
					dgAssert(body1);
					m_broadPhase->AddPair(body0, body1, timestep, threadID);
				} else {
					dgBroadPhaseTreeNode* const tmpNode1 = (dgBroadPhaseTreeNode*)root1;
					dgAssert(tmpNode1->m_left);
					dgAssert(tmpNode1->m_right);

					pool[stack][0] = root0;
					pool[stack][1] = tmpNode1->m_left;
					stack++;
					dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

					pool[stack][0] = root0;
					pool[stack][1] = tmpNode1->m_right;
					stack++;
					dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
				}
			} else if (root1->IsLeafNode()) {
				dgBroadPhaseTreeNode* const tmpNode0 = (dgBroadPhaseTreeNode*)root0;
				dgAssert(tmpNode0->m_left);
				dgAssert(tmpNode0->m_right);

				pool[stack][0] = root1;
				pool[stack][1] = tmpNode0->m_left;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

				pool[stack][0] = root1;
				pool[stack][1] = tmpNode0->m_right;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
			} else {

				dgBroadPhaseTreeNode* const tmpNode0 = (dgBroadPhaseTreeNode*)root0;
				dgBroadPhaseTreeNode* const tmpNode1 = (dgBroadPhaseTreeNode*)root1;
				dgAssert(tmpNode0->m_left);
				dgAssert(tmpNode0->m_right);
				dgAssert(tmpNode1->m_left);
				dgAssert(tmpNode1->m_right);

				pool[stack][0] = tmpNode0->m_left;
				pool[stack][1] = tmpNode1->m_left;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

				pool[stack][0] = tmpNode0->m_left;
				pool[stack][1] = tmpNode1->m_right;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

				pool[stack][0] = tmpNode0->m_right;
				pool[stack][1] = tmpNode1->m_left;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));

				pool[stack][0] = tmpNode0->m_right;
				pool[stack][1] = tmpNode1->m_right;
				stack++;
				dgAssert(stack < dgInt32(sizeof (pool) / sizeof (pool[0])));
			}
		}
	}
*/

	const dgBroadPhaseNode* pool[DG_BROADPHASE_MAX_STACK_DEPTH][2];
	pool[0][0] = node0;
	pool[0][1] = node1;
	dgInt32 stack = 1;

	while (stack) {
		stack--;

		const dgBroadPhaseNode* const left = pool[stack][0];
		const dgBroadPhaseNode* const right = pool[stack][1];

		if (left->IsLeafNode() && right->IsLeafNode()) {
			dgBody* const body0 = left->GetBody();
			dgBody* const body1 = right->GetBody();
			if (dgOverlapTest(body0->m_minAABB, body0->m_maxAABB, body1->m_minAABB, body1->m_maxAABB)) {
				m_broadPhase->AddPair(body0, body1, timestep, threadID);
			}
		} else {
			if (left->m_parent == right->m_parent) {
				if (!left->IsLeafNode()) {
					pool[stack][0] = left->GetLeft();
					pool[stack][1] = left->GetRight();
					stack++;
					dgAssert(stack < sizeof(pool) / sizeof(pool[0]));
				}
				if (!right->IsLeafNode()) {
					pool[stack][0] = right->GetLeft();
					pool[stack][1] = right->GetRight();
					stack++;
					dgAssert(stack < sizeof(pool) / sizeof(pool[0]));
				}
			}

			const dgBroadPhaseNode* leftPool[2];
			const dgBroadPhaseNode* rightPool[2];
			dgInt32 leftCount = 2;
			dgInt32 rightCount = 2;
			if (left->IsLeafNode()) {
				leftCount = 1;
				dgAssert(!right->IsLeafNode());
				leftPool[0] = left;
				rightPool[0] = right->GetLeft();
				rightPool[1] = right->GetRight();
			} else if (right->IsLeafNode()) {
				rightCount = 1;
				dgAssert(!left->IsLeafNode());
				leftPool[0] = left->GetLeft();
				leftPool[1] = left->GetRight();
				rightPool[0] = right;
			}
			else {
				leftPool[0] = left->GetLeft();
				leftPool[1] = left->GetRight();
				rightPool[0] = right->GetLeft();
				rightPool[1] = right->GetRight();
			}

			for (dgInt32 i = 0; i < leftCount; i++) {
				for (dgInt32 j = 0; j < rightCount; j++) {
					if (dgOverlapTest(leftPool[i]->m_minBox, leftPool[i]->m_maxBox, rightPool[j]->m_minBox, rightPool[j]->m_maxBox)) {
						pool[stack][0] = leftPool[i];
						pool[stack][1] = rightPool[j];
						stack++;
						dgAssert(stack < sizeof (pool) / sizeof (pool[0]));
					}
				}
			}
		}
	}
}
