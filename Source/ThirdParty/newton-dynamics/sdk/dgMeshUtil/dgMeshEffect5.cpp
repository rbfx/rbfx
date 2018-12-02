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
#include "dgMeshEffect.h"
#include "dgCollisionConvexHull.h"



bool dgMeshEffect::PlaneClip(const dgMeshEffect& convexMesh, const dgEdge* const convexFace)
{
	dgAssert (convexFace->m_incidentFace > 0);

	dgBigVector normal (convexMesh.FaceNormal(convexFace, &convexMesh.m_points.m_vertex[0].m_x, sizeof(dgBigVector)));
	dgFloat64 mag2 = normal.DotProduct(normal).GetScalar();
	if (mag2 < dgFloat64 (1.0e-30)) {
		dgAssert (0);
		return true;
	}  

	normal = normal.Normalize();
	dgBigVector origin (convexMesh.m_points.m_vertex[convexFace->m_incidentVertex]);
	dgBigPlane plane (normal, - origin.DotProduct(normal).GetScalar());

	dgAssert (!HasOpenEdges());

	dgInt32 pointCount = GetVertexCount();
	dgStack <dgFloat64> testPool (2 * pointCount + 1024);
	dgFloat64* const test = &testPool[0];
	for (dgInt32 i = 0; i < pointCount; i ++) {
		test[i] = plane.Evalue (m_points.m_vertex[i]);
		if (fabs (test[i]) < dgFloat32 (1.0e-5f)) {
			test[i] = dgFloat32 (0.0f);
		}
	}

	dgInt32 positive = 0;
	dgInt32 negative = 0;
	dgPolyhedra::Iterator iter (*this); 
	for (iter.Begin(); iter && !(positive && negative); iter ++){
		dgEdge* const edge = &(*iter);
		positive += test[edge->m_incidentVertex] > dgFloat32 (0.0f);
		negative += test[edge->m_incidentVertex] < dgFloat32 (0.0f);
	}
	if (positive  && !negative) {
		return false;
	}

	if (positive && negative) {
		const dgEdge* e0 = convexFace;
		const dgEdge* e1 = e0->m_next;
		const dgEdge* e2 = e1->m_next;

		dgMatrix matrix;
		dgBigVector p1 (convexMesh.m_points.m_vertex[e1->m_incidentVertex]);

		dgBigVector xDir (p1 - origin);
		dgAssert(xDir.m_w == dgFloat32(0.0f));
		dgAssert (xDir.DotProduct(xDir).GetScalar() > dgFloat32 (0.0f));
		matrix[2] = dgVector (normal);
		matrix[0] = dgVector(xDir.Scale(dgFloat64 (1.0f) / sqrt (xDir.DotProduct(xDir).GetScalar())));
		matrix[1] = matrix[2].CrossProduct(matrix[0]);
		matrix[3] = dgVector (origin);
		matrix[3][3] = dgFloat32 (1.0f);

		dgVector q0 (matrix.UntransformVector(dgVector(convexMesh.m_points.m_vertex[e0->m_incidentVertex])));
		dgVector q1 (matrix.UntransformVector(dgVector(convexMesh.m_points.m_vertex[e1->m_incidentVertex])));
		dgVector q2 (matrix.UntransformVector(dgVector(convexMesh.m_points.m_vertex[e2->m_incidentVertex])));

		dgVector p10 (q1 - q0);
		dgVector p20 (q2 - q0);
		dgVector faceNormal (matrix.UnrotateVector (dgVector(normal)));
		dgAssert(faceNormal.m_w == dgFloat32(0.0f));
		dgFloat32 areaInv = faceNormal.DotProduct(p10.CrossProduct(p20)).GetScalar();
		if (e2->m_next != e0) {
			const dgEdge* edge = e2;
			dgVector r1 (q2);
			dgVector q10 (p20);
			do {
				dgVector r2 (matrix.UntransformVector(dgVector(convexMesh.m_points.m_vertex[edge->m_next->m_incidentVertex])));
				dgVector q20 (r2 - q0);
				dgFloat32 areaInv1 = faceNormal.DotProduct(q10.CrossProduct(q20)).GetScalar();
				if (areaInv1 > areaInv) {
					e1 = edge;
					e2 = edge->m_next;
					q1 = r1;
					q2 = r2;
					areaInv = areaInv1;
				}
				r1 = r2;
				q10 = q20;
				edge = edge->m_next;
			} while (edge->m_next != e0);
		}

		dgAssert (areaInv > dgFloat32 (0.0f));
		areaInv = dgFloat32 (1.0f) / areaInv;

		dgVector uv0[3];
		dgVector uv1[3];
		memset(uv0, 0, sizeof(uv0));
		memset(uv1, 0, sizeof(uv1));
		if (m_attrib.m_uv0Channel.m_count && convexMesh.m_attrib.m_uv0Channel.m_count) {
			uv0[0] = dgVector (dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32 (e0->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32(e0->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
			uv0[1] = dgVector(dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32 (e1->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32(e1->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
			uv0[2] = dgVector(dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32 (e2->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv0Channel[dgInt32(e2->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
		}

		if (m_attrib.m_uv1Channel.m_count && convexMesh.m_attrib.m_uv1Channel.m_count) {
			uv1[0] = dgVector(dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e0->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e0->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
			uv1[1] = dgVector(dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e1->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e1->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
			uv1[2] = dgVector(dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e2->m_userData)].m_u), dgFloat32(convexMesh.m_attrib.m_uv1Channel[dgInt32(e2->m_userData)].m_v), dgFloat32(0.0f), dgFloat32(0.0f));
		}

		for (iter.Begin(); iter; iter ++){
			dgEdge* const edge = &(*iter);

			dgFloat64 side0 = test[edge->m_prev->m_incidentVertex];
			dgFloat64 side1 = test[edge->m_incidentVertex];

			if ((side0 < dgFloat32 (0.0f)) && (side1 > dgFloat64 (0.0f))) {
				dgBigVector dp (m_points.m_vertex[edge->m_incidentVertex] - m_points.m_vertex[edge->m_prev->m_incidentVertex]);
				dgAssert(dp.m_w == dgFloat32(0.0f));
				dgFloat64 param = - side0 / plane.DotProduct(dp).GetScalar();

				dgEdge* const splitEdge = InsertEdgeVertex (edge->m_prev, param);
				test[splitEdge->m_next->m_incidentVertex] = dgFloat64 (0.0f);
			} 
		}

		dgInt32 colorMark = IncLRU();
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			dgFloat64 side0 = test[edge->m_incidentVertex];
			dgFloat64 side1 = test[edge->m_next->m_incidentVertex];

			if ((side0 > dgFloat32 (0.0f)) || (side1 > dgFloat64 (0.0f))) {
				edge->m_mark = colorMark;
			}
		}

		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			dgFloat64 side0 = test[edge->m_incidentVertex];
			dgFloat64 side1 = test[edge->m_next->m_incidentVertex];
			if ((side0 == dgFloat32 (0.0f)) && (side1 == dgFloat64 (0.0f))) {
				dgEdge* ptr = edge->m_next;
				do {
					if (ptr->m_mark == colorMark) {
						edge->m_mark = colorMark;
						break;
					}
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}


		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			if ((edge->m_mark == colorMark) && (edge->m_next->m_mark < colorMark)) {
				dgEdge* const startEdge = edge->m_next;
				dgEdge* end = startEdge;
				do {
					if (end->m_mark == colorMark) {
						break;
					}

					end = end->m_next;
				} while (end != startEdge);
				dgAssert (end != startEdge);
				dgEdge* const devideEdge = ConnectVertex (startEdge, end);
				dgAssert (devideEdge);
				dgAssert (devideEdge->m_next->m_mark != colorMark);
				dgAssert (devideEdge->m_prev->m_mark != colorMark);
				dgAssert (devideEdge->m_twin->m_next->m_mark == colorMark);
				dgAssert (devideEdge->m_twin->m_prev->m_mark == colorMark);
				devideEdge->m_mark = colorMark - 1;
				devideEdge->m_twin->m_mark = colorMark;
			}
		}

		dgInt32 mark = IncLRU();
		dgList<dgEdge*> faceList (GetAllocator());
		for (iter.Begin(); iter; iter ++){
			dgEdge* const face = &(*iter);
			if ((face->m_mark >= colorMark) && (face->m_mark != mark)) {
				faceList.Append(face);
				dgEdge* edge = face;
				do {
					edge->m_mark = mark;
					edge = edge->m_next;
				} while (edge != face);
			}
		}

		for (dgList<dgEdge*>::dgListNode* node = faceList.GetFirst(); node; node = node->GetNext()) {
			dgEdge* const face = node->GetInfo();
			DeleteFace(face);
		}

		mark = IncLRU();
		faceList.RemoveAll();
		for (iter.Begin(); iter; iter ++){
			dgEdge* const face = &(*iter);
			if ((face->m_mark != mark) && (face->m_incidentFace < 0)) {
				faceList.Append(face);
				dgEdge* edge = face;
				do {
					edge->m_mark = mark;
					edge = edge->m_next;
				} while (edge != face);
			}
		}


		const dgInt32 capAttribute = convexMesh.m_attrib.m_materialChannel.m_count ? convexMesh.m_attrib.m_materialChannel[dgInt32 (convexFace->m_userData)] : 0;
		for (dgList<dgEdge*>::dgListNode* node = faceList.GetFirst(); node; node = node->GetNext()) {
			dgEdge* const face = node->GetInfo();

			dgEdge* edge = face;
			do {
				edge->m_incidentFace = 1;
				edge->m_userData = m_attrib.m_pointChannel.m_count;

				m_attrib.m_pointChannel.PushBack(edge->m_incidentVertex);
				if (m_attrib.m_normalChannel.m_count) {
					dgTriplex n;
					n.m_x = dgFloat32(normal.m_x);
					n.m_y = dgFloat32(normal.m_y);
					n.m_z = dgFloat32(normal.m_z);
					m_attrib.m_normalChannel.PushBack(n);
				}

				if (m_attrib.m_binormalChannel.m_count) {
					dgAssert(0);
				}

				if (m_attrib.m_colorChannel.m_count) {
					dgAssert(0);
				}

				if (m_attrib.m_materialChannel.m_count) {
					m_attrib.m_materialChannel.PushBack(capAttribute);
				}

				//dgVector p (matrix.UntransformVector (attibute.m_vertex));
				dgVector p (matrix.UntransformVector(m_points.m_vertex[edge->m_incidentVertex]));
				dgVector p_p0 (p - q0);
				dgVector p_p1 (p - q1);
				dgVector p_p2 (p - q2);
				dgAssert(faceNormal.m_w == dgFloat32 (0.0f));
				dgFloat32 alpha0 = faceNormal.DotProduct(p_p1.CrossProduct(p_p2)).GetScalar() * areaInv;
				dgFloat32 alpha1 = faceNormal.DotProduct(p_p2.CrossProduct(p_p0)).GetScalar() * areaInv;
				dgFloat32 alpha2 = faceNormal.DotProduct(p_p0.CrossProduct(p_p1)).GetScalar() * areaInv;

				//alpha0 = 0.0f;
				//alpha1 = 0.0f;
				//alpha2 = 0.0;;
				if (m_attrib.m_uv0Channel.m_count && convexMesh.m_attrib.m_uv0Channel.m_count) {
					dgAttibutFormat::dgUV uv;
					uv.m_u = uv0[0].m_x * alpha0 + uv0[1].m_x * alpha1 + uv0[2].m_x * alpha2;
					uv.m_v = uv0[0].m_y * alpha0 + uv0[1].m_y * alpha1 + uv0[2].m_y * alpha2;
					m_attrib.m_uv0Channel.PushBack(uv);
				}

				if (m_attrib.m_uv1Channel.m_count && convexMesh.m_attrib.m_uv1Channel.m_count) {
					dgAttibutFormat::dgUV uv;
					uv.m_u = uv1[0].m_x * alpha0 + uv1[1].m_x * alpha1 + uv1[2].m_x * alpha2;
					uv.m_v = uv1[0].m_y * alpha0 + uv1[1].m_y * alpha1 + uv1[2].m_y * alpha2;
					m_attrib.m_uv1Channel.PushBack(uv);
				}

				edge = edge->m_next;
			} while (edge != face);
		}
	}

	return true;
}


dgMeshEffect* dgMeshEffect::ConvexMeshIntersection (const dgMeshEffect* const convexMeshSrc) const
{
	dgMeshEffect convexMesh (*convexMeshSrc);
	convexMesh.ConvertToPolygons();
	dgMeshEffect* const convexIntersection = new (GetAllocator()) dgMeshEffect (*this);

	dgInt32 mark = convexMesh.IncLRU();
	dgPolyhedra::Iterator iter (convexMesh);

	for (iter.Begin(); iter; iter ++){
		dgEdge* const convexFace = &(*iter);
		if ((convexFace->m_incidentFace > 0) && (convexFace->m_mark != mark)) {
			dgEdge* ptr = convexFace;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != convexFace);
			if (!convexIntersection->PlaneClip(convexMesh, convexFace)) {
				delete convexIntersection;
				return NULL;
			}
		}
	}

	if (!convexIntersection->GetVertexCount()) {
		delete convexIntersection;
		return NULL;
	}
	convexIntersection->RemoveUnusedVertices(NULL);
	return convexIntersection;
}


void dgMeshEffect::ClipMesh (const dgMatrix& matrix, const dgMeshEffect* const clipMesh, dgMeshEffect** const back, dgMeshEffect** const front) const
{
	dgAssert (0);
/*
	dgMeshEffect clipper (*clipMesh);
	clipper.TransformMesh (matrix);

	dgMeshEffect* backMeshSource = NULL;
	dgMeshEffect* frontMeshSource = NULL;
	dgMeshEffect* backMeshClipper = NULL;
	dgMeshEffect* frontMeshClipper = NULL;

	ClipMesh (&clipper, &backMeshSource, &frontMeshSource);
	if (backMeshSource && frontMeshSource) {
		clipper.ClipMesh (this, &backMeshClipper, &frontMeshClipper);
		if (backMeshSource && frontMeshSource) {

			dgMeshEffect* backMesh;
			dgMeshEffect* frontMesh;

			backMesh = new (GetAllocator()) dgMeshEffect (GetAllocator(), true);
			frontMesh = new (GetAllocator()) dgMeshEffect (GetAllocator(), true);

			backMesh->BeginPolygon();
			frontMesh->BeginPolygon();

			backMesh->MergeFaces(backMeshSource);
			backMesh->MergeFaces(backMeshClipper);

			frontMesh->MergeFaces(frontMeshSource);
			frontMesh->ReverseMergeFaces(backMeshClipper);

			backMesh->EndPolygon(dgFloat64 (1.0e-5f));
			frontMesh->EndPolygon(dgFloat64 (1.0e-5f));

			*back = backMesh;
			*front = frontMesh;
		}
	}

	if (backMeshClipper) {
		delete backMeshClipper;
	}

	if (frontMeshClipper) {
		delete frontMeshClipper;
	}

	if (backMeshSource) {
		delete backMeshSource;
	}

	if (frontMeshSource) {
		delete frontMeshSource;
	}
*/
}




dgMeshEffect* dgMeshEffect::Union (const dgMatrix& matrix, const dgMeshEffect* const clipperMesh) const
{
	dgAssert (0);
	return NULL;
/*
	dgMeshEffect copy (*this);
	dgMeshEffect clipper (*clipperMesh);
	clipper.TransformMesh (matrix);

	dgBooleanMeshClipper::ClipMeshesAndColorize (&copy, &clipper);

	dgMeshEffect* const mesh = new (GetAllocator()) dgMeshEffect (GetAllocator());
	mesh->BeginFace();
	dgBooleanMeshClipper::CopyPoints(mesh, &copy);
	dgBooleanMeshClipper::AddExteriorFaces (mesh, &copy);

	dgBooleanMeshClipper::AddExteriorFaces (mesh, &clipper);
	mesh->EndFace ();
	mesh->RepairTJoints();
	mesh->RemoveUnusedVertices(NULL);
	return mesh;
*/
}

dgMeshEffect* dgMeshEffect::Difference (const dgMatrix& matrix, const dgMeshEffect* const clipperMesh) const
{
/*
	dgMeshEffect copy (*this);
	dgMeshEffect clipper (*clipperMesh);
	clipper.TransformMesh (matrix);

	dgBooleanMeshClipper::ClipMeshesAndColorize (&copy, &clipper);

	dgMeshEffect* const mesh = new (GetAllocator()) dgMeshEffect (GetAllocator());
	mesh->BeginFace();
	dgBooleanMeshClipper::CopyPoints(mesh, &copy);
	dgBooleanMeshClipper::AddExteriorFaces (mesh, &copy);
	dgBooleanMeshClipper::AddInteriorFacesInvertWinding (mesh, &clipper);
	mesh->EndFace ();
	mesh->RepairTJoints();
	mesh->RemoveUnusedVertices(NULL);
	return mesh;
*/

	dgAssert (0);
	return NULL;
}


class dgBooleanMeshClipper: public dgMeshEffect::dgMeshBVH
{
	class dgPoint: public dgBigVector
	{
		public:
		dgPoint()
			:dgBigVector(dgFloat32 (0.0f))
			,m_links(NULL)
			,m_lru(0)
		{
			dgAssert (0);
		}

		dgPoint(const dgBigVector& point, dgMemoryAllocator* const allocator)
			:dgBigVector(point)
			,m_links(allocator)
			,m_lru(0)
		{
		}
		
		dgList<dgTree<dgPoint, dgFloat64>::dgTreeNode*> m_links;
		dgInt32 m_lru;
	};

	class dgCurvesNetwork: public dgTree<dgPoint, dgFloat64>
	{
		public:
		dgCurvesNetwork ()
			:dgTree<dgPoint, dgFloat64>(NULL)
		{
			dgAssert (0);
		}

		dgCurvesNetwork (dgMemoryAllocator* const allocator)
			:dgTree<dgPoint, dgFloat64>(allocator)
		{
		}

		dgTreeNode* AddVertex(const dgBigVector& point, dgMemoryAllocator* const allocator)
		{
			dgFloat64 key = ((point.m_z * dgFloat64 (1024.0f) + point.m_y) * dgFloat64 (1024.0f)) + point.m_x;
			dgTreeNode* node = Find(key);
			if (!node) {
				dgPoint entry (point, allocator);
				node = Insert(entry, key);
			}
			return node;
		}

/*
		dgCurvesNetwork(dgBooleanMeshClipper* const BVHmeshA, dgBooleanMeshClipper* const BVHmeshB)
			:dgTree<dgPoint, dgFloat64>(BVHmeshA->m_mesh->GetAllocator())
//			,m_meshA(BVHmeshA->m_mesh)
//			,m_meshB(BVHmeshB->m_mesh)
//			,m_pointBaseA(m_meshA->GetVertexCount()) 
//			,m_pointBaseB(m_meshB->GetVertexCount()) 
//			,m_lru(0)
		{
		}
*/
/*
		dgHugeVector CalculateFaceNormal (const dgMeshEffect* const mesh, dgEdge* const face)
		{
			dgHugeVector plane(dgGoogol::m_zero, dgGoogol::m_zero, dgGoogol::m_zero, dgGoogol::m_zero);
			dgEdge* edge = face;
			dgHugeVector p0(mesh->GetVertex(edge->m_incidentVertex));
			edge = edge->m_next;
			dgHugeVector p1(mesh->GetVertex(edge->m_incidentVertex));
			dgHugeVector p1p0(p1 - p0);
			edge = edge->m_next;
			do {
				dgHugeVector p2(mesh->GetVertex(edge->m_incidentVertex));
				dgHugeVector p2p0(p2 - p0);
				plane += p2p0 * p1p0;
				p1p0 = p2p0;
				edge = edge->m_next;
			} while (edge != face);

			plane.m_w = dgGoogol::m_zero - (plane % p0);
			return plane;
		}


		bool IsPointInFace (const dgHugeVector& point, const dgMeshEffect* const mesh, dgEdge* const face, const dgHugeVector& normal) const 
		{
			dgEdge* edge = face;

			dgTrace (("%f %f %f\n", dgFloat64 (point.m_x), dgFloat64 (point.m_y), dgFloat64 (point.m_z)));
			do {
				dgBigVector p1(mesh->GetVertex(edge->m_incidentVertex));
				dgTrace (("%f %f %f\n", dgFloat64 (p1.m_x), dgFloat64 (p1.m_y), dgFloat64 (p1.m_z)));
				edge = edge->m_next;
			} while (edge != face);

			dgHugeVector p0(mesh->GetVertex(face->m_incidentVertex));
			do {
				dgHugeVector p1(mesh->GetVertex(edge->m_twin->m_incidentVertex));
				dgHugeVector p1p0(p1 - p0);
				dgHugeVector q1p0(point - p0);
				dgGoogol side (q1p0 % (normal * p1p0));
				if (side >= dgGoogol::m_zero) {
					return false;
				}
				p0 = p1;
				edge = edge->m_next;
			} while (edge != face);

			return true;
		}

		dgFloat64 ClipEdgeFace(const dgMeshEffect* const meshEdge, dgEdge* const edge, const dgMeshEffect* const meshFace, dgEdge* const face, const dgHugeVector& plane)
		{
			dgHugeVector p0 (meshEdge->GetVertex(edge->m_incidentVertex));
			dgHugeVector p1 (meshEdge->GetVertex(edge->m_twin->m_incidentVertex));
			
			dgGoogol test0 (plane.EvaluePlane(p0));
			dgGoogol test1 (plane.EvaluePlane(p1));

			if ((test0 * test1) > dgGoogol::m_zero) {
				// both point are in one side
				return -1.0f;
			}

			if ((test0 * test1) < dgGoogol::m_zero) {
				//point on different size, clip the line
				dgHugeVector p1p0 (p1 - p0);
				dgGoogol param = dgGoogol::m_zero - plane.EvaluePlane(p0) / (plane % p1p0);
				dgHugeVector p (p0 + p1p0.Scale (param));
				if (IsPointInFace (p, meshFace, face, plane)) {
					return param;
				}
				return -1.0f;
			} else {
				dgAssert (0);
				//special cases;
			}
			
			return -1.0f;
		}

		void AddPoint (dgMeshEffect* const edgeOwnerMesh, dgEdge* const edgeStart, dgMeshEffect* const faceOwnerMesh, dgEdge* const face, const dgHugeVector& plane, dgTreeNode** nodes, dgInt32& index)
		{
			dgEdge* edge = edgeStart;
			do {
				dgFloat64 param = ClipEdgeFace(edgeOwnerMesh, edge, faceOwnerMesh, face, plane);
				if (param > 0.0f) {
					dgPoint point(edgeOwnerMesh, edge, param, faceOwnerMesh, face);
					dgTreeNode* node = Find(dgNodeKey(edge, param));
					if (!node) {
						node = Insert(point, dgNodeKey(edge, param));
					}
					nodes[index] = node;
					index ++;
				}
				edge = edge->m_next;
			} while (edge != edgeStart);

		}

		void ClipMeshesFaces(dgEdge* const faceA, dgEdge* const faceB)
		{
			dgAssert (m_meshA->FindEdge(faceA->m_incidentVertex, faceA->m_twin->m_incidentVertex) == faceA);
			dgAssert (m_meshB->FindEdge(faceB->m_incidentVertex, faceB->m_twin->m_incidentVertex) == faceB);

			dgHugeVector planeA (CalculateFaceNormal (m_meshA, faceA));
			dgHugeVector planeB (CalculateFaceNormal (m_meshB, faceB));

			dgInt32 index = 0;
			dgTreeNode* nodes[16];
			AddPoint (m_meshA, faceA, m_meshB, faceB, planeB, nodes, index);
			AddPoint (m_meshB, faceB, m_meshA, faceA, planeA, nodes, index);
			dgAssert ((index == 0) || (index == 2));
			if (index == 2) {
				dgPoint& pointA = nodes[0]->GetInfo();
				dgPoint& pointB = nodes[1]->GetInfo();
				pointA.m_links.Append(nodes[1]);
				pointB.m_links.Append(nodes[0]);
			}
		}

		void GetCurve (dgList<dgTreeNode*>& curve, dgTreeNode* const node)
		{
			dgInt32 stack = 1;
			dgTreeNode* pool[64];

			pool[0] = node;
			while (stack) {
				stack --;
				dgTreeNode* const ptr = pool[stack];
				dgPoint& point = ptr->GetInfo();
				if (point.m_lru != m_lru) {
					point.m_lru = m_lru;
					curve.Append(ptr);
					for (dgList<dgTree<dgPoint, dgNodeKey>::dgTreeNode*>::dgListNode* ptrPoint = point.m_links.GetFirst(); ptrPoint; ptrPoint = ptrPoint->GetNext()) {
						dgTreeNode* const nextnode = ptrPoint->GetInfo();
						dgPoint& nextPoint = nextnode->GetInfo();
						if (nextPoint.m_lru != m_lru) {
							pool[stack] = nextnode;
							stack ++;
						}
					}
				}
			}
		}

		void EmbedCurveToSingleFace (dgList<dgTreeNode*>& curve, dgMeshEffect* const mesh)
		{
			dgEdge* const face = curve.GetFirst()->GetInfo()->GetInfo().m_face;

			dgInt32 indexBase = mesh->GetVertexCount();
			dgInt32 indexAttribBase = mesh->GetPropertiesCount();

			for (dgList<dgTreeNode*>::dgListNode* node = curve.GetFirst(); node; node = node->GetNext()) {
				dgPoint& point = node->GetInfo()->GetInfo();
				dgAssert (point.m_face == face);
				dgMeshEffect::dgVertexAtribute attribute(mesh->InterpolateVertex(point.m_posit, face));
				mesh->AddVertex(point.m_posit);
				mesh->AddAtribute(attribute);
			}

			dgList<dgEdge*> list(GetAllocator());
			dgInt32 i0 = curve.GetCount() - 1;
			for (dgInt32 i = 0; i < curve.GetCount(); i++) {
				dgEdge* const edge = mesh->AddHalfEdge(indexBase + i0, indexBase + i);
				dgEdge* const twin = mesh->AddHalfEdge(indexBase + i, indexBase + i0);

				edge->m_incidentFace = 1;
				twin->m_incidentFace = 1;
				edge->m_userData = indexAttribBase + i0;
				twin->m_userData = indexAttribBase + i;
				twin->m_twin = edge;
				edge->m_twin = twin;
				i0 = i;
				list.Append(edge);
			}

			dgEdge* closestEdge = NULL;
			dgFloat64 dist2 = dgFloat64 (1.0e10f);
			dgBigVector p(mesh->GetVertex(face->m_incidentVertex));

			list.Append(list.GetFirst()->GetInfo());
			list.Addtop(list.GetLast()->GetInfo());
			for (dgList<dgEdge*>::dgListNode* node = list.GetFirst()->GetNext(); node != list.GetLast(); node = node->GetNext()) {
				dgEdge* const edge = node->GetInfo();

				dgEdge* const prev = node->GetPrev()->GetInfo();
				edge->m_prev = prev;
				prev->m_next = edge;
				edge->m_twin->m_next = prev->m_twin;
				prev->m_twin->m_prev = edge->m_twin;

				dgEdge* const next = node->GetNext()->GetInfo();
				edge->m_next = next;
				next->m_prev = edge;
				edge->m_twin->m_prev = next->m_twin;
				next->m_twin->m_next = edge->m_twin;

				dgBigVector dist(mesh->GetVertex(edge->m_incidentVertex) - p);
				dgFloat64 err2 = dist % dist;
				if (err2 < dist2) {
					closestEdge = edge;
					dist2 = err2;
				}
			}

			dgBigVector faceNormal (mesh->FaceNormal(face, mesh->GetVertexPool(), mesh->GetVertexStrideInByte()));
			dgBigVector clipNormal (mesh->FaceNormal(closestEdge, mesh->GetVertexPool(), mesh->GetVertexStrideInByte()));
			if ((clipNormal % faceNormal) > dgFloat64(0.0f)) {
				closestEdge = closestEdge->m_twin->m_next;
			}
			dgEdge* const glueEdge = mesh->ConnectVertex (closestEdge, face);
			dgAssert (glueEdge);
			mesh->PolygonizeFace(glueEdge, mesh->GetVertexPool(), sizeof (dgBigVector));
		}

		void EmbedCurveToMulipleFaces (dgList<dgTreeNode*>& curve, dgMeshEffect* const mesh)
		{
			for (dgList<dgTreeNode*>::dgListNode* node = curve.GetFirst(); node; node = node->GetNext()) {
				dgPoint& point = node->GetInfo()->GetInfo();
				if (point.m_edgeOwnerMesh == mesh) {
					dgEdge* const edge = point.m_edge;
					dgBigVector p0 (mesh->GetVertex(edge->m_incidentVertex));
					dgBigVector p1 (mesh->GetVertex(edge->m_twin->m_incidentVertex));
					dgVector p1p0 (p1 - p0);
					dgVector qp0 (point.m_posit - p0);
					dgFloat64 param = (qp0 % p1p0) / (p1p0 % p1p0);
					dgAssert (param >= dgFloat64 (0.0f));
					dgAssert (param <= dgFloat64 (1.0f));
					dgEdge* const newEdge = mesh->InsertEdgeVertex (edge, param);
				}
//				mesh->AddVertex(point.m_posit);
//				mesh->AddAtribute(attribute);
			}
		}


		void AddCurveToMesh (dgList<dgTreeNode*>& curve, dgMeshEffect* const mesh)
		{
			bool isIscribedInFace = true; 
			dgEdge* const face = curve.GetFirst()->GetInfo()->GetInfo().m_face;
			for (dgList<dgTreeNode*>::dgListNode* node = curve.GetFirst(); isIscribedInFace && node; node = node->GetNext()) {
				dgPoint& point = node->GetInfo()->GetInfo();
				isIscribedInFace = isIscribedInFace && (point.m_face == face);
				isIscribedInFace = isIscribedInFace && (point.m_faceOwnerMesh == mesh);
			}

			if (isIscribedInFace) {
				EmbedCurveToSingleFace (curve, mesh);
			} else {
				EmbedCurveToMulipleFaces (curve, mesh);
			}
		}

		void Colorize()
		{
			m_lru ++;
			Iterator iter (*this);
			for (iter.Begin(); iter; iter ++) {
				dgPoint& point = iter.GetNode()->GetInfo();
				if (point.m_lru != m_lru) {
					dgList<dgTreeNode*> curve (GetAllocator());
					GetCurve (curve, iter.GetNode());
					AddCurveToMesh (curve, m_meshB);
					AddCurveToMesh (curve, m_meshA);
				}
			}

			m_meshA->SaveOFF("xxxA0.off");
			m_meshB->SaveOFF("xxxB0.off");
		}

		dgMeshEffect* m_meshA;
		dgMeshEffect* m_meshB;
		dgInt32 m_pointBaseA;
		dgInt32 m_pointBaseB;
		dgInt32 m_lru;
*/
	};

	class dgClippedFace: public dgMeshEffect
	{
		public:
		dgClippedFace ()
			:dgMeshEffect()
			,m_curveNetwork()
		{
			dgAssert (0);
		}

		dgClippedFace (dgMemoryAllocator* const allocator)
			:dgMeshEffect(allocator)
			,m_curveNetwork(allocator)
		{
		}

		dgClippedFace (const dgClippedFace& copy)
			:dgMeshEffect(copy)
			,m_curveNetwork(copy.m_curveNetwork)
		{
		}

		void InitFace(dgMeshEffect* const mesh, dgEdge* const face)
		{
			dgInt32 indexCount = 0;
			dgInt32 faceIndex[256];
			dgInt64 faceDataIndex[256];
			BeginFace ();
			dgEdge* ptr = face;
			do {
				dgAssert (0);
				//const dgMeshEffect::dgVertexAtribute& point =  mesh->GetAttribute(dgInt32 (ptr->m_userData));
				//AddPoint (&point.m_vertex.m_x, dgInt32 (point.m_material));
				faceIndex[indexCount] = indexCount;
				faceDataIndex[indexCount] = indexCount;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			AddFace (indexCount, faceIndex, faceDataIndex);
			EndFace ();
		}


		void AddSegment (const dgBigVector& plane, const dgBigVector* const segment)
		{
			dgAssert (0);
/*
			dgCurvesNetwork::dgTreeNode* const node0 = m_curveNetwork.AddVertex (segment[0], GetAllocator());
			dgCurvesNetwork::dgTreeNode* const node1 = m_curveNetwork.AddVertex (segment[1], GetAllocator());

			dgPoint& pointA = node0->GetInfo();
			dgPoint& pointB = node1->GetInfo();
			pointA.m_links.Append(node1);
			pointB.m_links.Append(node0);
*/
		}

		dgCurvesNetwork m_curveNetwork;
	};

	class dgClipppedFaces: public dgTree<dgClippedFace, dgEdge*>
	{
		public:
		dgClipppedFaces(dgMeshEffect* const mesh)
			:dgTree<dgClippedFace, dgEdge*>(mesh->GetAllocator())
			,m_parentMesh (mesh)
		{
		}

		void ClipMeshesFaces(dgEdge* const faceA, const dgMeshEffect* const meshB, dgEdge* const faceB, const dgBigVector& planeB, const dgBigVector* const segment)
		{
			dgTreeNode* node = Find (faceA);
			if (!node) {
				dgClippedFace tmp (m_parentMesh->GetAllocator());
				node = Insert (tmp, faceA);
				dgClippedFace& faceHead = node->GetInfo();
				faceHead.InitFace (m_parentMesh, faceA);
			}
			dgAssert (node);
			dgClippedFace& faceHead = node->GetInfo();
			faceHead.AddSegment(planeB, segment);
		}

		dgMeshEffect* m_parentMesh;
	};
	

	public:
	dgBooleanMeshClipper(dgMeshEffect* const mesh)
		:dgMeshBVH(mesh)
		,m_clippedFaces(mesh)
	{
		dgMeshBVH::Build();
	}

	~dgBooleanMeshClipper()
	{
	}

/*
	dgFloat64 IntersetionSegment(const dgMeshEffect* const meshEdge, dgEdge* const edge, const dgMeshEffect* const meshFace, dgEdge* const face, const dgHugeVector& plane)
	{
		dgHugeVector p0 (meshEdge->GetVertex(edge->m_incidentVertex));
		dgHugeVector p1 (meshEdge->GetVertex(edge->m_twin->m_incidentVertex));
			
		dgGoogol test0 (plane.EvaluePlane(p0));
		dgGoogol test1 (plane.EvaluePlane(p1));

		if ((test0 * test1) > dgGoogol::m_zero) {
			// both point are in one side
			return -1.0f;
		}

		if ((test0 * test1) < dgGoogol::m_zero) {
			//point on different size, clip the line
			dgHugeVector p1p0 (p1 - p0);
			dgGoogol param = dgGoogol::m_zero - plane.EvaluePlane(p0) / (plane % p1p0);
			dgHugeVector p (p0 + p1p0.Scale (param));
			if (IsPointInFace (p, meshFace, face, plane)) {
				return param;
			}
			return -1.0f;
		} else {
			dgAssert (0);
			//special cases;
		}
			
		return -1.0f;
	}
*/

	static dgHugeVector CalculateFaceNormal (const dgMeshEffect* const mesh, dgEdge* const face)
	{
		dgHugeVector plane(dgGoogol::m_zero, dgGoogol::m_zero, dgGoogol::m_zero, dgGoogol::m_zero);
		dgEdge* edge = face;
		dgHugeVector p0(mesh->GetVertex(edge->m_incidentVertex));
		edge = edge->m_next;
		dgHugeVector p1(mesh->GetVertex(edge->m_incidentVertex));
		dgHugeVector p1p0(p1 - p0);
		edge = edge->m_next;
		do {
			dgHugeVector p2(mesh->GetVertex(edge->m_incidentVertex));
			dgHugeVector p2p0(p2 - p0);
			plane += p1p0.CrossProduct(p2p0);
			p1p0 = p2p0;
			edge = edge->m_next;
		} while (edge != face);

		dgAssert(plane.m_w == dgGoogol(0.0));
		plane.m_w = dgGoogol::m_zero - plane.DotProduct(p0).GetScalar();
		return plane;
	}

	static bool IsPointInFace (const dgHugeVector& point, const dgMeshEffect* const mesh, dgEdge* const face, const dgHugeVector& normal)
	{
dgEdge* ptr = face;
dgTrace (("p (%f %f %f)\n", dgFloat64 (point.m_x), dgFloat64 (point.m_y), dgFloat64 (point.m_z)));
do {
	dgBigVector p1(mesh->GetVertex(ptr->m_incidentVertex));
	dgTrace (("f (%f %f %f)\n", dgFloat64 (p1.m_x), dgFloat64 (p1.m_y), dgFloat64 (p1.m_z)));
	ptr = ptr->m_next;
} while (ptr != face);

		dgEdge* edge = face;
		dgHugeVector p0(mesh->GetVertex(face->m_incidentVertex));
		do {
			dgHugeVector p1(mesh->GetVertex(edge->m_twin->m_incidentVertex));
			dgHugeVector p1p0(p1 - p0);
			dgHugeVector q1p0(point - p0);
			dgAssert(p1p0.m_w == dgGoogol(0.0));
			dgGoogol side (q1p0.DotProduct(p1p0.CrossProduct(normal)).GetScalar());
			if (side >= dgGoogol::m_zero) {
				return false;
			}
			p0 = p1;
			edge = edge->m_next;
		} while (edge != face);

		return true;
	}

	static bool ClipEdgeFace(dgBigVector& point, const dgMeshEffect* const meshEdge, dgEdge* const edgeSrc, const dgMeshEffect* const meshFace, dgEdge* const face, const dgHugeVector& plane)
	{
		const dgEdge* const edge = (edgeSrc->m_incidentVertex < edgeSrc->m_twin->m_incidentVertex) ? edgeSrc : edgeSrc->m_twin;
		dgHugeVector p0 (meshEdge->GetVertex(edge->m_incidentVertex));
		dgHugeVector p1 (meshEdge->GetVertex(edge->m_twin->m_incidentVertex));
			
		dgGoogol test0 (plane.EvaluePlane(p0));
		dgGoogol test1 (plane.EvaluePlane(p1));

		if ((test0 * test1) > dgGoogol::m_zero) {
			// both point are in one side
			return false;
		}

		if ((test0 * test1) < dgGoogol::m_zero) {
			//point on different size, clip the line
			dgHugeVector p1p0 (p1 - p0);
			dgAssert(p1p0.m_w == dgGoogol(0.0));
			dgGoogol param = dgGoogol::m_zero - plane.EvaluePlane(p0) / plane.DotProduct(p1p0).GetScalar();
			dgHugeVector p (p0 + p1p0.Scale (param));
			if (IsPointInFace (p, meshFace, face, plane)) {
				point = dgBigVector(p.m_x, p.m_y, p.m_z, p.m_w);
				return true;
			}
			return false;
		} else {
			dgAssert (0);
			//special cases;
		}
			
		return false;
	}

	static void CalculateIntersection (const dgMeshEffect* const edgeOwnerMesh, dgEdge* const edgeStart, const dgMeshEffect* const faceOwnerMesh, dgEdge* const face, const dgHugeVector& facePlane, dgBigVector* const data, dgInt32& index)
	{
		dgEdge* edge = edgeStart;
		do {
			bool isCleipped = ClipEdgeFace(data[index], edgeOwnerMesh, edge, faceOwnerMesh, face, facePlane);
			if (isCleipped) {
				index ++;
			}
			edge = edge->m_next;
		} while (edge != edgeStart);
	}

	static void ClipMeshesFaces(dgBooleanMeshClipper& bvhMeshA, dgEdge* const faceA, dgBooleanMeshClipper& bvhMeshB, dgEdge* const faceB)
	{
		const dgMeshEffect* const meshA = bvhMeshA.m_mesh;
		const dgMeshEffect* const meshB = bvhMeshB.m_mesh;
		dgAssert (meshA->FindEdge(faceA->m_incidentVertex, faceA->m_twin->m_incidentVertex) == faceA);
		dgAssert (meshB->FindEdge(faceB->m_incidentVertex, faceB->m_twin->m_incidentVertex) == faceB);

		dgHugeVector planeA (CalculateFaceNormal (meshA, faceA));
		dgHugeVector planeB (CalculateFaceNormal (meshB, faceB));

		dgBigVector points[16];
		dgInt32 pointCount = 0;
		CalculateIntersection (meshA, faceA, meshB, faceB, planeB, points, pointCount);
		CalculateIntersection (meshB, faceB, meshA, faceA, planeA, points, pointCount);
		dgAssert ((pointCount == 0) || (pointCount == 2));
		if (pointCount == 2) {
			dgBigVector facePlaneA (planeA.m_x, planeA.m_y, planeA.m_z, planeA.m_w);
			dgBigVector facePlaneB (planeB.m_x, planeB.m_y, planeB.m_z, planeB.m_w);

			bvhMeshA.m_clippedFaces.ClipMeshesFaces(faceA, meshB, faceB, facePlaneB, points);
			bvhMeshB.m_clippedFaces.ClipMeshesFaces(faceB, meshA, faceA, facePlaneA, points);
		}
	}

	static void ClipMeshesAndColorize(dgMeshEffect* const meshA, dgMeshEffect* const meshB)
	{
		dgAssert (0);
/*
		dgBooleanMeshClipper BVHmeshA(meshA);
		dgBooleanMeshClipper BVHmeshB(meshB);

		int stack = 1;
		
		dgMeshBVHNode* stackPool[2 * DG_MESH_EFFECT_BVH_STACK_DEPTH][2];

		stackPool[0][0] = BVHmeshA.m_rootNode;
		stackPool[0][1] = BVHmeshB.m_rootNode;
		while (stack) {
			stack --;
			dgMeshBVHNode* const nodeA = stackPool[stack][0];
			dgMeshBVHNode* const nodeB = stackPool[stack][1];
			if (dgOverlapTest (nodeA->m_p0, nodeA->m_p1, nodeB->m_p0, nodeB->m_p1)) {
				if (nodeA->m_face && nodeB->m_face) {
					ClipMeshesFaces(BVHmeshA, nodeA->m_face, BVHmeshB, nodeB->m_face);
				} else if (nodeA->m_face) {
					stackPool[stack][0] = nodeA;
					stackPool[stack][1] = nodeB->m_left;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));

					stackPool[stack][0] = nodeA;
					stackPool[stack][1] = nodeB->m_right;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));
					
				} else if (nodeB->m_face) {
					stackPool[stack][0] = nodeA->m_left;
					stackPool[stack][1] = nodeB;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));

					stackPool[stack][0] = nodeA->m_right;
					stackPool[stack][1] = nodeB;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));

				} else {
					stackPool[stack][0] = nodeA->m_left;
					stackPool[stack][1] = nodeB->m_left;
					stack ++;
					dgAssert (stack < sizeof (stackPool) / sizeof (stackPool[0]));

					stackPool[stack][0] = nodeA->m_left;
					stackPool[stack][1] = nodeB->m_right;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));

					stackPool[stack][0] = nodeA->m_right;
					stackPool[stack][1] = nodeB->m_left;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));

					stackPool[stack][0] = nodeA->m_right;
					stackPool[stack][1] = nodeB->m_right;
					stack++;
					dgAssert(stack < sizeof (stackPool) / sizeof (stackPool[0]));
				}
			}
		}
*/
		dgAssert (0);
//		network.Colorize();
	

/*
		dgInt32 baseAttibuteCountB = BVHmeshB.m_mesh->GetPropertiesCount();

		BVHmeshA.m_mesh->SaveOFF("xxxA0.off");
		BVHmeshB.m_mesh->SaveOFF("xxxB0.off");

		// edge-face, edge-edge and edge-vertex intersections until not more intersections are found 
		for (bool intersectionFound = true; intersectionFound;) {
			intersectionFound = false;

			intersectionFound |= BVHmeshA.CalculateEdgeFacesIntersetions(BVHmeshB);
			intersectionFound |= BVHmeshB.CalculateEdgeFacesIntersetions(BVHmeshA);

			intersectionFound |= BVHmeshA.CalculateVertexFacesIntersetions(BVHmeshB);
			intersectionFound |= BVHmeshB.CalculateVertexFacesIntersetions(BVHmeshA);


			BVHmeshA.m_mesh->SaveOFF("xxxA1.off");
			BVHmeshB.m_mesh->SaveOFF("xxxB1.off");

			intersectionFound |= BVHmeshA.CalculateEdgeEdgeIntersetions(BVHmeshB);

			BVHmeshA.m_mesh->SaveOFF("xxxA2.off");
			BVHmeshB.m_mesh->SaveOFF("xxxB2.off");

			intersectionFound |= BVHmeshA.CalculateEdgeVertexIntersetions(BVHmeshB);
			intersectionFound |= BVHmeshB.CalculateEdgeVertexIntersetions(BVHmeshA);

			BVHmeshA.m_mesh->SaveOFF("xxxA3.off");
			BVHmeshB.m_mesh->SaveOFF("xxxB3.off");
		}
*/		
	}

	dgClipppedFaces m_clippedFaces;
};



dgMeshEffect* dgMeshEffect::Intersection (const dgMatrix& matrix, const dgMeshEffect* const clipperMesh) const
{
	dgMeshEffect copy (*this);
	dgMeshEffect clipper (*clipperMesh);
	clipper.TransformMesh (matrix);

	dgBooleanMeshClipper::ClipMeshesAndColorize (&copy, &clipper);
/*
	dgMeshEffect* const mesh = new (GetAllocator()) dgMeshEffect (GetAllocator());
	mesh->BeginFace();
	dgBooleanMeshClipper::CopyPoints(mesh, &copy);
	dgBooleanMeshClipper::AddInteriorFaces (mesh, &copy);
	dgBooleanMeshClipper::AddInteriorFaces (mesh, &clipper);
	mesh->EndFace ();
	mesh->RepairTJoints();
	mesh->RemoveUnusedVertices(NULL);

	return mesh;
*/

	dgAssert (0);
	return NULL;
}


