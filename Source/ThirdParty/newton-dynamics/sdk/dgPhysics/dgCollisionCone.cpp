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
#include "dgCollisionCone.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgInt32 dgCollisionCone::m_shapeRefCount = 0;
dgCollisionConvex::dgConvexSimplexEdge dgCollisionCone::m_edgeArray[DG_CONE_SEGMENTS * 4];

dgCollisionCone::dgCollisionCone(dgMemoryAllocator* allocator, dgUnsigned32 signature, dgFloat32 radius, dgFloat32 height)
	:dgCollisionConvex(allocator, signature, m_coneCollision)
{
	Init (radius, height);
}

dgCollisionCone::dgCollisionCone(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	dgVector size;
	deserialization (userData, &size, sizeof (dgVector));
	Init (size.m_x, size.m_y);
}


dgCollisionCone::~dgCollisionCone()
{
	m_shapeRefCount --;
	dgAssert (m_shapeRefCount >= 0);

	dgCollisionConvex::m_simplex = NULL;
	dgCollisionConvex::m_vertex = NULL;
}

void dgCollisionCone::Init (dgFloat32 radius, dgFloat32 height)
{
	m_rtti |= dgCollisionCone_RTTI;

	m_radius = dgMax(dgAbs(radius), D_MIN_CONVEX_SHAPE_SIZE);
	m_height = dgMax (dgAbs (height * dgFloat32 (0.5f)), D_MIN_CONVEX_SHAPE_SIZE);

	dgFloat32 angle = dgFloat32(0.0f);
	for (dgInt32 i = 0; i < DG_CONE_SEGMENTS; i++) {
		dgFloat32 sinAngle = dgSin(angle);
		dgFloat32 cosAngle = dgCos(angle);
		m_vertex[i] = dgVector(-m_height, m_radius * cosAngle, m_radius * sinAngle, dgFloat32(0.0f));
		angle += dgPI2 / DG_CONE_SEGMENTS;
	}
	m_vertex[DG_CONE_SEGMENTS] = dgVector (m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));

	m_edgeCount = DG_CONE_SEGMENTS * 4;
	m_vertexCount = DG_CONE_SEGMENTS + 1;
	dgCollisionConvex::m_vertex = m_vertex;

	if (!m_shapeRefCount) {
		dgPolyhedra polyhedra(m_allocator);
		dgInt32 wireframe[DG_CONE_SEGMENTS];

		dgInt32 j = DG_CONE_SEGMENTS - 1;
		polyhedra.BeginFace ();
		for (dgInt32 i = 0; i < DG_CONE_SEGMENTS; i ++) { 
			wireframe[0] = j;
			wireframe[1] = i;
			wireframe[2] = DG_CONE_SEGMENTS;
			j = i;
			polyhedra.AddFace (3, wireframe);
		}

		for (dgInt32 i = 0; i < DG_CONE_SEGMENTS; i ++) { 
			wireframe[i] = DG_CONE_SEGMENTS - 1 - i;
		}
		polyhedra.AddFace (DG_CONE_SEGMENTS, wireframe);
		polyhedra.EndFace ();

		dgAssert (SanityCheck (polyhedra));

		dgUnsigned64 i = 0;
		dgPolyhedra::Iterator iter (polyhedra);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			edge->m_userData = i;
			i ++;
		}

		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);

			dgConvexSimplexEdge* const ptr = &m_edgeArray[edge->m_userData];

			ptr->m_vertex = edge->m_incidentVertex;
			ptr->m_next = &m_edgeArray[edge->m_next->m_userData];
			ptr->m_prev = &m_edgeArray[edge->m_prev->m_userData];
			ptr->m_twin = &m_edgeArray[edge->m_twin->m_userData];
		}
	}

	m_profile[0] = dgVector(m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	m_profile[1] = dgVector(-m_height,  m_radius, dgFloat32(0.0f), dgFloat32(0.0f));
	m_profile[2] = dgVector(-m_height, -m_radius, dgFloat32(0.0f), dgFloat32(0.0f));

	m_shapeRefCount ++;
	dgCollisionConvex::m_simplex = m_edgeArray;

	SetVolumeAndCG ();
}

dgInt32 dgCollisionCone::CalculateSignature (dgFloat32 radius, dgFloat32 height)
{
	dgUnsigned32 buffer[3];

	memset (buffer, 0, sizeof (buffer));
	buffer[0] = m_coneCollision;
	buffer[1] = dgCollision::Quantize (radius);
	buffer[2] = dgCollision::Quantize (height);
	return dgInt32 (dgCollision::Quantize(buffer, sizeof (buffer)));
}

dgInt32 dgCollisionCone::CalculateSignature () const
{
	return CalculateSignature (m_radius, m_height);
}


