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
#include "dgContactSolver.h"
#include "dgCollisionMesh.h"
#include "dgCollisionConvex.h"
#include "dgCollisionInstance.h"
#include "dgCollisionConvexHull.h"
#include "dgCollisionConvexPolygon.h"

#define DG_MAX_MIN_VOLUME				dgFloat32 (1.0e-6f)
#define DG_MAX_VERTEX_CLIP_FACE			16


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dgVector dgCollisionConvex::m_unitCircle[] = { dgVector(dgFloat32(0.0f), dgFloat32(1.0f), dgFloat32(0.000000f), dgFloat32(0.0f)),
												dgVector(dgFloat32(0.0f), dgFloat32(0.5f), dgFloat32(0.866025f), dgFloat32(0.0f)),
												dgVector(dgFloat32(0.0f), dgFloat32(-0.5f), dgFloat32(0.866026f), dgFloat32(0.0f)),
												dgVector(dgFloat32(0.0f), dgFloat32(-1.0f), dgFloat32(0.000000f), dgFloat32(0.0f)),
												dgVector(dgFloat32(0.0f), dgFloat32(-0.5f), dgFloat32(-0.866025f), dgFloat32(0.0f)),
												dgVector(dgFloat32(0.0f), dgFloat32(0.5f), dgFloat32(-0.866026f), dgFloat32(0.0f)) };



dgCollisionConvex::dgCollisionConvex (dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgCollisionID id)
	:dgCollision(allocator, signature, id) 
	,m_vertex (NULL)
	,m_simplex (NULL)
	,m_boxMinRadius (dgFloat32 (0.0f))
	,m_boxMaxRadius (dgFloat32 (0.0f))
	,m_simplexVolume (dgFloat32 (0.0f))
	,m_edgeCount (0)
	,m_vertexCount (0)
{
	m_rtti |= dgCollisionConvexShape_RTTI;
}


dgCollisionConvex::dgCollisionConvex (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollision (world, deserialization, userData, revisionNumber)
	,m_vertex (NULL)
	,m_simplex (NULL)
	,m_boxMinRadius (dgFloat32 (0.0f))
	,m_boxMaxRadius (dgFloat32 (0.0f))
	,m_simplexVolume (dgFloat32 (0.0f))
	,m_edgeCount (0)
	,m_vertexCount (0)
{
	dgAssert (m_rtti | dgCollisionConvexShape_RTTI);
}


dgCollisionConvex::~dgCollisionConvex ()
{
	if (m_vertex) {
		m_allocator->Free (m_vertex);
	}

	if (m_simplex) {
		m_allocator->Free (m_simplex);
	}
}

void dgCollisionConvex::SerializeLow(dgSerialize callback, void* const userData) const
{
	dgCollision::SerializeLow(callback, userData);
}


void dgCollisionConvex::SetVolumeAndCG ()
{
	dgVector faceVertex[DG_MAX_EDGE_COUNT];
	dgStack<dgInt8> edgeMarks (m_edgeCount);
	memset (&edgeMarks[0], 0, sizeof (dgInt8) * m_edgeCount);

	dgPolyhedraMassProperties localData;
	for (dgInt32 i = 0; i < m_edgeCount; i ++) {
		dgConvexSimplexEdge* const face = &m_simplex[i];
		if (!edgeMarks[i]) {
			dgConvexSimplexEdge* edge = face;
			dgInt32 count = 0;
			do {
				dgAssert ((edge - m_simplex) >= 0);
				edgeMarks[dgInt32 (edge - m_simplex)] = '1';
				faceVertex[count] = m_vertex[edge->m_vertex];
				count ++;
				dgAssert (count < dgInt32 (sizeof (faceVertex) / sizeof (faceVertex[0])));
				edge = edge->m_next;
			} while (edge != face);
			localData.AddCGFace (count, faceVertex);
		}
	}

	dgVector origin;
	dgVector inertia;
	dgVector crossInertia;
	dgFloat32 volume = localData.MassProperties (origin, inertia, crossInertia);
	m_simplexVolume = volume;

	// calculate the origin of the bound box of this primitive
	dgVector p0(dgFloat32 (0.0f)); 
	dgVector p1(dgFloat32 (0.0f)); 
	for (dgInt32 i = 0; i < 3; i ++) {
		dgVector dir (dgFloat32 (0.0f)); 
		dir[i] = dgFloat32 (-1.0f);
		p0[i] = SupportVertex(dir, NULL)[i];

		dir[i] = dgFloat32 (1.0f);
		p1[i] = SupportVertex(dir, NULL)[i];
	}
//	p0[3] = dgFloat32 (0.0f);
//	p1[3] = dgFloat32 (0.0f);
	dgAssert (p0.m_w == dgFloat32 (0.0f));
	dgAssert (p1.m_w == dgFloat32 (0.0f));
	m_boxSize = (p1 - p0) * dgVector::m_half; 
	m_boxOrigin = (p1 + p0) * dgVector::m_half; 
	m_boxMinRadius = dgMin(m_boxSize.m_x, m_boxSize.m_y, m_boxSize.m_z);
	m_boxMaxRadius = dgSqrt ((m_boxSize.DotProduct(m_boxSize)).GetScalar());

	MassProperties ();
}


bool dgCollisionConvex::SanityCheck (dgPolyhedra& hull) const
{
	dgPolyhedra::Iterator iter (hull);
	for (iter.Begin(); iter; iter ++) { 
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			return false;
		}
		dgEdge* ptr = edge;
		dgVector p0 (m_vertex[edge->m_incidentVertex]);
		ptr = ptr->m_next;
		dgVector p1 (m_vertex[ptr->m_incidentVertex]);
		dgVector e1 (p1 - p0);
		dgVector n0 (dgFloat32 (0.0f));
		for (ptr = ptr->m_next; ptr != edge; ptr = ptr->m_next) {
			dgVector p2 (m_vertex[ptr->m_incidentVertex]);
			dgVector e2 (p2 - p0);
			n0 += e1.CrossProduct(e2);
			e1 = e2;
		} 

		dgAssert (n0.m_w == dgFloat32 (0.0f));
		ptr = edge;
		do {
			dgVector q0 (m_vertex[ptr->m_twin->m_incidentVertex]);
			for (dgEdge* neiborg = ptr->m_twin->m_next->m_next; neiborg != ptr->m_twin; neiborg = neiborg->m_next) { 
				dgVector q1 (m_vertex[neiborg->m_incidentVertex]);
				dgVector q1q0 (q1 - q0);
				dgFloat32 project = q1q0.DotProduct(n0).GetScalar();
				if (project > dgFloat32 (1.0e-5f)) {
					return false;
				}
			}

			ptr = ptr->m_next;
		} while (ptr != edge);
	}

	return true;
}



