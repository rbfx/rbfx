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


#ifndef __DGCOLLISION_LUMPED_MASS_PARTICLES_H__
#define __DGCOLLISION_LUMPED_MASS_PARTICLES_H__


#include "dgCollision.h"
#include "dgCollisionConvex.h"


class dgCollisionLumpedMassParticles: public dgCollisionConvex
{
	public:
	dgCollisionLumpedMassParticles (const dgCollisionLumpedMassParticles& source);
	dgCollisionLumpedMassParticles (dgWorld* const world, dgCollisionID collisionID);
	dgCollisionLumpedMassParticles (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber);
	virtual ~dgCollisionLumpedMassParticles(void);

	dgInt32 GetCount() const;
	dgInt32 GetStrideInByte() const;
	const dgVector* GetVelocity() const;
	const dgVector* GetPositions() const;
	const dgVector* GetAcceleration() const;

	dgDynamicBody* GetOwner () const;
	void SetOwnerAndMassPraperties (dgDynamicBody* const body);
	virtual void IntegrateForces (dgFloat32 timestep) = 0;

	protected:
	virtual void FinalizeBuild();
	virtual dgInt32 CalculateSignature() const;
	virtual void RegisterCollision(const dgBody* const otherBody);
	virtual void SetCollisionBBox(const dgVector& p0, const dgVector& p1);
	virtual void Serialize(dgSerialize callback, void* const userData) const;
	virtual void CalcAABB(const dgMatrix& matrix, dgVector& p0, dgVector& p1) const;
	virtual dgMatrix CalculateInertiaAndCenterOfMass(const dgMatrix& m_alignMatrix, const dgVector& localScale, const dgMatrix& matrix) const;

	virtual void DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;
	dgFloat32 RayCast(const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const;

	//dgFloat32 CalculaleContactPenetration(const dgVector& point, const dgVector& normal) const;
	dgVector CalculateContactNormalAndPenetration(const dgVector& worldPosition) const;
	virtual void HandleCollision (dgFloat32 timestep, dgVector* const normalDir, dgVector* const normalAccel, dgFloat32* const frictionCoefficient);

	virtual dgInt32 GetMemoryBufferSizeInBytes() const = 0;

	dgArray<dgVector> m_posit;
	dgArray<dgVector> m_veloc;
	dgArray<dgVector> m_accel;
	dgArray<dgVector> m_externalAccel;
	dgArray<dgFloat32> m_mass;
	dgArray<dgFloat32> m_invMass;
	dgDynamicBody* m_body;
	dgFloat32 m_totalMass;
	dgFloat32 m_particleRadius;
	dgInt32 m_particlesCount;

	friend class dgBroadPhase;
	friend class dgDynamicBody;
	friend class dgWorldDynamicUpdate;
};

inline dgDynamicBody* dgCollisionLumpedMassParticles::GetOwner () const
{
	return m_body;
}
#endif 

