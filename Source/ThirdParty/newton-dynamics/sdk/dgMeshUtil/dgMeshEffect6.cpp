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

// algorithm from paper: Non-Distorted Texture Mapping Using Angle Based Flattening
// by: A. Sheffer and E. de Sturler
// http://www.math.vt.edu/people/sturler/Publications/UIUCDCS-R-2001-2257.pdf
// 
// also improvement from paper: ABF++: Fast and Robust Angle Based Flattening
// http://hal.archives-ouvertes.fr/docs/00/10/56/89/PDF/abf_plus_plus_temp.pdf
// by: Alla Sheffer, Bruno Lévy, Inria Lorraine, Maxim Mogilnitsky and Alexander Bogomyakov
//
// also looking at paper
// Least Squares Conformal Maps for Automatic Texture Atlas Generation
// http://www.cs.jhu.edu/~misha/Fall09/Levy02.pdf
// by Bruno Lévy Sylvain Petitjean Nicolas Ray Jérome Maillot
// for automatic seam and atlas generation 

#include "dgPhysicsStdafx.h"
#include "dgWorld.h"
#include "dgMeshEffect.h"

#define dgABF_MAX_ITERATIONS		5
#define dgABF_TOL2					dgFloat64 (1.0e-12)
#define dgABF_LINEAR_SOLVER_TOL		dgFloat64 (1.0e-14)
#define dgABF_PI					dgFloat64 (3.1415926535)

#define dgABF_UV_TOL2				dgFloat64 (1.0e-8)

#if 1
	#define DG_DEBUG_UV	dgTrace 
#else
	#define DG_DEBUG_UV	
#endif


class dgTriangleAnglesToUV: public dgSymmetricBiconjugateGradientSolve
{
	public:
	dgTriangleAnglesToUV (dgMeshEffect* const mesh, dgInt32 material, dgReportProgress progressReportCallback, void* const userData, const dgFloat64* const pinnedPoint, dgFloat64* const triangleAnglesVector = NULL)
		:m_hessianCoLumnIndex (mesh->GetAllocator())
		,m_hessianCoLumnValue(mesh->GetAllocator())
		,m_mesh(mesh)
		,m_triangleAngles(triangleAnglesVector)
		,m_pinnedPoints(pinnedPoint)
		,m_trianglesCount(0)
		,m_matrixElementCount(0)
		,m_allocated(false)
	{
		dgInt32 mark = m_mesh->IncLRU();
		dgMeshEffect::Iterator iter (*m_mesh);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
				dgEdge *ptr = edge;
				do {
					ptr->m_mark = mark;
					ptr = ptr->m_next;
				} while (ptr != edge);
				m_trianglesCount ++;
				dgAssert (edge->m_next->m_next->m_next == edge);
			}
		}

		m_triangles = (dgEdge**) m_mesh->GetAllocator()->MallocLow (m_trianglesCount * sizeof (dgEdge*));

		dgInt32 count = 0;
		mark = m_mesh->IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
				dgEdge *ptr = edge;
				do {
					ptr->m_incidentFace = count + 1;
					ptr->m_mark = mark;
					ptr = ptr->m_next;
				} while (ptr != edge);
				m_triangles[count] = ptr;
				count ++;
				dgAssert (count <= m_trianglesCount);
			}
		}

		if (!m_triangleAngles) {
			dgAssert (0);
			AnglesFromUV ();
		}
		
		m_uvArray = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (2 * m_mesh->GetVertexCount() * sizeof (dgFloat64));
		mark = m_mesh->IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if (edge->m_mark != mark) {
				dgEdge* ptr = edge;
				dgEdge* uvEdge = edge;
				do {
					if ((uvEdge->m_incidentFace < 0) && (ptr->m_incidentFace > 0)) {
						uvEdge = ptr;
					}
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				dgAssert (0);
/*
				dgInt32 index = dgInt32 (uvEdge->m_userData);
				dgMeshEffect::dgVertexAtribute& attribute = m_mesh->GetAttribute (index);
				m_uvArray[index * 2 + 0] = attribute.m_u0;
				m_uvArray[index * 2 + 1] = attribute.m_v0;
*/
			}
		}

		m_sinTable = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (3 * m_trianglesCount * sizeof (dgFloat64));
		m_cosTable = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (3 * m_trianglesCount * sizeof (dgFloat64));

		// pre-compute sin cos tables
		for (dgInt32 i = 0; i < m_trianglesCount * 3; i ++) {
			m_sinTable[i] = sin (m_triangleAngles[i]);
			m_cosTable[i] = cos (m_triangleAngles[i]);
		}

		m_vertexEdge = (dgEdge**) m_mesh->GetAllocator()->MallocLow (m_mesh->GetVertexCount() * sizeof (dgEdge*));
		mark = m_mesh->IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const vertex = &iter.GetNode()->GetInfo();
			if (vertex->m_mark != mark) {
				dgInt32 index = vertex->m_incidentVertex;
				m_vertexEdge[index] = vertex;
				dgEdge* ptr = vertex;
				do {
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next					;
				} while (ptr != vertex);
			}
		}

		m_gradients = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (2 * m_mesh->GetVertexCount() * sizeof (dgFloat64));
		m_diagonal = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (2 * m_mesh->GetVertexCount() * sizeof (dgFloat64));

		LagrangeOptimization();

		dgAssert (0);
/*
		dgStack<dgMeshEffect::dgVertexAtribute>attribArray (m_mesh->GetCount());
//		dgInt32 attribCount = m_mesh->EnumerateAttributeArray (&attribArray[0]);
		mark = m_mesh->IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if (edge->m_mark != mark) {
				dgInt32 vertexIndex = edge->m_incidentVertex;
				dgEdge* const vertexEdge = m_vertexEdge[vertexIndex];
				dgFloat64 u = m_uvArray[vertexIndex * 2 + 0];
				dgFloat64 v = m_uvArray[vertexIndex * 2 + 1];
				dgEdge* ptr = vertexEdge;
				do {
					if (ptr->m_incidentFace > 0) {
						dgInt32 index = dgInt32 (ptr->m_userData);
						dgMeshEffect::dgVertexAtribute& attribute = m_mesh->GetAttribute (index);
						attribute.m_u0 = u;
						attribute.m_v0 = v;
						attribute.m_material = material;
					}
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != vertexEdge);
			}
		}
*/
		dgAssert (0);
		//m_mesh->ApplyAttributeArray(&attribArray[0], attribCount);
	}

	~dgTriangleAnglesToUV()
	{
		m_mesh->GetAllocator()->FreeLow (m_diagonal);
		m_mesh->GetAllocator()->FreeLow (m_gradients);
		m_mesh->GetAllocator()->FreeLow (m_vertexEdge);
		m_mesh->GetAllocator()->FreeLow (m_sinTable);
		m_mesh->GetAllocator()->FreeLow (m_cosTable);
		m_mesh->GetAllocator()->FreeLow (m_uvArray);
		m_mesh->GetAllocator()->FreeLow (m_triangles);

		if (m_allocated) {
			m_mesh->GetAllocator()->FreeLow (m_triangleAngles);
		}
	}

/*
	void GenerateUVCoordinates ()
	{
		m_mesh->SaveOFF("xxx.off");

		dgStack<dgInt8> attibuteUsed (m_attibuteCount);
		memset (&attibuteUsed[0], 0, attibuteUsed.GetSizeInBytes());
		dgInt32 mark = m_mesh->IncLRU();
		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
			dgEdge* const face = m_betaEdge[i * 3];
			if (face->m_mark != mark) {
				dgEdge* ptr = face;
				do {
					if (ptr->m_incidentFace > 0) {
						dgInt32 index = dgInt32 (ptr->m_userData);
						attibuteUsed[index] = 1;
						m_uvArray[index].m_u0 = dgFloat32 (0.0f);
						m_uvArray[index].m_v0 = dgFloat32 (0.0f);
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != face);

				dgEdge* const twinFace = face->m_twin;
				const dgBigVector& p0 = m_mesh->GetVertex(face->m_incidentVertex);
				const dgBigVector& p1 = m_mesh->GetVertex(twinFace->m_incidentVertex);
				dgBigVector p10 (p1 - p0);
				dgFloat64 e0length = sqrt (p10 % p10);

				ptr = twinFace;
				do {
					if (ptr->m_incidentFace > 0) {
						dgInt32 index = dgInt32 (ptr->m_userData);
						attibuteUsed[index] = 1;
						m_uvArray[index].m_u0 = e0length;
						m_uvArray[index].m_v0 = dgFloat32 (0.0f);
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != twinFace);

				dgList<dgEdge*> stack(m_mesh->GetAllocator());
				stack.Append(face);
				while (stack.GetCount()) {
					dgList<dgEdge*>::dgListNode* const node = stack.GetFirst();
					dgEdge* const face = node->GetInfo();
					stack.Remove (node);
					if (face->m_mark != mark) {
						dgInt32 uvIndex2 = dgInt32 (face->m_prev->m_userData);
						if (!attibuteUsed[uvIndex2]) {

							dgInt32 uvIndex0 = dgInt32 (face->m_userData);
							dgInt32 uvIndex1 = dgInt32 (face->m_next->m_userData);

							dgInt32 edgeIndex0 = GetAlphaLandaIndex (face);
							dgInt32 edgeIndex1 = GetAlphaLandaIndex (face->m_next);
							dgInt32 edgeIndex2 = GetAlphaLandaIndex (face->m_prev);

							dgFloat64 refAngleCos = cos (m_variables[edgeIndex0]);
							dgFloat64 refAngleSin = sin (m_variables[edgeIndex0]);
							dgFloat64 scale = sin (m_variables[edgeIndex1]) / sin (m_variables[edgeIndex2]);

							dgFloat64 du = (m_uvArray[uvIndex1].m_u0 - m_uvArray[uvIndex0].m_u0) * scale;
							dgFloat64 dv = (m_uvArray[uvIndex1].m_v0 - m_uvArray[uvIndex0].m_v0) * scale;
							dgFloat64 u = m_uvArray[uvIndex0].m_u0 + du * refAngleCos - dv * refAngleSin; 
							dgFloat64 v = m_uvArray[uvIndex0].m_v0 + du * refAngleSin + dv * refAngleCos; 

							dgEdge* ptr = face->m_prev;
							do {
								if (ptr->m_incidentFace > 0) {
									dgInt32 index = dgInt32 (ptr->m_userData);
									attibuteUsed[index] = 1;
									m_uvArray[index].m_u0 = u;
									m_uvArray[index].m_v0 = v;
								}
								ptr = ptr->m_twin->m_next;
							} while (ptr != face->m_prev);
						}

						face->m_mark = mark;
						face->m_next->m_mark = mark;
						face->m_prev->m_mark = mark;

						if (face->m_next->m_twin->m_incidentFace > 0) {
							stack.Append(face->m_next->m_twin);
						}

						if (face->m_prev->m_twin->m_incidentFace > 0) {
							stack.Append(face->m_prev->m_twin);
						}
					}
				}
			}
		}
	}
*/

	dgInt32 GetAlphaLandaIndex (const dgEdge* const edge) const
	{
		return edge->m_incidentFace - 1;
	}

	void AnglesFromUV ()
	{
		m_allocated = true;
		m_triangleAngles = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (3 * m_trianglesCount * sizeof (dgFloat64));

		// calculate initial beta angle for each triangle
		for (dgInt32 i = 0; i < m_trianglesCount; i ++) {
			dgEdge* const edge = m_triangles[i];

			const dgBigVector& p0 = m_mesh->GetVertex(edge->m_incidentVertex);
			const dgBigVector& p1 = m_mesh->GetVertex(edge->m_next->m_incidentVertex);
			const dgBigVector& p2 = m_mesh->GetVertex(edge->m_prev->m_incidentVertex);

			dgBigVector e10 (p1 - p0);
			dgBigVector e20 (p2 - p0);
			dgBigVector e12 (p2 - p1);
			dgAssert(e10.m_w == dgFloat32(0.0f));
			dgAssert(e20.m_w == dgFloat32(0.0f));
			dgAssert(e12.m_w == dgFloat32(0.0f));

			e10 = e10.Scale (dgFloat64 (1.0) / sqrt (e10.DotProduct(e10).GetScalar()));
			e20 = e20.Scale (dgFloat64 (1.0) / sqrt (e20.DotProduct(e20).GetScalar()));
			e12 = e20.Scale (dgFloat64 (1.0) / sqrt (e12.DotProduct(e12).GetScalar()));

			m_triangleAngles[i * 3 + 0] = acos (dgClamp(e10.DotProduct(e20).GetScalar(), dgFloat64 (-1.0f), dgFloat64 (1.0f)));
			m_triangleAngles[i * 3 + 1] = acos (dgClamp(e10.DotProduct(e20).GetScalar(), dgFloat64 (-1.0f), dgFloat64 (1.0f)));
			m_triangleAngles[i * 3 + 2] = dgABF_PI - m_triangleAngles[i * 3 + 0] - m_triangleAngles[i * 3 + 1];
		}
	}

