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
#include "dAnimIKBlendNodeTake.h"

dAnimTakeData::dAnimTakeData(int tracksCount)
	:dRefCounter()
	,m_tracks()
{
	for (int i = 0; i < tracksCount; i ++) {
		m_tracks.Append();
	}
}

dAnimTakeData::~dAnimTakeData()
{
}

dAnimTakeData::dAnimTakeTrack::dAnimTakeTrack()
	:m_position()
	,m_rotation()
{
}

dAnimTakeData::dAnimTakeTrack::~dAnimTakeTrack()
{
}

dAnimIKBlendNodeTake::dAnimIKBlendNodeTake(dAnimIKController* const character, dAnimTakeData* const takeData)
	:dAnimIKBlendNode(character, NULL)
	,m_takeData(takeData)
{
	m_takeData->AddRef();
}

dAnimIKBlendNodeTake::~dAnimIKBlendNodeTake()
{
	m_takeData->Release();
}

void dAnimIKBlendNodeTake::Evaluate(dAnimPose& output, dFloat timestep)
{
	dAssert(0);
//	output.CopySource(m_pose);
}