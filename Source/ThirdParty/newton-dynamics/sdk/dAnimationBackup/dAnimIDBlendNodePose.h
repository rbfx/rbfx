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

#ifndef __D_ANIMATION_EFFECTOR_BLEND_POSE_h__
#define __D_ANIMATION_EFFECTOR_BLEND_POSE_h__

#include "dAnimIDBlendNode.h"

class dAnimIDBlendNodePose: public dAnimIDBlendNode
{
	public:
	dAnimIDBlendNodePose(dAnimIDController* const character);
	virtual ~dAnimIDBlendNodePose();

	dAnimationPose& GetPose() { return m_pose; }
	virtual void Evaluate(dAnimationPose& output, dFloat timestep);

	protected:
	dAnimationPose m_pose;
};


#endif