/*
	// objective function
	f[u2_,v2_,u3_,v3_,u4_,v4_,u5_,v5_,u6_,v6_] := 
		(cos[a0] * sin[b0] * (u1 - u0) + sin[a0] * sin[b0] * (v1 - v0) - sin[c0] * (u6 - u0)) ^ 2 +
		(cos[a0] * sin[b0] * (v1 - v0) + sin[a0] * sin[b0] * (u1 - u0) - sin[c0] * (v6 - v0)) ^ 2 + 
		(cos[a1] * sin[b1] * (u2 - u0) + sin[a1] * sin[b1] * (v2 - v0) - sin[c1] * (u1 - u0)) ^ 2 +
		(cos[a1] * sin[b1] * (v2 - v0) + sin[a1] * sin[b1] * (u2 - u0) - sin[c1] * (v1 - v0)) ^ 2 + 
		(cos[a2] * sin[b2] * (u5 - u0) + sin[a2] * sin[b2] * (v5 - v0) - sin[c2] * (u2 - u0)) ^ 2 +
		(cos[a2] * sin[b2] * (v5 - v0) + sin[a2] * sin[b2] * (u5 - u0) - sin[c2] * (v2 - v0)) ^ 2 + 
		(cos[a3] * sin[b3] * (u2 - u1) + sin[a3] * sin[b3] * (v2 - v1) - sin[c3] * (u3 - u1)) ^ 2 +
		(cos[a3] * sin[b3] * (v2 - v1) + sin[a3] * sin[b3] * (u2 - u1) - sin[c3] * (v3 - v1)) ^ 2 + 
		(cos[a4] * sin[b4] * (u3 - u1) + sin[a4] * sin[b4] * (v3 - v1) - sin[c4] * (u4 - u1)) ^ 2 +
		(cos[a4] * sin[b4] * (v3 - v1) + sin[a4] * sin[b4] * (u3 - u1) - sin[c4] * (v4 - v1)) ^ 2 + 
		(cos[a5] * sin[b5] * (u4 - u1) + sin[a5] * sin[b5] * (v4 - v1) - sin[c5] * (u6 - u1)) ^ 2 +
		(cos[a5] * sin[b5] * (v4 - v1) + sin[a5] * sin[b5] * (u4 - u1) - sin[c5] * (v6 - v1)) ^ 2 + 
		(cos[a6] * sin[b6] * (u4 - u2) + sin[a6] * sin[b6] * (v4 - v2) - sin[c6] * (u3 - u2)) ^ 2 +
		(cos[a6] * sin[b6] * (v4 - v2) + sin[a6] * sin[b6] * (u4 - u2) - sin[c6] * (v3 - v2)) ^ 2 + 
		(cos[a7] * sin[b7] * (u5 - u2) + sin[a7] * sin[b7] * (v5 - v2) - sin[c7] * (u4 - u2)) ^ 2 +
		(cos[a7] * sin[b7] * (v5 - v2) + sin[a7] * sin[b7] * (u5 - u2) - sin[c7] * (v4 - v2)) ^ 2 + 
		(cos[a8] * sin[b8] * (u5 - u4) + sin[a8] * sin[b8] * (v5 - v4) - sin[c8] * (u6 - u4)) ^ 2 +
		(cos[a8] * sin[b8] * (v5 - v4) + sin[a8] * sin[b8] * (u5 - u4) - sin[c8] * (v6 - v4)) ^ 2
*/
	void TraceObjectiveFunction() const
	{
		DG_DEBUG_UV (("f["));
		for (dgInt32 i = 2; i < m_mesh->GetVertexCount(); i ++) {
			DG_DEBUG_UV (("u%d_,v%d_", i, i));
			if (i != (m_mesh->GetVertexCount() - 1)) {
				DG_DEBUG_UV ((","));
			}
		}
		DG_DEBUG_UV (("] := \n"));

		for (dgInt32 i = 0; i < m_trianglesCount; i ++) {
			dgEdge* const face = m_triangles[i];

			dgInt32 v0 = face->m_incidentVertex;
			dgInt32 v1 = face->m_next->m_incidentVertex;
			dgInt32 v2 = face->m_prev->m_incidentVertex;
			(void)(v0);
			(void)(v1);
			(void)(v2);
			DG_DEBUG_UV (("(cos[a%d] * sin[b%d] * (u%d - u%d) + sin[a%d] * sin[b%d] * (v%d - v%d) - sin[c%d] * (u%d - u%d)) ^ 2 +\n", i, i, v1, v0, i, i, v1, v0, i, v2, v0));
			DG_DEBUG_UV (("(cos[a%d] * sin[b%d] * (v%d - v%d) + sin[a%d] * sin[b%d] * (u%d - u%d) - sin[c%d] * (v%d - v%d)) ^ 2", i, i, v1, v0, i, i, v1, v0, i, v2, v0));
			if (i != (m_trianglesCount - 1)) {
				DG_DEBUG_UV ((" + \n"));
			} else {
				DG_DEBUG_UV (("\n"));
			}
		}
	}

	dgFloat64 CalculateExpression_U_face (const dgEdge* const face) const
	{
		dgInt32 faceIndex = GetAlphaLandaIndex (face);
		dgEdge* const faceStartEdge = m_triangles[faceIndex];

		dgInt32 uvIndex0 = dgInt32 (faceStartEdge->m_incidentVertex);
		dgInt32 uvIndex1 = dgInt32 (faceStartEdge->m_next->m_incidentVertex);
		dgInt32 uvIndex2 = dgInt32 (faceStartEdge->m_prev->m_incidentVertex);

		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		dgInt32 alphaIndex2 = faceIndex * 3 + 2;

		DG_DEBUG_UV (("("));
		DG_DEBUG_UV (("cos(a%d) * sin(b%d) * (u%d - u%d) + ", faceIndex, faceIndex, uvIndex1, uvIndex0));
		DG_DEBUG_UV (("sin(a%d) * sin(b%d) * (v%d - v%d) + ", faceIndex, faceIndex, uvIndex1, uvIndex0));
		DG_DEBUG_UV (("sin(c%d) * (u%d - u%d)", faceIndex, uvIndex2, uvIndex0));
		DG_DEBUG_UV ((")"));

		dgFloat64 gradient = m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1] * (m_uvArray[uvIndex1 * 2] - m_uvArray[uvIndex0 * 2]) + 
							 m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1] * (m_uvArray[uvIndex1 * 2 + 1] - m_uvArray[uvIndex0 * 2 + 1]) +
							 m_sinTable[alphaIndex2] * (m_uvArray[uvIndex2 * 2] - m_uvArray[uvIndex0 * 2]);
		return gradient;
	}

	dgFloat64 CalculateExpression_V_face (const dgEdge* const face) const
	{
		dgInt32 faceIndex = GetAlphaLandaIndex (face);
		dgEdge* const faceStartEdge = m_triangles[faceIndex];

		dgInt32 uvIndex0 = dgInt32 (faceStartEdge->m_incidentVertex);
		dgInt32 uvIndex1 = dgInt32 (faceStartEdge->m_next->m_incidentVertex);
		dgInt32 uvIndex2 = dgInt32 (faceStartEdge->m_prev->m_incidentVertex);

		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		dgInt32 alphaIndex2 = faceIndex * 3 + 2;

		DG_DEBUG_UV (("("));
		DG_DEBUG_UV (("cos(a%d) * sin(b%d) * (v%d - v%d) + ", faceIndex, faceIndex, uvIndex1, uvIndex0));
		DG_DEBUG_UV (("sin(a%d) * sin(b%d) * (u%d - u%d) + ", faceIndex, faceIndex, uvIndex1, uvIndex0));
		DG_DEBUG_UV (("sin(c%d) * (v%d - v%d)", faceIndex, uvIndex2, uvIndex0));
		DG_DEBUG_UV ((")"));

		dgFloat64 gradient = m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1] * (m_uvArray[uvIndex1 * 2 + 1] - m_uvArray[uvIndex0 * 2 + 1]) + 
							 m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1] * (m_uvArray[uvIndex1 * 2] - m_uvArray[uvIndex0 * 2]) +
							 m_sinTable[alphaIndex2] * (m_uvArray[uvIndex2 * 2 + 1] - m_uvArray[uvIndex0 * 2 + 1]);
		return gradient;
	}


	dgFloat64 CalculateGradient_U_Coefficent (const dgEdge* const edge, bool u) const
	{
		DG_DEBUG_UV (("("));
		dgInt32 faceIndex = GetAlphaLandaIndex (edge);
		dgEdge* const faceStartEdge = m_triangles[faceIndex];

		dgFloat64 gradient = dgFloat64 (0.0f);

		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		dgInt32 alphaIndex2 = faceIndex * 3 + 2;
		if (faceStartEdge == edge) {
			if (u) {
				DG_DEBUG_UV ((" - cos(a%d) * sin(b%d) - sin(c%d)", faceIndex, faceIndex, faceIndex));
				gradient = - m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1] - m_sinTable[alphaIndex2];
			} else {
				DG_DEBUG_UV ((" - sin(a%d) * sin(b%d) - sin(c%d)", faceIndex, faceIndex, faceIndex));
				gradient = - m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1] - m_sinTable[alphaIndex2];
			}
		} else if (faceStartEdge->m_next == edge) {
			if (u) {
				DG_DEBUG_UV (("cos(a%d) * sin(b%d)", faceIndex, faceIndex));
				gradient = m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1];
			} else {
				DG_DEBUG_UV (("sin(a%d) * sin(b%d)", faceIndex, faceIndex));
				gradient = m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1];
			}
		} else {
			dgAssert (faceStartEdge->m_prev == edge);
			if (u) {
				DG_DEBUG_UV ((" - sin(c%d)", faceIndex));
				gradient = -m_sinTable[alphaIndex2];
			} else {
				DG_DEBUG_UV (("0"));
			}
		}
		DG_DEBUG_UV ((")"));
		return gradient;
	}

	dgFloat64 CalculateGradient_V_Coefficent (const dgEdge* const edge, bool u) const
	{
		DG_DEBUG_UV (("("));
		dgInt32 faceIndex = GetAlphaLandaIndex (edge);
		dgEdge* const faceStartEdge = m_triangles[faceIndex];

		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		dgInt32 alphaIndex2 = faceIndex * 3 + 2;

		dgFloat64 gradient = dgFloat64 (0.0f);
		if (faceStartEdge == edge) {
			if (!u) {
				DG_DEBUG_UV ((" - cos(a%d) * sin(b%d) - sin(c%d)", faceIndex, faceIndex, faceIndex));
				gradient = - m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1] - m_sinTable[alphaIndex2];
			} else {
				DG_DEBUG_UV ((" - sin(a%d) * sin(b%d) - sin(c%d)", faceIndex, faceIndex, faceIndex));
				gradient = - m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1] - m_sinTable[alphaIndex2];
			}
		} else if (faceStartEdge->m_next == edge) {
			if (!u) {
				DG_DEBUG_UV (("cos(a%d) * sin(b%d)", faceIndex, faceIndex));
				gradient = m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1];
			} else {
				DG_DEBUG_UV (("sin(a%d) * sin(b%d)", faceIndex, faceIndex));
				gradient = m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1];
			}
		} else {
			dgAssert (faceStartEdge->m_prev == edge);
			if (!u) {
				DG_DEBUG_UV ((" - sin(c%d)", faceIndex));
				gradient = -m_sinTable[alphaIndex2];
			} else {
				DG_DEBUG_UV (("0"));
			}
		}
		DG_DEBUG_UV ((")"));
		return gradient;
	}


	dgFloat64 CalculateHessianExpression_U_V (const dgEdge* const face) const
	{
		dgInt32 faceIndex = GetAlphaLandaIndex (face);
		//dgEdge* const faceStartEdge = m_triangles[faceIndex];
		//dgInt32 uvIndex0 = dgInt32 (faceStartEdge->m_incidentVertex);
		//dgInt32 uvIndex1 = dgInt32 (faceStartEdge->m_next->m_incidentVertex);
		//dgInt32 uvIndex2 = dgInt32 (faceStartEdge->m_prev->m_incidentVertex);
		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		//dgInt32 alphaIndex2 = faceIndex * 3 + 2;
		DG_DEBUG_UV (("( - sin(a%d) * sin(b%d))", faceIndex, faceIndex));
		return - m_sinTable[alphaIndex0] * m_sinTable[alphaIndex1];
	}


	dgFloat64 CalculateHessianExpression_V_V (const dgEdge* const face) const
	{
		dgInt32 faceIndex = GetAlphaLandaIndex (face);
		//dgEdge* const faceStartEdge = m_triangles[faceIndex];
		//dgInt32 uvIndex0 = dgInt32 (faceStartEdge->m_incidentVertex);
		//dgInt32 uvIndex1 = dgInt32 (faceStartEdge->m_next->m_incidentVertex);
		//dgInt32 uvIndex2 = dgInt32 (faceStartEdge->m_prev->m_incidentVertex);

		dgInt32 alphaIndex0 = faceIndex * 3;
		dgInt32 alphaIndex1 = faceIndex * 3 + 1;
		dgInt32 alphaIndex2 = faceIndex * 3 + 2;
		DG_DEBUG_UV (("(- cos(a%d) * sin(b%d) - sin(c%d))", faceIndex, faceIndex, faceIndex));
		return - m_cosTable[alphaIndex0] * m_sinTable[alphaIndex1] - m_sinTable[alphaIndex2];
	}


	void CalculateGradientU (dgInt32 vertexIndex)
	{
		// calculate U Gradient derivative
		const dgEdge* const vertex = m_vertexEdge[vertexIndex];
		DG_DEBUG_UV (("du%d =\n", vertexIndex));
		dgFloat64 gradient = dgFloat64 (0.0f);
		const dgEdge* ptr = vertex;
		do {
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr, true) ;
				DG_DEBUG_UV ((" * "));
				gradient += a * CalculateExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr, false);
				DG_DEBUG_UV ((" * "));
				gradient += a * CalculateExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);
		m_gradients[2 * vertexIndex] = - gradient;
		DG_DEBUG_UV (("\n"));

		// calculate diagonal derivative
		DG_DEBUG_UV (("H(u%d,u%d) =\n", vertexIndex, vertexIndex));
		dgFloat64 diagonal = dgFloat64 (0.0f);
		ptr = vertex;
		do {
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 diag = CalculateGradient_U_Coefficent (ptr, true);
				diagonal += diag * diag;
				DG_DEBUG_UV (("^2 +\n"));

				DG_DEBUG_UV (("2 * "));
				diag = CalculateGradient_U_Coefficent (ptr, false);
				diagonal += diag * diag;
				DG_DEBUG_UV (("^2 +\n"));
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);
		dgAssert (diagonal > dgFloat32 (0.0f));

		m_hessianCoLumnValue[m_matrixElementCount] = diagonal;
		m_hessianCoLumnIndex[m_matrixElementCount] = vertexIndex * 2 + 0;
		m_matrixElementCount ++;
		m_diagonal[2 * vertexIndex] = diagonal;
		DG_DEBUG_UV (("\n"));

		// calculate of diagonal UiVi derivative
		DG_DEBUG_UV (("H(u%d,v%d) =\n", vertexIndex, vertexIndex));
		dgFloat64 hessianUV = dgFloat64 (0.0);
		ptr = vertex;
		do {
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr, true);
				DG_DEBUG_UV ((" * "));
				hessianUV += a * CalculateHessianExpression_U_V (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr, false);
				DG_DEBUG_UV ((" * "));
				hessianUV += a * CalculateHessianExpression_V_V (ptr);
				DG_DEBUG_UV ((" +\n"));
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);

		m_hessianCoLumnValue[m_matrixElementCount] = hessianUV;
		m_hessianCoLumnIndex[m_matrixElementCount] = vertexIndex * 2 + 1;
		m_matrixElementCount ++;
		DG_DEBUG_UV (("\n"));


