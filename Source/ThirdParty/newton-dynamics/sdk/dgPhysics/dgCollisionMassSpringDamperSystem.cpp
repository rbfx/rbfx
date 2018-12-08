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
#include "dgContact.h"
#include "dgMeshEffect.h"
#include "dgDynamicBody.h"
#include "dgCollisionMassSpringDamperSystem.h"


dgCollisionMassSpringDamperSystem::dgCollisionMassSpringDamperSystem (dgWorld* const world, dgInt32 shapeID, dgInt32 pointCount, const dgFloat32* const points, dgInt32 strideInBytes, const dgFloat32* const pointsMasses, dgInt32 linksCount, const dgInt32* const links, const dgFloat32* const linksSpring, const dgFloat32* const LinksDamper)
	:dgCollisionDeformableMesh(world, m_deformableSolidMesh)
{
	m_rtti |= dgCollisionMassSpringDamperSystem_RTTI;

	m_particlesCount = pointCount;
	m_posit.Resize(m_particlesCount);
	m_mass.Resize(m_particlesCount);
	m_invMass.Resize(m_particlesCount);
	const dgInt32 stride = strideInBytes / sizeof (dgFloat32);
	m_totalMass = dgFloat32(0.0f);	
	for (dgInt32 i = 0; i < pointCount; i++) {
		m_totalMass += m_totalMass;
		m_mass[i] = pointsMasses[i];
		m_invMass[i] = dgFloat32(1.0f / pointsMasses[i]);
		m_posit[i] = dgVector(points[i * stride + 0], points[i * stride + 1], points[i * stride + 2], dgFloat32(0.0f));
	}

	m_linksCount = linksCount;
	m_linkList.Resize(linksCount);
	for (dgInt32 i = 0; i < linksCount; i++) {
		dgInt32 v0 = links[i * 2 + 0];
		dgInt32 v1 = links[i * 2 + 1];
		dgAssert(v0 != v1);
		dgAssert(v0 >= 0);
		dgAssert(v1 >= 0);
		dgAssert(v0 < pointCount);
		dgAssert(v1 < pointCount);
		m_linkList[i].m_m0 = dgInt16(dgMin(v0, v1));
		m_linkList[i].m_m1 = dgInt16(dgMax(v0, v1));
		m_linkList[i].m_spring = linksSpring[i];
		m_linkList[i].m_damper = LinksDamper[i];

		const dgVector& p0 = m_posit[v0];
		const dgVector& p1 = m_posit[v1];
		dgVector dp(p0 - p1);
		dgAssert (dp.m_w == dgFloat32 (0.0f));
		m_linkList[i].m_restlength = dgSqrt(dp.DotProduct(dp).GetScalar());
		dgAssert(m_linkList[i].m_restlength > dgFloat32(1.0e-2f));
	}

	FinalizeBuild();
}


dgCollisionMassSpringDamperSystem::dgCollisionMassSpringDamperSystem(const dgCollisionMassSpringDamperSystem& source)
	:dgCollisionDeformableMesh(source)
{
	m_rtti |= source.m_rtti;
}

