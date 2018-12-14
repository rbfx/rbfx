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

#ifndef __DGCONSTRAINT_H__
#define __DGCONSTRAINT_H__

#include "dgBodyMasterList.h"

#define DG_MAX_BOUND				dgFloat32 (1.0e15f)
#define DG_MIN_BOUND				(-DG_MAX_BOUND)
#define DG_INDEPENDENT_ROW			-1 
#define DG_CONSTRAINT_MAX_ROWS		(3 * 16)
#define MIN_JOINT_PIN_LENGTH		dgFloat32 (50.0f)

class dgBody;
class dgWorld;
class dgConstraint;
class dgBilateralBounds;

typedef void (dgApi *ConstraintsForceFeeback) (const dgConstraint& me, dgFloat32 timestep, dgInt32 threadIndex);

class dgConstraintInfo
{
	public:
	dgMatrix m_attachMatrix_0;
	dgMatrix m_attachMatrix_1;
	dgFloat32 m_minLinearDof[3];
	dgFloat32 m_maxLinearDof[3];
	dgFloat32 m_minAngularDof[3];
	dgFloat32 m_maxAngularDof[3];
	dgBody* m_attachBody_0;
	dgBody* m_attachBody_1;
	dgFloat32 m_extraParameters[64];
	dgInt32 m_collideCollisionOn;
	char m_discriptionType[64];
};


class dgJointCallbackParam
{
	public:
	dgFloat32 m_accel;
	dgFloat32 m_minFriction;
	dgFloat32 m_maxFriction;
	dgFloat32 m_timestep;
};


class dgForceImpactPair
{
	public:
	dgFloat32 m_force;
	dgFloat32 m_impact;
};

class dgBilateralBounds
{
	public:
	dgForceImpactPair* m_jointForce;
	dgFloat32 m_low;
	dgFloat32 m_upper;
	dgInt32 m_normalIndex;
};

DG_MSC_VECTOR_ALIGMENT
class dgJacobian
{
	public:
	dgVector m_linear;
	dgVector m_angular;
} DG_GCC_VECTOR_ALIGMENT;

DG_MSC_VECTOR_ALIGMENT
class dgJacobianPair
{
	public:
	dgJacobian m_jacobianM0;
	dgJacobian m_jacobianM1;
} DG_GCC_VECTOR_ALIGMENT;

class dgLeftHandSide;
class dgRightHandSide;

class dgJointAccelerationDecriptor
{
	public: 
	dgInt32 m_rowsCount;
	dgFloat32 m_timeStep;
	dgFloat32 m_invTimeStep;
	dgFloat32 m_firstPassCoefFlag;
	dgRightHandSide* m_rightHandSide;
	const dgLeftHandSide* m_leftHandSide;
};


