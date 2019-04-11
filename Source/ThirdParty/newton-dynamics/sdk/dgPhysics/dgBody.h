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

#ifndef _DG_BODY_H_
#define _DG_BODY_H_

#include "dgPhysicsStdafx.h"
#include "dgCollision.h"
#include "dgBodyMasterList.h"

class dgLink;
class dgBody;  
class dgWorld;  
class dgCollision;
class dgBroadPhaseNode;
class dgSkeletonContainer;
class dgCollisionInstance;
class dgBroadPhaseBodyNode;
class dgBroadPhaseAggregate;


#define DG_MINIMUM_MASS		dgFloat32(1.0e-5f)
#define DG_INFINITE_MASS	dgFloat32(1.0e15f)

#define OverlapTest(body0,body1) dgOverlapTest ((body0)->m_minAABB, (body0)->m_maxAABB, (body1)->m_minAABB, (body1)->m_maxAABB)



DG_MSC_VECTOR_ALIGMENT
struct dgLineBox
{
	dgVector m_l0;
	dgVector m_l1;
	dgVector m_boxL0;
	dgVector m_boxL1;
} DG_GCC_VECTOR_ALIGMENT;


DG_MSC_VECTOR_ALIGMENT
class dgBody  
{
	public:
	typedef void (dgApi *OnBodyDestroy) (dgBody& me);
	typedef void (dgApi *OnApplyExtForceAndTorque) (dgBody& me, dgFloat32 timestep, dgInt32 threadIndex);
	typedef void (dgApi *OnMatrixUpdateCallback) (const dgBody& me, const dgMatrix& matrix, dgInt32 threadIndex);

	enum dgRTTI
	{
		m_baseBodyRTTI = 1<<0,
		m_dynamicBodyRTTI = 1<<1,
		m_kinematicBodyRTTI = 1<<2,
		m_dynamicBodyAsymentricRTTI = 1 << 3,
		//m_deformableBodyRTTI = 1<<3,
	};

	enum dgType
	{
		m_dynamicBody = 0,
		m_kinematicBody,
		m_dynamicBodyAsymatric,
		//m_deformableBody,
	};

	class dgSetInfo
	{
		public:
		DG_INLINE dgSetInfo()
		{
		}

		DG_INLINE void Init(dgBody* const self)
		{
			m_parent = self;
			m_rank = 0;
			m_bodyCount = 1;
			m_jointCount = 0;
			m_rowCount = 0;
		}

		dgBody* m_parent;
		dgInt32 m_rank;
		dgInt32 m_bodyCount;
		dgInt32 m_jointCount;
		dgInt32 m_rowCount;
	};

	DG_CLASS_ALLOCATOR(allocator)

