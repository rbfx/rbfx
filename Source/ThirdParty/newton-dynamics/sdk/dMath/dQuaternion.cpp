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

#include "dStdAfxMath.h"
#include "dMathDefines.h"
#include "dVector.h"
#include "dMatrix.h"
#include "dQuaternion.h"



dQuaternion::dQuaternion (const dMatrix &matrix)
{
	enum QUAT_INDEX
	{
		X_INDEX=0,
		Y_INDEX=1,
		Z_INDEX=2
	};
	static QUAT_INDEX QIndex [] = {Y_INDEX, Z_INDEX, X_INDEX};

	dFloat trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
	dAssert (matrix[2].DotProduct3(matrix[0].CrossProduct(matrix[1])) > 0.0f);

	if (trace > dFloat(0.0f)) {
		trace = dSqrt (trace + dFloat(1.0f));
		m_q0 = dFloat (0.5f) * trace;
		trace = dFloat (0.5f) / trace;
		m_q1 = (matrix[1][2] - matrix[2][1]) * trace;
		m_q2 = (matrix[2][0] - matrix[0][2]) * trace;
		m_q3 = (matrix[0][1] - matrix[1][0]) * trace;

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

		trace = dFloat(1.0f) + matrix[i][i] - matrix[j][j] - matrix[k][k];
		trace = dSqrt (trace);

		dFloat* const ptr = &m_q1;
		ptr[i] = dFloat (0.5f) * trace;
		trace = dFloat (0.5f) / trace;
		m_q0 = (matrix[j][k] - matrix[k][j]) * trace;
		ptr[j] = (matrix[i][j] + matrix[j][i]) * trace;
		ptr[k] = (matrix[i][k] + matrix[k][i]) * trace;
	}

#if _DEBUG

	dMatrix tmp (*this, matrix.m_posit);
	dMatrix unitMatrix (tmp * matrix.Inverse());
	for (int i = 0; i < 4; i ++) {
		dFloat err = dAbs (unitMatrix[i][i] - dFloat(1.0f));
		dAssert (err < dFloat (1.0e-3f));
	}

	dFloat err = dAbs (DotProduct(*this) - dFloat(1.0f));
	dAssert (err < dFloat(1.0e-3f));
#endif

}


dQuaternion::dQuaternion (const dVector &unitAxis, dFloat angle)
{
	angle *= dFloat (0.5f);
	m_q0 = dCos (angle);
	dFloat sinAng = dSin (angle);

#ifdef _DEBUG
	if (dAbs (angle) > dFloat(1.0e-6f)) {
		dAssert (dAbs (dFloat(1.0f) - unitAxis.DotProduct3(unitAxis)) < dFloat(1.0e-3f));
	} 
#endif
	m_q1 = unitAxis.m_x * sinAng;
	m_q2 = unitAxis.m_y * sinAng;
	m_q3 = unitAxis.m_z * sinAng;
}


dVector dQuaternion::CalcAverageOmega (const dQuaternion &q1, dFloat invdt) const
{
	dQuaternion q0 (*this);
	if (q0.DotProduct (q1) < 0.0f) {
		q0.Scale(-1.0f);
	}
	dQuaternion dq (q0.Inverse() * q1);
	dVector omegaDir (dq.m_q1, dq.m_q2, dq.m_q3);

	dFloat dirMag2 = omegaDir.DotProduct3(omegaDir);
	if (dirMag2	< dFloat(dFloat (1.0e-5f) * dFloat (1.0e-5f))) {
		return dVector (dFloat(0.0f), dFloat(0.0f), dFloat(0.0f), dFloat(0.0f));
	}

	dFloat dirMagInv = dFloat (1.0f) / dSqrt (dirMag2);
	dFloat dirMag = dirMag2 * dirMagInv;

	dFloat omegaMag = dFloat(2.0f) * dAtan2 (dirMag, dq.m_q0) * invdt;
	return omegaDir.Scale (dirMagInv * omegaMag);
}


