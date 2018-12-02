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
#include "dgCollisionBox.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dgInt32 dgCollisionBox::m_initSimplex = 0;
dgCollisionConvex::dgConvexSimplexEdge dgCollisionBox::m_edgeArray[24];
dgCollisionConvex::dgConvexSimplexEdge* dgCollisionBox::m_edgeEdgeMap[12];
dgCollisionConvex::dgConvexSimplexEdge* dgCollisionBox::m_vertexToEdgeMap[8];
dgInt32 dgCollisionBox::m_faces[][4] =
{
	{0, 1, 3, 2},
	{0, 4, 5, 1},
	{1, 5, 7, 3},
	{0, 2, 6, 4},
	{2, 3, 7, 6},
	{4, 6, 7, 5},
};


dgVector dgCollisionBox::m_indexMark (1.0f, 2.0f, 4.0f, 0.0f);

dgCollisionBox::dgCollisionBox(dgMemoryAllocator* allocator, dgUnsigned32 signature, dgFloat32 size_x, dgFloat32 size_y, dgFloat32 size_z)
	:dgCollisionConvex(allocator, signature, m_boxCollision)
{
	Init (size_x, size_y, size_z);
}

dgCollisionBox::dgCollisionBox(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	dgVector size;
	deserialization (userData, &size, sizeof (dgVector));
	Init (size.m_x, size.m_y, size.m_z);
}

