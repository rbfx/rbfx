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


#ifndef __D_VEHICLE_CHASSIS_H__
#define __D_VEHICLE_CHASSIS_H__

#include "dStdafxVehicle.h"
#include "dVehicleSolver.h"
#include "dVehicleInterface.h"
#include "dVehicleDashControl.h"


class dVehicleChassis: public dCustomControllerBase
{
	public:
	class dDriverInput
	{
		public:
		dDriverInput()
			:m_throttle(0.0f)
			,m_brakePedal(0.0f)
			,m_clutchPedal(0.0f)
			,m_steeringValue(0.0f)
			,m_handBrakeValue(0.0f)
			,m_gear(0)
			,m_ignitionKey(0)
			,m_lockDifferential(0)
			,m_manualTransmission(0)
		{
		}

		dFloat m_throttle;
		dFloat m_brakePedal;
		dFloat m_clutchPedal;
		dFloat m_steeringValue;
		dFloat m_handBrakeValue;
		int m_gear;
		int m_ignitionKey;
		int m_lockDifferential;
		int m_manualTransmission;
	};

	private:
	class dCollectCollidingBodies
	{
		public:
		dCollectCollidingBodies(NewtonBody* const me)
			:m_exclude(me)
			,m_count(0)
			,m_staticCount(0)
		{
		}

		NewtonBody* m_exclude;
		int m_count;
		int m_staticCount;
		NewtonBody* m_array[16];
	};
/*
	public:
	DVEHICLE_API dEngineJoint* AddEngineJoint(dFloat mass, dFloat armatureRadius);
	DVEHICLE_API dWheelJoint* AddTire (const dMatrix& locationInGlobalSpace, const dTireInfo& tireInfo);
	
	DVEHICLE_API dDifferentialJoint* AddDifferential(dDifferentialJoint* const leftDifferential, dDifferentialJoint* const rightDifferential);

	DVEHICLE_API void LinkTiresKinematically(dWheelJoint* const tire0, dWheelJoint* const tire1);

	DVEHICLE_API dEngineJoint* GetEngineJoint() const;
	

	DVEHICLE_API dVector GetUpAxis() const;
	DVEHICLE_API dVector GetRightAxis() const;
	DVEHICLE_API dVector GetFrontAxis() const;
	DVEHICLE_API dMatrix GetBasisMatrix() const;

	DVEHICLE_API void SetCenterOfGravity(const dVector& comRelativeToGeomtriCenter);

	DVEHICLE_API dList<dWheelJoint*>::dListNode* GetFirstTire() const;
	DVEHICLE_API dList<dWheelJoint*>::dListNode* GetNextTire (dList<dWheelJoint*>::dListNode* const tireNode) const;

	DVEHICLE_API dList<dDifferentialJoint*>::dListNode* GetFirstDifferential() const;
	DVEHICLE_API dList<dDifferentialJoint*>::dListNode* GetNextDifferential(dList<dDifferentialJoint*>::dListNode* const differentialNode) const;

	DVEHICLE_API dList<NewtonBody*>::dListNode* GetFirstBodyPart() const;
	DVEHICLE_API dList<NewtonBody*>::dListNode* GetNextBodyPart(dList<NewtonBody*>::dListNode* const partNode) const;

	DVEHICLE_API dVector GetTireNormalForce(const dWheelJoint* const tire) const;
	DVEHICLE_API dVector GetTireLateralForce(const dWheelJoint* const tire) const;
	DVEHICLE_API dVector GetTireLongitudinalForce(const dWheelJoint* const tire) const;

	DVEHICLE_API dBrakeController* GetBrakes() const;
	DVEHICLE_API dEngineController* GetEngine() const;
	DVEHICLE_API dBrakeController* GetHandBrakes() const;
	DVEHICLE_API dSteeringController* GetSteering() const;
	
	DVEHICLE_API void SetEngine(dEngineController* const engine);
	DVEHICLE_API void SetBrakes(dBrakeController* const brakes);
	DVEHICLE_API void SetHandBrakes(dBrakeController* const brakes);
	DVEHICLE_API void SetSteering(dSteeringController* const steering);
	DVEHICLE_API void SetContactFilter(dTireFrictionModel* const filter);

	DVEHICLE_API dFloat GetAerodynamicsDowforceCoeficient() const;
	DVEHICLE_API void SetAerodynamicsDownforceCoefficient(dFloat downWeightRatioAtSpeedFactor, dFloat speedFactor, dFloat maxWeightAtTopSpeed);

	DVEHICLE_API dFloat GetWeightDistribution() const;
	DVEHICLE_API void SetWeightDistribution(dFloat weightDistribution);

	
	DVEHICLE_API void DrawSchematic (dCustomJoint::dDebugDisplay* const debugContext, dFloat x, dFloat y, dFloat scale) const;	


	bool ControlStateChanged() const;
	void Init (NewtonBody* const body, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	void Init (NewtonCollision* const chassisShape, dFloat mass, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	void Cleanup();
	
	void CalculateAerodynamicsForces ();
	void CalculateSuspensionForces (dFloat timestep);
	void CalculateTireForces (dFloat timestep, int threadID);
	void Collide (dWheelJoint* const tire, int threadIndex);
	
	dVector GetLastLateralForce(dWheelJoint* const tire) const;
	
	dMatrix m_localFrame;
	dList<NewtonBody*> m_bodyList;
	dList<dWheelJoint*> m_tireList;
	dList<dDifferentialJoint*> m_differentialList;

	dEngineJoint* m_engine;
	void* m_collisionAggregate;
	
	dBrakeController* m_brakesControl;
	dEngineController* m_engineControl;
	dBrakeController* m_handBrakesControl;
	dSteeringController* m_steeringControl; 
	dTireFrictionModel* m_contactFilter;
	NewtonApplyForceAndTorque m_forceAndTorqueCallback;

	dFloat m_speed;
	dFloat m_totalMass;
	dFloat m_gravityMag;
	dFloat m_sideSlip;
	dFloat m_prevSideSlip;
	dFloat m_weightDistribution;
	dFloat m_aerodynamicsDownForce0;
	dFloat m_aerodynamicsDownForce1;
	dFloat m_aerodynamicsDownSpeedCutOff;
	dFloat m_aerodynamicsDownForceCoefficient;

	bool m_finalized;
	friend class dEngineController;
	friend class dVehicleChassisManager;
*/