	dgBody();
	dgBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber);
	virtual ~dgBody();

	dgType GetType () const;
	dgInt32 IsRTTIType (dgUnsigned32 rtti) const;

	void* GetUserData() const;
	void SetUserData (void* const userData);
	dgWorld* GetWorld() const;

	const dgMatrix& GetMatrix() const;
	const dgQuaternion& GetRotation() const;
	const dgVector& GetPosition() const;
	
	void SetCentreOfMass (const dgVector& com);
	const dgVector& GetCentreOfMass () const;

	void GetAABB (dgVector &p0, dgVector &p1) const;	

	const dgVector& GetOmega() const;
	const dgVector& GetVelocity() const;

	dgVector GetVelocityAtPoint (const dgVector& point) const;

	void SetOmega (const dgVector& omega);
	void SetVelocity (const dgVector& velocity);

	void SetOmegaNoSleep(const dgVector& omega);
	void SetVelocityNoSleep(const dgVector& velocity);

	dgUnsigned32 GetGroupID () const;
	virtual void SetGroupID (dgUnsigned32 id);
	dgInt32 GetUniqueID () const;

	bool GetContinueCollisionMode () const;
	void SetContinueCollisionMode (bool mode);
	bool GetCollisionWithLinkedBodies () const;
	void SetCollisionWithLinkedBodies (bool state);

	void Freeze ();
	void Unfreeze ();
	bool GetFreeze () const;
	void SetFreeze (bool state);

	bool GetSleepState () const;
	void SetSleepState (bool state);

	bool GetAutoSleep () const;
	void SetAutoSleep (bool state);

	bool GetGyroMode() const;
	void SetGyroMode(bool state);
	
	dgCollisionInstance* GetCollision () const;
	dgBodyMasterList::dgListNode* GetMasterList() const;

	OnBodyDestroy GetDestructorCallback () const;
	void SetDestructorCallback (OnBodyDestroy destructor);
	OnMatrixUpdateCallback GetMatrixUpdateCallback () const;
	void SetMatrixUpdateCallback (OnMatrixUpdateCallback callback);

	dgVector GetMass() const;
	dgVector GetInvMass() const;
	void SetInvMass(const dgVector& invMass);
	const dgMatrix& GetInvInertiaMatrix () const;

	bool IsCollidable() const;
	void UpdateCollisionMatrix(dgFloat32 timestep, dgInt32 threadIndex);

	virtual dgMatrix CalculateInertiaMatrix () const;
	virtual dgMatrix CalculateInvInertiaMatrix () const;
	virtual dgMatrix CalculateLocalInertiaMatrix () const;

	virtual dgVector GetAlpha() const;
	virtual dgVector GetAccel() const;

	virtual void SetAlpha(const dgVector& alpha);
	virtual void SetAccel(const dgVector& accel);

	virtual void SetCollidable (bool state) = 0;
	virtual bool IsInEquilibrium () const = 0;

	virtual const dgVector& GetForce() const = 0;
	virtual const dgVector& GetTorque() const = 0;
	virtual void AddForce (const dgVector& force) = 0;
	virtual void AddTorque (const dgVector& torque) = 0;
	virtual void SetForce (const dgVector& force) = 0;
	virtual void SetTorque (const dgVector& torque) = 0;

	virtual dgFloat32 GetLinearDamping () const = 0;
	virtual dgVector GetAngularDamping () const = 0;
	virtual void SetLinearDamping (dgFloat32 linearDamp) = 0;
	virtual void SetAngularDamping (const dgVector& angularDamp) = 0;
	virtual void AddDampingAcceleration(dgFloat32 timestep) = 0;

	virtual void SetMassMatrix (dgFloat32 mass, const dgMatrix& inertia);
	virtual void SetMassProperties (dgFloat32 mass, const dgCollisionInstance* const collision);

	virtual dgVector PredictLinearVelocity(dgFloat32 timestep) const = 0;
	virtual dgVector PredictAngularVelocity(dgFloat32 timestep) const = 0;
	virtual void InvalidateCache();

	virtual void SetMatrix(const dgMatrix& matrix);
	virtual void SetMatrixResetSleep(const dgMatrix& matrix);
	virtual void SetMatrixNoSleep(const dgMatrix& matrix);
	virtual void IntegrateVelocity (dgFloat32 timestep);
	virtual void AttachCollision (dgCollisionInstance* const collision);
    
	virtual void ApplyExtenalForces (dgFloat32 timestep, dgInt32 threadIndex) = 0;		
	virtual OnApplyExtForceAndTorque GetExtForceAndTorqueCallback () const = 0;
	virtual void SetExtForceAndTorqueCallback (OnApplyExtForceAndTorque callback) = 0;
	
	virtual dgFloat32 RayCast (const dgLineBox& line, OnRayCastAction filter, OnRayPrecastAction preFilter, void* const userData, dgFloat32 minT) const;
	virtual void Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData);
	
	virtual dgConstraint* GetFirstJoint() const;
	virtual dgConstraint* GetNextJoint(dgConstraint* const joint) const;
	virtual dgConstraint* GetFirstContact() const;
	virtual dgConstraint* GetNextContact(dgConstraint* const joint) const;
	virtual dgVector CalculateInverseDynamicForce (const dgVector& desiredVeloc, dgFloat32 timestep) const;
	virtual dgSkeletonContainer* GetSkeleton() const;

	void SetMatrixOriginAndRotation(const dgMatrix& matrix);

	dgBroadPhaseBodyNode* GetBroadPhase () const;
	void SetBroadPhase(dgBroadPhaseBodyNode* const node);
	dgBroadPhaseAggregate* GetBroadPhaseAggregate() const;
	void SetBroadPhaseAggregate(dgBroadPhaseAggregate* const aggregate);

	void AddImpulse (const dgVector& pointVeloc, const dgVector& pointPosit, dgFloat32 timestep);
	void ApplyImpulsePair (const dgVector& linearImpulse, const dgVector& angularImpulse, dgFloat32 timestep);
	void ApplyImpulsesAtPoint (dgInt32 count, dgInt32 strideInBytes, const dgFloat32* const impulseArray, const dgFloat32* const pointArray, dgFloat32 timestep);

	dgInt32 GetSerializedID() const;
	void CalcInvInertiaMatrix();

	void InitJointSet ();

	void SetCollision(dgCollisionInstance* const collision);

	protected:
	void UpdateWorlCollisionMatrix() const;
	void UpdateLumpedMatrix();

	dgVector CalculateLinearMomentum() const; 
	dgVector CalculateAngularMomentum() const; 

	dgMatrix m_matrix;
	dgQuaternion m_rotation;
	dgMatrix m_invWorldInertiaMatrix;
	dgVector m_mass;
	dgVector m_invMass;
	dgVector m_veloc;
	dgVector m_omega;
	dgVector m_accel;
	dgVector m_alpha;
	dgVector m_minAABB;
	dgVector m_maxAABB;
	dgVector m_localCentreOfMass;	
	dgVector m_globalCentreOfMass;	
	dgVector m_impulseForce;	
	dgVector m_impulseTorque;	
	dgVector m_gyroTorque;
	dgQuaternion m_gyroRotation;

	mutable dgInt32 m_criticalSectionLock;
	union 
	{
		dgUnsigned32 m_flags;
		struct {
			dgUnsigned32 m_freeze					: 1;
			dgUnsigned32 m_resting					: 1;
			dgUnsigned32 m_sleeping					: 1;
			dgUnsigned32 m_autoSleep				: 1;
			dgUnsigned32 m_inCallback				: 1;
			dgUnsigned32 m_jointSet					: 1;
			dgUnsigned32 m_collidable				: 1;
			dgUnsigned32 m_equilibrium				: 1;
			dgUnsigned32 m_spawnnedFromCallback		: 1;
			dgUnsigned32 m_continueCollisionMode	: 1;
			dgUnsigned32 m_collideWithLinkedBodies	: 1;
			dgUnsigned32 m_transformIsDirty			: 1;
			dgUnsigned32 m_gyroTorqueOn				: 1;
		};
	};

	void* m_userData;
	dgWorld* m_world;
	dgCollisionInstance* m_collision;
	dgBroadPhaseBodyNode* m_broadPhaseNode;
	dgBodyMasterList::dgListNode* m_masterNode;
	dgBroadPhaseAggregate* m_broadPhaseaggregateNode;
	OnBodyDestroy m_destructor;
	OnMatrixUpdateCallback m_matrixUpdate;

	dgSetInfo m_disjointInfo;
	dgInt32 m_index;
	dgInt32 m_uniqueID;
	dgInt32 m_bodyGroupId;
	dgInt32 m_rtti;
	dgInt32 m_type;
	dgInt32 m_serializedEnum;
	dgUnsigned32 m_dynamicsLru;
	dgUnsigned32 m_genericLRUMark;

	friend class dgWorld;
	friend class dgSolver;
	friend class dgContact;
	friend class dgConstraint;	
	friend class dgBroadPhase;
	friend class dgCollisionBVH;
	friend class dgBroadPhaseNode;
	friend class dgBodyMasterList;
	friend class dgCollisionScene;
	friend class dgCollisionConvex;
	friend class dgInverseDynamics;
	friend class dgCollisionCompound;
	friend class dgCollisionUserMesh;
	friend class dgBodyMasterListRow;
	friend class dgParallelBodySolver;
	friend class dgWorldDynamicUpdate;
	friend class dgBroadPhaseBodyNode;
	friend class dgBilateralConstraint;
	friend class dgBroadPhaseAggregate;
	friend class dgCollisionConvexPolygon;
	friend class dgCollidingPairCollector;
	friend class dgCollisionLumpedMassParticles;

} DG_GCC_VECTOR_ALIGMENT;

