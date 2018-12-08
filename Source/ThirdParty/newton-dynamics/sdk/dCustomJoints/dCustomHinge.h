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

// dCustomHinge.h: interface for the dCustomHinge class.
//
//////////////////////////////////////////////////////////////////////


#ifndef _CUSTOMHINGE_H_
#define _CUSTOMHINGE_H_

#include "dCustomJoint.h"

class dCustomHinge: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomHinge (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomHinge (const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomHinge();

	CUSTOM_JOINTS_API void EnableLimits(bool state);
	CUSTOM_JOINTS_API void SetLimits(dFloat minAngle, dFloat maxAngle);

	CUSTOM_JOINTS_API dFloat GetJointAngle () const;
	CUSTOM_JOINTS_API dVector GetPinAxis () const;
	CUSTOM_JOINTS_API dFloat GetJointOmega () const;

	CUSTOM_JOINTS_API dFloat GetFriction () const;	
	CUSTOM_JOINTS_API void SetFriction (dFloat frictionTorque);

	CUSTOM_JOINTS_API void EnableMotor(bool state, dFloat motorSpeed);
	CUSTOM_JOINTS_API void SetAsSpringDamper(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData); 
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 
	CUSTOM_JOINTS_API virtual void SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& eulers, dFloat timestep);
	
	void SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	
	dAngularIntegration m_curJointAngle;
	dFloat m_minAngle;
	dFloat m_maxAngle;
	dFloat m_friction;
	dFloat m_jointOmega;
	dFloat m_spring;
	dFloat m_damper;
	dFloat m_motorSpeed;
	dFloat m_springDamperRelaxation;
	bool m_limitReached;
	DECLARE_CUSTOM_JOINT(dCustomHinge, dCustomJoint)
};

#endif 