void dgCollisionBox::Init (dgFloat32 size_x, dgFloat32 size_y, dgFloat32 size_z)
{
	m_rtti |= dgCollisionBox_RTTI;
	m_size[0].m_x = dgMax (dgAbs (size_x) * dgFloat32 (0.5f), D_MIN_CONVEX_SHAPE_SIZE);
	m_size[0].m_y = dgMax (dgAbs (size_y) * dgFloat32 (0.5f), D_MIN_CONVEX_SHAPE_SIZE);
	m_size[0].m_z = dgMax (dgAbs (size_z) * dgFloat32 (0.5f), D_MIN_CONVEX_SHAPE_SIZE);
	m_size[0].m_w = dgFloat32 (0.0f);

	m_size[1].m_x = - m_size[0].m_x;
	m_size[1].m_y = - m_size[0].m_y;
	m_size[1].m_z = - m_size[0].m_z;
	m_size[1].m_w = dgFloat32 (0.0f);

	m_edgeCount = 24;
	m_vertexCount = 8;

	m_vertex[0]	= dgVector ( m_size[0].m_x,  m_size[0].m_y,  m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[1]	= dgVector (-m_size[0].m_x,  m_size[0].m_y,  m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[2]	= dgVector ( m_size[0].m_x, -m_size[0].m_y,  m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[3]	= dgVector (-m_size[0].m_x, -m_size[0].m_y,  m_size[0].m_z, dgFloat32 (0.0f));

	m_vertex[4]	= dgVector ( m_size[0].m_x,  m_size[0].m_y, -m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[5]	= dgVector (-m_size[0].m_x,  m_size[0].m_y, -m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[6]	= dgVector ( m_size[0].m_x, -m_size[0].m_y, -m_size[0].m_z, dgFloat32 (0.0f));
	m_vertex[7]	= dgVector (-m_size[0].m_x, -m_size[0].m_y, -m_size[0].m_z, dgFloat32 (0.0f));

	dgCollisionConvex::m_vertex = m_vertex;
	dgCollisionConvex::m_simplex = m_edgeArray;

	if (!m_initSimplex) {
		dgPolyhedra polyhedra (GetAllocator());
		polyhedra.BeginFace();
		for (dgInt32 i = 0; i < 6; i ++) {
			polyhedra.AddFace (4, &m_faces[i][0]);
		}
		polyhedra.EndFace();

		int index = 0;
		dgInt32 mark = polyhedra.IncLRU();;
		dgPolyhedra::Iterator iter (polyhedra);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if (edge->m_mark != mark) {
				dgEdge* ptr = edge;
				do {
					ptr->m_mark = mark;
					ptr->m_userData = index;
					index ++;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge) ;
			}
		}
		dgAssert (index == 24);

		polyhedra.IncLRU();
		mark = polyhedra.IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			dgEdge *ptr = edge;
			do {
				ptr->m_mark = mark;
				dgConvexSimplexEdge* const simplexPtr = &m_simplex[ptr->m_userData];
				simplexPtr->m_vertex = ptr->m_incidentVertex;
				simplexPtr->m_next = &m_simplex[ptr->m_next->m_userData];
				simplexPtr->m_prev = &m_simplex[ptr->m_prev->m_userData];
				simplexPtr->m_twin = &m_simplex[ptr->m_twin->m_userData];
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge) ;
		} 

		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			m_vertexToEdgeMap[edge->m_incidentVertex] = &m_simplex[edge->m_userData];
		}

		dgInt32 count = 0;
		mark = polyhedra.IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if (edge->m_mark != mark) {
				edge->m_mark = mark;
				edge->m_twin->m_mark = mark;
				m_edgeEdgeMap[count] = &m_simplex[edge->m_userData];
				count ++;
				dgAssert (count <= 12);
			}
		}

		m_initSimplex = 1;
	}

	SetVolumeAndCG ();
}

dgCollisionBox::~dgCollisionBox()
{
	dgCollisionConvex::m_simplex = NULL;
	dgCollisionConvex::m_vertex = NULL;
}


void dgCollisionBox::SetCollisionBBox (const dgVector& p0__, const dgVector& p1__)
{
	dgAssert (0);
}

dgInt32 dgCollisionBox::CalculateSignature (dgFloat32 dx, dgFloat32 dy, dgFloat32 dz)
{
	dgUnsigned32 buffer[4];

	dx = dgAbs (dx);
	dy = dgAbs (dy);
	dz = dgAbs (dz);
	buffer[0] = m_boxCollision;
	buffer[1] = Quantize (dx * dgFloat32 (0.5f));
	buffer[2] = Quantize (dy * dgFloat32 (0.5f));
	buffer[3] = Quantize (dz * dgFloat32 (0.5f));
	return Quantize(buffer, sizeof (buffer));
}


dgInt32 dgCollisionBox::CalculateSignature () const
{
	return CalculateSignature(m_size[0].m_x, m_size[0].m_y, m_size[0].m_z);
}


dgVector dgCollisionBox::SupportVertex (const dgVector& dir0, dgInt32* const vertexIndex) const
{
	dgVector mask0(dir0.Abs() > m_flushZero);
	dgVector dir(dir0 & mask0);

	dgAssert (dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgVector mask (dir < dgVector (dgFloat32 (0.0f)));
	if (vertexIndex) {
		dgVector index (m_indexMark * (mask & dgVector::m_one));
		index = (index.AddHorizontal()).GetInt();
		*vertexIndex = dgInt32 (index.m_ix);
	}
	return (m_size[1] & mask) + m_size[0].AndNot(mask);
}

dgVector dgCollisionBox::SupportVertexSpecial(const dgVector& dir0, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgVector mask0(dir0.Abs() > m_flushZero);
	dgVector dir(dir0 & mask0);

	dgAssert(dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));
	dgAssert(dir.m_w == dgFloat32(0.0f));
	dgVector mask(dir < dgVector(dgFloat32(0.0f)));
	if (vertexIndex) {
		dgVector index(m_indexMark * (mask & dgVector::m_one));
		index = (index.AddHorizontal()).GetInt();
		*vertexIndex = dgInt32 (index.m_ix);
	}

	dgVector padd (DG_PENETRATION_TOL);
	padd = padd & dgVector::m_triplexMask;
	dgVector size0 (m_size[0] - padd);
	dgVector size1 (m_size[1] + padd);
	return (size1 & mask) + size0.AndNot(mask);
}

dgVector dgCollisionBox::SupportVertexSpecialProjectPoint(const dgVector& point, const dgVector& dir0) const
{
	dgVector mask0(dir0.Abs() > m_flushZero);
	dgVector dir(dir0 & mask0);
	dgAssert(dgAbs((dir.DotProduct(dir).GetScalar() - dgFloat32(1.0f))) < dgFloat32(1.0e-3f));
	return point + dir.Scale (DG_PENETRATION_TOL);
}

void dgCollisionBox::CalcAABB (const dgMatrix& matrix, dgVector &p0, dgVector &p1) const
{
	dgVector size (matrix[0].Abs().Scale(m_size[0].m_x) + matrix[1].Abs().Scale(m_size[0].m_y) + matrix[2].Abs().Scale(m_size[0].m_z));
	p0 = (matrix[3] - size) & dgVector::m_triplexMask;
	p1 = (matrix[3] + size) & dgVector::m_triplexMask;
}


dgFloat32 dgCollisionBox::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	dgAssert (localP0.m_w == dgFloat32 (0.0f));
	dgAssert (localP1.m_w == dgFloat32 (0.0f));

	dgInt32 index = 0;
	dgFloat32 signDir = dgFloat32 (0.0f);
	dgFloat32 tmin = dgFloat32 (0.0f);
	dgFloat32 tmax = dgFloat32 (1.0f);
	for (dgInt32 i = 0; i < 3; i++) {
		dgFloat32 dp = localP1[i] - localP0[i];
		if (dgAbs (dp) < dgFloat32 (1.0e-8f)) {
			if (localP0[i] <= m_size[1][i] || localP0[i] >= m_size[0][i]) {
				return dgFloat32 (1.2f);
			}
		} else {
			dp = dgFloat32 (1.0f) / dp; 
			dgFloat32 t1 = (m_size[1][i] - localP0[i]) * dp;
			dgFloat32 t2 = (m_size[0][i] - localP0[i]) * dp;

			dgFloat32 sign = dgFloat32 (-1.0f);
			if (t1 > t2) {
				sign = 1;
				dgSwap(t1, t2);
			}
			if (t1 > tmin) {
				signDir = sign;
				index = i;
				tmin = t1;
			}
			if (t2 < tmax) {
				tmax = t2;
			}
			if (tmin > tmax) {
				return dgFloat32 (1.2f);
			}
		}
	}

	if (tmin > dgFloat32 (0.0f)) {
		dgAssert (tmin <= 1.0f);
		contactOut.m_normal = dgVector (dgFloat32 (0.0f));
		contactOut.m_normal[index] = signDir;
		//contactOut.m_userId = SetUserDataID();
	} else {
		tmin = dgFloat32 (1.2f);
	}
	return tmin;

}


