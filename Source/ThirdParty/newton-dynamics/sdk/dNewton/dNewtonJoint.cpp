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

#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonBody.h"
#include "dNewtonJoint.h"
#include "dNewtonDynamicBody.h"


dNewtonJoint::dNewtonJoint(dJointType type)
	:m_type(type) 
	,m_joint(NULL)
{
}

void dNewtonJoint::OnSubmitConstraint (dFloat timestep, int threadIndex)
{
}


void dNewtonJoint::SetJoint(dCustomJoint* const joint)
{
	m_joint = joint;
	m_joint->SetUserData (this);
	m_joint->SetUserDestructorCallback (OnJointDestroyCallback);
}

dNewtonJoint::~dNewtonJoint()
{
	if (m_joint && m_joint->GetUserData()) {
		m_joint->SetUserData (NULL);
		m_joint->SetUserDestructorCallback (NULL);
		delete m_joint;
	}
}

dNewtonDynamicBody* dNewtonJoint::GetBody0 () const
{
	return (dNewtonDynamicBody*)NewtonBodyGetUserData (m_joint->GetBody0()); 
}

dNewtonDynamicBody* dNewtonJoint::GetBody1 () const
{
	return  m_joint->GetBody1() ? (dNewtonDynamicBody*)NewtonBodyGetUserData (m_joint->GetBody1()) : NULL; 
}


void dNewtonJoint::OnJointDestroyCallback (const dCustomJoint* const customJoint)
{
	dNewtonJoint* const joint = (dNewtonJoint*) customJoint->GetUserData();
	joint->m_joint = NULL;
	((dCustomJoint*)customJoint)->SetUserData(NULL);
	delete joint;
}


dNewtonBallAndSocketJoint::dNewtonBallAndSocketJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1)
    :dNewtonJoint(m_ballAndSocket)
{
    SetJoint (new dCustomBallAndSocket (dMatrix(pinAndPivotFrame), body0->GetNewtonBody(), body1 ? body1->GetNewtonBody() : NULL));
}


dNewtonHingeJoint::dNewtonHingeJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1)
	:dNewtonJoint(m_hinge)
{
	SetJoint (new dCustomHinge (dMatrix(pinAndPivotFrame), body0->GetNewtonBody(), body1 ? body1->GetNewtonBody() : NULL));
}

dFloat dNewtonHingeJoint::GetFriction () const
{
	return ((dCustomHinge*)m_joint)->GetFriction();
}

void dNewtonHingeJoint::SetFriction (dFloat friction)
{
	((dCustomHinge*)m_joint)->SetFriction(friction);
}

void dNewtonHingeJoint::EnableLimits(bool state)
{
    ((dCustomHinge*)m_joint)->EnableLimits(state);
}

void dNewtonHingeJoint::SetLimits(dFloat minAngle, dFloat maxAngle)
{
    ((dCustomHinge*)m_joint)->SetLimits(minAngle, maxAngle);
}

dNewtonSliderJoint::dNewtonSliderJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1)
	:dNewtonJoint(m_slider)
{
	SetJoint (new dCustomSlider (dMatrix(pinAndPivotFrame), body0->GetNewtonBody(), body1 ? body1->GetNewtonBody() : NULL));
}

void dNewtonSliderJoint::EnableLimits(bool state)
{
    ((dCustomSlider*) m_joint)->EnableLimits(state);
}

void dNewtonSliderJoint::SetLimits(dFloat minDist, dFloat maxDist)
{
    ((dCustomSlider*) m_joint)->SetLimits(minDist, maxDist);
}

dNewtonDoubleHinge::dNewtonDoubleHinge(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1)
	:dNewtonJoint(m_universal)
{
	SetJoint (new dCustomDoubleHinge (dMatrix(pinAndPivotFrame), body0->GetNewtonBody(), body1 ? body1->GetNewtonBody() : NULL));
}

void dNewtonDoubleHinge::EnableLimit_0(bool state)
{
	dAssert (0);
//	((dCustomDoubleHinge*) m_joint)->EnableLimit_0(state);
}

void dNewtonDoubleHinge::EnableLimit_1(bool state)
{
	dAssert (0);
//	((dCustomDoubleHinge*) m_joint)->EnableLimit_1(state);
}

void dNewtonDoubleHinge::SetLimits_0(dFloat minAngle, dFloat maxAngle)
{
	dAssert (0);
//	((dCustomDoubleHinge*) m_joint)->SetLimits_0 (minAngle, maxAngle);
}

void dNewtonDoubleHinge::SetLimits_1(dFloat minAngle, dFloat maxAngle)
{
	dAssert (0);
//	((dCustomDoubleHinge*) m_joint)->SetLimits_1 (minAngle, maxAngle);
}


dNewtonCylindricalJoint::dNewtonCylindricalJoint(const dFloat* const pinAndPivotFrame, dNewtonDynamicBody* const body0, dNewtonDynamicBody* const body1)
    :dNewtonJoint(m_cylindrical)
{
    SetJoint (new dCustomCorkScrew (dMatrix(pinAndPivotFrame), body0->GetNewtonBody(), body1 ? body1->GetNewtonBody() : NULL));
}

void dNewtonCylindricalJoint::EnableLimit_0(bool state)
{
	dAssert (0);
//    ((dCustomCorkScrew*) m_joint)->EnableLinearLimits(state);
}

void dNewtonCylindricalJoint::EnableLimit_1(bool state)
{
    ((dCustomCorkScrew*) m_joint)->EnableAngularLimits(state);
}

void dNewtonCylindricalJoint::SetLimits_0(dFloat minDist, dFloat maxDist)
{
	dAssert (0);
//    ((dCustomCorkScrew*) m_joint)->SetLinearLimis (minDist, maxDist);
}

void dNewtonCylindricalJoint::SetLimits_1(dFloat minAngle, dFloat maxAngle)
{
	dAssert (0);
//    ((dCustomCorkScrew*) m_joint)->SetAngularLimis(minAngle, maxAngle);
}


dNewtonGearJoint::dNewtonGearJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1)
    :dNewtonJoint(m_gear)
{
    SetJoint (new dCustomGear (ratio, body0Pin, body1Pin, body0->GetNewtonBody(), body1->GetNewtonBody()));
}

dNewtonPulleyJoint::dNewtonPulleyJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1)
    :dNewtonJoint(m_pulley)
{
    SetJoint (new dCustomPulley (ratio, body0Pin, body1Pin, body0->GetNewtonBody(), body1->GetNewtonBody()));
}

dNewtonGearAndRackJoint::dNewtonGearAndRackJoint(dFloat ratio, const dFloat* const body0Pin, dNewtonDynamicBody* const body0, const dFloat* const body1Pin, dNewtonDynamicBody* const body1)
    :dNewtonJoint(m_gearAndRack)
{
    SetJoint (new dCustomRackAndPinion (ratio, body0Pin, body1Pin, body0->GetNewtonBody(), body1->GetNewtonBody()));
}

