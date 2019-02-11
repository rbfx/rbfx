/////////////////////////////////////////////////////////////////////////////
// Name:        dAnimationTake.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 08:02:08
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////

#ifndef _D_ANIMATION_TAKE_H_
#define _D_ANIMATION_TAKE_H_

#include "dNodeInfo.h"

class dAnimationTake: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dAnimationTake,dNodeInfo,DSCENE_API)

	dAnimationTake();
	dAnimationTake(dScene* const world);
	virtual ~dAnimationTake(void);

	dFloat GetPeriod() const { return m_period;}
	void SetPeriod(dFloat period) { m_period = period; }

	protected:
	dFloat m_period;
	virtual void Serialize (TiXmlElement* const rootNode) const; 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);
};

#endif