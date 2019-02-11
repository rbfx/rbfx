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
#include "dgWorld.h"
#include "dgDynamicBody.h"
#include "dgCollisionInstance.h"
#include "dgCollisionLumpedMassParticles.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgVector dgDynamicBody::m_equilibriumError2 (DG_ERR_TOLERANCE2);


dgDynamicBodyAsymetric::dgDynamicBodyAsymetric()
	:dgDynamicBody()
	, m_principalAxis(dgGetIdentityMatrix())
{
	m_type = m_dynamicBody;
	m_rtti |= m_dynamicBodyAsymentricRTTI;
	dgAssert(dgInt32(sizeof(dgDynamicBody) & 0x0f) == 0);

}

dgDynamicBodyAsymetric::dgDynamicBodyAsymetric(dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber)
	:dgDynamicBody(world, collisionNode, serializeCallback, userData, revisionNumber)
	, m_principalAxis(dgGetIdentityMatrix())
{
	m_type = m_dynamicBody;
	m_rtti |= m_dynamicBodyRTTI;
	serializeCallback(userData, &m_principalAxis, sizeof(m_principalAxis));
}

void dgDynamicBodyAsymetric::Serialize(const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData)
{
	dgDynamicBody::Serialize(collisionRemapId, serializeCallback, userData);
	serializeCallback(userData, &m_principalAxis, sizeof(m_principalAxis));
}


void dgDynamicBodyAsymetric::SetMassMatrix(dgFloat32 mass, const dgMatrix& inertia)
{
	dgVector II;
	m_principalAxis = inertia;
	m_principalAxis.EigenVectors(II);
	dgMatrix massMatrix(dgGetIdentityMatrix());
	massMatrix[0][0] = II[0];
	massMatrix[1][1] = II[1];
	massMatrix[2][2] = II[2];
	dgBody::SetMassMatrix(mass, massMatrix);
}

dgMatrix dgDynamicBodyAsymetric::CalculateLocalInertiaMatrix() const
{
	dgMatrix matrix(m_principalAxis);
	matrix.m_posit = dgVector::m_wOne;
	dgMatrix diagonal(dgGetIdentityMatrix());
	diagonal[0][0] = m_mass[0];
	diagonal[1][1] = m_mass[1];
	diagonal[2][2] = m_mass[2];
	return matrix * diagonal * matrix.Inverse();
}

dgMatrix dgDynamicBodyAsymetric::CalculateInertiaMatrix() const
{
	dgMatrix matrix(m_principalAxis * m_matrix);
	matrix.m_posit = dgVector::m_wOne;
	dgMatrix diagonal(dgGetIdentityMatrix());
	diagonal[0][0] = m_mass[0];
	diagonal[1][1] = m_mass[1];
	diagonal[2][2] = m_mass[2];
	return matrix * diagonal * matrix.Inverse();
}

dgMatrix dgDynamicBodyAsymetric::CalculateInvInertiaMatrix() const
{
	dgMatrix matrix(m_principalAxis * m_matrix);
	matrix.m_posit = dgVector::m_wOne;
	dgMatrix diagonal(dgGetIdentityMatrix());
	diagonal[0][0] = m_invMass[0];
	diagonal[1][1] = m_invMass[1];
	diagonal[2][2] = m_invMass[2];
	return matrix * diagonal * matrix.Inverse();
}

void dgDynamicBodyAsymetric::IntegrateOpenLoopExternalForce(dgFloat32 timestep)
{
	dgDynamicBody::IntegrateOpenLoopExternalForce(timestep);
}


dgDynamicBody::dgDynamicBody()
	:dgBody()
	,m_externalForce(dgFloat32 (0.0f))
	,m_externalTorque(dgFloat32 (0.0f))
	,m_savedExternalForce(dgFloat32 (0.0f))
	,m_savedExternalTorque(dgFloat32 (0.0f))
	,m_dampCoef(dgFloat32 (0.0f))
	,m_cachedDampCoef(dgFloat32(0.0f))
	,m_cachedTimeStep(dgFloat32(0.0f))
	,m_sleepingCounter(0)
	,m_isInDestructionArrayLRU(0)
	,m_skeleton(NULL)
	,m_applyExtForces(NULL)
	,m_linearDampOn(true)
	,m_angularDampOn(true)
{
	m_type = m_dynamicBody;
	m_rtti |= m_dynamicBodyRTTI;
	dgAssert ( dgInt32 (sizeof (dgDynamicBody) & 0x0f) == 0);
}