/*
		// calculate off diagonal partial derivatives
		ptr = vertex;
		do {
			// derivative respect to U(i, j)
			dgInt32 vertexIndex2 = ptr->m_twin->m_incidentVertex;
			DG_DEBUG_UV (("H(u%d,u%d) =\n", vertexIndex, vertexIndex2));
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr, true);
				DG_DEBUG_UV ((" * "));
//				gradient += diag * TraceExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr, false);
				DG_DEBUG_UV ((" * "));
//				gradient += diag * TraceExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));
			}

			if (ptr->m_twin->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr->m_twin->m_next, true);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr->m_twin->m_next, false);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));
			}

			// derivative respect to V(i, j)
			DG_DEBUG_UV (("H(u%d,v%d) =\n", vertexIndex, vertexIndex2));
			if (ptr->m_incidentFace > 0) {

				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr, true);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr, false);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));

			}

			if (ptr->m_twin->m_incidentFace > 0) {

				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_U_Coefficent (ptr->m_twin->m_next, true);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_U_Coefficent (ptr->m_twin->m_next, false);
				DG_DEBUG_UV ((" * "));
				//				gradient += diag * TraceExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));

			}

			DG_DEBUG_UV (("\n"));
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);
*/
	}


	void CalculateGradientV (dgInt32 vertexIndex)
	{
		// calculate U Gradient derivative
		const dgEdge* const vertex = m_vertexEdge[vertexIndex];
		DG_DEBUG_UV (("dv%d =\n", vertexIndex));
			
		dgFloat64 gradient = dgFloat64 (0.0f);
		const dgEdge* ptr = vertex;
		do {
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 a = CalculateGradient_V_Coefficent (ptr, true);
				DG_DEBUG_UV ((" * "));
				gradient += a * CalculateExpression_U_face (ptr);
				DG_DEBUG_UV ((" +\n"));

				DG_DEBUG_UV (("2 * "));
				a = CalculateGradient_V_Coefficent (ptr, false);
				DG_DEBUG_UV ((" * "));
				gradient += a * CalculateExpression_V_face (ptr);
				DG_DEBUG_UV ((" +\n"));
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);
		m_gradients[2 * vertexIndex + 1] = - gradient;
		DG_DEBUG_UV (("\n"));


		// calculate diagonal derivative
		DG_DEBUG_UV (("H(v%d,v%d) =\n", vertexIndex, vertexIndex));
		dgFloat64 diagonal = dgFloat64 (0.0f);
		ptr = vertex;
		do {
			if (ptr->m_incidentFace > 0) {
				DG_DEBUG_UV (("2 * "));
				dgAssert (ptr->m_incidentVertex == vertexIndex);
				dgFloat64 diag = CalculateGradient_V_Coefficent (ptr, true);
				diagonal += diag * diag;
				DG_DEBUG_UV (("^2 +\n"));

				DG_DEBUG_UV (("2 * "));
				diag = CalculateGradient_V_Coefficent (ptr, false);
				diagonal += diag * diag;
				DG_DEBUG_UV (("^2 +\n"));
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != vertex);
		dgAssert (diagonal > dgFloat32 (0.0f));

		m_hessianCoLumnValue[m_matrixElementCount] = diagonal;
		m_hessianCoLumnIndex[m_matrixElementCount] = vertexIndex * 2 + 1;
		m_matrixElementCount ++;
		m_diagonal[2 * vertexIndex + 1] = diagonal;
		DG_DEBUG_UV (("\n"));





	}



	void CalculateGradientVectorAndHessianMatrix ()
	{
		// trace objective function
//		TraceObjectiveFunction();

		// trace gradients
		DG_DEBUG_UV (("\n"));
		dgInt32 count = m_mesh->GetVertexCount();
		for (dgInt32 i = 0; i < count; i ++) {
			CalculateGradientU (i);
			CalculateGradientV (i);
		}
		DG_DEBUG_UV (("\n"));
	}


	bool InversePrecoditionerTimeVector (dgFloat64* const out, const dgFloat64* const v) const
	{
		const dgInt32 count = m_mesh->GetVertexCount();
		for (dgInt32 i = 0; i < count; i ++) {
			out[2 * i + 0] = m_pinnedPoints[i] * v[i * 2 + 0] / m_diagonal[2 * i + 0];
			out[2 * i + 1] = m_pinnedPoints[i] * v[i * 2 + 1] / m_diagonal[2 * i + 1];
		}
		return true;
	}


	void MatrixTimeVector (dgFloat64* const out, const dgFloat64* const v) const
	{
/*
		const dgInt32 count = m_mesh->GetVertexCount();
		for (dgInt32 i = 0; i < count; i ++) {
			dgEdge* const vertex = m_vertexEdge[i];
			dgAssert (vertex->m_incidentVertex == i);
			out[i * 2 + 0] = m_diagonal[2 * i + 0] * v[2 * i + 0];
			out[i * 2 + 1] = m_diagonal[2 * i + 1] * v[2 * i + 1];
		}
		DG_DEBUG_UV (("\n"));
		dgInt32 count = m_mesh->GetVertexCount();
		for (dgInt32 i = 0; count; i ++) {
			CalculateHessianDiagonalUU (i);
		}
		DG_DEBUG_UV (("\n"));
*/
	}

	void LagrangeOptimization()
	{
		CalculateGradientVectorAndHessianMatrix ();
		Solve(2 * m_mesh->GetVertexCount(), dgABF_UV_TOL2, m_uvArray, m_gradients);
	}

	dgArray<dgInt32> m_hessianCoLumnIndex;
	dgArray<dgFloat64> m_hessianCoLumnValue;
	dgMeshEffect* m_mesh;
	dgEdge** m_triangles;
	dgEdge** m_vertexEdge;
	dgFloat64* m_uvArray;
	dgFloat64* m_sinTable;
	dgFloat64* m_cosTable;
	dgFloat64* m_gradients;
	dgFloat64* m_diagonal;
	dgFloat64* m_triangleAngles;
	const dgFloat64* m_pinnedPoints;

	dgInt32 m_trianglesCount;
	dgInt32 m_matrixElementCount;
	bool m_allocated;
};

