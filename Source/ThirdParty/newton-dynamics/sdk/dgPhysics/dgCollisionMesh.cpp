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
#include "dgCollisionMesh.h"
#include "dgCollisionConvexPolygon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


dgPolygonMeshDesc::dgPolygonMeshDesc(dgCollisionParamProxy& proxy, void* const userData)
	:dgFastAABBInfo()
	,m_boxDistanceTravelInMeshSpace(dgFloat32 (0.0f))
	,m_threadNumber(proxy.m_threadIndex)
	,m_faceCount(0)
	,m_vertexStrideInBytes(0)
	,m_skinThickness(proxy.m_skinThickness)
	,m_userData (userData)
	,m_objBody (proxy.m_body0)
	,m_polySoupBody(proxy.m_body1)
	,m_convexInstance(proxy.m_instance0)
	,m_polySoupInstance(proxy.m_instance1)
	,m_vertex(NULL)
	,m_faceIndexCount(NULL)
	,m_faceVertexIndex(NULL)
	,m_faceIndexStart(NULL)
	,m_hitDistance(NULL)
	,m_maxT(dgFloat32 (1.0f))
	,m_doContinuesCollisionTest(proxy.m_continueCollision)
{
	dgAssert (m_polySoupInstance->IsType (dgCollision::dgCollisionMesh_RTTI));
	dgAssert (m_convexInstance->IsType (dgCollision::dgCollisionConvexShape_RTTI));

	const dgMatrix& hullMatrix = m_convexInstance->GetGlobalMatrix();
	const dgMatrix& soupMatrix = m_polySoupInstance->GetGlobalMatrix();

	dgMatrix& matrix = *this;
	matrix = hullMatrix * soupMatrix.Inverse();
	dgMatrix convexMatrix (dgGetIdentityMatrix());

	switch (m_polySoupInstance->GetScaleType())
	{
		case dgCollisionInstance::m_unit:
		{
			break;
		}

		case dgCollisionInstance::m_uniform:
		{
			const dgVector& invScale = m_polySoupInstance->GetInvScale();
			convexMatrix[0][0] = invScale.GetScalar();
			convexMatrix[1][1] = invScale.GetScalar();
			convexMatrix[2][2] = invScale.GetScalar();
			matrix.m_posit = matrix.m_posit * (invScale | dgVector::m_wOne);
			break;
		}

		case dgCollisionInstance::m_nonUniform:
		{
			const dgVector& invScale = m_polySoupInstance->GetInvScale();
			dgMatrix tmp (matrix[0] * invScale, matrix[1] * invScale, matrix[2] * invScale, dgVector::m_wOne);
			convexMatrix = tmp * matrix.Inverse();
			convexMatrix.m_posit = dgVector::m_wOne;
			matrix.m_posit = matrix.m_posit * (invScale | dgVector::m_wOne);
			break;
		}

		case dgCollisionInstance::m_global:
		default:
		{
		   dgAssert (0);
		}
	}

	dgMatrix fullMatrix (convexMatrix * matrix);
	m_convexInstance->CalcAABB(fullMatrix, m_p0, m_p1);

	dgVector p0;
	dgVector p1;
	SetTransposeAbsMatrix(matrix);
	m_convexInstance->CalcAABB(convexMatrix, p0, p1);
	m_size = dgVector::m_half * (p1 - p0);
	m_posit = matrix.TransformVector(dgVector::m_half * (p1 + p0));
	dgAssert (m_posit.m_w == dgFloat32 (1.0f));
}

