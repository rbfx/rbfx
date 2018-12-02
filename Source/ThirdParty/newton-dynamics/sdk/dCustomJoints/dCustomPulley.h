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


// dCustomPulley.h: interface for the dCustomPulley class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CustomPulley_H__
#define _CustomPulley_H__

#include "dCustomJoint.h"

// this joint is for used in conjunction with Slider of other linear joints
// is is useful for establishing synchronization between the rel;ative position 
// or relative linear velocity of two sliding bodies according to the law of pulley
// velErro = -(v0 + n * v1)
// where v0 and v1 are the linear velocity
// n is the number of turn on the pulley, in the case of this joint N coudl
// be a value with fraction, as this joint is a generalization of the pulley idea.
class dCustomPulley: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomPulley(dFloat pulleyRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const parenPin, NewtonBody* const parent);
	CUSTOM_JOINTS_API virtual ~dCustomPulley();

	protected:
	//CUSTOM_JOINTS_API dCustomPulley (NewtonBody* const child, NewtonBody* const parent, NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 

	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);

	dFloat m_gearRatio;
	DECLARE_CUSTOM_JOINT(dCustomPulley, dCustomJoint)
};

/*
class CustomPulleyAndTwist: public dCustomPulley
{
	public:
	CUSTOM_JOINTS_API CustomPulleyAndTwist(dFloat pulleyRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const parenPin, NewtonBody* const parent);
	CUSTOM_JOINTS_API virtual ~dCustomPulley();

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);

	dFloat m_gearRatio;
};
*/

#endif 

