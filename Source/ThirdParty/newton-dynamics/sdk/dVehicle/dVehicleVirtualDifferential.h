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


#ifndef __D_VEHICLE_VIRTUAL_DIFFERENTIAL_H__
#define __D_VEHICLE_VIRTUAL_DIFFERENTIAL_H__

#include "dStdafxVehicle.h"
#include "dVehicleInterface.h"
#include "dVehicleVirtualJoints.h"

class dVehicleVirtualDifferential: public dVehicleDifferentialInterface
{
	public:
	DVEHICLE_API dVehicleVirtualDifferential(dVehicleNode* const parent, dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire);
	DVEHICLE_API virtual ~dVehicleVirtualDifferential();
	

	protected:
	void ApplyExternalForce();
	void Integrate(dFloat timestep);
	dComplementaritySolver::dBilateralJoint* GetJoint();
	void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	dDifferentialMount m_differential;
	dDifferentialJoint m_leftDifferential;
	dDifferentialJoint m_rightDifferential;
	dVehicleTireInterface* m_leftTire;
	dVehicleTireInterface* m_rightTire;
	dFloat m_diffOmega;
	dFloat m_shaftOmega;

	friend class dVehicleChassis;
};


#endif 

