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


#ifndef __D_VEHICLE_NODE_H__
#define __D_VEHICLE_NODE_H__

#include "dStdafxVehicle.h"

class dVehicleInterface;
class dKinematicLoopJoint;
class dVehicleTireInterface;


class dVehicleNode: public dAnimAcyclicJoint
{
	public:
	DVEHICLE_API dVehicleNode(dVehicleNode* const parent);
	DVEHICLE_API virtual ~dVehicleNode();
	
	DVEHICLE_API virtual void CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const;

	virtual dVehicleInterface* GetAsVehicle() const { return NULL; }
	virtual dVehicleTireInterface* GetAsTire() const { return NULL; }

	protected:
	virtual void RigidBodyToStates();
	virtual void Integrate(dFloat timestep);
	virtual void StatesToRigidBody(dFloat timestep);
	void CalculateAABB(const NewtonCollision* const collision, const dMatrix& matrix, dVector& minP, dVector& maxP) const;

	friend class dVehicleSolver;
};


#endif 

