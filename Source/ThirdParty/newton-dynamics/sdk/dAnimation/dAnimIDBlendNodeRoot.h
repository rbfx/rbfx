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

#ifndef __D_ANIM_ID_BLEND_ROOT_h__
#define __D_ANIM_ID_BLEND_ROOT_h__

#include "dAnimIDBlendNode.h"

class dAnimIDBlendNodeRoot: public dAnimIDBlendNode
{
	public:
	dAnimIDBlendNodeRoot(dAnimIDController* const character, dAnimIDBlendNode* const childNode);
	virtual ~dAnimIDBlendNodeRoot();

	dAnimationPose& GetPose() { return m_pose; }

	void Update(dFloat timestep);
	virtual void Evaluate(dAnimationPose& output, dFloat timestep);

	protected:
	dAnimationPose m_pose;
	dAnimIDController* m_rig;
};


#endif