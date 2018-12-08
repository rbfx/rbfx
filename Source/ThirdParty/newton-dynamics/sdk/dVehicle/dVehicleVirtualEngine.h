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


#ifndef __D_VEHICLE_VIRTUAL_ENGINE_H__
#define __D_VEHICLE_VIRTUAL_ENGINE_H__

#include "dStdafxVehicle.h"
#include "dVehicleInterface.h"
#include "dVehicleVirtualJoints.h"

class dVehicleVirtualEngine: public dVehicleEngineInterface
{
	class dEngineTorqueNode
	{
		public:
		dEngineTorqueNode() {}
		dEngineTorqueNode(dFloat rpm, dFloat torque)
			:m_rpm(rpm)
			, m_torque(torque)
		{
		}
		dFloat m_rpm;
		dFloat m_torque;
	};


	class dEngineMetricInfo: public dEngineInfo
	{
		public:
		dEngineMetricInfo(const dEngineInfo& info);
		dFloat m_peakPowerTorque;
		//dFloat m_crownGearRatio;

		dFloat GetTorque (dFloat rpm) const;

		dEngineTorqueNode m_torqueCurve[6];
	};
	public:

	DVEHICLE_API dVehicleVirtualEngine(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential);
	DVEHICLE_API virtual ~dVehicleVirtualEngine();

	protected:
	virtual dFloat GetRpm() const;
	virtual dFloat GetRedLineRpm() const;

	void ApplyExternalForce();
	void InitEngineTorqueCurve();
	void Integrate(dFloat timestep);
	void SetThrottle (dFloat throttle);
	void SetInfo(const dEngineInfo& info);
	
	dComplementaritySolver::dBilateralJoint* GetJoint();
	void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	dEngineMetricInfo m_metricInfo;
	dEngineJoint m_joint;
	dFloat m_omega;
	friend class dVehicleChassis;
};


#endif 

