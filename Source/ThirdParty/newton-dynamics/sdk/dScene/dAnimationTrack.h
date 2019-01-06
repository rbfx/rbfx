/////////////////////////////////////////////////////////////////////////////
// Name:        dAnimationTrack.h
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

#ifndef _D_ANIMATION_TRACK_H_
#define _D_ANIMATION_TRACK_H_

#include "dNodeInfo.h"

class dAnimationTrack: public dNodeInfo
{
	public:
	class dCurveValue
	{
		public:
		dFloat m_x;
		dFloat m_y;
		dFloat m_z;
		dFloat m_time;
	};
	D_DEFINE_CLASS_NODE(dAnimationTrack,dNodeInfo,DSCENE_API)

	dAnimationTrack();
	dAnimationTrack(dScene* const world);
	virtual ~dAnimationTrack(void);

	const dList<dCurveValue>& GetPositions() const;
	const dList<dCurveValue>& GetRotations() const;

	void AddPosition(dFloat time, dFloat x, dFloat y, dFloat z);
	void AddRotation(dFloat time, dFloat x, dFloat y, dFloat z);
	void OptimizeCurves();

	protected:
	void OptimizeCurve(dList<dCurveValue>& curve);
	virtual void BakeTransform(const dMatrix& matrix);
	virtual void Serialize (TiXmlElement* const rootNode) const; 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dList<dCurveValue> m_position;
	dList<dCurveValue> m_rotation;
};

#endif