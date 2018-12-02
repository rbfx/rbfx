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

#ifndef _DG_DYNAMIC_BODY_H_
#define _DG_DYNAMIC_BODY_H_

#include "dgPhysicsStdafx.h"
#include "dgBody.h"

#define DG_MAX_SPEED_ATT	dgFloat32(0.02f)

#define DG_FREEZE_ACCEL		dgFloat32(0.1f)
#define DG_FREEZE_SPEED		dgFloat32(0.032f)

#define DG_FREEZE_ACCEL2	(DG_FREEZE_ACCEL * DG_FREEZE_ACCEL)
#define DG_FREEZE_SPEED2	(DG_FREEZE_SPEED * DG_FREEZE_SPEED)

#define DG_FREEZE_MAG		DG_FREEZE_ACCEL
#define DG_FREEZE_MAG2		(DG_FREEZE_MAG * DG_FREEZE_MAG)

#define DG_ERR_TOLERANCE	dgFloat32(1.0e-2f)
#define DG_ERR_TOLERANCE2	(DG_ERR_TOLERANCE * DG_ERR_TOLERANCE)

class dgSkeletonContainer;

DG_MSC_VECTOR_ALIGMENT
class dgDynamicBody : public dgBody 
{
	public:
	dgDynamicBody();
	dgDynamicBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber);
	virtual ~dgDynamicBody ();

	virtual const dgVector& GetForce() const;
	virtual const dgVector& GetTorque() const;
	
	virtual void AddForce (const dgVector& force);
	virtual void AddTorque (const dgVector& torque);
	virtual void SetForce (const dgVector& force);
	virtual void SetTorque (const dgVector& torque);

	virtual dgFloat32 GetLinearDamping () const;
	virtual dgVector GetAngularDamping () const;
	virtual void SetLinearDamping (dgFloat32 linearDamp);
	virtual void SetAngularDamping (const dgVector& angularDamp);

	virtual void AttachCollision (dgCollisionInstance* const collision);
	virtual dgVector PredictLinearVelocity(dgFloat32 timestep) const;
	virtual dgVector PredictAngularVelocity(dgFloat32 timestep) const;

	virtual void InvalidateCache();
	virtual void SetMatrixResetSleep(const dgMatrix& matrix);
	virtual void SetMatrixNoSleep(const dgMatrix& matrix);

	virtual bool IsInEquilibrium () const;
	virtual void SetCollidable (bool state) {}

	virtual void ApplyExtenalForces (dgFloat32 timestep, dgInt32 threadIndex);
	virtual OnApplyExtForceAndTorque GetExtForceAndTorqueCallback () const;
	virtual void SetExtForceAndTorqueCallback (OnApplyExtForceAndTorque callback);
	virtual void Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData);

	virtual dgSkeletonContainer* GetSkeleton() const;
	void SetSkeleton(dgSkeletonContainer* const skeleton);

	void IntegrateImplicit(dgFloat32 timeStep);
	void IntegrateExplicit(dgFloat32 timeStep, dgInt32 method);

	virtual void IntegrateOpenLoopExternalForce(dgFloat32 timeStep);
	const dgVector& GetDampCoeffcient (dgFloat32 timestep);

	private:
	virtual void AddDampingAcceleration(dgFloat32 timestep);

	virtual dgVector GetAlpha() const;
	virtual dgVector GetAccel() const;
	virtual void SetAlpha(const dgVector& alpha);
	virtual void SetAccel(const dgVector& accel);

	dgVector m_externalForce;
	dgVector m_externalTorque;
	dgVector m_savedExternalForce;
	dgVector m_savedExternalTorque;
	dgVector m_dampCoef;
	dgVector m_cachedDampCoef;
	dgFloat32 m_cachedTimeStep;
	dgInt32 m_sleepingCounter;
	dgUnsigned32 m_isInDestructionArrayLRU;
	dgSkeletonContainer* m_skeleton;
	OnApplyExtForceAndTorque m_applyExtForces;
	bool m_linearDampOn;
	bool m_angularDampOn;
	static dgVector m_equilibriumError2;

	friend class dgWorld;
	friend class dgSolver;
	friend class dgBroadPhase;
	friend class dgBodyMasterList;
	friend class dgInverseDynamics;
	friend class dgSkeletonContainer;
	friend class dgWorldDynamicUpdate;
	friend class dgParallelBodySolver;
	friend class dgCollisionDeformableMesh;
	friend class dgCollisionDeformableSolidMesh;
	friend class dgCollisionMassSpringDamperSystem;
} DG_GCC_VECTOR_ALIGMENT;


DG_MSC_VECTOR_ALIGMENT
class dgDynamicBodyAsymetric: public dgDynamicBody
{
	public:
	dgDynamicBodyAsymetric();
	dgDynamicBodyAsymetric(dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber);

	virtual void Serialize(const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData);

	virtual dgMatrix CalculateInertiaMatrix() const;
	virtual dgMatrix CalculateLocalInertiaMatrix() const;
	virtual dgMatrix CalculateInvInertiaMatrix() const;
	virtual void SetMassMatrix(dgFloat32 mass, const dgMatrix& inertia);
	virtual void IntegrateOpenLoopExternalForce(dgFloat32 timestep);

