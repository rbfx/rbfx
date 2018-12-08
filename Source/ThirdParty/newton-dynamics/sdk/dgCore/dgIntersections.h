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

#ifndef __dgIntersections__
#define __dgIntersections__

#include "dgStdafx.h"
#include "dgObb.h"
#include "dgDebug.h"
#include "dgVector.h"
#include "dgMatrix.h"

class dgPlane;
class dgObject;
class dgPolyhedra;

enum dgIntersectStatus
{
	t_StopSearh,
	t_ContinueSearh
};

typedef dgIntersectStatus (*dgAABBIntersectCallback) (void* const context, 
													  const dgFloat32* const polygon, dgInt32 strideInBytes,
													  const dgInt32* const indexArray, dgInt32 indexCount, dgFloat32 hitDistance);

typedef dgFloat32 (*dgRayIntersectCallback) (void* const context, 
											 const dgFloat32* const polygon, dgInt32 strideInBytes,
											 const dgInt32* const indexArray, dgInt32 indexCount);

dgBigVector dgPointToRayDistance (const dgBigVector& point, const dgBigVector& ray_p0, const dgBigVector& ray_p1); 
dgBigVector dgPointToTriangleDistance (const dgBigVector& point, const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2);
dgBigVector dgPointToTetrahedrumDistance (const dgBigVector& point, const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3);

bool dgRayBoxClip (dgVector& ray_p0, dgVector& ray_p1, const dgVector& boxP0, const dgVector& boxP1); 
void dgRayToRayDistance (const dgVector& ray_p0, const dgVector& ray_p1, const dgVector& ray_q0, const dgVector& ray_q1, dgVector& p0Out, dgVector& p1Out); 
dgFloat32 dgRayCastBox (const dgVector& p0, const dgVector& p1, const dgVector& boxP0, const dgVector& boxP1, dgVector& normalOut);
dgFloat32 dgRayCastSphere (const dgVector& p0, const dgVector& p1, const dgVector& origin, dgFloat32 radius);


DG_INLINE dgInt32 dgOverlapTest (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
//	dgVector val0 ((p0 < q1) & (p1 > q0));
//	dgInt32 mask1 = val0.GetSignMask();
	dgVector val((p0 - q1) * (p1 - q0));
	dgInt32 mask = val.GetSignMask();
	return ((mask & 0x07) == 0x07);
}


DG_INLINE dgInt32 dgBoxInclusionTest (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
	dgVector val (dgVector::m_negOne & ((p0 >= q0) & (p1 <= q1)));
	dgInt32 mask = val.GetSignMask();
	return ((mask & 0x07) == 0x07);
}

DG_INLINE dgInt32 dgCompareBox (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
	dgAssert(0);
	return (p0.m_x != q0.m_x) || (p0.m_y != q0.m_y) || (p0.m_z != q0.m_z) || (p1.m_x != q1.m_x) || (p1.m_y != q1.m_y) || (p1.m_z != q1.m_z);
}


DG_INLINE void dgMovingAABB (dgVector& p0, dgVector& p1, const dgVector& veloc, const dgVector& omega, dgFloat32 timestep, dgFloat32 maxRadius, dgFloat32 minRadius)
{
	dgVector linearStep (veloc.Scale (timestep));

	// estimate the maximum effect of the angular velocity and enlarge that box by that value (use 45 degrees as max angle not 90)
	dgFloat32 maxAngle = dgMin (dgSqrt (omega.DotProduct3(omega) * timestep * timestep), dgFloat32 (45.0f * dgDEG2RAD));

	dgFloat32 angularTravel = (maxRadius - minRadius) * maxAngle;
	dgVector angularStep (angularTravel, angularTravel, angularTravel, dgFloat32 (0.0f));
	
	dgVector r0 (p0 - angularStep);
	dgVector r1 (p1 + angularStep);
	dgVector q0 (r0 + linearStep);
	dgVector q1 (r1 + linearStep);
	//p0 = dgVector (dgMin (r0.m_x, q0.m_x), dgMin (r0.m_y, q0.m_y), dgMin (r0.m_z, q0.m_z), dgFloat32 (0.0f));
	p0 = r0.GetMin (q0);
	//p1 = dgVector (dgMax (r1.m_x, q1.m_x), dgMax (r1.m_y, q1.m_y), dgMax (r1.m_z, q1.m_z), dgFloat32 (0.0f));
	p1 = r1.GetMax (q1);
	p0.m_w = dgFloat32 (0.0f);
	p1.m_w = dgFloat32 (0.0f);
}


