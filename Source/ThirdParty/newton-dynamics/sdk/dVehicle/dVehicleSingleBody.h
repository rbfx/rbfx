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

#ifndef __D_SINGLE_BODY_VEHICLE_H__
#define __D_SINGLE_BODY_VEHICLE_H__

#include "dStdafxVehicle.h"
#include "dVehicleInterface.h"

class dAnimIDRigKinematicLoopJoint;
class dVehicleSingleBody: public dVehicleInterface
{
	public:
	DVEHICLE_API dVehicleSingleBody(dVehicleChassis* const chassis);
	DVEHICLE_API virtual ~dVehicleSingleBody();

	DVEHICLE_API dMatrix GetMatrix () const;
	DVEHICLE_API dVehicleDifferentialInterface* AddDifferential(dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire);
	DVEHICLE_API dVehicleEngineInterface* AddEngine(const dVehicleEngineInterface::dEngineInfo& engineInfo, dVehicleDifferentialInterface* const diffrential);
	DVEHICLE_API dVehicleTireInterface* AddTire (const dMatrix& locationInGlobalSpace, const dVehicleTireInterface::dTireInfo& tireInfo, const dMatrix& localFrame);

	protected:
	void RigidBodyToStates();
	void ApplyExternalForce(dFloat timestep);
	void StatesToRigidBody(dFloat timestep);
	int GetKinematicLoops(dAnimIDRigKinematicLoopJoint** const jointArray);
	void CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const;

	DVEHICLE_API void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	dVector m_gravity;
	dVehicleNode m_groundNode;
	dVehicleChassis* m_chassis;

	friend class dVehicleChassis;
	friend class dVehicleVirtualTire;
	friend class dVehicleVirtualEngine;
	friend class dVehicleVirtualDifferential;
};


#endif 

