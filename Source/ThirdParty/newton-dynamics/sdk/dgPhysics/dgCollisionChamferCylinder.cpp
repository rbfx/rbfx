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
#include "dgCollisionChamferCylinder.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



dgInt32 dgCollisionChamferCylinder::m_shapeRefCount = 0;
dgVector dgCollisionChamferCylinder::m_yzMask (0, 0xffffffff, 0xffffffff, 0);
dgVector dgCollisionChamferCylinder::m_shapesDirs[DG_MAX_CHAMFERCYLINDER_DIR_COUNT];
dgCollisionConvex::dgConvexSimplexEdge dgCollisionChamferCylinder::m_edgeArray[(4 * DG_CHAMFERCYLINDER_SLICES + 2)* DG_CHAMFERCYLINDER_BRAKES];

dgCollisionChamferCylinder::dgCollisionChamferCylinder(dgMemoryAllocator* allocator, dgUnsigned32 signature, dgFloat32 radius, dgFloat32 height)
	:dgCollisionConvex(allocator, signature, m_chamferCylinderCollision)
{
	Init (radius, height);
}

dgCollisionChamferCylinder::dgCollisionChamferCylinder(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	dgVector size;
	deserialization (userData, &size, sizeof (dgVector));
	Init (size.m_x, size.m_y);
}


dgCollisionChamferCylinder::~dgCollisionChamferCylinder()
{
	m_shapeRefCount --;
	dgAssert (m_shapeRefCount >= 0);

	dgCollisionConvex::m_simplex = NULL;
	dgCollisionConvex::m_vertex = NULL;
}


void dgCollisionChamferCylinder::Init (dgFloat32 radius, dgFloat32 height)
{
	m_rtti |= dgCollisionChamferCylinder_RTTI;

	m_radius = dgMax (dgAbs (radius), D_MIN_CONVEX_SHAPE_SIZE);
	m_height = dgMax (dgAbs (height * dgFloat32 (0.5f)), D_MIN_CONVEX_SHAPE_SIZE);

	dgFloat32 sliceAngle = dgFloat32 (0.0f);
	dgFloat32 sliceStep = dgPI  / DG_CHAMFERCYLINDER_SLICES; 
	dgFloat32 breakStep = dgPI2 / DG_CHAMFERCYLINDER_BRAKES;

	dgMatrix rot (dgPitchMatrix (breakStep));	
	dgInt32 index = 0;
	for (dgInt32 j = 0; j <= DG_CHAMFERCYLINDER_SLICES; j ++) {
		dgVector p0 (-m_height * dgCos(sliceAngle), dgFloat32 (0.0f), m_radius + m_height * dgSin(sliceAngle), dgFloat32 (0.0f));
		sliceAngle += sliceStep;
		for (dgInt32 i = 0; i < DG_CHAMFERCYLINDER_BRAKES; i ++) {
			m_vertex[index] = p0;
			index ++;
			p0 = rot.UnrotateVector (p0);
		}
	}

	m_edgeCount = (4 * DG_CHAMFERCYLINDER_SLICES + 2)* DG_CHAMFERCYLINDER_BRAKES;
	m_vertexCount = DG_CHAMFERCYLINDER_BRAKES * (DG_CHAMFERCYLINDER_SLICES + 1);
	dgCollisionConvex::m_vertex = m_vertex;

	if (!m_shapeRefCount) {
		dgPolyhedra polyhedra(m_allocator);
		dgInt32 wireframe[DG_CHAMFERCYLINDER_SLICES + 10];

		for (dgInt32 i = 0; i < DG_MAX_CHAMFERCYLINDER_DIR_COUNT; i ++) {
			dgMatrix matrix (dgPitchMatrix (dgFloat32 (dgPI2 * i) / DG_MAX_CHAMFERCYLINDER_DIR_COUNT));
			m_shapesDirs[i] = matrix.RotateVector (dgVector (dgFloat32 (0.0f), dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f)));
		}


		dgInt32 index0 = 0;
		for (dgInt32 j = 0; j < DG_CHAMFERCYLINDER_SLICES; j ++) {
			dgInt32 index1 = index0 + DG_CHAMFERCYLINDER_BRAKES - 1;
			for (dgInt32 i = 0; i < DG_CHAMFERCYLINDER_BRAKES; i ++) {
				wireframe[0] = index0;
				wireframe[1] = index1;
				wireframe[2] = index1 + DG_CHAMFERCYLINDER_BRAKES;
				wireframe[3] = index0 + DG_CHAMFERCYLINDER_BRAKES;

				index1 = index0;
				index0 ++;
				polyhedra.AddFace (4, wireframe);
			}
		}

		for (dgInt32 i = 0; i < DG_CHAMFERCYLINDER_BRAKES; i ++) { 
			wireframe[i] = i;
		}
		polyhedra.AddFace (DG_CHAMFERCYLINDER_BRAKES, wireframe);

		for (dgInt32 i = 0; i < DG_CHAMFERCYLINDER_BRAKES; i ++) { 
			wireframe[i] = DG_CHAMFERCYLINDER_BRAKES * (DG_CHAMFERCYLINDER_SLICES + 1) - i - 1;
		}
		polyhedra.AddFace (DG_CHAMFERCYLINDER_BRAKES, wireframe);
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

	m_shapeRefCount ++;
	dgCollisionConvex::m_simplex = m_edgeArray;

	SetVolumeAndCG ();
}


