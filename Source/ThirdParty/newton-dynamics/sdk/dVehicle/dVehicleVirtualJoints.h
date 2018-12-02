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


#ifndef __D_VEHICLE_VIRTUAL_JOINTS_H__
#define __D_VEHICLE_VIRTUAL_JOINTS_H__

#include "dStdafxVehicle.h"

class dVehicleNode;
class dVehicleVirtualTire;

// tang of 10 degrees
#define D_TIRE_MAX_LATERAL_SLIP				(0.175f)
#define D_TIRE_MAX_ELASTIC_DEFORMATION		(0.05f)
#define D_TIRE_MAX_ELASTIC_NORMAL_STIFFNESS (10.0f / D_TIRE_MAX_ELASTIC_DEFORMATION)

class dTireJoint: public dComplementaritySolver::dBilateralJoint
{
	public:
	dTireJoint();
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dVehicleVirtualTire* m_tire;
};

class dDifferentialMount: public dComplementaritySolver::dBilateralJoint
{
	public:
	dDifferentialMount();

	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }
	bool m_slipeOn;
};

class dEngineJoint: public dComplementaritySolver::dBilateralJoint
{
	public:
	dEngineJoint();

	void SetTorqueAndRpm (dFloat torque, dFloat rpm);
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dFloat m_targetRpm;
	dFloat m_targetTorque;
};

class dKinematicLoopJoint : public dComplementaritySolver::dBilateralJoint
{
	public:
	dKinematicLoopJoint();
	~dKinematicLoopJoint(){}
	bool IsActive() const { return m_isActive; }
	dVehicleNode* GetOwner0() const{return m_owner0;}
	dVehicleNode* GetOwner1() const{return m_owner1;}
	void SetOwners(dVehicleNode* const owner0, dVehicleNode* const owner1);

	virtual int GetMaxDof() const = 0;

	dVehicleNode* m_owner0;
	dVehicleNode* m_owner1;
	bool m_isActive;
};


class dTireContact: public dKinematicLoopJoint
{
	public:
	class dTireModel
	{
		public:
		dTireModel ()
			:m_lateralSlip(0.0f)
			,m_longitodinalSlip(0.0f)
			,m_alingMoment(0.0f)
			,m_lateralForce(0.0f)
			,m_longitunalForce(0.0f)
		{
		}

		dFloat m_lateralSlip;
		dFloat m_longitodinalSlip;
		dFloat m_alingMoment;
		dFloat m_lateralForce;
		dFloat m_longitunalForce;
	};

	dTireContact();

	void ResetContact ();
	void Debug(dCustomJoint::dDebugDisplay* const debugContext, dFloat scale) const;
	void SetContact (const dVector& posit, const dVector& normal, const dVector& lateralDir, dFloat penetration, dFloat staticFriction, dFloat kineticFriction);

	private:
	int GetMaxDof() const { return 3;}
	void TireForces(dFloat longitudinalSlip, dFloat lateralSlip, dFloat frictionCoef);
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dVector m_point;
	dVector m_normal;
	dVector m_lateralDir;
	dVector m_longitudinalDir;
	dFloat m_penetration;
	dFloat m_staticFriction;
	dFloat m_kineticFriction;
	dFloat m_load;
	dTireModel m_tireModel;
	dFloat m_normalFilter[4];
	bool m_isActiveFilter[4];

	friend class dVehicleVirtualTire;
};


class dDifferentialJoint: public dKinematicLoopJoint
{
	public:
	dDifferentialJoint ();

	private:
	int GetMaxDof() const { return 1;}
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }
};


#endif 

