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


#ifndef __D_VEHICLE_SOLVER_H__
#define __D_VEHICLE_SOLVER_H__

#include "dStdafxVehicle.h"


class dVehicleNode;
class dVehicleChassis;
class dKinematicLoopJoint;

class dVehicleSolver: public dContainersAlloc
{
	class dNodePair;
	class dVectorPair;
	class dMatrixData;
	class dBodyJointMatrixDataPair;

	public:
	DVEHICLE_API dVehicleSolver();
	DVEHICLE_API virtual ~dVehicleSolver();
	DVEHICLE_API void Finalize(dVehicleChassis* const vehicle);

	void Update(dFloat timestep);

	private:
	int CalculateNodeCount () const;
	void SortGraph(dVehicleNode* const root, int& index);

	void InitMassMatrix();
	void InitLoopMassMatrix();
	void Factorize(dVehicleNode* const node);
	int BuildJacobianMatrix(dFloat timestep);
	void GetJacobians(dVehicleNode* const node);
	void CalculateLoopMassMatrixCoefficients(dFloat* const diagDamp);
	int BuildJacobianMatrix(dFloat timestep, dComplementaritySolver::dBilateralJoint* const joint);

	void CalculateJointDiagonal(dVehicleNode* const node);
	void CalculateJacobianBlock(dVehicleNode* const node);
	void CalculateBodyDiagonal(dVehicleNode* const child);
	void CalculateInertiaMatrix(dVehicleNode* const node) const;

	void UpdateForces(const dVectorPair* const force) const;
	void CalculateJointAccel(dVectorPair* const accel) const;
	void SolveAuxiliary(dVectorPair* const force, const dVectorPair* const accel) const;
	void CalculateForce(dVectorPair* const force, const dVectorPair* const accel) const;

	void SolveBackward(dVectorPair* const force, const dVectorPair* const accel) const;
	void SolveForward(dVectorPair* const force, const dVectorPair* const accel, int startNode) const;

	void BodyDiagInvTimeSolution(dVehicleNode* const node, dVectorPair& force) const;
	void JointDiagInvTimeSolution(dVehicleNode* const node, dVectorPair& force) const;
	void JointJacobianTimeMassForward(dVehicleNode* const node, dVectorPair& force) const;
	void BodyJacobianTimeSolutionBackward(dVehicleNode* const node, dVectorPair& force) const;
	void BodyJacobianTimeMassForward(dVehicleNode* const node, const dVectorPair& force, dVectorPair& parentForce) const;
	void JointJacobianTimeSolutionBackward(dVehicleNode* const node, dVectorPair& force, const dVectorPair& parentForce) const;

	void DebugMassMatrix();

	dVehicleChassis* m_vehicle;
	dVehicleNode** m_nodesOrder;

	// cache temporary variables
	int* m_matrixRowsIndex;
	dNodePair* m_pairs;
	dFloat* m_deltaForce;
	dFloat* m_massMatrix10;
	dFloat* m_massMatrix11;
	dBodyJointMatrixDataPair* m_data;
	dKinematicLoopJoint** m_loopJoints;	
	
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

