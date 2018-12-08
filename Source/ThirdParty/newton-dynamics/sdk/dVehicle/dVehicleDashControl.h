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


#ifndef __D_VEHICLE_DASH_CONTROL_H__
#define __D_VEHICLE_DASH_CONTROL_H__

#include "dStdafxVehicle.h"

class dVehicleChassis;
class dVehicleTireInterface;

class dVehicleDashControl: public dCustomAlloc
{
	public:
	DVEHICLE_API dVehicleDashControl(dVehicleChassis* const vehicle);
	DVEHICLE_API virtual ~dVehicleDashControl();

	DVEHICLE_API void SetParam(dFloat param);
	DVEHICLE_API dFloat GetParam() const{return m_param;}

	DVEHICLE_API bool ParamChanged() const;
	virtual void Update(dFloat timestep) = 0;

	protected:
	dVehicleChassis* m_vehicle;
	dFloat m_param;
	dFloat m_paramMemory;
	mutable dFloat m_timer;
};

class dVehicleEngineControl: public dVehicleDashControl 
{
	public:
	DVEHICLE_API dVehicleEngineControl(dVehicleChassis* const vehicle);
	DVEHICLE_API void SetEngine (dVehicleEngineInterface* const engine);

	void SetGear (int gear);
	void SetClutch (dFloat clutch);
	dVehicleEngineInterface* GetEngine() const {return m_engine;}

	virtual void Update(dFloat timestep);

	private:
	dVehicleEngineInterface* m_engine;
};

class dVehicleTireControl: public dVehicleDashControl 
{
	public:
	DVEHICLE_API dVehicleTireControl(dVehicleChassis* const vehicle);
	DVEHICLE_API virtual ~dVehicleTireControl();

	DVEHICLE_API void AddTire(dVehicleTireInterface* const tire);
	
	protected:
	dList<dVehicleTireInterface*> m_tires;
	bool m_isSleeping;
};

class dVehicleSteeringControl: public dVehicleTireControl 
{
	public:
	dVehicleSteeringControl(dVehicleChassis* const vehicle);

	protected:
	virtual void Update(dFloat timestep);
	friend class dVehicleChassis;
};


class dVehicleBrakeControl: public dVehicleTireControl
{
	public:
	dVehicleBrakeControl(dVehicleChassis* const vehicle);

	DVEHICLE_API dFloat GetBrakeTorque () const;
	DVEHICLE_API void SetBrakeTorque (dFloat torque);

	protected:
	virtual void Update(dFloat timestep);

	dFloat m_maxTorque;
	friend class dVehicleChassis;
};


#endif 