dgDynamicBody::dgDynamicBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionCashe, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber)
	:dgBody(world, collisionCashe, serializeCallback, userData, revisionNumber)
	,m_externalForce(dgFloat32 (0.0f))
	,m_externalTorque(dgFloat32 (0.0f))
	,m_savedExternalForce(dgFloat32 (0.0f))
	,m_savedExternalTorque(dgFloat32 (0.0f))
	,m_dampCoef(dgFloat32 (0.0f))
	,m_cachedDampCoef(dgFloat32(0.0f))
	,m_cachedTimeStep(dgFloat32(0.0f))
	,m_sleepingCounter(0)
	,m_isInDestructionArrayLRU(0)
	,m_skeleton(NULL)
	,m_applyExtForces(NULL)
	,m_linearDampOn(true)
	,m_angularDampOn(true)
{
	dgInt32 val;
	m_type = m_dynamicBody;
	m_rtti |= m_dynamicBodyRTTI;

	m_invWorldInertiaMatrix[3][3] = dgFloat32 (1.0f);
	serializeCallback (userData, &m_mass, sizeof (m_mass));
	serializeCallback (userData, &m_invMass, sizeof (m_invMass));
	serializeCallback (userData, &m_dampCoef, sizeof (m_dampCoef));
	serializeCallback(userData, &val, sizeof (dgInt32));
	m_linearDampOn = (val & 1) ? true : false;
	m_angularDampOn = (val & 2) ? true : false;
}

dgDynamicBody::~dgDynamicBody()
{
	if (m_skeleton) {
		dgSkeletonContainer* const skel = m_skeleton;
		m_skeleton = NULL;
		m_world->DestroySkeletonContainer(skel);
	}
}

void dgDynamicBody::Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData)
{
	dgBody::Serialize (collisionRemapId, serializeCallback, userData);

	dgInt32 val = (m_linearDampOn ? 1 : 0) & (m_angularDampOn ? 2 : 0) ;
	serializeCallback (userData, &m_mass, sizeof (m_mass));
	serializeCallback (userData, &m_invMass, sizeof (m_invMass));
	serializeCallback (userData, &m_dampCoef, sizeof (m_dampCoef));
	serializeCallback (userData, &val, sizeof (dgInt32));
}


void dgDynamicBody::SetMatrixResetSleep(const dgMatrix& matrix)
{
	dgBody::SetMatrixResetSleep(matrix);
	m_savedExternalForce = dgVector (dgFloat32 (0.0f));
	m_savedExternalTorque = dgVector (dgFloat32 (0.0f));
	CalcInvInertiaMatrix();
}

void dgDynamicBody::SetMatrixNoSleep(const dgMatrix& matrix)
{
	dgBody::SetMatrixNoSleep(matrix);
	CalcInvInertiaMatrix();
}

void dgDynamicBody::AttachCollision (dgCollisionInstance* const collision)
{
	dgBody::AttachCollision(collision);
	if (m_collision->IsType(dgCollision::dgCollisionMesh_RTTI) || m_collision->IsType(dgCollision::dgCollisionScene_RTTI)) {
		//SetMassMatrix (m_mass.m_w, m_mass.m_x, m_mass.m_y, m_mass.m_z);
		SetMassMatrix (m_mass.m_w, CalculateLocalInertiaMatrix());
	}
}


dgVector dgDynamicBody::GetAlpha() const
{
	return m_alpha;
}

dgVector dgDynamicBody::GetAccel() const
{
	return m_accel;
}

void dgDynamicBody::SetAlpha(const dgVector& alpha)
{
	m_alpha = alpha;
}

void dgDynamicBody::SetAccel(const dgVector& accel)
{
	m_accel = accel;
}


bool dgDynamicBody::IsInEquilibrium() const
{
	if (m_equilibrium) {
		dgVector deltaAccel((m_externalForce - m_savedExternalForce).Scale(m_invMass.m_w));
		dgAssert(deltaAccel.m_w == 0.0f);
		dgFloat32 deltaAccel2 = deltaAccel.DotProduct(deltaAccel).GetScalar();
		if (deltaAccel2 > DG_ERR_TOLERANCE2) {
			return false;
		}
		dgVector deltaAlpha(m_matrix.UnrotateVector(m_externalTorque - m_savedExternalTorque) * m_invMass);
		dgAssert(deltaAlpha.m_w == 0.0f);
		dgFloat32 deltaAlpha2 = deltaAlpha.DotProduct(deltaAlpha).GetScalar();
		if (deltaAlpha2 > DG_ERR_TOLERANCE2) {
			return false;
		}
		return true;
	}
	return false;
}

