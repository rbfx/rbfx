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


// NewtonVehicleControllerManager.h: interface for the NewtonVehicleControllerManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_VEHICLE_CONTROLLER_MANAGER_H_
#define D_CUSTOM_VEHICLE_CONTROLLER_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomAlloc.h"
#include "dCustomControllerManager.h"

#define VEHICLE_PLUGIN_NAME			"__vehicleManager__"
//#define VEHICLE_USE_ZERO_TORQUE_DIFFERENTIAL

class dAxelJoint;
class dWheelJoint;
class dEngineJoint;
class dGearBoxJoint;
class dEngineMountJoint;
class dDifferentialJoint;
class dDifferentialMountJoint;
class dCustomVehicleController;
class dCustomVehicleControllerManager;

class dVehicleDriverInput
{
	public:
	dVehicleDriverInput()
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
	}

	dFloat m_mass;
	dFloat m_radio;
	dFloat m_width;
	dFloat m_pivotOffset;
	dFloat m_maxSteeringAngle;
	dFloat m_dampingRatio;
	dFloat m_springStrength;
	dFloat m_suspensionLength;
	dFloat m_corneringStiffness;
	dFloat m_aligningMomentTrail;
	int m_hasFender;
	dSuspensionType m_suspentionType;
};

class dTireFrictionModel
{
	public:
	dTireFrictionModel(const dCustomVehicleController* const controller)
		:m_controller(controller)
	{
	}

	virtual dFloat GetFrictionCoefficient(const NewtonMaterial* const material, const NewtonBody* const tireBody, const NewtonBody* const otherBody) const
	{
		// the vehicle model is realistic, please do not use fake larger than one fiction coefficient, or you vehicle will simply role over
		return 1.0f;
	}

	virtual void CalculateTireForces(const dWheelJoint* const tire, const NewtonBody* const otherBody,
		dFloat tireLoad, dFloat longitudinalSlip, dFloat lateralSlip, dFloat corneringStiffness,
		dFloat& longitudinalForce, dFloat& lateralForce, dFloat& aligningTorque) const;


	const dCustomVehicleController* m_controller;
};


class dEngineTorqueNode
{
	public:
	dEngineTorqueNode() {}
	dEngineTorqueNode(dFloat rpm, dFloat torque)
		:m_rpm(rpm)
		,m_torque(torque)
	{
	}
	dFloat m_rpm;
	dFloat m_torque;
};

class dEngineInfo
{
	public:
	dEngineInfo()
	{
		memset(this, 0, sizeof(dEngineInfo));
	}

	dVector m_location;
	dFloat m_mass;
	dFloat m_radio;
	dFloat m_idleTorque;
	dFloat m_rpmAtIdleTorque;
	dFloat m_peakTorque;
	dFloat m_rpmAtPeakTorque;
	dFloat m_peakHorsePower;
	dFloat m_rpmAtPeakHorsePower;
	dFloat m_rpmAtRedLine;

	dFloat m_vehicleTopSpeed;
	dFloat m_reverseGearRatio;
	dFloat m_gearRatios[10];
	dFloat m_gearRatiosSign;
	dFloat m_clutchFrictionTorque;

	dFloat m_aerodynamicDownforceFactor;
	dFloat m_aerodynamicDownforceFactorAtTopSpeed;
	dFloat m_aerodynamicDownForceSurfaceCoeficident;

	int m_gearsCount;
	int m_differentialLock;

	private:
	void SetTorqueRPMTable();
	void ConvertToMetricSystem();
	
	dEngineTorqueNode m_torqueCurve[6];
	dFloat m_crownGearRatio;
	dFloat m_peakPowerTorque;
	friend class dEngineController;
	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
};

