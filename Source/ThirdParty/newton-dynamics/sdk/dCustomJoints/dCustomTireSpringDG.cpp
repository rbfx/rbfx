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

// dCustomTireSpringDG.cpp: implementation of the dCustomTireSpringDG class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomTireSpringDG.h"
#include "dCustomControllerManager.h"

IMPLEMENT_CUSTOM_JOINT(dCustomTireSpringDG);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* dCustomVehicleControllerDG */

void dCustomVehicleControllerDG::Init(NewtonBody* const body, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag)
{
  //dCustomVehicleControllerManagerDG* const manager = (dCustomVehicleControllerManagerDG*)GetManager();
  //NewtonWorld* const world = manager->GetWorld();
  //
}

void dCustomVehicleControllerDG::Init(NewtonCollision* const chassisShape, dFloat mass, const dMatrix& localFrame, NewtonApplyForceAndTorque forceAndTorque, dFloat gravityMag)
{
  //dCustomVehicleControllerManagerDG* const manager = (dCustomVehicleControllerManagerDG*)GetManager();
  //NewtonWorld* const world = manager->GetWorld();
  //

}

void dCustomVehicleControllerDG::PreUpdate(dFloat timestep, int threadIndex)
{
	//dCustomVehicleControllerManagerDG* const manager = (dCustomVehicleControllerManagerDG*)GetManager();
	//
	//Beep(200,400);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* dCustomVehicleControllerManagerDG */

dCustomVehicleControllerManagerDG::dCustomVehicleControllerManagerDG(NewtonWorld* const world, int materialCount, int* const otherMaterials)
  :dCustomControllerManager<dCustomVehicleControllerDG>(world, VEHICLE_DG_PLUGIN_NAME)
{
  //
}

dCustomVehicleControllerManagerDG::~dCustomVehicleControllerManagerDG()
{
  //
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* dCustomTireSpringDG */

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

dCustomTireSpringDG::dCustomTireSpringDG(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent),
	mUseTorque(false),
	mUseBreak(false),
	mUseHardBreak(false),
	mUseSteer(false),
	mRealOmega(0.0f),
	mTireOmegaCorrection(0.975f),
	mFpsRequest(120.0f),
	mDistance(0.0f),
	mAttachMass(0.0f),
    mIxx(0.0f),
    mIyy(0.0f),
    mIzz(0.0f),
	mTireTorque(0.0f),
	mBrakeTorque(0.0f),
	mSteerAngle(0.0f),
	mAttachmentLength(10.0f),
	mMinSuspenssion(-0.25f),
	mMaxSuspenssion(0.0f),
	mRefFrameLocalMatrix(dGetIdentityMatrix()),
	mAccel(0.0f),
	mSpringK(150.0f),
    mSpringD(5.0f),
    mCenterInTire(dVector(0.0f,0.0f,0.0f)),
    mCenterInChassis(dVector(0.0f, 0.0f, 0.0f)),
    mChassisPivotMatrix(dGetIdentityMatrix()),
    mTirePivotMatrix(dGetIdentityMatrix())
{

	// calculate the two local matrix of the pivot point
	CalculateLocalMatrix(pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
	//
	mRefFrameLocalMatrix = m_localMatrix0;
	NewtonBodyGetMass(parent, &mAttachMass, &mIxx, &mIyy, &mIzz);
}

dCustomTireSpringDG::dCustomTireSpringDG(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent)
	:dCustomJoint(6, child, parent),
    mUseTorque(false),
	mUseBreak(false),
    mUseHardBreak(false),
	mUseSteer(false),
	mRealOmega(0.0f),
	mTireOmegaCorrection(0.975f),
	mFpsRequest(120.0f),
    mDistance(0.0f),
    mAttachMass(0.0f),
    mIxx(0.0f),
    mIyy(0.0f),
    mIzz(0.0f),
	mTireTorque(0.0f),
	mBrakeTorque(0.0f),
	mSteerAngle(0.0f),
	mAttachmentLength(10.0f),
	mMinSuspenssion(-0.25f),
	mMaxSuspenssion(0.0f),
    mRefFrameLocalMatrix(dGetIdentityMatrix()),
    mAccel(0.0f),
    mSpringK(150.0f),
	mSpringD(5.0f),
	mCenterInTire(0.0f),
	mCenterInChassis(0.0f),
	mChassisPivotMatrix(dGetIdentityMatrix()),
	mTirePivotMatrix(dGetIdentityMatrix())
{
	dMatrix	dummy;
	CalculateLocalMatrix(pinAndPivotFrameChild, m_localMatrix0, dummy);
	CalculateLocalMatrix(pinAndPivotFrameParent, dummy, m_localMatrix1);
    //
    mRefFrameLocalMatrix = m_localMatrix0;
	NewtonBodyGetMass(parent, &mAttachMass, &mIxx, &mIyy, &mIzz);
}

void dCustomTireSpringDG::SetEngineFpsRequest(dFloat val)
{
	mFpsRequest = val;
}

void dCustomTireSpringDG::SetTireAttachmentLength(dFloat val)
{
	mAttachmentLength = val;
}

void dCustomTireSpringDG::SetTireOmegaCorrection(dFloat val)
{
	mTireOmegaCorrection = val;
}

void dCustomTireSpringDG::SetTireSuspenssion(dFloat tvSpringK, dFloat tvSpringD, dFloat tvMinLimit, dFloat tvMaxLimit)
{
	mSpringK = tvSpringK;
	mSpringD = tvSpringD;
	mMinSuspenssion = tvMinLimit;
	mMaxSuspenssion = tvMaxLimit;
}

bool dCustomTireSpringDG::GetUseHardBreak()
{
	return mUseHardBreak;
}

void dCustomTireSpringDG::SetUseHardBreak(bool val)
{
	mUseHardBreak = val;
}

dFloat dCustomTireSpringDG::GetTireIxx()
{
	return mIxx;
}

dFloat dCustomTireSpringDG::GetTireIyy()
{
	return mIyy;
}

dFloat dCustomTireSpringDG::GetTireIzz()
{
	return mIzz;
}

dFloat dCustomTireSpringDG::GetRealTireOmega()
{
	return mRealOmega;
}

dCustomTireSpringDG::~dCustomTireSpringDG()
{
    //
}
//
dFloat dCustomTireSpringDG::GetTireTorque()
{
	return mTireTorque;
}
//
void dCustomTireSpringDG::SetUseTorque(bool val)
{
	mUseTorque = val;
}
//
bool dCustomTireSpringDG::GetUseTorque()
{
	return mUseTorque;
}
//
bool dCustomTireSpringDG::GetUseSteer()
{
	return mUseSteer;
}
//
bool dCustomTireSpringDG::GetUseBreak()
{
	return mUseBreak;
}
//
void dCustomTireSpringDG::SetTireTorque(dFloat val)
{
	mTireTorque = val;
}

void dCustomTireSpringDG::SetTireBreak(dFloat val)
{
	mBrakeTorque = val;
}

void dCustomTireSpringDG::SetTireSteer(dFloat val)
{
	mSteerAngle = val;
}
//
dFloat dCustomTireSpringDG::GetAccel()
{
	return mAccel;
}

dFloat dCustomTireSpringDG::GetSpringK()
{
	return mSpringK;
}

dFloat dCustomTireSpringDG::GetSpringD()
{
	return mSpringD;
}


dVector dCustomTireSpringDG::GetCenterInTire()
{
	return mCenterInTire;
}

dVector dCustomTireSpringDG::GetCenterInChassis()
{
	return mCenterInChassis;
}

dMatrix dCustomTireSpringDG::GetChassisPivotMatrix()
{
	return mChassisPivotMatrix;
}

dMatrix dCustomTireSpringDG::GetTirePivotMatrix()
{
	return mTirePivotMatrix;
}

void dCustomTireSpringDG::SetAccel(dFloat val)
{
	mAccel = val;
}

void dCustomTireSpringDG::SetSpringK(dFloat val)
{
	mSpringK = val;
}

void dCustomTireSpringDG::SetSpringD(dFloat val)
{
	mSpringD = val;
}

void dCustomTireSpringDG::SetCenterInTire(dVector val)
{
	mCenterInTire = val;
}

void dCustomTireSpringDG::SetCenterInChassis(dVector val)
{
	mCenterInChassis = val;
}

void dCustomTireSpringDG::SetChassisPivotMatrix(dMatrix val)
{
	mChassisPivotMatrix = val;
}

void dCustomTireSpringDG::SetTirePivotMatrix(dMatrix val)
{
	mTirePivotMatrix = val;
}

void dCustomTireSpringDG::SetUseSteer(bool val)
{
	mUseSteer = val;
}

void dCustomTireSpringDG::SetUseBreak(bool val)
{
	mUseBreak = val;
}

void dCustomTireSpringDG::SetDistance()
{
  mDistance = (mChassisPivotMatrix.m_posit - mTirePivotMatrix.m_posit).DotProduct3(mChassisPivotMatrix.m_up);
}

dFloat dCustomTireSpringDG::GetDistance()
{
	return mDistance;
}

void dCustomTireSpringDG::SteeringController(dFloat steptime)
{
	dMatrix srotation;
	//
	if (mUseSteer) {
		//
		if (dAbs(VEHICLE_STEER_ANG - mSteerAngle) > VEHICLE_STEER_ACTIVE) {
			srotation = dYawMatrix(mSteerAngle * VEHICLE_DEGTOANG);
			m_localMatrix0 = (srotation * mRefFrameLocalMatrix);
		}
		//
	}
}

void dCustomTireSpringDG::TireCenterPin(dFloat steptime)
{
	// Wheel constraints simulation.
	mCenterInTire = mTirePivotMatrix.m_posit;
	//
	mCenterInChassis = mChassisPivotMatrix.m_posit + mChassisPivotMatrix.m_up * (mCenterInTire - mChassisPivotMatrix.m_posit).DotProduct3(mChassisPivotMatrix.m_up);
	//
	NewtonUserJointAddLinearRow(m_joint, &mCenterInChassis[0], &mCenterInTire[0], &mChassisPivotMatrix.m_front[0]);
	NewtonUserJointAddLinearRow(m_joint, &mCenterInChassis[0], &mCenterInTire[0], &mChassisPivotMatrix.m_right[0]);
}

void dCustomTireSpringDG::TireCenterBolt(dFloat steptime)
{
	dVector pointInPinInTire;
	dVector pointInPinInChassis;
	//
	pointInPinInTire = mCenterInChassis + (mChassisPivotMatrix.m_front * mAttachmentLength);
	pointInPinInChassis = mCenterInTire + (mTirePivotMatrix.m_front * mAttachmentLength);
	//
	NewtonUserJointAddLinearRow(m_joint, &pointInPinInTire[0], &pointInPinInChassis[0], &mChassisPivotMatrix.m_right[0]);
	NewtonUserJointAddLinearRow(m_joint, &pointInPinInTire[0], &pointInPinInChassis[0], &mChassisPivotMatrix.m_up[0]);
}

void dCustomTireSpringDG::SuspenssionSpringLimits(dFloat steptime)
{
	// Suspenssion limitas
	if (mDistance < mMinSuspenssion) {
		NewtonUserJointAddLinearRow(m_joint, &mCenterInChassis[0], &mCenterInChassis[0], &mChassisPivotMatrix.m_up[0]);
		NewtonUserJointSetRowMinimumFriction(m_joint, -0.0);
	}
	else
	if (mDistance > mMaxSuspenssion) {
		NewtonUserJointAddLinearRow(m_joint, &mCenterInChassis[0], &mCenterInChassis[0], &mChassisPivotMatrix.m_up[0]);
		NewtonUserJointSetRowMaximumFriction(m_joint, 0.0);
	}
}

void dCustomTireSpringDG::TireBreakAction(NewtonBody* const attBody, dFloat steptime)
{
	dMatrix tireMatrix;
	//
	NewtonBodyGetMatrix(attBody, &tireMatrix[0][0]);
	//
	if ((mUseBreak) || (mUseHardBreak)) {
		if (dAbs(mBrakeTorque) > VEHICLE_BREAK_CHECK) {
			NewtonUserJointAddAngularRow(m_joint, 0.0, &tireMatrix.m_front[0]);
			//dFloat relOmega = mRealOmega / steptime;
			// D.G: I get a error when I enable this newton function.
			// D.G: Maybe my mRealOmega calcul is wrong.
			// D.G: The error happen at the begin if you break or randomly, most of time the vehicle is in air when it happen.
			// D.G: I'm not sure if the function is really needed here.
			//NewtonUserJointSetRowAcceleration(m_joint, relOmega);
			//
			NewtonUserJointSetRowMinimumFriction(m_joint, -mBrakeTorque);
			NewtonUserJointSetRowMaximumFriction(m_joint, mBrakeTorque);
		}
		mBrakeTorque = 0.0;
	}
}

void dCustomTireSpringDG::Deserialize(NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &mUseBreak, sizeof(bool));
	callback(userData, &mUseSteer, sizeof(bool));
	callback(userData, &mUseTorque, sizeof(bool));
	callback(userData, &mIxx, sizeof(dFloat));
	callback(userData, &mIyy, sizeof(dFloat));
	callback(userData, &mIzz, sizeof(dFloat));
	callback(userData, &mRealOmega, sizeof(dFloat));
	callback(userData, &mTireOmegaCorrection, sizeof(dFloat));
	callback(userData, &mFpsRequest, sizeof(dFloat));
	callback(userData, &mTireTorque, sizeof(dFloat));
	callback(userData, &mBrakeTorque, sizeof(dFloat));
	callback(userData, &mSteerAngle, sizeof(dFloat));
	callback(userData, &mAttachmentLength, sizeof(dFloat));
	callback(userData, &mDistance, sizeof(dFloat));
	callback(userData, &mMinSuspenssion, sizeof(dFloat));
	callback(userData, &mMaxSuspenssion, sizeof(dFloat));
	callback(userData, &mSpringK, sizeof(dFloat));
	callback(userData, &mSpringD, sizeof(dFloat));
	callback(userData, &mAccel, sizeof(dFloat));
	callback(userData, &mAttachMass, sizeof(dFloat));
	callback(userData, &mCenterInTire, sizeof(dVector));
	callback(userData, &mCenterInChassis, sizeof(dVector));
	callback(userData, &mChassisPivotMatrix, sizeof(dMatrix));
	callback(userData, &mTirePivotMatrix, sizeof(dMatrix));
	callback(userData, &mRefFrameLocalMatrix, sizeof(dMatrix));
}

void dCustomTireSpringDG::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize(callback, userData);
	callback(userData, &mUseBreak, sizeof(bool));
	callback(userData, &mUseSteer, sizeof(bool));
	callback(userData, &mUseTorque, sizeof(bool));
	callback(userData, &mIxx, sizeof(dFloat));
	callback(userData, &mIyy, sizeof(dFloat));
	callback(userData, &mIzz, sizeof(dFloat));
	callback(userData, &mRealOmega, sizeof(dFloat));
	callback(userData, &mTireOmegaCorrection, sizeof(dFloat));
	callback(userData, &mFpsRequest, sizeof(dFloat));
	callback(userData, &mTireTorque, sizeof(dFloat));
	callback(userData, &mBrakeTorque, sizeof(dFloat));
	callback(userData, &mSteerAngle, sizeof(dFloat));
	callback(userData, &mAttachmentLength, sizeof(dFloat));
	callback(userData, &mDistance, sizeof(dFloat));
	callback(userData, &mMinSuspenssion, sizeof(dFloat));
	callback(userData, &mMaxSuspenssion, sizeof(dFloat));
	callback(userData, &mSpringK, sizeof(dFloat));
	callback(userData, &mSpringD, sizeof(dFloat));
	callback(userData, &mAccel, sizeof(dFloat));
	callback(userData, &mAttachMass, sizeof(dFloat));
	callback(userData, &mCenterInTire, sizeof(dVector));
	callback(userData, &mCenterInChassis, sizeof(dVector));
	callback(userData, &mChassisPivotMatrix, sizeof(dMatrix));
	callback(userData, &mTirePivotMatrix, sizeof(dMatrix));
	callback(userData, &mRefFrameLocalMatrix, sizeof(dMatrix));
}

void dCustomTireSpringDG::Debug(dDebugDisplay* const debugDisplay) const
{
	dMatrix matrix0;
	dMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);
	debugDisplay->DrawFrame(matrix0);
	debugDisplay->DrawFrame(matrix1);
	//
	if (m_options.m_option0) {

	}
	//
}

