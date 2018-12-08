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
#include "dAnimationEffectorBlendPose.h"

dAnimationEffectorBlendPose::dAnimationEffectorBlendPose(dAnimationCharacterRig* const character)
	:dAnimationEffectorBlendNode(character, NULL)
	,m_pose(character)
{
}

dAnimationEffectorBlendPose::~dAnimationEffectorBlendPose()
{
}

void dAnimationEffectorBlendPose::Evaluate(dAnimationPose& output, dFloat timestep)
{
	output.CopySource(m_pose);
}