class dgAngleBasedFlatteningMapping: public dgSymmetricBiconjugateGradientSolve
{
	public: 
	dgAngleBasedFlatteningMapping (dgMeshEffect* const mesh, dgInt32 material, dgReportProgress progressReportCallback, void* const userData)
		:m_mesh(mesh)
		,m_progressReportUserData(userData)
		,m_progressReportCallback(progressReportCallback)
	{
dgAssert (0);
/*
		AllocVectors();
		InitEdgeVector();
		CalculateInitialAngles ();
		LagrangeOptimization();

		dgEdge* const face = m_betaEdge[0];
		dgEdge* ptr = face;
		do {
			if (ptr->m_incidentFace > 0) {
				dgInt32 index = dgInt32 (ptr->m_userData);
				dgMeshEffect::dgVertexAtribute& attribute = m_mesh->GetAttribute (index);
				attribute.m_u0 = dgFloat32 (0.0f);
				attribute.m_v0 = dgFloat32 (0.0f);
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != face);

		dgEdge* const twinFace = face->m_twin;
		const dgBigVector& p0 = m_mesh->GetVertex(face->m_incidentVertex);
		const dgBigVector& p1 = m_mesh->GetVertex(twinFace->m_incidentVertex);
		dgBigVector p10 (p1 - p0);
		dgAssert(p10.m_w == dgFloat32(0.0f));
		dgFloat64 e0length = sqrt (p10.DotProduct(p10).GetScalar());

		ptr = twinFace;
		do {
			if (ptr->m_incidentFace > 0) {
				dgInt32 index = dgInt32 (ptr->m_userData);
				dgMeshEffect::dgVertexAtribute& attribute = m_mesh->GetAttribute (index);
				attribute.m_u0 = e0length;
				attribute.m_v0 = dgFloat32 (0.0f);
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != twinFace);

		DeleteAuxiliaryVectors();

		m_deltaVariables[0] = 0.0f;
		m_deltaVariables[1] = 0.0f;
		for (dgInt32 i = 2; i < m_totalVariablesCount; i ++) {
			m_deltaVariables[i] = 1.0f;
		}
		dgTriangleAnglesToUV anglesToUV (mesh, material, progressReportCallback, userData, m_deltaVariables, m_variables);
*/
	}

	~dgAngleBasedFlatteningMapping()
	{
		m_mesh->GetAllocator()->FreeLow (m_variables);
		m_mesh->GetAllocator()->FreeLow (m_deltaVariables);
	}


