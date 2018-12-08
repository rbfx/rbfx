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

/*
* Vehicle Multi body Joint write by Dave Gravel 2018.
* I have write this vehicle multi body code for share with other newton users, and maybe it can help someone.
* Have fun!!!
* 
* Informations, Problems:
* You can contact me from the newton forum http://newtondynamics.com/forum/index.php
* Or you can message me on https://www.facebook.com/dave.gravel1
* My youtube channel https://www.youtube.com/user/EvadLevarg/videos
*/

// dCustomTireSpringDG.h: interface for the dCustomTireSpringDG class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOMTIRESPRINGDG_H_
#define _CUSTOMTIRESPRINGDG_H_

#include "dCustomJoint.h"
#include "dCustomAlloc.h"
#include "dCustomControllerManager.h"

#define VEHICLE_DG_PLUGIN_NAME			"__davegravelvehicleMan__"

const dFloat VEHICLE_DEGTOANG = 3.141592f / 180.0f;
const dFloat VEHICLE_ANGTODEG = 180.0f / 3.141592f;
const dFloat VEHICLE_DEGTORAD = 1.0f / VEHICLE_ANGTODEG;
const dFloat VEHICLE_STEER_ANG = 90.0f;
const dFloat VEHICLE_STEER_ACTIVE = 1.0e-4f;
const dFloat VEHICLE_BREAK_CHECK = 1.0e-3f;
const dFloat VEHICLE_ATTACH_FORCE_SIDE = -1;

class dCustomVehicleControllerDG : public dCustomControllerBase
{
	public:
	CUSTOM_JOINTS_API void Init(NewtonBody* const body, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);
	CUSTOM_JOINTS_API void Init(NewtonCollision* const chassisShape, dFloat mass, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag);

	protected:
	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep, int threadIndex);
	virtual void PostUpdate(dFloat timestep, int threadIndex) {};
	//
	friend class dCustomVehicleControllerManager;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class dCustomVehicleControllerManagerDG : public dCustomControllerManager<dCustomVehicleControllerDG>
{
	public:
	CUSTOM_JOINTS_API dCustomVehicleControllerManagerDG(NewtonWorld* const world, int materialCount, int* const otherMaterials);
	CUSTOM_JOINTS_API virtual ~dCustomVehicleControllerManagerDG();
	//
	friend class dCustomVehicleControllerDG;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class dCustomTireSpringDG : public dCustomJoint
{
public:
	CUSTOM_JOINTS_API dCustomTireSpringDG(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomTireSpringDG(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomTireSpringDG();
	CUSTOM_JOINTS_API void SetUseSteer(bool val);
	CUSTOM_JOINTS_API void SetUseBreak(bool val);
	CUSTOM_JOINTS_API void SetUseHardBreak(bool val);
	CUSTOM_JOINTS_API bool GetUseHardBreak();
	CUSTOM_JOINTS_API bool GetUseSteer();
	CUSTOM_JOINTS_API bool GetUseBreak();
	CUSTOM_JOINTS_API void SetUseTorque(bool val);
	CUSTOM_JOINTS_API bool GetUseTorque();
	CUSTOM_JOINTS_API void SetEngineFpsRequest(dFloat val);
	CUSTOM_JOINTS_API void SetTireTorque(dFloat val);
	CUSTOM_JOINTS_API void SetTireBreak(dFloat val);
	CUSTOM_JOINTS_API void SetTireSteer(dFloat val);
	CUSTOM_JOINTS_API void SetTireAttachmentLength(dFloat val);
	CUSTOM_JOINTS_API void SetTireOmegaCorrection(dFloat val);
	CUSTOM_JOINTS_API void SetTireSuspenssion(dFloat tvSpringK, dFloat tvSpringD, dFloat tvSpringMassEffective, dFloat tvMinLimit, dFloat tvMaxLimit);
	//
	CUSTOM_JOINTS_API dFloat GetTireTorque();
	CUSTOM_JOINTS_API dFloat GetTireIxx();
	CUSTOM_JOINTS_API dFloat GetTireIyy();
	CUSTOM_JOINTS_API dFloat GetTireIzz();

	//
	// Public because it is use inside the Listener
	// I don't think they need to become implemented for the dll interface.
	CUSTOM_JOINTS_API void SetDistance();
	CUSTOM_JOINTS_API void SetAccel(dFloat val);
	CUSTOM_JOINTS_API void SetSpringK(dFloat val);
	CUSTOM_JOINTS_API void SetSpringD(dFloat val);
	CUSTOM_JOINTS_API void SetSpringMassEffective(dFloat val);
	//
	CUSTOM_JOINTS_API void SetCenterInTire(dVector val);
	CUSTOM_JOINTS_API void SetCenterInChassis(dVector val);
	CUSTOM_JOINTS_API void SetChassisPivotMatrix(dMatrix val);
	CUSTOM_JOINTS_API void SetTirePivotMatrix(dMatrix val);
	CUSTOM_JOINTS_API void TireMatrixProjection();
	//
	CUSTOM_JOINTS_API dFloat GetDistance();
	CUSTOM_JOINTS_API dFloat GetRealTireOmega();
	CUSTOM_JOINTS_API dFloat GetAccel();
	CUSTOM_JOINTS_API dFloat GetSpringK();
	CUSTOM_JOINTS_API dFloat GetSpringD();
	CUSTOM_JOINTS_API dFloat GetSpringMassEffective();
	//
	CUSTOM_JOINTS_API dVector GetCenterInTire();
	CUSTOM_JOINTS_API dVector GetCenterInChassis();
	CUSTOM_JOINTS_API dMatrix GetChassisPivotMatrix();
	CUSTOM_JOINTS_API dMatrix GetTirePivotMatrix();
	//
protected:
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	//
	dFloat GetVecLength(dVector const val);
	//
	void SteeringController(dFloat steptime);
	void TireCenterPin(dFloat steptime);
	void TireCenterBolt(dFloat steptime);
	void SuspenssionSpringLimits(dFloat steptime);
	void TireBreakAction(NewtonBody* const attBody, dFloat steptime);
	//
	bool mUseTorque;
	bool mUseBreak;
	bool mUseHardBreak;
	bool mUseSteer;
	//
	dFloat mRealOmega;
	dFloat mTireOmegaCorrection;
	dFloat mFpsRequest;
	dFloat mDistance;
	dFloat mAttachMass;
	dFloat mIxx;
	dFloat mIyy;
	dFloat mIzz;
	dFloat mTireTorque;
	dFloat mBrakeTorque;
	dFloat mSteerAngle;
	dFloat mAttachmentLength;
	dFloat mMinSuspenssion;
	dFloat mMaxSuspenssion;
	dMatrix mRefFrameLocalMatrix;
	// Use by the listener
	dFloat mAccel;
	dFloat mSpringK;
	dFloat mSpringD;
	dFloat mSpringMassEffective;
	dVector mCenterInTire;
	dVector mCenterInChassis;
	dMatrix mChassisPivotMatrix;
	dMatrix mTirePivotMatrix;
	//
	DECLARE_CUSTOM_JOINT(dCustomTireSpringDG, dCustomJoint)
};

#endif 