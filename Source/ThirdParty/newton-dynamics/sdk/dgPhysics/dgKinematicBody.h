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

#ifndef _DG_KINEMATIC_BODY_H_
#define _DG_KINEMATIC_BODY_H_

#include "dgPhysicsStdafx.h"
#include "dgBody.h"


DG_MSC_VECTOR_ALIGMENT
class dgKinematicBody: public dgBody 
{
	public:
	dgKinematicBody();
	dgKinematicBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber);
	virtual ~dgKinematicBody ();
	
	virtual const dgVector& GetForce() const {return m_dummy;}
	virtual const dgVector& GetTorque() const {return m_dummy;}
	
	virtual void AddForce (const dgVector& force) {}
	virtual void AddTorque (const dgVector& torque) {}
	virtual void SetForce (const dgVector& force) {}
	virtual void SetTorque (const dgVector& torque) {} 
	virtual void ApplyExtenalForces (dgFloat32 timestep, dgInt32 threadIndex) {}
	virtual OnApplyExtForceAndTorque GetExtForceAndTorqueCallback () const {return NULL;}
	virtual void SetExtForceAndTorqueCallback (OnApplyExtForceAndTorque callback) {}

	virtual dgFloat32 GetLinearDamping () const {return dgFloat32 (0.0f);}
	virtual dgVector GetAngularDamping () const {return m_dummy;}
	virtual void SetLinearDamping (dgFloat32 linearDamp) {}
	virtual void SetAngularDamping (const dgVector& angularDamp) {}

	virtual dgVector PredictLinearVelocity(dgFloat32 timestep) const {return m_veloc;}
	virtual dgVector PredictAngularVelocity(dgFloat32 timestep) const {return m_omega;}

	virtual bool IsInEquilibrium  () const {return true;}
	virtual void SetCollidable (bool state) {m_collidable = state;}
	virtual void Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData);
	virtual void AddDampingAcceleration(dgFloat32 timestep) {}

	static dgVector m_dummy;

	friend class dgWorld;
	friend class dgBroadPhase;
	friend class dgWorldDynamicUpdate;
} DG_GCC_VECTOR_ALIGMENT;



#endif 

