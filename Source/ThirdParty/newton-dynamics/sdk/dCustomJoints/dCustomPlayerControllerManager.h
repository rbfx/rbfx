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


// NewtonPlayerControllerManager.h: interface for the NewtonPlayerControllerManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_PLAYER_CONTROLLER_MANAGER_H_
#define D_CUSTOM_PLAYER_CONTROLLER_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomControllerManager.h"


#define PLAYER_PLUGIN_NAME				"__playerManager__"
#define PLAYER_CONTROLLER_MAX_CONTACTS	32
#define PLAYER_MIN_RESTRAINING_DISTANCE	1.0e-2f

class dCustomPlayerController: public dCustomControllerBase
{
	public:
	CUSTOM_JOINTS_API dCustomPlayerController();
	CUSTOM_JOINTS_API ~dCustomPlayerController();

	CUSTOM_JOINTS_API void Init(dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stepHigh, const dMatrix& localAxis);

	dFloat GetHigh() const 
	{
		return m_height;
	}

	const dVector GetUpDir () const
	{
		return m_upVector;	
	}
	const dVector& GetGroundPlane() const
	{
		return m_groundPlane;
	}

	bool IsInFreeFall() const
	{
		return m_isJumping;
	}

	void SetRestrainingDistance (dFloat distance)
	{
		m_restrainingDistance = dMax (dAbs (distance), dFloat(PLAYER_MIN_RESTRAINING_DISTANCE));
	}

	void SetClimbSlope (dFloat slopeInRadians)
	{
		m_maxSlope = dCos (dAbs(slopeInRadians));
	}

	virtual void PreUpdate(dFloat timestep, int threadIndex)
	{
	}

	CUSTOM_JOINTS_API virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	CUSTOM_JOINTS_API void SetPlayerOrigin (dFloat originHigh);

	CUSTOM_JOINTS_API dVector CalculateDesiredOmega (dFloat headingAngle, dFloat timestep) const;
	CUSTOM_JOINTS_API dVector CalculateDesiredVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, const dVector& gravity, dFloat timestep) const;
	
	CUSTOM_JOINTS_API virtual void PostUpdate(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API void SetPlayerVelocity (dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat headingAngle, const dVector& gravity, dFloat timestep);

	private:
	void UpdateGroundPlane (dMatrix& matrix, const dMatrix& castMatrix, const dVector& target, int threadIndex);
	dFloat CalculateContactKinematics(const dVector& veloc, const NewtonWorldConvexCastReturnInfo* const contact) const;

	dVector m_upVector;
	dVector m_frontVector;
	dVector m_groundPlane;
	dVector m_groundVelocity;
	dFloat m_outerRadio;
	dFloat m_innerRadio;
	dFloat m_height;
	dFloat m_stairStep;
	dFloat m_maxSlope;
	dFloat m_sphereCastOrigin;
	dFloat m_restrainingDistance;
	bool m_isJumping;
	NewtonCollision* m_castingShape;
	NewtonCollision* m_supportShape;
	NewtonCollision* m_upperBodyShape;
};


class dCustomPlayerControllerManager: public dCustomControllerManager<dCustomPlayerController>
{
	public:
	CUSTOM_JOINTS_API dCustomPlayerControllerManager(NewtonWorld* const world);
	CUSTOM_JOINTS_API ~dCustomPlayerControllerManager();

	virtual void PreUpdate(dFloat timestep)
	{
	}

	CUSTOM_JOINTS_API virtual void ApplyPlayerMove (dCustomPlayerController* const controller, dFloat timestep) = 0; 

	CUSTOM_JOINTS_API virtual dCustomPlayerController* CreatePlayer (dFloat mass, dFloat outerRadius, dFloat innerRadius, dFloat height, dFloat stairStep, const dMatrix& localAxis);
	CUSTOM_JOINTS_API virtual int ProcessContacts (const dCustomPlayerController* const controller, NewtonWorldConvexCastReturnInfo* const contacts, int count) const; 
};

#endif 