void dgDynamicBody::ApplyExtenalForces (dgFloat32 timestep, dgInt32 threadIndex)
{
	m_externalForce = dgVector::m_zero;
	m_externalTorque = dgVector::m_zero;
	if (m_applyExtForces) {
		m_applyExtForces(*this, timestep, threadIndex);
	}

	
	m_gyroRotation = m_rotation;
	m_gyroTorque = dgVector::m_zero;

	m_externalForce += m_impulseForce;
	m_externalTorque += m_impulseTorque;
	m_impulseForce = dgVector::m_zero;
	m_impulseTorque = dgVector::m_zero;
}

void dgDynamicBody::AddDampingAcceleration(dgFloat32 timestep)
{
/*
	if (dgAbs(m_cachedTimeStep - timestep) > dgFloat32(1.0e-6f)) {
		m_cachedTimeStep = timestep;
		const dgFloat32 tau = dgFloat32(1.0f) / (dgFloat32(60.0f) * timestep);
		m_cachedDampCoef.m_x = dgPow(dgFloat32(1.0f) - m_dampCoef.m_x, tau);
		m_cachedDampCoef.m_y = dgPow(dgFloat32(1.0f) - m_dampCoef.m_y, tau);
		m_cachedDampCoef.m_z = dgPow(dgFloat32(1.0f) - m_dampCoef.m_z, tau);
		m_cachedDampCoef.m_w = dgPow(dgFloat32(1.0f) - m_dampCoef.m_w, tau);
	} 
*/
	dgVector damp (GetDampCoeffcient (timestep));
	if (m_linearDampOn) {
		m_veloc = m_veloc.Scale(damp.m_w);
	}

	if (m_angularDampOn) {
		dgVector omegaDamp(damp & dgVector::m_triplexMask);
		dgVector omega(m_matrix.UnrotateVector(m_omega) * omegaDamp);
		m_omega = m_matrix.RotateVector(omega);
	}
}


void dgDynamicBody::InvalidateCache ()
{
	m_sleepingCounter = 0;
	m_savedExternalForce = dgVector (dgFloat32 (0.0f));
	m_savedExternalTorque = dgVector (dgFloat32 (0.0f));
	dgBody::InvalidateCache ();
}

