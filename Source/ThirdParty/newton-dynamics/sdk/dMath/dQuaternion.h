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

#ifndef __dQuaternion__
#define __dQuaternion__

#include "dVector.h"
class dMatrix;

D_MSC_VECTOR_ALIGMENT
class dQuaternion
{
	public:
	dQuaternion (); 
	dQuaternion (const dMatrix& matrix);
	dQuaternion (dFloat w, dFloat x, dFloat y, dFloat z); 
	dQuaternion (const dVector& unit_Axis, dFloat Angle = 0.0f);
	
	void Scale (dFloat scale); 
	void Normalize (); 
	inline dFloat DotProduct (const dQuaternion& q) const;
	dQuaternion Inverse () const; 

	dVector RotateVector (const dVector& point) const;
	dVector UnrotateVector (const dVector& point) const;

    void GetEulerAngles(dVector& euler1, dVector& euler2, dEulerAngleOrder order = m_pitchYawRoll) const;
	dVector CalcAverageOmega (const dQuaternion &q1, dFloat invdt) const;
	dQuaternion Slerp (const dQuaternion &q1, dFloat t) const;
	dQuaternion IntegrateOmega (const dVector& omega, dFloat timestep) const;

	dQuaternion operator* (const dQuaternion &q) const;
	dQuaternion operator+ (const dQuaternion &q) const; 
	dQuaternion operator- (const dQuaternion &q) const; 

	dFloat m_x;
	dFloat m_y;
	dFloat m_z;
	dFloat m_w;
};

inline dQuaternion::dQuaternion () 
	:m_x(0.0f)
	,m_y(0.0f)
	,m_z(0.0f)
	,m_w(1.0f)
{
}

inline dQuaternion::dQuaternion (dFloat w, dFloat x, dFloat y, dFloat z) 
	:m_x(x)
	,m_y(y)
	,m_z(z)
	,m_w(w)
{
}

inline void dQuaternion::Scale (dFloat scale) 
{
	m_w *= scale;
	m_x *= scale;
	m_y *= scale;
	m_z *= scale;
}

inline void dQuaternion::Normalize () 
{
	Scale (1.0f / dSqrt (DotProduct (*this)));
}

inline dFloat dQuaternion::DotProduct (const dQuaternion &q1) const
{
	return m_w * q1.m_w + m_x * q1.m_x + m_y * q1.m_y + m_z * q1.m_z;
}

inline dQuaternion dQuaternion::Inverse () const 
{
	return dQuaternion (m_w, -m_x, -m_y, -m_z);
}

inline dQuaternion dQuaternion::operator+ (const dQuaternion &q) const
{
	return dQuaternion (m_w + q.m_w, m_x + q.m_x, m_y + q.m_y, m_z + q.m_z);
}

inline dQuaternion dQuaternion::operator- (const dQuaternion &B) const
{
	return dQuaternion (m_w - B.m_w, m_x - B.m_x, m_y - B.m_y, m_z - B.m_z);
}

inline dQuaternion dQuaternion::operator* (const dQuaternion &q) const
{
	return dQuaternion (q.m_w * m_w - q.m_x * m_x - q.m_y * m_y - q.m_z * m_z, 
				 		q.m_x * m_w + q.m_w * m_x - q.m_z * m_y + q.m_y * m_z, 
						q.m_y * m_w + q.m_z * m_x + q.m_w * m_y - q.m_x * m_z, 
						q.m_z * m_w - q.m_y * m_x + q.m_x * m_y + q.m_w * m_z); 
}


#endif 

