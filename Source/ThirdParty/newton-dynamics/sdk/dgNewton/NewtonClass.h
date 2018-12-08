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

#ifndef __NewotnClass_H__
#define __NewotnClass_H__

#include "NewtonStdAfx.h"
#include "Newton.h"

class Newton; 


class Newton: public dgWorld 
{
	public:
	DG_CLASS_ALLOCATOR(allocator)

	Newton (dgMemoryAllocator* const allocator);
	~Newton ();

	void UpdatePhysics (dgFloat32 timestep);
	void UpdatePhysicsAsync (dgFloat32 timestep);
	static void* DefaultAllocMemory (dgInt32 size);
	static void DefaultFreeMemory (void* const ptr, dgInt32 size);

	NewtonWorldDestructorCallback m_destructor;
};


class NewtonUserJoint: public dgUserConstraint  
{	
	public:
	NewtonUserJoint (dgWorld* const world, dgInt32 maxDof, NewtonUserBilateralCallback callback, dgBody* const dyn0, dgBody* const dyn1);
	~NewtonUserJoint ();

	dgUnsigned32 JacobianDerivative (dgContraintDescritor& params); 
	void Serialize (dgSerialize serializeCallback, void* const userData);

	void AddAngularRowJacobian (const dgVector& dir, dgFloat32 relAngle);
	void AddGeneralRowJacobian (const dgFloat32* const jacobian0, const dgFloat32* const jacobian1);
	void AddLinearRowJacobian (const dgVector& pivot0, const dgVector& pivot1, const dgVector& dir);

	dgInt32 GetJacobianCount () const;
	void GetJacobianAt (dgInt32 index, dgFloat32* const jacobian0, dgFloat32* const jacobian1) const;

	dgFloat32 GetRowForce (dgInt32 row) const;
	void SetHighFriction (dgFloat32 friction);
	void SetLowerFriction (dgFloat32 friction);
	void SetRowStiffness (dgFloat32 stiffness);
	void SetAcceleration (dgFloat32 acceleration);
	void SetAsInverseDynamicsRow();
	dgFloat32 GetAcceleration () const;
//	dgFloat32 GetInverseDynamicsAcceleration() const;
	dgFloat32 CalculateZeroMotorAcceleration() const;
	
	void SetSpringDamperAcceleration (dgFloat32 rowStiffness, dgFloat32 springK, dgFloat32 springD);
	void SetUpdateFeedbackFunction (NewtonUserBilateralCallback getFeedback);

	dgInt32 SubmitImmediateModeConstraint(NewtonImmediateModeConstraint* const descriptor, dFloat timestep);

	protected:
	NewtonUserJoint(NewtonUserBilateralCallback callback, dgBody* const body);

	private:
	NewtonUserBilateralCallback m_jacobianFnt;

	dgForceImpactPair* m_forceArray;
	dgContraintDescritor* m_param;
	dgInt32 m_rows;
};


class NewtonUserJointInverseDynamicsEffector : public NewtonUserJoint
{
	public:
	NewtonUserJointInverseDynamicsEffector (dgInverseDynamics* const invDynSolver, dgInverseDynamics::dgNode* const invDynNode, NewtonUserBilateralCallback callback);
	~NewtonUserJointInverseDynamicsEffector();

	private:
	dgInverseDynamics* m_invDynSolver;
};

#endif