void dgCollisionConvex::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgInt8 mark[DG_MAX_EDGE_COUNT];
	dgVector tmp[DG_MAX_EDGE_COUNT];
	dgTriplex vertex[DG_MAX_EDGE_COUNT];

	matrix.TransformTriplex (&tmp[0].m_x, sizeof (dgVector), &m_vertex[0].m_x, sizeof (dgVector), m_vertexCount);

	memset (mark, 0, sizeof (mark));
	for (dgInt32 i = 0; i < m_edgeCount; i ++) {
		if (!mark[i]) {
			dgConvexSimplexEdge* const face = &m_simplex[i];
			dgConvexSimplexEdge* edge = face;
			dgInt32 count = 0;
			do {
				mark[edge - m_simplex] = '1';
				dgInt32 index = edge->m_vertex;
				vertex[count].m_x = tmp[index].m_x;
				vertex[count].m_y = tmp[index].m_y;
				vertex[count].m_z = tmp[index].m_z;
				count ++;
				edge = edge->m_next;
			} while (edge != face);
			callback (userData, count, &vertex[0].m_x, 0);
		}
	}
}

void dgCollisionConvex::CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const
{
	dgVector origin (matrix.TransformVector(m_boxOrigin));
	dgVector size (matrix.m_front.Abs().Scale(m_boxSize.m_x) + matrix.m_up.Abs().Scale(m_boxSize.m_y) + matrix.m_right.Abs().Scale(m_boxSize.m_z));

	p0 = (origin - size) & dgVector::m_triplexMask;
	p1 = (origin + size) & dgVector::m_triplexMask;
}



void dgCollisionConvex::CalculateInertia (void* userData, int indexCount, const dgFloat32* const faceVertex, int faceId)
{
	dgPolyhedraMassProperties& localData = *((dgPolyhedraMassProperties*) userData);
	localData.AddInertiaAndCrossFace(indexCount, faceVertex);
}


dgFloat32 dgCollisionConvex::CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const
{
	dgPolyhedraMassProperties localData;
	DebugCollision (offset, CalculateInertia, &localData);
	return localData.MassProperties (centerOfMass, inertia, crossInertia);
}


void dgCollisionConvex::MassProperties ()
{
	m_centerOfMass.m_w = dgCollisionConvex::CalculateMassProperties (dgGetIdentityMatrix(), m_inertia, m_crossInertia, m_centerOfMass);
	if (m_centerOfMass.m_w < DG_MAX_MIN_VOLUME) {
		m_centerOfMass.m_w = DG_MAX_MIN_VOLUME;
	}
	dgFloat32 invVolume = dgFloat32 (1.0f) / m_centerOfMass.m_w;
	m_inertia = m_inertia.Scale (invVolume);
	m_crossInertia = m_crossInertia.Scale (invVolume);
	m_centerOfMass = m_centerOfMass.Scale (invVolume);

	// complete the calculation 
	dgCollision::MassProperties ();
}

