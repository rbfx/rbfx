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

#ifndef __dgMatrix__
#define __dgMatrix__


#include "dgStdafx.h"
#include "dgDebug.h"
#include "dgVector.h"
#include "dgPlane.h"
#include <math.h>

class dgMatrix;
class dgQuaternion;

const dgMatrix& dgGetZeroMatrix ();
const dgMatrix& dgGetIdentityMatrix();


DG_MSC_VECTOR_ALIGMENT
class dgMatrix
{
	public:
	DG_CLASS_ALLOCATOR(allocator)

	dgMatrix ();
	dgMatrix (const dgFloat32* const array);
	dgMatrix (const dgVector &front, const dgVector &up, const dgVector &right, const dgVector &posit);
	dgMatrix (const dgQuaternion &rotation, const dgVector &position);

	// create a orthonormal normal vector basis, front become m_front vector, and m_up and m_right are mutualiperpendicular to fron and to each other
	dgMatrix (const dgVector &front);

	// create a covariance Matrix = transpose(p) * q 
	dgMatrix (const dgVector& p, const dgVector& q);

	dgVector& operator[] (dgInt32 i);
	const dgVector& operator[] (dgInt32 i) const;

	dgMatrix Inverse () const;
	dgMatrix Inverse4x4 () const;
	dgMatrix Transpose () const;
	dgMatrix Transpose4X4 () const;
	dgMatrix Symetric3by3Inverse () const;
	dgVector RotateVector (const dgVector &v) const;
	dgVector UnrotateVector (const dgVector &v) const;
	dgVector TransformVector (const dgVector &v) const;
	dgVector UntransformVector (const dgVector &v) const;
	dgPlane TransformPlane (const dgPlane &localPlane) const;
	dgPlane UntransformPlane (const dgPlane &globalPlane) const;
	void TransformBBox (const dgVector& p0local, const dgVector& p1local, dgVector& p0, dgVector& p1) const; 

	void CalcPitchYawRoll (dgVector& euler0, dgVector& euler1) const;
	void TransformTriplex (dgFloat32* const dst, dgInt32 dstStrideInBytes,
						   const dgFloat32* const src, dgInt32 srcStrideInBytes, dgInt32 count) const;

#ifndef _NEWTON_USE_DOUBLE
	void TransformTriplex (dgFloat64* const dst, dgInt32 dstStrideInBytes,
						   const dgFloat64* const src, dgInt32 srcStrideInBytes, dgInt32 count) const;

	void TransformTriplex (dgFloat64* const dst, dgInt32 dstStrideInBytes,
						   const dgFloat32* const src, dgInt32 srcStrideInBytes, dgInt32 count) const;
#endif

	bool TestIdentity() const;
	bool TestSymetric3x3() const;
	bool TestOrthogonal(dgFloat32 tol = dgFloat32 (1.0e-4f)) const;

	dgMatrix Multiply3X3 (const dgMatrix &B) const;
	dgMatrix operator* (const dgMatrix &B) const;

	// these function can only be called when dgMatrix is a PDS matrix
	void EigenVectors (const dgMatrix* initialGuess = NULL);
	void EigenVectors (dgVector &eigenValues, const dgMatrix* const initialGuess = NULL);
	void PolarDecomposition (dgMatrix& transformMatrix, dgVector& scale, dgMatrix& stretchAxis, const dgMatrix* initialStretchAxis = NULL) const;

	// constructor for polar composition
	dgMatrix (const dgMatrix& transformMatrix, const dgVector& scale, const dgMatrix& stretchAxis);

	dgVector m_front;
	dgVector m_up;
	dgVector m_right;
	dgVector m_posit;

	static dgMatrix m_zeroMatrix;
	static dgMatrix m_identityMatrix;
} DG_GCC_VECTOR_ALIGMENT;



DG_INLINE dgMatrix::dgMatrix ()
{
}

DG_INLINE dgMatrix::dgMatrix (const dgFloat32* const array)
{
	memcpy (&m_front.m_x, array, sizeof (dgMatrix)) ;
}

DG_INLINE dgMatrix::dgMatrix (const dgVector &front, const dgVector &up, const dgVector &right, const dgVector &posit)
	:m_front (front), m_up(up), m_right(right), m_posit(posit)
{
}

