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

#ifndef __D_ANIM_IK_BLEND_NODE_h__
#define __D_ANIM_IK_BLEND_NODE_h__

#include "dAnimPose.h"
#include "dAnimationStdAfx.h"

//class dAnimIKBlendNode;
//class dAnimIDRigEffector;
class dAnimIKController;

class dAnimIKBlendNode: public dCustomAlloc
{
	public:
	dAnimIKBlendNode(dAnimIKController* const character, dAnimIKBlendNode* const child);
	virtual ~dAnimIKBlendNode();

	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const
	{
		if (m_child) {
			m_child->Debug(debugContext);
		}
	}

	virtual void Evaluate(dAnimPose& output, dFloat timestep)
	{
		if (m_child) {
			m_child->Evaluate(output, timestep);
		}
	}

	dAnimIKController* m_character;
	dAnimIKBlendNode* m_child;
};

#endif