DG_INLINE dgFloat32 BoxPenetration (const dgVector& minBox, const dgVector& maxBox)
{
	dgAssert(maxBox.m_x >= minBox.m_x);
	dgAssert(maxBox.m_y >= minBox.m_y);
	dgAssert(maxBox.m_z >= minBox.m_z);

	dgVector mask ((minBox * maxBox) < dgVector::m_zero);
	dgVector dist (maxBox.GetMin (minBox.Abs()) & mask);
	dist = dist.GetMin(dist.ShiftTripleRight());
	dist = dist.GetMin(dist.ShiftTripleRight());
	return dist.GetScalar();
}

DG_INLINE dgFloat32 dgBoxDistanceToOrigin2 (const dgVector& minBox, const dgVector& maxBox)
{
	dgAssert(maxBox.m_x >= minBox.m_x);
	dgAssert(maxBox.m_y >= minBox.m_y);
	dgAssert(maxBox.m_z >= minBox.m_z);
	dgVector mask ((minBox * maxBox) > dgVector::m_zero);
	dgVector dist (maxBox.Abs().GetMin (minBox.Abs()) & mask);
	return dist.DotProduct(dist).GetScalar();
}


DG_MSC_VECTOR_ALIGMENT
class dgFastRayTest
{
	public:
	DG_INLINE dgFastRayTest(const dgVector& l0, const dgVector& l1)
		:m_p0(l0)
		,m_p1(l1)
		,m_diff((l1 - l0) & dgVector::m_triplexMask)
		,m_minT(dgFloat32(0.0f))
		,m_maxT(dgFloat32(1.0f))
	{
		dgAssert(m_p0.m_w == dgFloat32(0.0f));
		dgAssert(m_p1.m_w == dgFloat32(0.0f));
		dgAssert(m_diff.m_w == dgFloat32(0.0f));

		dgAssert (m_diff.DotProduct(m_diff).GetScalar() > dgFloat32 (0.0f));
		m_isParallel = (m_diff.Abs() < dgVector(1.0e-8f));
		m_dpInv = (((dgVector(dgFloat32(1.0e-20)) & m_isParallel) | m_diff.AndNot(m_isParallel)).Reciproc()) & dgVector::m_triplexMask;
		m_unitDir = m_diff.Normalize();
	}

	dgFloat32 PolygonIntersect(const dgVector& normal, dgFloat32 maxT, const dgFloat32* const polygon, dgInt32 strideInBytes, const dgInt32* const indexArray, dgInt32 indexCount) const;

	DG_INLINE dgInt32 BoxTest(const dgVector& minBox, const dgVector& maxBox) const
	{
#if 1
		dgVector test(((m_p0 <= minBox) | (m_p0 >= maxBox)) & m_isParallel);
		if (test.GetSignMask() & 0x07) {
			return 0;
		}

		dgVector tt0(m_dpInv * (minBox - m_p0));
		dgVector tt1(m_dpInv * (maxBox - m_p0));

		dgVector t0(m_minT.GetMax(tt0.GetMin(tt1)));
		dgVector t1(m_maxT.GetMin(tt0.GetMax(tt1)));
		t0 = t0.GetMax(t0.ShiftTripleRight());
		t1 = t1.GetMin(t1.ShiftTripleRight());
		t0 = t0.GetMax(t0.ShiftTripleRight());
		t1 = t1.GetMin(t1.ShiftTripleRight());
		return ((t0 < t1).GetSignMask() & 1);

#else

		dgFloat32 tmin = 0.0f;
		dgFloat32 tmax = 1.0f;

		for (dgInt32 i = 0; i < 3; i++) {
			if (m_isParallel[i]) {
				if (m_p0[i] <= minBox[i] || m_p0[i] >= maxBox[i]) {
					return 0;
				}
			} else {
				dgFloat32 t1 = (minBox[i] - m_p0[i]) * m_dpInv[i];
				dgFloat32 t2 = (maxBox[i] - m_p0[i]) * m_dpInv[i];

				if (t1 > t2) {
					dgSwap(t1, t2);
				}
				if (t1 > tmin) {
					tmin = t1;
				}
				if (t2 < tmax) {
					tmax = t2;
				}
				if (tmin > tmax) {
					return 0;
				}
			}
		}
		return 0x1;
#endif
	}

	DG_INLINE dgFloat32 BoxIntersect(const dgVector& minBox, const dgVector& maxBox) const
	{
		dgVector test(((m_p0 <= minBox) | (m_p0 >= maxBox)) & m_isParallel);
		if (test.GetSignMask() & 0x07) {
			return dgFloat32(1.2f);
		}
		dgVector tt0(m_dpInv * (minBox - m_p0));
		dgVector tt1(m_dpInv * (maxBox - m_p0));
		dgVector t0(m_minT.GetMax(tt0.GetMin(tt1)));
		dgVector t1(m_maxT.GetMin(tt0.GetMax(tt1)));
		t0 = t0.GetMax(t0.ShiftTripleRight());
		t1 = t1.GetMin(t1.ShiftTripleRight());
		t0 = t0.GetMax(t0.ShiftTripleRight());
		t1 = t1.GetMin(t1.ShiftTripleRight());
		dgVector mask(t0 < t1);
		dgVector maxDist(dgFloat32(1.2f));
		t0 = (t0 & mask) | maxDist.AndNot(mask);
		dgAssert((mask.GetSignMask() & 1) == (t0.m_x < dgFloat32(1.0f)));
		return t0.GetScalar();
	}

