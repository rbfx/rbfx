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

#ifndef __D_VEHICLE_INTERFACE_H__
#define __D_VEHICLE_INTERFACE_H__

#include "dStdafxVehicle.h"
#include "dVehicleNode.h"

class dVehicleChassis;

class dVehicleDifferentialInterface: public dVehicleNode
{
	public:
	DVEHICLE_API dVehicleDifferentialInterface(dVehicleNode* const parent);
	virtual ~dVehicleDifferentialInterface() {}
};

class dVehicleEngineInterface: public dVehicleNode
{
	public:
	enum dGearRatioIndex
	{
		m_reverseGear = 0,
		m_neutralGear = 1,
		m_firstGear = 2,
		m_gears_count = 10,
	};

	class dEngineInfo
	{
		public:
		dEngineInfo ()
		{
			memset (this, 0, sizeof (dEngineInfo));

			m_mass = 50.0f;
			m_armatureRadius = 0.125f;
			m_idleTorque = 200.0f;				// IDLE_TORQUE
			m_rpmAtIdleTorque = 450.0f;			// IDLE_TORQUE_RPM
			m_peakTorque = 500.0f;				// PEAK_TORQUE
			m_rpmAtPeakTorque = 3000.0f;		// PEAK_TORQUE_RPM
			m_peakHorsePower = 400.0f;			// PEAK_HP
			m_rpmAtPeakHorsePower = 5200.0f;	// PEAK_HP_RPM
			m_rpmAtRedLine = 6000.0f;			// REDLINE_TORQUE_RPM

			m_gearsCount = 8;
			m_crownGear = 4.0f;
			m_clutchTorque = 1000.0f;
			m_gearRatios[dVehicleEngineInterface::m_reverseGear] = -2.90f;	// reverse
			m_gearRatios[dVehicleEngineInterface::m_neutralGear] = 0.0f;    // neutral
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 0] = 2.66f; // GEAR_1
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 1] = 1.78f;	// GEAR_2
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 2] = 1.30f;	// GEAR_3
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 3] = 1.00f;	// GEAR_4
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 4] = 0.74f;	// GEAR_5
			m_gearRatios[dVehicleEngineInterface::m_firstGear + 5] = 0.50f;	// GEAR_6
		}

		dFloat m_mass;
		dFloat m_armatureRadius;

		dFloat m_idleTorque;
		dFloat m_rpmAtIdleTorque;
		dFloat m_peakTorque;
		dFloat m_rpmAtPeakTorque;
		dFloat m_peakHorsePower;
		dFloat m_rpmAtPeakHorsePower;
		dFloat m_rpmAtRedLine;

		dFloat m_crownGear;
		dFloat m_clutchTorque;
		dFloat m_gearRatios[m_gears_count];
		int m_gearsCount;

	};

	DVEHICLE_API dVehicleEngineInterface(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential);
	virtual ~dVehicleEngineInterface() {}

	virtual dFloat GetRpm () const {return 0.0f;}
	virtual dFloat GetRedLineRpm () const {return 1.0f;}
	virtual void SetThrottle (dFloat throttle) {}

	virtual void SetGear (int gear) {}
	virtual void SetClutch (dFloat clutch) {}

	const dEngineInfo& GetInfo() const { return m_info; }
	DVEHICLE_API void SetInfo(const dEngineInfo& info) { m_info = info; }
	
	dEngineInfo m_info;
	dVehicleDifferentialInterface* m_differential;
};

class dVehicleTireInterface: public dVehicleNode
{
	public:
	enum dSuspensionType
	{
		m_offroad,
		m_confort,
		m_race,
		m_roller,
	};

	class dTireInfo
	{
		public:
		dTireInfo()
		{
			memset(this, 0, sizeof(dTireInfo));

			dFloat gravity = 9.8f;
			dFloat vehicleMass = 1000.0f;

			m_mass = 25.0f;
			m_radio = 0.5f;
			m_width = 0.15f;
			m_pivotOffset = 0.01f;
			m_steerRate = 0.5f * dPi;
			m_frictionCoefficient = 0.8f;
			m_maxSteeringAngle = 40.0f * dDegreeToRad;

			m_suspensionLength = 0.3f;
			m_dampingRatio = 15.0f * vehicleMass;
			m_springStiffness = dAbs(vehicleMass * 9.8f * 8.0f / m_suspensionLength);

			m_corneringStiffness = dAbs(vehicleMass * gravity * 1.0f);
			m_longitudinalStiffness = dAbs(vehicleMass * gravity * 1.0f);
		}

		dVector m_location;
		dFloat m_mass;
		dFloat m_radio;
		dFloat m_width;
		dFloat m_steerRate;
		dFloat m_pivotOffset;
		dFloat m_dampingRatio;
		dFloat m_springStiffness;
		dFloat m_suspensionLength;
		dFloat m_maxSteeringAngle;
		dFloat m_corneringStiffness;
		dFloat m_longitudinalStiffness;
		dFloat m_frictionCoefficient;
		//dFloat m_aligningMomentTrail;
		//int m_hasFender;
		//dSuspensionType m_suspentionType;
	};

	DVEHICLE_API dVehicleTireInterface(dVehicleNode* const parent, const dTireInfo& info);
	virtual ~dVehicleTireInterface() {}

	virtual dVehicleTireInterface* GetAsTire() const { return (dVehicleTireInterface*) this; }

	const dTireInfo& GetInfo() const { return m_info; }
	void SetInfo(const dTireInfo& info) { m_info = info; }

	virtual dMatrix GetLocalMatrix() const = 0;
	virtual dMatrix GetGlobalMatrix() const = 0;
	virtual NewtonCollision* GetCollisionShape() const = 0;

	virtual dFloat GetSteeringAngle() const = 0;
	virtual void SetSteeringAngle(dFloat steeringAngle) = 0;

	virtual dFloat GetBrakeTorque() const = 0;
	virtual void SetBrakeTorque(dFloat brakeTorque) = 0;

	protected:
	dTireInfo m_info;
};

class dVehicleInterface: public dVehicleNode
{
	public:
	DVEHICLE_API dVehicleInterface(dVehicleChassis* const chassis);
	virtual ~dVehicleInterface() {};

	virtual dMatrix GetMatrix() const = 0;
	virtual dVehicleInterface* GetAsVehicle() const {return (dVehicleInterface*)this;} 

	protected:
	virtual dVehicleDifferentialInterface* AddDifferential(dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire) = 0;
	virtual dVehicleEngineInterface* AddEngine(const dVehicleEngineInterface::dEngineInfo& engineInfo, dVehicleDifferentialInterface* const differential) = 0;
	virtual dVehicleTireInterface* AddTire(const dMatrix& locationInGlobalSpace, const dVehicleTireInterface::dTireInfo& tireInfo, const dMatrix& localFrame) = 0;

	friend class dVehicleChassis;
};
#endif 