void dgCollisionBox::MassProperties ()
{
	m_centerOfMass = dgVector (dgFloat32 (0.0f));
	m_crossInertia = dgVector (dgFloat32 (0.0f));
	dgFloat32 volume = dgFloat32 (8.0f) * m_size[0].m_x * m_size[0].m_y * m_size[0].m_z; 
	m_inertia = dgVector (dgFloat32 (1.0f / 3.0f) * (m_size[0].m_y * m_size[0].m_y + m_size[0].m_z * m_size[0].m_z),
						  dgFloat32 (1.0f / 3.0f) * (m_size[0].m_x * m_size[0].m_x + m_size[0].m_z * m_size[0].m_z),
						  dgFloat32 (1.0f / 3.0f) * (m_size[0].m_x * m_size[0].m_x + m_size[0].m_y * m_size[0].m_y),
						  dgFloat32 (0.0f));
	m_centerOfMass.m_w = volume;
}



void dgCollisionBox::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollisionConvex::GetCollisionInfo(info);

	info->m_box.m_x = m_size[0].m_x * dgFloat32 (2.0f);
	info->m_box.m_y = m_size[0].m_y * dgFloat32 (2.0f);
	info->m_box.m_z = m_size[0].m_z * dgFloat32 (2.0f);
}

void dgCollisionBox::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);
	dgVector size (m_size[0].Scale (dgFloat32 (2.0f)));
	callback (userData, &size, sizeof (dgVector));
}

const dgCollisionConvex::dgConvexSimplexEdge** dgCollisionBox::GetVertexToEdgeMapping() const 
{
	return (const dgConvexSimplexEdge**)&m_vertexToEdgeMap[0];
}