dgCollisionMassSpringDamperSystem::dgCollisionMassSpringDamperSystem(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionDeformableMesh(world, deserialization, userData, revisionNumber)
{
}

dgCollisionMassSpringDamperSystem::~dgCollisionMassSpringDamperSystem(void)
{
}

dgInt32 dgCollisionMassSpringDamperSystem::GetMemoryBufferSizeInBytes() const
{
	dgInt32 sizeInByte = 0;
	sizeInByte += 3 * m_particlesCount * sizeof (dgVector);
	sizeInByte += 1 * m_particlesCount * sizeof (dgFloat32);
	return sizeInByte;
}

#if 0
void dgCollisionMassSpringDamperSystem::CalculateAcceleration(dgFloat32 timestep)
{
	// Ks is in [sec^-2] a spring constant unit acceleration, not a spring force acceleration. 
	// Kc is in [sec^-1] a damper constant unit velocity, not a damper force acceleration. 

	dgInt32 iter = 4;
	dgVector* const accel = &m_accel[0];
	dgVector* const veloc = &m_veloc[0];
	dgVector* const posit = &m_posit[0];
	dgVector* const extAccel = &m_externalAccel[0];
	const dgSpringDamperLink* const links = &m_linkList[0];

	dgVector* const dx = dgAlloca(dgVector, m_linksCount);
	dgVector* const dv = dgAlloca(dgVector, m_linksCount);
	dgVector* const dpdv = dgAlloca(dgVector, m_linksCount);

	dgVector* const normalDir = dgAlloca(dgVector, m_particlesCount);
	dgVector* const normalAccel = dgAlloca(dgVector, m_particlesCount);
	dgFloat32* const spring_A01 = dgAlloca(dgFloat32, m_linksCount);
	dgFloat32* const spring_B01 = dgAlloca(dgFloat32, m_linksCount);
	dgFloat32* const frictionCoeffecient = dgAlloca(dgFloat32, m_particlesCount);

	//	dgFloat32* const damper_A01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgFloat32* const damper_B01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgFloat32* const damper_C01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgVector* const collisionDir1 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const collisionDir2 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const tmp1 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const tmp2 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const diag = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const offDiag = dgAlloca(dgVector, m_particlesCount);

	dgVector unitAccel(m_body->m_externalForce * Scale (m_body->m_invMass.m_w));
	dgVector deltaOmega(m_body->m_invWorldInertiaMatrix.RotateVector(m_body->m_externalTorque.Scale(timestep)));

	m_body->m_alpha = dgVector::m_zero;
	m_body->m_omega = dgVector::m_zero;
	m_body->m_externalForce = dgVector::m_zero;
	m_body->m_externalTorque = dgVector::m_zero;

	// here I need to add all other external acceleration like wind and pressure, friction and collision.
	for (dgInt32 i = 0; i < m_particlesCount; i++) {
		extAccel[i] = unitAccel;
	}

	//dgFloat32 ks_dt = -timestep * kSpring;
	//dgFloat32 kd_dt0 = -timestep * kDamper;
	//dgFloat32 kd_dt1 = -timestep * kDamper * dgFloat32(2.0f);

	dgVector dtRK4(timestep / iter);
	dgVector epsilon(dgFloat32(1.0e-14f));

	dtRK4 = dtRK4 & dgVector::m_triplexMask;
	HandleCollision(timestep, normalDir, normalAccel, frictionCoeffecient);
	for (dgInt32 k = 0; k < iter; k++) {

		for (dgInt32 i = 0; i < m_particlesCount; i++) {
			accel[i] = dgVector::m_zero; 
		}

		for (dgInt32 i = 0; i < m_linksCount; i++) {
			const dgInt32 j0 = links[i].m_m0;
			const dgInt32 j1 = links[i].m_m1;
			dv[i] = veloc[j0] - veloc[j1];
			dx[i] = posit[j0] - posit[j1];

			const dgVector p0p1(dx[i]);
			const dgVector v0v1(dv[i]);
			const dgVector length2(p0p1.DotProduct(p0p1));
			const dgVector mask(length2 > m_smallestLenght2);

			const dgVector lenght2((length2 & mask) | length2.AndNot(mask));
			const dgFloat32 length = (lenght2.Sqrt()).GetScalar();
			const dgFloat32 den = dgFloat32(1.0f) / length;
			const dgFloat32 lenghtRatio = links[i].m_restlength * den;
			const dgFloat32 compression = dgFloat32(1.0f) - lenghtRatio;
			const dgVector fs(p0p1.Scale(links[i].m_spring * compression));
			const dgVector fd(p0p1.Scale(links[i].m_damper * den * den * (v0v1.DotProduct(p0p1)).GetScalar()));

			dgAssert(fs.m_w == dgFloat32(0.0f));
			dgAssert(fs.m_w == dgFloat32(0.0f));
			dgAssert(p0p1.m_w == dgFloat32(0.0f));

			dpdv[i] = p0p1 * v0v1;
			accel[j0] -= (fs + fd);
			accel[j1] += (fs + fd);

			dgFloat32 ks_dt = -dtRK4.GetScalar() * links[i].m_spring;
			spring_A01[i] = ks_dt * compression;
			spring_B01[i] = ks_dt * lenghtRatio * den * den;
		}

		for (dgInt32 i = 0; i < m_linksCount; i++) {
			const dgVector dv0(dv[i]);
			const dgVector A01(spring_A01[i]);
			const dgVector B01(spring_B01[i]);
			const dgVector dfdx(A01 * dv0 + B01 * dx[i] * dpdv[i]);

			const dgInt32 j0 = links[i].m_m0;
			const dgInt32 j1 = links[i].m_m1;

			dgAssert(dfdx.m_w == dgFloat32(0.0f));
			accel[j0] += dfdx;
			accel[j1] -= dfdx;
		}

		for (dgInt32 i = 0; i < m_particlesCount; i++) {
			//dgVector dirAccel1 (collisionDir1[i] * accel[i].DotProduct(collisionDir1[i]));
			//dgVector dirAccel2 (collisionDir2[i] * accel[i].DotProduct(collisionDir2[i]));
			//tmp0[i] = accel[i] + collidingAccel[i] - dirAccel0 - dirAccel1 - dirAccel2;
			
			dgVector netAccel (accel[i] + extAccel[i]);
			dgVector tangentDir(veloc[i] - normalDir[i] * normalDir[i].DotProduct(veloc[i]));
			dgVector mag(tangentDir.DotProduct(tangentDir) + epsilon);
			
			dgFloat32 tangentFrictionAccel = dgAbs(netAccel.DotProduct(normalDir[i]).GetScalar());
			dgVector friction(tangentDir.Scale(frictionCoeffecient[i] * tangentFrictionAccel / dgSqrt(mag.GetScalar())));

			//dgVector particleAccel (accel[i] + normalAccel[i] - normalDirAccel);
			dgVector normalDirAccel(normalDir[i] * netAccel.DotProduct(normalDir[i]));
			//accel[i] = accel[i] + normalAccel[i] - normalDirAccel - friction;
			netAccel = netAccel + normalAccel[i] - normalDirAccel - friction;
			veloc[i] += netAccel * dtRK4;
			posit[i] += veloc[i] * dtRK4;
		}
	}
}


#else
void dgCollisionMassSpringDamperSystem::CalculateAcceleration(dgFloat32 timestep)
{
	// Ks is in [sec^-2] a spring constant unit acceleration, not a spring force acceleration. 
	// Kc is in [sec^-1] a damper constant unit velocity, not a damper force acceleration. 

	dgInt32 iter = 4;
	dgVector* const accel = &m_accel[0];
	dgVector* const veloc = &m_veloc[0];
	dgVector* const posit = &m_posit[0];
	dgVector* const extAccel = &m_externalAccel[0];
	const dgSpringDamperLink* const links = &m_linkList[0];
/*
	
	dgVector* const dx = dgAlloca(dgVector, m_linksCount);
	dgVector* const dv = dgAlloca(dgVector, m_linksCount);
	dgVector* const dpdv = dgAlloca(dgVector, m_linksCount);

	dgVector* const normalDir = dgAlloca(dgVector, m_particlesCount);
	dgVector* const normalAccel = dgAlloca(dgVector, m_particlesCount);
	dgFloat32* const spring_A01 = dgAlloca(dgFloat32, m_linksCount);
	dgFloat32* const spring_B01 = dgAlloca(dgFloat32, m_linksCount);
	dgFloat32* const frictionCoeffecient = dgAlloca(dgFloat32, m_particlesCount);
*/
	//	dgFloat32* const damper_A01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgFloat32* const damper_B01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgFloat32* const damper_C01 = dgAlloca(dgFloat32, m_linksCount);
	//	dgVector* const collisionDir1 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const collisionDir2 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const tmp1 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const tmp2 = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const diag = dgAlloca(dgVector, m_particlesCount);
	//	dgVector* const offDiag = dgAlloca(dgVector, m_particlesCount);
	//dgVector deltaOmega(m_body->m_invWorldInertiaMatrix.RotateVector(m_body->m_externalTorque.Scale(timestep)));

	dgWorld* const world = m_body->GetWorld();
	world->m_solverJacobiansMemory.ResizeIfNecessary(GetMemoryBufferSizeInBytes() + 1024);

	dgVector* const normalAccel = (dgVector*)&world->m_solverJacobiansMemory[0];
	dgVector* const normalDir = &normalAccel[m_particlesCount];
	dgVector* const diagonal = &normalDir[m_particlesCount];
	dgFloat32* const frictionCoeffecient = (dgFloat32*)&diagonal[m_particlesCount];

	dgVector unitAccel(m_body->m_externalForce * dgVector(m_body->m_invMass.m_w));

	// here I need to add all other external acceleration like wind and pressure, friction and collision.
	for (dgInt32 i = 0; i < m_particlesCount; i++) {
		extAccel[i] = unitAccel;
	}

	dgAssert(m_body->IsRTTIType(dgBody::m_dynamicBodyRTTI));
	m_body->m_alpha = dgVector::m_zero;
	m_body->m_omega = dgVector::m_zero;
	m_body->m_externalForce = dgVector::m_zero;
	m_body->m_externalTorque = dgVector::m_zero;

	//dgFloat32 ks_dt = -timestep * kSpring;
	//dgFloat32 kd_dt0 = -timestep * kDamper;
	//dgFloat32 kd_dt1 = -timestep * kDamper * dgFloat32(2.0f);

	//dgFloat32 dtRK4__ = timestep / iter;
	dgVector dtRK4 (timestep / iter);
	dgVector epsilon(dgFloat32(1.0e-14f));

//	dtRK4 = dtRK4 & dgVector::m_triplexMask;
	HandleCollision(timestep, normalDir, normalAccel, frictionCoeffecient);
	for (dgInt32 k = 0; k < iter; k++) {
		for (dgInt32 i = 0; i < m_particlesCount; i++) {
			accel[i] = dgVector::m_zero; 
			diagonal[i] = dgVector::m_zero; 
		}
/*
		for (dgInt32 i = 0; i < m_linksCount; i++) {
			const dgInt32 j0 = links[i].m_m0;
			const dgInt32 j1 = links[i].m_m1;

			const dgVector p0p1(posit[j0] - posit[j1]);
			const dgVector v0v1(veloc[j0] - veloc[j1]);
			const dgVector dpdv(p0p1 * v0v1);

			const dgVector mag2(p0p1.DotProduct(p0p1));
			const dgVector mask(mag2 > m_smallestLenght2);

			const dgVector lenght2((mag2 & mask) | mag2.AndNot(mask));
			const dgFloat32 length = (lenght2.Sqrt()).GetScalar();
			const dgFloat32 invDen = dgFloat32(1.0f) / length;
			const dgFloat32 lenghtRatio = restLenght[i] * invDen;
			const dgFloat32 compression = dgFloat32(1.0f) - lenghtRatio;
			const dgVector fs(p0p1.Scale(kSpring * compression));
			const dgVector fd(p0p1.Scale(kDamper * invDen * invDen * (v0v1.DotProduct(p0p1)).GetScalar()));
			const dgVector dfsdx(v0v1.Scale (ks_dt * compression) + ((p0p1 * dpdv).Scale (ks_dt * lenghtRatio * invDen * invDen)));
			dgAssert(fs.m_w == dgFloat32(0.0f));
			dgAssert(fs.m_w == dgFloat32(0.0f));
			dgAssert(p0p1.m_w == dgFloat32(0.0f));
			dgAssert(dfsdx.m_w == dgFloat32(0.0f));

			const dgVector nextAccel(fs + fd - dfsdx);
			accel[j0] -= nextAccel;
			accel[j1] += nextAccel;
		}
*/

		for (dgInt32 i = 0; i < m_linksCount; i++) {
			const dgInt32 j0 = links[i].m_m0;
			const dgInt32 j1 = links[i].m_m1;
			const dgVector p0p1(posit[j0] - posit[j1]);
			const dgVector v0v1(veloc[j0] - veloc[j1]);
			const dgVector dvdp(v0v1 * p0p1);

			const dgVector p0p1Mag2(p0p1.DotProduct(p0p1));
			const dgVector p0p1Mask(p0p1Mag2 > m_smallestLenght2);
			const dgVector p0p1Lenght2((p0p1Mag2 & p0p1Mask) | m_smallestLenght2.AndNot(p0p1Mask));

			const dgFloat32 p0p1Length = p0p1Lenght2.Sqrt().GetScalar();
			const dgFloat32 p0p1InvMag = dgFloat32 (1.0f) / p0p1Length;

			const dgFloat32 k01 = -links[i].m_spring * (p0p1Length - links[i].m_restlength) * p0p1InvMag;
			const dgFloat32 d01 = -links[i].m_damper * dvdp.GetScalar() * p0p1InvMag * p0p1InvMag;
			const dgFloat32 h01dt = - dtRK4.GetScalar() * links[i].m_spring * links[i].m_restlength * p0p1InvMag * p0p1InvMag * p0p1InvMag;

			const dgVector diag ((p0p1 * p0p1).Scale(h01dt * dtRK4.GetScalar()));

			const dgFloat32 dtdfp0dx0 = h01dt * v0v1.DotProduct(p0p1).GetScalar();
			const dgVector netForce (p0p1.Scale(k01 + d01 + dtdfp0dx0));
			

			diagonal[j0] -= diag;
			diagonal[j1] -= diag;
			accel[j0] += netForce;
			accel[j1] -= netForce;
		}


		for (dgInt32 i = 0; i < m_particlesCount; i++) {
			//dgVector dirAccel1 (collisionDir1[i] * accel[i].DotProduct(collisionDir1[i]));
			//dgVector dirAccel2 (collisionDir2[i] * accel[i].DotProduct(collisionDir2[i]));
			//tmp0[i] = accel[i] + collidingAccel[i] - dirAccel0 - dirAccel1 - dirAccel2;

			dgVector netAccel (accel[i] + extAccel[i]);
			dgVector tangentDir(veloc[i] - normalDir[i] * (normalDir[i].DotProduct(veloc[i])));
			dgVector mag(tangentDir.DotProduct(tangentDir) + epsilon);
			
			dgFloat32 tangentFrictionAccel = dgAbs(netAccel.DotProduct(normalDir[i]).GetScalar());
			dgVector friction(tangentDir.Scale(frictionCoeffecient[i] * tangentFrictionAccel / dgSqrt(mag.GetScalar())));

			//dgVector particleAccel (accel[i] + normalAccel[i] - normalDirAccel);
			dgVector normalDirAccel(normalDir[i] * (netAccel.DotProduct(normalDir[i])));
			//accel[i] = accel[i] + normalAccel[i] - normalDirAccel - friction;
			netAccel = netAccel + normalAccel[i] - normalDirAccel - friction;
			veloc[i] += netAccel * dtRK4;
			posit[i] += veloc[i] * dtRK4;
		}
	}
}

#endif