DG_INLINE dgMatrix::dgMatrix (const dgVector& p, const dgVector& q)
	:m_front(q * p.BroadcastX())
	,m_up   (q * p.BroadcastY())
	,m_right(q * p.BroadcastZ())
	,m_posit (dgVector::m_wOne)
{
}

DG_INLINE dgMatrix::dgMatrix (const dgVector& front)
{
	m_front = front; 
	if (dgAbs (front.m_z) > dgFloat32 (0.577f)) {
		m_right = front.CrossProduct(dgVector (-front.m_y, front.m_z, dgFloat32(0.0f), dgFloat32(0.0f)));
	} else {
	  	m_right = front.CrossProduct(dgVector (-front.m_y, front.m_x, dgFloat32(0.0f), dgFloat32(0.0f)));
	}
  	//m_right = m_right.Scale (dgRsqrt (m_right.DotProduct(m_right).GetScalar()));
	m_right = m_right.Normalize();
  	m_up = m_right.CrossProduct(m_front);

	m_front.m_w = dgFloat32(0.0f);
	m_up.m_w = dgFloat32(0.0f);
	m_right.m_w = dgFloat32(0.0f);
	m_posit = dgVector (dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f));

	dgAssert ((dgAbs (m_front.DotProduct(m_front).GetScalar()) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f)); 
	dgAssert ((dgAbs (m_up.DotProduct(m_up).GetScalar()) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f)); 
	dgAssert ((dgAbs (m_right.DotProduct(m_right).GetScalar()) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f)); 
	dgAssert ((dgAbs (m_right.DotProduct(m_front.CrossProduct(m_up)).GetScalar()) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f)); 
}

DG_INLINE dgVector& dgMatrix::operator[] (dgInt32  i)
{
	dgAssert (i < 4);
	dgAssert (i >= 0);
	return (&m_front)[i];
}

DG_INLINE const dgVector& dgMatrix::operator[] (dgInt32  i) const
{
	dgAssert (i < 4);
	dgAssert (i >= 0);
	return (&m_front)[i];
}


DG_INLINE dgMatrix dgMatrix::Transpose () const
{
#if 1
	dgMatrix inv;
	dgVector::Transpose4x4(inv[0], inv[1], inv[2], inv[3], m_front, m_up, m_right, dgVector::m_wOne);
	return inv;
#else
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, dgFloat32(0.0f)),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, dgFloat32(0.0f)),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, dgFloat32(0.0f)),
					 dgVector::m_wOne);
#endif
}

DG_INLINE dgMatrix dgMatrix::Transpose4X4 () const
{
#if 1
	dgMatrix inv;
	dgVector::Transpose4x4(inv[0], inv[1], inv[2], inv[3], m_front, m_up, m_right, m_posit);
	return inv;
#else
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, m_posit.m_x),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, m_posit.m_y),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, m_posit.m_z),
					 dgVector (m_front.m_w, m_up.m_w, m_right.m_w, m_posit.m_w));
#endif
}

DG_INLINE dgVector dgMatrix::RotateVector (const dgVector &v) const
{
	return m_front * v.BroadcastX() + m_up * v.BroadcastY() + m_right * v.BroadcastZ();
}

DG_INLINE dgVector dgMatrix::UnrotateVector (const dgVector &v) const
{
#if 1
	return Transpose().RotateVector(v);
#else
	return (v.DotProduct(m_front) & dgVector::m_xMask) + (v.DotProduct(m_up) & dgVector::m_yMask) + (v.DotProduct(m_right) & dgVector::m_zMask);
#endif
}

DG_INLINE dgVector dgMatrix::TransformVector (const dgVector &v) const
{
	return RotateVector(v) + m_posit;
}

DG_INLINE dgVector dgMatrix::UntransformVector (const dgVector &v) const
{
	return UnrotateVector(v - m_posit);
}

DG_INLINE dgPlane dgMatrix::TransformPlane (const dgPlane &localPlane) const
{
	return dgPlane (RotateVector (localPlane), localPlane.m_w - (localPlane.DotProduct(UnrotateVector (m_posit)).GetScalar()));  
}

DG_INLINE dgPlane dgMatrix::UntransformPlane (const dgPlane &globalPlane) const
{
	return dgPlane (UnrotateVector (globalPlane), globalPlane.Evalue(m_posit));
}