dgMatrix dgCollisionConvex::CalculateInertiaAndCenterOfMass (const dgMatrix& m_alignMatrix, const dgVector& localScale, const dgMatrix& matrix) const
{
	if ((dgAbs (localScale.m_x - localScale.m_y) < dgFloat32 (1.0e-5f)) && (dgAbs (localScale.m_x - localScale.m_z) < dgFloat32 (1.0e-5f))) {
		dgAssert (m_alignMatrix[0][0] == dgFloat32(1.0f));
		dgAssert (m_alignMatrix[1][1] == dgFloat32(1.0f));
		dgAssert (m_alignMatrix[2][2] == dgFloat32(1.0f));

		// using general central theorem, is much faster and more accurate;
		//IImatrix = IIorigin + mass * [(displacemnet % displacemnet) * identityMatrix - transpose(displacement) * displacement)];
		dgFloat32 mag2 = localScale.m_x * localScale.m_x;
		dgMatrix inertia (dgGetIdentityMatrix());
		inertia[0][0] = m_inertia[0] * mag2;
		inertia[1][1] = m_inertia[1] * mag2;
		inertia[2][2] = m_inertia[2] * mag2;
		inertia[0][1] = m_crossInertia[2] * mag2;
		inertia[1][0] = m_crossInertia[2] * mag2;
		inertia[0][2] = m_crossInertia[1] * mag2;
		inertia[2][0] = m_crossInertia[1] * mag2;
		inertia[1][2] = m_crossInertia[0] * mag2;
		inertia[2][1] = m_crossInertia[0] * mag2;
		inertia = matrix.Inverse() * inertia * matrix;
		
		dgAssert (localScale.m_w == dgFloat32 (0.0f));
		dgVector origin (matrix.TransformVector (m_centerOfMass * localScale));

		dgFloat32 originMag2 = origin.DotProduct(origin & dgVector::m_triplexMask).GetScalar();
		dgMatrix Covariance(origin, origin);
		dgMatrix parallel(dgGetIdentityMatrix());
		for (dgInt32 i = 0; i < 3; i++) {
			parallel[i][i] = originMag2;
			inertia[i] += (parallel[i] - Covariance[i]);
			dgAssert(inertia[i][i] > dgFloat32(0.0f));
		}

		inertia.m_posit = origin;
		inertia.m_posit.m_w = 1.0f;
		return inertia;
	} else {
		// for non uniform scale we need to the general divergence theorem
		dgVector inertiaII;
		dgVector crossInertia;
		dgVector centerOfMass;
		dgMatrix scaledMatrix(matrix);
		scaledMatrix[0] = scaledMatrix[0].Scale(localScale.m_x);
		scaledMatrix[1] = scaledMatrix[1].Scale(localScale.m_y);
		scaledMatrix[2] = scaledMatrix[2].Scale(localScale.m_z);
		scaledMatrix = m_alignMatrix * scaledMatrix;

		dgFloat32 volume = CalculateMassProperties (scaledMatrix, inertiaII, crossInertia, centerOfMass);
		if (volume < DG_MAX_MIN_VOLUME) {
			volume = DG_MAX_MIN_VOLUME;
		}

		dgFloat32 invVolume = dgFloat32 (1.0f) / volume;
		centerOfMass = centerOfMass.Scale(invVolume);
		inertiaII = inertiaII.Scale (invVolume);
		crossInertia = crossInertia.Scale (invVolume);
		dgMatrix inertia (dgGetIdentityMatrix());
		inertia[0][0] = inertiaII[0];
		inertia[1][1] = inertiaII[1];
		inertia[2][2] = inertiaII[2];
		inertia[0][1] = crossInertia[2];
		inertia[1][0] = crossInertia[2];
		inertia[0][2] = crossInertia[1];
		inertia[2][0] = crossInertia[1];
		inertia[1][2] = crossInertia[0];
		inertia[2][1] = crossInertia[0];
		inertia[3] = centerOfMass;
		return inertia;
	}
}


dgFloat32 dgCollisionConvex::GetVolume () const
{
	return m_centerOfMass.m_w;
}

dgFloat32 dgCollisionConvex::GetBoxMinRadius () const 
{
	return m_boxMinRadius;
} 

dgFloat32 dgCollisionConvex::GetBoxMaxRadius () const 
{
	return m_boxMaxRadius;
} 



