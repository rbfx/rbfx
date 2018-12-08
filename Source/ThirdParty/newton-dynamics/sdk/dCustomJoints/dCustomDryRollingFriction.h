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


// CustomUpVector.h: interface for the CustomUpVector class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DRY_ROLLOING_FRICTION3D1046F4_
#define _DRY_ROLLOING_FRICTION3D1046F4_


#include "dCustomJoint.h"


// this joint is usefully to simulate the rolling friction of a rolling ball over 
// a flat surface.
// normally this is not important for non spherical objects, but for games like 
// poll, pinball, bolling, golf or any other where the movement of balls is the main objective
// the rolling friction is a real big problem.
class dCustomDryRollingFriction: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomDryRollingFriction(NewtonBody* const child, dFloat radius, dFloat coefficient);
	CUSTOM_JOINTS_API virtual ~dCustomDryRollingFriction();

	protected:
	//CUSTOM_JOINTS_API dCustomDryRollingFriction (NewtonBody* const child, NewtonBody* const parent, NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const; 

	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);

	dFloat m_frictionCoef;
	dFloat m_frictionTorque;

	DECLARE_CUSTOM_JOINT(dCustomDryRollingFriction, dCustomJoint)

};



#endif // !defined(AFX_DRY_ROLLOING_FRICTION3D1046F4_H)

