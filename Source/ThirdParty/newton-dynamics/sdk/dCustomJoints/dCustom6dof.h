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


// dCustom6DOF.h: interface for the dCustom6DOF class.
//////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_6DOF_H_
#define _CUSTOM_6DOF_H_

#include "dCustomJoint.h"

class dCustom6dof: public dCustomJoint  
{
	class dAngleData
	{
		public:
		dAngleData()
			:m_currentAngle()
			,m_minAngle(-0.0f)
			,m_maxAngle(0.0f)
		{
		}

		dAngularIntegration m_currentAngle;
		dFloat m_minAngle;
		dFloat m_maxAngle;
	};

	public:
	CUSTOM_JOINTS_API dCustom6dof (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustom6dof (const dMatrix& pinAndPivotChildFrame, const dMatrix& pinAndPivotParentFrame,  NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustom6dof();

	CUSTOM_JOINTS_API void DisableAxisX();
	CUSTOM_JOINTS_API void DisableAxisY();
	CUSTOM_JOINTS_API void DisableAxisZ();
	CUSTOM_JOINTS_API void DisableRotationX();
	CUSTOM_JOINTS_API void DisableRotationY();
	CUSTOM_JOINTS_API void DisableRotationZ();

	CUSTOM_JOINTS_API void GetLinearLimits (dVector& minLinearLimits, dVector& maxLinearLimits) const;
	CUSTOM_JOINTS_API void SetLinearLimits (const dVector& minLinearLimits, const dVector& maxLinearLimits);

	CUSTOM_JOINTS_API void SetYawLimits(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void SetRollLimits(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void SetPitchLimits(dFloat minAngle, dFloat maxAngle);

	CUSTOM_JOINTS_API void GetYawLimits(dFloat& minAngle, dFloat& maxAngle) const;
	CUSTOM_JOINTS_API void GetRollLimits(dFloat& minAngle, dFloat& maxAngle) const;
	CUSTOM_JOINTS_API void GetPitchLimits(dFloat& minAngle, dFloat& maxAngle) const;

	CUSTOM_JOINTS_API dFloat GetYaw() const { return m_yaw.m_currentAngle.GetAngle();}
	CUSTOM_JOINTS_API dFloat GetRoll() const { return m_roll.m_currentAngle.GetAngle();}
	CUSTOM_JOINTS_API dFloat GetPitch() const { return m_pitch.m_currentAngle.GetAngle();}

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints (dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize (NewtonSerializeCallback callback, void* const userData) const;

	private:
	void CalculateJointAngles(const dMatrix& matrix0, const dMatrix& matrix1);
	
	protected:
	dVector m_minLinearLimits;
	dVector m_maxLinearLimits;
	dAngleData m_pitch;
	dAngleData m_yaw;
	dAngleData m_roll;

	DECLARE_CUSTOM_JOINT(dCustom6dof, dCustomJoint)
};

#endif

