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
#include "dAnimIKBlendNode.h"
#include "dAnimIKController.h"

dAnimIKBlendNode::dAnimIKBlendNode(dAnimIKController* const character, dAnimIKBlendNode* const child)
	:dCustomAlloc()
	,m_character(character)
	,m_child(child)
{
}

dAnimIKBlendNode::~dAnimIKBlendNode()
{
	if (m_child) {
		delete m_child;
	}
}

