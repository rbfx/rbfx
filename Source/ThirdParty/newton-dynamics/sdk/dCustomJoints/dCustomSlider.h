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


// dCustomSlider.h: interface for the dCustomSlider class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_SLIDER_H__
#define _CUSTOM_SLIDER_H__

#include "dCustomJoint.h"

class dCustomSlider: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomSlider (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomSlider (const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomSlider();

	CUSTOM_JOINTS_API void EnableLimits(bool state);
	CUSTOM_JOINTS_API void SetLimits(dFloat mindist, dFloat maxdist);
	CUSTOM_JOINTS_API void SetAsSpringDamper(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper);

	CUSTOM_JOINTS_API dFloat GetJointPosit () const;
	CUSTOM_JOINTS_API dFloat GetJointSpeed () const;
	
	CUSTOM_JOINTS_API dFloat GetFriction () const;
	CUSTOM_JOINTS_API void SetFriction (dFloat friction);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData); 
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);

	protected:
	CUSTOM_JOINTS_API virtual void SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	void SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& p0, const dVector& p1, dFloat timestep);
	void SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& p0, const dVector& p1, dFloat timestep);
	void SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& p0, const dVector& p1, dFloat timestep);

	dFloat m_speed;
	dFloat m_posit;
	dFloat m_minDist;
	dFloat m_maxDist;
	dFloat m_friction;
	dFloat m_spring;
	dFloat m_damper;
	dFloat m_springDamperRelaxation;
	DECLARE_CUSTOM_JOINT(dCustomSlider, dCustomJoint)
};

#endif

