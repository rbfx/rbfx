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

#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonTransformLerp.h"

dNewtonTransformLerp::dNewtonTransformLerp ()
	:m_posit0(0.0f, 0.0f, 0.0f, 1.0f)
	,m_posit1(m_posit0)
	,m_rotat0()
	,m_rotat1(m_rotat0)
	,m_lock(0)
{
}

dNewtonTransformLerp::dNewtonTransformLerp (const dFloat* const matrix)
	:m_posit0(matrix[12], matrix[13], matrix[14], matrix[15])
	,m_posit1(m_posit0)
	,m_rotat0(dMatrix (matrix))
	,m_rotat1(m_rotat0)
	,m_lock(0)
{
}

dNewtonTransformLerp::~dNewtonTransformLerp()
{
}


void dNewtonTransformLerp::SetTargetMatrixLow (const dFloat* const matrix)
{
	dMatrix mat (matrix);
	m_posit1 = mat.m_posit;
	m_rotat1 = dQuaternion (mat);

	dFloat angle = m_rotat0.DotProduct(m_rotat1);
	if (angle < 0.0f) {
		m_rotat1.Scale(-1.0f);
	}
}


void dNewtonTransformLerp::GetTargetMatrix (dFloat* const matrix) const
{
	dNewton::ScopeLock scopelock (&m_lock);
	dMatrix mat (m_rotat1, m_posit1);
	memcpy (matrix, &mat[0][0], sizeof (dMatrix));
}

void dNewtonTransformLerp::GetBaseMatrix (dFloat* const matrix) const
{
	dNewton::ScopeLock scopelock (&m_lock);
	dMatrix mat (m_rotat0, m_posit0);
	memcpy (matrix, &mat[0][0], sizeof (dMatrix));
}

void dNewtonTransformLerp::SetTargetMatrix (const dFloat* const matrix)
{
	dNewton::ScopeLock scopelock (&m_lock);
	SetTargetMatrixLow (matrix);
}


void dNewtonTransformLerp::ResetMatrix (const dFloat* const matrix)
{
	dNewton::ScopeLock scopelock (&m_lock);
	SetTargetMatrixLow (matrix);
	m_posit0 = m_posit1;
	m_rotat0 = m_rotat1;
}


void dNewtonTransformLerp::Update (const dFloat* const matrix)
{
	dNewton::ScopeLock scopelock (&m_lock);
	m_posit0 = m_posit1;
	m_rotat0 = m_rotat1;
	SetTargetMatrixLow (matrix);
}


void dNewtonTransformLerp::InterpolateMatrix (dFloat param, dFloat* const matrix) const
{
	dNewton::ScopeLock scopelock (&m_lock);
	dVector posit (m_posit0 + (m_posit1 - m_posit0).Scale (param));
	dQuaternion rotation (m_rotat0.Slerp(m_rotat1, param));
	dMatrix tmpMatrix (rotation, posit);
	memcpy (matrix, &tmpMatrix[0][0], sizeof (dMatrix));
}