class dWheelJoint: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dWheelJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const tireBody, NewtonBody* const chassisBody, dCustomVehicleController* const controller, const dTireInfo& tireInfo);

	NewtonBody* GetTireBody() const
	{
		return GetBody0();
	}

	dCustomVehicleController* GetController() const
	{
		return m_controller;
	}

	dFloat GetTireLoad() const
	{
		return NewtonUserJointGetRowForce(m_joint, 4);
	}

	dVector GetLongitudinalForce() const
	{
		return m_longitudinalDir.Scale(NewtonUserJointGetRowForce(m_joint, 1));
	}

	dVector GetLateralForce() const
	{
		return m_lateralDir.Scale(NewtonUserJointGetRowForce(m_joint, 0));
	}

	CUSTOM_JOINTS_API void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API void ResetTransform();
	CUSTOM_JOINTS_API dFloat CalculateTireParametricPosition(const dMatrix& tireMatrix, const dMatrix& chassisMatrix) const;
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);
	
	dVector m_lateralDir;
	dVector m_longitudinalDir;
	dFloat m_tireLoad;
	dFloat m_radio;
	dFloat m_width;
	dFloat m_steerRate;
	dFloat m_steerAngle0;
	dFloat m_steerAngle1;
	dFloat m_brakeTorque;
	dFloat m_dampingRatio;
	dFloat m_springStrength;
	dFloat m_suspensionLength;
	dFloat m_lateralSlip;
	dFloat m_aligningTorque;
	dFloat m_longitudinalSlip;
	dFloat m_aligningMomentTrail;
	dFloat m_corneringStiffness;
	dFloat m_maxSteeringAngle;

	dCustomVehicleController* m_controller;
	int m_suspentionType;
	int m_hasFender;
	int m_contactCount;
	int m_index;
	NewtonWorldConvexCastReturnInfo m_contactInfo[4];
	dVector m_contactTangentDir0[4];
	dFloat m_lateralSpeed[4];
	dFloat m_longitudinalSpeed[4];

	friend class dBrakeController;
	friend class dEngineController;
	friend class dSteeringController;
	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dWheelJoint, dCustomJoint)
};


class dEngineMountJoint: public dCustomHinge
{
	public:
	CUSTOM_JOINTS_API dEngineMountJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const engineBody, NewtonBody* const chassisBody);

	protected:
	CUSTOM_JOINTS_API void ResetTransform();
	CUSTOM_JOINTS_API void SubmitConstraintsFreeDof(dFloat timestep, const dMatrix& matrix0, const dMatrix& matrix1);

	dMatrix m_baseOffsetMatrix;
	friend class dEngineController;
	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dEngineMountJoint, dCustomHinge)
};

class dEngineJoint: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dEngineJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const engineBody, NewtonBody* const chassisBody);
	NewtonBody* GetEngineBody() const
	{
		return GetBody0();
	}

	protected:
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);

	dEngineMountJoint* m_engineMount;
	dFloat m_torque;
	dFloat m_rpm;
	dFloat m_targetRpm;

	friend class dEngineController;
	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dEngineJoint, dCustomJoint)
};


class dGearBoxJoint: public dCustomGear
{
	public:
	dGearBoxJoint(const dVector& childPin, NewtonBody* const differential, NewtonBody* const engine, dFloat maxFrictionToque);

	void SetGearRatio(dFloat gear)
	{
		m_gearRatio = -gear;
	}

	void SetFritionTorque(dFloat param)
	{
		m_param = param;
	}

	protected:
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);

	dFloat m_param;
	dFloat m_cluthFrictionTorque;

	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dGearBoxJoint, dCustomGear)
};


class dDifferentialMountJoint: public dCustomDoubleHinge
{
	public:
	CUSTOM_JOINTS_API dDifferentialMountJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const differentialBody, NewtonBody* const chassisBody);

	protected:
	CUSTOM_JOINTS_API void ResetTransform();

	dMatrix m_baseOffsetMatrix;
	friend class dDifferentialJoint;
	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dDifferentialMountJoint, dCustomDoubleHinge)
};

#ifdef VEHICLE_USE_ZERO_TORQUE_DIFFERENTIAL
class dDifferentialJoint: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dDifferentialJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const differentialBody, NewtonBody* const chassisBody);

	NewtonBody* GetDifferentialBody() const
	{
		return GetBody0();
	}

	protected:
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API void ResetTransform();

//	dMatrix m_baseOffsetMatrix;
	dDifferentialMountJoint* m_differentialMount;
	dFloat m_turnSpeed;
	int m_isTractionDifferential;

	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dDifferentialJoint, dCustomJoint)
};

#else
class dDifferentialJoint: public dCustomDoubleHinge
{
	public:
	CUSTOM_JOINTS_API dDifferentialJoint(const dMatrix& pinAndPivotFrame, NewtonBody* const differentialBody, NewtonBody* const chassisBody);

	NewtonBody* GetDifferentialBody() const
	{
		return GetBody0();
	}