	dgVector m_p0;
	dgVector m_p1;
	dgVector m_diff;
	dgVector m_dpInv;
	dgVector m_minT;
	dgVector m_maxT;
	dgVector m_unitDir;
	dgVector m_isParallel;
} DG_GCC_VECTOR_ALIGMENT;


DG_MSC_VECTOR_ALIGMENT 
class dgFastAABBInfo: public dgObb
{
	public:
	DG_INLINE dgFastAABBInfo()
		:dgObb()
		,m_separationDistance(dgFloat32(1.0e10f))
		,m_absDir(dgGetIdentityMatrix())
		
	{
	}

	DG_INLINE dgFastAABBInfo(const dgMatrix& matrix, const dgVector& size)
		:dgObb(matrix, size)
		,m_separationDistance(dgFloat32(1.0e10f))
	{
		SetTransposeAbsMatrix (matrix);
		dgVector size1 (matrix[0].Abs().Scale(size.m_x) + matrix[1].Abs().Scale(size.m_y) + matrix[2].Abs().Scale(size.m_z));
		m_p0 = (matrix[3] - size1) & dgVector::m_triplexMask;
		m_p1 = (matrix[3] + size1) & dgVector::m_triplexMask;
	}

	DG_INLINE dgFastAABBInfo(const dgVector& p0, const dgVector& p1)
		:dgObb(dgGetIdentityMatrix(), dgVector::m_half * (p1 - p0))
		,m_separationDistance(dgFloat32(1.0e10f))
		,m_absDir(dgGetIdentityMatrix())
		,m_p0(p0)
		,m_p1(p1)
	{
		m_posit = ((dgVector::m_half * (p1 + p0)) & dgVector::m_triplexMask) | dgVector::m_wOne;
	}

	DG_INLINE void SetTransposeAbsMatrix (const dgMatrix& matrix)
	{
		m_absDir = matrix.Transpose();
		m_absDir[0] = m_absDir[0].Abs();
		m_absDir[1] = m_absDir[1].Abs();
		m_absDir[2] = m_absDir[2].Abs();
		//m_absDir[3] = dgVector::m_wOne;
	}

	DG_INLINE dgFloat32 PolygonBoxRayDistance (const dgVector& faceNormal, dgInt32 indexCount, const dgInt32* const indexArray, dgInt32 stride, const dgFloat32* const vertexArray, const dgFastRayTest& ray) const
	{
		dgVector minBox;
		dgVector maxBox;
		MakeBox1 (indexCount, indexArray, stride, vertexArray, minBox, maxBox);
		dgFloat32 dist0 = ray.BoxIntersect(minBox, maxBox);
		if (dist0 < dgFloat32 (1.0f)) {
			dgMatrix faceMatrix (MakeFaceMatrix (faceNormal, indexCount, indexArray, stride, vertexArray));

			MakeBox2 (faceMatrix, indexCount, indexArray, stride, vertexArray, minBox, maxBox);
			dgVector veloc (faceMatrix.RotateVector(ray.m_diff) & dgVector::m_triplexMask);
			dgFastRayTest localRay (dgVector (dgFloat32 (0.0f)), veloc);
			dgFloat32 dist1 = localRay.BoxIntersect(minBox, maxBox);
			dist0 = dgMax (dist1, dist0);
		}
		return dist0;
	}