dQuaternion dQuaternion::Slerp (const dQuaternion &QB, dFloat t) const 
{
	dQuaternion Q;

	dFloat dot = DotProduct (QB);
	dAssert (dot >= 0.0f);

	if ((dot + dFloat(1.0f)) > dFloat(1.0e-5f)) {
		dFloat Sclp;
		dFloat Sclq;
		if (dot < dFloat(0.995f)) {

			dFloat ang = dAcos (dot);
			dFloat sinAng = dSin (ang);

			dFloat den = dFloat(1.0f) / sinAng;

			Sclp = dSin ((dFloat(1.0f) - t ) * ang) * den;
			Sclq = dSin (t * ang) * den;

		} else  {
			Sclp = dFloat(1.0f) - t;
			Sclq = t;
		}

		Q.m_q0 = m_q0 * Sclp + QB.m_q0 * Sclq;
		Q.m_q1 = m_q1 * Sclp + QB.m_q1 * Sclq;
		Q.m_q2 = m_q2 * Sclp + QB.m_q2 * Sclq;
		Q.m_q3 = m_q3 * Sclp + QB.m_q3 * Sclq;

	} else {
		Q.m_q0 =  m_q3;
		Q.m_q1 = -m_q2;
		Q.m_q2 =  m_q1;
		Q.m_q3 =  m_q0;

		dFloat Sclp = dSin ((dFloat(1.0f) - t) * dFloat (dPi *0.5f));
		dFloat Sclq = dSin (t * dFloat (dPi * 0.5f));

		Q.m_q0 = m_q0 * Sclp + Q.m_q0 * Sclq;
		Q.m_q1 = m_q1 * Sclp + Q.m_q1 * Sclq;
		Q.m_q2 = m_q2 * Sclp + Q.m_q2 * Sclq;
		Q.m_q3 = m_q3 * Sclp + Q.m_q3 * Sclq;
	}

	dot = Q.DotProduct (Q);
	if ((dot) < (1.0f - 1.0e-4f) ) {
		dot = dFloat(1.0f) / dSqrt (dot);
		//dot = dgRsqrt (dot);
		Q.m_q0 *= dot;
		Q.m_q1 *= dot;
		Q.m_q2 *= dot;
		Q.m_q3 *= dot;
	}
	return Q;
}

dVector dQuaternion::RotateVector (const dVector& point) const
{
	dMatrix matrix (*this, dVector (0.0f, 0.0f, 0.0f, 1.0f));
	return matrix.RotateVector(point);
}

dVector dQuaternion::UnrotateVector (const dVector& point) const
{
	dMatrix matrix (*this, dVector (0.0f, 0.0f, 0.0f, 1.0f));
	return matrix.UnrotateVector(point);
}

//dVector dQuaternion::GetEulerAngles (dEulerAngleOrder order) const
void dQuaternion::GetEulerAngles(dVector& euler1, dVector& euler2, dEulerAngleOrder order) const 
{
	dMatrix matrix (*this, dVector (0.0f,0.0f,0.0f,1.0f));
	//return matrix.GetEulerAngles (order);
    matrix.GetEulerAngles (euler1, euler2, order);
}


dQuaternion dQuaternion::IntegrateOmega (const dVector& omega, dFloat timestep) const
{
	// this is correct
	dQuaternion rotation (*this);
	dFloat omegaMag2 = omega.DotProduct3(omega);
	const dFloat errAngle = 0.0125f * dDegreeToRad;
	const dFloat errAngle2 = errAngle * errAngle;
	if (omegaMag2 > errAngle2) {
		dFloat invOmegaMag = 1.0f / dSqrt (omegaMag2);
		dVector omegaAxis (omega.Scale (invOmegaMag));
		dFloat omegaAngle = invOmegaMag * omegaMag2 * timestep;
		dQuaternion deltaRotation (omegaAxis, omegaAngle);
		rotation = rotation * deltaRotation;
		rotation.Scale(1.0f / dSqrt (rotation.DotProduct (rotation)));
	}
	return rotation;
}
