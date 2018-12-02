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


#ifndef _D_CUSTOM_BALLANDSOCKET_H_
#define _D_CUSTOM_BALLANDSOCKET_H_

#include "dCustomJoint.h"

class dCustomBallAndSocket: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomBallAndSocket(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomBallAndSocket(const dMatrix& pinAndPivotFrame0, const dMatrix& pinAndPivotFrame1, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomBallAndSocket();

	CUSTOM_JOINTS_API void EnableTwist(bool state);
	CUSTOM_JOINTS_API void SetTwistLimits(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void GetTwistLimits(dFloat& minAngle, dFloat& maxAngle) const;

	CUSTOM_JOINTS_API void EnableCone(bool state);
	CUSTOM_JOINTS_API dFloat GetConeLimits() const;
	CUSTOM_JOINTS_API void SetConeLimits(dFloat maxAngle);

	CUSTOM_JOINTS_API void SetTwistFriction (dFloat frictionTorque);
	CUSTOM_JOINTS_API dFloat GetTwistFriction (dFloat frictionTorque) const;

	CUSTOM_JOINTS_API void SetConeFriction(dFloat frictionTorque);
	CUSTOM_JOINTS_API dFloat GetConeFriction(dFloat frictionTorque) const;

	CUSTOM_JOINTS_API void SetTwistSpringDamper(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper);
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	
	protected:
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	void SubmitConstraintTwistLimits(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& relOmega, dFloat timestep);

	dAngularIntegration m_twistAngle;
	dFloat m_minTwistAngle;
	dFloat m_maxTwistAngle;
	dFloat m_maxConeAngle;
	dFloat m_coneFriction;
	dFloat m_twistFriction;

	DECLARE_CUSTOM_JOINT(dCustomBallAndSocket, dCustomJoint)
};

#endif 