// *****************************************************************************
// 
//	 Implementation	
// 
// *****************************************************************************
DG_INLINE const dgMatrix& dgBody::GetInvInertiaMatrix () const 
{
	return m_invWorldInertiaMatrix;
}

DG_INLINE dgVector dgBody::GetInvMass() const
{
	return m_invMass;
}

DG_INLINE void dgBody::SetInvMass(const dgVector& invMass)
{
	m_invMass = invMass;
}

DG_INLINE dgVector dgBody::GetMass() const
{
	return m_mass;
}


DG_INLINE dgBody::dgType dgBody::GetType () const
{
	return dgType (m_type);
}

DG_INLINE dgInt32 dgBody::IsRTTIType (dgUnsigned32 rtti) const
{
	return rtti & m_rtti;
}

DG_INLINE void dgBody::SetUserData(void* const userData)
{
	m_userData = userData;
}

DG_INLINE void* dgBody::GetUserData() const
{
	return m_userData;
}

DG_INLINE dgWorld* dgBody::GetWorld() const
{
	return m_world;
}


DG_INLINE dgUnsigned32 dgBody::GetGroupID () const
{
	return dgUnsigned32 (m_bodyGroupId);
}

DG_INLINE void dgBody::SetGroupID (dgUnsigned32 id)
{
	m_bodyGroupId = dgInt32 (id);
}



