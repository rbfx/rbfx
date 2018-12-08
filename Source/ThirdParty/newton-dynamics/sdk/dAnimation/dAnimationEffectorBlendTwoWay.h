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

#ifndef __D_ANIMATION_EFFECTOR_BLEND_TWO_WAY_h__
#define __D_ANIMATION_EFFECTOR_BLEND_TWO_WAY_h__

#include "dAnimationEffectorBlendNode.h"

class dAnimationEffectorBlendTwoWay: public dAnimationEffectorBlendNode
{
	public:
	dAnimationEffectorBlendTwoWay(dAnimationCharacterRig* const character,
								  dAnimationEffectorBlendNode* const node0,
								  dAnimationEffectorBlendNode* const node1);
	virtual ~dAnimationEffectorBlendTwoWay();

	dFloat GetParam () const {return m_param;}
	void SetParam (dFloat param) {m_param = param;}

	dAnimationPose& GetPose() { return m_pose; }
	virtual void Evaluate(dAnimationPose& output, dFloat timestep);
	void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	protected:
	dAnimationEffectorBlendNode* m_node0;
	dAnimationEffectorBlendNode* m_node1;
	dAnimationPose m_pose;
	dFloat m_param;
};


#endif