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


//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_TRANSFORM_MANAGER_H_
#define D_CUSTOM_TRANSFORM_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomControllerManager.h"

class dNewton;

// a Skeleton Transform controller is use to calculate local transform on contractions of rigid bodies and joint that form part of a hierarchical Skeleton
class dNewtonTransformController: public dCustomControllerBase
{
	private:
	CNEWTON_API virtual void PreUpdate(dFloat timestep, int threadIndex){}
	CNEWTON_API virtual void PostUpdate(dFloat timestep, int threadIndex){}
};

class dNewtonTransformManager: public dCustomControllerManager<dNewtonTransformController> 
{
	public:
	CNEWTON_API dNewtonTransformManager (dNewton* const world);
	CNEWTON_API virtual ~dNewtonTransformManager();

	private:
	CNEWTON_API virtual void Debug () const {};
	CNEWTON_API virtual void PreUpdate(dFloat timestep){};
	CNEWTON_API virtual void PostUpdate (dFloat timestep);

	static void UpdateTransformKernel (NewtonWorld* const world, void* const context, int threadIndex);
};


#endif 

