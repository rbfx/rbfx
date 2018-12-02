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


// dCustomDoubleHinge.h: interface for the dCustomDoubleHinge class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_DOUBLE_HINGE_H_
#define _CUSTOM_DOUBLE_HINGE_H_

#include "dCustomHinge.h"

class dCustomDoubleHinge: public dCustomHinge
{
	public:
	CUSTOM_JOINTS_API dCustomDoubleHinge(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomDoubleHinge(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomDoubleHinge();

	CUSTOM_JOINTS_API void EnableLimits1(bool state);
	CUSTOM_JOINTS_API void SetLimits1(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API dFloat GetJointAngle1() const;
	CUSTOM_JOINTS_API dVector GetPinAxis1() const;
	CUSTOM_JOINTS_API dFloat GetJointOmega1() const;
	CUSTOM_JOINTS_API dFloat GetFriction1() const;
	CUSTOM_JOINTS_API void SetFriction1(dFloat frictionTorque);
	CUSTOM_JOINTS_API void SetAsSpringDamper1(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper);

	CUSTOM_JOINTS_API void SetHardMiddleAxis(bool state);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	protected:

	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& eulers, dFloat timestep);

	void SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	dAngularIntegration m_curJointAngle1;
	dFloat m_minAngle1;
	dFloat m_maxAngle1;
	dFloat m_friction1;
	dFloat m_jointOmega1;
	dFloat m_spring1;
	dFloat m_damper1;
	dFloat m_springDamperRelaxation1;

	DECLARE_CUSTOM_JOINT(dCustomDoubleHinge, dCustomHinge)
};



#endif 