	DG_INLINE dgFloat32 PolygonBoxDistance (const dgVector& faceNormal, dgInt32 indexCount, const dgInt32* const indexArray, dgInt32 stride, const dgFloat32* const vertexArray) const
	{
		dgVector minBox;
		dgVector maxBox;
		MakeBox1 (indexCount, indexArray, stride, vertexArray, minBox, maxBox);
		dgVector mask(minBox * maxBox < dgVector(dgFloat32(0.0f)));
		dgVector dist(maxBox.GetMin(minBox.Abs()) & mask);
		dist = dist.GetMin(dist.ShiftTripleRight());
		dist = dist.GetMin(dist.ShiftTripleRight());
		dgFloat32 dist0 = dist.GetScalar();
		if (dist0 > dgFloat32 (0.0f)) {
			dgMatrix faceMatrix (MakeFaceMatrix (faceNormal, indexCount, indexArray, stride, vertexArray));
			MakeBox2 (faceMatrix, indexCount, indexArray, stride, vertexArray, minBox, maxBox);
			dgVector mask2(minBox * maxBox < dgVector(dgFloat32(0.0f)));
			dgVector dist2(maxBox.GetMin(minBox.Abs()) & mask2);
			dist2 = dist2.GetMin(dist2.ShiftTripleRight());
			dist2 = dist2.GetMin(dist2.ShiftTripleRight());
			dgFloat32 dist1 = dist2.GetScalar();
			dist0 = (dist1 > dgFloat32 (0.0f)) ? dgMax (dist0, dist1) : dgFloat32 (0.0f);
			if (dist0 <= dgFloat32(0.0f)) {
				dgVector p1p0((minBox.Abs()).GetMin(maxBox.Abs()).AndNot(mask2));
				dist2 = p1p0.DotProduct(p1p0);
				dist2 = dist2.Sqrt() * dgVector::m_negOne;
				dist0 = dist2.GetScalar();
			}
		} else {
			dgVector p1p0((minBox.Abs()).GetMin(maxBox.Abs()).AndNot(mask));
			dist = p1p0.DotProduct(p1p0);
			dist = dist.Sqrt() * dgVector::m_negOne;
			dist0 = dist.GetScalar();
		}
		return	dist0;
	}

	private:
	DG_INLINE void MakeBox1 (dgInt32 indexCount, const dgInt32* const indexArray, dgInt32 stride, const dgFloat32* const vertexArray, dgVector& minBox, dgVector& maxBox) const
	{
		dgVector faceBoxP0 (&vertexArray[indexArray[0] * stride]);
		dgVector faceBoxP1 (faceBoxP0);
		for (dgInt32 i = 1; i < indexCount; i ++) {
			dgVector p (&vertexArray[indexArray[i] * stride]);
			faceBoxP0 = faceBoxP0.GetMin(p); 
			faceBoxP1 = faceBoxP1.GetMax(p); 
		}

		minBox = faceBoxP0 - m_p1;
		maxBox = faceBoxP1 - m_p0;
	}

	DG_INLINE void MakeBox2 (const dgMatrix& faceMatrix, dgInt32 indexCount, const dgInt32* const indexArray, dgInt32 stride, const dgFloat32* const vertexArray, dgVector& minBox, dgVector& maxBox) const
	{
		dgVector faceBoxP0 (faceMatrix.TransformVector (dgVector (&vertexArray[indexArray[0] * stride])));
		dgVector faceBoxP1 (faceBoxP0);
		for (dgInt32 i = 1; i < indexCount; i ++) {
			dgVector p (faceMatrix.TransformVector (dgVector (&vertexArray[indexArray[i] * stride])));
			faceBoxP0 = faceBoxP0.GetMin(p); 
			faceBoxP1 = faceBoxP1.GetMax(p); 
		}
		faceBoxP0 = faceBoxP0 & dgVector::m_triplexMask;
		faceBoxP1 = faceBoxP1 & dgVector::m_triplexMask;

		dgMatrix matrix = *this * faceMatrix;
		dgVector size (matrix[0].Abs().Scale(m_size.m_x) + matrix[1].Abs().Scale(m_size.m_y) + matrix[2].Abs().Scale(m_size.m_z));
		dgVector boxP0 ((matrix.m_posit - size) & dgVector::m_triplexMask);
		dgVector boxP1 ((matrix.m_posit + size) & dgVector::m_triplexMask);

		minBox = faceBoxP0 - boxP1;
		maxBox = faceBoxP1 - boxP0;
	}


	DG_INLINE dgMatrix MakeFaceMatrix (const dgVector& faceNormal, dgInt32 indexCount, const dgInt32* const indexArray, dgInt32 stride, const dgFloat32* const vertexArray) const
	{
		dgMatrix faceMatrix;
		dgVector origin (&vertexArray[indexArray[0] * stride]);

		faceMatrix[0] = faceNormal;
		faceMatrix[1] = dgVector (&vertexArray[indexArray[1] * stride]) - origin;
		//faceMatrix[1] = faceMatrix[1] * (faceMatrix[1].DotProduct(faceMatrix[1]).InvSqrt());
		faceMatrix[1] = faceMatrix[1].Normalize();
		faceMatrix[2] = faceMatrix[0].CrossProduct(faceMatrix[1]);
		faceMatrix[3] = origin | dgVector::m_wOne; 
		return faceMatrix.Inverse();
	}

	protected:
	dgMatrix m_absDir;
	mutable dgFloat32 m_separationDistance;
//	dgFloat32 m_padding[3];
	dgVector m_p0;
	dgVector m_p1;

	friend class dgAABBPolygonSoup;
	friend class dgCollisionUserMesh;
	friend class dgCollisionHeightField;
} DG_GCC_VECTOR_ALIGMENT;


#endif

