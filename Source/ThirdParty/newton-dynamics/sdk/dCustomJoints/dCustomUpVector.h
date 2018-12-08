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


// dCustomUpVector.h: interface for the dCustomUpVector class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOMUPVECTOR_H_
#define _CUSTOMUPVECTOR_H_

#include "dCustomJoint.h"

// This joint is useful to for implementing character controllers, and also precise object picking
class dCustomUpVector: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dCustomUpVector(const dVector& pin, NewtonBody* child);
	CUSTOM_JOINTS_API virtual ~dCustomUpVector();

	CUSTOM_JOINTS_API void SetPinDir (const dVector& pin);

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
};

#endif // !defined(AFX_CUSTOMUPVECTOR_H__EAE1E36C_6FDF_4D86_B4EE_855E3D1046F4_H)