	void AllocVectors()
	{
		CalculateNumberOfVariables();
		dgInt32 vertexCount = m_mesh->GetVertexCount();

		// alloc intermediate vectors
		m_betaEdge = (dgEdge**) m_mesh->GetAllocator()->MallocLow(m_anglesCount * sizeof (dgEdge*));
		m_interiorIndirectMap = (dgInt32*) m_mesh->GetAllocator()->MallocLow (vertexCount * sizeof (dgInt32));
		m_beta = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_anglesCount * sizeof (dgFloat64));
		m_weight= (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_anglesCount * sizeof (dgFloat64));
		m_sinTable = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_anglesCount * sizeof (dgFloat64));
		m_cosTable = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_anglesCount * sizeof (dgFloat64));
		m_gradients = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_totalVariablesCount * sizeof (dgFloat64));
		
		// allocate angle and internal vertex vector
		m_variables = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_totalVariablesCount * sizeof (dgFloat64));
		m_deltaVariables = (dgFloat64*) m_mesh->GetAllocator()->MallocLow (m_totalVariablesCount * sizeof (dgFloat64));
	}

	void DeleteAuxiliaryVectors()
	{
		// delete intermediate vectors
		m_mesh->GetAllocator()->FreeLow (m_betaEdge);
		m_mesh->GetAllocator()->FreeLow (m_interiorIndirectMap);
		m_mesh->GetAllocator()->FreeLow (m_sinTable);
		m_mesh->GetAllocator()->FreeLow (m_cosTable);
		m_mesh->GetAllocator()->FreeLow (m_beta);
		m_mesh->GetAllocator()->FreeLow (m_weight);
		m_mesh->GetAllocator()->FreeLow (m_gradients);
		
		m_beta = NULL;
		m_weight = NULL;
		m_betaEdge = NULL;
		m_sinTable = NULL;
		m_cosTable = NULL;
		m_gradients = NULL;
		m_interiorIndirectMap = NULL;
	}


	void CalculateNumberOfVariables()
	{
		//m_mesh->SaveOFF("xxx.off");
		m_anglesCount = 0;
		m_triangleCount = 0;
		m_interiorVertexCount = 0;

		dgInt32 mark = m_mesh->IncLRU();
		dgMeshEffect::Iterator iter (*m_mesh);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
				dgEdge *ptr = edge;
				do {
					m_anglesCount ++;
					ptr->m_mark = mark;
					ptr = ptr->m_next;
				} while (ptr != edge);
				m_triangleCount ++;
				dgAssert (edge->m_next->m_next->m_next == edge);
			}
		}

		mark = m_mesh->IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
				bool isInterior = true;
				dgEdge *ptr = edge;
				do {
					isInterior &= (ptr->m_incidentFace > 0);
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				m_interiorVertexCount += isInterior ? 1 : 0;
			}
		}
		m_totalVariablesCount = m_anglesCount + m_triangleCount + 2 * m_interiorVertexCount;
	}

	void InitEdgeVector()
	{
		dgInt32 count = 0;
		dgInt32 mark = m_mesh->IncLRU();
		dgMeshEffect::Iterator iter (*m_mesh);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
				dgEdge *ptr = edge;
				do {
					ptr->m_mark = mark;
					m_betaEdge[count] = ptr;
					ptr->m_incidentFace = count + 1;
					count ++;
					dgAssert (count <= m_anglesCount);
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}

		count = 0;
		mark = m_mesh->IncLRU();		
		memset (m_interiorIndirectMap, -1, m_mesh->GetVertexCount() * sizeof (m_interiorIndirectMap[0]));
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {

				bool isInterior = true;
				dgEdge* ptr = edge;
				do {
					isInterior &= (ptr->m_incidentFace > 0);
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				if (isInterior) {
					m_interiorIndirectMap[edge->m_incidentVertex] = m_anglesCount + m_triangleCount + count;
					count ++;
				}
			}
		}
	}

	dgInt32 GetAlphaLandaIndex (const dgEdge* const edge) const
	{
		return edge->m_incidentFace - 1;
	}

	dgInt32 GetTriangleIndex (const dgInt32 alphaIndex) const
	{
		return alphaIndex / 3 + m_anglesCount;
	}

	dgInt32 GetTriangleIndex (const dgEdge* const edge) const
	{
		return GetAlphaLandaIndex(edge) / 3 + m_anglesCount;
	}

	dgInt32 GetInteriorVertex(const dgEdge* const edge) const
	{
		return m_interiorIndirectMap[edge->m_incidentVertex];
	}

	void CalculateInitialAngles ()
	{
		// calculate initial beta angle for each triangle
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgEdge* const edge = m_betaEdge[i];

			const dgBigVector& p0 = m_mesh->GetVertex(edge->m_incidentVertex);
			const dgBigVector& p1 = m_mesh->GetVertex(edge->m_next->m_incidentVertex);
			const dgBigVector& p2 = m_mesh->GetVertex(edge->m_prev->m_incidentVertex);

			dgBigVector e10 (p1 - p0);
			dgBigVector e20 (p2 - p0);

			e10 = e10.Scale (dgFloat64 (1.0) / sqrt (e10.DotProduct(e10).GetScalar()));
			e20 = e20.Scale (dgFloat64 (1.0) / sqrt (e20.DotProduct(e20).GetScalar()));
			dgAssert(e10.m_w == dgFloat32(0.0f));
			dgAssert(e20.m_w == dgFloat32(0.0f));

			m_beta[i] = acos (dgClamp(e10.DotProduct(e20).GetScalar(), dgFloat64 (-1.0f), dgFloat64 (1.0f)));
			dgAssert (m_beta[i] > dgFloat64 (0.0f));
		}

		#ifdef _DEBUG
		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
			dgInt32 i0 = i * 3 + 0;
			dgInt32 i1 = i * 3 + 1;
			dgInt32 i2 = i * 3 + 2;
			dgAssert (fabs (m_beta[i0] + m_beta[i1] + m_beta[i2] - dgABF_PI) < dgFloat64 (1.0e-6f));
		}
		#endif

		// for each interior vertex apply the scale factor
		dgInt32 mark = m_mesh->IncLRU();
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgEdge* const edge = m_betaEdge[i];
			if ((edge->m_mark != mark) && (GetInteriorVertex(edge) >= 0)) {
				dgFloat64 scale = dgFloat64 (0.0f);
				dgEdge* ptr = edge; 
				do {
					dgInt32 index = GetAlphaLandaIndex (ptr);
					dgAssert (index >= 0);
					dgAssert (index <= m_anglesCount);
					scale += m_beta[index];
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				dgAssert (scale > dgFloat32 (0.0f));

				scale = dgFloat64 (2.0f) * dgABF_PI / scale;
				ptr = edge;
				do {
					dgInt32 index = GetAlphaLandaIndex (ptr);
					dgAssert (index >= 0);
					dgAssert (index <= m_anglesCount);
					m_beta[index] *= scale;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
			}
		}

		// initialized each alpha lambda to the beta angle and also calcual ethe derivatoe coeficent (2.0 / (betai * betai)) 
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgAssert (m_beta[i] > dgFloat64 (0.0f));
			m_variables[i] = m_beta[i];
			m_weight[i] = dgFloat64 (2.0f) / (m_beta[i] * m_beta[i]);
		}
	}

	// angular derivative component
	// (wi * (xi - bi) + T0
	// where wi = 2.0 / (bi ^ 2)
	dgFloat64 CalculateAngularGradientDerivative (dgInt32 alphaIndex) const
	{
		dgFloat64 gradient = (m_variables[alphaIndex] - m_beta[alphaIndex]) * m_weight[alphaIndex] + m_variables[GetTriangleIndex(alphaIndex)];
		dgAssert (fabs(gradient) < dgFloat64(1.0e10f));
		return gradient;
	}

	// Vi if the the edge is an interior vertex
	dgFloat64 CalculateInteriorVertexGradient (dgInt32 alphaIndex) const
	{
		dgInt32 index = GetInteriorVertex(m_betaEdge[alphaIndex]);
		dgFloat64 gradient = (index != -1) ? m_variables[index] : dgFloat32 (0.0f);
		dgAssert (fabs(gradient) < dgFloat64(1.0e10f));
		return gradient;
	}

	// Wj * cos(alpha) * sum (alphai) for eadh previsu or next interior incdent vertex
	dgFloat64 CalculatePlanarityGradient (dgInt32 alphaIndex) const
	{
		dgFloat64 gradient = dgFloat64 (0.0f);

		dgEdge* const incidentEdge = m_betaEdge[alphaIndex];

		if (GetInteriorVertex (incidentEdge->m_next) != -1) {
			dgEdge* const edge = m_betaEdge[GetAlphaLandaIndex(incidentEdge->m_next)];
			dgFloat64 product = m_cosTable[GetAlphaLandaIndex(edge->m_prev)];
			dgEdge* ptr = edge->m_twin->m_next;
			do {
				product *= m_sinTable[GetAlphaLandaIndex(ptr->m_prev)];
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			dgInt32 interiorVertexIndex = GetInteriorVertex (incidentEdge->m_next) + m_interiorVertexCount;
			gradient -= m_variables[interiorVertexIndex] * product;
		}

		if (GetInteriorVertex (incidentEdge->m_prev) != -1) {
			dgEdge* const edge = m_betaEdge[GetAlphaLandaIndex(incidentEdge->m_prev)];
			dgFloat64 product = m_cosTable[GetAlphaLandaIndex(edge->m_next)];
			dgEdge* ptr = edge->m_twin->m_next;
			do {
				product *= m_sinTable[GetAlphaLandaIndex(ptr->m_next)];
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			dgInt32 interiorVertexIndex = GetInteriorVertex (incidentEdge->m_prev) + m_interiorVertexCount;
			gradient += m_variables[interiorVertexIndex] * product;
		}
		dgAssert (fabs(gradient) < dgFloat64(1.0e10f));
		return gradient;
	}

	// sample of the Gradient Vector according to Mathematic to a generic mesh, this can be generalize for and arbitrary mesh topology
	// x0 - x14 are the planar angles in 2d
	// b0 - b14 are the mesh angles in 3d.
	// T0 - T4 are the triangle lambdas
	// V0 - V1 interior vertex lambdas
	// W0 - W1 interior vertex wheel lambdas 
	//
    // Gradient derivatives: 
	//0   (2 (-b0 + x0))/b0^2 + T0 + W2 Cos[x0] Sin[x5] Sin[x9] 
	//1   (2 (-b1 + x1))/b1^2 + T0 - W2 Cos[x1] Sin[x10] Sin[x3] 
	//2   (2 (-b2 + x2))/b2^2 + T0 + V2  
	//3   (2 (-b3 + x3))/b3^2 + T1 - W2 Cos[x3] Sin[x1] Sin[x10] + W3 Cos[x3] Sin[x11] Sin[x12] Sin[x8] 
	//4   (2 (-b4 + x4))/b4^2 + T1 + V2 - W3 Cos[x4] Sin[x13] Sin[x6] Sin[x9] 
	//5   (2 (-b5 + x5))/b5^2 + T1 + V3 + W2 Cos[x5] Sin[x0] Sin[x9] 
	//6   (2 (-b6 + x6))/b6^2 + T2 - W3 Cos[x6] Sin[x13] Sin[x4] Sin[x9] 
	//7   (2 (-b7 + x7))/b7^2 + T2 + V3 
	//8   (2 (-b8 + x8))/b8^2 + T2 + W3 Cos[x8] Sin[x11] Sin[x12] Sin[x3] 
	//9   (2 (-b09 + x09))/b09^2 + T3 + W2 Cos[x9] Sin[x0] Sin[x5] - W3 Cos[x9] Sin[x13] Sin[x4] Sin[x6] 
	//10  (2 (-b10 + x10))/b10^2 + T3 + V3 - W2 Cos[x10] Sin[x1] Sin[x3] 
	//11  (2 (-b11 + x11))/b11^2 + T3 + V2 + W3 Cos[x11] Sin[x12] Sin[x3] Sin[x8] 
	//12  (2 (-b12 + x12))/b12^2 + T4 + W3 Cos[x12] Sin[x11] Sin[x3] Sin[x8] 
	//13  (2 (-b13 + x13))/b13^2 + T4 - W3 Cos[x13] Sin[x4] Sin[x6] Sin[x9] 
	//14  (2 (-b14 + x14))/b14^2 + T4 + V3 
	//
	//15  x0 + x1 + x2 - pi
	//16  x3 + x4 + x5 - pi 
	//17  x6 + x7 + x8 - pi  
	//18  x10 + x11 + x9 - pi 
	//19  x12 + x13 + x14 - pi 
	//
	//20  x11 + x2 + x4 - 2 pi 
	//21  x10 + x14 + x5 + x7 - 2 pi 
	//
	//22  Sin[x0] Sin[x5] Sin[x9] - Sin[x1] Sin[x10] Sin[x3] 
	//23  Sin[x11] Sin[x12] Sin[x3] Sin[x8] - Sin[x13] Sin[x4] Sin[x6] Sin[x9]
	dgFloat64 CalculateGradientVector ()
	{
		// pre-compute sin cos tables
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			m_sinTable[i] = sin (m_variables[i]);
			m_cosTable[i] = cos (m_variables[i]);
		}

		dgFloat64 gradientNorm = dgFloat64 (0.0f);

		// calculate gradients due to the difference between a matching edge angle and it projected angle msu be mminimal Wei * (Xei - Bei) ^ e = minimal
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgFloat64 gradient = CalculateAngularGradientDerivative (i) + CalculateInteriorVertexGradient (i) + CalculatePlanarityGradient (i);
			m_gradients[i] = -gradient;
			gradientNorm += gradient * gradient;
		}

		// calculate gradient due to the equality that the sum on the internal angle of a triangle must add to 180 degree. (Xt0 + Xt1 + Xt2 - pi) = 0
		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
			dgFloat64 gradient = m_variables[i * 3 + 0] + m_variables[i * 3 + 1] + m_variables[i * 3 + 2] - dgABF_PI;
			m_gradients[m_anglesCount + i] = -gradient;
			gradientNorm += gradient * gradient;
		}

		// calculate the gradient due to the equality that the sum of all the angle incident to and interior vertex must be 3060 degree sum (Xvi) - 2 * pi = 0 
		dgInt32 mark = m_mesh->IncLRU();
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgEdge* const edge = m_betaEdge[i];

			if ((edge->m_mark != mark) && GetInteriorVertex(edge) != -1) {
				dgInt32 vertexIndex = GetInteriorVertex(edge);
				dgFloat64 gradient = - dgFloat64 (2.0f) * dgABF_PI;

				dgEdge* ptr = edge; 
				do {
					dgInt32 index = GetAlphaLandaIndex(ptr);
					gradient += m_variables[index];
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				m_gradients[vertexIndex] = - gradient;
				gradientNorm += gradient * gradient;
			}
		}

		// calculate the gradient due to the equality that the difference of the product of the sin of the angle to the
		// incident to an interior vertex must be zero product (sin (Xvi + 1) - product (sin (Xvi - 1)  = 0 
		mark = m_mesh->IncLRU();
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			dgEdge* const edge = m_betaEdge[i];

			dgInt32 vertexIndex = GetInteriorVertex(edge);
			if ((edge->m_mark != mark) && (vertexIndex != -1)) {
				vertexIndex += m_interiorVertexCount;
				dgFloat64 partialProdut0 =  dgFloat64 (1.0f);
				dgFloat64 partialProdut1 =  dgFloat64 (1.0f);
				dgEdge* ptr = edge; 
				do {
					dgInt32 index0 = GetAlphaLandaIndex(ptr->m_next);
					dgInt32 index1 = GetAlphaLandaIndex(ptr->m_prev);
					partialProdut0 *= m_sinTable[index0];
					partialProdut1 *= m_sinTable[index1];
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				dgFloat64 gradient = partialProdut0 - partialProdut1;
				m_gradients[vertexIndex] = - gradient;
				gradientNorm += gradient * gradient;
			}
		}

		return gradientNorm;
	}

	// the Hessian matrix is compose of these second partial derivatives
	// these derivatives are too complex and make the solver to spend too much time, 
	// [0][0]  2/b0^2 - W2 Sin[x0] Sin[x5] Sin[x9], 0, 0, 0, 0, W2 Cos[x0] Cos[x5] Sin[x9], 0, 0, 0, W2 Cos[x0] Cos[x9] Sin[x5], 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, Cos[x0] Sin[x5] Sin[x9], 0, 
	// [0][[1] {0, 2/b1^2 + W2 Sin[x1] Sin[x10] Sin[x3], 0, -W2 Cos[x1] Cos[x3] Sin[x10], 0, 0, 0, 0, 0, 0, -W2 Cos[x1] Cos[x10] Sin[x3], 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, -Cos[x1] Sin[x10] Sin[x3], 0}, 
	// ...

	// the optimize version of the algorithms assume that the second derivatives are linear, therefore all sine terms are neglected, I will do the same
	// [ 0][0-n]  2/b0^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  Cos[x0] Sin[x5] Sin[x9], 0 
	// [ 1][0-n]  0, 2/b1^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, -Cos[x1] Sin[x10] Sin[x3], 0 
	// [ 2][0-n]  0, 0, 2/b2^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 
	// [ 3][0-n]  0, 0, 0, 2/b3^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -Cos[x3] Sin[x1] Sin[x10], Cos[x3] Sin[x11] Sin[x12] Sin[x8] 
	// [ 4][0-n]  0, 0, 0, 0, 2/b4^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -Cos[x4] Sin[x13] Sin[x6] Sin[x9]}	
	// [ 5][0-n]  0, 0, 0, 0, 0, 2/b5^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, Cos[x5] Sin[x0] Sin[x9], 0 
	// [ 6][0-n]  0, 0, 0, 0, 0, 0, 2/b6^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -Cos[x6] Sin[x13] Sin[x4] Sin[x9] 
	// [ 7][0-n]  0, 0, 0, 0, 0, 0, 0, 2/b7^2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0
	// [ 8][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 2/b8^2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, Cos[x8] Sin[x11] Sin[x12] Sin[x3] 
	// [ 9][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b9^2,  0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, Cos[x9] Sin[x0] Sin[x5], -Cos[x9] Sin[x13] Sin[x4] Sin[x6]
	// [10][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b10^2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, -Cos[x10] Sin[x1] Sin[x3], 0}, 
	// [11][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b11^2, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, Cos[x11] Sin[x12] Sin[x3] Sin[x8]
	// [12][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b12^2, 0, 0, 0, 0, 0, 0, 1,	0, 0, 0, Cos[x12] Sin[x11] Sin[x3] Sin[x8]
	// [13][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b13^2, 0, 0, 0, 0, 0, 1, 0, 0, 0, -Cos[x13] Sin[x4] Sin[x6] Sin[x9]
	// [14][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2/b14^2, 0, 0, 0, 0, 1, 0, 1, 0, 0

	// [15][0-n]  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [16][0-n]  0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [17][0-n]  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [18][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [19][0-n]  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0

	// [20][0-n]  0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [21][0-n]  0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0

	// [22][0-n]  Cos[x0] Sin[x5] Sin[x9], -Cos[x1] Sin[x10] Sin[x3], 0, -Cos[x3] Sin[x1] Sin[x10], 0, Cos[x5] Sin[x0] Sin[x9], 0, 0, 0, Cos[x9] Sin[x0] Sin[x5], -Cos[x10] Sin[x1] Sin[x3], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	// [23][0-n]  0, 0, 0, Cos[x3] Sin[x11] Sin[x12] Sin[x8], -Cos[x4] Sin[x13] Sin[x6] Sin[x9], 0, -Cos[x6] Sin[x13] Sin[x4] Sin[x9], 0, Cos[x8] Sin[x11] Sin[x12] Sin[x3], -Cos[x9] Sin[x13] Sin[x4] Sin[x6], 0, Cos[x11] Sin[x12] Sin[x3] Sin[x8], Cos[x12] Sin[x11] Sin[x3] Sin[x8], -Cos[x13] Sin[x4] Sin[x6] Sin[x9], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	void MatrixTimeVector (dgFloat64* const out, const dgFloat64* const v) const
	{
		for (dgInt32 i = 0; i < m_interiorVertexCount; i ++) {
			out[i + m_anglesCount + m_triangleCount] = dgFloat64 (0.0f);
			out[i + m_anglesCount + m_triangleCount + m_interiorVertexCount] = dgFloat64 (0.0f);
		}

		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			out[i] = m_weight[i] * v[i];

			dgEdge* const edge = m_betaEdge[i];
			dgInt32 vertexIndex = GetInteriorVertex(edge);
			if (vertexIndex >= 0) {
				out[i] += v[vertexIndex];
				out[vertexIndex] += v[i];
			}
		}

		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
			dgInt32 j = i * 3;
			out[j + 0] += v[i + m_anglesCount];
			out[j + 1] += v[i + m_anglesCount];
			out[j + 2] += v[i + m_anglesCount];
			out[i + m_anglesCount] = v[j + 0] + v[j + 1] +  v[j + 2];
		}

		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			{
				dgEdge* const edge = m_betaEdge[i]->m_prev;
				dgInt32 vertexIndex = GetInteriorVertex(edge);
				if (vertexIndex >= 0) {
					dgInt32 index = GetAlphaLandaIndex(edge->m_next);
					dgFloat64 product = m_cosTable[index];
					dgEdge* ptr = edge->m_twin->m_next; 
					do {
						dgInt32 m = GetAlphaLandaIndex(ptr->m_next);
						product *= m_sinTable[m];
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);
					out[i] += v[vertexIndex + m_interiorVertexCount] * product;
					out[vertexIndex + m_interiorVertexCount] += product * v[i];
				}
			}

			{
				dgEdge* const edge = m_betaEdge[i]->m_next;
				dgInt32 vertexIndex = GetInteriorVertex(edge);
				if (vertexIndex >= 0) {
					dgInt32 index = GetAlphaLandaIndex(edge->m_prev);
					dgFloat64 product = m_cosTable[index];
					dgEdge* ptr = edge->m_twin->m_next; 
					do {
						dgInt32 m = GetAlphaLandaIndex(ptr->m_prev);
						product *= m_sinTable[m];
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);
					out[i] -= v[vertexIndex + m_interiorVertexCount] * product;
					out[vertexIndex + m_interiorVertexCount] -= product * v[i];
				}
			}
		}
	}

	bool InversePrecoditionerTimeVector (dgFloat64* const out, const dgFloat64* const v) const
	{
		for (dgInt32 i = 0; i < m_anglesCount; i ++) {
			out[i] = v[i] / m_weight[i];
		}
		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
			out[i + m_anglesCount] = v[i + m_anglesCount];
		}

		for (dgInt32 i = 0; i < m_interiorVertexCount; i ++) {
			out[i + m_anglesCount + m_triangleCount] = v[i + m_anglesCount + m_triangleCount];
			out[i + m_anglesCount + m_triangleCount + m_interiorVertexCount] = v[i + m_anglesCount + m_triangleCount + m_interiorVertexCount];
		}

		m_progressNum ++;
		if (m_progressReportCallback) {
			if ((m_progressNum & 127) == 127) {
				m_continueExecution = m_progressReportCallback (dgMin (dgFloat32 (m_progressNum) / m_progressDen, dgFloat32 (1.0f)), m_progressReportUserData);
			}
		}
		return m_continueExecution;
	}

	void LagrangeOptimization()
	{
		memset (m_deltaVariables, 0, m_totalVariablesCount * sizeof (dgFloat64));
		memset (&m_variables[m_anglesCount], 0, m_triangleCount * sizeof (dgFloat64));	

		for (dgInt32 i = 0; i < m_interiorVertexCount; i ++) {
			m_variables[i + m_anglesCount + m_triangleCount] = dgFloat32 (1.0f);
			m_variables[i + m_anglesCount + m_triangleCount + m_interiorVertexCount] = dgFloat32 (1.0f);
		}
		
		m_progressNum = 0;
		m_continueExecution = true;

/*
		dgFloat64 gradientNorm = CalculateGradientVector ();
		for (dgInt32 iter = 0; (iter < dgABF_MAX_ITERATIONS) && (gradientNorm > dgABF_TOL2) && m_continueExecution; iter++) {
			m_progressDen = m_progressNum + m_totalVariablesCount;
			Solve(m_totalVariablesCount, dgABF_LINEAR_SOLVER_TOL, m_deltaVariables, m_gradients);
			for (dgInt32 i = 0; i < m_totalVariablesCount; i ++) {
				m_variables[i] += m_deltaVariables[i];
			}
			gradientNorm = CalculateGradientVector ();
		}
*/

#ifdef _DEBUG
		// calculate gradient due to the equality that the sum on the internal angle of a triangle must add to 180 degree. (Xt0 + Xt1 + Xt2 - pi) = 0
//		for (dgInt32 i = 0; i < m_triangleCount; i ++) {
//			dgFloat64 gradient = m_variables[i * 3 + 0] + m_variables[i * 3 + 1] + m_variables[i * 3 + 2] - dgABF_PI;
//			dgAssert (fabs (gradient) < dgFloat64 (1.0e-2f));
//		}
#endif
	}

	dgMeshEffect* m_mesh;
	dgEdge** m_betaEdge;
	dgInt32* m_interiorIndirectMap;

	dgFloat64* m_beta;
	dgFloat64* m_weight;
	dgFloat64* m_sinTable;
	dgFloat64* m_cosTable;
	dgFloat64* m_variables;
	dgFloat64* m_gradients;
	dgFloat64* m_deltaVariables;

	dgInt32 m_anglesCount;
	dgInt32 m_triangleCount;
	dgInt32 m_interiorVertexCount;
	dgInt32 m_totalVariablesCount;

	void* m_progressReportUserData;
	dgReportProgress m_progressReportCallback;
	mutable dgInt32 m_progressNum;
	mutable dgInt32 m_progressDen;
	mutable bool m_continueExecution;
};	

