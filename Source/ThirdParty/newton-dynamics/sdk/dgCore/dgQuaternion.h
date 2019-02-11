/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgQuaternion__
#define __dgQuaternion__

#include "dgStdafx.h"

class dgVector;
class dgMatrix;

DG_MSC_VECTOR_ALIGMENT
class dgQuaternion
{
	public:
	dgQuaternion (); 
	dgQuaternion (const dgMatrix& matrix);
	dgQuaternion (dgFloat32 q0, dgFloat32 q1, dgFloat32 q2, dgFloat32 q3); 
	dgQuaternion (const dgVector &unit_Axis, dgFloat32 angle = dgFloat32 (0.0f));

//	dgFloat32& operator[] (dgInt32 i);
//	const dgFloat32& operator[] (dgInt32 i) const;

	void Scale (dgFloat32 scale); 
	void Normalize (); 
	dgQuaternion Inverse () const; 
	dgQuaternion Slerp (const dgQuaternion &q1, dgFloat32 t) const;

	dgFloat32 DotProduct (const dgQuaternion &QB) const;
	dgVector CalcAverageOmega (const dgQuaternion &q1, dgFloat32 invdt) const;

	dgQuaternion operator* (const dgQuaternion &B) const;
	dgQuaternion operator+ (const dgQuaternion &B) const; 
	dgQuaternion operator- (const dgQuaternion &B) const; 

	dgFloat32 m_x;
	dgFloat32 m_y;
	dgFloat32 m_z;
	dgFloat32 m_w;
} DG_GCC_VECTOR_ALIGMENT;


DG_INLINE dgQuaternion::dgQuaternion()
	:m_x(dgFloat32(0.0f))
	,m_y(dgFloat32(0.0f))
	,m_z(dgFloat32(0.0f))
	,m_w(dgFloat32(1.0f))
{
}

DG_INLINE dgQuaternion::dgQuaternion(dgFloat32 Q0, dgFloat32 Q1, dgFloat32 Q2, dgFloat32 Q3)
	:m_x(Q1)
	,m_y(Q2)
	,m_z(Q3)
	,m_w(Q0)
{
//	dgAssert (dgAbs (DotProduct (*this) -dgFloat32 (1.0f)) < dgFloat32(1.0e-4f));
}

/*
DG_INLINE dgFloat32& dgQuaternion::operator[] (dgInt32 i)
{
	dgAssert(i < 4);
	dgAssert(i >= 0);
	return (&m_w)[i];
}

DG_INLINE const dgFloat32& dgQuaternion::operator[] (dgInt32 i) const
{
	dgAssert(i < 4);
	dgAssert(i >= 0);
	return (&m_w)[i];
}
*/

DG_INLINE void dgQuaternion::Scale (dgFloat32 scale) 
{
	m_w *= scale;
	m_x *= scale;
	m_y *= scale;
	m_z *= scale;
}

DG_INLINE void dgQuaternion::Normalize () 
{
	Scale (dgRsqrt (DotProduct (*this)));
}

DG_INLINE dgFloat32 dgQuaternion::DotProduct (const dgQuaternion &q1) const
{
	return m_w * q1.m_w + m_x * q1.m_x + m_y * q1.m_y + m_z * q1.m_z;
}

DG_INLINE dgQuaternion dgQuaternion::Inverse () const 
{
	return dgQuaternion (m_w, -m_x, -m_y, -m_z);
}

DG_INLINE dgQuaternion dgQuaternion::operator+ (const dgQuaternion &q) const
{
	return dgQuaternion (m_w + q.m_w, m_x + q.m_x, m_y + q.m_y, m_z + q.m_z);
}

DG_INLINE dgQuaternion dgQuaternion::operator- (const dgQuaternion &q) const
{
	return dgQuaternion (m_w - q.m_w, m_x - q.m_x, m_y - q.m_y, m_z - q.m_z);
}

DG_INLINE dgQuaternion dgQuaternion::operator* (const dgQuaternion &q) const
{
	return dgQuaternion (q.m_w * m_w - q.m_x * m_x - q.m_y * m_y - q.m_z * m_z, 
				 		 q.m_x * m_w + q.m_w * m_x - q.m_z * m_y + q.m_y * m_z, 
						 q.m_y * m_w + q.m_z * m_x + q.m_w * m_y - q.m_x * m_z, 
						 q.m_z * m_w - q.m_y * m_x + q.m_x * m_y + q.m_w * m_z); 
}



#endif

