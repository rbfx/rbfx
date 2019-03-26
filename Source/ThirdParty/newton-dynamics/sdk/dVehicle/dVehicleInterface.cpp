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
#include "dVehicleChassis.h"
#include "dVehicleInterface.h"


dVehicleDifferentialInterface::dVehicleDifferentialInterface(dVehicleNode* const parent)
	:dVehicleNode(parent)
{
}

dVehicleEngineInterface::dVehicleEngineInterface(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential)
	:dVehicleNode(parent)
	,m_info(info)
	,m_differential(differential)
{
}

dVehicleTireInterface::dVehicleTireInterface(dVehicleNode* const parent, const dTireInfo& info)
	:dVehicleNode(parent)
	,m_info(info)
{
	dAssert(0);
//	SetWorld(parent->GetWorld());
}


dVehicleInterface::dVehicleInterface(dVehicleChassis* const chassis)
	:dVehicleNode(NULL)
{
	dAssert(0);
//	SetWorld(NewtonBodyGetWorld(chassis->GetBody()));
}

