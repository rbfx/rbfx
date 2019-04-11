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

#include "dgStdafx.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"

dgBigVector dgBigVector::m_zero (dgFloat64 (0.0f));
dgBigVector dgBigVector::m_one (dgFloat64 (1.0f));
dgBigVector dgBigVector::m_two (dgFloat64 (2.0f));
dgBigVector dgBigVector::m_half (dgFloat32 (0.5f));
dgBigVector dgBigVector::m_three (dgFloat32 (3.0f));
dgBigVector dgBigVector::m_negOne (dgFloat32 (-1.0f));
dgBigVector dgBigVector::m_wOne (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (1.0f));
dgBigVector dgBigVector::m_triplexMask (dgInt32 (-1), dgInt32 (-1),	dgInt32 (-1), dgInt32 (0));
dgBigVector dgBigVector::m_signMask (dgBigVector(dgInt32 (-1), dgInt32 (-1), dgInt32 (-1), dgInt32 (-1)).ShiftRightLogical(1));

dgBigVector dgBigVector::m_xMask (dgInt32 (-1), dgInt32 ( 0),	dgInt32 ( 0), dgInt32 ( 0));
dgBigVector dgBigVector::m_yMask (dgInt32 ( 0), dgInt32 (-1),	dgInt32 ( 0), dgInt32 ( 0));
dgBigVector dgBigVector::m_zMask (dgInt32 ( 0), dgInt32 ( 0),	dgInt32 (-1), dgInt32 ( 0));
dgBigVector dgBigVector::m_wMask (dgInt32 ( 0), dgInt32 ( 0),	dgInt32 ( 0), dgInt32 (-1));

dgSpatialVector dgSpatialVector::m_zero (dgFloat32 (0.0f));


#ifndef _NEWTON_USE_DOUBLE

dgVector dgVector::m_xMask (dgInt32 (-1), dgInt32 ( 0),	dgInt32 ( 0), dgInt32 ( 0));
dgVector dgVector::m_yMask (dgInt32 ( 0), dgInt32 (-1),	dgInt32 ( 0), dgInt32 ( 0));
dgVector dgVector::m_zMask (dgInt32 ( 0), dgInt32 ( 0),	dgInt32 (-1), dgInt32 ( 0));
dgVector dgVector::m_wMask (dgInt32 ( 0), dgInt32 ( 0),	dgInt32 ( 0), dgInt32 (-1));
dgVector dgVector::m_triplexMask (dgInt32 (-1), dgInt32 (-1), dgInt32 (-1), dgInt32 (0));
dgVector dgVector::m_signMask (dgVector(dgInt32 (-1), dgInt32 (-1), dgInt32 (-1), dgInt32 (-1)).ShiftRightLogical(1));

dgVector dgVector::m_zero (dgFloat32 (0.0f));
dgVector dgVector::m_one  (dgFloat32 (1.0f));
dgVector dgVector::m_two  (dgFloat32 (2.0f));
dgVector dgVector::m_wOne (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (1.0f));
dgVector dgVector::m_half (dgFloat32 (0.5f));
dgVector dgVector::m_three (dgFloat32 (3.0f));
dgVector dgVector::m_negOne (dgFloat32 (-1.0f));

#endif


dgMatrix dgMatrix::m_zeroMatrix (dgVector (dgFloat32(0.0f)),
								 dgVector (dgFloat32(0.0f)),
								 dgVector (dgFloat32(0.0f)),
								 dgVector (dgFloat32(0.0f)));

dgMatrix dgMatrix::m_identityMatrix (dgVector (dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f)),
									 dgVector (dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f)),
									 dgVector (dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.0f)),
									 dgVector (dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(1.0f)));


#ifndef _NEWTON_USE_DOUBLE
#endif

const dgMatrix& dgGetIdentityMatrix()
{
	return dgMatrix::m_identityMatrix;
}

const dgMatrix& dgGetZeroMatrix ()
{
	return dgMatrix::m_zeroMatrix;
}