void dgDynamicBody::IntegrateImplicit(dgFloat32 timestep)
{
	// using simple backward Euler or implicit integration, this is. 
	// f'(t + dt) = (f(t + dt) - f(t)) / dt  

	// therefore: 
	// f(t + dt) = f(t) + f'(t + dt) * dt

	// approximate f'(t + dt) by expanding the Taylor of f(w + dt)
	// f(w + dt) = f(w) + f'(w) * dt + f''(w) * dt^2 / 2! + ....

	// assume dt^2 is negligible, therefore we can truncate the expansion to
	// f(w + dt) ~= f(w) + f'(w) * dt

	// calculating dw as the  f'(w) = d(wx, wy, wz) | dt
	// dw/dt = a = (Tl - (wl x (wl * Il)) * Il^1

	// expanding f(w) 
	// f'(wx) = Ix * ax = Tx - (Iz - Iy) * wy * wz 
	// f'(wy) = Iy * ay = Ty - (Ix - Iz) * wz * wx
	// f'(wz) = Iz * az = Tz - (Iy - Ix) * wx * wy
	//
	// calculation the expansion 
	// Ix * ax = (Tx - (Iz - Iy) * wy * wz) - ((Iz - Iy) * wy * az + (Iz - Iy) * ay * wz) * dt
	// Iy * ay = (Ty - (Ix - Iz) * wz * wx) - ((Ix - Iz) * wz * ax + (Ix - Iz) * az * wx) * dt
	// Iz * az = (Tz - (Iy - Ix) * wx * wy) - ((Iy - Ix) * wx * ay + (Iy - Ix) * ax * wy) * dt   
	//
	// factorizing a we get
	// Ix * ax + (Iz - Iy) * dwy * az + (Iz - Iy) * dwz * ay = Tx - (Iz - Iy) * wy * wz
	// Iy * ay + (Ix - Iz) * dwz * ax + (Ix - Iz) * dwx * az = Ty - (Ix - Iz) * wz * wx
	// Iz * az + (Iy - Ix) * dwx * ay + (Iy - Ix) * dwy * ax = Tz - (Iy - Ix) * wx * wy

	dgVector localOmega(m_matrix.UnrotateVector(m_omega));
	dgVector localTorque(m_matrix.UnrotateVector(m_externalTorque - m_gyroTorque));

	// and solving for alpha we get the angular acceleration at t + dt
	// calculate gradient at a full time step
	dgVector gradientStep(localTorque.Scale(timestep));

	// derivative at half time step. (similar to midpoint Euler so that it does not loses too much energy)
	dgVector dw(localOmega.Scale(dgFloat32(0.5f) * timestep));
	//dgVector dw(localOmega.Scale(dgFloat32(1.0f) * timestep));

	dgFloat32 jacobianMatrix[3][3];
	// calculates Jacobian matrix
	//dWx / dwx = Ix
	//dWx / dwy = (Iz - Iy) * wz * dt
	//dWx / dwz = (Iz - Iy) * wy * dt
	jacobianMatrix[0][0] = m_mass[0];
	jacobianMatrix[0][1] = (m_mass[2] - m_mass[1]) * dw[2];
	jacobianMatrix[0][2] = (m_mass[2] - m_mass[1]) * dw[1];

	//dWy / dwx = (Ix - Iz) * wz * dt
	//dWy / dwy = Iy				 
	//dWy / dwz = (Ix - Iz) * wx * dt
	jacobianMatrix[1][0] = (m_mass[0] - m_mass[2]) * dw[2];
	jacobianMatrix[1][1] = m_mass[1];
	jacobianMatrix[1][2] = (m_mass[0] - m_mass[2]) * dw[0];

	//dWz / dwx = (Iy - Ix) * wy * dt 
	//dWz / dwy = (Iy - Ix) * wx * dt 
	//dWz / dwz = Iz
	jacobianMatrix[2][0] = (m_mass[1] - m_mass[0]) * dw[1];
	jacobianMatrix[2][1] = (m_mass[1] - m_mass[0]) * dw[0];
	jacobianMatrix[2][2] = m_mass[2];

	// since the matrix is well behave, we can unroll Gauss elimination with back substitution 
	dgAssert(jacobianMatrix[0][0] > dgFloat32(0.0f));
	dgFloat32 den = dgFloat32(1.0f) / jacobianMatrix[0][0];
	dgFloat32 scale = jacobianMatrix[1][0] * den;
	jacobianMatrix[1][0] -= jacobianMatrix[0][0] * scale;
	jacobianMatrix[1][1] -= jacobianMatrix[0][1] * scale;
	jacobianMatrix[1][2] -= jacobianMatrix[0][2] * scale;
	gradientStep[1] -= gradientStep[0] * scale;

	scale = jacobianMatrix[2][0] * den;
	jacobianMatrix[2][0] -= jacobianMatrix[0][0] * scale;
	jacobianMatrix[2][1] -= jacobianMatrix[0][1] * scale;
	jacobianMatrix[2][2] -= jacobianMatrix[0][2] * scale;
	gradientStep[2] -= gradientStep[0] * scale;

	dgAssert(jacobianMatrix[1][1] > dgFloat32(0.0f));
	scale = jacobianMatrix[2][1] / jacobianMatrix[1][1];
	jacobianMatrix[2][1] -= jacobianMatrix[1][1] * scale;
	jacobianMatrix[2][2] -= jacobianMatrix[1][2] * scale;
	gradientStep[2] -= gradientStep[1] * scale;

	dgAssert(jacobianMatrix[2][2] > dgFloat32(0.0f));
	gradientStep[2] = gradientStep[2] / jacobianMatrix[2][2];
	gradientStep[1] = (gradientStep[1] - jacobianMatrix[1][2] * gradientStep[2]) / jacobianMatrix[1][1];
	gradientStep[0] = (gradientStep[0] - jacobianMatrix[0][1] * gradientStep[1] - jacobianMatrix[0][2] * gradientStep[2]) / jacobianMatrix[0][0];
	localOmega += gradientStep;

	m_accel = m_externalForce.Scale(m_invMass.m_w);
	m_alpha = m_matrix.RotateVector(localTorque * m_invMass);

	m_veloc += m_accel.Scale(timestep);
	m_omega = m_matrix.RotateVector(localOmega);
}