	public:
	DVEHICLE_API dVehicleChassis ();
	dVehicleInterface* GetVehicle() {return m_vehicle;}
	DVEHICLE_API dVehicleTireInterface* AddTire (const dMatrix& locationInGlobalSpace, const dVehicleTireInterface::dTireInfo& tireInfo);
	DVEHICLE_API dVehicleDifferentialInterface* AddDifferential(dVehicleTireInterface* const leftTire, dVehicleTireInterface* const rightTire);
	DVEHICLE_API dVehicleEngineInterface* AddEngine(const dVehicleEngineInterface::dEngineInfo& engineInfo, dVehicleDifferentialInterface* const differential);

	DVEHICLE_API dVehicleBrakeControl* GetBrakeControl();
	DVEHICLE_API dVehicleEngineControl* GetEngineControl();
	DVEHICLE_API dVehicleBrakeControl* GetHandBrakeControl();
	DVEHICLE_API dVehicleSteeringControl* GetSteeringControl ();

	DVEHICLE_API void ApplyDriverInputs(const dDriverInput& driveInputs, dFloat timestep);

	DVEHICLE_API void Finalize();

	void ApplyExternalForces(dFloat timestep);

	protected:
	DVEHICLE_API virtual void PreUpdate(dFloat timestep, int threadIndex);
	DVEHICLE_API virtual void PostUpdate(dFloat timestep, int threadIndex);
	DVEHICLE_API virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	void Init(NewtonBody* const body, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	void Init(NewtonCollision* const chassisShape, dFloat mass, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	void Cleanup();
	
	void CalculateTireContacts(dFloat timestep);
	void CalculateSuspensionForces(dFloat timestep);

	static int OnAABBOverlap(const NewtonBody * const body, void* const me);
	
	dMatrix m_localFrame;
	dVehicleSolver m_solver;
	dVector m_gravity;
	dVector m_obbSize;
	dVector m_obbOrigin;

	dVehicleInterface* m_vehicle;
	dVehicleBrakeControl* m_brakeControl;
	dVehicleEngineControl* m_engineControl;
	dVehicleBrakeControl* m_handBrakeControl;
	dVehicleSteeringControl* m_steeringControl;

	friend class dVehicleSolver;
	friend class dVehicleManager;
	friend class dVehicleVirtualTire;
	friend class dVehicleVirtualDifferential;
};


#endif 