	dgMatrix m_principalAxis;
} DG_GCC_VECTOR_ALIGMENT;



DG_INLINE const dgVector& dgDynamicBody::GetForce() const
{
	return m_externalForce; 
}

DG_INLINE const dgVector& dgDynamicBody::GetTorque() const
{
	return m_externalTorque;
}


DG_INLINE dgFloat32 dgDynamicBody::GetLinearDamping () const
{
	return m_dampCoef.m_w / DG_MAX_SPEED_ATT;
}

DG_INLINE dgVector dgDynamicBody::GetAngularDamping () const
{
	return dgVector (m_dampCoef.m_x / DG_MAX_SPEED_ATT,
					 m_dampCoef.m_y / DG_MAX_SPEED_ATT,
					 m_dampCoef.m_z / DG_MAX_SPEED_ATT, dgFloat32 (0.0f));
}

DG_INLINE void dgDynamicBody::SetLinearDamping (dgFloat32 linearDamp)
{
	linearDamp = dgClamp (linearDamp, dgFloat32(0.0f), dgFloat32(1.0f));
	m_dampCoef.m_w = DG_MAX_SPEED_ATT * linearDamp;

	m_linearDampOn = m_dampCoef.m_w > dgFloat32 (1.0e-7f);
	m_cachedTimeStep = dgFloat32(0.0f);
}

DG_INLINE void dgDynamicBody::SetAngularDamping (const dgVector& angularDamp)
{
	dgFloat32 tmp = dgClamp (angularDamp.m_x, dgFloat32(0.0f), dgFloat32(1.0f));
	m_dampCoef.m_x = DG_MAX_SPEED_ATT * tmp;

	tmp = dgClamp (angularDamp.m_y, dgFloat32(0.0f), dgFloat32(1.0f));
	m_dampCoef.m_y = DG_MAX_SPEED_ATT * tmp;

	tmp = dgClamp (angularDamp.m_z, dgFloat32(0.0f), dgFloat32(1.0f));
	m_dampCoef.m_z = DG_MAX_SPEED_ATT * tmp;

	m_angularDampOn = m_dampCoef.DotProduct(m_dampCoef & dgVector::m_triplexMask).GetScalar() > dgFloat32 (1.0e-12f);
	m_cachedTimeStep = dgFloat32(0.0f);
}

DG_INLINE void dgDynamicBody::AddForce (const dgVector& force)
{
	SetForce (m_externalForce + force);
}

DG_INLINE void dgDynamicBody::AddTorque (const dgVector& torque)
{
	SetTorque (torque + m_externalTorque);
}


DG_INLINE void dgDynamicBody::SetForce (const dgVector& force)
{
	m_externalForce = force;
}

DG_INLINE void dgDynamicBody::SetTorque (const dgVector& torque)
{
	m_externalTorque = torque;
}


DG_INLINE dgVector dgDynamicBody::PredictLinearVelocity(dgFloat32 timestep) const
{
	return 	m_veloc + m_externalForce.Scale (timestep * m_invMass.m_w);
}

DG_INLINE dgVector dgDynamicBody::PredictAngularVelocity(dgFloat32 timestep) const
{
	return m_omega + m_invWorldInertiaMatrix.RotateVector(m_externalTorque).Scale (timestep);
}


DG_INLINE dgBody::OnApplyExtForceAndTorque dgDynamicBody::GetExtForceAndTorqueCallback () const
{
	return m_applyExtForces;
}

DG_INLINE void dgDynamicBody::SetExtForceAndTorqueCallback (OnApplyExtForceAndTorque callback)
{
	m_applyExtForces = callback;
}

DG_INLINE dgSkeletonContainer* dgDynamicBody::GetSkeleton() const
{
	return m_skeleton;
}

DG_INLINE void dgDynamicBody::SetSkeleton(dgSkeletonContainer* const skeleton)
{
	dgAssert (!(m_skeleton && skeleton));
	m_skeleton = skeleton;
}


DG_INLINE const dgVector& dgDynamicBody::GetDampCoeffcient (dgFloat32 timestep)
{
	if (dgAbs(m_cachedTimeStep - timestep) > dgFloat32(1.0e-6f)) {
		m_cachedTimeStep = timestep;
		const dgFloat32 tau = dgFloat32(1.0f) / (dgFloat32(60.0f) * timestep);
		m_cachedDampCoef.m_x = dgPow(dgFloat32(1.0f) - m_dampCoef.m_x, tau);
		m_cachedDampCoef.m_y = dgPow(dgFloat32(1.0f) - m_dampCoef.m_y, tau);
		m_cachedDampCoef.m_z = dgPow(dgFloat32(1.0f) - m_dampCoef.m_z, tau);
		m_cachedDampCoef.m_w = dgPow(dgFloat32(1.0f) - m_dampCoef.m_w, tau);
	}
	return m_cachedDampCoef;
}


#endif 


