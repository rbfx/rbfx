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

#include "dAnimIDBlendNode.h"

class dAnimIDBlendNodeTwoWay: public dAnimIDBlendNode
{
	public:
	dAnimIDBlendNodeTwoWay(dAnimIDController* const character,
								  dAnimIDBlendNode* const node0,
								  dAnimIDBlendNode* const node1);
	virtual ~dAnimIDBlendNodeTwoWay();

	dFloat GetParam () const {return m_param;}
	void SetParam (dFloat param) {m_param = param;}

	dAnimationPose& GetPose() { return m_pose; }
	virtual void Evaluate(dAnimationPose& output, dFloat timestep);
	void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	protected:
	dAnimIDBlendNode* m_node0;
	dAnimIDBlendNode* m_node1;
	dAnimationPose m_pose;
	dFloat m_param;
};


#endif