DG_INLINE dgBodyMasterList::dgListNode* dgBody::GetMasterList() const
{
	return m_masterNode;
}

DG_INLINE void dgBody::GetAABB (dgVector &p0, dgVector &p1) const
{
	p0.m_x = m_minAABB.m_x;
	p0.m_y = m_minAABB.m_y;
	p0.m_z = m_minAABB.m_z;
	p1.m_x = m_maxAABB.m_x;
	p1.m_y = m_maxAABB.m_y;
	p1.m_z = m_maxAABB.m_z;
}

DG_INLINE const dgVector& dgBody::GetOmega() const
{
	return m_omega;
}

DG_INLINE const dgVector& dgBody::GetVelocity() const
{
	return m_veloc; 
}

DG_INLINE void dgBody::SetOmegaNoSleep(const dgVector& omega)
{
	m_omega = omega;
}

DG_INLINE void dgBody::SetOmega (const dgVector& omega)
{
	SetOmegaNoSleep(omega);
	m_equilibrium = false;
}

DG_INLINE dgVector dgBody::GetVelocityAtPoint (const dgVector& point) const
{
	return m_veloc + m_omega.CrossProduct(point - m_globalCentreOfMass);
}

DG_INLINE void dgBody::SetVelocityNoSleep(const dgVector& velocity)
{
	m_veloc = velocity;
}

DG_INLINE void dgBody::SetVelocity (const dgVector& velocity)
{
	SetVelocityNoSleep(velocity);
	m_equilibrium = false;
}

DG_INLINE const dgMatrix& dgBody::GetMatrix() const
{
	return m_matrix;
}

DG_INLINE const dgVector& dgBody::GetPosition() const
{
	return m_matrix.m_posit;
}

DG_INLINE const dgQuaternion& dgBody::GetRotation() const
{
	return m_rotation;
}

DG_INLINE const dgVector& dgBody::GetCentreOfMass () const
{
	return m_localCentreOfMass;
}

DG_INLINE void dgBody::SetCentreOfMass (const dgVector& com)
{
	m_localCentreOfMass.m_x = com.m_x;
	m_localCentreOfMass.m_y = com.m_y;
	m_localCentreOfMass.m_z = com.m_z;
	m_localCentreOfMass.m_w = dgFloat32 (1.0f);
	m_globalCentreOfMass = m_matrix.TransformVector (m_localCentreOfMass);
}


DG_INLINE dgInt32 dgBody::GetUniqueID () const 
{
	return m_uniqueID;
}

DG_INLINE void dgBody::SetDestructorCallback (OnBodyDestroy destructor)
{
	m_destructor = destructor;
}

DG_INLINE dgBody::OnBodyDestroy dgBody::GetDestructorCallback () const
{
	return m_destructor;
}

DG_INLINE void dgBody::SetMatrixUpdateCallback (OnMatrixUpdateCallback callback)
{
	m_matrixUpdate = callback;
}

