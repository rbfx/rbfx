/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/


#ifndef _CUSTOM_HINGE_ACTUATOR_H_
#define _CUSTOM_HINGE_ACTUATOR_H_

#include "dCustomJoint.h"
#include "dCustomHinge.h"

class dCustomHingeActuator: public dCustomHinge
{
	public:
	CUSTOM_JOINTS_API dCustomHingeActuator(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API dCustomHingeActuator(const dMatrix& pinAndPivotFrame, dFloat angularRate, dFloat minAngle, dFloat maxAngle, NewtonBody* const child, NewtonBody* const parent = NULL);
	CUSTOM_JOINTS_API virtual ~dCustomHingeActuator();

	CUSTOM_JOINTS_API dFloat GetActuatorAngle() const;

	CUSTOM_JOINTS_API dFloat GetTargetAngle() const;
	CUSTOM_JOINTS_API void SetTargetAngle(dFloat angle);

	CUSTOM_JOINTS_API dFloat GetMinAngularLimit() const;
	CUSTOM_JOINTS_API void SetMinAngularLimit(dFloat limit);

	CUSTOM_JOINTS_API dFloat GetMaxAngularLimit() const;
	CUSTOM_JOINTS_API void SetMaxAngularLimit(dFloat limit);

	CUSTOM_JOINTS_API dFloat GetAngularRate() const;
	CUSTOM_JOINTS_API void SetAngularRate(dFloat rate);

    CUSTOM_JOINTS_API dFloat GetMaxTorque() const;
    CUSTOM_JOINTS_API void SetMaxTorque(dFloat torque);

	protected:
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, const dVector& eulers, dFloat timestep);

	dAngularIntegration m_targetAngle;
	dFloat m_maxTorque;
	DECLARE_CUSTOM_JOINT(dCustomHingeActuator, dCustomHinge)
};

#endif
