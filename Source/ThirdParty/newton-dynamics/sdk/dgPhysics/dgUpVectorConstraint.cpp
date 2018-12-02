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

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgUpVectorConstraint.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgUpVectorConstraint::dgUpVectorConstraint ()
	:dgBilateralConstraint() 
{
	dgAssert ( dgInt32 (sizeof (dgUpVectorConstraint) & 15) == 0);
	dgAssert ((((dgUnsigned64) &m_localMatrix0) & 15) == 0);

	m_localMatrix0 = dgGetIdentityMatrix();
	m_localMatrix1 = dgGetIdentityMatrix();

//	dgUpVectorConstraintArray& array = * world;
//	constraint = array.GetElement();

	SetStiffness (dgFloat32 (0.995f));
	m_maxDOF = 2;
	m_constId = m_upVectorConstraint;
	m_callBack = NULL;
}

dgUpVectorConstraint::~dgUpVectorConstraint ()
{
}

/*
dgUpVectorConstraint* dgUpVectorConstraint::Create(dgWorld* world)
{
	dgUpVectorConstraint* constraint;
//	constraint = dgUpVectorConstraintArray::GetPool().GetElement();

	dgUpVectorConstraintArray& array = * world;
	constraint = array.GetElement();

	dgAssert ((((dgUnsigned64) &constraint->m_localMatrix0) & 15) == 0);

	constraint->Init ();


	constraint->SetStiffness (dgFloat32 (0.995f));
	constraint->m_maxDOF = 2;
	constraint->m_constId = dgUpVectorConstraintId;
	constraint->m_callBack = NULL;
	return constraint;
}

void dgUpVectorConstraint::Remove(dgWorld* world)
{
	dgUpVectorConstraintArray& array = * world;
	dgBilateralConstraint::Remove (world);
//	dgUpVectorConstraintArray::GetPool().RemoveElement (this);
	array.RemoveElement (this);
}
*/

void dgUpVectorConstraint::InitPinDir (const dgVector& pin)
{

	const dgMatrix& matrix = m_body0->GetMatrix();

	dgVector pivot (matrix.m_posit); 
	SetPivotAndPinDir (pivot, pin, m_localMatrix0, m_localMatrix1);

}

void dgUpVectorConstraint::SetPinDir (const dgVector& pin)
{
	m_localMatrix1 = dgMatrix (pin);
}

dgVector dgUpVectorConstraint::GetPinDir () const
{
	return m_localMatrix1.m_front;
}


void dgUpVectorConstraint::SetJointParameterCallback (dgUpVectorJointCallback callback)
{
	m_callBack = callback;
}


dgUnsigned32 dgUpVectorConstraint::JacobianDerivative (dgContraintDescritor& params)
{
	dgMatrix matrix0;
	dgMatrix matrix1;
	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	dgVector lateralDir (matrix0.m_front.CrossProduct(matrix1.m_front));

	dgInt32 ret = 0;
	dgFloat32 mag = lateralDir.DotProduct(lateralDir).GetScalar();
	if (mag > dgFloat32 (1.0e-6f)) {
		mag = dgSqrt (mag);
		lateralDir = lateralDir.Scale (dgFloat32 (1.0f) / mag);
		dgFloat32 angle = dgAsin (mag);
		CalculateAngularDerivative (0, params, lateralDir, m_stiffness, angle, &m_jointForce[0]);

		dgVector frontDir (lateralDir.CrossProduct(matrix1.m_front));
		CalculateAngularDerivative (1, params, frontDir, m_stiffness, dgFloat32 (0.0f), &m_jointForce[1]);
		ret = 2;
	} else {
		CalculateAngularDerivative (0, params, matrix0.m_up, m_stiffness, 0.0, &m_jointForce[0]);
		CalculateAngularDerivative (1, params, matrix0.m_right, m_stiffness, dgFloat32 (0.0f), &m_jointForce[1]);
		ret = 2;
	}
	return dgUnsigned32 (ret);
}



