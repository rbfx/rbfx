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
#include "dgMeshEffect.h"
#include "dgDynamicBody.h"
#include "dgCollisionIncompressibleParticles.h"


dgCollisionIncompressibleParticles::dgCollisionIncompressibleParticles(dgWorld* const world, dgMeshEffect* const mesh)
	:dgCollisionLumpedMassParticles(world, m_deformableSolidMesh)
{
	m_rtti |= dgCollisionIncompressibleParticles_RTTI;

/*
	dgInt32 count = mesh->GetVertexCount();
	dgVector* const points = dgAlloca(dgVector, count);

	for (dgInt32 i = 0; i < count; i++) {
		dgBigVector p(mesh->GetVertex(i));
		points[i] = p;
	}

	m_indexToVertexCount = count;
	m_indexToVertexMap.Resize(count);
	dgInt32* const indexToVertexMap = &m_indexToVertexMap[0];
	m_particlesCount = dgVertexListToIndexList(&points[0].m_x, sizeof(dgVector), 3 * sizeof(dgFloat32), 0, count, indexToVertexMap, dgFloat32(1.0e-5f));
	for (dgInt32 i = 0; i < m_particlesCount; i++) {
		m_posit[i] = points[i];
	}


	dgInt32 edgeCount = 0;
	dgSoftLink* const links = dgAlloca(dgSoftLink, mesh->GetCount() / 2);
	for (void* edgePtr = mesh->GetFirstEdge(); edgePtr; edgePtr = mesh->GetNextEdge(edgePtr)) {
		const dgEdge* const edge = mesh->GetPolyhedraEdgeFromNode(edgePtr);

		dgInt32 v0 = indexToVertexMap[edge->m_incidentVertex];
		dgInt32 v1 = indexToVertexMap[edge->m_twin->m_incidentVertex];
		links[edgeCount].m_m0 = dgInt16(dgMin(v0, v1));
		links[edgeCount].m_m1 = dgInt16(dgMax(v0, v1));
		edgeCount++;
		if ((edge->m_incidentFace > 0) && (edge->m_twin->m_incidentFace > 0)) {
			v0 = indexToVertexMap[edge->m_prev->m_incidentVertex];
			v1 = indexToVertexMap[edge->m_twin->m_prev->m_incidentVertex];
			links[edgeCount].m_m0 = dgInt16(dgMin(v0, v1));
			links[edgeCount].m_m1 = dgInt16(dgMax(v0, v1));
			edgeCount++;
		}
	}
	dgSort(links, edgeCount, CompareEdges);

	dgInt32 uniqueEdgeCount = 0;
	for (dgInt32 i = 1; i < edgeCount; i++) {
		if (CompareEdges(&links[i], &links[uniqueEdgeCount], NULL) > 0) {
			uniqueEdgeCount++;
			links[uniqueEdgeCount] = links[i];
		}
	}
	uniqueEdgeCount++;
	m_linksCount = uniqueEdgeCount;
	m_linkList.Resize(m_linksCount);
	for (dgInt32 i = 0; i < m_linksCount; i++) {
		m_linkList[i] = links[i];
	}
	FinalizeBuild();
*/
}


dgCollisionIncompressibleParticles::dgCollisionIncompressibleParticles(const dgCollisionIncompressibleParticles& source)
	:dgCollisionLumpedMassParticles(source)
{
	m_rtti |= source.m_rtti;
}

dgCollisionIncompressibleParticles::dgCollisionIncompressibleParticles(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionLumpedMassParticles(world, deserialization, userData, revisionNumber)
{
}

dgCollisionIncompressibleParticles::~dgCollisionIncompressibleParticles(void)
{
}


void dgCollisionIncompressibleParticles::CalculateAcceleration(dgFloat32 timestep)
{
	dgAssert(0);
}
