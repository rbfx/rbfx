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

#ifndef _DG_INVERESE_DYNAMINS_H_
#define _DG_INVERESE_DYNAMINS_H_

#include "dgConstraint.h"

class dgDynamicBody;

class dgInverseDynamics
{
	public:
	class dgNode;
	class dgNodePair;
	class dgForcePair;
	class dgMatriData;
	class dgJointInfo;
	class dgBodyJointMatrixDataPair;
	class dgLoopingJoint
	{
		public: 
		dgLoopingJoint(dgBilateralConstraint* const joint, dgInt32 index0, dgInt32 index1, dgInt32 infoIndex, dgInt32 isEffector)
			:m_joint(joint)
			,m_m0(dgInt16(index0))
			,m_m1(dgInt16(index1))
			,m_infoIndex(dgInt16 (infoIndex))
			,m_isEffector(dgInt16 (isEffector))
		{
		}

		dgBilateralConstraint* m_joint;
		dgInt16 m_m0;
		dgInt16 m_m1;
		dgInt16 m_infoIndex;
		dgInt16 m_isEffector;
	};

	DG_CLASS_ALLOCATOR(allocator)
	dgInverseDynamics(dgWorld* const world);
	~dgInverseDynamics();

	dgWorld* GetWorld() const; 
	dgInt32 GetJointCount () const {return m_nodeCount - 1;}

	void Finalize ();
	
	dgNode* GetRoot () const;
	dgNode* AddRoot(dgDynamicBody* const rootBody);
	dgNode* AddChild (dgBilateralConstraint* const joint, dgNode* const parent);

	bool AddEffector (dgBilateralConstraint* const joint);
	bool AddLoopJoint (dgBilateralConstraint* const joint);
	void RemoveLoopJoint (dgBilateralConstraint* const joint);

	dgDynamicBody* GetBody(dgNode* const node) const;
	dgBilateralConstraint* GetJoint(dgNode* const node) const;
	dgNode* GetParent (dgNode* const node) const;
	dgNode* GetFirstChild (dgNode* const parent) const;
	dgNode* GetNextSiblingChild (dgNode* const sibling) const;

	void Update (dgFloat32 timestep, dgInt32 threadIndex);
	
	private:
	bool SanityCheck(const dgForcePair* const force, const dgForcePair* const accel) const;
	
	DG_INLINE void CalculateOpenLoopForce (dgForcePair* const force, const dgForcePair* const accel) const;
	DG_INLINE dgInt32 GetJacobianDerivatives (dgBilateralConstraint* const constraint, dgContraintDescritor& constraintParams) const;
	DG_INLINE void CalculateJointAccel(dgJointInfo* const jointInfoArray, dgRightHandSide* const rightHandSide, dgForcePair* const accel) const;
	DG_INLINE void CalculateRowJacobianDerivatives(dgInt32 index, const dgVector& invMass0, const dgVector& invMass1, const dgMatrix& invInertia0, const dgMatrix& invInertia1, const dgContraintDescritor& constraintParams, dgLeftHandSide* const row, dgRightHandSide* const rightHandSide, int isIkRow) const;

	dgNode* FindNode(dgDynamicBody* const node) const;
	void SortGraph(dgNode* const root, dgInt32& index);
		
	void InitMassMatrix (const dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgInt8* const memoryBuffer);

	dgInt32 GetMemoryBufferSizeInBytes (const dgJointInfo* const jointInfoArray, const dgRightHandSide* const rightHandSide) const;

	dgInt32 GetJacobianDerivatives(dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, dgFloat32 timestep, dgInt32 threadIndex) const;
	void CalculateInternalForces (dgJacobian* const externalForces, const dgJointInfo* const jointInfoArray, const dgLeftHandSide* const matrixRow, dgRightHandSide* const rightHandSide, const dgForcePair* const force) const;
	void CalculateCloseLoopsForces(dgJacobian* const externalForces, const dgJointInfo* const jointInfoArray, dgRightHandSide* const rightHandSide, const dgForcePair* const accel, dgForcePair* const force) const;
	void CalculateMotorsAccelerations (const dgJacobian* const externalForces, const dgJointInfo* const jointInfoArray, dgLeftHandSide* const matrixRow, dgFloat32 timestep) const;
	void RemoveLoopJoint(dgList<dgLoopingJoint>::dgListNode* const node);
	dgList<dgLoopingJoint>::dgListNode* FindLoopJointNode(dgBilateralConstraint* const joint) const;

	dgWorld* m_world;
	dgNode* m_skeleton;
	dgNode** m_nodesOrder;
	dgNodePair* m_pairs;
	dgFloat32* m_deltaForce;
	dgFloat32* m_massMatrix11;
	dgFloat32* m_massMatrix10;
//	dgFloat32* m_lowerTriangularMassMatrix11;
	dgRightHandSide** m_righHandSize;
	dgLeftHandSide** m_rowArray;
	dgInverseDynamicsList::dgListNode* m_reference;
	dgList<dgLoopingJoint> m_loopingJoints;
	dgInt16 m_nodeCount;
	dgInt16 m_rowCount;
	dgInt16 m_ikRowCount;
	dgInt16 m_auxiliaryRowCount;

	friend class dgWorld;
	friend class dgWorldDynamicUpdate;
};

#endif