dgVector dgCollisionConvex::CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& globalPlane, const dgCollisionInstance& parentScale) const
{
	dgPlane localPlane (globalMatrix.UntransformPlane (globalPlane));

	const dgVector& scale = parentScale.m_scale;
	switch (parentScale.m_scaleType)
	{
		case dgCollisionInstance::m_unit:
			break;

		case dgCollisionInstance::m_uniform:
		{
			localPlane.m_w *= parentScale.m_invScale.m_x;
			break;
		}

		case dgCollisionInstance::m_nonUniform:
		{
			localPlane = localPlane * (scale | dgVector::m_wOne);
			dgFloat32 mag2 = localPlane.DotProduct(localPlane & dgVector::m_triplexMask).GetScalar();
			localPlane = localPlane.Scale (dgRsqrt(mag2));
			break;
		}

		default:
		{
			localPlane = localPlane * (scale | dgVector::m_wOne);
			dgFloat32 mag2 = localPlane.DotProduct(localPlane & dgVector::m_triplexMask).GetScalar();
			localPlane = localPlane.Scale (dgRsqrt(mag2));
			localPlane = parentScale.m_aligmentMatrix.UntransformPlane (localPlane);
		}
	}

	dgVector cg (CalculateVolumeIntegral (localPlane));
	
	dgFloat32 volume = cg.m_w * scale.m_x * scale.m_y * scale.m_z;
	cg = parentScale.m_aligmentMatrix.RotateVector (cg);
	cg = cg * scale;
	cg = globalMatrix.TransformVector (cg);
	cg.m_w = volume;
	return cg;
}



dgVector dgCollisionConvex::CalculateVolumeIntegral (const dgPlane& plane) const 
{
	dgInt8 mark[DG_MAX_EDGE_COUNT];
	dgFloat32 test[DG_MAX_EDGE_COUNT];
	dgVector faceVertex[DG_MAX_EDGE_COUNT];

	dgInt32 positive = 0;
	dgInt32 negative = 0;
	for (dgInt32 i = 0; i < m_vertexCount; i ++) {
		test[i] = plane.Evalue (m_vertex[i]);
		if (test[i] > dgFloat32 (1.0e-5f)) {
			positive ++;
		} else if (test[i] < -dgFloat32 (1.0e-5f)) {
			negative ++;
		} else {
			test[i] = dgFloat32 (0.0f);
		}
	}

	if (positive == m_vertexCount) {
		return dgVector (dgFloat32 (0.0f));
	}

	if (negative == m_vertexCount) {
		//dgVector volume (this->GetVolume());
		//volume.m_w = m_simplexVolume;
		//return volume;
		return m_centerOfMass;
	}

	dgPolyhedraMassProperties localData;
	dgConvexSimplexEdge* capEdge = NULL;

	dgVector cg (dgFloat32 (0.0f)); 
	memset (mark, 0, sizeof (mark));
	for (dgInt32 i = 0; i < m_edgeCount; i ++) {
		if (!mark[i]) {
			dgConvexSimplexEdge* const face = &m_simplex[i];
			dgConvexSimplexEdge* edge = face;
			dgInt32 count = 0;
			dgFloat32 size0 = test[edge->m_prev->m_vertex];
			do {
				//edge->m_mark = m_mark;
				mark[edge - m_simplex] = '1';
				dgFloat32 size1 = test[edge->m_vertex];
				if (size0 <= dgFloat32 (0.0f)) {
					faceVertex[count] = m_vertex[edge->m_prev->m_vertex];
					count ++;
					if (size1 > dgFloat32 (0.0f)) {
						dgVector dp (m_vertex[edge->m_vertex] - m_vertex[edge->m_prev->m_vertex]);
						dgAssert (dp.m_w == dgFloat32 (0.0f));
						faceVertex[count] = m_vertex[edge->m_prev->m_vertex] - dp.Scale (size0 / dp.DotProduct(plane).GetScalar());
						count ++;
					}
				} else if (size1 < dgFloat32 (0.0f)) {
					dgVector dp (m_vertex[edge->m_vertex] - m_vertex[edge->m_prev->m_vertex]);
					dgAssert (dp.m_w == dgFloat32 (0.0f));
					faceVertex[count] = m_vertex[edge->m_prev->m_vertex] - dp.Scale (size0 / dp.DotProduct(plane).GetScalar());
					count ++;
					dgAssert (count < dgInt32 (sizeof (faceVertex) / sizeof (faceVertex[0])));
				}

				if (!capEdge) {
					if ((size1 > dgFloat32 (0.0f)) && (size0 < dgFloat32 (0.0f))) {
						capEdge = edge->m_prev->m_twin;
					}
				}

				size0 = size1;
				edge = edge->m_next;
			} while (edge != face);

			if (count) {
				localData.AddCGFace(count, faceVertex);
			}
		}
	}


	if (capEdge) {
		dgInt32 count = 0;
		dgConvexSimplexEdge* edge = capEdge;
		dgConvexSimplexEdge* ptr = NULL;
		do {
			dgVector dp (m_vertex[edge->m_twin->m_vertex] - m_vertex[edge->m_vertex]);
			dgAssert (dp.m_w == dgFloat32 (0.0f));
			faceVertex[count] = m_vertex[edge->m_vertex] - dp.Scale (test[edge->m_vertex] / dp.DotProduct(plane).GetScalar());
			count ++;
			if (count == 127) {
				// something is wrong return zero
				return dgVector (dgFloat32 (0.0f));
			}

			for (ptr = edge->m_next; ptr != edge; ptr = ptr->m_next) {
				dgInt32 index0 = ptr->m_twin->m_vertex;
				if (test[index0] > dgFloat32 (0.0f)) {
					index0 = ptr->m_vertex;
					if (test[index0] < dgFloat32 (0.0f)) {
						break;
					}
				}
			}
			edge = ptr->m_twin;
		} while (edge != capEdge);
		localData.AddCGFace(count, faceVertex);
	}

	dgVector inertia;
	dgVector crossInertia;
	dgFloat32 volume = localData.MassProperties (cg, inertia, crossInertia);
	cg = cg.Scale (dgFloat32 (1.0f) / dgMax (volume, dgFloat32 (1.0e-6f)));
	cg.m_w = volume;
	return cg; 
}