void dgCollisionChamferCylinder::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgInt32 slices = 12;
	dgInt32 brakes = 24;
	dgFloat32 sliceAngle = dgFloat32 (0.0f);
	dgFloat32 sliceStep = dgPI  / slices; 
	dgFloat32 breakStep = dgPI2 / brakes;

	dgTriplex pool[24 * (12 + 1)];

	dgMatrix rot (dgPitchMatrix (breakStep));	
	dgInt32 index = 0;
	for (dgInt32 j = 0; j <= slices; j ++) {
		dgVector p0 (-m_height * dgCos(sliceAngle), dgFloat32 (0.0f), m_radius + m_height * dgSin(sliceAngle), dgFloat32 (0.0f));
		sliceAngle += sliceStep;
		for (dgInt32 i = 0; i < brakes; i ++) {
			pool[index].m_x = p0.m_x;
			pool[index].m_y = p0.m_y;
			pool[index].m_z = p0.m_z;
			index ++;
			p0 = rot.UnrotateVector (p0);
		}
	}

	matrix.TransformTriplex (&pool[0].m_x, sizeof (dgTriplex), &pool[0].m_x, sizeof (dgTriplex), 24 * (12 + 1));

	dgTriplex face[32];

	index = 0;
	for (dgInt32 j = 0; j < slices; j ++) {
		dgInt32 index0 = index + brakes - 1;
		for (dgInt32 i = 0; i < brakes; i ++) {
			face[0] = pool[index];
			face[1] = pool[index0];
			face[2] = pool[index0 + brakes];
			face[3] = pool[index + brakes];
			index0 = index;
			index ++;
			callback (userData, 4, &face[0].m_x, 0);
		}
	}

	for (dgInt32 i = 0; i < brakes; i ++) { 
		face[i] = pool[i];
	}
	callback (userData, 24, &face[0].m_x, 0);

	for (dgInt32 i = 0; i < brakes; i ++) { 
		face[i] = pool[brakes * (slices + 1) - i - 1];
	}
	callback (userData, 24, &face[0].m_x, 0);
}


dgInt32 dgCollisionChamferCylinder::CalculateSignature (dgFloat32 radius, dgFloat32 height)
{
	dgUnsigned32 buffer[3];

	memset (buffer, 0, sizeof (buffer));
	buffer[0] = m_chamferCylinderCollision;
	buffer[1] = dgCollision::Quantize (radius);
	buffer[2] = dgCollision::Quantize (height);
	return dgInt32 (dgCollision::Quantize(buffer, sizeof (buffer)));
}

dgInt32 dgCollisionChamferCylinder::CalculateSignature () const
{
	return CalculateSignature (m_radius, m_height);
}