dgBigVector dgMeshEffect::GetOrigin ()const
{
    dgBigVector origin (dgFloat64 (0.0f), dgFloat64 (0.0f), dgFloat64 (0.0f), dgFloat64 (0.0f));	
    for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i ++) {
        origin += m_points.m_vertex[i];
    }	
    return origin.Scale (dgFloat64 (1.0f) / m_points.m_vertex.m_count);
}

/*
void dgMeshEffect::ClearAttributeArray ()
{
	dgAssert(0);

    dgStack<dgVertexAtribute>attribArray (m_pointCount);

    memset (&attribArray[0], 0, m_pointCount * sizeof (dgVertexAtribute));
    dgInt32 mark = IncLRU();
    dgPolyhedra::Iterator iter (*this);	
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
        if (edge->m_mark < mark){
            dgEdge* ptr = edge;

            dgInt32 index = ptr->m_incidentVertex;
            dgVertexAtribute& attrib = attribArray[index];
            attrib.m_vertex = m_points[index];
            do {
                ptr->m_mark = mark;
                ptr->m_userData = index;
                ptr = ptr->m_twin->m_next;
            } while (ptr !=  edge);

        }
    }
    ApplyAttributeArray (&attribArray[0], m_pointCount);
}
*/

void dgMeshEffect::CalculateNormals (dgFloat64 angleInRadians)
{
    dgEdge* edgeBuffer[256];
	dgBigVector faceNormal[256];

	UnpackAttibuteData ();
	m_attrib.m_normalChannel.Reserve(m_attrib.m_pointChannel.m_count);

    dgInt32 mark = IncLRU();
    dgPolyhedra::Iterator iter (*this);	
    dgFloat32 smoothValue = dgCos (angleInRadians); 

	dgTree<dgInt32, dgEdge*> normalsMap(GetAllocator()) ;
    for(iter.Begin(); iter; iter ++) {
        dgEdge* const edge = &(*iter);
        if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
			dgInt32 edgeIndex = 0;
			normalsMap.RemoveAll();
            dgEdge* edgePtr = edge;
            do {
                dgVector normal (FaceNormal (edgePtr, &m_points.m_vertex[0].m_x, sizeof (dgBigVector)));
				dgAssert (normal.m_w == dgFloat32 (0.0f));
                normal = normal.Scale (dgFloat32 (1.0f) / dgFloat32 (sqrt(normal.DotProduct(normal).GetScalar()) + dgFloat32(1.0e-16f)));
                faceNormal[edgeIndex] = normal;
                normalsMap.Insert(edgeIndex, edgePtr);
                edgeIndex ++;
                edgePtr = edgePtr->m_twin->m_next;
            } while (edgePtr != edge);

            dgEdge* startEdge = edge;
            dgVector normal0 (faceNormal[normalsMap.Find(startEdge)->GetInfo()]);
            for (dgEdge* ptr = edge->m_prev->m_twin ; (ptr->m_mark != mark) && (ptr != edge) && (ptr->m_incidentFace > 0); ptr = ptr->m_prev->m_twin) {
                const dgVector& normal1 (faceNormal[normalsMap.Find(ptr)->GetInfo()]);
				dgAssert(normal0.m_w == dgFloat32(0.0f));
                dgFloat32 dot = normal0.DotProduct(normal1).GetScalar();
                if (dot < smoothValue) {
                    break;
                }
                startEdge = ptr;
                normal0 = normal1;
            }

            dgInt32 attribCount = 1;
            edgeBuffer[0] = startEdge;
            normal0 = faceNormal[normalsMap.Find(startEdge)->GetInfo()];
            dgVector normal (normal0);
            for (dgEdge* ptr = startEdge->m_twin->m_next; (ptr->m_mark != mark) && (ptr != startEdge) && (ptr->m_incidentFace > 0); ptr = ptr->m_twin->m_next) { 
                const dgVector& normal1 (faceNormal[normalsMap.Find(ptr)->GetInfo()]);
				dgAssert(normal0.m_w == dgFloat32(0.0f));
                dgFloat32 dot = normal0.DotProduct(normal1).GetScalar();
                if (dot < smoothValue)  {
                    break;
                }
                edgeBuffer[attribCount] = ptr;
				attribCount ++;
                normal += normal1;
                normal0 = normal1;
            } 

			dgAssert(normal.m_w == dgFloat32(0.0f));
            normal = normal.Scale (dgFloat32 (1.0f) / dgFloat32(sqrt(normal.DotProduct(normal).GetScalar()) + dgFloat32(1.0e-16f)));
			dgTriplex n;
			n.m_x = normal.m_x;
			n.m_y = normal.m_y;
			n.m_z = normal.m_z;
            for (dgInt32 i = 0; i < attribCount; i ++) {
				edgeBuffer[i]->m_mark = mark;
				dgInt32 index = dgInt32 (edgeBuffer[i]->m_userData);
				m_attrib.m_normalChannel[index] = n;
            }
        }
    }
	PackAttibuteData ();
}

