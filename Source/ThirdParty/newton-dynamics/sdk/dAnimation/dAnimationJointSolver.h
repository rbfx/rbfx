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


#ifndef __D_ANIMATION_JOINT_SOLVER_H__
#define __D_ANIMATION_JOINT_SOLVER_H__

#include "dAnimationStdAfx.h"

class dAnimationJoint;
class dAnimationJointRoot;
class dAnimationLoopJoint;
class dAnimationContraint;

class dAnimationJointSolver: public dCustomAlloc
{
	class dNodePair;
	class dVectorPair;
	class dMatrixData;
	class dBodyJointMatrixDataPair;

	public:
	dAnimationJointSolver();
	virtual ~dAnimationJointSolver();
	void Finalize(dAnimationJointRoot* const rootNode);

	void Update(dFloat timestep);

	private:
	int CalculateNodeCount () const;
	void SortGraph(dAnimationJoint* const root, int& index);

	void InitMassMatrix();
	void InitLoopMassMatrix();
	void Factorize(dAnimationJoint* const node);
	int BuildJacobianMatrix(dFloat timestep);
	void GetJacobians(dAnimationJoint* const node);
	void CalculateLoopMassMatrixCoefficients();
	int BuildJacobianMatrix(dFloat timestep, dAnimationContraint* const joint);

	void CalculateJointDiagonal(dAnimationJoint* const node);
	void CalculateJacobianBlock(dAnimationJoint* const node);
	void CalculateBodyDiagonal(dAnimationJoint* const child);
	void CalculateInertiaMatrix(dAnimationJoint* const node) const;

	void UpdateForces(const dVectorPair* const force) const;
	void CalculateJointAccel(dVectorPair* const accel) const;
	void SolveAuxiliary(dVectorPair* const force, const dVectorPair* const accel) const;
	void CalculateOpenLoopForce(dVectorPair* const force, const dVectorPair* const accel) const;

	void SolveBackward(dVectorPair* const force, const dVectorPair* const accel) const;
	void SolveForward(dVectorPair* const force, const dVectorPair* const accel, int startNode) const;

	void BodyDiagInvTimeSolution(dAnimationJoint* const node, dVectorPair& force) const;
	void JointDiagInvTimeSolution(dAnimationJoint* const node, dVectorPair& force) const;
	void JointJacobianTimeMassForward(dAnimationJoint* const node, dVectorPair& force) const;
	void BodyJacobianTimeSolutionBackward(dAnimationJoint* const node, dVectorPair& force) const;
	void BodyJacobianTimeMassForward(dAnimationJoint* const node, const dVectorPair& force, dVectorPair& parentForce) const;
	void JointJacobianTimeSolutionBackward(dAnimationJoint* const node, dVectorPair& force, const dVectorPair& parentForce) const;

	void DebugMassMatrix();

	dAnimationJointRoot* m_rootNode;
	dAnimationJoint** m_nodesOrder;
//	dAnimationContraint** m_nodesOrder;

	// cache temporary variables
	int* m_matrixRowsIndex;
	dNodePair* m_pairs;
	dFloat* m_deltaForce;
	dFloat* m_massMatrix10;
	dFloat* m_massMatrix11;
	dBodyJointMatrixDataPair* m_data;
	dAnimationLoopJoint** m_loopJoints;
	
	dComplementaritySolver::dJacobianPair* m_leftHandSide;
	dComplementaritySolver::dJacobianColum* m_rightHandSide;
	
	int m_rowCount;
	int m_nodeCount;
	int m_maxNodeCount;
	int m_loopRowCount;
//	int m_loopNodeCount;
	int m_loopJointCount;
	int m_auxiliaryRowCount;
};


#endif 