	protected:
	CUSTOM_JOINTS_API void ResetTransform();
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);

	dMatrix m_baseOffsetMatrix;
	dFloat m_turnSpeed;
	int m_isTractionDifferential;

	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dDifferentialJoint, dCustomDoubleHinge)
};

#endif

class dAxelJoint: public dCustomGear
{
	public:
	CUSTOM_JOINTS_API dAxelJoint(const dVector& childPin, const dVector& parentPin, const dVector& referencePin, NewtonBody* const child, NewtonBody* const parent);

	void SetGear(dFloat gearRatio)
	{
		m_gearRatio = gearRatio;
	}

	protected:
	CUSTOM_JOINTS_API void SubmitConstraints(dFloat timestep, int threadIndex);

	friend class dCustomVehicleController;
	friend class dCustomVehicleControllerManager;
	DECLARE_CUSTOM_JOINT(dAxelJoint, dCustomGear)
};


class dVehicleController: public dCustomAlloc
{
	public:
	dVehicleController(dCustomVehicleController* const controller)
		:m_controller(controller)
		, m_param(0.0f)
		, m_paramMemory(0.0f)
		, m_timer(60)
	{
	}

	~dVehicleController()
	{
	}

	void SetParam(dFloat param)
	{
		m_paramMemory = m_param;
		m_param = param;
	}

	dFloat GetParam() const
	{
		return m_param;
	}

	bool ParamChanged() const
	{
		m_timer--;
		if (dAbs(m_paramMemory - m_param) > 1.e-3f) {
			m_timer = 30;
		}
		return m_timer > 0;
	}


	virtual void Update(dFloat timestep)
	{
		dAssert(0);
	}

	dCustomVehicleController* m_controller;

	dFloat m_param;
	dFloat m_paramMemory;
	mutable dFloat m_timer;
};

class dEngineController: public dVehicleController
{
	public:
	enum dDrivingState
	{
		m_engineOff,
		m_engineIdle,
		m_engineStop,
		m_driveForward,
		m_driveReverse,
		m_engineStopDelay,
	};

	CUSTOM_JOINTS_API dEngineController(dCustomVehicleController* const controller);
	CUSTOM_JOINTS_API dEngineController(dCustomVehicleController* const controller, const dEngineInfo& info, dDifferentialJoint* const differential, dWheelJoint* const crownGearCalculator);
	CUSTOM_JOINTS_API ~dEngineController();

	CUSTOM_JOINTS_API void ApplyTorque(dFloat torque, dFloat rpm);

	CUSTOM_JOINTS_API dEngineInfo GetInfo() const;
	CUSTOM_JOINTS_API void SetInfo(const dEngineInfo& info);

	CUSTOM_JOINTS_API dFloat GetRPM() const;
	CUSTOM_JOINTS_API dFloat GetIdleRPM() const;
	CUSTOM_JOINTS_API dFloat GetRedLineRPM() const;
	CUSTOM_JOINTS_API dFloat GetSpeed() const;
	CUSTOM_JOINTS_API dFloat GetTopSpeed() const;

	CUSTOM_JOINTS_API int GetGear() const;
	CUSTOM_JOINTS_API void SetGear(int gear);

	CUSTOM_JOINTS_API void SetIgnition(bool key);
	CUSTOM_JOINTS_API bool GetIgnition() const;

	CUSTOM_JOINTS_API int GetFirstGear() const;
	CUSTOM_JOINTS_API int GetLastGear() const;
	CUSTOM_JOINTS_API int GetNeutralGear() const;
	CUSTOM_JOINTS_API int GetReverseGear() const;

	CUSTOM_JOINTS_API bool GetTransmissionMode() const;
	CUSTOM_JOINTS_API void SetTransmissionMode(bool mode);

	CUSTOM_JOINTS_API bool GetDifferentialLock() const;
	CUSTOM_JOINTS_API void SetDifferentialLock(bool mode);
	CUSTOM_JOINTS_API void SetClutchParam(dFloat cluthParam);

	CUSTOM_JOINTS_API void PlotEngineCurve() const;

	protected:
	virtual void Update(dFloat timestep);

	dFloat GetTopGear() const;
	dFloat GetRadiansPerSecond() const;
	void InitEngineTorqueCurve();
	dFloat GetGearRatio() const;

