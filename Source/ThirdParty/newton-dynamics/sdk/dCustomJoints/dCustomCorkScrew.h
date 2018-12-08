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


// dCustomCorkScrew.h: interface for the dCustomCorkScrew class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOMCORKSCREW_H_
#define _CUSTOMCORKSCREW_H_

#include "dCustomSlider.h"

class dCustomCorkScrew: public dCustomSlider
{
	public:
	CUSTOM_JOINTS_API dCustomCorkScrew (const dMatrix& pinAndPivotFrame, NewtonBody* child, NewtonBody* parent = NULL);
	CUSTOM_JOINTS_API dCustomCorkScrew (const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomCorkScrew();

	CUSTOM_JOINTS_API void EnableAngularLimits(bool state);
	CUSTOM_JOINTS_API void SetAngularLimits(dFloat minAngle, dFloat maxAngle);

	CUSTOM_JOINTS_API void SetAngularSpringDamper(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 

	CUSTOM_JOINTS_API virtual void SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);
	void SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	dAngularIntegration m_curJointAngle;
	dFloat m_minAngle;
	dFloat m_maxAngle;
	dFloat m_angularFriction;
	dFloat m_angularOmega;

	dFloat m_angularSpring;
	dFloat m_angularDamper;
	dFloat m_angularSpringDamperRelaxation;

	DECLARE_CUSTOM_JOINT(dCustomCorkScrew, dCustomSlider)
};

#endif

