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


//class dTireContact: public dAnimIDRigKinematicLoopJoint
class dTireContact
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

class dEngineBlockJoint : public dComplementaritySolver::dBilateralJoint
{
	public:
	dEngineBlockJoint();

	private:
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }
};

//class dEngineCrankJoint: public dAnimIDRigKinematicLoopJoint
class dEngineCrankJoint
{
	public:
	dEngineCrankJoint();

	void SetTorqueAndRpm(dFloat torque, dFloat rpm);
	private:
	int GetMaxDof() const { return 1; }
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dFloat m_targetRpm;
	dFloat m_targetTorque;
};

//class dGearBoxJoint: public dAnimIDRigKinematicLoopJoint
class dGearBoxJoint
{
	public:
	dGearBoxJoint();

	void SetGearRatio(dFloat ratio);
	void SetClutchTorque(dFloat toqrue);

	private:
	int GetMaxDof() const { return 1; }
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dFloat m_gearRatio;
	dFloat m_crowndGear;
	dFloat m_clutchTorque;
};

//class dTireAxleJoint: public dAnimIDRigKinematicLoopJoint
class dTireAxleJoint
{
	public:
	dTireAxleJoint();

	private:
	int GetMaxDof() const { return 1; }
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const { dAssert(0); }

	dFloat m_diffSign;

	friend class dVehicleVirtualDifferential;
};

#endif 

