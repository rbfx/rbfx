/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_ACTUATOR_JOINTS_H_
#define _D_NEWTON_ACTUATOR_JOINTS_H_

#include "dStdAfxNewton.h"
#include "dNewtonJoint.h"


class dNewtonHingeActuator: public dNewtonJoint
{
	public:
	CNEWTON_API dNewtonHingeActuator(const dFloat* const pinAndPivotFrame, dFloat angularRate, dFloat minAngle, dFloat maxAngle, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1);
	CNEWTON_API bool GetEnableFlag () const;
	CNEWTON_API dFloat GetTargetAngle() const;
	CNEWTON_API dFloat GetMinAngularLimit() const;
	CNEWTON_API dFloat GetMaxAngularLimit() const;
	CNEWTON_API dFloat GetAngularRate() const;
	CNEWTON_API void SetMinAngularLimit(dFloat limit);
	CNEWTON_API void SetMaxAngularLimit(dFloat limit);
	CNEWTON_API void SetAngularRate(dFloat rate);
	CNEWTON_API void SetTargetAngle(dFloat angle);
	CNEWTON_API void SetEnableFlag (bool flag);
	CNEWTON_API dFloat GetActuatorAngle() const;
};



class dNewtonSliderActuator: public dNewtonJoint
{
	public:
	CNEWTON_API dNewtonSliderActuator(const dFloat* const pinAndPivotFrame, dFloat speed, dFloat minPosit, dFloat maxPosit, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1);
	CNEWTON_API dFloat GetTargetPosit() const;
	CNEWTON_API dFloat GetLinearRate() const;
	CNEWTON_API dFloat GetMinPositLimit() const;
	CNEWTON_API dFloat GetMaxPositLimit() const;
	CNEWTON_API void SetTargetPosit(dFloat posit);
	CNEWTON_API void SetMinPositLimit(dFloat limit);
	CNEWTON_API void SetMaxPositLimit(dFloat limit);
	CNEWTON_API void SetLinearRate(dFloat rate);
	CNEWTON_API dFloat GetActuatorPosit() const;
};


class dNewtonUniversalActuator: public dNewtonJoint 
{
	public:
	CNEWTON_API dNewtonUniversalActuator(const dFloat* const pinAndPivotFrame, dFloat angularRate0, dFloat minAngle0, dFloat maxAngle0, dFloat angularRate1, dFloat minAngle1, dFloat maxAngle1, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1);
	CNEWTON_API void SetEnableFlag0 (bool flag);
	CNEWTON_API void SetTargetAngle0(dFloat angle);
	CNEWTON_API void SetMinAngularLimit0(dFloat limit);
	CNEWTON_API void SetMaxAngularLimit0(dFloat limit);
	CNEWTON_API void SetAngularRate0(dFloat rate);
	CNEWTON_API void SetEnableFlag1 (bool flag);
	CNEWTON_API void SetTargetAngle1(dFloat angle);
	CNEWTON_API void SetMinAngularLimit1(dFloat limit);
	CNEWTON_API void SetMaxAngularLimit1(dFloat limit);
	CNEWTON_API void SetAngularRate1(dFloat rate);

	CNEWTON_API bool GetEnableFlag0 () const;
	CNEWTON_API dFloat GetTargetAngle0() const;
	CNEWTON_API dFloat GetAngularRate0() const;
	CNEWTON_API dFloat GetMinAngularLimit0() const;
	CNEWTON_API dFloat GetMaxAngularLimit0() const;
	CNEWTON_API bool GetEnableFlag1 () const;
	CNEWTON_API dFloat GetTargetAngle1() const;
	CNEWTON_API dFloat GetAngularRate1() const;
	CNEWTON_API dFloat GetMinAngularLimit1() const;
	CNEWTON_API dFloat GetMaxAngularLimit1() const;
	CNEWTON_API dFloat GetActuatorAngle0() const;
	CNEWTON_API dFloat GetActuatorAngle1() const;
};



#endif

