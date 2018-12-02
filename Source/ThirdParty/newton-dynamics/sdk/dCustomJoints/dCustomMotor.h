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


// dCustomMotor.h: interface for the dCustomMotor class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CustomMotor_H__
#define _CustomMotor_H__

#include "dCustomJoint.h"

class dCustomMotor: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dCustomMotor(const dVector& pin, NewtonBody* const body, NewtonBody* const referenceBody);
	CUSTOM_JOINTS_API virtual ~dCustomMotor();

	CUSTOM_JOINTS_API dFloat GetSpeed() const;
	CUSTOM_JOINTS_API void SetSpeed(dFloat speed);
	CUSTOM_JOINTS_API void SetTorque(dFloat torque);

	protected:
	CUSTOM_JOINTS_API dCustomMotor(int dof, const dVector& pin, NewtonBody* const body, NewtonBody* const referenceBody);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);

	dVector m_localReferencePin;
	dFloat m_motorOmega;
	dFloat m_targetSpeed;
	dFloat m_motorTorque;
	NewtonBody* m_referenceBody;
	DECLARE_CUSTOM_JOINT(dCustomMotor, dCustomJoint)
};


class dCustomMotor2 : public dCustomMotor
{
	public:
	CUSTOM_JOINTS_API dCustomMotor2(const dVector& pin0, const dVector& pin1, NewtonBody* const body, NewtonBody* const referenceBody0, NewtonBody* const referenceBody1);
	CUSTOM_JOINTS_API virtual ~dCustomMotor2();

	CUSTOM_JOINTS_API dFloat GetSpeed1() const;
	CUSTOM_JOINTS_API void SetSpeed1(dFloat speed);
	CUSTOM_JOINTS_API void SetTorque1(dFloat torque);

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);

	dVector m_localReferencePin1;
	dFloat m_motorOmega1;
	dFloat m_targetSpeed1;
	dFloat m_motorTorque1;
	NewtonBody* m_referenceBody1;
	DECLARE_CUSTOM_JOINT(dCustomMotor2, dCustomMotor)
};


#endif 

