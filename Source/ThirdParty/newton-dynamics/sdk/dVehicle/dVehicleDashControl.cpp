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


#include "dStdafxVehicle.h"
#include "dVehicleInterface.h"
#include "dVehicleDashControl.h"

dVehicleDashControl::dVehicleDashControl(dVehicleChassis* const vehicle)
	:dCustomAlloc()
	,m_vehicle(vehicle)
	,m_param(0.0f)
	,m_paramMemory(0.0f)
	,m_timer(60)
{
}

dVehicleDashControl::~dVehicleDashControl()
{
}

void dVehicleDashControl::SetParam(dFloat param)
{
	m_paramMemory = m_param;
	m_param = param;
}

bool dVehicleDashControl::ParamChanged() const
{
	m_timer--;
	if (dAbs(m_paramMemory - m_param) > 1.e-3f) {
		m_timer = 30;
	}
	return m_timer > 0;
}

// ****************************************************************************
//
// ****************************************************************************
dVehicleEngineControl::dVehicleEngineControl(dVehicleChassis* const vehicle)
	:dVehicleDashControl(vehicle)
	,m_engine(NULL)
{
}

void dVehicleEngineControl::SetEngine (dVehicleEngineInterface* const engine)
{
	m_engine = engine; 
}

void dVehicleEngineControl::Update(dFloat timestep)
{
	if (m_engine) {
		m_engine->SetThrottle (m_param);
	}
}

// ****************************************************************************
//
// ****************************************************************************
dVehicleTireControl::dVehicleTireControl(dVehicleChassis* const vehicle)
	:dVehicleDashControl(vehicle)
	,m_tires()
	,m_isSleeping(false)
{
}
	
dVehicleTireControl::~dVehicleTireControl()
{
	m_tires.RemoveAll();
}

void dVehicleTireControl::AddTire(dVehicleTireInterface* const tire)
{
	m_tires.Append(tire);
}


// ****************************************************************************
//
// ****************************************************************************
dVehicleSteeringControl::dVehicleSteeringControl(dVehicleChassis* const vehicle)
	:dVehicleTireControl(vehicle)
{
}

void dVehicleSteeringControl::Update(dFloat timestep)
{
	m_isSleeping = true;
	for (dList<dVehicleTireInterface*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext()) {
		dVehicleTireInterface* const tire = node->GetInfo();
		const dVehicleTireInterface::dTireInfo& info = tire->GetInfo();

		dFloat angle = tire->GetSteeringAngle();
		dFloat targetAngle = m_param * info.m_maxSteeringAngle;
		if (angle < targetAngle) {
			angle += info.m_steerRate * timestep;
			if (angle > targetAngle) {
				angle = targetAngle;
			}
		} else if (angle > targetAngle) {
			angle -= info.m_steerRate * timestep;
			if (angle < targetAngle) {
				angle = targetAngle;
			}
		}
		tire->SetSteeringAngle(angle);
		m_isSleeping &= dAbs(targetAngle - angle) < 1.0e-4f;
	}
}

// ****************************************************************************
//
// ****************************************************************************
dVehicleBrakeControl::dVehicleBrakeControl(dVehicleChassis* const vehicle)
	:dVehicleTireControl(vehicle)
	,m_maxTorque(0.0f)
{
}

dFloat dVehicleBrakeControl::GetBrakeTorque() const
{
	return m_maxTorque;
}

void dVehicleBrakeControl::SetBrakeTorque(dFloat torque)
{
	m_maxTorque = dAbs(torque);
}

void dVehicleBrakeControl::Update(dFloat timestep)
{
	m_isSleeping = true;
	for (dList<dVehicleTireInterface*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext()) {
		dVehicleTireInterface* const tire = node->GetInfo();
		dFloat torque = dMax(m_maxTorque * m_param, tire->GetBrakeTorque());
		tire->SetBrakeTorque(torque);
	}
}
