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

#ifndef __D_ANIM_ID_BLEND_NODE_h__
#define __D_ANIM_ID_BLEND_NODE_h__

class dAnimIDBlendNode;
class dAnimIDController;
class dAnimIDRigEffector;

class dAnimIDBlendNode: public dCustomAlloc
{
	public:
	class dAnimationTransform
	{
		public:
		dVector m_posit;
		dQuaternion m_rotation;
		dAnimIDRigEffector* m_effector;
	};

	class dAnimationPose : public dList<dAnimationTransform>
	{
		public:
		dAnimationPose(dAnimIDController* const character);

		void SetTargetPose(dAnimIDController* const character) const;
		void CopySource(const dAnimationPose& source);
	};

	dAnimIDBlendNode(dAnimIDController* const character, dAnimIDBlendNode* const child);
	virtual ~dAnimIDBlendNode();

	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const
	{
		if (m_child) {
			m_child->Debug(debugContext);
		}
	}

	virtual void Evaluate(dAnimationPose& output, dFloat timestep)
	{
		if (m_child) {
			m_child->Evaluate(output, timestep);
		}
	}

	dAnimIDController* m_character;
	dAnimIDBlendNode* m_child;
};

#endif