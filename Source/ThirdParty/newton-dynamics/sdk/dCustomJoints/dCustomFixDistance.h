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



#ifndef _D_CUSTOM_FIX_DISTANCE_H_
#define _D_CUSTOM_FIX_DISTANCE_H_

#include "dCustom6dof.h"


class dCustomFixDistance: public dCustomJoint  
{
	public:
	CUSTOM_JOINTS_API dCustomFixDistance(const dVector& pivotFrame0, const dVector& pivotFrame1, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomFixDistance();

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);

	dFloat m_distance;
	DECLARE_CUSTOM_JOINT(dCustomFixDistance, dCustomJoint)
};

#endif 