dgMatrix::dgMatrix (const dgQuaternion &rotation, const dgVector &position)
{
	dgFloat32 x2 = dgFloat32 (2.0f) * rotation.m_x * rotation.m_x;
	dgFloat32 y2 = dgFloat32 (2.0f) * rotation.m_y * rotation.m_y;
	dgFloat32 z2 = dgFloat32 (2.0f) * rotation.m_z * rotation.m_z;

#ifdef _DEBUG
	dgFloat32 w2 = dgFloat32 (2.0f) * rotation.m_w * rotation.m_w;
	dgAssert (dgAbs (w2 + x2 + y2 + z2 - dgFloat32(2.0f)) <dgFloat32 (1.0e-3f));
#endif

	dgFloat32 xy = dgFloat32 (2.0f) * rotation.m_x * rotation.m_y;
	dgFloat32 xz = dgFloat32 (2.0f) * rotation.m_x * rotation.m_z;
	dgFloat32 xw = dgFloat32 (2.0f) * rotation.m_x * rotation.m_w;
	dgFloat32 yz = dgFloat32 (2.0f) * rotation.m_y * rotation.m_z;
	dgFloat32 yw = dgFloat32 (2.0f) * rotation.m_y * rotation.m_w;
	dgFloat32 zw = dgFloat32 (2.0f) * rotation.m_z * rotation.m_w;

	m_front = dgVector (dgFloat32(1.0f) - y2 - z2, xy + zw,                   xz - yw				    , dgFloat32(0.0f));
	m_up    = dgVector (xy - zw,                   dgFloat32(1.0f) - x2 - z2, yz + xw					, dgFloat32(0.0f));
	m_right = dgVector (xz + yw,                   yz - xw,                   dgFloat32(1.0f) - x2 - y2 , dgFloat32(0.0f));


	m_posit.m_x = position.m_x;
	m_posit.m_y = position.m_y;
	m_posit.m_z = position.m_z;
	m_posit.m_w = dgFloat32(1.0f);
}


dgMatrix::dgMatrix (const dgMatrix& transformMatrix, const dgVector& scale, const dgMatrix& stretchAxis)
{
	dgMatrix scaledAxis;
	scaledAxis[0] = stretchAxis[0].Scale (scale[0]);
	scaledAxis[1] = stretchAxis[1].Scale (scale[1]);
	scaledAxis[2] = stretchAxis[2].Scale (scale[2]);
	scaledAxis[3] = stretchAxis[3];

	*this = stretchAxis.Transpose() * scaledAxis * transformMatrix;
}

dgMatrix dgMatrix::Multiply3X3 (const dgMatrix &B) const
{
	return dgMatrix (B.m_front * m_front.BroadcastX() + B.m_up * m_front.BroadcastY() + B.m_right * m_front.BroadcastZ(), 
					 B.m_front * m_up.BroadcastX()    + B.m_up * m_up.BroadcastY()    + B.m_right * m_up.BroadcastZ(), 
					 B.m_front * m_right.BroadcastX() + B.m_up * m_right.BroadcastY() + B.m_right * m_right.BroadcastZ(), 
					 dgVector::m_wOne); 
}

dgMatrix dgMatrix::operator* (const dgMatrix &B) const
{
	return dgMatrix (B.m_front * m_front.BroadcastX() + B.m_up * m_front.BroadcastY() + B.m_right * m_front.BroadcastZ() + B.m_posit * m_front.BroadcastW(), 
					 B.m_front * m_up.BroadcastX()    + B.m_up * m_up.BroadcastY()    + B.m_right * m_up.BroadcastZ()    + B.m_posit * m_up.BroadcastW(), 
					 B.m_front * m_right.BroadcastX() + B.m_up * m_right.BroadcastY() + B.m_right * m_right.BroadcastZ() + B.m_posit * m_right.BroadcastW(), 
					 B.m_front * m_posit.BroadcastX() + B.m_up * m_posit.BroadcastY() + B.m_right * m_posit.BroadcastZ() + B.m_posit * m_posit.BroadcastW()); 
}