dgInt32 dgCollisionBox::CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const
{
	dgVector support[4];
	dgInt32 featureCount = 3;

	const dgConvexSimplexEdge** const vertToEdgeMapping = GetVertexToEdgeMapping();
	if (vertToEdgeMapping) {
		dgInt32 edgeIndex;
		support[0] = SupportVertex (normal, &edgeIndex);

		dgFloat32 dist = normal.DotProduct(support[0] - point).GetScalar();
		if (dist <= DG_PENETRATION_TOL) {
			dgVector normalAlgin (normal.Abs());
			if (!((normalAlgin.m_x > dgFloat32 (0.9999f)) || (normalAlgin.m_y > dgFloat32 (0.9999f)) || (normalAlgin.m_z > dgFloat32 (0.9999f)))) {
				// 0.25 degrees
				const dgFloat32 tiltAngle = dgFloat32 (0.005f);
				const dgFloat32 tiltAngle2 = tiltAngle * tiltAngle ;
				dgPlane testPlane (normal, - (normal.DotProduct(support[0]).GetScalar()));

				featureCount = 1;
				const dgConvexSimplexEdge* const edge = vertToEdgeMapping[edgeIndex];
				const dgConvexSimplexEdge* ptr = edge;
				do {
					const dgVector& p = m_vertex[ptr->m_twin->m_vertex];
					dgFloat32 test1 = testPlane.Evalue(p);
					dgVector dist1 (p - support[0]);
					dgFloat32 angle2 = test1 * test1 / (dist1.DotProduct(dist1).GetScalar());
					if (angle2 < tiltAngle2) {
						support[featureCount] = p;
						featureCount ++;
					}
					ptr = ptr->m_twin->m_next;
				} while ((ptr != edge) && (featureCount < 3));
			}
		}
	}

	dgInt32 count = 0;
	switch (featureCount)
	{
		case 1:
		{
			contactsOut[0] = support[0] - normal * normal.DotProduct(support[0] - point);
			count = 1;
			break;
		}

		case 2:
		{
			contactsOut[0] = support[0] - normal * normal.DotProduct(support[0] - point);
			contactsOut[1] = support[1] - normal * normal.DotProduct(support[1] - point);
			count = 2;
			break;
		}

		default:
		{
			dgFloat32 test[8];
			dgAssert(normal.m_w == dgFloat32(0.0f));
			dgPlane plane(normal, -(normal.DotProduct(point).GetScalar()));
			for (dgInt32 i = 0; i < 8; i++) {
				dgAssert(m_vertex[i].m_w == dgFloat32(0.0f));
				test[i] = plane.DotProduct(m_vertex[i] | dgVector::m_wOne).m_x;
			}

			dgConvexSimplexEdge* edge = NULL;
			for (dgInt32 i = 0; i < dgInt32 (sizeof (m_edgeEdgeMap) / sizeof (m_edgeEdgeMap[0])); i ++) {
				dgConvexSimplexEdge* const ptr = m_edgeEdgeMap[i];
				dgFloat32 side0 = test[ptr->m_vertex];
				dgFloat32 side1 = test[ptr->m_twin->m_vertex];
				if ((side0 * side1) < dgFloat32 (0.0f)) {
					edge = ptr;
					break;
				}
			}

			if (edge) {
				if (test[edge->m_vertex] < dgFloat32 (0.0f)) {
					edge = edge->m_twin;
				}
				dgAssert (test[edge->m_vertex] > dgFloat32 (0.0f));

				dgConvexSimplexEdge* ptr = edge;
				dgConvexSimplexEdge* firstEdge = NULL;
				dgFloat32 side0 = test[edge->m_vertex];
				do {
					dgAssert (m_vertex[ptr->m_twin->m_vertex].m_w == dgFloat32 (0.0f));
					dgFloat32 side1 = test[ptr->m_twin->m_vertex];
					if (side1 < side0) {
						if (side1 < dgFloat32 (0.0f)) {
							firstEdge = ptr;
							break;
						}

						side0 = side1;
						edge = ptr->m_twin;
						ptr = edge;
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				if (firstEdge) {
					edge = firstEdge;
					ptr = edge;
					do {
						dgVector dp (m_vertex[ptr->m_twin->m_vertex] - m_vertex[ptr->m_vertex]);
						dgFloat32 t = plane.DotProduct(dp).m_x;
						if (t >= dgFloat32 (-1.e-24f)) {
							t = dgFloat32 (0.0f);
						} else {
							t = test[ptr->m_vertex] / t;
							if (t > dgFloat32 (0.0f)) {
								t = dgFloat32 (0.0f);
							}
							if (t < dgFloat32 (-1.0f)) {
								t = dgFloat32 (-1.0f);
							}
						}

						dgAssert (t <= dgFloat32 (0.01f));
						dgAssert (t >= dgFloat32 (-1.05f));
						contactsOut[count] = m_vertex[ptr->m_vertex] - dp.Scale (t);
						count ++;

						dgConvexSimplexEdge* ptr1 = ptr->m_next;
						for (; ptr1 != ptr; ptr1 = ptr1->m_next) {
							dgInt32 index0 = ptr1->m_twin->m_vertex;
							if (test[index0] >= dgFloat32 (0.0f)) {
								dgAssert (test[ptr1->m_vertex] <= dgFloat32 (0.0f));
								break;
							}
						}
						dgAssert (ptr != ptr1);
						ptr = ptr1->m_twin;

					} while ((ptr != edge) && (count < 8));
				}
			}
		}
	}

	if (count > 2) {
		count = RectifyConvexSlice (count, normal, contactsOut);
	}
	return count;
}