void dgMeshEffect::SphericalMapping (dgInt32 material, const dgMatrix& uvAligment)
{
    dgBigVector origin (GetOrigin());
    dgStack<dgBigVector>sphere (m_points.m_vertex.m_count);
    for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i ++) {
		dgBigVector point(uvAligment.RotateVector(m_points.m_vertex[i] - origin));
		dgAssert(point.m_w == dgFloat32(0.0f));
		dgAssert(point.DotProduct(point).GetScalar() > dgFloat32(0.0f));
		point = point.Normalize();

		dgFloat64 u = dgAsin(dgClamp(point.m_x, dgFloat64(-1.0f + 1.0e-6f), dgFloat64(1.0f - 1.0e-6f)));
		dgFloat64 v = dgAtan2(point.m_y, point.m_z);

		u = dgFloat32(1.0f) - (dgFloat64(dgPI / 2.0f) - u) / dgFloat64(dgPI);
		dgAssert(u >= dgFloat32(0.0f));
		dgAssert(u <= dgFloat32(1.0f));

		v = v + dgPI;
		sphere[i].m_x = u;
		sphere[i].m_y = v;
    }

	UnpackAttibuteData ();
	m_attrib.m_uv0Channel.Reserve(m_attrib.m_pointChannel.m_count);
	m_attrib.m_materialChannel.Reserve(m_attrib.m_pointChannel.m_count);
	
    dgPolyhedra::Iterator iter (*this);	
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
		dgAttibutFormat::dgUV uv;
		uv.m_u = dgFloat32 (sphere[edge->m_incidentVertex].m_x);
		uv.m_v = dgFloat32 (sphere[edge->m_incidentVertex].m_y);
		m_attrib.m_uv0Channel[dgInt32 (edge->m_userData)] = uv;
		m_attrib.m_materialChannel[dgInt32 (edge->m_userData)] = material;
    }

    dgInt32 mark = IncLRU ();
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
        if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
			dgAttibutFormat::dgUV uvRef(m_attrib.m_uv0Channel[dgInt32(edge->m_userData)]);
			dgFloat32 UVrefSin = dgSin(uvRef.m_v);
			dgFloat32 UVrefCos = dgCos(uvRef.m_v);
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				dgAttibutFormat::dgUV uv(m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)]);
				dgFloat32 sinAngle = UVrefCos * dgSin(uv.m_v) - UVrefSin * dgCos(uv.m_v);
				dgFloat32 cosAngle = UVrefCos * dgCos(uv.m_v) + UVrefSin * dgSin(uv.m_v);
				dgFloat32 deltaAngle = dgAtan2(sinAngle, cosAngle);
				uv.m_v = (uvRef.m_v + deltaAngle) / dgPI2;
				m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)] = uv;
				ptr = ptr->m_next;
			} while (ptr != edge);
        }
    }

	PackAttibuteData();
}