void dgMatrix::TransformTriplex (dgFloat32* const dst, dgInt32 dstStrideInBytes, const dgFloat32* const src, dgInt32 srcStrideInBytes, dgInt32 count) const
{
	dgInt32 dstStride = dgInt32 (dstStrideInBytes /sizeof (dgFloat32));
	dgInt32 srcStride = dgInt32 (srcStrideInBytes / sizeof (dgFloat32));

	dgInt32 dstIndex = 0;
	dgInt32 srcIndex = 0;
	for (dgInt32 i = 0 ; i < count; i ++ ) {
		dgFloat32 x = src[srcIndex + 0];
		dgFloat32 y = src[srcIndex + 1];
		dgFloat32 z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}

#ifndef _NEWTON_USE_DOUBLE
void dgMatrix::TransformTriplex (dgFloat64* const dst, dgInt32 dstStrideInBytes, const dgFloat64* const src, dgInt32 srcStrideInBytes, dgInt32 count) const
{
	dgInt32 dstStride = dgInt32 (dstStrideInBytes /sizeof (dgFloat64));
	dgInt32 srcStride = dgInt32 (srcStrideInBytes / sizeof (dgFloat64));

	dgInt32 dstIndex = 0;
	dgInt32 srcIndex = 0;
	for (dgInt32 i = 0 ; i < count; i ++ ) {
		dgFloat64 x = src[srcIndex + 0];
		dgFloat64 y = src[srcIndex + 1];
		dgFloat64 z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}

void dgMatrix::TransformTriplex (dgFloat64* const dst, dgInt32 dstStrideInBytes, const dgFloat32* const src, dgInt32 srcStrideInBytes, dgInt32 count) const
{
	dgInt32 dstStride = dgInt32 (dstStrideInBytes /sizeof (dgFloat64));
	dgInt32 srcStride = dgInt32 (srcStrideInBytes / sizeof (dgFloat32));

	dgInt32 dstIndex = 0;
	dgInt32 srcIndex = 0;
	for (dgInt32 i = 0 ; i < count; i ++ ) {
		dgFloat64 x = src[srcIndex + 0];
		dgFloat64 y = src[srcIndex + 1];
		dgFloat64 z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}
#endif

void dgMatrix::TransformBBox (const dgVector& p0local, const dgVector& p1local, dgVector& p0, dgVector& p1) const
{
	const dgMatrix& matrix = *this;
	//dgVector size ((p1local - p0local).Scale (dgFloat32 (0.5f)));
	//dgVector center (TransformVector ((p1local + p0local).Scale (dgFloat32 (0.5f))));
	dgVector size ((p1local - p0local) * dgVector::m_half);
	dgVector center (TransformVector ((p1local + p0local) * dgVector::m_half));
	dgVector extends (size.m_x * dgAbs(matrix[0][0]) + size.m_y * dgAbs(matrix[1][0]) + size.m_z * dgAbs(matrix[2][0]),  
					  size.m_x * dgAbs(matrix[0][1]) + size.m_y * dgAbs(matrix[1][1]) + size.m_z * dgAbs(matrix[2][1]),  
	                  size.m_x * dgAbs(matrix[0][2]) + size.m_y * dgAbs(matrix[1][2]) + size.m_z * dgAbs(matrix[2][2]), dgFloat32 (0.0f));  

	p0 = center - extends;
	p1 = center + extends;
}

dgMatrix dgMatrix::Inverse4x4 () const
{
	dgMatrix tmp (*this);
	dgMatrix inv (dgGetIdentityMatrix());
	for (dgInt32 i = 0; i < 4; i++) {
		dgFloat32 pivot = dgAbs(tmp[i][i]);
		if (pivot < dgFloat32(0.01f)) {
			dgInt32 permute = i;
			for (dgInt32 j = i + 1; j < 4; j++) {
				dgFloat32 pivot1 = dgAbs(tmp[j][i]);
				if (pivot1 > pivot) {
					permute = j;
					pivot = pivot1;
				}
			}
			dgAssert(pivot > dgFloat32(1.0e-6f));
			if (permute != i) {
				//for (dgInt32 j = 0; j < 4; j++) {
					dgSwap(inv[i], inv[permute]);
					dgSwap(tmp[i], tmp[permute]);
				//}
			}
		}

		for (dgInt32 j = i + 1; j < 4; j++) {
//			dgFloat32 scale = tmp[j][i] / tmp[i][i];
//			for (dgInt32 k = 0; k < 4; k++) {
//				tmp[j][k] -= scale * tmp[i][k];
//				inv[j][k] -= scale * inv[i][k];
//			}
			dgVector scale (tmp[j][i] / tmp[i][i]);
			tmp[j] -= tmp[i] * scale;
			inv[j] -= inv[i] * scale;
			tmp[j][i] = dgFloat32 (0.0f);
		}
	}

	for (dgInt32 i = 3; i >= 0; i--) {
		dgVector acc(dgVector::m_zero);
		for (dgInt32 j = i + 1; j < 4; j++) {
//			dgFloat32 pivot = tmp[i][j];
//			for (dgInt32 k = 0; k < 4; k++) {
//				acc[k] += pivot * inv[j][k];
//			}
			dgVector pivot(tmp[i][j]);
			acc += pivot * inv[j];
		}
		//dgFloat32 den = dgFloat32(1.0f) / tmp[i][i];
		//for (dgInt32 k = 0; k < 4; k++) {
		//	inv[i][k] = den * (inv[i][k] - acc[k]);
		//}
		dgVector den(dgFloat32(1.0f) / tmp[i][i]);
		inv[i] = den * (inv[i] - acc);
	}
#ifdef _DEBUG
	tmp = *this * inv;
	for (dgInt32 i = 0; i < 4; i++) {
		dgAssert(dgAbs(tmp[i][i] - dgFloat32(1.0f)) < dgFloat32(1.0e-6f));
		for (dgInt32 j = i + 1; j < 4; j++) {
			dgAssert(dgAbs(tmp[i][j]) < dgFloat32(1.0e-6f));
			dgAssert(dgAbs(tmp[j][i]) < dgFloat32(1.0e-6f));
		}
	}
#endif

	return inv;
}

dgMatrix dgMatrix::Symetric3by3Inverse () const
{
	dgMatrix copy(*this);
	dgMatrix inverse(dgGetIdentityMatrix());
	for (dgInt32 i = 0; i < 3; i++) {
		dgVector den(dgFloat32(1.0f) / copy[i][i]);
		copy[i] = copy[i] * den;
		inverse[i] = inverse[i] * den;
		for (dgInt32 j = 0; j < 3; j++) {
			if (j != i) {
				dgVector pivot(copy[j][i]);
				copy[j] -= copy[i] * pivot;
				inverse[j] -= inverse[i] * pivot;
			}
		}
	}
	
#ifdef _DEBUG
	dgMatrix test(*this * inverse);
	dgAssert(dgAbs(test[0][0] - dgFloat32(1.0f)) < dgFloat32(0.01f));
	dgAssert(dgAbs(test[1][1] - dgFloat32(1.0f)) < dgFloat32(0.01f));
	dgAssert(dgAbs(test[2][2] - dgFloat32(1.0f)) < dgFloat32(0.01f));
#endif

	return inverse;
}

void dgMatrix::CalcPitchYawRoll (dgVector& euler0, dgVector& euler1) const
{
	const dgMatrix& matrix = *this;
	dgAssert (matrix[2].DotProduct3(matrix[0].CrossProduct(matrix[1])) > 0.0f);
	dgAssert (dgAbs (matrix[2].DotProduct3(matrix[0].CrossProduct(matrix[1])) - dgFloat32 (1.0f)) < dgFloat32 (1.0e-4f));
/*
	// Assuming the angles are in radians.
	if (matrix[0][2] > dgFloat32 (0.99995f)) {
		dgFloat32 picth0 = dgFloat32 (0.0f);
		dgFloat32 yaw0 = dgFloat32 (-dgPI * 0.5f);
		dgFloat32 roll0 = - dgAtan2(matrix[2][1], matrix[1][1]);
		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth0;
		euler1[1] = yaw0;
		euler1[2] = roll0;

	} else if (matrix[0][2] < dgFloat32 (-0.99995f)) {
		dgFloat32 picth0 = dgFloat32 (0.0f);
		dgFloat32 yaw0 = dgFloat32 (dgPI * 0.5f);
		dgFloat32 roll0 = dgAtan2(matrix[2][1], matrix[1][1]);
		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth0;
		euler1[1] = yaw0;
		euler1[2] = roll0;
	} else {
		dgFloat32 yaw0 = -dgAsin ( matrix[0][2]);
		dgFloat32 yaw1 = dgFloat32 (dgPI) - yaw0;
		dgFloat32 sign0 = dgSign(dgCos (yaw0));
		dgFloat32 sign1 = dgSign(dgCos (yaw1));

		dgFloat32 picth0 = dgAtan2(matrix[1][2] * sign0, matrix[2][2] * sign0);
		dgFloat32 picth1 = dgAtan2(matrix[1][2] * sign1, matrix[2][2] * sign1);

		dgFloat32 roll0 = dgAtan2(matrix[0][1] * sign0, matrix[0][0] * sign0);
		dgFloat32 roll1 = dgAtan2(matrix[0][1] * sign1, matrix[0][0] * sign1);

		if (yaw1 > dgFloat32 (dgPI)) {
			yaw1 -= dgFloat32 (2.0f * dgPI);
		}

		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth1;
		euler1[1] = yaw1;
		euler1[2] = roll1;
	}
*/

	// Assuming the angles are in radians.
	if (matrix[0][2] > dgFloat32 (0.99995f)) {
		dgFloat32 picth0 = dgFloat32(0.0f);
		dgFloat32 yaw0 = dgFloat32(-dgPI * 0.5f);
		dgFloat32 roll0 = -dgAtan2(matrix[2][1], matrix[1][1]);
		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth0;
		euler1[1] = yaw0;
		euler1[2] = roll0;

	} else if (matrix[0][2] < dgFloat32 (-0.99995f)) {
		dgFloat32 picth0 = dgFloat32 (0.0f);
		dgFloat32 yaw0 = dgFloat32(dgPI * 0.5f);
		dgFloat32 roll0 = dgAtan2(matrix[2][1], matrix[1][1]);
		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth0;
		euler1[1] = yaw0;
		euler1[2] = roll0;

	} else {
		dgFloat32 yaw0 = -dgAsin(matrix[0][2]);
		dgFloat32 yaw1 = dgFloat32(dgPI) - yaw0;

		dgFloat32 picth0 = dgAtan2( matrix[1][2],  matrix[2][2]);
		dgFloat32 picth1 = dgAtan2(-matrix[1][2], -matrix[2][2]);

		dgFloat32 roll0 = dgAtan2( matrix[0][1],  matrix[0][0]);
		dgFloat32 roll1 = dgAtan2(-matrix[0][1], -matrix[0][0]);

		if (yaw1 > dgFloat32 (dgPI)) {
			yaw1 -= dgFloat32 (2.0f * dgPI);
		}

		euler0[0] = picth0;
		euler0[1] = yaw0;
		euler0[2] = roll0;

		euler1[0] = picth1;
		euler1[1] = yaw1;
		euler1[2] = roll1;
	}

	euler0[3] = dgFloat32(0.0f);
	euler1[3] = dgFloat32(0.0f);

#ifdef _DEBUG
	dgMatrix m0 (dgPitchMatrix (euler0[0]) * dgYawMatrix(euler0[1]) * dgRollMatrix(euler0[2]));
	dgMatrix m1 (dgPitchMatrix (euler1[0]) * dgYawMatrix(euler1[1]) * dgRollMatrix(euler1[2]));
	for (dgInt32 i = 0; i < 3; i ++) {
		for (dgInt32 j = 0; j < 3; j ++) {
			dgFloat32 error = dgAbs (m0[i][j] - matrix[i][j]);
			dgAssert (error < 5.0e-2f);
			error = dgAbs (m1[i][j] - matrix[i][j]);
			dgAssert (error < 5.0e-2f);
		}
	}
#endif
}


void dgMatrix::PolarDecomposition (dgMatrix& transformMatrix, dgVector& scale, dgMatrix& stretchAxis, const dgMatrix* const initialStretchAxis) const
{
	// a polar decomposition decompose matrix A = O * S
	// where S = sqrt (transpose (L) * L)

/*
	// calculate transpose (L) * L 
	dgMatrix LL ((*this) * Transpose());

	// check is this is a pure uniformScale * rotation * translation
	dgFloat32 det2 = (LL[0][0] + LL[1][1] + LL[2][2]) * dgFloat32 (1.0f / 3.0f);

	dgFloat32 invdet2 = 1.0f / det2;

	dgMatrix pureRotation (LL);
	pureRotation[0] = pureRotation[0].Scale (invdet2);
	pureRotation[1] = pureRotation[1].Scale (invdet2);
	pureRotation[2] = pureRotation[2].Scale (invdet2);

	dgFloat32 sign = ((((*this)[0] * (*this)[1]) % (*this)[2]) > 0.0f) ? 1.0f : -1.0f;
	dgFloat32 det = (pureRotation[0] * pureRotation[1]) % pureRotation[2];
	if (dgAbs (det - dgFloat32 (1.0f)) < dgFloat32 (1.0e-5f)) {
		// this is a pure scale * rotation * translation
		det = sign * dgSqrt (det2);
		scale[0] = det;
		scale[1] = det;
		scale[2] = det;
		det = dgFloat32 (1.0f)/ det;
		transformMatrix.m_front = m_front.Scale (det);
		transformMatrix.m_up = m_up.Scale (det);
		transformMatrix.m_right = m_right.Scale (det);
		transformMatrix[0][3] = dgFloat32 (0.0f);
		transformMatrix[1][3] = dgFloat32 (0.0f);
		transformMatrix[2][3] = dgFloat32 (0.0f);
		transformMatrix.m_posit = m_posit;
		stretchAxis = dgGetIdentityMatrix();

	} else {
		stretchAxis = LL;
		stretchAxis.EigenVectors (scale);

		// I need to deal with by seeing of some of the Scale are duplicated
		// do this later (maybe by a given rotation around the non uniform axis but I do not know if it will work)
		// for now just us the matrix

		scale[0] = sign * dgSqrt (scale[0]);
		scale[1] = sign * dgSqrt (scale[1]);
		scale[2] = sign * dgSqrt (scale[2]);
		scale[3] = dgFloat32 (0.0f);

		dgMatrix scaledAxis;
		scaledAxis[0] = stretchAxis[0].Scale (dgFloat32 (1.0f) / scale[0]);
		scaledAxis[1] = stretchAxis[1].Scale (dgFloat32 (1.0f) / scale[1]);
		scaledAxis[2] = stretchAxis[2].Scale (dgFloat32 (1.0f) / scale[2]);
		scaledAxis[3] = stretchAxis[3];
		dgMatrix symetricInv (stretchAxis.Transpose() * scaledAxis);

		transformMatrix = symetricInv * (*this);
		transformMatrix.m_posit = m_posit;
	}
*/
/*
// test the fucking factorization 
dgMatrix xxxxx(dgRollMatrix(30.0f * dgDEG2RAD));
xxxxx = dgYawMatrix(30.0f * dgDEG2RAD) * xxxxx;
dgMatrix xxxxx1(dgGetIdentityMatrix());
xxxxx1[0][0] = 2.0f;
dgMatrix xxxxx2(xxxxx.Inverse() * xxxxx1 * xxxxx);
dgMatrix xxxxx3 (xxxxx2);
xxxxx2.EigenVectors(scale);
dgMatrix xxxxx4(xxxxx2.Inverse() * xxxxx1 * xxxxx2);
*/

	const dgMatrix& me = *this;
	dgFloat32 sign = dgSign (me[2].DotProduct3 (me[0].CrossProduct(me[1])));
	stretchAxis = me * Transpose();
	stretchAxis.EigenVectors (scale);

	// I need to deal with by seeing of some of the Scale are duplicated
	// do this later (maybe by a given rotation around the non uniform axis but I do not know if it will work)
	// for now just us the matrix

	scale[0] = sign * dgSqrt (scale[0]);
	scale[1] = sign * dgSqrt (scale[1]);
	scale[2] = sign * dgSqrt (scale[2]);
	scale[3] = dgFloat32 (0.0f);

	dgMatrix scaledAxis;
	scaledAxis[0] = stretchAxis[0].Scale (dgFloat32 (1.0f) / scale[0]);
	scaledAxis[1] = stretchAxis[1].Scale (dgFloat32 (1.0f) / scale[1]);
	scaledAxis[2] = stretchAxis[2].Scale (dgFloat32 (1.0f) / scale[2]);
	scaledAxis[3] = stretchAxis[3];
	dgMatrix symetricInv (stretchAxis.Transpose() * scaledAxis);

	transformMatrix = symetricInv * (*this);
	transformMatrix.m_posit = m_posit;
}


/*
for some reason I cannot get this to work
void dgMatrix::EigenVectors (dgVector& eigenValues, const dgMatrix& initialGuess)
{
	dgVector xxx1;
	dgMatrix xxx0 (*this);
	xxx0.EigenVectors____ (xxx1, initialGuess);

	static dgInt32 offDiagonalIndex[][2] = {{0, 1}, {0, 2}, {1, 2}};

	dgMatrix eigenVector (dgGetIdentityMatrix());
	dgMatrix& mat = *this;

	for (dgInt32 m = 0; m < 20; m ++) {
		dgFloat32 ajk[3];
		ajk[0] = dgAbs(mat[0][1]);
		ajk[1] = dgAbs(mat[0][2]);
		ajk[2] = dgAbs(mat[1][2]);
		dgFloat32 sm = ajk[0] + ajk[1] + ajk[2];
		if (sm < dgFloat32 (1.0e-12f)) {
			eigenValues = dgVector (mat[0][0], mat[1][1], mat[2][2], dgFloat32 (1.0f));
			*this = eigenVector;
			return;
		}

		dgInt32 index = 0;
		dgFloat32 maxA = ajk[0];
		if (maxA < ajk[1]) {
			index = 1;
			maxA = ajk[1];
		}

		if (maxA < ajk[2]) {
			index = 2;
			maxA = ajk[2];
		}

		dgInt32 j = offDiagonalIndex[index][0];
		dgInt32 k = offDiagonalIndex[index][1];

		dgFloat32 Ajj = mat[j][j];
		dgFloat32 Akk = mat[k][k];
		dgFloat32 Ajk = mat[j][k];
		dgFloat32 phi = dgFloat32 (0.5f) * dgAtan2 (dgFloat32 (2.0f) * Ajk, Akk - Ajj);

		dgFloat32 c = dgCos (phi);
		dgFloat32 s = dgSin (phi);

		dgMatrix givensRotation (dgGetIdentityMatrix());
		givensRotation[j][j] = c;
		givensRotation[k][k] = c;
		givensRotation[j][k] = -s;
		givensRotation[k][j] = s;

		eigenVector = eigenVector * givensRotation;
		mat = givensRotation * mat * givensRotation.Transpose();
		mat[j][k] = dgFloat32 (0.0f);
		mat[k][j] = dgFloat32 (0.0f);
	}
	dgAssert (0);
}
*/

void dgMatrix::EigenVectors (dgVector &eigenValues, const dgMatrix* const initialGuess)
{
	dgMatrix& mat = *this;
	dgMatrix eigenVectors (dgGetIdentityMatrix());
	if (initialGuess) {
		eigenVectors = *initialGuess;
		mat = eigenVectors.Transpose4X4() * mat * eigenVectors;
	}

	dgVector d (mat[0][0], mat[1][1], mat[2][2], dgFloat32 (0.0f)); 
	dgVector b (d);
	for (dgInt32 i = 0; i < 50; i++) {
		dgFloat32 sm = dgAbs(mat[0][1]) + dgAbs(mat[0][2]) + dgAbs(mat[1][2]);

		if (sm < dgFloat32 (1.0e-12f)) {
			// order the eigenvalue vectors	
			dgVector tmp (eigenVectors.m_front.CrossProduct(eigenVectors.m_up));
			if (tmp.DotProduct3(eigenVectors.m_right) < dgFloat32(0.0f)) {
				dgAssert (0.0f);
				//eigenVectors.m_right = eigenVectors.m_right.Scale (-dgFloat32(1.0f));
				eigenVectors.m_right = eigenVectors.m_right * dgVector::m_negOne;
			}
			break;
		}

		dgFloat32 thresh = dgFloat32 (0.0f);
		if (i < 3) {
			thresh = (dgFloat32)(0.2f / 9.0f) * sm;
		}

		dgVector z (dgVector::m_zero);
		for (dgInt32 ip = 0; ip < 2; ip ++) {
			for (dgInt32 iq = ip + 1; iq < 3; iq ++) {
				dgFloat32 g = dgFloat32 (100.0f) * dgAbs(mat[ip][iq]);
				if ((i > 3) && ((dgAbs(d[ip]) + g) == dgAbs(d[ip])) && ((dgAbs(d[iq]) + g) == dgAbs(d[iq]))) {
					mat[ip][iq] = dgFloat32 (0.0f);
				} else if (dgAbs(mat[ip][iq]) > thresh) {

					dgFloat32 t;
					dgFloat32 h = d[iq] - d[ip];
					if (dgAbs(h) + g == dgAbs(h)) {
						t = mat[ip][iq] / h;
					} else {
						dgFloat32 theta = dgFloat32 (0.5f) * h / mat[ip][iq];
						t = dgFloat32(1.0f) / (dgAbs(theta) + dgSqrt(dgFloat32(1.0f) + theta * theta));
						if (theta < dgFloat32 (0.0f)) {
							t = -t;
						}
					}
					dgFloat32 c = dgRsqrt (dgFloat32 (1.0f) + t * t); 
					dgFloat32 s = t * c; 
					dgFloat32 tau = s / (dgFloat32(1.0f) + c); 
					h = t * mat[ip][iq];
					z[ip] -= h; 
					z[iq] += h; 
					d[ip] -= h; 
					d[iq] += h;
					mat[ip][iq] = dgFloat32(0.0f);

					for (dgInt32 j = 0; j <= ip - 1; j ++) {
						dgFloat32 g0 = mat[j][ip]; 
						dgFloat32 h0 = mat[j][iq]; 
						mat[j][ip] = g0 - s * (h0 + g0 * tau); 
						mat[j][iq] = h0 + s * (g0 - h0 * tau);
					}
					for (dgInt32 j = ip + 1; j <= iq - 1; j ++) {
						dgFloat32 g0 = mat[ip][j]; 
						dgFloat32 h0 = mat[j][iq]; 
						mat[ip][j] = g0 - s * (h0 + g0 * tau); 
						mat[j][iq] = h0 + s * (g0 - h0 * tau);
					}
					for (dgInt32 j = iq + 1; j < 3; j ++) {
						dgFloat32 g0 = mat[ip][j]; 
						dgFloat32 h0 = mat[iq][j]; 
						mat[ip][j] = g0 - s * (h0 + g0 * tau); 
						mat[iq][j] = h0 + s * (g0 - h0 * tau);
					}

					dgVector sv (s);
					dgVector tauv (tau);
					dgVector gv (eigenVectors[ip]);
					dgVector hv (eigenVectors[iq]);
					eigenVectors[ip] -= sv * (hv + gv * tauv); 
					eigenVectors[iq] += sv * (gv - hv * tauv);
				}
			}
		}

		b += z; 
		d = b; 
	}

	eigenValues = d;
	*this = eigenVectors;
}

dgSpatialMatrix dgSpatialMatrix::Inverse(dgInt32 rows) const
{
	dgSpatialMatrix tmp(*this);
	dgSpatialMatrix inv(dgFloat64(0.0f));
	for (dgInt32 i = 0; i < rows; i++) {
		inv[i][i] = dgFloat32(1.0f);
	}

#if 0
	for (dgInt32 i = 0; i < rows; i++) {
		dgFloat64 val = tmp[i][i];
		dgAssert(fabs(val) > dgFloat32(1.0e-12f));
		dgFloat64 den = dgFloat32(1.0f) / val;

		tmp[i] = tmp[i].Scale(den);
		tmp[i][i] = dgFloat32(1.0f);
		inv[i] = inv[i].Scale(den);

		for (dgInt32 j = 0; j < i; j++) {
			dgFloat64 pivot = -tmp[j][i];
			tmp[j] = tmp[j] + tmp[i].Scale(pivot);
			inv[j] = inv[j] + inv[i].Scale(pivot);
		}

		for (dgInt32 j = i + 1; j < rows; j++) {
			dgFloat64 pivot = -tmp[j][i];
			tmp[j] = tmp[j] + tmp[i].Scale(pivot);
			inv[j] = inv[j] + inv[i].Scale(pivot);
		}
	}

#else

	for (dgInt32 i = 0; i < rows; i++) {
		dgFloat64 pivot = dgAbs(tmp[i][i]);
		if (pivot < dgFloat64(0.01f)) {
			int permute = i;
			for (dgInt32 j = i + 1; j < rows; j++) {
				dgFloat64 pivot1 = dgAbs(tmp[j][i]);
				if (pivot1 > pivot) {
					permute = j;
					pivot = pivot1;
				}
			}
			dgAssert(pivot > dgFloat32(1.0e-6f));
			if (permute != i) {
				for (dgInt32 j = 0; j < rows; j++) {
					dgSwap(tmp[i][j], tmp[permute][j]);
					dgSwap(tmp[i][j], tmp[permute][j]);
				}
			}
		}

		for (dgInt32 j = i + 1; j < rows; j++) {
			dgFloat64 scale = tmp[j][i] / tmp[i][i];
			tmp[j][i] = dgFloat64(0.0f);
			for (int k = i + 1; k < rows; k++) {
				tmp[j][k] -= scale * tmp[i][k];
			}
			for (int k = 0; k <= i; k++) {
				inv[j][k] -= scale * inv[i][k];
			}
		}
	}

	for (dgInt32 i = rows - 1; i >= 0; i--) {
		dgSpatialVector acc(dgFloat64(0.0f));
		for (dgInt32 j = i + 1; j < rows; j++) {
			dgFloat64 pivot = tmp[i][j];
			for (int k = 0; k < rows; k++) {
				acc[k] += pivot * inv[j][k];
			}
		}
		dgFloat64 den = dgFloat64(1.0f) / tmp[i][i];
		for (dgInt32 k = 0; k < rows; k++) {
			inv[i][k] = den * (inv[i][k] - acc[k]);
		}
	}
#endif

#ifdef _DEBUG
	for (dgInt32 i = 0; i < rows; i++) {
		for (dgInt32 j = 0; j < rows; j++) {
			tmp[i][j] = m_rows[j][i];
		}
	}
	for (dgInt32 i = 0; i < rows; i++) {
		dgSpatialVector v(inv.VectorTimeMatrix(tmp[i], rows));
		dgAssert(dgAbs(v[i] - dgFloat64(1.0f)) < dgFloat64(1.0e-6f));
		for (dgInt32 j = 0; j < rows; j++) {
			if (j != i) {
				dgAssert(dgAbs(v[j]) < dgFloat64(1.0e-6f));
			}
		}
	}
#endif

	return inv;
}
