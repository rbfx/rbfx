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
#include "dAnimationRigLimb.h"
#include "dAnimationRigEffector.h"
#include "dAnimationCharacterRig.h"
#include "dAnimationEffectorBlendNode.h"

dAnimationPose::dAnimationPose(dAnimationCharacterRig* const character)
	:dList<dAnimationTransform>()
{
	dList<dAnimationRigEffector*>& effectorsList = character->GetEffectors();
	for (dList<dAnimationRigEffector*>::dListNode* node = effectorsList.GetFirst(); node; node = node->GetNext()) {
		dAnimationRigEffector* const effector = node->GetInfo();

		const dMatrix& localMatrix = effector->GetLocalMatrix();

		dAnimationTransform frame;
		frame.m_effector = effector;
		frame.m_posit = localMatrix.m_posit;
		frame.m_rotation = dQuaternion(localMatrix);
		Append(frame);
	}
}

void dAnimationPose::CopySource(const dAnimationPose& source)
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

void dAnimationPose::SetTargetPose(dAnimationCharacterRig* const character) const
{
	dMatrix rootMatrix(character->GetBasePoseMatrix());
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dAnimationTransform& frame = node->GetInfo();
		dMatrix matrix(dMatrix(frame.m_rotation, frame.m_posit) * rootMatrix);
		frame.m_effector->SetTargetPose(matrix);
	}
}

dAnimationEffectorBlendNode::dAnimationEffectorBlendNode(dAnimationCharacterRig* const character, dAnimationEffectorBlendNode* const child)
	:dCustomAlloc()
	,m_character(character)
	,m_child(child)
{
}

dAnimationEffectorBlendNode::~dAnimationEffectorBlendNode()
{
	if (m_child) {
		delete m_child;
	}
}