void dgCollisionChamferCylinder::SetCollisionBBox (const dgVector& p0__, const dgVector& p1__)
{
	dgAssert (0);
}


void dgCollisionChamferCylinder::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollisionConvex::GetCollisionInfo(info);

	info->m_chamferCylinder.m_r = m_radius;
	info->m_chamferCylinder.m_height = m_height * dgFloat32 (2.0f);
}

void dgCollisionChamferCylinder::Serialize(dgSerialize callback, void* const userData) const
{
	dgVector size (m_radius, m_height * dgFloat32 (2.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));

	SerializeLow(callback, userData);
	callback (userData, &size, sizeof (dgVector));
}



dgFloat32 dgCollisionChamferCylinder::RayCast(const dgVector& q0, const dgVector& q1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	if (q0.m_x > m_height) {
		if (q1.m_x < m_height) {
			dgFloat32 t1 = (m_height - q0.m_x) / (q1.m_x - q0.m_x);
			dgFloat32 y = q0.m_y + (q1.m_y - q0.m_y) * t1;
			dgFloat32 z = q0.m_z + (q1.m_z - q0.m_z) * t1;
			if ((y * y + z * z) < m_radius * m_radius) {
				contactOut.m_normal = dgVector(dgFloat32(1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
				return t1;
			}
		}
	}

	if (q0.m_x < -m_height) {
		if (q1.m_x > -m_height) {
			dgFloat32 t1 = (-m_height - q0.m_x) / (q1.m_x - q0.m_x);
			dgFloat32 y = q0.m_y + (q1.m_y - q0.m_y) * t1;
			dgFloat32 z = q0.m_z + (q1.m_z - q0.m_z) * t1;
			if ((y * y + z * z) < m_radius * m_radius) {
				contactOut.m_normal = dgVector(dgFloat32(-1.0f), dgFloat32(0.0f), dgFloat32(0.0f), dgFloat32(0.0f));
				return t1;
			}
		}
	}

	dgVector dq((q1 - q0) & dgVector::m_triplexMask);

	// avoid NaN as a result of a division by zero
	if (dq.DotProduct(dq).GetScalar() <= 0.0f) {
		return dgFloat32(1.2f);
	}

	//dgVector dir(dq * dq.InvMagSqrt());
	dgVector dir(dq.Normalize());
	if (dgAbs(dir.m_x) > 0.9999f) {
		return dgCollisionConvex::RayCast(q0, q1, maxT, contactOut, body, NULL, NULL);
	}

	dgVector p0(q0 & dgVector::m_triplexMask);
	dgVector p1(q1 & dgVector::m_triplexMask);

	p0.m_x = dgFloat32 (0.0f);
	p1.m_x = dgFloat32 (0.0f);

	dgVector dp (p1 - p0);
	dgFloat32 a = dp.DotProduct(dp).GetScalar();
	dgFloat32 b = dgFloat32 (2.0f) * dp.DotProduct(p0).GetScalar();
	dgFloat32 c = p0.DotProduct(p0).GetScalar() - m_radius * m_radius;

	dgFloat32 disc = b * b - dgFloat32 (4.0f) * a * c;
	if (disc >= dgFloat32 (0.0f)) {
		disc = dgSqrt (disc);
		dgVector origin0(p0 + dp.Scale ((-b + disc) / (dgFloat32 (2.0f) * a)));
		dgVector origin1(p0 + dp.Scale ((-b - disc) / (dgFloat32 (2.0f) * a)));
		dgFloat32 t0 = dgRayCastSphere(q0, q1, origin0, m_height);
		dgFloat32 t1 = dgRayCastSphere(q0, q1, origin1, m_height);
		if(t1 < t0) {
			t0 = t1;
			origin0 = origin1;
		}

		if ((t0 >= 0.0f) && (t0 <= 1.0f)) {
			contactOut.m_normal = q0 + dq.Scale(t0) - origin0;
			dgAssert(contactOut.m_normal.m_w == dgFloat32(0.0f));

			//contactOut.m_normal = contactOut.m_normal * contactOut.m_normal.DotProduct(contactOut.m_normal).InvSqrt();
			contactOut.m_normal = contactOut.m_normal.Normalize();
			return t0;
		}
	} else {
		dgVector origin0 (dgPointToRayDistance (dgVector::m_zero, p0, p1)); 
		origin0 = origin0.Scale(m_radius / dgSqrt(origin0.DotProduct(origin0).GetScalar()));
		dgFloat32 t0 = dgRayCastSphere(q0, q1, origin0, m_height);
		if ((t0 >= 0.0f) && (t0 <= 1.0f)) {
			contactOut.m_normal = q0 + dq.Scale(t0) - origin0;
			dgAssert(contactOut.m_normal.m_w == dgFloat32(0.0f));
			
			//contactOut.m_normal = contactOut.m_normal * contactOut.m_normal.DotProduct(contactOut.m_normal).InvSqrt();
			contactOut.m_normal = contactOut.m_normal.Normalize();
			return t0;
		}
	}
	return dgFloat32(1.2f);
}


dgVector dgCollisionChamferCylinder::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert (dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	dgFloat32 x = dir.GetScalar();
	if (dgAbs (x) > dgFloat32 (0.9999f)) {
		//return dgVector ((x > dgFloat32 (0.0f)) ? m_height : - m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f)); 
		return dgVector (dgSign (x) * m_height, m_radius, dgFloat32 (0.0f), dgFloat32 (0.0f)); 
	}

	dgVector sideDir (m_yzMask & dir);
	//sideDir = sideDir * sideDir.InvMagSqrt();
	sideDir = sideDir.Normalize();
	return sideDir.Scale(m_radius) + dir.Scale (m_height);
}


dgVector dgCollisionChamferCylinder::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert (dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	dgFloat32 x = dir.GetScalar();
	if (dgAbs (x) > dgFloat32 (0.99995f)) {
		return dgVector (dgFloat32 (0.0f), m_radius, dgFloat32 (0.0f), dgFloat32 (0.0f)); 
	}

	dgVector sideDir (m_yzMask & dir);
	dgAssert (sideDir.DotProduct(sideDir).GetScalar() > dgFloat32 (0.0f));
	return sideDir.Normalize().Scale(m_radius);
}

dgVector dgCollisionChamferCylinder::SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const
{
	dgAssert (dir.m_w == 0.0f);
	return point + dir.Scale(m_height - DG_PENETRATION_TOL);
}


dgInt32 dgCollisionChamferCylinder::CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut) const
{
	dgInt32 count = 0;
	const dgFloat32 inclination = dgFloat32 (0.9999f);
	if (normal.m_x < -inclination) {
		dgMatrix matrix(normal);
		dgFloat32 x = dgSqrt (dgMax (m_height * m_height - origin.m_x * origin.m_x, dgFloat32 (0.0f)));
		matrix.m_posit.m_x = origin.m_x;
		dgVector scale(m_radius + x);
		const int n = sizeof (m_unitCircle) / sizeof (m_unitCircle[0]);
		for (dgInt32 i = 0; i < n; i++) {
			contactsOut[i] = matrix.TransformVector(m_unitCircle[i] * scale) & dgVector::m_triplexMask;
		}
		count = RectifyConvexSlice(n, normal, contactsOut);
	} else if (normal.m_x > inclination) {
		dgMatrix matrix(normal);
		dgFloat32 x = dgSqrt (dgMax (m_height * m_height - origin.m_x * origin.m_x, dgFloat32 (0.0f)));
		matrix.m_posit.m_x = origin.m_x;
		dgVector scale(m_radius + x);
		const int n = sizeof (m_unitCircle) / sizeof (m_unitCircle[0]);
		for (dgInt32 i = 0; i < n; i++) {
			contactsOut[i] = matrix.TransformVector(m_unitCircle[i] * scale) & dgVector::m_triplexMask;
		}
		count = RectifyConvexSlice(n, normal, contactsOut);
	} else {
		count = 1;
		contactsOut[0] = SupportVertex (normal, NULL);
	}
	return count;
}
