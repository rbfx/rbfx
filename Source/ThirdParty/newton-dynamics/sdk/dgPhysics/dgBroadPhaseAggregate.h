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

#ifndef __DG_BROADPHASE_AGGREGATE_H__
#define __DG_BROADPHASE_AGGREGATE_H__

#include "dgPhysicsStdafx.h"
#include "dgBroadPhase.h"

class dgBroadPhaseAggregate: public dgBroadPhaseNode
{
	public:
	dgBroadPhaseAggregate (dgBroadPhase* const broadPhase);
	virtual ~dgBroadPhaseAggregate();;

	virtual bool IsLeafNode() const
	{
		return true;
	}

	virtual bool IsAggregate() const
	{
		return true;
	}

	bool GetSelfCollision() const 
	{
		return m_isSelfCollidable ? true : false;
	}
	
	void SetSelfCollision(bool state) 
	{
		m_isSelfCollidable = state;
	}

	void AddBody (dgBody* const body);
	void RemoveBody (dgBody* const body);

	void ImproveEntropy ();
	void SubmitSelfPairs(dgFloat32 timestep, dgInt32 threadID) const;
	void SummitPairs(dgBody* const body, dgFloat32 timestep, dgInt32 threadID) const;
	void SummitPairs(dgBroadPhaseAggregate* const aggregate, dgFloat32 timestep, dgInt32 threadID) const;
	private:
	void SubmitSelfPairs(dgBroadPhaseNode* const node0, dgBroadPhaseNode* const node1, dgFloat32 timestep, dgInt32 threadID) const;

	public:
	dgBroadPhaseNode* m_root;
	dgBroadPhase* m_broadPhase;
	dgList<dgBroadPhaseNode*>::dgListNode* m_updateNode;
	dgList<dgBroadPhaseAggregate*>::dgListNode* m_myAggregateNode;
	dgList<dgBroadPhaseTreeNode*> m_fitnessList;
	dgFloat64 m_treeEntropy;
	bool m_isInEquilibrium;
	bool m_isSelfCollidable;
};

#endif
