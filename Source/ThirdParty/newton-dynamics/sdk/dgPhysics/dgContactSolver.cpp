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


dgVector dgContactSolver::m_hullDirs[] =
{
	dgVector(dgFloat32(0.577350f), dgFloat32(-0.577350f), dgFloat32(0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(-0.577350f), dgFloat32(-0.577350f), dgFloat32(-0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.577350f), dgFloat32(-0.577350f), dgFloat32(-0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(-0.577350f), dgFloat32(0.577350f), dgFloat32(0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.577350f), dgFloat32(0.577350f), dgFloat32(-0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(-0.577350f), dgFloat32(0.577350f), dgFloat32(-0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(-0.577350f), dgFloat32(-0.577350f), dgFloat32(0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.577350f), dgFloat32(0.577350f), dgFloat32(0.577350f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.000000f), dgFloat32(-1.000000f), dgFloat32(0.000000f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.000000f), dgFloat32(1.000000f), dgFloat32(0.000000f), dgFloat32(0.0f)),
	dgVector(dgFloat32(1.000000f), dgFloat32(0.000000f), dgFloat32(0.000000f), dgFloat32(0.0f)),
	dgVector(dgFloat32(-1.000000f), dgFloat32(0.000000f), dgFloat32(0.000000f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.000000f), dgFloat32(0.000000f), dgFloat32(1.000000f), dgFloat32(0.0f)),
	dgVector(dgFloat32(0.000000f), dgFloat32(0.000000f), dgFloat32(-1.000000f), dgFloat32(0.0f)),
};

dgInt32 dgContactSolver::m_rayCastSimplex[4][4] =
{
	{ 0, 1, 2, 3 },
	{ 0, 2, 3, 1 },
	{ 2, 1, 3, 0 },
	{ 1, 0, 3, 2 },
};

dgContactSolver::dgContactSolver(dgCollisionInstance* const instance)
	:dgDownHeap<dgMinkFace*, dgFloat32>(m_heapBuffer, sizeof (m_heapBuffer))
	,m_proxy (NULL)
	,m_instance0(instance)
	,m_instance1(instance)
	,m_vertexIndex(0)
{
}

dgContactSolver::dgContactSolver(dgCollisionParamProxy* const proxy)
	:dgDownHeap<dgMinkFace*, dgFloat32>(m_heapBuffer, sizeof (m_heapBuffer))
	,m_normal (proxy->m_contactJoint->m_separtingVector)
	,m_proxy (proxy)
	,m_instance0(proxy->m_instance0)
	,m_instance1(proxy->m_instance1)
	,m_vertexIndex(0)
{
}

// for ray Cast

DG_INLINE void dgContactSolver::SupportVertex(const dgVector& dir0, dgInt32 vertexIndex)
{
	dgAssert(dir0.m_w == dgFloat32(0.0f));
	dgAssert(dgAbs(dir0.DotProduct(dir0).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));
	dgVector dir1 (dir0.Scale(dgFloat32 (-1.0f)));

	const dgMatrix& matrix0 = m_instance0->m_globalMatrix;
	const dgMatrix& matrix1 = m_instance1->m_globalMatrix;
	dgVector p(matrix0.TransformVector(m_instance0->SupportVertexSpecial(matrix0.UnrotateVector (dir0), NULL)) & dgVector::m_triplexMask);
	dgVector q(matrix1.TransformVector(m_instance1->SupportVertexSpecial(matrix1.UnrotateVector (dir1), NULL)) & dgVector::m_triplexMask);
	m_hullDiff[vertexIndex] = p - q;
	m_hullSum[vertexIndex] = p + q;
}


DG_INLINE dgBigVector dgContactSolver::ReduceLine(dgInt32& indexOut)
{
	const dgBigVector p0(m_hullDiff[0]);
	const dgBigVector p1(m_hullDiff[1]);
	const dgBigVector dp(p1 - p0);
	dgBigVector v;

	const dgFloat64 mag2 = dp.DotProduct(dp).GetScalar();
	dgAssert (mag2 > dgFloat64 (0.0f));
	if (mag2 < dgFloat32(1.0e-24f)) {
		v = p0;
		indexOut = 1;
	} else {
		const dgFloat64 alpha0 = - p0.DotProduct(dp).GetScalar();
		if (alpha0 > mag2) {
			v = p1;
			indexOut = 1;
			m_hullSum[0] = m_hullSum[1];
			m_hullDiff[0] = m_hullDiff[1];
		} else if (alpha0 < dgFloat64(0.0f)) {
			v = p0;
			indexOut = 1;
		} else {
			v = p0 + dp.Scale(alpha0 / mag2);
		}
	}
	return v;
}

DG_INLINE dgBigVector dgContactSolver::ReduceTriangle (dgInt32& indexOut)
{
	const dgBigVector p0(m_hullDiff[0]);
	const dgBigVector p1(m_hullDiff[1]);
	const dgBigVector p2(m_hullDiff[2]);
	const dgBigVector e10 (p1 - p0);
	const dgBigVector e20 (p2 - p0);
	const dgFloat64 a00 = e10.DotProduct(e10).GetScalar();
	const dgFloat64 a11 = e20.DotProduct(e20).GetScalar();
	const dgFloat64 a01 = e10.DotProduct(e20).GetScalar();

	const dgFloat64 det = a00 * a11 - a01 * a01;
	dgAssert(det >= dgFloat32(0.0f));
	if (dgAbs(det) > dgFloat32(1.0e-24f)) {
		const dgFloat64 b0 = -e10.DotProduct(p0).GetScalar();
		const dgFloat64 b1 = -e20.DotProduct(p0).GetScalar();

		const dgFloat64 u2 = b1 * a00 - a01 * b0;
		const dgFloat64 u1 = b0 * a11 - a01 * b1;

		if (u2 < dgFloat32(0.0f)) {
			// this looks funny but it is correct
		} else if (u1 < dgFloat32(0.0f)) {
			m_hullSum[1] = m_hullSum[2];
			m_hullDiff[1] = m_hullDiff[2];
		} else if ((u1 + u2) > det) {
			m_hullSum[0] = m_hullSum[2];
			m_hullDiff[0] = m_hullDiff[2];
		} else {
			return p0 + (e10.Scale(u1) + e20.Scale(u2)).Scale(dgFloat64(1.0f) / det);
		}
		indexOut = 2;
		return ReduceLine(indexOut);
	} 
	// this is a degenerated triangle. this should never happens
	dgAssert(0);
	return dgBigVector::m_zero;
}

DG_INLINE dgBigVector dgContactSolver::ReduceTetrahedrum (dgInt32& indexOut)
{
	const dgBigVector p0(m_hullDiff[0]);
	const dgBigVector p1(m_hullDiff[1]);
	const dgBigVector p2(m_hullDiff[2]);
	const dgBigVector p3(m_hullDiff[3]);
	const dgBigVector e10(p1 - p0);
	const dgBigVector e20(p2 - p0);
	const dgBigVector e30(p3 - p0);

	const dgFloat64 d0 = sqrt (e10.DotProduct(e10).GetScalar());
	if (d0 > dgFloat64(0.0f)) {
		const dgFloat64 invd0 = dgFloat64(1.0f) / d0;
		const dgFloat64 l10 = e20.DotProduct(e10).GetScalar() * invd0;
		const dgFloat64 l20 = e30.DotProduct(e10).GetScalar() * invd0;
		const dgFloat64 desc11 = e20.DotProduct(e20).GetScalar() - l10 * l10;
		if (desc11 > dgFloat64(0.0f)) {
			const dgFloat64 d1 = sqrt(desc11);
			const dgFloat64 invd1 = dgFloat64(1.0f) / d1;
			const dgFloat64 l21 = (e30.DotProduct(e20).GetScalar() - l20 * l10) * invd1;
			const dgFloat64 desc22 = e30.DotProduct(e30).GetScalar() - l20 * l20 - l21 * l21;
			if (desc22 > dgFloat64(0.0f)) {
				const dgFloat64 d2 = sqrt(desc22);
				const dgFloat64 invd2 = dgFloat64(1.0f) / d2;
				const dgFloat64 b0 = -e10.DotProduct(p0).GetScalar();
				const dgFloat64 b1 = -e20.DotProduct(p0).GetScalar();
				const dgFloat64 b2 = -e30.DotProduct(p0).GetScalar();

				dgFloat64 u1 = b0 * invd0;
				dgFloat64 u2 = (b1 - l10 * u1) * invd1;
				dgFloat64 u3 = (b2 - l20 * u1 - l21 * u2) * invd2 * invd2;
				u2 = (u2 - l21 * u3) * invd1;
				u1 = (u1 - l10 * u2 - l20 * u3) * invd0;
				if (u3 < dgFloat64(0.0f)) {
					// this looks funny but it is correct
				} else if (u2 < dgFloat64(0.0f)) {
					m_hullSum[2] = m_hullSum[3];
					m_hullDiff[2] = m_hullDiff[3];
				} else if (u1 < dgFloat64(0.0f)) {
					m_hullSum[1] = m_hullSum[3];
					m_hullDiff[1] = m_hullDiff[3];
				} else if (u1 + u2 + u3 > dgFloat64(1.0f)) {
					m_hullSum[0] = m_hullSum[3];
					m_hullDiff[0] = m_hullDiff[3];
				} else {
					return dgBigVector::m_zero;
				}
				indexOut = 3;
				return ReduceTriangle(indexOut);
			}
		}
	}
	// this is a degenerated tetra. this should never happens.
	// it seems this does happens about once per several millions calls, 
	// I will assume is acceptable. No fall back needed
	//dgAssert (0);
	return dgBigVector::m_zero;
}


//DG_INLINE dgInt32 dgContactSolver::CalculateClosestSimplex ()
dgInt32 dgContactSolver::CalculateClosestSimplex()
{
	dgBigVector v(dgFloat32 (0.0f));
	dgInt32 index = 1;
	if (m_vertexIndex <= 0) {
		SupportVertex (m_proxy->m_contactJoint->m_separtingVector, 0);
		v = m_hullDiff[0];
	} else {
		switch (m_vertexIndex) 
		{
			case 1:
			{
				v = m_hullDiff[0];
				break;
			}

			case 2:
			{
				v = ReduceLine (m_vertexIndex);
				break;
			}

			case 3:
			{
				v = ReduceTriangle (m_vertexIndex);
				break;
			}

			case 4:
			{
				v = ReduceTetrahedrum (m_vertexIndex);
				break;
			}
		}
		index = m_vertexIndex;
	}

	dgInt32 iter = 0;
	dgInt32 cycling = 0;
	dgFloat64 minDist = dgFloat32 (1.0e20f);
	do {
		dgFloat64 dist = v.DotProduct(v).GetScalar();
		if (dist < dgFloat32 (1.0e-9f)) {
			// very deep penetration, resolve with generic minkowsky solver
			return -index; 
		}

		if (dist < minDist) {
			minDist = dist;
			cycling = -1;
		}
		cycling ++;
		if (cycling > 4) {
			// for now return -1
			return -index;
		}

		const dgVector dir (v.Scale (-dgRsqrt(dist)));
		dgAssert (dir.m_w == dgFloat32 (0.0f));
		SupportVertex (dir, index);

		const dgBigVector w (m_hullDiff[index]);
		const dgVector wv (w - v);
		dgAssert (wv.m_w == dgFloat32 (0.0f));
		const dgFloat64 dist1 = dir.DotProduct(wv).GetScalar();
		if (dist1 < dgFloat64 (1.0e-3f)) {
			m_normal = dir;
			break;
		}

		index ++;
		switch (index) 
		{
			case 2:
			{
				v = ReduceLine (index);
				break;
			}

			case 3:
			{
				v = ReduceTriangle (index);
				break;
			}

			case 4:
			{
				v = ReduceTetrahedrum (index);
				break;
			}
		}

		iter ++;
	} while (iter < DG_CONNICS_CONTATS_ITERATIONS); 
	return (index < 4) ? index : -4;
}


DG_INLINE dgMinkFace* dgContactSolver::NewFace()
{
	dgMinkFace* face = (dgMinkFace*)m_freeFace;
	if (m_freeFace) {
		m_freeFace = m_freeFace->m_next;
	} else {
		face = &m_facePool[m_faceIndex];
		m_faceIndex++;
		dgAssert(m_faceIndex < DG_CONVEX_MINK_MAX_FACES);
	}

#ifdef _DEBUG
	memset(face, 0, sizeof (dgMinkFace));
#endif
	return face;
}

DG_INLINE dgMinkFace* dgContactSolver::AddFace(dgInt32 v0, dgInt32 v1, dgInt32 v2)
{
	dgMinkFace* const face = NewFace();
	face->m_mark = 0;
	face->m_vertex[0] = dgInt16(v0);
	face->m_vertex[1] = dgInt16(v1);
	face->m_vertex[2] = dgInt16(v2);
	return face;
}

DG_INLINE void dgContactSolver::DeleteFace(dgMinkFace* const face)
{
	dgFaceFreeList* const freeFace = (dgFaceFreeList*)face;
	freeFace->m_next = m_freeFace;
	m_freeFace = freeFace;
}


DG_INLINE void dgContactSolver::PushFace(dgMinkFace* const face)
{
	dgInt32 i0 = face->m_vertex[0];
	dgInt32 i1 = face->m_vertex[1];
	dgInt32 i2 = face->m_vertex[2];

	dgPlane plane(m_hullDiff[i0], m_hullDiff[i1], m_hullDiff[i2]);
	dgFloat32 mag2 = plane.DotProduct(plane & dgVector::m_triplexMask).GetScalar();
	face->m_alive = 1;
	if (mag2 > dgFloat32(1.0e-16f)) {
		face->m_plane = plane.Scale(dgRsqrt(mag2));
		dgMinkFace* face1 = face;
		Push(face1, face->m_plane.m_w);
	} else {
		face->m_plane = dgPlane(dgVector::m_zero);
	}
}

dgInt32 dgContactSolver::CalculateIntersectingPlane(dgInt32 count)
{
	dgAssert(count >= 1);
	if (count == 1) {
		SupportVertex(m_proxy->m_contactJoint->m_separtingVector.Scale(dgFloat32(-1.0f)), 1);
		dgVector err(m_hullDiff[1] - m_hullDiff[0]);
		dgAssert (err.m_w == dgFloat32 (0.0f));
		if (err.DotProduct(err).GetScalar() < dgFloat32(1.0e-8f)) {
			return -1;
		}
		count = 2;
	}
	
	if (count == 2) {
		dgVector e0(m_hullDiff[1] - m_hullDiff[0]);
		dgAssert (e0.m_w == dgFloat32 (0.0f));
		dgAssert(e0.DotProduct(e0).GetScalar() > dgFloat32(0.0f));
		dgMatrix matrix(e0.Scale(dgRsqrt(e0.DotProduct(e0).GetScalar())));
		dgMatrix rotation(dgPitchMatrix(dgFloat32(45.0f * dgDEG2RAD)));
		dgFloat32 maxArea = dgFloat32(0.0f);
		for (dgInt32 i = 0; i < 8; i++) {
			SupportVertex(matrix[1], 3);
			dgVector e1(m_hullDiff[3] - m_hullDiff[0]);
			dgAssert (e1.m_w == dgFloat32 (0.0f));
			dgVector area(e0.CrossProduct(e1));
			dgFloat32 area2 = area.DotProduct(area).GetScalar();
			if (area2 > maxArea) {
				m_hullSum[2] = m_hullSum[3];
				m_hullDiff[2] = m_hullDiff[3];
				maxArea = area2;
			}
			matrix = rotation * matrix;
		}
		if (dgAbs (maxArea) < dgFloat32(1e-15f)) {
			return -1;
		}
		dgAssert(maxArea > dgFloat32(0.0f));
		count++;
	}

	dgFloat32 volume = dgFloat32(0.0f);
	if (count == 3) {
		dgVector e10(m_hullDiff[1] - m_hullDiff[0]);
		dgVector e20(m_hullDiff[2] - m_hullDiff[0]);
		dgVector normal(e10.CrossProduct(e20));
		dgAssert (normal.m_w == dgFloat32 (0.0f));
		dgFloat32 mag2 = normal.DotProduct(normal).GetScalar();
		dgAssert(mag2 > dgFloat32(0.0f));
		normal = normal.Scale(dgRsqrt(mag2));
		SupportVertex(normal, 3);
		volume = normal.DotProduct(m_hullDiff[3] - m_hullDiff[0]).GetScalar();
		if (dgAbs(volume) < dgFloat32(1.0e-10f)) {
			normal = normal.Scale(dgFloat32(-1.0f));
			SupportVertex(normal, 3);
			volume = - normal.DotProduct(m_hullDiff[3] - m_hullDiff[0]).GetScalar();
			if (dgAbs(volume) < dgFloat32(1.0e-10f)) {
				volume = dgFloat32(0.0f);
			}
		}
		count = 4;
	} else if (count == 4) {
		dgVector e0(m_hullDiff[1] - m_hullDiff[0]);
		dgVector e1(m_hullDiff[2] - m_hullDiff[0]);
		dgVector e2(m_hullDiff[3] - m_hullDiff[0]);
		dgVector n(e1.CrossProduct(e2));
		dgAssert (n.m_w == dgFloat32 (0.0f));
		volume = e0.DotProduct(n).GetScalar();
	}


	dgAssert(count == 4);
	if (volume > dgFloat32(0.0f)) {
		dgSwap(m_hullSum[1], m_hullSum[0]);
		dgSwap(m_hullDiff[1], m_hullDiff[0]);
	}

	if (dgAbs(volume) < dgFloat32(1e-15f)) {

		// this volume is unrealizable, let us build  a different tetrahedron using the method of core 200
		dgVector e1;
		dgVector e2;
		dgVector e3;
		dgVector normal(dgFloat32(0.0f));

		const dgInt32 nCount = dgInt32(sizeof(m_hullDirs) / sizeof(m_hullDirs[0]));
		const dgFloat32 DG_CALCULATE_SEPARATING_PLANE_ERROR = dgFloat32(1.0f / 1024.0f);

		dgFloat32 error2 = dgFloat32(0.0f);
		SupportVertex(m_hullDirs[0], 0);

		dgInt32 i = 1;
		for (; i < nCount; i++) {
			SupportVertex(m_hullDirs[i], 1);
			e1 = m_hullDiff[1] - m_hullDiff[0];
			dgAssert (e1.m_w == dgFloat32 (0.0f));
			error2 = e1.DotProduct(e1).GetScalar();
			if (error2 > DG_CALCULATE_SEPARATING_PLANE_ERROR) {
				break;
			}
		}

		for (i++; i < nCount; i++) {
			SupportVertex(m_hullDirs[i], 2);
			e2 = m_hullDiff[2] - m_hullDiff[0];
			normal = e1.CrossProduct(e2);
			dgAssert (normal.m_w == dgFloat32 (0.0f));
			error2 = normal.DotProduct(normal).GetScalar();
			if (error2 > DG_CALCULATE_SEPARATING_PLANE_ERROR) {
				break;
			}
		}

		error2 = dgFloat32(0.0f);
		for (i++; i < nCount; i++) {
			SupportVertex(m_hullDirs[i], 3);
			e3 = m_hullDiff[3] - m_hullDiff[0];
			dgAssert (normal.m_w == dgFloat32 (0.0f));
			error2 = normal.DotProduct(e3).GetScalar();
			if (dgAbs(error2) > DG_CALCULATE_SEPARATING_PLANE_ERROR) {
				break;
			}
		}

		if (i >= nCount) {
//			dgAssert(0);
			return -1;
		}

		if (error2 > dgFloat32(0.0f)) {
			dgSwap(m_hullSum[1], m_hullSum[2]);
			dgSwap(m_hullDiff[1], m_hullDiff[2]);
		}

#ifdef _DEBUG
		{
			dgVector f0(m_hullDiff[1] - m_hullDiff[0]);
			dgVector f1(m_hullDiff[2] - m_hullDiff[0]);
			dgVector f2(m_hullDiff[3] - m_hullDiff[0]);
			dgVector n(f1.CrossProduct(f2));
			dgAssert (n.m_w == dgFloat32 (0.0f));
			dgFloat32 volume1 = f0.DotProduct(n).GetScalar();
			dgAssert(volume1 < dgFloat32(0.0f));
		}
#endif
	}

	// clear the face cache!!
	Flush();
	m_faceIndex = 0;
	m_vertexIndex = 4;
	m_freeFace = NULL;

	dgMinkFace* const f0 = AddFace(0, 1, 2);
	dgMinkFace* const f1 = AddFace(0, 2, 3);
	dgMinkFace* const f2 = AddFace(2, 1, 3);
	dgMinkFace* const f3 = AddFace(1, 0, 3);

	f0->m_twin[0] = f3;
	f0->m_twin[1] = f2;
	f0->m_twin[2] = f1;

	f1->m_twin[0] = f0;
	f1->m_twin[1] = f2;
	f1->m_twin[2] = f3;

	f2->m_twin[0] = f0;
	f2->m_twin[1] = f3;
	f2->m_twin[2] = f1;

	f3->m_twin[0] = f0;
	f3->m_twin[1] = f1;
	f3->m_twin[2] = f2;

	PushFace(f0);
	PushFace(f1);
	PushFace(f2);
	PushFace(f3);

	dgInt32 cycling = 0;
	dgInt32 iterCount = 0;
	dgFloat32 cyclingMem[4];
	cyclingMem[0] = dgFloat32(1.0e10f);
	cyclingMem[1] = dgFloat32(1.0e10f);
	cyclingMem[2] = dgFloat32(1.0e10f);
	cyclingMem[3] = dgFloat32(1.0e10f);

	const dgFloat32 resolutionScale = dgFloat32(0.125f);
	const dgFloat32 minTolerance = DG_PENETRATION_TOL;

	while (GetCount()) {
		dgMinkFace* const faceNode = (*this)[0];
		Pop();

		if (faceNode->m_alive) {
			SupportVertex(faceNode->m_plane & dgVector::m_triplexMask, m_vertexIndex);
			const dgVector& p = m_hullDiff[m_vertexIndex];
			dgFloat32 dist = faceNode->m_plane.Evalue(p);
			dgFloat32 distTolerance = dgMax(dgAbs(faceNode->m_plane.m_w) * resolutionScale, minTolerance);

			if (dist < distTolerance) {
				dgVector sum[3];
				dgVector diff[3];
				m_normal = faceNode->m_plane & dgVector::m_triplexMask;
				for (dgInt32 i = 0; i < 3; i++) {
					dgInt32 j = faceNode->m_vertex[i];
					sum[i] = m_hullSum[j];
					diff[i] = m_hullDiff[j];
				}
				for (dgInt32 i = 0; i < 3; i++) {
					m_hullSum[i] = sum[i];
					m_hullDiff[i] = diff[i];
				}
				return 3;
			}

			iterCount++;
			bool isCycling = false;
			cyclingMem[cycling] = dist;
			if (iterCount > 10) {
				dgInt32 cyclingIndex = cycling;
				for (dgInt32 i = 0; i < 3; i++) {
					dgInt32 cyclingIndex0 = (cyclingIndex - 1) & 3;
					if (((cyclingMem[cyclingIndex0] - cyclingMem[cyclingIndex]) < dgFloat32(-1.0e-5f))) {
						isCycling = true;
						cyclingMem[0] = dgFloat32(1.0e10f);
						cyclingMem[1] = dgFloat32(1.0e10f);
						cyclingMem[2] = dgFloat32(1.0e10f);
						cyclingMem[3] = dgFloat32(1.0e10f);
						break;
					}
					cyclingIndex = cyclingIndex0;
				}
			}
			cycling = (cycling + 1) & 3;

			if (!isCycling) {
				m_faceStack[0] = faceNode;
				dgInt32 stackIndex = 1;
				dgInt32 deletedCount = 0;

				while (stackIndex) {
					stackIndex--;
					dgMinkFace* const face = m_faceStack[stackIndex];

					if (!face->m_mark && (face->m_plane.Evalue(p) > dgFloat32(0.0f))) {
#ifdef _DEBUG
						for (dgInt32 i = 0; i < deletedCount; i++) {
							dgAssert(m_deletedFaceList[i] != face);
						}
#endif

						m_deletedFaceList[deletedCount] = face;
						deletedCount++;
						dgAssert(deletedCount < sizeof (m_deletedFaceList) / sizeof (m_deletedFaceList[0]));
						face->m_mark = 1;

						for (dgInt32 i = 0; i < 3; i++) {
							dgMinkFace* const twinFace = face->m_twin[i];
							if (twinFace && !twinFace->m_mark) {
								m_faceStack[stackIndex] = twinFace;
								stackIndex++;
								dgAssert(stackIndex < sizeof (m_faceStack) / sizeof (m_faceStack[0]));
							}
						}
					}
				}

				//dgAssert (SanityCheck());
				dgInt32 newCount = 0;
				for (dgInt32 i = 0; i < deletedCount; i++) {
					dgMinkFace* const face = m_deletedFaceList[i];
					face->m_alive = 0;
					dgAssert(face->m_mark == 1);
					dgInt32 j0 = 2;
					for (dgInt32 j1 = 0; j1 < 3; j1++) {
						dgMinkFace* const twinFace = face->m_twin[j0];
						if (twinFace && !twinFace->m_mark) {
							dgMinkFace* const newFace = AddFace(m_vertexIndex, face->m_vertex[j0], face->m_vertex[j1]);
							PushFace(newFace);

							newFace->m_twin[1] = twinFace;
							dgInt32 index = (twinFace->m_twin[0] == face) ? 0 : ((twinFace->m_twin[1] == face) ? 1 : 2);
							twinFace->m_twin[index] = newFace;

							m_coneFaceList[newCount] = newFace;
							newCount++;
							dgAssert(newCount < sizeof (m_coneFaceList) / sizeof (m_coneFaceList[0]));
						}
						j0 = j1;
					}
				}

				dgInt32 i0 = newCount - 1;
				for (dgInt32 i1 = 0; i1 < newCount; i1++) {
					dgMinkFace* const faceA = m_coneFaceList[i0];
					dgAssert(faceA->m_mark == 0);

					dgInt32 j0 = newCount - 1;
					for (dgInt32 j1 = 0; j1 < newCount; j1++) {
						if (i0 != j0) {
							dgMinkFace* const faceB = m_coneFaceList[j0];
							dgAssert(faceB->m_mark == 0);
							if (faceA->m_vertex[2] == faceB->m_vertex[1]) {
								faceA->m_twin[2] = faceB;
								faceB->m_twin[0] = faceA;
								break;
							}
						}
						j0 = j1;
					}
					i0 = i1;
				}

				m_vertexIndex++;
				dgAssert(m_vertexIndex < sizeof (m_hullDiff) / sizeof (m_hullDiff[0]));

				dgAssert(SanityCheck());
			}
		} else {
			DeleteFace(faceNode);
		}
	}
	
	return -1;
}

DG_INLINE void dgContactSolver::CalculateContactFromFeacture(dgInt32 featureType)
{
	dgVector d;
	dgVector s;
	switch (featureType) 
	{
		case 1:
		{
			s = m_hullSum[0];
			d = m_hullDiff[0];
			break;
		}
		case 2:
		{
			const dgVector& p0 = m_hullDiff[0];
			const dgVector& p1 = m_hullDiff[1];
			dgVector dp(p1 - p0);
			dgAssert(dp.m_w == dgFloat32 (0.0f));
			dgAssert(dp.DotProduct(dp).GetScalar() > dgFloat32(0.0f));
			dgFloat32 alpha0 = - p0.DotProduct(dp).GetScalar() / dp.DotProduct(dp).GetScalar();
			dgAssert(alpha0 <= dgFloat32(1.01f));
			dgAssert(alpha0 >= dgFloat32(-0.01f));
			d = p0 + dp.Scale(alpha0);
			s = m_hullSum[0] + (m_hullSum[1] - m_hullSum[0]).Scale(alpha0);
			break;
		}

		case 3:
		default:
		{
			dgVector e10(m_hullDiff[1] - m_hullDiff[0]);
			dgVector e20(m_hullDiff[2] - m_hullDiff[0]);
			dgVector normal(e10.CrossProduct(e20));
			dgAssert(normal.m_w == dgFloat32 (0.0f));
			dgAssert(normal.DotProduct(normal).GetScalar() > dgFloat32(0.0f));

			dgFloat32 alphas[3];
			for (dgInt32 i0 = 2, i1 = 0; i1 < 3; i1++) {
				const dgVector& p1p0 = m_hullDiff[i0];
				const dgVector& p2p0 = m_hullDiff[i1];
				alphas[i0] = normal.DotProduct(p1p0.CrossProduct(p2p0)).GetScalar();
				i0 = i1;
			}

			dgFloat32 alphaDen = alphas[0] + alphas[1] + alphas[2];
			if (alphaDen > dgFloat32(1.0e-16f)) {
				dgAssert(alphaDen > dgFloat32(0.0f));

				alphaDen = dgFloat32(1.0f / alphaDen);
				alphas[0] *= alphaDen;
				alphas[1] *= alphaDen;
				alphas[2] *= alphaDen;
				s = m_hullSum[0].Scale(alphas[1]) + m_hullSum[1].Scale(alphas[2]) + m_hullSum[2].Scale(alphas[0]);
				d = m_hullDiff[0].Scale(alphas[1]) + m_hullDiff[1].Scale(alphas[2]) + m_hullDiff[2].Scale(alphas[0]);
			} else {
				// this is a degenerated face that is so small that lose accuracy in 32 bit floats
				// get the closest point from the longest edge

				dgAssert(e10.m_w == dgFloat32 (0.0f));
				dgAssert(e20.m_w == dgFloat32 (0.0f));
				dgVector dir((e10.DotProduct(e10).GetScalar() > e20.DotProduct(e20).GetScalar()) ? e10 : e20);
				dgInt32 i0 = 0;
				dgInt32 i1 = 0;
				dgFloat32 dist0 = dir.DotProduct(m_hullDiff[0]).GetScalar();
				dgFloat32 dist1 = -dist0;
				for (dgInt32 i = 1; i < 3; i++) {
					dgFloat32 test = dir.DotProduct(m_hullDiff[i]).GetScalar();
					if (test > dist0) {
						i0 = i;
						dist0 = test;
					}
					test *= dgFloat32(-1.0f);
					if (test > dist1) {
						i1 = i;
						dist1 = test;
					}
				}

				if (i0 != i1) {
					const dgVector& p0 = m_hullDiff[i0];
					const dgVector& p1 = m_hullDiff[i1];
					dgVector dp(p1 - p0);
					dgAssert(dp.m_w == dgFloat32 (0.0f));
					dgAssert(dp.DotProduct(dp).GetScalar() > dgFloat32(0.0f));
					dgFloat32 alpha0 = - p0.DotProduct(dp).GetScalar() / dp.DotProduct(dp).GetScalar();
					dgAssert(alpha0 <= dgFloat32(1.01f));
					dgAssert(alpha0 >= dgFloat32(-0.01f));
					d = p0 + dp.Scale(alpha0);
					s = m_hullSum[0] + (m_hullSum[i1] - m_hullSum[i0]).Scale(alpha0);
				} else {
					s = m_hullSum[i0];
					d = m_hullDiff[i0];
				}
			}
		}
	}

	m_closestPoint0 = dgVector::m_half * (s + d);
	m_closestPoint1 = dgVector::m_half * (s - d);
	dgAssert(m_normal.m_w == dgFloat32 (0.0f));
	dgAssert(dgAbs(m_normal.DotProduct(m_normal).GetScalar() - dgFloat32(1.0f)) < dgFloat32(1.0e-4f));
	m_proxy->m_contactJoint->m_separtingVector = m_normal;
}


bool dgContactSolver::SanityCheck() const
{
	for (dgInt32 i = 0; i < m_faceIndex; i++) {
		const dgMinkFace* const face = &m_facePool[i];
		if (face->m_alive) {
			for (dgInt32 j = 0; j < 3; j++) {
				dgMinkFace* const twin = face->m_twin[j];
				if (!twin) {
					return false;
				}

				if (!twin->m_alive) {
					return false;
				}

				bool pass = false;
				for (dgInt32 k = 0; k < 3; k++) {
					if (twin->m_twin[k] == face) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return pass;
				}
			}
		}
	}
	return true;
}


bool dgContactSolver::CalculateClosestPoints()
{
	dgInt32 simplexPointCount = CalculateClosestSimplex();
	if (simplexPointCount < 0) {
		simplexPointCount = CalculateIntersectingPlane(-simplexPointCount);
	}

	if (simplexPointCount > 0) {
		dgAssert((simplexPointCount > 0) && (simplexPointCount <= 3));
		CalculateContactFromFeacture(simplexPointCount);

		const dgMatrix& matrix0 = m_instance0->m_globalMatrix;
		const dgMatrix& matrix1 = m_instance1->m_globalMatrix;
		m_closestPoint0 = matrix0.TransformVector(m_instance0->SupportVertexSpecialProjectPoint(matrix0.UntransformVector(m_closestPoint0), matrix0.UnrotateVector(m_normal)));
		m_closestPoint1 = matrix1.TransformVector(m_instance1->SupportVertexSpecialProjectPoint(matrix1.UntransformVector(m_closestPoint1), matrix1.UnrotateVector(m_normal.Scale(-1.0f))));
		m_vertexIndex = simplexPointCount;
	}
	return simplexPointCount >= 0;
}

dgInt32 dgContactSolver::ConvexPolygonToLineIntersection(const dgVector& normal, dgInt32 count1, dgVector* const shape1, dgInt32 count2, dgVector* const shape2, dgVector* const contactOut, dgVector* const mem) const
{
	dgInt32 count = 0;
	dgVector* output = mem;

	dgAssert(count1 >= 3);
	dgAssert(count2 <= 2);
	dgAssert(normal.m_w == dgFloat32 (0.0f));

	dgVector* ptr = NULL;
	// face line intersection
	if (count2 == 2) {
		ptr = (dgVector*)&shape2[0];
		dgInt32 i0 = count1 - 1;
		for (dgInt32 i1 = 0; i1 < count1; i1++) {
			dgVector n(normal.CrossProduct(shape1[i1] - shape1[i0]));
			dgAssert(n.m_w == dgFloat32 (0.0f));
			dgAssert(n.DotProduct(n).GetScalar() > dgFloat32(0.0f));
			dgPlane plane(n, - n.DotProduct(shape1[i0]).GetScalar());

			dgFloat32 test0 = plane.Evalue(ptr[0]);
			dgFloat32 test1 = plane.Evalue(ptr[1]);
			if (test0 >= dgFloat32(0.0f)) {
				if (test1 >= dgFloat32(0.0f)) {
					output[count + 0] = ptr[0];
					output[count + 1] = ptr[1];
					count += 2;
				} else {
					dgVector dp(ptr[1] - ptr[0]);
					dgAssert (dp.m_w == dgFloat32 (0.0f));
					dgFloat32 den = plane.DotProduct(dp).GetScalar();
					if (dgAbs(den) < 1.0e-10f) {
						den = 1.0e-10f;
					}
					output[count + 0] = ptr[0];
					dgAssert (dp.m_w == dgFloat32 (0.0f));
					output[count + 1] = ptr[0] - dp.Scale(test0 / den);
					count += 2;
				}
			} else if (test1 >= dgFloat32(0.0f)) {
				dgVector dp(ptr[1] - ptr[0]);
				dgAssert (dp.m_w == dgFloat32 (0.0f));
				dgFloat32 den = plane.DotProduct(dp).GetScalar();
				if (dgAbs(den) < 1.0e-10f) {
					den = 1.0e-10f;
				}
				dgAssert (dp.m_w == dgFloat32 (0.0f));
				output[count] = ptr[0] - dp.Scale(test0 / den);
				count++;
				output[count] = ptr[1];
				count++;
			} else {
				return 0;
			}


			count2 = count;
			ptr = output;
			output = &output[count];
			count = 0;
			i0 = i1;
			//dgAssert (output < &pool[sizeof (pool)/sizeof (pool[0])]);
		}
	} else if (count2 == 1) {
		const dgVector& p = shape2[0];
		dgInt32 i0 = count1 - 1;
		for (dgInt32 i1 = 0; i1 < count1; i1++) {
			dgVector n(normal.CrossProduct(shape1[i1] - shape1[i0]));
			dgAssert(n.m_w == dgFloat32 (0.0f));
			dgAssert(n.DotProduct(n).GetScalar() > dgFloat32(0.0f));
			dgPlane plane(n, - n.DotProduct(shape1[i0]).GetScalar());
			dgFloat32 test0 = plane.Evalue(p);
			if (test0 < dgFloat32(-1.e-3f)) {
				return 0;
			}
			i0 = i1;
		}
		ptr = output;
		output[count] = p;
		count++;

	} else {
		count2 = 0;
	}

	for (dgInt32 i0 = 0; i0 < count2; i0++) {
		contactOut[i0] = ptr[i0];
	}
	return count2;
}


DG_INLINE dgContactSolver::dgPerimenterEdge* dgContactSolver::ReduceContacts(dgPerimenterEdge* poly, dgInt32 maxCount) const
{
	dgInt32 buffer[DG_MAX_EDGE_COUNT];
	dgUpHeap<dgPerimenterEdge*, dgFloat32> heap(buffer, sizeof (buffer));

	dgInt32 restart = 1;
	while (restart) {
		restart = 0;
		dgPerimenterEdge* ptr0 = poly;
		poly = poly->m_next;
		if (poly->m_next != poly) {
			heap.Flush();
			dgPerimenterEdge* ptr = poly;
			do {
				dgVector error(*ptr->m_next->m_vertex - *ptr->m_vertex);
				dgAssert(error.m_w == 0.0f);
				dgFloat32 dist2 = error.DotProduct(error).GetScalar();
				if (dist2 < DG_MINK_VERTEX_ERR2) {
					ptr0->m_next = ptr->m_next;
					if (ptr == poly) {
						poly = ptr0;
						restart = 1;
						break;
					}
					ptr = ptr0;
				} else {
					heap.Push(ptr, dist2);
					ptr0 = ptr;
				}

				ptr = ptr->m_next;
			} while (ptr != poly);
		}
	}

	if (heap.GetCount()) {
		if (maxCount > 8) {
			maxCount = 8;
		}
		while (heap.GetCount() > maxCount) {
			dgPerimenterEdge* ptr = heap[0];
			heap.Pop();
			for (dgInt32 i = 0; i < heap.GetCount(); i++) {
				if (heap[i] == ptr->m_next) {
					heap.Remove(i);
					break;
				}
			}

			ptr->m_next = ptr->m_next->m_next;
			dgVector error(*ptr->m_next->m_vertex - *ptr->m_vertex);
			dgAssert(error.m_w == 0.0f);
			dgFloat32 dist2 = error.DotProduct(error).GetScalar();
			heap.Push(ptr, dist2);
		}
		poly = heap[0];
	}

	return poly;
}

dgInt32 dgContactSolver::ConvexPolygonsIntersection(const dgVector& normal, dgInt32 count0, dgVector* const shape0, dgInt32 count1, dgVector* const shape1, dgVector* const contactOut, dgInt32 maxContacts) const
{
	dgInt32 count = 0;
	if (count1 <= 2) {
		count = ConvexPolygonToLineIntersection(normal.Scale (dgFloat32 (-1.0f)), count0, shape0, count1, shape1, contactOut, &contactOut[count0 + count1 + maxContacts]);
	} else if (count0 <= 2) {
		count = ConvexPolygonToLineIntersection(normal, count1, shape1, count0, shape0, contactOut, &contactOut[count0 + count1 + maxContacts]);
	} else {
		dgAssert(count0 >= 3);
		dgAssert(count1 >= 3);

		dgPerimenterEdge subdivision[128];
		dgAssert((2 * (count0 + count1)) < dgInt32(sizeof (subdivision) / sizeof (subdivision[0])));

		for (dgInt32 i0 = 1; i0 < count1; i0++) {
			subdivision[i0].m_vertex = &shape1[i0];
			subdivision[i0].m_prev = &subdivision[i0 - 1];
			subdivision[i0].m_next = &subdivision[i0 + 1];
		}
		subdivision[0].m_vertex = &shape1[0];
		subdivision[0].m_prev = &subdivision[count1 - 1];
		subdivision[0].m_next = &subdivision[1];

		subdivision[count1 - 1].m_next = &subdivision[0];

		dgPerimenterEdge* edgeClipped[2];
		dgVector* output = &contactOut[count0 + count1 + maxContacts];

		edgeClipped[0] = NULL;
		edgeClipped[1] = NULL;
		dgInt32 j0 = 0;
		dgInt32 edgeIndex = count1;
		dgPerimenterEdge* poly = &subdivision[0];
		for (dgInt32 j1 = count0 - 1; j1 >= 0; j1--) {
			dgVector n(normal.CrossProduct(shape0[j1] - shape0[j0]));
			dgAssert(n.m_w == 0.0f);
			dgPlane plane(n, - n.DotProduct(shape0[j0]).GetScalar());
			j0 = j1;
			count = 0;
			dgPerimenterEdge* tmp = poly;
			dgInt32 isInside = 0;
			dgFloat32 test0 = plane.Evalue(*tmp->m_vertex);
			do {
				dgFloat32 test1 = plane.Evalue(*tmp->m_next->m_vertex);

				if (test0 >= dgFloat32(0.0f)) {
					isInside |= 1;
					if (test1 < dgFloat32(0.0f)) {
						const dgVector& p0 = *tmp->m_vertex;
						const dgVector& p1 = *tmp->m_next->m_vertex;
						dgVector dp(p1 - p0);
						dgAssert(dp.m_w == 0.0f);
						dgFloat32 den = plane.DotProduct(dp).GetScalar();
						if (dgAbs(den) < dgFloat32(1.0e-24f)) {
							den = (den >= dgFloat32(0.0f)) ? dgFloat32(1.0e-24f) : dgFloat32(-1.0e-24f);
						}

						den = test0 / den;
						if (den >= dgFloat32(0.0f)) {
							den = dgFloat32(0.0f);
						} else if (den <= -1.0f) {
							den = dgFloat32(-1.0f);
						}
						dgAssert (dp.m_w == dgFloat32 (0.0f));
						output[0] = p0 - dp.Scale(den);
						edgeClipped[0] = tmp;
						count++;
					}
				} else if (test1 >= dgFloat32(0.0f)) {
					const dgVector& p0 = *tmp->m_vertex;
					const dgVector& p1 = *tmp->m_next->m_vertex;
					isInside |= 1;
					dgVector dp(p1 - p0);
					dgAssert(dp.m_w == 0.0f);
					dgFloat32 den = plane.DotProduct(dp).GetScalar();
					if (dgAbs(den) < dgFloat32(1.0e-24f)) {
						den = (den >= dgFloat32(0.0f)) ? dgFloat32(1.0e-24f) : dgFloat32(-1.0e-24f);
					}
					den = test0 / den;
					if (den >= dgFloat32(0.0f)) {
						den = dgFloat32(0.0f);
					} else if (den <= -1.0f) {
						den = dgFloat32(-1.0f);
					}
					dgAssert (dp.m_w == dgFloat32 (0.0f));
					output[1] = p0 - dp.Scale(den);
					edgeClipped[1] = tmp;
					count++;
				}

				test0 = test1;
				tmp = tmp->m_next;
			} while (tmp != poly && (count < 2));

			if (!isInside) {
				return 0;
			}

			if (count == 2) {
				dgPerimenterEdge* const newEdge = &subdivision[edgeIndex];
				newEdge->m_next = edgeClipped[1];
				newEdge->m_prev = edgeClipped[0];
				edgeClipped[0]->m_next = newEdge;
				edgeClipped[1]->m_prev = newEdge;

				newEdge->m_vertex = &output[0];
				edgeClipped[1]->m_vertex = &output[1];
				poly = newEdge;

				output += 2;
				edgeIndex++;
				//dgAssert (output < &pool[sizeof (pool)/sizeof (pool[0])]);
				dgAssert(edgeIndex < dgInt32(sizeof (subdivision) / sizeof (subdivision[0])));
			}
		}

		dgAssert(poly);
		poly = ReduceContacts(poly, maxContacts);
		count = 0;
		dgPerimenterEdge* intersection = poly;
		do {
			contactOut[count] = *intersection->m_vertex;
			count++;
			intersection = intersection->m_next;
		} while (intersection != poly);
	}
	return count;
}

dgFloat32 dgContactSolver::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut)
{
	dgVector normal(dgFloat32 (0.0f));
	dgVector point (localP0);
	dgVector point0 (localP0);
	dgVector p0p1 (localP0 - localP1);

	// avoid NaN as a result of a division by zero
	if ((p0p1.TestZero().GetSignMask() & 7) == 7) {
		return dgFloat32(1.2f);
	}

	dgFloat32 param = dgFloat32 (0.0f);

	dgInt32 index = 0;
	memset (m_hullSum, 0, 4 * sizeof (m_hullSum[0]));
	const dgCollisionConvex* const collision = (dgCollisionConvex*)m_instance0->GetChildShape();

	dgVector dir1 (p0p1.Normalize());
	m_hullDiff[0] = collision->SupportVertex (dir1, NULL) - point;
	dgBigVector v (m_hullDiff[0]);
	index = 1;
	do {
		dgInt32 iter = 0;
		dgInt32 cycling = 0;
		dgFloat64 minDist = dgFloat32 (1.0e20f);

		do {
			dgAssert (v.m_w == dgFloat32 (0.0f));
			const dgFloat64 distance = v.DotProduct(v).GetScalar();
			if (distance < dgFloat32 (1.0e-9f)) {
				index = -1; 
				break;
			}

			if (distance < minDist) {
				minDist = distance;
				cycling = -1;
			}
			cycling ++;
			if (cycling > 4) {
				//dgAssert (0);
				index = -1; 
				break;
			}

			dgVector dir (v.Scale (-dgRsqrt(dgFloat32 (distance))));
			dgAssert (dir.m_w == dgFloat32 (0.0f));
			m_hullDiff[index] = collision->SupportVertex (dir, NULL) - point;
			const dgBigVector w (m_hullDiff[index]);
			const dgVector wv (w - v);
			dgAssert (wv.m_w == dgFloat32 (0.0f));
			const dgFloat32 distance1 = dir.DotProduct(wv).GetScalar();
			if (distance1 < dgFloat64 (1.0e-3f)) {
				normal = dir;
				break;
			}

			index ++;
			switch (index) 
			{
				case 2:
				{
					v = ReduceLine (index);
					break;
				}

				case 3:
				{
					v = ReduceTriangle (index);
					break;
				}

				case 4:
				{
					v = ReduceTetrahedrum (index);
					break;
				}
			}

			iter ++;
		} while (iter < DG_CONNICS_CONTATS_ITERATIONS); 

		dgAssert (index);
		if (index > 0) {
			dgVector q (v + point);
			dgFloat32 den = normal.DotProduct(p0p1).GetScalar();
			if (dgAbs (den) < dgFloat32(1.0e-12f))  {
				den = dgSign(den) * dgFloat32(1.0e-12f);
			}
			dgAssert (normal.m_w == dgFloat32 (0.0f));
			dgFloat32 t1 = normal.DotProduct(localP0 - q).GetScalar() / den;

			if (t1 < param) {
				index = -1;
				t1 = dgFloat32 (0.0f);
			} else if (t1 > maxT) {
				index = -1;
				t1 = dgFloat32 (1.0f);
			}
			param = t1;
	
			point = localP0 - p0p1.Scale (param);
			dgVector step (point0 - point);
			point0 = point;
			for(dgInt32 i = 0; i < index; i ++) {
				m_hullDiff[i] += step;
			}

			switch (index) 
			{
				case 1:
				{
					v = m_hullDiff[0];
					break;
				}

				case 2:
				{
					v = ReduceLine (index);
					break;
				}

				case 3:
				{
					v = ReduceTriangle (index);
					break;
				}

				case 4:
				{
					v = ReduceTetrahedrum (index);
					break;
				}
			}
		}
	} while (index >= 0);

	if ((param > dgFloat32 (0.0f)) && (param < maxT)) {
		contactOut.m_normal = normal;
	} else {
		param = dgFloat32 (1.2f);
	}

	return param;
}


DG_INLINE void dgContactSolver::TranslateSimplex(const dgVector& step)
{
	m_instance1->m_globalMatrix.m_posit -= step;
	for (dgInt32 i = 0; i < m_vertexIndex; i++) {
		m_hullSum[i] -= step;
		m_hullDiff[i] += step;
	}
}

dgInt32 dgContactSolver::CalculateConvexCastContacts()
{
	dgInt32 iter = 0;
	dgInt32 count = 0;
	
	dgFloat32 tacc = dgFloat32(0.0f);
	dgFloat32 timestep = m_proxy->m_timestep;
	m_proxy->m_contactJoint->m_closestDistance = dgFloat32(1.0e10f);

	dgVector savedPosition1 (m_instance1->m_globalMatrix.m_posit);

	dgVector relVeloc (m_proxy->m_body0->GetVelocity() - m_proxy->m_body1->GetVelocity());
	do {
		bool state = CalculateClosestPoints();
		if (!state) {
			break;
		}
		dgAssert(m_normal.m_w == dgFloat32(0.0f));
		dgFloat32 den = m_normal.DotProduct(relVeloc).GetScalar();
		if (den <= dgFloat32(1.0e-6f)) {
			// bodies are residing from each other, even if they are touching they are not considered to be colliding because the motion will move them apart 
			// get the closet point and the normal at contact point
			m_proxy->m_timestep = dgFloat32 (1.0e10f);
			m_proxy->m_normal = m_normal.Scale(-1.0f);
			m_proxy->m_closestPointBody0 = m_closestPoint0;
			m_proxy->m_closestPointBody1 = m_closestPoint1;
			break;
		}

		dgFloat32 num = m_normal.DotProduct(m_closestPoint1 - m_closestPoint0).GetScalar() - m_proxy->m_skinThickness;
		if ((num <= dgFloat32(1.0e-5f)) && (tacc <= timestep)) {
			// bodies collide at time tacc, but we do not set it yet
			dgVector step(relVeloc.Scale(tacc));
			m_proxy->m_timestep = tacc;
			m_proxy->m_closestPointBody0 = m_closestPoint0 + step;
			m_proxy->m_closestPointBody1 = m_closestPoint1 + step;
			m_proxy->m_normal = m_normal.Scale (dgFloat32 (-1.0f));
			m_proxy->m_contactJoint->m_closestDistance = m_proxy->m_normal.DotProduct(m_closestPoint0 - m_closestPoint1).GetScalar();
			dgFloat32 penetration = dgMax(num * dgFloat32(-1.0f) + DG_PENETRATION_TOL, dgFloat32(0.0f));
			m_proxy->m_contactJoint->m_closestDistance = penetration;
			if (m_proxy->m_contacts && !m_proxy->m_intersectionTestOnly) {
				if (m_proxy->m_instance0->GetCollisionMode() & m_proxy->m_instance1->GetCollisionMode()) {

					m_normal = m_normal.Scale (dgFloat32 (-1.0f));
					m_proxy->m_contactJoint->m_contactActive = 1;
					dgVector contactPoint((m_closestPoint0 + m_closestPoint1).Scale(dgFloat32(0.5f)));
					count = CalculateContacts(m_closestPoint0, m_closestPoint1, m_normal);
					if (count) {
						count = dgMin(m_proxy->m_maxContacts, count);
						dgContactPoint* const contactOut = m_proxy->m_contacts;

						for (int i = 0; i < count; i++) {
							contactOut[i].m_point = m_hullDiff[i] + step;
							contactOut[i].m_normal = m_normal;
							contactOut[i].m_penetration = penetration;
						}
					}
				}
			}
			break;
		}

		dgAssert (den > dgFloat32 (0.0f));
		dgFloat32 dt = num / den;
		if ((tacc + dt) >= timestep) {
			// object do not collide on this timestep
			m_proxy->m_timestep = tacc + dt;
			m_proxy->m_normal = m_normal.Scale (dgFloat32 (-1.0f));
			m_proxy->m_closestPointBody0 = m_closestPoint0;
			m_proxy->m_closestPointBody1 = m_closestPoint1;
			break;
		}

		tacc += dt;
		dgVector step(relVeloc.Scale(dt));
		TranslateSimplex(step);

		iter++;
	} while (iter < DG_SEPARATION_PLANES_ITERATIONS);

	m_instance1->m_globalMatrix.m_posit = savedPosition1;
	return count;
}


dgInt32 dgContactSolver::CalculateConvexToConvexContacts ()
{
	dgInt32 count = 0;
	if (m_proxy->m_intersectionTestOnly) {
		CalculateClosestPoints();
		dgFloat32 penetration = m_normal.DotProduct(m_closestPoint1 - m_closestPoint0).GetScalar() - m_proxy->m_skinThickness - DG_PENETRATION_TOL;
		dgInt32 retVal = (penetration <= dgFloat32(0.0f)) ? -1 : 0;
		m_proxy->m_contactJoint->m_contactActive = retVal;
		return retVal;
	} else {
		bool colliding = CalculateClosestPoints();
		if (colliding) { 
			dgFloat32 penetration = m_normal.DotProduct(m_closestPoint1 - m_closestPoint0).GetScalar() - m_proxy->m_skinThickness - DG_PENETRATION_TOL;
			if (penetration <= dgFloat32(1.0e-5f)) {
				m_proxy->m_contactJoint->m_contactActive = 1;
				if (m_instance0->GetCollisionMode() & m_instance1->GetCollisionMode()) {
					count = CalculateContacts(m_closestPoint0, m_closestPoint1, m_normal.Scale(-1.0f));
				}
			}

			m_proxy->m_closestPointBody0 = m_closestPoint0;
			m_proxy->m_closestPointBody1 = m_closestPoint1;
			m_proxy->m_contactJoint->m_closestDistance = penetration;
			m_proxy->m_contactJoint->m_separationDistance = penetration;

			m_normal = m_normal.Scale (dgFloat32 (-1.0f));
			penetration = -penetration;
			m_proxy->m_normal = m_normal;
			count = dgMin(m_proxy->m_maxContacts, count);
			dgContactPoint* const contactOut = m_proxy->m_contacts;
//if (count){
//dgTrace (("%d\n", xxx)); 
//}
			for (int i = 0; i < count; i ++) {
				contactOut[i].m_point = m_hullDiff[i];
				contactOut[i].m_normal = m_normal;
				contactOut[i].m_penetration = penetration;

//dgTrace (("p (%f %f %f) ", contactOut[i].m_point.m_x, contactOut[i].m_point.m_y, contactOut[i].m_point.m_z));
//dgTrace (("n (%f %f %f) ", contactOut[i].m_normal.m_x, contactOut[i].m_normal.m_y, contactOut[i].m_normal.m_z));
//dgTrace (("h (%f) ", contactOut[i].m_normal.m_x));
//dgTrace (("\n", penetration)); 
			}
		}
	}

	return count;
}


dgInt32 dgContactSolver::CalculateContacts(const dgVector& point0, const dgVector& point1, const dgVector& normal)
{
	dgInt32 count = 0;

	const dgInt32 baseCount = 16;

	dgVector* const contactsOut = &m_hullDiff[0];
	dgAssert(m_instance1->IsType(dgCollision::dgCollisionConvexShape_RTTI));
	dgAssert(m_instance0->IsType(dgCollision::dgCollisionConvexShape_RTTI));

	dgInt32 count1 = 0;
	dgVector* const shape1 = &contactsOut[baseCount];

	dgAssert(normal.m_w == dgFloat32(0.0f));

	dgVector origin((point0 + point1).Scale(dgFloat32(0.5f)));
	const dgMatrix& matrix1 = m_instance1->m_globalMatrix;
	dgVector ponintOnInstance1(matrix1.UntransformVector(origin));
	dgVector normalOnInstance1(matrix1.UnrotateVector(normal));
	dgFloat32 dist = (normal.DotProduct(point0 - point1)).GetScalar();
	if (dist < dgFloat32(0.0f)) {
		count1 = m_instance1->CalculatePlaneIntersection(normalOnInstance1, ponintOnInstance1, shape1);
	}
	if (!count1) {
		dgVector step(normal.Scale(DG_PENETRATION_TOL * dgFloat32(2.0f)));
		dgVector alternatePoint(point1);
		for (dgInt32 i = 0; (i < 3) && !count1; i++) {
			alternatePoint -= step;
			dgVector alternatePointOnInstance1(matrix1.UntransformVector(alternatePoint));
			count1 = m_instance1->CalculatePlaneIntersection(normalOnInstance1, alternatePointOnInstance1, shape1);
		}
		//dgAssert(count1);
		step = matrix1.UnrotateVector(normal * ((alternatePoint - origin).DotProduct(normal)));
		for (dgInt32 i = 0; i < count1; i++) {
			shape1[i] -= step;
		}
	}

	if (count1) {
		for (int i = 0; i < count1; i++) {
			shape1[i] = matrix1.TransformVector(shape1[i]);
		}

		dgInt32 count0 = 0;
		dgVector* const shape0 = &contactsOut[baseCount + count1];

		const dgMatrix& matrix0 = m_instance0->m_globalMatrix;
		dgVector pointOnInstance0(matrix0.UntransformVector(origin));
		dgVector normalOnInstance0(matrix0.UnrotateVector(normal.Scale(dgFloat32(-1.0f))));
		if (dist < dgFloat32(0.0f)) {
			count0 = m_instance0->CalculatePlaneIntersection(normalOnInstance0, pointOnInstance0, shape0);
		}
		if (!count0) {
			dgVector step(normal.Scale(DG_PENETRATION_TOL * dgFloat32(2.0f)));
			dgVector alternatePoint(point0);
			for (dgInt32 i = 0; (i < 3) && !count0; i++) {
				alternatePoint += step;
				dgVector alternatePointOnInstance0(matrix0.UntransformVector(alternatePoint));
				count0 = m_instance0->CalculatePlaneIntersection(normalOnInstance0, alternatePointOnInstance0, shape0);
			}
			dgAssert(count0);
			step = matrix0.UnrotateVector(normal * ((alternatePoint - origin).DotProduct(normal)));
			for (dgInt32 i = 0; i < count0; i++) {
				shape0[i] -= step;
			}
		}

		if (count0) {
			for (dgInt32 i = 0; i < count0; i++) {
				shape0[i] = matrix0.TransformVector(shape0[i]);
			}

			if (count1 == 1) {
				count = 1;
				contactsOut[0] = shape1[0];
			} else if (count0 == 1) {
				count = 1;
				contactsOut[0] = shape0[0];
			} else if ((count1 == 2) && (count0 == 2)) {
				dgVector p0(shape1[0]);
				dgVector p1(shape1[1]);
				const dgVector& q0 = shape0[0];
				const dgVector& q1 = shape0[1];
				dgVector p10(p1 - p0);
				dgVector q10(q1 - q0);
				dgAssert(p10.m_w == dgFloat32(0.0f));
				dgAssert(q10.m_w == dgFloat32(0.0f));
				p10 = p10.Scale(dgRsqrt(p10.DotProduct(p10).GetScalar() + dgFloat32(1.0e-8f)));
				q10 = q10.Scale(dgRsqrt(q10.DotProduct(q10).GetScalar() + dgFloat32(1.0e-8f)));
				dgFloat32 dot = q10.DotProduct(p10).GetScalar();
				if (dgAbs(dot) > dgFloat32(0.998f)) {
					dgFloat32 pl0 = p0.DotProduct(p10).GetScalar();
					dgFloat32 pl1 = p1.DotProduct(p10).GetScalar();
					dgFloat32 ql0 = q0.DotProduct(p10).GetScalar();
					dgFloat32 ql1 = q1.DotProduct(p10).GetScalar();
					if (pl0 > pl1) {
						dgSwap(pl0, pl1);
						dgSwap(p0, p1);
						p10 = p10.Scale(dgFloat32(-1.0f));
					}
					if (ql0 > ql1) {
						dgSwap(ql0, ql1);
					}
					if (!((ql0 > pl1) && (ql1 < pl0))) {
						dgFloat32 clip0 = (ql0 > pl0) ? ql0 : pl0;
						dgFloat32 clip1 = (ql1 < pl1) ? ql1 : pl1;

						count = 2;
						contactsOut[0] = p0 + p10.Scale(clip0 - pl0);
						contactsOut[1] = p0 + p10.Scale(clip1 - pl0);
					}
				} else {
					count = 1;
					dgVector c0;
					dgVector c1;
					dgRayToRayDistance(p0, p1, q0, q1, c0, c1);
					contactsOut[0] = (c0 + c1).Scale(dgFloat32(0.5f));
				}
			} else {
				dgAssert((count1 >= 2) && (count0 >= 2));
				count = ConvexPolygonsIntersection(normal, count0, shape0, count1, shape1, contactsOut, baseCount);
			}
		}
	}

	if (!count && m_proxy->m_continueCollision) {
		count = 1;
		contactsOut[0] = origin;
	}

	return count;
}
