/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __D_ANIMATION_JOINT_RAGDOLL_H__
#define __D_ANIMATION_JOINT_RAGDOLL_H__

#include "dAnimationStdAfx.h"
#include "dAnimationJoint.h"

//class dAnimIDRigLimb;
//class dAnimIDRigJoint;
//class dAnimIDRigKinematicLoopJoint;
/*
class dAnimAcyclicJoint: public dContainersAlloc
{
	public:
	dAnimAcyclicJoint(dAnimAcyclicJoint* const parent);
	virtual ~dAnimAcyclicJoint();

	void* GetUserData();
	void SetUserData(void* const userData);
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	NewtonWorld* GetWorld() const { return m_world; }
	void SetWorld(NewtonWorld* const world) { m_world = world; }

	dAnimAcyclicJoint* GetParent() const { return m_parent; }
	dList<dAnimAcyclicJoint*>& GetChildren() { return m_children;} 
	const dList<dAnimAcyclicJoint*>& GetChildren() const { return m_children; }

	int GetIndex() const { return m_solverIndex; }
	void SetIndex(int index) { m_solverIndex = index; }

	bool IsLoopNode() const { return m_isLoop; }
	void SetLoopNode(bool staste) { m_isLoop = staste; }

	dComplementaritySolver::dBodyState* GetProxyBody() { return &m_proxyBody; }
	const dComplementaritySolver::dBodyState* GetProxyBody() const { return &m_proxyBody; }
	virtual dComplementaritySolver::dBilateralJoint* GetProxyJoint() { return m_proxyJoint; }

	virtual void ApplyExternalForce(dFloat timestep);
	virtual int GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray);

	virtual dAnimIDRigLimb* GetAsRigLimb() {return NULL;}
	virtual dAnimIDRigJoint* GetAsRigJoint() {return NULL;}
	virtual dAnimAcyclicJoint* GetAsAcyclicJoint() { return this; }

	protected:
	virtual void Finalize();
	virtual void UpdateJointAcceleration();

	dComplementaritySolver::dBodyState m_proxyBody;
	dComplementaritySolver::dBilateralJoint* m_proxyJoint;
	dAnimAcyclicJoint* m_parent;
	void* m_userData;
	NewtonWorld* m_world;
	int m_solverIndex;
	bool m_isLoop;
	dList<dAnimAcyclicJoint*> m_children;

	friend class dAnimAcyclicSolver;
};
*/

class dAnimationJointRagdoll: public dAnimationJoint, public dAnimationContraint
{
	class dRagDollMotor;
	public:
	dAnimationJointRagdoll(const dMatrix& pinAndPivotInGlobalSpace, NewtonBody* const body, const dMatrix& bindMarix, dAnimationJoint* const parent);
	virtual ~dAnimationJointRagdoll();
	protected:

	virtual void RigidBodyToStates();

//	virtual void Init(dBodyState* const state0, dBodyState* const state1);

	//protected:
	virtual void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	virtual void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const;
//	virtual void JointAccelerations(dJointAccelerationDecriptor* const accelParam);

	dComplementaritySolver::dJacobian m_jacobial01[3];
	dComplementaritySolver::dJacobian m_jacobial10[3];
	dVector m_rowAccel;

};



#endif