void dgPolygonMeshDesc::SortFaceArray ()
{
	dgInt32 stride = 8;
	if (m_faceCount >= 8) {
		dgInt32 stack[DG_MAX_COLLIDING_FACES][2];

		stack[0][0] = 0;
		stack[0][1] = m_faceCount - 1;
		dgInt32 stackIndex = 1;
		while (stackIndex) {
			stackIndex --;
			dgInt32 lo = stack[stackIndex][0];
			dgInt32 hi = stack[stackIndex][1];
			if ((hi - lo) > stride) {
				dgInt32 i = lo;
				dgInt32 j = hi;
				dgFloat32 dist = m_hitDistance[(lo + hi) >> 1];
				do {    
					while (m_hitDistance[i] < dist) i ++;
					while (m_hitDistance[j] > dist) j --;

					if (i <= j)	{
						dgSwap (m_hitDistance[i], m_hitDistance[j]);
						dgSwap (m_faceIndexStart[i], m_faceIndexStart[j]);
						dgSwap (m_faceIndexCount[i], m_faceIndexCount[j]);
						i++; 
						j--;
					}
				} while (i <= j);

				if (i < hi) {
					stack[stackIndex][0] = i;
					stack[stackIndex][1] = hi;
					stackIndex ++;
				}
				if (lo < j) {
					stack[stackIndex][0] = lo;
					stack[stackIndex][1] = j;
					stackIndex ++;
				}
				dgAssert (stackIndex < dgInt32 (sizeof (stack) / (2 * sizeof (stack[0][0]))));
			}
		}
	}

	stride = stride * 2;
	if (m_faceCount < stride) {
		stride = m_faceCount;
	}
	for (dgInt32 i = 1; i < stride; i ++) {
		if (m_hitDistance[i] < m_hitDistance[0]) {
			dgSwap (m_hitDistance[i], m_hitDistance[0]);
			dgSwap (m_faceIndexStart[i], m_faceIndexStart[0]);
			dgSwap (m_faceIndexCount[i], m_faceIndexCount[0]);
		}
	}

	for (dgInt32 i = 1; i < m_faceCount; i ++) {
		dgInt32 j = i;
		dgInt32 ptr = m_faceIndexStart[i];
		dgInt32 count = m_faceIndexCount[i];
		dgFloat32 dist = m_hitDistance[i];
		for ( ; dist < m_hitDistance[j - 1]; j --) {
			dgAssert (j > 0);
			m_hitDistance[j] = m_hitDistance [j-1];
			m_faceIndexStart[j] = m_faceIndexStart[j-1];
			m_faceIndexCount[j] = m_faceIndexCount[j-1];
		}
		m_hitDistance[j] = dist;
		m_faceIndexStart[j] = ptr;
		m_faceIndexCount[j] = count;
	}

#ifdef _DEBUG
	for (dgInt32 i = 0; i < m_faceCount - 1; i ++) {
		dgAssert (m_hitDistance[i] <= m_hitDistance[i+1]);
	}
#endif
}


dgCollisionMesh::dgCollisionMesh(dgWorld* const world, dgCollisionID type)
	:dgCollision(world->GetAllocator(), 0, type)
{
	m_rtti |= dgCollisionMesh_RTTI;
	m_debugCallback = NULL;
	SetCollisionBBox (dgVector (dgFloat32 (0.0f)), dgVector (dgFloat32 (0.0f)));
}

dgCollisionMesh::dgCollisionMesh (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollision(world, deserialization, userData, revisionNumber)
{
	dgAssert (m_rtti | dgCollisionMesh_RTTI);

	m_debugCallback = NULL;
	SetCollisionBBox (dgVector (dgFloat32 (0.0f)), dgVector (dgFloat32 (0.0f)));
}

dgCollisionMesh::~dgCollisionMesh()
{
}

void dgCollisionMesh::SetCollisionBBox (const dgVector& p0, const dgVector& p1)
{
	dgAssert (p0.m_x <= p1.m_x);
	dgAssert (p0.m_y <= p1.m_y);
	dgAssert (p0.m_z <= p1.m_z);

	m_boxSize = (p1 - p0).Scale (dgFloat32 (0.5f)) & dgVector::m_triplexMask;
	m_boxOrigin = (p1 + p0).Scale (dgFloat32 (0.5f)) & dgVector::m_triplexMask; 
}

dgInt32 dgCollisionMesh::CalculateSignature () const
{
	dgAssert (0);
	return 0;
}


dgInt32 dgCollisionMesh::CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const
{
	return 0;
}

void dgCollisionMesh::SetDebugCollisionCallback (dgCollisionMeshCollisionCallback debugCallback)
{
	m_debugCallback = debugCallback;
}




#ifdef DG_DEBUG_AABB
dgVector dgCollisionMesh::BoxSupportMapping  (const dgVector& dir) const
{
	return dgVector (dir.m_x < dgFloat32 (0.0f) ? m_p0.m_x : m_p1.m_x, 
					 dir.m_y < dgFloat32 (0.0f) ? m_p0.m_y : m_p1.m_y, 
					 dir.m_z < dgFloat32 (0.0f) ? m_p0.m_z : m_p1.m_z, dgFloat32 (0.0f));
}
#endif




dgVector dgCollisionMesh::CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& plane, const dgCollisionInstance& parentScale) const
{
	return dgVector (dgFloat32 (0.0f));
}


