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
#include "dgCollisionConvexPolygon.h"
#include "dgCollisionCylinder.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////// 

dgInt32 dgCollisionCylinder::m_shapeRefCount = 0;
dgCollisionConvex::dgConvexSimplexEdge dgCollisionCylinder::m_edgeArray[DG_TAPED_CYLINDER_SEGMENTS * 2 * 3];

dgCollisionCylinder::dgCollisionCylinder(dgMemoryAllocator* allocator, dgUnsigned32 signature, dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
	:dgCollisionConvex(allocator, signature, m_cylinderCollision)
{
	Init (radio0, radio1, height);
}


dgCollisionCylinder::dgCollisionCylinder(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	dgVector size;
	deserialization (userData, &size, sizeof (dgVector));
	Init (size.m_x, size.m_y, size.m_z);
}

void dgCollisionCylinder::Init (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
{
	m_rtti |= dgCollisionCylinder_RTTI;
	m_radio0 = dgMax (dgAbs (radio0), D_MIN_CONVEX_SHAPE_SIZE);
	m_radio1 = dgMax (dgAbs (radio1), D_MIN_CONVEX_SHAPE_SIZE);
	m_height = dgMax (dgAbs (height * dgFloat32 (0.5f)), D_MIN_CONVEX_SHAPE_SIZE);

	dgFloat32 angle = dgFloat32 (0.0f);
	for (dgInt32 i = 0; i < DG_TAPED_CYLINDER_SEGMENTS; i ++) {
		dgFloat32 sinAngle = dgSin (angle);
		dgFloat32 cosAngle = dgCos (angle);
		m_vertex[i                       ]       = dgVector (- m_height, m_radio0 * cosAngle, m_radio0 * sinAngle, dgFloat32 (0.0f));
		m_vertex[i + DG_TAPED_CYLINDER_SEGMENTS] = dgVector (  m_height, m_radio1 * cosAngle, m_radio1 * sinAngle, dgFloat32 (0.0f));
		angle += dgPI2 / DG_TAPED_CYLINDER_SEGMENTS;
	}

	m_edgeCount = DG_TAPED_CYLINDER_SEGMENTS * 6;
	m_vertexCount = DG_TAPED_CYLINDER_SEGMENTS * 2;
	dgCollisionConvex::m_vertex = m_vertex;

	if (!m_shapeRefCount) {
		dgPolyhedra polyhedra(m_allocator);
		dgInt32 wireframe[DG_TAPED_CYLINDER_SEGMENTS];

		dgInt32 j = DG_TAPED_CYLINDER_SEGMENTS - 1;
		polyhedra.BeginFace ();
		for (dgInt32 i = 0; i < DG_TAPED_CYLINDER_SEGMENTS; i ++) { 
			wireframe[0] = j;
			wireframe[1] = i;
			wireframe[2] = i + DG_TAPED_CYLINDER_SEGMENTS;
			wireframe[3] = j + DG_TAPED_CYLINDER_SEGMENTS;
			j = i;
			polyhedra.AddFace (4, wireframe);
		}

		for (dgInt32 i = 0; i < DG_TAPED_CYLINDER_SEGMENTS; i ++) { 
			wireframe[i] = DG_TAPED_CYLINDER_SEGMENTS - 1 - i;
		}
		polyhedra.AddFace (DG_TAPED_CYLINDER_SEGMENTS, wireframe);

		for (dgInt32 i = 0; i < DG_TAPED_CYLINDER_SEGMENTS; i ++) { 
			wireframe[i] = i + DG_TAPED_CYLINDER_SEGMENTS;
		}
		polyhedra.AddFace (DG_TAPED_CYLINDER_SEGMENTS, wireframe);
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

	m_profile[0] = dgVector( m_height,  m_radio1, dgFloat32 (0.0f), dgFloat32 (0.0f));
	m_profile[1] = dgVector(-m_height,  m_radio0, dgFloat32 (0.0f), dgFloat32 (0.0f));
	m_profile[2] = dgVector(-m_height, -m_radio0, dgFloat32 (0.0f), dgFloat32 (0.0f));
	m_profile[3] = dgVector(m_height,  -m_radio1, dgFloat32 (0.0f), dgFloat32 (0.0f));

	m_shapeRefCount ++;
	dgCollisionConvex::m_simplex = m_edgeArray;

	SetVolumeAndCG ();
}

dgCollisionCylinder::~dgCollisionCylinder()
{
	m_shapeRefCount --;
	dgAssert (m_shapeRefCount >= 0);

	dgCollisionConvex::m_simplex = NULL;
	dgCollisionConvex::m_vertex = NULL;
}

dgInt32 dgCollisionCylinder::CalculateSignature (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
{
	dgUnsigned32 buffer[4];

	buffer[0] = m_cylinderCollision;
	buffer[1] = Quantize (radio0);
	buffer[2] = Quantize (radio1);
	buffer[3] = Quantize (height);
	return Quantize(buffer, sizeof (buffer));
}

dgInt32 dgCollisionCylinder::CalculateSignature () const
{
	return CalculateSignature (m_radio0, m_radio1, m_height);
}


void dgCollisionCylinder::SetCollisionBBox (const dgVector& p0, const dgVector& p1)
{
	dgAssert (0);
}


void dgCollisionCylinder::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgTriplex pool[24 * 2];

	dgFloat32 angle = dgFloat32 (0.0f);
	for (dgInt32 i = 0; i < 24; i ++) {
		dgFloat32 z = dgSin (angle);
		dgFloat32 y = dgCos (angle);
		pool[i].m_x = - m_height;
		pool[i].m_y = y * m_radio0;
		pool[i].m_z = z * m_radio0;
		pool[i + 24].m_x = m_height;
		pool[i + 24].m_y = y * m_radio1;
		pool[i + 24].m_z = z * m_radio1;
		angle += dgPI2 / dgFloat32 (24.0f);
	}

	matrix.TransformTriplex (&pool[0].m_x, sizeof (dgTriplex), &pool[0].m_x, sizeof (dgTriplex), 24 * 2);

	dgTriplex face[24];

	dgInt32 j = 24 - 1;
	for (dgInt32 i = 0; i < 24; i ++) { 
		face[0] = pool[j];
		face[1] = pool[i];
		face[2] = pool[i + 24];
		face[3] = pool[j + 24];
		j = i;
		callback (userData, 4, &face[0].m_x, 0);
	}

	for (dgInt32 i = 0; i < 24; i ++) { 
		face[i] = pool[24 - 1 - i];
	}
	callback (userData, 24, &face[0].m_x, 0);
		
	for (dgInt32 i = 0; i < 24; i ++) { 
		face[i] = pool[i + 24];
	}
	callback (userData, 24, &face[0].m_x, 0);
}



dgVector dgCollisionCylinder::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert (dgAbs (dir.DotProduct(dir).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	if (dir.m_x < dgFloat32(-0.9999f)) {
		return dgVector(-m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else if (dir.m_x > dgFloat32(0.9999f)) {
		return dgVector(m_height, dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
	} else {
		dgVector dir_yz (dir);
		dir_yz.m_x = dgFloat32 (0.0f);
		dgFloat32 mag2 = dir_yz.DotProduct(dir_yz).GetScalar();
		dgAssert (mag2 > dgFloat32 (0.0f));
		dir_yz = dir_yz.Scale (dgFloat32 (1.0f) / dgSqrt (mag2));
		dgVector p0 (dir_yz.Scale (m_radio0));
		dgVector p1 (dir_yz.Scale (m_radio1));

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


dgVector dgCollisionCylinder::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));

	const dgFloat32 thickness = DG_PENETRATION_TOL + skinThickness;
	if (dir.m_x < dgFloat32 (-0.9999f)) {
		return dgVector (-(m_height - thickness), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	} else if (dir.m_x > dgFloat32 (0.9999f)) {
		return dgVector ( m_height - thickness, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	} else {
		dgVector dir_yz(dir);
		dir_yz.m_x = dgFloat32(0.0f);
		dgFloat32 mag2 = dir_yz.DotProduct(dir_yz).GetScalar();
		dgAssert (mag2 > dgFloat32 (0.0f));
		dir_yz = dir_yz.Scale(dgFloat32(1.0f) / dgSqrt(mag2));
		dgVector p0(dir_yz.Scale(m_radio0 - thickness));
		dgVector p1(dir_yz.Scale(m_radio1 - thickness));

		p0.m_x = - (m_height - thickness);
		p1.m_x =   m_height - thickness;

		dgFloat32 dist0 = dir.DotProduct(p0).GetScalar();
		dgFloat32 dist1 = dir.DotProduct(p1).GetScalar();

		if (dist1 >= dist0) {
			p0 = p1;
		}
		return p0;
	}
}

dgVector dgCollisionCylinder::SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));
	return point + dir.Scale (DG_PENETRATION_TOL);
}

dgFloat32 dgCollisionCylinder::CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const
{
	return dgCollisionConvex::CalculateMassProperties (offset, inertia, crossInertia, centerOfMass);
}

dgInt32 dgCollisionCylinder::CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut) const
{
	dgInt32 count = 0;
	const dgFloat32 inclination = dgFloat32(0.9998f);
	if (normal.m_x < dgFloat32 (-0.995f)) {
		if (normal.m_x < -inclination) {
			dgMatrix matrix(normal);
			matrix.m_posit.m_x = origin.m_x;
			dgVector scale(m_radio0);
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
			dgVector origin1(origin.m_x, origin.m_y * cosAng + origin.m_z * sinAng,	origin.m_z * cosAng - origin.m_y * sinAng, dgFloat32(0.0f));

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
	} else if (normal.m_x > dgFloat32 (0.995f)) {
		if (normal.m_x > inclination) {
			dgMatrix matrix(normal);
			matrix.m_posit.m_x = origin.m_x;
			dgVector scale(m_radio1);
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
		int i0 = 3;
		dgVector test0((m_profile[i0] - origin1).DotProduct(normal1));
		for (int i = 0; (i < 4) && (count < 2); i++) {
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


void dgCollisionCylinder::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollisionConvex::GetCollisionInfo(info);

	info->m_cylinder.m_radio0 = m_radio0;
	info->m_cylinder.m_radio1 = m_radio1;
	info->m_cylinder.m_height = m_height * dgFloat32 (2.0f);
}


void dgCollisionCylinder::Serialize(dgSerialize callback, void* const userData) const
{
	dgVector size (m_radio0, m_radio1, m_height * dgFloat32 (2.0f), dgFloat32 (0.0f));
	SerializeLow(callback, userData);
	callback (userData, &size, sizeof (dgVector));
}

