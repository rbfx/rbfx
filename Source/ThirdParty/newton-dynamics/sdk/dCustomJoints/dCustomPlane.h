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



// dCustomPlane.h: interface for the dCustomPlane class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_PLANE_H_
#define _CUSTOM_PLANE_H_

#include "dCustomJoint.h"


class dCustomPlane: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomPlane (const dVector& pivot, const dVector& normal, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomPlane();

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData); 
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	
	DECLARE_CUSTOM_JOINT(dCustomPlane, dCustomJoint)
	//DECLARE_CUSTOM_JOINT_EXPORT_IMPORT(PINT_JOINTS_API, Plane3DOF, dCustom6dof);
};




#endif 

