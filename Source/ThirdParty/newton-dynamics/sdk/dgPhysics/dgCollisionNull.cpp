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
#include "dgContact.h"
#include "dgCollisionNull.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


dgCollisionNull::dgCollisionNull(dgMemoryAllocator* const allocator, dgUnsigned32 signature)
	:dgCollisionConvex(allocator, signature, m_nullCollision) 
{
	m_rtti |= dgCollisionNull_RTTI;
	m_inertia = dgVector (dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f));
}

dgCollisionNull::dgCollisionNull(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	m_rtti |= dgCollisionNull_RTTI;
}

void dgCollisionNull::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);
}

dgCollisionNull::~dgCollisionNull()
{
}

void dgCollisionNull::SetCollisionBBox (const dgVector& p0, const dgVector& p1)
{
	dgAssert (0);
}

void dgCollisionNull::DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
}

dgInt32 dgCollisionNull::CalculateSignature () const
{
	return dgInt32 (GetSignature());
}

void dgCollisionNull::CalcAABB (const dgMatrix& matrix, dgVector &p0, dgVector &p1) const
{
	p0 = matrix[3] & dgVector::m_triplexMask;
	p1 = matrix[3] & dgVector::m_triplexMask;
}

dgVector dgCollisionNull::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (0);
	return dgVector::m_zero;
}

dgVector dgCollisionNull::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert(0);
	return dgVector::m_zero;
}

dgFloat32 dgCollisionNull::GetVolume () const
{
	return dgFloat32 (0.0f);
}

dgFloat32 dgCollisionNull::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	return dgFloat32 (1.2f);
}


dgVector dgCollisionNull::CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& plane, const dgCollisionInstance& parentScale) const
{
	dgAssert (0);
	return dgVector::m_zero;
}

