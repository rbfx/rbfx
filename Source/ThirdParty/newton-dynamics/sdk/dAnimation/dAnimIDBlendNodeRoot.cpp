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
#include "dAnimIDBlendNodeRoot.h"

dAnimIDBlendNodeRoot::dAnimIDBlendNodeRoot(dAnimIDController* const character, dAnimIDBlendNode* const childNode)
	:dAnimIDBlendNode(character, childNode)
	,m_pose(character)
	,m_rig(character)
{
}

dAnimIDBlendNodeRoot::~dAnimIDBlendNodeRoot()
{
}

void dAnimIDBlendNodeRoot::Update(dFloat timestep)
{
	Evaluate(m_pose, timestep);
	m_pose.SetTargetPose(m_rig);
}


void dAnimIDBlendNodeRoot::Evaluate(dAnimationPose& output, dFloat timestep)
{
	dAnimIDBlendNode::Evaluate(output, timestep);
}