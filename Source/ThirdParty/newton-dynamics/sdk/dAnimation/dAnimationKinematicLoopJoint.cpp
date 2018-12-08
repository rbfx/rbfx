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

#include "dAnimationStdAfx.h"
#include "dAnimationAcyclicJoint.h"
#include "dAnimationKinematicLoopJoint.h"

dAnimationKinematicLoopJoint::dAnimationKinematicLoopJoint()
	:dComplementaritySolver::dBilateralJoint()
	,m_owner0(NULL)
	,m_owner1(NULL)
	,m_isActive(false)
{
}

void dAnimationKinematicLoopJoint::SetOwners(dAnimationAcyclicJoint* const owner0, dAnimationAcyclicJoint* const owner1)
{
	m_owner0 = owner0;
	m_owner1 = owner1;
	Init(m_owner0->GetProxyBody(), m_owner1->GetProxyBody());
}