void dgCollisionCone::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	#define NUMBER_OF_DEBUG_SEGMENTS  40
	dgTriplex pool[NUMBER_OF_DEBUG_SEGMENTS + 1];
	dgTriplex face[NUMBER_OF_DEBUG_SEGMENTS];

	dgFloat32 angle = dgFloat32 (0.0f);
	for (dgInt32 i = 0; i < NUMBER_OF_DEBUG_SEGMENTS; i ++) {
		dgFloat32 z = dgSin (angle) * m_radius;
		dgFloat32 y = dgCos (angle) * m_radius;
		pool[i].m_x = -m_height;
		pool[i].m_y = y;
		pool[i].m_z = z;
		angle += dgPI2 / dgFloat32 (NUMBER_OF_DEBUG_SEGMENTS);
	}
	
	pool[NUMBER_OF_DEBUG_SEGMENTS].m_x = m_height;
	pool[NUMBER_OF_DEBUG_SEGMENTS].m_y = dgFloat32 (0.0f);
	pool[NUMBER_OF_DEBUG_SEGMENTS].m_z = dgFloat32 (0.0f);

	matrix.TransformTriplex (&pool[0].m_x, sizeof (dgTriplex), &pool[0].m_x, sizeof (dgTriplex), NUMBER_OF_DEBUG_SEGMENTS + 1);
	dgInt32 j = NUMBER_OF_DEBUG_SEGMENTS - 1;
	for (dgInt32 i = 0; i < NUMBER_OF_DEBUG_SEGMENTS; i ++) { 
		face[0] = pool[j];
		face[1] = pool[i];
		face[2] = pool[NUMBER_OF_DEBUG_SEGMENTS];
		j = i;
		callback (userData, 3, &face[0].m_x, 0);
	}

	for (dgInt32 i = 0; i < NUMBER_OF_DEBUG_SEGMENTS; i ++) { 
		face[i] = pool[NUMBER_OF_DEBUG_SEGMENTS - 1 - i];
	}
	callback (userData, NUMBER_OF_DEBUG_SEGMENTS, &face[0].m_x, 0);
}

void dgCollisionCone::SetCollisionBBox (const dgVector& p0__, const dgVector& p1__)
{
	dgAssert (0);
}