void dgCollisionMesh::DebugCollision (const dgMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgAssert (0);
}


dgFloat32 dgCollisionMesh::GetVolume () const
{
//	dgAssert (0);
	return dgFloat32 (0.0f); 
}

dgFloat32 dgCollisionMesh::GetBoxMinRadius () const
{
	return dgFloat32 (0.0f);  
}

dgFloat32 dgCollisionMesh::GetBoxMaxRadius () const
{
	return dgFloat32 (0.0f);  
}



void dgCollisionMesh::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgAssert (0);
	dgCollision::GetCollisionInfo(info);
//	info->m_offsetMatrix = GetLocalMatrix();
}

void dgCollisionMesh::Serialize(dgSerialize callback, void* const userData) const
{
	dgAssert (0);
}

dgVector dgCollisionMesh::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgAssert (0);
	return dgVector (0, 0, 0, 0);
}


void dgCollisionMesh::CalcAABB(const dgMatrix& matrix, dgVector &p0, dgVector &p1) const
{
	dgVector origin (matrix.TransformVector(m_boxOrigin));
	dgVector size (matrix.m_front.Abs().Scale(m_boxSize.m_x) + matrix.m_up.Abs().Scale(m_boxSize.m_y) + matrix.m_right.Abs().Scale(m_boxSize.m_z));

	p0 = (origin - size) & dgVector::m_triplexMask;
	p1 = (origin + size) & dgVector::m_triplexMask;
}


dgInt32 dgCollisionMesh::CalculatePlaneIntersection (const dgFloat32* const vertex, const dgInt32* const index, dgInt32 indexCount, dgInt32 stride, const dgPlane& localPlane, dgVector* const contactsOut) const
{
	dgInt32 count = 0;
	dgInt32 j = index[indexCount - 1] * stride;
	dgVector p0 (&vertex[j]);
	dgFloat32 side0 = localPlane.Evalue (p0);
	for (dgInt32 i = 0; i < indexCount; i ++) {
		j = index[i] * stride;
		dgVector p1 (&vertex[j]);
		dgFloat32 side1 = localPlane.Evalue (p1);

		if (side0 < dgFloat32 (0.0f)) {
			if (side1 >= dgFloat32 (0.0f)) {
				dgVector dp (p1 - p0);
				dgAssert (dp.m_w == dgFloat32 (0.0f));
				dgFloat32 t = localPlane.DotProduct(dp).GetScalar();
				dgAssert (dgAbs (t) >= dgFloat32 (0.0f));
				if (dgAbs (t) < dgFloat32 (1.0e-8f)) {
					t = dgSign(t) * dgFloat32 (1.0e-8f);	
				}
				dgAssert (0);
				contactsOut[count] = p0 - dp.Scale (side0 / t);
				count ++;

			} 
		} else if (side1 <= dgFloat32 (0.0f)) {
			dgVector dp (p1 - p0);
			dgAssert (dp.m_w == dgFloat32 (0.0f));
			dgFloat32 t = localPlane.DotProduct(dp).GetScalar();
			dgAssert (dgAbs (t) >= dgFloat32 (0.0f));
			if (dgAbs (t) < dgFloat32 (1.0e-8f)) {
				t = dgSign(t) * dgFloat32 (1.0e-8f);	
			}
			dgAssert (0);
			contactsOut[count] = p0 - dp.Scale (side0 / t);
			count ++;
		}

		side0 = side1;
		p0 = p1;
	}

	return count;
}