void dgDynamicBody::IntegrateExplicit(dgFloat32 timestep, dgInt32 method)
{
	// Simple forward Euler (not enough to cope with skew and high angular velocities generated by gyro effects)
	// f'(t) = (f(t + dt) - f(t)) / dt  

	// therefore: 
	// f(t + dt) = f(t) + f'(t) * dt
	dgVector externalTorque(m_externalTorque - m_gyroTorque);
	m_accel = m_externalForce.Scale(m_invMass.m_w);
	m_alpha = externalTorque * m_invMass;
	m_veloc += m_accel.Scale(timestep);

	dgTrace (("forward Euler %d: ", method));
	switch (method)
	{
		case 0:
		{
			// subdivide time step and keep everything constants during the sub steps.
			dgVector dt (timestep * dgFloat32(0.25f));
			for (dgInt32 i = 0; i < 4; i++) {
				dgVector toque(m_externalTorque - m_gyroTorque);
				dgVector alpha(m_matrix.RotateVector(m_invMass * m_matrix.UnrotateVector(toque)));
				m_omega += alpha * dt;
			}
			break;
		}

		case 1:
		{
			// subdivide time step and recalculate gyro toque after each sub step, 
			// but keeps everything else constants during the sub steps.
			dgVector dt (timestep * dgFloat32(0.25f));
			dgVector externTorque(m_matrix.UnrotateVector(m_externalTorque));
			dgVector localOmega(m_matrix.UnrotateVector(m_omega));
			for (dgInt32 i = 0; i < 4; i++) {
				dgVector gyroTorque(localOmega.CrossProduct(m_mass * localOmega));
				dgVector torque(externTorque - gyroTorque);
				dgVector alpha(torque * m_invMass);
				localOmega += alpha * dt;
			}
			m_omega = m_matrix.RotateVector(localOmega);
			break;
		}

		default:
		{
			// subdivide time step, recalculate gyro toque and rotation matrix, 
			// but still doing forward Euler steps.
			dgVector dt (timestep * dgFloat32(0.25f));
			dgVector externTorque(m_matrix.UnrotateVector(m_externalTorque));
			for (dgInt32 i = 0; i < 4; i++) {
				dgMatrix matrix(m_gyroRotation, dgVector::m_wOne);
				dgVector localOmega(matrix.UnrotateVector(m_omega));
				dgVector gyroTorque(localOmega.CrossProduct(m_mass * localOmega));
				dgVector torque(externTorque - gyroTorque);
				dgVector alpha(torque * m_invMass);
				localOmega += alpha * dt;
				m_omega = matrix.RotateVector(localOmega);

				// integrate rotation here
				dgAssert(m_veloc.m_w == dgFloat32(0.0f));
				dgAssert(m_omega.m_w == dgFloat32(0.0f));
				dgFloat32 omegaMag2 = m_omega.DotProduct(m_omega).GetScalar() + dgFloat32 (1.0e-12f);
				dgFloat32 invOmegaMag = dgRsqrt(omegaMag2);
				dgVector omegaAxis(m_omega.Scale(invOmegaMag));
				dgFloat32 omegaAngle = invOmegaMag * omegaMag2 * dt.GetScalar();
				dgQuaternion deltaRotation(omegaAxis, omegaAngle);
				m_gyroRotation = m_gyroRotation * deltaRotation;
				dgAssert((m_gyroRotation.DotProduct(m_gyroRotation) - dgFloat32(1.0f)) < dgFloat32(1.0e-5f));
			}
			break;
		}
	}
}

void dgDynamicBody::IntegrateOpenLoopExternalForce(dgFloat32 timestep)
{
	if (!m_equilibrium) {
		if (!m_collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
#if 0
			IntegrateExplicit(timestep, 0);
//			IntegrateExplicit(timestep, 1);
//			IntegrateExplicit(timestep, 2);
#else
			IntegrateImplicit(timestep);
#endif
		} else {
			dgCollisionLumpedMassParticles* const lumpedMassShape = (dgCollisionLumpedMassParticles*)m_collision->m_childShape;
			lumpedMassShape->IntegrateForces(timestep);
		}
	} else {
		m_accel = dgVector::m_zero;
		m_alpha = dgVector::m_zero;
	}
}


