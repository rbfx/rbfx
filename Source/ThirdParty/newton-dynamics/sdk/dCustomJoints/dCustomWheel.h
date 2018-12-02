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



// dCustomWheel.h: interface for the dCustomWheel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_WHEEL_H_
#define _CUSTOM_WHEEL_H_

#include "dCustomSlidingContact.h"

class dCustomWheel: public dCustomSlidingContact
{
	public:
	CUSTOM_JOINTS_API dCustomWheel(const dMatrix& pinAndPivotFrame, NewtonBody* child, NewtonBody* parent = NULL);
	CUSTOM_JOINTS_API dCustomWheel(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomWheel();

	CUSTOM_JOINTS_API dFloat GetSteerAngle() const;
	CUSTOM_JOINTS_API dFloat GetTargetSteerAngle() const;
	CUSTOM_JOINTS_API void SetTargetSteerAngle(dFloat angle);

	CUSTOM_JOINTS_API dFloat GetSteerRate() const;
	CUSTOM_JOINTS_API void SetSteerRate(dFloat rate);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitAnglarStructuralRows(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	dFloat m_steerAngle;
	dFloat m_steerSpeed;
	dFloat m_currentSteerAngle;

	DECLARE_CUSTOM_JOINT(dCustomWheel, dCustomSlidingContact)


};


#endif 