	dFloat IntepolateTorque(dFloat rpm) const;
	void UpdateAutomaticGearBox(dFloat timestep, dFloat omega);

	dEngineInfo m_info;
	dEngineInfo m_infoCopy;
	dWheelJoint* m_crownGearCalculator;
	dDifferentialJoint* m_differential;
	dGearBoxJoint* m_gearBoxJoint;
	dFloat m_clutchParam;
	int m_gearTimer;
	int m_currentGear;
	int m_drivingState;
	int m_ignitionKey;
	int m_automaticTransmissionMode;
	int m_stopDelay;
	friend class dCustomVehicleController;
};

class dSteeringController: public dVehicleController
{
	public:
	CUSTOM_JOINTS_API dSteeringController(dCustomVehicleController* const controller);
	CUSTOM_JOINTS_API void AddTire(dWheelJoint* const tire);

	protected:
	virtual void Update(dFloat timestep);

	dList<dWheelJoint*> m_tires;
	bool m_isSleeping;
	friend class dCustomVehicleController;
};

class dTractionSteeringController: public dSteeringController
{
	public:
	CUSTOM_JOINTS_API dTractionSteeringController(dCustomVehicleController* const controller, dDifferentialJoint* const differential);

	protected:
	virtual void Update(dFloat timestep);
	dDifferentialJoint* m_differential;
	friend class dCustomVehicleController;
};

class dBrakeController: public dVehicleController
{
	public:
	CUSTOM_JOINTS_API dBrakeController(dCustomVehicleController* const controller, dFloat maxBrakeTorque = 10.0f);
	CUSTOM_JOINTS_API void AddTire(dWheelJoint* const tire);

	protected:
	virtual void Update(dFloat timestep);

	dList<dWheelJoint*> m_tires;
	dFloat m_maxTorque;
	friend class dCustomVehicleController;
};

class dCustomVehicleController: public dCustomControllerBase
{
	class dTireFilter;
	public:
	CUSTOM_JOINTS_API void ApplyDefualtDriver(const dVehicleDriverInput& driveInputs, dFloat timestep);

	CUSTOM_JOINTS_API void Finalize();
	CUSTOM_JOINTS_API dEngineJoint* AddEngineJoint(dFloat mass, dFloat armatureRadius);
	CUSTOM_JOINTS_API dWheelJoint* AddTire (const dMatrix& locationInGlobalSpace, const dTireInfo& tireInfo);
	CUSTOM_JOINTS_API dDifferentialJoint* AddDifferential(dWheelJoint* const leftTire, dWheelJoint* const rightTire);
	CUSTOM_JOINTS_API dDifferentialJoint* AddDifferential(dDifferentialJoint* const leftDifferential, dDifferentialJoint* const rightDifferential);

	CUSTOM_JOINTS_API void LinkTiresKinematically(dWheelJoint* const tire0, dWheelJoint* const tire1);

	CUSTOM_JOINTS_API dEngineJoint* GetEngineJoint() const;
	

	CUSTOM_JOINTS_API dVector GetUpAxis() const;
	CUSTOM_JOINTS_API dVector GetRightAxis() const;
	CUSTOM_JOINTS_API dVector GetFrontAxis() const;
	CUSTOM_JOINTS_API dMatrix GetBasisMatrix() const;

	CUSTOM_JOINTS_API void SetCenterOfGravity(const dVector& comRelativeToGeomtriCenter);

	CUSTOM_JOINTS_API dList<dWheelJoint*>::dListNode* GetFirstTire() const;
	CUSTOM_JOINTS_API dList<dWheelJoint*>::dListNode* GetNextTire (dList<dWheelJoint*>::dListNode* const tireNode) const;

	CUSTOM_JOINTS_API dList<dDifferentialJoint*>::dListNode* GetFirstDifferential() const;
	CUSTOM_JOINTS_API dList<dDifferentialJoint*>::dListNode* GetNextDifferential(dList<dDifferentialJoint*>::dListNode* const differentialNode) const;

	CUSTOM_JOINTS_API dList<NewtonBody*>::dListNode* GetFirstBodyPart() const;
	CUSTOM_JOINTS_API dList<NewtonBody*>::dListNode* GetNextBodyPart(dList<NewtonBody*>::dListNode* const partNode) const;

