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

	class dCurve: public dList <dCurveValue>
	{
		public:
		dCurve()
			:dList <dCurveValue>()
		{
		}

		dCurveValue Evaluate(dFloat t)
		{
			for (dListNode* ptr = GetFirst(); ptr->GetNext(); ptr = ptr->GetNext()) {
				dCurveValue& info1 = ptr->GetNext()->GetInfo();
				if (info1.m_time >= t) {
					dCurveValue& info0 = ptr->GetInfo();
					dCurveValue val;
					dFloat param = (t - info0.m_time) / (info1.m_time - info0.m_time);
					val.m_x = info0.m_x + (info1.m_x - info0.m_x) * param;
					val.m_y = info0.m_y + (info1.m_y - info0.m_y) * param;
					val.m_z = info0.m_z + (info1.m_z - info0.m_z) * param;
					val.m_time = info0.m_time + (info1.m_time - info0.m_time) * param;
					return val;
				}
			}
			dAssert(0);
			return dCurveValue();
		}
	};

	D_DEFINE_CLASS_NODE(dAnimationTrack,dNodeInfo,DSCENE_API)

	dAnimationTrack();
	dAnimationTrack(dScene* const world);
	virtual ~dAnimationTrack(void);

	const dList<dCurveValue>& GetScales() const;
	const dList<dCurveValue>& GetPositions() const;
	const dList<dCurveValue>& GetRotations() const;

	void AddScale(dFloat time, dFloat x, dFloat y, dFloat z);
	void AddPosition(dFloat time, dFloat x, dFloat y, dFloat z);
	void AddRotation(dFloat time, dFloat x, dFloat y, dFloat z);
	void AddKeyframe(dFloat time, const dMatrix& matrix);

	void OptimizeCurves();
	virtual void FreezeScale(const dMatrix& matrix);

	protected:
	void ResampleAnimation();
	void OptimizeCurve(dList<dCurveValue>& curve);
	dFloat FixAngleAlias(dFloat angle0, dFloat angle1) const;
	dFloat Interpolate(dFloat x0, dFloat t0, dFloat x1, dFloat t1, dFloat t) const;
	
	virtual void BakeTransform(const dMatrix& matrix);
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dCurve m_scale;
	dCurve m_position;
	dCurve m_rotation;
};

inline const dList<dAnimationTrack::dCurveValue>& dAnimationTrack::GetScales() const
{
	return m_scale;
}

inline const dList<dAnimationTrack::dCurveValue>& dAnimationTrack::GetPositions() const
{
	return m_position;
}

inline const dList<dAnimationTrack::dCurveValue>& dAnimationTrack::GetRotations() const
{
	return m_rotation;
}


#endif