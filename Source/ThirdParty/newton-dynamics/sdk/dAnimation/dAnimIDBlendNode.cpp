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
#include "dAnimIDRigLimb.h"
#include "dAnimIDRigEffector.h"
#include "dAnimIDBlendNode.h"
#include "dAnimIDController.h"

dAnimIDBlendNode::dAnimationPose::dAnimationPose(dAnimIDController* const character)
	:dList<dAnimationTransform>()
{
	dList<dAnimIDRigEffector*>& effectorsList = character->GetEffectors();
	for (dList<dAnimIDRigEffector*>::dListNode* node = effectorsList.GetFirst(); node; node = node->GetNext()) {
		dAnimIDRigEffector* const effector = node->GetInfo();

		const dMatrix& localMatrix = effector->GetLocalMatrix();

		dAnimationTransform frame;
		frame.m_effector = effector;
		frame.m_posit = localMatrix.m_posit;
		frame.m_rotation = dQuaternion(localMatrix);
		Append(frame);
	}
}

void dAnimIDBlendNode::dAnimationPose::CopySource(const dAnimationPose& source)
{
	dListNode* destNode = GetFirst();
	for (dListNode* sourceNode = source.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext()) {
		const dAnimationTransform& srcFrame = sourceNode->GetInfo();
		dAnimationTransform& dstFrame = destNode->GetInfo();
		dAssert(srcFrame.m_effector == dstFrame.m_effector);
		dstFrame.m_rotation = srcFrame.m_rotation;
		dstFrame.m_posit = srcFrame.m_posit;
		destNode = destNode->GetNext();
	}
}

void dAnimIDBlendNode::dAnimationPose::SetTargetPose(dAnimIDController* const character) const
{
	dMatrix rootMatrix(character->GetBasePoseMatrix());
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dAnimationTransform& frame = node->GetInfo();
		dMatrix matrix(dMatrix(frame.m_rotation, frame.m_posit) * rootMatrix);
		frame.m_effector->SetTargetPose(matrix);
	}
}

dAnimIDBlendNode::dAnimIDBlendNode(dAnimIDController* const character, dAnimIDBlendNode* const child)
	:dCustomAlloc()
	,m_character(character)
	,m_child(child)
{
}

dAnimIDBlendNode::~dAnimIDBlendNode()
{
	if (m_child) {
		delete m_child;
	}
}