	CUSTOM_JOINTS_API dVector GetTireNormalForce(const dWheelJoint* const tire) const;
	CUSTOM_JOINTS_API dVector GetTireLateralForce(const dWheelJoint* const tire) const;
	CUSTOM_JOINTS_API dVector GetTireLongitudinalForce(const dWheelJoint* const tire) const;

	CUSTOM_JOINTS_API dBrakeController* GetBrakes() const;
	CUSTOM_JOINTS_API dEngineController* GetEngine() const;
	CUSTOM_JOINTS_API dBrakeController* GetHandBrakes() const;
	CUSTOM_JOINTS_API dSteeringController* GetSteering() const;
	
	CUSTOM_JOINTS_API void SetEngine(dEngineController* const engine);
	CUSTOM_JOINTS_API void SetBrakes(dBrakeController* const brakes);
	CUSTOM_JOINTS_API void SetHandBrakes(dBrakeController* const brakes);
	CUSTOM_JOINTS_API void SetSteering(dSteeringController* const steering);
	CUSTOM_JOINTS_API void SetContactFilter(dTireFrictionModel* const filter);

	CUSTOM_JOINTS_API dFloat GetAerodynamicsDowforceCoeficient() const;
	CUSTOM_JOINTS_API void SetAerodynamicsDownforceCoefficient(dFloat downWeightRatioAtSpeedFactor, dFloat speedFactor, dFloat maxWeightAtTopSpeed);

	CUSTOM_JOINTS_API dFloat GetWeightDistribution() const;
	CUSTOM_JOINTS_API void SetWeightDistribution(dFloat weightDistribution);

	CUSTOM_JOINTS_API virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;
	CUSTOM_JOINTS_API void DrawSchematic (dCustomJoint::dDebugDisplay* const debugContext, dFloat x, dFloat y, dFloat scale) const;	

	protected:
	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep, int threadIndex);
	virtual void PostUpdate(dFloat timestep, int threadIndex) {};
	
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
	friend class dCustomVehicleControllerManager;
};

class dCustomVehicleControllerManager: public dCustomControllerManager<dCustomVehicleController> 
{
	public:
	CUSTOM_JOINTS_API dCustomVehicleControllerManager(NewtonWorld* const world, int materialCount, int* const otherMaterials);
	CUSTOM_JOINTS_API virtual ~dCustomVehicleControllerManager();

	CUSTOM_JOINTS_API virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext);
	CUSTOM_JOINTS_API virtual void UpdateDriverInput(dCustomVehicleController* const vehicle, dFloat timestep) {}

	CUSTOM_JOINTS_API virtual dCustomVehicleController* CreateVehicle (NewtonBody* const body, const dMatrix& vehicleFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	CUSTOM_JOINTS_API virtual dCustomVehicleController* CreateVehicle (NewtonCollision* const chassisShape, const dMatrix& vehicleFrame, dFloat mass, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	CUSTOM_JOINTS_API virtual void DestroyController (dCustomVehicleController* const controller);
	CUSTOM_JOINTS_API virtual int OnTireAabbOverlap(const NewtonMaterial* const material, const dWheelJoint* const tire, const NewtonBody* const otherBody) const;
//	CUSTOM_JOINTS_API virtual int OnTireAabbOverlap(const NewtonJoint* const contactJoint, dFloat timestep, int threadIndex) const;

	CUSTOM_JOINTS_API int GetTireMaterial() const;

	protected:
	void OnTireContactsProcess(const NewtonJoint* const contactJoint, dWheelJoint* const tire, const NewtonBody* const otherBody, dFloat timestep);
	int OnContactGeneration(const dWheelJoint* const tire, const NewtonBody* const otherBody, const NewtonCollision* const othercollision, NewtonUserContactPoint* const contactBuffer, int maxCount, int threadIndex) const;
	
	static int OnTireAabbOverlap(const NewtonJoint* const contactJoint, dFloat timestep, int threadIndex);
	static void OnTireContactsProcess(const NewtonJoint* const contactJoint, dFloat timestep, int threadIndex);
	static int OnContactGeneration (const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonCollision* const collision0, const NewtonBody* const body1, const NewtonCollision* const collision1, NewtonUserContactPoint* const contactBuffer, int maxCount, int threadIndex);

	const void* m_tireShapeTemplateData;
	NewtonCollision* m_tireShapeTemplate;
	int m_tireMaterial;

	friend class dCustomVehicleController;
};

#endif 