DG_INLINE dgBody::OnMatrixUpdateCallback dgBody::GetMatrixUpdateCallback () const
{
	return m_matrixUpdate;
}

DG_INLINE dgCollisionInstance* dgBody::GetCollision () const
{
	return m_collision;
}

DG_INLINE void dgBody::SetContinueCollisionMode (bool mode)
{
	m_continueCollisionMode = dgUnsigned32 (mode);
}

DG_INLINE bool dgBody::GetContinueCollisionMode () const
{
	return m_continueCollisionMode;
}

DG_INLINE void dgBody::SetCollisionWithLinkedBodies (bool state)
{
	m_collideWithLinkedBodies = dgUnsigned32 (state);
}

DG_INLINE bool dgBody::GetCollisionWithLinkedBodies () const
{
	return m_collideWithLinkedBodies;
}

DG_INLINE bool dgBody::GetFreeze () const
{
	return m_freeze;
}


DG_INLINE void dgBody::SetAutoSleep (bool state)
{
	SetSleepState(false);
	m_autoSleep = dgUnsigned32 (state);
}

DG_INLINE bool dgBody::GetAutoSleep () const
{
	return m_autoSleep;
}

DG_INLINE bool dgBody::GetSleepState () const
{
//	return m_equilibrium;
	return m_sleeping;
}

DG_INLINE bool dgBody::GetGyroMode() const
{
	return m_gyroTorqueOn;
}

DG_INLINE void dgBody::SetGyroMode(bool state)
{
	m_gyroTorqueOn = state;
}

DG_INLINE bool dgBody::IsCollidable() const
{
	return m_collidable;
}


DG_INLINE void dgBody::SetMatrixOriginAndRotation(const dgMatrix& matrix)
{
	m_matrix = matrix;
	dgAssert (m_matrix.TestOrthogonal(dgFloat32 (1.0e-4f)));

	m_rotation = dgQuaternion (m_matrix);
	m_globalCentreOfMass = m_matrix.TransformVector (m_localCentreOfMass);
	UpdateLumpedMatrix();
}


DG_INLINE void dgBody::SetBroadPhase(dgBroadPhaseBodyNode* const node)
{
	m_broadPhaseNode = node;
}

DG_INLINE dgBroadPhaseBodyNode* dgBody::GetBroadPhase() const
{
	return m_broadPhaseNode;
}

DG_INLINE dgBroadPhaseAggregate* dgBody::GetBroadPhaseAggregate() const
{
	return m_broadPhaseaggregateNode;
}

DG_INLINE void dgBody::SetBroadPhaseAggregate(dgBroadPhaseAggregate* const aggregate)
{
	m_broadPhaseaggregateNode = aggregate;
}

DG_INLINE void dgBody::CalcInvInertiaMatrix ()
{
	dgAssert (m_invWorldInertiaMatrix[0][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[1][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[2][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[3][3] == dgFloat32 (1.0f));

	m_invWorldInertiaMatrix = CalculateInvInertiaMatrix ();

	dgAssert (m_invWorldInertiaMatrix[0][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[1][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[2][3] == dgFloat32 (0.0f));
	dgAssert (m_invWorldInertiaMatrix[3][3] == dgFloat32 (1.0f));
}

DG_INLINE dgVector dgBody::CalculateLinearMomentum() const
{
	return dgVector (m_veloc.Scale (m_mass.m_w));
}

DG_INLINE dgVector dgBody::CalculateAngularMomentum() const
{
	dgVector localOmega(m_matrix.UnrotateVector(m_omega));
	dgVector localAngularMomentum(m_mass * localOmega);
	return m_matrix.RotateVector(localAngularMomentum);
}

DG_INLINE dgSkeletonContainer* dgBody::GetSkeleton() const
{
	return NULL;
}

DG_INLINE dgInt32 dgBody::GetSerializedID() const
{
	return m_serializedEnum;
}

DG_INLINE void dgBody::InitJointSet ()
{
	m_index = -1;
	m_jointSet = 1;
	m_disjointInfo.Init (this);
}

DG_INLINE void dgBody::SetCollision(dgCollisionInstance* const collision)
{
	m_collision = collision;
}

#endif 