void dCustomTireSpringDG::SubmitConstraints(dFloat timestep, int threadIndex)
{
	NewtonBody* BodyAttach;
	//NewtonBody* BodyFrame;
	//
	dVector tireOmega = dVector(0.0f, 0.0f, 0.0f);
	//BodyFrame = GetBody0();
	BodyAttach = GetBody1();
	//
	SteeringController(timestep);
	//
	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(mChassisPivotMatrix, mTirePivotMatrix);
	//
	NewtonBodyGetOmega(BodyAttach, &tireOmega[0]);
	//
    mRealOmega = dAbs(tireOmega.DotProduct3(mChassisPivotMatrix.m_front));
	//
	TireCenterPin(timestep);
	//
	TireCenterBolt(timestep);
    //
	SuspenssionSpringLimits(timestep);
	//
	TireBreakAction(BodyAttach, timestep);
}

dFloat dCustomTireSpringDG::GetVecLength(dVector const val)
{
	return dSqrt(val.m_x * val.m_x + val.m_y * val.m_y + val.m_z * val.m_z);
}

void dCustomTireSpringDG::TireMatrixProjection()
{
	NewtonBody* tire;
	NewtonBody* chassis;
	//
	dMatrix tireMatrix;
	dMatrix chassisMatrix;
	dMatrix tireMatrixInGlobalSpace;
	dMatrix chassisMatrixInGlobalSpace;
	dMatrix projectedChildMatrixInGlobalSpace;
	//
	dVector projectDist;
	dVector projTireOmega;
	dVector chassisOmega;
	dVector tireOmega;
	dVector tireOmegaCorrection;
	//
	// D.G: Thanks Julio for the helps in the past for this part of code.
	// D.G: This code come from a pretty old vehicle implementation.
    tire = GetBody1();
    chassis = GetBody0();
	//
	NewtonBodyGetMatrix(tire, &tireMatrix[0][0]);
	NewtonBodyGetMatrix(chassis, &chassisMatrix[0][0]);
	//
	// project the tire matrix to the right space.
    tireMatrixInGlobalSpace = (m_localMatrix1 * tireMatrix);
    chassisMatrixInGlobalSpace = (m_localMatrix0 * chassisMatrix);
	//
    projectDist = (tireMatrixInGlobalSpace.m_posit - chassisMatrixInGlobalSpace.m_posit).DotProduct3(chassisMatrixInGlobalSpace.m_up);
	chassisMatrixInGlobalSpace.m_posit = chassisMatrixInGlobalSpace.m_posit + (chassisMatrixInGlobalSpace.m_up * projectDist);
	//
	chassisMatrixInGlobalSpace.m_up = tireMatrixInGlobalSpace.m_right.CrossProduct(chassisMatrixInGlobalSpace.m_front);
	chassisMatrixInGlobalSpace.m_up = chassisMatrixInGlobalSpace.m_up * (1.0f / dSqrt(chassisMatrixInGlobalSpace.m_up.DotProduct3(chassisMatrixInGlobalSpace.m_up)));
	chassisMatrixInGlobalSpace.m_right = chassisMatrixInGlobalSpace.m_front.CrossProduct(chassisMatrixInGlobalSpace.m_up);
	chassisMatrixInGlobalSpace.m_up.m_w = 0.0;
	chassisMatrixInGlobalSpace.m_right.m_w = 0.0;
	//
    projectedChildMatrixInGlobalSpace = (m_localMatrix1.Inverse() * chassisMatrixInGlobalSpace);
	NewtonBodySetMatrix(tire, &projectedChildMatrixInGlobalSpace[0][0]);
	//
	NewtonBodyGetOmega(chassis, &chassisOmega[0]);
	// D.G: Temporary disabled.
	// D.G: The result is a lot better without this part.
	// D.G: Causing problems, something is calculed wrong and the result give some bad bounce force on the wheels.
	//
	/*
	NewtonBodyGetCentreOfMass(chassis, @chassisCom.V[0]);
	NewtonBodyGetMatrix(chassis, @rlmat.V[0].V[0]);

    chassisCom: = OXTransformVector(chassisCom, rlmat);
    chassisVeloc: = VectorAdd(chassisVeloc, VectorCrossProduct(chassisOmega, VectorSubtract(chassisMatrixInGlobalSpace.m_posit, chassisCom)));

    projTireVeloc: = VectorSubtract{ VectorAdd }(chassisVeloc, VectorScale(chassisMatrixInGlobalSpace.m_up, VectorDotProduct(chassisVeloc, chassisMatrixInGlobalSpace.m_up)));
    projTireVeloc: = VectorAdd{ VectorSubtract }(projTireVeloc, VectorScale(chassisMatrixInGlobalSpace.m_up, VectorDotProduct(tireVeloc, chassisMatrixInGlobalSpace.m_up)));
	//NegateVector(projTireVeloc);
	NewtonBodySetVelocity(tire, @projTireVeloc.V[0]);
	*/
	// project angular velocity
	NewtonBodyGetOmega(tire, &tireOmega[0]);
    tireOmegaCorrection = tireOmega;
	//
	if (GetVecLength(tireOmegaCorrection) > mFpsRequest) {
      // I need to use this omega correction fix when NewtonBodySetLinearDamping,NewtonBodySetAngularDamping is set to zero.
      // Because the tire rotation overpass the physics rotation limit in time, and it can give bad result without this fix.  
	  // I prefered to have the body totally free rolling, because of this I need this fix.
	  // If you don't use this fix, Don't set the NewtonBodySetLinearDamping,NewtonBodySetAngularDamping to zero value, just don't call this both functions on the body at all. 
	  //
	  //printf("Omega tire correction, Fix a Rotation limitation. \n");
	  tireOmegaCorrection = (tireOmegaCorrection * mTireOmegaCorrection);
	  NewtonBodySetOmega(tire, &tireOmegaCorrection[0]);
	}
	//
    projTireOmega = chassisOmega - chassisMatrixInGlobalSpace.m_front * (chassisOmega.DotProduct3(chassisMatrixInGlobalSpace.m_front));
    projTireOmega = projTireOmega + chassisMatrixInGlobalSpace.m_front * (tireOmegaCorrection.DotProduct3(chassisMatrixInGlobalSpace.m_front));
	NewtonBodySetOmega(tire, &projTireOmega[0]);
}