void dgMeshEffect::CylindricalMapping (dgInt32 cylinderMaterial, dgInt32 capMaterial, const dgMatrix& uvAligment)
{
	dgBigVector origin (GetOrigin());
	dgStack<dgBigVector> buffer(m_points.m_vertex.m_count);
	dgBigVector pMin(dgFloat64(1.0e10f), dgFloat64(1.0e10f), dgFloat64(1.0e10f), dgFloat64(0.0f));
	dgBigVector pMax(dgFloat64(-1.0e10f), dgFloat64(-1.0e10f), dgFloat64(-1.0e10f), dgFloat64(0.0f));

	for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i ++) {
		buffer[i] = uvAligment.RotateVector (m_points.m_vertex[i] - origin);
		const dgBigVector& tmp = buffer[i];
		pMin.m_x = dgMin (pMin.m_x, tmp.m_x);
		pMax.m_x = dgMax (pMax.m_x, tmp.m_x);
		pMin.m_y = dgMin (pMin.m_y, tmp.m_y);
		pMax.m_y = dgMax (pMax.m_y, tmp.m_y);
		pMin.m_z = dgMin (pMin.m_z, tmp.m_z);
		pMax.m_z = dgMax (pMax.m_z, tmp.m_z);
	}

	dgStack<dgBigVector>cylinder (m_points.m_vertex.m_count);
    dgBigVector scale (dgFloat64 (1.0f)/ (pMax.m_x - pMin.m_x), dgFloat64 (1.0f)/ (pMax.m_y - pMin.m_y), dgFloat64 (1.0f)/ (pMax.m_z - pMin.m_z), dgFloat64 (0.0f));
    for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i ++) {
		//dgBigVector point (uvAligment.RotateVector (m_points.m_vertex[i] - origin));
		dgBigVector point (buffer[i]);
		dgFloat64 u = (point.m_x - pMin.m_x) * scale.m_x;

		dgAssert(point.m_w == dgFloat32(0.0f));
		dgAssert(point.DotProduct(point).GetScalar() > dgFloat32 (0.0f));
		point = point.Normalize();
		dgFloat64 v = dgAtan2 (point.m_y, point.m_z);

		v = v + dgPI;
		cylinder[i].m_x = u;
		cylinder[i].m_y = v;
    }

	UnpackAttibuteData();
	m_attrib.m_uv0Channel.Reserve(m_attrib.m_pointChannel.m_count);
	m_attrib.m_materialChannel.Reserve(m_attrib.m_pointChannel.m_count);

    dgPolyhedra::Iterator iter (*this);	
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
		dgAttibutFormat::dgUV uv;
		uv.m_u = dgFloat32(cylinder[edge->m_incidentVertex].m_x);
		uv.m_v = dgFloat32(cylinder[edge->m_incidentVertex].m_y);
		m_attrib.m_uv0Channel[dgInt32(edge->m_userData)] = uv;
		m_attrib.m_materialChannel[dgInt32(edge->m_userData)] = cylinderMaterial;
    }

    dgInt32 mark = IncLRU ();
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
        if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
			dgAttibutFormat::dgUV uvRef(m_attrib.m_uv0Channel[dgInt32(edge->m_userData)]);
			dgFloat32 UVrefSin = dgSin(uvRef.m_v);
			dgFloat32 UVrefCos = dgCos(uvRef.m_v);
			dgEdge* ptr = edge;
            do {
				ptr->m_mark = mark;
				dgAttibutFormat::dgUV uv(m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)]);
				dgFloat32 sinAngle = UVrefCos * dgSin(uv.m_v) - UVrefSin * dgCos(uv.m_v);
				dgFloat32 cosAngle = UVrefCos * dgCos(uv.m_v) + UVrefSin * dgSin(uv.m_v);
				dgFloat32 deltaAngle = dgAtan2(sinAngle, cosAngle);
				uv.m_v = (uvRef.m_v + deltaAngle) / dgPI2;
				m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)] = uv;
                ptr = ptr->m_next;
            } while (ptr != edge);
        }
    }

    // apply cap mapping
    mark = IncLRU ();
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace > 0) && (edge->m_mark != mark)) {
			//dgVector p0(uvAligment.RotateVector(m_points.m_vertex[edge->m_incidentVertex] - origin));
			//dgVector p1(uvAligment.RotateVector(m_points.m_vertex[edge->m_next->m_incidentVertex] - origin));
			dgVector p0(buffer[edge->m_incidentVertex]);
			dgVector p1(buffer[edge->m_next->m_incidentVertex]);

			dgVector e1(p1 - p0);
			dgBigVector normal(dgFloat32(0.0f));
			for (dgEdge* ptr = edge->m_next; ptr != edge; ptr = ptr->m_next) {
				//dgVector p2(uvAligment.RotateVector(m_points.m_vertex[ptr->m_next->m_incidentVertex] - origin));
				dgVector p2(buffer[ptr->m_next->m_incidentVertex]);
				dgBigVector e2(p2 - p0);
				normal += e1.CrossProduct(e2);
				e1 = e2;
			}
			normal = normal.Normalize();
			if (dgAbs(normal.m_x) > dgFloat32 (0.99f)) {
				dgEdge* ptr = edge;
				do {
					dgAttibutFormat::dgUV uv;
					//dgVector p(uvAligment.RotateVector(m_points.m_vertex[ptr->m_incidentVertex] - origin));
					dgVector p(buffer[ptr->m_incidentVertex]);
					uv.m_u = dgFloat32((p.m_y - pMin.m_y) * scale.m_y);
					uv.m_v = dgFloat32((p.m_z - pMin.m_z) * scale.m_z);
					m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)] = uv;
					m_attrib.m_materialChannel[dgInt32(ptr->m_userData)] = capMaterial;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}

			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);

		}
    }
	PackAttibuteData();
}

void dgMeshEffect::BoxMapping (dgInt32 front, dgInt32 side, dgInt32 top, const dgMatrix& uvAligment)
{
/*
    dgBigVector minVal;
    dgBigVector maxVal;

    dgGetMinMax (minVal, maxVal, &m_points.m_vertex[0][0], m_points.m_vertex.m_count, sizeof (dgBigVector));
    dgBigVector dist (maxVal - minVal);
    dist[0] = dgMax (dgFloat64 (1.0e-3f), dist[0]);
    dist[1] = dgMax (dgFloat64 (1.0e-3f), dist[1]);
    dist[2] = dgMax (dgFloat64 (1.0e-3f), dist[2]);
    dgBigVector scale (dgFloat64 (1.0f)/ dist[0], dgFloat64 (1.0f)/ dist[1], dgFloat64 (1.0f)/ dist[2], dgFloat64 (0.0f));
*/

	dgBigVector origin(GetOrigin());
	dgStack<dgBigVector> buffer(m_points.m_vertex.m_count);
	dgBigVector pMin(dgFloat64(1.0e10f), dgFloat64(1.0e10f), dgFloat64(1.0e10f), dgFloat64(0.0f));
	dgBigVector pMax(dgFloat64(-1.0e10f), dgFloat64(-1.0e10f), dgFloat64(-1.0e10f), dgFloat64(0.0f));

	for (dgInt32 i = 0; i < m_points.m_vertex.m_count; i++) {
		buffer[i] = uvAligment.RotateVector(m_points.m_vertex[i] - origin);
		const dgBigVector& tmp = buffer[i];
		pMin.m_x = dgMin(pMin.m_x, tmp.m_x);
		pMax.m_x = dgMax(pMax.m_x, tmp.m_x);
		pMin.m_y = dgMin(pMin.m_y, tmp.m_y);
		pMax.m_y = dgMax(pMax.m_y, tmp.m_y);
		pMin.m_z = dgMin(pMin.m_z, tmp.m_z);
		pMax.m_z = dgMax(pMax.m_z, tmp.m_z);
	}
    dgInt32 materialArray[3];

    dgBigVector dist (pMax);
	dist[0] = dgMax(dgFloat64(1.0e-3f), dist[0]);
	dist[1] = dgMax(dgFloat64(1.0e-3f), dist[1]);
	dist[2] = dgMax(dgFloat64(1.0e-3f), dist[2]);
    dgBigVector scale (dgFloat64 (0.5f)/ dist[0], dgFloat64 (0.5f)/ dist[1], dgFloat64 (0.5f)/ dist[2], dgFloat64 (0.0f));

	UnpackAttibuteData();
	m_attrib.m_uv0Channel.Reserve(m_attrib.m_pointChannel.m_count);
	m_attrib.m_materialChannel.Reserve(m_attrib.m_pointChannel.m_count);

    materialArray[0] = front;
    materialArray[1] = side;
    materialArray[2] = top;

    dgInt32 mark = IncLRU();
    dgPolyhedra::Iterator iter (*this);	
    for(iter.Begin(); iter; iter ++){
        dgEdge* const edge = &(*iter);
        if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
            const dgBigVector& p0 = buffer[edge->m_incidentVertex];
            const dgBigVector& p1 = buffer[edge->m_next->m_incidentVertex];
            const dgBigVector& p2 = buffer[edge->m_prev->m_incidentVertex];

            edge->m_mark = mark;
            edge->m_next->m_mark = mark;
            edge->m_prev->m_mark = mark;

            dgBigVector e0 (p1 - p0);
            dgBigVector e1 (p2 - p0);
            dgBigVector n (e0.CrossProduct(e1));

            dgInt32 index = 0;
            dgFloat64 maxProjection = dgFloat32 (0.0f);

            for (dgInt32 i = 0; i < 3; i ++) {
                dgFloat64 proj = fabs (n[i]);
                if (proj > maxProjection) {
                    index = i;
                    maxProjection = proj;
                }
            }

            dgInt32 u = (index + 1) % 3;
            dgInt32 v = (u + 1) % 3;
            if (index == 1) {
                dgSwap (u, v);
            }
            dgEdge* ptr = edge;
            do {
				dgAttibutFormat::dgUV uv;
				dgBigVector p(scale * buffer[ptr->m_incidentVertex] - dgFloat32 (0.5f));
				uv.m_u = dgFloat32 (p[u]);
				uv.m_v = dgFloat32 (p[v]);
				m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)] = uv;
				m_attrib.m_materialChannel[dgInt32(ptr->m_userData)] = materialArray[index];
                ptr = ptr->m_next;
            } while (ptr !=  edge);
        }
    }
	PackAttibuteData();
}


void dgMeshEffect::UniformBoxMapping (dgInt32 material, const dgMatrix& textureMatrix)
{
	UnpackAttibuteData();
	m_attrib.m_uv0Channel.Reserve(m_attrib.m_pointChannel.m_count);
	m_attrib.m_materialChannel.Reserve(m_attrib.m_pointChannel.m_count);

    dgInt32 mark = IncLRU();
    for (dgInt32 i = 0; i < 3; i ++) {
        dgMatrix rotationMatrix (dgGetIdentityMatrix());
        if (i == 1) {
            rotationMatrix = dgYawMatrix(dgFloat32 (90.0f * dgDEG2RAD));
        } else if (i == 2) {
            rotationMatrix = dgPitchMatrix(dgFloat32 (90.0f * dgDEG2RAD));
        }

        dgPolyhedra::Iterator iter (*this);	

        for(iter.Begin(); iter; iter ++){
            dgEdge* const edge = &(*iter);
            if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
                dgBigVector n (FaceNormal(edge, &m_points.m_vertex[0].m_x, sizeof (dgBigVector)));
                dgVector normal (rotationMatrix.RotateVector(dgVector (n.Normalize())));
                normal.m_x = dgAbs (normal.m_x);
                normal.m_y = dgAbs (normal.m_y);
                normal.m_z = dgAbs (normal.m_z);
                if ((normal.m_z >= (normal.m_x - dgFloat32 (1.0e-4f))) && (normal.m_z >= (normal.m_y - dgFloat32 (1.0e-4f)))) {
                    dgEdge* ptr = edge;
                    do {
                        ptr->m_mark = mark;
						dgAttibutFormat::dgUV uv;
                        dgVector p (textureMatrix.TransformVector(rotationMatrix.RotateVector(m_points.m_vertex[ptr->m_incidentVertex])));
                        uv.m_u = p.m_x;
                        uv.m_v = p.m_y;
						m_attrib.m_uv0Channel[dgInt32(ptr->m_userData)] = uv;
						m_attrib.m_materialChannel[dgInt32(ptr->m_userData)] = material;

                        ptr = ptr->m_next;
                    }while (ptr !=  edge);
                }
            }
        }
    }

    PackAttibuteData();
}


void dgMeshEffect::AngleBaseFlatteningMapping (dgInt32 material, dgReportProgress progressReportCallback, void* const userData)
{
	dgSetPrecisionDouble presicion;

	dgMeshEffect tmp (*this);

	dgBigVector minBox;
	dgBigVector maxBox;
	tmp.CalculateAABB(minBox, maxBox);

	dgBigVector size (maxBox - minBox);
	dgFloat32 scale = dgFloat32 (1.0 / dgMax (size.m_x, size.m_y, size.m_z));

	dgMatrix matrix (dgGetIdentityMatrix());
	matrix[0][0] = scale;
	matrix[1][1] = scale;
	matrix[2][2] = scale;
	tmp.ApplyTransform(matrix);

	dgAngleBasedFlatteningMapping angleBadedFlattening (&tmp, material, progressReportCallback, userData);
}
