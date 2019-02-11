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
#include "dgVector.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"

enum QUAT_INDEX
{
	X_INDEX = 0,
	Y_INDEX = 1,
	Z_INDEX = 2
};
static QUAT_INDEX QIndex[] = { Y_INDEX, Z_INDEX, X_INDEX };

dgQuaternion::dgQuaternion (const dgMatrix& matrix)
{
	dgFloat32 trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
	if (trace > dgFloat32(0.0f)) {
		trace = dgSqrt (trace + dgFloat32(1.0f));
		m_w = dgFloat32 (0.5f) * trace;
		trace = dgFloat32 (0.5f) / trace;
		m_x = (matrix[1][2] - matrix[2][1]) * trace;
		m_y = (matrix[2][0] - matrix[0][2]) * trace;
		m_z = (matrix[0][1] - matrix[1][0]) * trace;

	} else {
		QUAT_INDEX i = X_INDEX;
		if (matrix[Y_INDEX][Y_INDEX] > matrix[X_INDEX][X_INDEX]) {
			i = Y_INDEX;
		}
		if (matrix[Z_INDEX][Z_INDEX] > matrix[i][i]) {
			i = Z_INDEX;
		}
		QUAT_INDEX j = QIndex [i];
		QUAT_INDEX k = QIndex [j];

		trace = dgFloat32(1.0f) + matrix[i][i] - matrix[j][j] - matrix[k][k];
		trace = dgSqrt (trace);

		dgFloat32* const ptr = &m_x;
		ptr[i] = dgFloat32 (0.5f) * trace;
		trace  = dgFloat32 (0.5f) / trace;
		m_w   = (matrix[j][k] - matrix[k][j]) * trace;
		ptr[j] = (matrix[i][j] + matrix[j][i]) * trace;
		ptr[k] = (matrix[i][k] + matrix[k][i]) * trace;
	}

#ifdef _DEBUG
	dgMatrix tmp (*this, matrix.m_posit);
	dgMatrix unitMatrix (tmp * matrix.Inverse());
	for (dgInt32 i = 0; i < 4; i ++) {
		dgFloat32 err = dgAbs (unitMatrix[i][i] - dgFloat32(1.0f));
		dgAssert (err < dgFloat32 (1.0e-2f));
	}

	dgFloat32 err = dgAbs (DotProduct(*this) - dgFloat32(1.0f));
	dgAssert (err < dgFloat32(dgEPSILON * 100.0f));
#endif
}

dgQuaternion::dgQuaternion (const dgVector &unitAxis, dgFloat32 angle)
{
	angle *= dgFloat32 (0.5f);
	m_w = dgCos (angle);
	dgFloat32 sinAng = dgSin (angle);

#ifdef _DEBUG
	if (dgAbs (angle) > dgFloat32(dgEPSILON / 10.0f)) {
		dgAssert (dgAbs (dgFloat32(1.0f) - unitAxis.DotProduct3(unitAxis)) < dgFloat32(dgEPSILON * 10.0f));
	} 
#endif
	m_x = unitAxis.m_x * sinAng;
	m_y = unitAxis.m_y * sinAng;
	m_z = unitAxis.m_z * sinAng;

}

dgVector dgQuaternion::CalcAverageOmega (const dgQuaternion &q1, dgFloat32 invdt) const
{
	dgQuaternion q0 (*this);
	if (q0.DotProduct (q1) < 0.0f) {
		q0.Scale(-1.0f);
	}
	dgQuaternion dq (q0.Inverse() * q1);
	dgVector omegaDir (dq.m_x, dq.m_y, dq.m_z, dgFloat32 (0.0f));

	dgFloat32 dirMag2 = omegaDir.DotProduct3(omegaDir);
	if (dirMag2	< dgFloat32(dgFloat32 (1.0e-5f) * dgFloat32 (1.0e-5f))) {
		return dgVector (dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	}

	dgFloat32 dirMagInv = dgRsqrt (dirMag2);
	dgFloat32 dirMag = dirMag2 * dirMagInv;

	dgFloat32 omegaMag = dgFloat32(2.0f) * dgAtan2 (dirMag, dq.m_w) * invdt;
	return omegaDir.Scale (dirMagInv * omegaMag);
}

dgQuaternion dgQuaternion::Slerp (const dgQuaternion &q1, dgFloat32 t) const 
{
	dgQuaternion q0;

	dgFloat32 dot = DotProduct (q1);
	if ((dot + dgFloat32(1.0f)) > dgEPSILON) {
		dgFloat32 Sclp;
		dgFloat32 Sclq;
		if (dot < (dgFloat32(1.0f) - dgEPSILON) ) {
			dgFloat32 ang = dgAcos (dot);

			dgFloat32 sinAng = dgSin (ang);
			dgFloat32 den = dgFloat32(1.0f) / sinAng;

			Sclp = dgSin ((dgFloat32(1.0f) - t ) * ang) * den;
			Sclq = dgSin (t * ang) * den;
		} else  {
			Sclp = dgFloat32(1.0f) - t;
			Sclq = t;
		}

		q0.m_w = m_w * Sclp + q1.m_w * Sclq;
		q0.m_x = m_x * Sclp + q1.m_x * Sclq;
		q0.m_y = m_y * Sclp + q1.m_y * Sclq;
		q0.m_z = m_z * Sclp + q1.m_z * Sclq;

	} else {
		q0.m_w =  m_z;
		q0.m_x = -m_y;
		q0.m_y =  m_x;
		q0.m_z =  m_w;

		dgFloat32 Sclp = dgSin ((dgFloat32(1.0f) - t) * dgPI * dgFloat32 (0.5f));
		dgFloat32 Sclq = dgSin (t * dgPI * dgFloat32 (0.5f));

		q0.m_w = m_w * Sclp + q0.m_w * Sclq;
		q0.m_x = m_x * Sclp + q0.m_x * Sclq;
		q0.m_y = m_y * Sclp + q0.m_y * Sclq;
		q0.m_z = m_z * Sclp + q0.m_z * Sclq;
	}

	dot = q0.DotProduct (q0);
	if ((dot) < dgFloat32(1.0f - dgEPSILON * 10.0f) ) {
		//dot = dgFloat32(1.0f) / dgSqrt (dot);
		dot = dgRsqrt (dot);
		q0.m_w *= dot;
		q0.m_x *= dot;
		q0.m_y *= dot;
		q0.m_z *= dot;
	}
	return q0;
}