DG_INLINE void dgMatrix::EigenVectors (const dgMatrix* const initialGuess)
{
	dgVector eigenValues;
	EigenVectors (eigenValues, initialGuess);
}

DG_INLINE dgMatrix dgMatrix::Inverse () const
{
#if 1
	// much faster inverse
	dgMatrix inv;
	dgVector::Transpose4x4 (inv[0], inv[1], inv[2], inv[3], m_front, m_up, m_right, dgVector::m_wOne);
	inv.m_posit -= inv[0] * m_posit.BroadcastX() + inv[1] * m_posit.BroadcastY() + inv[2] * m_posit.BroadcastZ();
	return inv;
#else
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, dgFloat32(0.0f)),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, dgFloat32(0.0f)),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, dgFloat32(0.0f)),
					 dgVector (- m_posit.DotProduct(m_front).GetScalar(), - m_posit.DotProduct(m_up).GetScalar(), - m_posit.DotProduct(m_right).GetScalar(), dgFloat32(1.0f)));
#endif
}

DG_INLINE bool dgMatrix::TestIdentity() const
{
	const dgMatrix& me = *this;
	for (int i = 0; i < 4; i++) {
		if (me[i][i] != dgFloat32 (1.0f)) {
			return false;
		}
		for (int j = i + 1; j < 4; j++) {
			if (me[i][j] != dgFloat32 (0.0f)) {
				return false;
			}
			if (me[j][i] != dgFloat32(0.0f)) {
				return false;
			}
		}
	}
	return true;
}

DG_INLINE bool dgMatrix::TestOrthogonal(dgFloat32 tol) const
{
	dgVector n (m_front.CrossProduct(m_up));
	dgFloat32 a = m_right.DotProduct(m_right).GetScalar();
	dgFloat32 b = m_up.DotProduct(m_up).GetScalar();
	dgFloat32 c = m_front.DotProduct(m_front).GetScalar();
	dgFloat32 d = n.DotProduct(m_right).GetScalar();

#ifdef _DEBUG
	const dgMatrix& me = *this;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			dgAssert(dgCheckFloat(me[i][j]));
		}
	}
#endif

	return (m_front[3] == dgFloat32 (0.0f)) & 
		   (m_up[3] == dgFloat32 (0.0f)) & 
		   (m_right[3] == dgFloat32 (0.0f)) & 
		   (m_posit[3] == dgFloat32 (1.0f)) &
		   (dgAbs(a - dgFloat32 (1.0f)) < tol) & 
		   (dgAbs(b - dgFloat32 (1.0f)) < tol) &
		   (dgAbs(c - dgFloat32 (1.0f)) < tol) &
		   (dgAbs(d - dgFloat32 (1.0f)) < tol); 
}

DG_INLINE bool dgMatrix::TestSymetric3x3() const
{
	const dgMatrix& me = *this;
	return (dgAbs (me[0][1] - me[1][0]) < dgFloat32 (1.0e-5f)) && 
		   (dgAbs (me[0][2] - me[2][0]) < dgFloat32 (1.0e-5f)) &&
		   (dgAbs (me[1][2] - me[2][1]) < dgFloat32 (1.0e-5f)) &&
		   (me[0][3] == dgFloat32 (0.0f)) &&
		   (me[1][3] == dgFloat32 (0.0f)) &&
		   (me[2][3] == dgFloat32 (0.0f)) &&
		   (me[3][0] == dgFloat32 (0.0f)) &&
		   (me[3][1] == dgFloat32 (0.0f)) &&
		   (me[3][2] == dgFloat32 (0.0f)) &&
		   (me[3][3] == dgFloat32 (1.0f));
}


DG_INLINE dgMatrix dgPitchMatrix(dgFloat32 ang)
{
	dgFloat32 sinAng = dgSin (ang);
	dgFloat32 cosAng = dgCos (ang);
	return dgMatrix (dgVector (dgFloat32(1.0f),  dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f)), 
		dgVector (dgFloat32(0.0f),  cosAng,          sinAng,          dgFloat32(0.0f)),
		dgVector (dgFloat32(0.0f), -sinAng,          cosAng,          dgFloat32(0.0f)), 
		dgVector (dgFloat32(0.0f),  dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f))); 

}