dgVector dgCollisionConvex::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (dir.m_w == dgFloat32 (0.0f));
	dgAssert (dgAbs(dir.DotProduct(dir).GetScalar() - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	dgInt16 cache[16];
	memset (cache, -1, sizeof (cache));
	dgConvexSimplexEdge* edge = &m_simplex[0];
	
	dgInt32 index = edge->m_vertex;
	dgFloat32 side0 = m_vertex[index].DotProduct(dir).GetScalar();

	cache [index & (sizeof (cache) / sizeof (cache[0]) - 1)] = dgInt16 (index);
	dgConvexSimplexEdge* ptr = edge;
	dgInt32 maxCount = 128;
	do {
		dgInt32 index1 = ptr->m_twin->m_vertex;
		if (cache [index1 & (sizeof (cache) / sizeof (cache[0]) - 1)] != index1) {
			cache [index1 & (sizeof (cache) / sizeof (cache[0]) - 1)] = dgInt16 (index1);
			dgFloat32 side1 = m_vertex[index1].DotProduct(dir).GetScalar();
			if (side1 > side0) {
				index = index1;
				side0 = side1;
				edge = ptr->m_twin;
				ptr = edge;
			}
		}
		ptr = ptr->m_twin->m_next;
		maxCount --;
	} while ((ptr != edge) && maxCount);
	dgAssert (maxCount);

	dgAssert (index != -1);
	return m_vertex[index];
}


bool dgCollisionConvex::SanityCheck(dgInt32 count, const dgVector& normal, dgVector* const contactsOut) const
{
	if (count > 1) {
		dgInt32 j = count - 1;
		for (dgInt32 i = 0; i < count; i ++) {
			dgVector error (contactsOut[i] - contactsOut[j]);
			dgAssert (error.m_w == dgFloat32 (0.0f));
			if (error.DotProduct(error).GetScalar() <= dgFloat32 (1.0e-20f)) {
				return false;
			} 
			j = i;
		}

		if (count >= 3) {
			dgVector n (dgFloat32 (0.0f));
			dgVector e0 (contactsOut[1] - contactsOut[0]);
			for (dgInt32 i = 2; i < count; i ++) {
				dgVector e1 (contactsOut[i] - contactsOut[0]);
				n += e0.CrossProduct(e1);
				e0 = e1;
			} 
			dgAssert (n.m_w == dgFloat32 (0.0f));
			dgAssert (n.DotProduct(n).GetScalar() > dgFloat32 (0.0f));
			n = n.Scale (dgRsqrt(n.DotProduct(n).GetScalar()));
			dgFloat32 projection;
			projection = n.DotProduct(normal).GetScalar();
			dgAssert (projection > dgFloat32 (0.9f));
			if (projection < dgFloat32 (0.9f)) {
				return false;
			}

			e0 = contactsOut[count-1] - contactsOut[count-2];
			j = count - 1;
			for (dgInt32 i = 0; i < count; i ++) {
				dgVector e1 (contactsOut[i] - contactsOut[j]);
				dgVector n1 (e0.CrossProduct(e1));
				dgAssert (n1.m_w == dgFloat32 (0.0f));
				dgFloat32 error = n1.DotProduct(normal).GetScalar();
				dgAssert (error >= dgFloat32 (-1.0e-4f));
				if (error < dgFloat32 (-1.0e-4f)) {
					return false;
				}
				j = i;
				e0 = e1;
			}
		}
	}
	return true;
}

dgInt32 dgCollisionConvex::SimplifyClipPolygon (dgInt32 count, const dgVector& normal, dgVector* const polygon) const
{
	dgInt8 mark[DG_MAX_VERTEX_CLIP_FACE * 8];
	dgInt8 buffer[8 * DG_MAX_VERTEX_CLIP_FACE * (sizeof (dgInt32) + sizeof (dgFloat32))];

	dgAssert (count < dgInt32 (sizeof (mark) / sizeof (mark[0])));
	dgUpHeap<dgInt32, dgFloat32> sortHeap (buffer, sizeof (buffer));	

	dgAssert (normal.m_w == dgFloat32 (0.0f));
	while (count > DG_MAX_VERTEX_CLIP_FACE) {
		sortHeap.Flush();

		//dgInt32 i0 = count - 2;
		//dgInt32 i1 = count - 1;
		for (dgInt32 i0 = count - 2, i1 = count - 1, i2 = 0; i2 < count; i2 ++) {
			mark[i2] = 0;

			dgVector e0 = polygon[i1] - polygon[i0];
			dgVector e1 = polygon[i2] - polygon[i0];
			dgFloat32 area = dgAbs (normal.DotProduct(e0.CrossProduct(e1)).GetScalar());

			sortHeap.Push(i1, area);

			i0 = i1;
			i1 = i2;
		}

		dgInt32 removeCount = count - DG_MAX_VERTEX_CLIP_FACE;
		while (sortHeap.GetCount() && removeCount) {
			dgInt32 m1 = sortHeap[0];
			sortHeap.Pop();

			dgInt32 m0 = (m1 - 1) >= 0 ? m1 - 1 : count - 1;
			dgInt32 m2 = (m1 + 1) < count ? m1 + 1 : 0;
			dgAssert (m0 >= 0);

			if (!(mark[m0] || mark[m2])) {
				mark[m1] = 1;
				removeCount --;
			}
		}

		dgInt32 i0 = 0;
		for (dgInt32 i1 = 0; i1 < count; i1 ++) {
			if (!mark[i1]) {
				polygon[i0] = polygon[i1];
				i0 ++;
			}
		}
		count = i0;
	}

	return count;
}


dgInt32 dgCollisionConvex::RectifyConvexSlice (dgInt32 count, const dgVector& normal, dgVector* const contactsOut) const
{
	struct DG_CONVEX_FIXUP_FACE
	{
		dgInt32 m_vertex;
		DG_CONVEX_FIXUP_FACE* m_next;
	};

	DG_CONVEX_FIXUP_FACE linkFace[DG_CLIP_MAX_POINT_COUNT * 2];

	dgAssert (count > 2);
	dgAssert (normal.m_w == dgFloat32 (0.0f));
	DG_CONVEX_FIXUP_FACE* poly = &linkFace[0];
	for (dgInt32 i = 0; i < count; i ++) {
		dgAssert (contactsOut[i].m_w == dgFloat32 (0.0f));
		linkFace[i].m_vertex = i;
		linkFace[i].m_next = &linkFace[i + 1];
	}
	linkFace[count - 1].m_next = &linkFace[0];

	dgInt32 restart = 1;
	dgInt32 tmpCount = count;
	while (restart && (tmpCount >= 2)) {
		restart = 0;
		DG_CONVEX_FIXUP_FACE* ptr = poly; 
		dgInt32 loops = tmpCount;
		do {
			dgInt32 i0 = ptr->m_vertex;
			dgInt32 i1 = ptr->m_next->m_vertex;
			dgVector error (contactsOut[i1] - contactsOut[i0]);
			dgAssert (error.m_w == dgFloat32 (0.0f));
			dgFloat32 dist2 = error.DotProduct(error).GetScalar();
			if (dist2 < dgFloat32 (0.003f * 0.003f)) {
				if (ptr->m_next == poly) {
					poly = ptr;
				} 
				restart = 1;
				tmpCount --;
				contactsOut[i1].m_w = dgFloat32 (1.0f);
				ptr->m_next = ptr->m_next->m_next;
			} else {
				ptr = ptr->m_next;
			}

			loops --;
		} while (loops);
	}

	restart = 1;
	while (restart && (tmpCount >= 3)) {
		restart = 0;
		DG_CONVEX_FIXUP_FACE* ptr = poly;
		dgInt32 loops = tmpCount;
		do {
			dgInt32 i0 = ptr->m_vertex;
			dgInt32 i1 = ptr->m_next->m_vertex;
			dgInt32 i2 = ptr->m_next->m_next->m_vertex;
			dgVector e0 (contactsOut[i2] - contactsOut[i1]);
			dgVector e1 (contactsOut[i0] - contactsOut[i1]);
			dgVector n (e0.CrossProduct(e1));
			dgFloat32 area = normal.DotProduct(n).GetScalar();
			if (area <= dgFloat32 (1.0e-5f)) {
				if (ptr->m_next == poly) {
					poly = ptr;
				}
				restart = 1;
				tmpCount --;
				contactsOut[i1].m_w = dgFloat32 (1.0f);
				ptr->m_next = ptr->m_next->m_next;
			} else {
				ptr = ptr->m_next;
			}
			loops --;
		} while (loops);
	}

	if (tmpCount < count) {
		dgInt32 newCount = 0;
		for (; newCount < count; newCount ++) {
			if (contactsOut[newCount].m_w == dgFloat32 (1.0f)) {
				break;
			}
		}

		for (dgInt32 i = newCount + 1; i < count; i ++) {
			if (contactsOut[i].m_w == dgFloat32 (0.0f)) {
				contactsOut[newCount] = contactsOut[i];
				newCount ++;
			}
		}
		count = newCount;
		dgAssert (tmpCount == count);
	}


	if (count > DG_MAX_VERTEX_CLIP_FACE) {
		count = SimplifyClipPolygon (count, normal, contactsOut);
	}

	dgAssert (SanityCheck(count, normal, contactsOut));
	return count;
}

dgVector dgCollisionConvex::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const 
{
	return SupportVertex(dir, vertexIndex);
}

dgVector dgCollisionConvex::SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const
 {
	return point;
 }


dgFloat32 dgCollisionConvex::RayCast(const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	dgCollisionInstance instance(*body->GetCollision(), this);
	dgContactSolver rayCaster(&instance);
	instance.m_material.m_userData = NULL;
	return rayCaster.RayCast(localP0, localP1, maxT, contactOut);
}

dgInt32 dgCollisionConvex::CalculatePlaneIntersection (const dgVector& normal, const dgVector& origin, dgVector* const contactsOut) const
{
	dgVector support[4];
	dgInt32 featureCount = 3;
	const dgConvexSimplexEdge* edge = &m_simplex[0];
	const dgConvexSimplexEdge** const vertToEdgeMapping = GetVertexToEdgeMapping();
	dgAssert (normal.m_w == dgFloat32 (0.0f));
	if (vertToEdgeMapping) {
		dgInt32 edgeIndex;
		featureCount = 1;
		support[0] = SupportVertex (normal, &edgeIndex);
		edge = vertToEdgeMapping[edgeIndex];

		// 5 degrees
		const dgFloat32 tiltAngle = dgFloat32 (0.087f);
		const dgFloat32 tiltAngle2 = tiltAngle * tiltAngle ;
		dgPlane testPlane (normal, - (normal.DotProduct(support[0]).GetScalar()));
		const dgConvexSimplexEdge* ptr = edge;
		do {
			const dgVector& p = m_vertex[ptr->m_twin->m_vertex];
			dgFloat32 test = testPlane.Evalue(p);
			dgVector dist (p - support[0]);
			dgAssert (dist.m_w == dgFloat32 (0.0f));
			dgFloat32 angle2 = test * test / (dist.DotProduct(dist).GetScalar());

			if (angle2 < tiltAngle2) {
				support[featureCount] = p;
				featureCount ++;
			}
			ptr = ptr->m_twin->m_next;
		} while ((ptr != edge) && (featureCount < 3));
	}

	dgInt32 count = 0;
	dgPlane plane (normal, - normal.DotProduct(origin).GetScalar());
	switch (featureCount)
	{
		case 1:
			contactsOut[0] = support[0] - normal * normal.DotProduct(support[0] - origin);
			count = 1;
			break;

		case 2:
			contactsOut[0] = support[0] - normal * normal.DotProduct(support[0] - origin);
			contactsOut[1] = support[1] - normal * normal.DotProduct(support[1] - origin);
			count = 2;
			break;

		default:
		{
			dgFloat32 side0 = plane.Evalue(m_vertex[edge->m_vertex]);
			dgFloat32 side1 = side0;
			const dgConvexSimplexEdge* firstEdge = NULL;
			if (side0 > dgFloat32 (0.0f)) {
				const dgConvexSimplexEdge* ptr = edge;
				do {
					dgAssert (m_vertex[ptr->m_twin->m_vertex].m_w == dgFloat32 (0.0f));
					side1 = plane.Evalue (m_vertex[ptr->m_twin->m_vertex]);
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


				if (!firstEdge) {
					// we may have a local minimal in the convex hull do to a big flat face
					for (dgInt32 i = 0; i < m_edgeCount; i ++) {
						ptr = &m_simplex[i];
						side0 = plane.Evalue (m_vertex[ptr->m_vertex]);
						side1 = plane.Evalue (m_vertex[ptr->m_twin->m_vertex]);
						if ((side1 < dgFloat32 (0.0f)) && (side0 > dgFloat32 (0.0f))){
							firstEdge = ptr;
							break;
						}
					}
				}

			} else if (side0 < dgFloat32 (0.0f)) {
				const dgConvexSimplexEdge* ptr = edge;
				do {
					dgAssert (m_vertex[ptr->m_twin->m_vertex].m_w == dgFloat32 (0.0f));
					side1 = plane.Evalue (m_vertex[ptr->m_twin->m_vertex]);
					if (side1 > side0) {
						if (side1 >= dgFloat32 (0.0f)) {
							side0 = side1;
							firstEdge = ptr->m_twin;
							break;
						}

						side0 = side1;
						edge = ptr->m_twin;
						ptr = edge;
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				if (!firstEdge) {
					// we may have a local minimal in the convex hull due to a big flat face
					for (dgInt32 i = 0; i < m_edgeCount; i ++) {
						ptr = &m_simplex[i];
						side0 = plane.Evalue (m_vertex[ptr->m_vertex]);
						//dgFloat32 side1 = plane.Evalue (m_vertex[ptr->m_twin->m_vertex]);
						side1 = plane.Evalue(m_vertex[ptr->m_twin->m_vertex]);
						if ((side1 < dgFloat32 (0.0f)) && (side0 > dgFloat32 (0.0f))){
							firstEdge = ptr;
							break;
						}
					}
				}
			}

			if (firstEdge) {
				dgAssert (side0 >= dgFloat32 (0.0f));
				dgAssert ((side1 = plane.Evalue (m_vertex[firstEdge->m_vertex])) >= dgFloat32 (0.0f));
				dgAssert ((side1 = plane.Evalue (m_vertex[firstEdge->m_twin->m_vertex])) < dgFloat32 (0.0f));
				dgAssert (dgAbs (side0 - plane.Evalue (m_vertex[firstEdge->m_vertex])) < dgFloat32 (1.0e-5f));

				dgInt32 maxCount = 0;
				const dgConvexSimplexEdge* ptr = firstEdge;
				do {
					if (side0 > dgFloat32 (0.0f)) {
						dgAssert (plane.Evalue (m_vertex[ptr->m_vertex]) > dgFloat32 (0.0f));
						dgAssert (plane.Evalue (m_vertex[ptr->m_twin->m_vertex]) < dgFloat32 (0.0f));

						dgVector dp (m_vertex[ptr->m_twin->m_vertex] - m_vertex[ptr->m_vertex]);
						dgAssert (dp.m_w == dgFloat32 (0.0f));
						dgFloat32 t = plane.DotProduct(dp).GetScalar();
						if (t >= dgFloat32 (-1.e-24f)) {
							t = dgFloat32 (0.0f);
						} else {
							t = side0 / t;
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

						dgConvexSimplexEdge* ptr1 = ptr->m_next;
						for (; ptr1 != ptr; ptr1 = ptr1->m_next) {
							dgAssert (m_vertex[ptr->m_twin->m_vertex].m_w == dgFloat32 (0.0f));
							side0 = plane.Evalue (m_vertex[ptr1->m_twin->m_vertex]); 
							if (side0 >= dgFloat32 (0.0f)) {
								break;
							}
						}
						dgAssert (ptr1 != ptr);
						ptr = ptr1->m_twin;
					} else {
						contactsOut[count] = m_vertex[ptr->m_vertex];
						dgConvexSimplexEdge* ptr1 = ptr->m_next;
						for (; ptr1 != ptr; ptr1 = ptr1->m_next) {
							dgAssert (m_vertex[ptr1->m_twin->m_vertex].m_w == dgFloat32 (0.0f));
							side0 = plane.Evalue (m_vertex[ptr1->m_twin->m_vertex]); 
							if (side0 >= dgFloat32 (0.0f)) {
								break;
							}
						}

						if (ptr1 == ptr) {
							ptr = ptr1->m_prev->m_twin;
						} else {
							ptr = ptr1->m_twin;
						}
					}

					count ++;
					maxCount ++;
					if (count >= DG_CLIP_MAX_POINT_COUNT) {
						for (count = 0; count < (DG_CLIP_MAX_POINT_COUNT >> 1); count ++) {
							contactsOut[count] = contactsOut[count * 2];
						}
					}

				} while ((ptr != firstEdge) && (maxCount < DG_CLIP_MAX_COUNT));
				dgAssert (maxCount < DG_CLIP_MAX_COUNT);

				if (count > 2) {
					count = RectifyConvexSlice (count, normal, contactsOut);
				}
			}
		}
	}
	return count;
}