DG_MSC_VECTOR_ALIGMENT
class dgContraintDescritor
{
	public:
	dgJacobianPair m_jacobian[DG_CONSTRAINT_MAX_ROWS];
	dgBilateralBounds m_forceBounds[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_jointAccel[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_jointStiffness[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_restitution[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_penetration[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_penetrationStiffness[DG_CONSTRAINT_MAX_ROWS];
	dgFloat32 m_zeroRowAcceleration[DG_CONSTRAINT_MAX_ROWS];
	dgWorld* m_world;
	dgInt32 m_threadIndex;
	dgFloat32 m_timestep;
	dgFloat32 m_invTimestep;
} DG_GCC_VECTOR_ALIGMENT;


typedef void (dgApi *OnConstraintDestroy) (dgConstraint& me);

DG_MSC_VECTOR_ALIGMENT
class dgConstraint
{
	public:
	DG_CLASS_ALLOCATOR(allocator)

	enum dgConstraintID
	{
		m_ballConstraint,
		m_hingeConstraint,
		m_sliderConstraint,
		m_contactConstraint,
		m_upVectorConstraint,
		m_universalConstraint,
		m_corkScrewConstraint,
		m_unknownConstraint
	};

	dgUnsigned32 GetId () const;
	dgBody* GetBody0 ()	const;
	dgBody* GetBody1 ()	const;

	void SetBodies (dgBody* const body0, dgBody* const body1);

	dgBodyMasterListRow::dgListNode* GetLink0()	const;
	dgBodyMasterListRow::dgListNode* GetLink1()	const;
	void* GetUserData () const;

	bool IsActive() const;
	bool IsCollidable () const;
	bool IsBilateral () const;
	bool IsSkeleton () const;
	bool IsSkeletonLoop () const;

	virtual void ResetMaxDOF();
	dgInt32 GetMaxDOF() const;
	
	void SetUserData (void *userData);
	void SetCollidable (bool state);
	virtual void SetDestructorCallback (OnConstraintDestroy destructor) = 0;

	virtual dgFloat32 GetStiffness() const;
	virtual void SetStiffness(dgFloat32 stiffness);
	
	virtual dgInt32 GetSolverModel() const;
	virtual void SetSolverModel(dgInt32 model);

	virtual void GetInfo (dgConstraintInfo* const info) const;

	ConstraintsForceFeeback GetUpdateFeedbackFunction ();
	virtual void JointAccelerations(dgJointAccelerationDecriptor* const params) = 0; 

	class dgPointParam
	{
		public:
		dgVector m_r0;
		dgVector m_r1;
		dgVector m_posit0;
		dgVector m_posit1;
		dgVector m_veloc0;
		dgVector m_veloc1;
		//dgVector m_centripetal0;
		//dgVector m_centripetal1;
		dgFloat32 m_stiffness;
	};

	protected:
	dgConstraint();
	virtual ~dgConstraint();

	virtual void ResetInverseDynamics() = 0;
	virtual dgUnsigned32 JacobianDerivative (dgContraintDescritor& params) = 0; 
	
	void SetUpdateFeedbackFunction (ConstraintsForceFeeback function);
	void InitPointParam (dgPointParam& param, dgFloat32 stiffness, const dgVector& p0Global, const dgVector& p1Global) const;
	void InitInfo (dgConstraintInfo* const info) const;

	void* m_userData;
	dgBody* m_body0;
	dgBody* m_body1;
	dgBodyMasterListRow::dgListNode* m_link0;
	dgBodyMasterListRow::dgListNode* m_link1;
	ConstraintsForceFeeback m_updaFeedbackCallback;
	dgInt32 m_clusterLRU;
	dgUnsigned32 m_index;
	dgUnsigned32 m_dynamicsLru;
	dgUnsigned32 m_maxDOF				: 6;
	dgUnsigned32 m_constId				: 6;		
	dgUnsigned32 m_solverModel			: 2;
	dgUnsigned32 m_enableCollision		: 1;
	dgUnsigned32 m_contactActive		: 1;
	dgUnsigned32 m_isBilateral			: 1;
	dgUnsigned32 m_graphTagged			: 1;
	dgUnsigned32 m_isInSkeleton			: 1;
	dgUnsigned32 m_isInSkeletonLoop		: 1;
	
	friend class dgWorld;
	friend class dgJacobianMemory;
	friend class dgBodyMasterList;
	friend class dgInverseDynamics;
	friend class dgSkeletonContainer;
	friend class dgWorldDynamicUpdate;
	friend class dgParallelBodySolver;
	friend class dgParallelSolverJointAcceleration;
	friend class dgParallelSolverInitFeedbackUpdate;
	friend class dgParallelSolverBuildJacobianMatrix;
	friend class dgBroadPhaseMaterialCallbackWorkerThread;
} DG_GCC_VECTOR_ALIGMENT;

DG_INLINE dgConstraint::dgConstraint() 
	:m_userData(NULL)
	,m_body0(NULL)
	,m_body1(NULL)
	,m_link0(NULL)
	,m_link1(NULL)
	,m_updaFeedbackCallback(NULL)
	,m_clusterLRU(-1)
	,m_index(0)
	,m_dynamicsLru(0)
	,m_maxDOF(6)
	,m_constId(m_unknownConstraint)
	,m_solverModel(2)
	,m_enableCollision(false)
	,m_contactActive(false)
	,m_isBilateral(false)
	,m_graphTagged(false)
	,m_isInSkeleton(false)
	,m_isInSkeletonLoop(false)
{
	dgAssert ((((dgUnsigned64) this) & 15) == 0);
}

DG_INLINE dgConstraint::~dgConstraint()
{
}

DG_INLINE ConstraintsForceFeeback dgConstraint::GetUpdateFeedbackFunction ()
{
	return m_updaFeedbackCallback;
}

DG_INLINE void dgConstraint::SetUpdateFeedbackFunction (ConstraintsForceFeeback function)
{
	m_updaFeedbackCallback = function;
}

DG_INLINE bool dgConstraint::IsBilateral() const
{
	return m_isBilateral ? true : false;
}

DG_INLINE bool dgConstraint::IsSkeleton () const
{
	return m_isInSkeleton ? true : false;
}

DG_INLINE bool dgConstraint::IsSkeletonLoop () const
{
	return m_isInSkeletonLoop ? true : false;
}

DG_INLINE bool dgConstraint::IsCollidable () const
{
	return m_enableCollision ? true : false;
}

DG_INLINE void dgConstraint::SetCollidable (bool state)
{
	m_enableCollision = dgUnsigned32 (state);
}

DG_INLINE dgUnsigned32 dgConstraint::GetId () const
{
	return m_constId;
}

DG_INLINE dgBody* dgConstraint::GetBody0 () const
{
	return m_body0;
}

DG_INLINE dgBody* dgConstraint::GetBody1 () const
{
	return m_body1;
}

DG_INLINE void dgConstraint::SetBodies (dgBody* const body0, dgBody* const body1)
{
	m_body0 = body0;
	m_body1 = body1;
}


DG_INLINE dgBodyMasterListRow::dgListNode* dgConstraint::GetLink0()	const
{
	return m_link0;
}
DG_INLINE dgBodyMasterListRow::dgListNode* dgConstraint::GetLink1()	const
{
	return m_link1;
}


DG_INLINE dgFloat32 dgConstraint::GetStiffness() const
{
	return dgFloat32 (1.0f);
}

DG_INLINE void dgConstraint::SetStiffness(dgFloat32 stiffness)
{
}

DG_INLINE dgInt32 dgConstraint::GetSolverModel() const
{
	return m_solverModel;
}

DG_INLINE void dgConstraint::SetSolverModel(dgInt32 model)
{
	m_solverModel = dgClamp(model, 0, 2);
}

DG_INLINE void dgConstraint::ResetMaxDOF()
{
}

DG_INLINE dgInt32 dgConstraint::GetMaxDOF() const
{
	return dgInt32 (m_maxDOF);
}

DG_INLINE bool dgConstraint::IsActive() const
{
	return m_contactActive ? true : false;
}


#endif 

