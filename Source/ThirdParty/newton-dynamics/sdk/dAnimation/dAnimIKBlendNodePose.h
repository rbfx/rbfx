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

#ifndef __D_ANIM_IK_BLEND_NODE_POSE_h__
#define __D_ANIM_IK_BLEND_NODE_POSE_h__
#include "dAnimPose.h"
#include "dAnimIKBlendNode.h"

class dAnimIKBlendNodePose: public dAnimIKBlendNode
{
	public:
	dAnimIKBlendNodePose(dAnimIKController* const character);
	virtual ~dAnimIKBlendNodePose();

	dAnimPose& GetPose() { return m_pose; }
	virtual void Evaluate(dAnimPose& output, dFloat timestep);

	protected:
	dAnimPose m_pose;
};


#endif