DG_INLINE dgMatrix dgYawMatrix(dgFloat32 ang)
{
	dgFloat32 sinAng = dgSin (ang);
	dgFloat32 cosAng = dgCos (ang);
	return dgMatrix (dgVector (cosAng,          dgFloat32(0.0f), -sinAng,          dgFloat32(0.0f)), 
		dgVector (dgFloat32(0.0f), dgFloat32(1.0f),  dgFloat32(0.0f), dgFloat32(0.0f)), 
		dgVector (sinAng,          dgFloat32(0.0f),  cosAng,          dgFloat32(0.0f)), 
		dgVector (dgFloat32(0.0f), dgFloat32(0.0f),  dgFloat32(0.0f), dgFloat32(1.0f))); 
}

DG_INLINE dgMatrix dgRollMatrix(dgFloat32 ang)
{
	dgFloat32 sinAng = dgSin (ang);
	dgFloat32 cosAng = dgCos (ang);
	return dgMatrix (dgVector ( cosAng,          sinAng,          dgFloat32(0.0f), dgFloat32(0.0f)), 
					 dgVector (-sinAng,          cosAng,          dgFloat32(0.0f), dgFloat32(0.0f)),
					 dgVector ( dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f)), 
					 dgVector ( dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f))); 
}																		 



DG_MSC_VECTOR_ALIGMENT
class dgSpatialMatrix
{
	public:
	DG_INLINE dgSpatialMatrix()
	{
	}

	DG_INLINE dgSpatialMatrix(dgFloat32 val)
	{
		const dgSpatialVector row (val);
		for (dgInt32 i = 0; i < 6; i++) {
			m_rows[i] = row;
		}
	}

	DG_INLINE dgSpatialVector& operator[] (dgInt32 i)
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return m_rows[i];
	}

	DG_INLINE const dgSpatialVector& operator[] (dgInt32 i) const
	{
		dgAssert(i < 6);
		dgAssert(i >= 0);
		return m_rows[i];
	}

	DG_INLINE dgSpatialVector VectorTimeMatrix(const dgSpatialVector& jacobian) const
	{
		dgSpatialVector tmp(m_rows[0].Scale (jacobian[0]));
		for (dgInt32 i = 1; i < 6; i++) {
			tmp = tmp + m_rows[i].Scale(jacobian[i]);
		}
		return tmp;
	}

	DG_INLINE dgSpatialVector VectorTimeMatrix(const dgSpatialVector& jacobian, dgInt32 dof) const
	{
		dgSpatialVector tmp(dgFloat32 (0.0f));
		for (dgInt32 i = 0; i < dof; i++) {
			tmp = tmp + m_rows[i].Scale(jacobian[i]);
		}
		return tmp;
	}

	DG_INLINE dgSpatialMatrix Inverse(dgInt32 rows) const
	{
		dgSpatialMatrix copy(*this);
		dgSpatialMatrix inverse(dgFloat64(0.0f));
		for (dgInt32 i = 0; i < rows; i++) {
			inverse[i][i] = dgFloat32(1.0f);
		}

		for (dgInt32 i = 0; i < rows; i++) {
			dgFloat64 val = copy[i][i];
			dgAssert(fabs(val) > dgFloat32(1.0e-12f));
			dgFloat64 den = dgFloat32(1.0f) / val;

			copy[i] = copy[i].Scale(den);
			copy[i][i] = dgFloat32(1.0f);
			inverse[i] = inverse[i].Scale(den);

			for (dgInt32 j = 0; j < i; j++) {
				dgFloat64 pivot = -copy[j][i];
				copy[j] = copy[j] + copy[i].Scale(pivot);
				inverse[j] = inverse[j] + inverse[i].Scale(pivot);
			}

			for (dgInt32 j = i + 1; j < rows; j++) {
				dgFloat64 pivot = -copy[j][i];
				copy[j] = copy[j] + copy[i].Scale(pivot);
				inverse[j] = inverse[j] + inverse[i].Scale(pivot);
			}
		}
		return inverse;
	}

	dgSpatialVector m_rows[6];
} DG_GCC_VECTOR_ALIGMENT;


#endif

