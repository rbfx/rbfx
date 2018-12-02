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

#ifndef _D_NEWTON_JOINT_H_
#define _D_NEWTON_JOINT_H_

#include "dStdAfxNewton.h"
#include "dNewtonBody.h"
#include "dNewtonAlloc.h"
#include "dNewtonDynamicBody.h"


class dNewtonJoint: public dNewtonAlloc
{
	public:
	enum dJointType
	{
        // general 6dof joint
        //m_3dof,

        // basics joints
		m_ballAndSocket,
		m_hinge,
		m_slider,
		m_universal,
        m_cylindrical,

        // relational joints
        m_gear,
        m_pulley,
        m_gearAndRack,

        // robotic joints
        m_hingeActuator,
        m_sliderActuator,
        m_universalActuator,
       
        // special joints
        //m_uprighVector,
        //m_dryRollingFriction,
        //m_kinematicContoller,

        // this is the end
		m_unknown,
	};
	
	CNEWTON_API virtual ~dNewtonJoint();

	CNEWTON_API dNewtonDynamicBody* GetBody0 () const;
	CNEWTON_API dNewtonDynamicBody* GetBody1 () const;

	protected:
	CNEWTON_API dNewtonJoint(dJointType type);
	CNEWTON_API void SetJoint(dCustomJoint* const joint);
	CNEWTON_API virtual void OnSubmitConstraint (dFloat timestep, int threadIndex);

	private:
	CNEWTON_API static void OnJointDestroyCallback (const dCustomJoint* const me);

	protected:
	dJointType m_type;
	dCustomJoint* m_joint;
};

class dNewtonBallAndSocketJoint: public dNewtonJoint 
{
	public:
	CNEWTON_API dNewtonBallAndSocketJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1 = NULL);
};

class dNewtonHingeJoint: public dNewtonJoint 
{
	public:
	CNEWTON_API dNewtonHingeJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1 = NULL);
	CNEWTON_API dFloat GetFriction () const;
	CNEWTON_API void SetFriction (dFloat friction);

    CNEWTON_API void EnableLimits(bool state);
   	CNEWTON_API void SetLimits(dFloat minAngle, dFloat maxAngle);
};

class dNewtonSliderJoint: public dNewtonJoint 
{
	public:
	CNEWTON_API dNewtonSliderJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1 = NULL);

    CNEWTON_API void EnableLimits(bool state);
    CNEWTON_API void SetLimits(dFloat minDist, dFloat maxDist);
};


class dNewtonDoubleHinge: public dNewtonJoint 
{
	public:
	CNEWTON_API dNewtonDoubleHinge(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1 = NULL);
	CNEWTON_API void EnableLimit_0(bool state);
	CNEWTON_API void EnableLimit_1(bool state);
	CNEWTON_API void SetLimits_0(dFloat minAngle, dFloat maxAngle);
	CNEWTON_API void SetLimits_1(dFloat minAngle, dFloat maxAngle);
};

class dNewtonCylindricalJoint: public dNewtonJoint 
{
    public:
    CNEWTON_API dNewtonCylindricalJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1 = NULL);
    CNEWTON_API void EnableLimit_0(bool state);
    CNEWTON_API void EnableLimit_1(bool state);
    CNEWTON_API void SetLimits_0(dFloat minDist, dFloat maxDist);
    CNEWTON_API void SetLimits_1(dFloat minAngle, dFloat maxAngle);
};


class dNewtonGearJoint: public dNewtonJoint 
{
    public:
    CNEWTON_API dNewtonGearJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1);
};

class dNewtonPulleyJoint: public dNewtonJoint 
{
    public:
    CNEWTON_API dNewtonPulleyJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1);
};


class dNewtonGearAndRackJoint: public dNewtonJoint 
{
    public:
    CNEWTON_API dNewtonGearAndRackJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1);
};

#endif
