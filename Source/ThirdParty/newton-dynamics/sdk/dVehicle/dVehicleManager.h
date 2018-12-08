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


#ifndef __D_VEHICLE_MANAGER_H__
#define __D_VEHICLE_MANAGER_H__

#include "dStdafxVehicle.h"
#include "dVehicleChassis.h"

#define D_VEHICLE_MANAGER_NAME			"__ReducedCoordinadeVehicleManager__"

class dVehicleManager: public dCustomControllerManager<dVehicleChassis>
{
	public:
	DVEHICLE_API dVehicleManager(NewtonWorld* const world);
	DVEHICLE_API virtual ~dVehicleManager();
	
	DVEHICLE_API virtual void UpdateDriverInput(dVehicleChassis* const vehicle, dFloat timestep) {}

//	DVEHICLE_API virtual dVehicleChassis* CreateVehicle(NewtonBody* const body, const dMatrix& vehicleFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
//	DVEHICLE_API virtual dVehicleChassis* CreateVehicle(NewtonCollision* const chassisShape, const dMatrix& vehicleFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);

	DVEHICLE_API virtual dVehicleChassis* CreateSingleBodyVehicle(NewtonBody* const body, const dMatrix& vehicleFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	DVEHICLE_API virtual dVehicleChassis* CreateSingleBodyVehicle(NewtonCollision* const chassisShape, const dMatrix& vehicleFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);

	DVEHICLE_API virtual void DestroyController(dVehicleChassis* const controller);
	protected:

	DVEHICLE_API virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext);
	friend class dVehicleChassis;
};



#endif 

