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

#ifndef __AFX_BROADPHASE_PERSINTENT_H_
#define __AFX_BROADPHASE_PERSINTENT_H_

#include "dgPhysicsStdafx.h"
#include "dgBroadPhase.h"


class dgBroadPhaseSegregated : public dgBroadPhase
{
	public:
	DG_CLASS_ALLOCATOR(allocator);

	dgBroadPhaseSegregated (dgWorld* const world);
	virtual ~dgBroadPhaseSegregated ();

	protected:
	virtual dgInt32 GetType() const;
	virtual void Add(dgBody* const body);
	virtual void Remove(dgBody* const body);
	virtual void InvalidateCache();
	virtual dgBroadPhaseAggregate* CreateAggregate();
	virtual void DestroyAggregate(dgBroadPhaseAggregate* const aggregate);

	virtual void CheckStaticDynamic(dgBody* const body, dgFloat32 mass);
	virtual void LinkAggregate(dgBroadPhaseAggregate* const aggregate);
	virtual void UnlinkAggregate(dgBroadPhaseAggregate* const aggregate);
	virtual void FindCollidingPairs (dgBroadphaseSyncDescriptor* const descriptor, dgList<dgBroadPhaseNode*>::dgListNode* const node, dgInt32 threadID);

	virtual void ResetEntropy();
	virtual void UpdateFitness();
	virtual void ForEachBodyInAABB(const dgVector& minBox, const dgVector& maxBox, OnBodiesInAABB callback, void* const userData) const;
	virtual void RayCast(const dgVector& p0, const dgVector& p1, OnRayCastAction filter, OnRayPrecastAction prefilter, void* const userData) const;
	virtual dgInt32 Collide(dgCollisionInstance* const shape, const dgMatrix& matrix, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const;
	virtual dgInt32 ConvexCast(dgCollisionInstance* const shape, const dgMatrix& matrix, const dgVector& target, dgFloat32* const param, OnRayPrecastAction prefilter, void* const userData, dgConvexCastReturnInfo* const info, dgInt32 maxContacts, dgInt32 threadIndex) const;
	void RemoveNode(dgBroadPhaseNode* const node);

	private:
	void AddStaticBody(dgBody* const body);
	void AddDynamicBody(dgBody* const body);

	dgFloat64 m_staticEntropy;
	dgFloat64 m_dynamicsEntropy;
	dgFitnessList m_staticFitness;
	dgFitnessList m_dynamicsFitness;
	bool m_staticNeedsUpdate;
};
#endif