dgVector dgCollisionCone::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));

	if (dir.m_x < dgFloat32(-0.9999f)) {
		return dgVector(-m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else if (dir.m_x > dgFloat32(0.9999f)) {
		return dgVector(m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else {
		dgVector dir_yz(dir);
		dir_yz.m_x = dgFloat32(0.0f);
		dgFloat32 mag2 = dir_yz.DotProduct(dir_yz).GetScalar();
		dgAssert(mag2 > dgFloat32(0.0f));
		dir_yz = dir_yz.Scale(dgFloat32(1.0f) / dgSqrt(mag2));

		dgVector p0(dir_yz.Scale(m_radius));
		dgVector p1(dgVector::m_zero);

		p0.m_x = -m_height;
		p1.m_x =  m_height;

		dgFloat32 dist0 = dir.DotProduct(p0).GetScalar();
		dgFloat32 dist1 = dir.DotProduct(p1).GetScalar();

		if (dist1 >= dist0) {
			p0 = p1;
		}
		return p0;
	}
}

dgVector dgCollisionCone::SupportVertexSpecial(const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));

	if (dir.m_x < dgFloat32(-0.9999f)) {
		return dgVector(-(m_height - DG_PENETRATION_TOL), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else if (dir.m_x > dgFloat32(0.9999f)) {
		return dgVector(m_height - DG_PENETRATION_TOL, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else {
		dgVector dir_yz(dir);
		dir_yz.m_x = dgFloat32(0.0f);
		dgFloat32 mag2 = dir_yz.DotProduct(dir_yz).GetScalar();
		dgAssert(mag2 > dgFloat32(0.0f));
		dir_yz = dir_yz.Scale(dgFloat32(1.0f) / dgSqrt(mag2));

		dgVector p0(dir_yz.Scale(m_radius - DG_PENETRATION_TOL));
		dgVector p1(dgVector::m_zero);

		p0.m_x = -(m_height - DG_PENETRATION_TOL);
		p1.m_x =  m_height - DG_PENETRATION_TOL;

		dgFloat32 dist0 = dir.DotProduct(p0).GetScalar();
		dgFloat32 dist1 = dir.DotProduct(p1).GetScalar();

		if (dist1 >= dist0) {
			p0 = p1;
		}
		return p0;
	}
}

dgVector dgCollisionCone::SupportVertexSpecialProjectPoint(const dgVector& point, const dgVector& dir) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));
	return point + dir.Scale(DG_PENETRATION_TOL);
}

void dgCollisionCone::MassProperties ()
{
	m_centerOfMass = dgVector (-dgFloat32 (1.0f/2.0f) * m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	m_crossInertia = dgVector (dgFloat32 (0.0f));

	dgFloat32 volume = dgFloat32 (dgPI * 2.0f / 3.0f) * m_radius * m_radius * m_height;
	dgFloat32 inertiaxx = dgFloat32 (3.0f / 10.0f) * m_radius * m_radius;
	dgFloat32 inertiayyzz = (dgFloat32 (3.0f / 20.0f) * m_radius * m_radius + dgFloat32 (3.0f / 20.0f) * m_height * m_height);

//	dgCollisionConvex::MassProperties ();
	m_inertia[0] = inertiaxx;
	m_inertia[1] = inertiayyzz;
	m_inertia[2] = inertiayyzz;
	m_centerOfMass.m_w = volume;
}


dgInt32 dgCollisionCone::CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut) const
{
	dgInt32 count = 0;
	if (normal.m_x > dgFloat32(0.99f)) {
		contactsOut[0] = dgVector(m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
		count = 1;
	} else if (normal.m_x < dgFloat32(-0.995f)) {
		const dgFloat32 inclination = dgFloat32(0.9998f);
		if (normal.m_x < -inclination) {
			dgMatrix matrix(normal);
			matrix.m_posit.m_x = origin.m_x;
			dgVector scale(m_radius);
			const int n = sizeof (m_unitCircle) / sizeof (m_unitCircle[0]);
			for (dgInt32 i = 0; i < n; i++) {
				contactsOut[i] = matrix.TransformVector(m_unitCircle[i] * scale) & dgVector::m_triplexMask;
			}
			count = RectifyConvexSlice(n, normal, contactsOut);
		} else {
			dgFloat32 magInv = dgRsqrt(normal.m_y * normal.m_y + normal.m_z * normal.m_z);
			dgFloat32 cosAng = normal.m_y * magInv;
			dgFloat32 sinAng = normal.m_z * magInv;

			dgAssert(dgAbs(normal.m_z * cosAng - normal.m_y * sinAng) < dgFloat32(1.0e-4f));
			dgVector normal1(normal.m_x, normal.m_y * cosAng + normal.m_z * sinAng, dgFloat32(0.0f), dgFloat32(0.0f));
			dgVector origin1(origin.m_x, origin.m_y * cosAng + origin.m_z * sinAng, origin.m_z * cosAng - origin.m_y * sinAng, dgFloat32(0.0f));

			count = dgCollisionConvex::CalculatePlaneIntersection(normal1, origin1, contactsOut);
			if (count > 6) {
				dgInt32 dy = 2 * 6;
				dgInt32 dx = 2 * count;
				dgInt32 acc = dy - count;
				dgInt32 index = 0;
				for (dgInt32 i = 0; i < count; i++) {
					if (acc > 0) {
						contactsOut[index] = contactsOut[i];
						index++;
						acc -= dx;
					}
					acc += dy;
				}
				count = index;
			}

			for (dgInt32 i = 0; i < count; i++) {
				dgFloat32 y = contactsOut[i].m_y;
				dgFloat32 z = contactsOut[i].m_z;
				contactsOut[i].m_y = y * cosAng - z * sinAng;
				contactsOut[i].m_z = z * cosAng + y * sinAng;
			}
		}
	} else {
		dgFloat32 magInv = dgRsqrt(normal.m_y * normal.m_y + normal.m_z * normal.m_z);
		dgFloat32 cosAng = normal.m_y * magInv;
		dgFloat32 sinAng = normal.m_z * magInv;

		dgAssert(dgAbs(normal.m_z * cosAng - normal.m_y * sinAng) < dgFloat32(1.0e-4f));
		dgVector normal1(normal.m_x, normal.m_y * cosAng + normal.m_z * sinAng, dgFloat32(0.0f), dgFloat32(0.0f));
		dgVector origin1(origin.m_x, origin.m_y * cosAng + origin.m_z * sinAng, origin.m_z * cosAng - origin.m_y * sinAng, dgFloat32(0.0f));

		count = 0;
		int i0 = 2;
		dgVector test0((m_profile[i0] - origin1).DotProduct(normal1));
		for (int i = 0; (i < 3) && (count < 2); i++) {
			dgVector test1((m_profile[i] - origin1).DotProduct(normal1));
			dgVector acrossPlane(test0 * test1);
			if (acrossPlane.m_x < 0.0f) {
				dgVector step(m_profile[i] - m_profile[i0]);
				contactsOut[count] = m_profile[i0] - step.Scale(test0.m_x / (step.DotProduct(normal1).m_x));
				count++;
			}
			i0 = i;
			test0 = test1;
		}

		for (dgInt32 i = 0; i < count; i++) {
			dgFloat32 y = contactsOut[i].m_y;
			dgFloat32 z = contactsOut[i].m_z;
			contactsOut[i].m_y = y * cosAng - z * sinAng;
			contactsOut[i].m_z = z * cosAng + y * sinAng;
		}
	}
	
	return count;
}



void dgCollisionCone::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollisionConvex::GetCollisionInfo(info);

	info->m_cone.m_r = m_radius;
	info->m_cone.m_height = m_height * dgFloat32 (2.0f);
}

void dgCollisionCone::Serialize(dgSerialize callback, void* const userData) const
{
	dgVector size (m_radius, m_height * dgFloat32 (2.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	SerializeLow(callback, userData);
	callback (userData, &size, sizeof (dgVector));
}
