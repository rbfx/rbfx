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


#ifndef __D_ANIMATION_ACYCLIC_SOLVER_H__
#define __D_ANIMATION_ACYCLIC_SOLVER_H__

class dAnimationAcyclicJoint;
class dAnimationKinematicLoopJoint;

class dAnimationAcyclicSolver: public dContainersAlloc
{
	class dNodePair;
	class dVectorPair;
	class dMatrixData;
	class dBodyJointMatrixDataPair;

	public:
	dAnimationAcyclicSolver();
	virtual ~dAnimationAcyclicSolver();
	//void Finalize(dVehicleChassis* const vehicle);
	void Finalize(dAnimationAcyclicJoint* const rootNode);

	void Update(dFloat timestep);

	private:
	int CalculateNodeCount () const;
	void SortGraph(dAnimationAcyclicJoint* const root, int& index);

	void InitMassMatrix();
	void InitLoopMassMatrix();
	void Factorize(dAnimationAcyclicJoint* const node);
	int BuildJacobianMatrix(dFloat timestep);
	void GetJacobians(dAnimationAcyclicJoint* const node);
	void CalculateLoopMassMatrixCoefficients();
	int BuildJacobianMatrix(dFloat timestep, dComplementaritySolver::dBilateralJoint* const joint);

	void CalculateJointDiagonal(dAnimationAcyclicJoint* const node);
	void CalculateJacobianBlock(dAnimationAcyclicJoint* const node);
	void CalculateBodyDiagonal(dAnimationAcyclicJoint* const child);
	void CalculateInertiaMatrix(dAnimationAcyclicJoint* const node) const;

	void UpdateForces(const dVectorPair* const force) const;
	void CalculateJointAccel(dVectorPair* const accel) const;
	void SolveAuxiliary(dVectorPair* const force, const dVectorPair* const accel) const;
	void CalculateOpenLoopForce(dVectorPair* const force, const dVectorPair* const accel) const;

	void SolveBackward(dVectorPair* const force, const dVectorPair* const accel) const;
	void SolveForward(dVectorPair* const force, const dVectorPair* const accel, int startNode) const;

	void BodyDiagInvTimeSolution(dAnimationAcyclicJoint* const node, dVectorPair& force) const;
	void JointDiagInvTimeSolution(dAnimationAcyclicJoint* const node, dVectorPair& force) const;
	void JointJacobianTimeMassForward(dAnimationAcyclicJoint* const node, dVectorPair& force) const;
	void BodyJacobianTimeSolutionBackward(dAnimationAcyclicJoint* const node, dVectorPair& force) const;
	void BodyJacobianTimeMassForward(dAnimationAcyclicJoint* const node, const dVectorPair& force, dVectorPair& parentForce) const;
	void JointJacobianTimeSolutionBackward(dAnimationAcyclicJoint* const node, dVectorPair& force, const dVectorPair& parentForce) const;

	void DebugMassMatrix();

	dAnimationAcyclicJoint* m_rootNode;
	dAnimationAcyclicJoint** m_nodesOrder;

	// cache temporary variables
	int* m_matrixRowsIndex;
	dNodePair* m_pairs;
	dFloat* m_deltaForce;
	dFloat* m_massMatrix10;
	dFloat* m_massMatrix11;
	dBodyJointMatrixDataPair* m_data;
	dAnimationKinematicLoopJoint** m_loopJoints;
	
	dComplementaritySolver::dJacobianPair* m_leftHandSide;
	dComplementaritySolver::dJacobianColum* m_rightHandSide;
	
	int m_rowCount;
	int m_nodeCount;
	int m_maxNodeCount;
	int m_loopRowCount;
	int m_loopNodeCount;
	int m_loopJointCount;
	int m_auxiliaryRowCount;
};


#endif 

