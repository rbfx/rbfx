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



// dCustomKinematicController.h: interface for the dCustomKinematicController class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CUSTOM_KINEMATIC_CONTROLLER_H__
#define __CUSTOM_KINEMATIC_CONTROLLER_H__


#include "dCustomJoint.h"

class dCustomKinematicController: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dCustomKinematicController (NewtonBody* const body, const dVector& attachmentPointInGlobalSpace);
	CUSTOM_JOINTS_API dCustomKinematicController (NewtonBody* const body, const dMatrix& attachmentMatrixInGlobalSpace);
	CUSTOM_JOINTS_API dCustomKinematicController (NewtonInverseDynamics* const invDynSolver, void* const invDynNode, const dMatrix& attachmentMatrixInGlobalSpace);
	CUSTOM_JOINTS_API virtual ~dCustomKinematicController();

	CUSTOM_JOINTS_API void SetPickMode (int mode);
	CUSTOM_JOINTS_API void SetMaxLinearFriction(dFloat force); 
	CUSTOM_JOINTS_API void SetMaxAngularFriction(dFloat torque); 
	CUSTOM_JOINTS_API void SetLimitRotationVelocity(dFloat omegaCap);
	
	CUSTOM_JOINTS_API void SetTargetPosit (const dVector& posit); 
	CUSTOM_JOINTS_API void SetTargetRotation (const dQuaternion& rotation); 
	CUSTOM_JOINTS_API void SetTargetMatrix (const dMatrix& matrix); 

	CUSTOM_JOINTS_API void ResetAutoSleep();

	CUSTOM_JOINTS_API dMatrix GetBodyMatrix () const;
	CUSTOM_JOINTS_API dMatrix GetTargetMatrix () const;
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData); 
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const;

	void Init (NewtonBody* const body, const dMatrix& matrix);

	dMatrix m_targetMatrix;
	dFloat m_maxLinearFriction;
	dFloat m_maxAngularFriction;
	dFloat m_omegaCap;
	char m_pickingMode;
	char m_autoSleepState;

	DECLARE_CUSTOM_JOINT(dCustomKinematicController, dCustomJoint)
};

#endif 

