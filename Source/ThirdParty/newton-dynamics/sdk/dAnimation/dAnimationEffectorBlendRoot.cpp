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
#include "dAnimationEffectorBlendRoot.h"

dAnimationEffectorBlendRoot::dAnimationEffectorBlendRoot(dAnimationCharacterRig* const character, dAnimationEffectorBlendNode* const childNode)
	:dAnimationEffectorBlendNode(character, childNode)
	,m_pose(character)
	,m_rig(character)
{
}

dAnimationEffectorBlendRoot::~dAnimationEffectorBlendRoot()
{
}

void dAnimationEffectorBlendRoot::Update(dFloat timestep)
{
	Evaluate(m_pose, timestep);
	m_pose.SetTargetPose(m_rig);
}


void dAnimationEffectorBlendRoot::Evaluate(dAnimationPose& output, dFloat timestep)
{
	dAnimationEffectorBlendNode::Evaluate(output, timestep);
}