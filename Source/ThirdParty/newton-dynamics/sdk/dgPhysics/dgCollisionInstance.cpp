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
#include "dgCollisionBox.h"
#include "dgCollisionBVH.h"
#include "dgCollisionMesh.h"
#include "dgCollisionNull.h"
#include "dgCollisionCone.h"
#include "dgCollisionSphere.h"
#include "dgCollisionCapsule.h"
#include "dgCollisionCylinder.h"
#include "dgCollisionInstance.h"
#include "dgCollisionCompound.h"
#include "dgCollisionHeightField.h"
#include "dgCollisionConvexPolygon.h"
#include "dgCollisionChamferCylinder.h"
#include "dgCollisionCompoundFractured.h"
#include "dgCollisionDeformableSolidMesh.h"
#include "dgCollisionMassSpringDamperSystem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


dgVector dgCollisionInstance::m_padding (DG_MAX_COLLISION_AABB_PADDING, DG_MAX_COLLISION_AABB_PADDING, DG_MAX_COLLISION_AABB_PADDING, dgFloat32 (0.0f));

dgCollisionInstance::dgCollisionInstance()
	:m_globalMatrix(dgGetIdentityMatrix())
	,m_localMatrix (dgGetIdentityMatrix())
	,m_aligmentMatrix (dgGetIdentityMatrix())
	,m_scale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_invScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_maxScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_material()
	,m_world(NULL)
	,m_childShape (NULL)
	,m_subCollisionHandle(NULL)
	,m_parent(NULL)
	,m_skinThickness(dgFloat32 (0.0f))
	,m_collisionMode(1)
	,m_refCount(1)
	,m_scaleType(m_unit)
{
}

dgCollisionInstance::dgCollisionInstance(const dgWorld* const world, const dgCollision* const childCollision, dgInt32 shapeID, const dgMatrix& matrix)
	:m_globalMatrix(matrix)
	,m_localMatrix (matrix)
	,m_aligmentMatrix (dgGetIdentityMatrix())
	,m_scale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_invScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_maxScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_material()
	,m_world(world)
	,m_childShape (childCollision)
	,m_subCollisionHandle(NULL)
	,m_parent(NULL)
	,m_skinThickness(dgFloat32 (0.0f))
	,m_collisionMode(1)
	,m_refCount(1)
	,m_scaleType(m_unit)
{
	m_material.m_userId = shapeID;
	m_childShape->AddRef();
}

dgCollisionInstance::dgCollisionInstance(const dgCollisionInstance& instance)
	:m_globalMatrix(instance.m_globalMatrix)
	,m_localMatrix (instance.m_localMatrix)
	,m_aligmentMatrix (instance.m_aligmentMatrix)
	,m_scale(instance.m_scale)
	,m_invScale(instance.m_invScale)
	,m_maxScale(instance.m_maxScale)
	,m_material(instance.m_material)
	,m_world(instance.m_world)
	,m_childShape (instance.m_childShape)
	,m_subCollisionHandle(NULL)
	,m_parent(NULL)
	,m_skinThickness(instance.m_skinThickness)
	,m_collisionMode(instance.m_collisionMode)
	,m_refCount(1)
	,m_scaleType(instance.m_scaleType)
{
	if (m_childShape->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		if (m_childShape->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
			dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) m_childShape;
			m_childShape = new (m_world->GetAllocator()) dgCollisionCompoundFractured (*compound, this);
		} else if (m_childShape->IsType (dgCollision::dgCollisionScene_RTTI)) {
			dgCollisionScene* const scene = (dgCollisionScene*) m_childShape;
			m_childShape = new (m_world->GetAllocator()) dgCollisionScene (*scene, this);
		} else {
			dgCollisionCompound *const compound = (dgCollisionCompound*) m_childShape;
			m_childShape = new (m_world->GetAllocator()) dgCollisionCompound (*compound, this);
		}
	} else if (m_childShape->IsType (dgCollision::dgCollisionMassSpringDamperSystem_RTTI)) {
		dgCollisionMassSpringDamperSystem* const deformable = (dgCollisionMassSpringDamperSystem*) m_childShape;
		m_childShape = new (m_world->GetAllocator()) dgCollisionMassSpringDamperSystem (*deformable);
	} else if (m_childShape->IsType (dgCollision::dgCollisionDeformableSolidMesh_RTTI)) {
		dgCollisionDeformableSolidMesh* const deformable = (dgCollisionDeformableSolidMesh*) m_childShape;
		m_childShape = new (m_world->GetAllocator()) dgCollisionDeformableSolidMesh (*deformable);
	} else {
		m_childShape->AddRef();
	}

	if (m_world->m_onCollisionInstanceCopyConstrutor) {
		m_world->m_onCollisionInstanceCopyConstrutor (m_world, this, &instance);
	}
}

dgCollisionInstance::dgCollisionInstance(const dgWorld* const constWorld, dgDeserialize serialize, void* const userData, dgInt32 revisionNumber)
	:m_globalMatrix(dgGetIdentityMatrix())
	,m_localMatrix (dgGetIdentityMatrix())
	,m_aligmentMatrix (dgGetIdentityMatrix())
	,m_scale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_invScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_maxScale(dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f))
	,m_material()
	,m_world(constWorld)
	,m_childShape (NULL)
	,m_subCollisionHandle(NULL)
	,m_parent(NULL)
	,m_skinThickness(dgFloat32 (0.0f))
	,m_collisionMode(1)
	,m_refCount(1)
	,m_scaleType(m_unit)
{
	dgInt32 saved;
	dgInt32 signature;
	dgInt32 primitive;
	dgInt32 scaleType;
	
	serialize (userData, &m_globalMatrix, sizeof (m_globalMatrix));
	serialize (userData, &m_localMatrix, sizeof (m_localMatrix));
	serialize (userData, &m_aligmentMatrix, sizeof (m_aligmentMatrix));
	serialize (userData, &m_scale, sizeof (m_scale));
	serialize (userData, &m_invScale, sizeof (m_invScale));
	serialize (userData, &m_maxScale, sizeof (m_maxScale));
	serialize (userData, &m_skinThickness, sizeof (m_skinThickness));
	serialize (userData, &m_material, sizeof (m_material));
	serialize (userData, &m_collisionMode, sizeof (m_collisionMode));
	serialize (userData, &scaleType, sizeof (scaleType));
	serialize (userData, &primitive, sizeof (primitive));
	serialize (userData, &signature, sizeof (signature));
	serialize (userData, &saved, sizeof (saved));

	m_scaleType = dgScaleType(scaleType);

	dgWorld* const world = (dgWorld*) constWorld;
	if (saved) {
		const dgCollision* collision = NULL;
		dgBodyCollisionList::dgTreeNode* node = world->dgBodyCollisionList::Find (dgUnsigned32 (signature));

		if (node) {
			collision = node->GetInfo();
			collision->AddRef();

		} else {

			dgCollisionID primitiveType = dgCollisionID(primitive);

			dgMemoryAllocator* const allocator = world->GetAllocator();
			switch (primitiveType)
			{
				case m_heightField:
				{
					collision = new (allocator) dgCollisionHeightField (world, serialize, userData, revisionNumber);
					break;
				}

				case m_boundingBoxHierachy:
				{
					collision = new (allocator) dgCollisionBVH (world, serialize, userData, revisionNumber);
					break;
				}

				case m_compoundCollision:
				{
					collision = new (allocator) dgCollisionCompound (world, serialize, userData, this, revisionNumber);
					break;
				}

				case m_compoundFracturedCollision:
				{
					collision = new (allocator) dgCollisionCompoundFractured (world, serialize, userData, this, revisionNumber);
					break;
				}

				case m_sceneCollision:
				{
					collision = new (allocator) dgCollisionScene (world, serialize, userData, this, revisionNumber);
					break;
				}


				case m_sphereCollision:
				{
					collision = new (allocator) dgCollisionSphere (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_boxCollision:
				{
					collision = new (allocator) dgCollisionBox (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_coneCollision:
				{
					collision = new (allocator) dgCollisionCone (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_capsuleCollision:
				{
					collision = new (allocator) dgCollisionCapsule (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_cylinderCollision:
				{
					collision = new (allocator) dgCollisionCylinder (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_chamferCylinderCollision:
				{
					collision = new (allocator) dgCollisionChamferCylinder (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_convexHullCollision:
				{
					collision = new (allocator) dgCollisionConvexHull (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

				case m_nullCollision:
				{
					collision = new (allocator) dgCollisionNull (world, serialize, userData, revisionNumber);
					node = world->dgBodyCollisionList::Insert (collision, collision->GetSignature());
					collision->AddRef();
					break;
				}

//				case m_deformableMesh:
//				{
//					dgAssert (0);
//					return NULL;
//				}

				default:
				dgAssert (0);
			}
		}
		m_childShape = collision;
	}
	dgDeserializeMarker (serialize, userData);
}

dgCollisionInstance::~dgCollisionInstance()
{
	if (m_world->m_onCollisionInstanceDestruction) {
		m_world->m_onCollisionInstanceDestruction (m_world, this);
	}
	dgWorld* const world = (dgWorld*)m_world;
	world->ReleaseCollision(m_childShape);
}

void dgCollisionInstance::Serialize(dgSerialize serialize, void* const userData, bool saveShape) const
{
	dgInt32 save = saveShape ? 1 : 0;
	dgInt32 primitiveType = m_childShape->GetCollisionPrimityType();
	dgInt32 signature = m_childShape->GetSignature();
	dgInt32 scaleType = m_scaleType;

	serialize (userData, &m_globalMatrix, sizeof (m_globalMatrix));
	serialize (userData, &m_localMatrix, sizeof (m_localMatrix));
	serialize (userData, &m_aligmentMatrix, sizeof (m_aligmentMatrix));
	serialize (userData, &m_scale, sizeof (m_scale));
	serialize (userData, &m_invScale, sizeof (m_invScale));
	serialize (userData, &m_maxScale, sizeof (m_maxScale));
	serialize (userData, &m_skinThickness, sizeof (m_skinThickness));
	serialize (userData, &m_material, sizeof (m_material));
	serialize (userData, &m_collisionMode, sizeof (m_collisionMode));
	serialize (userData, &scaleType, sizeof (scaleType));
	serialize (userData, &primitiveType, sizeof (primitiveType));
	serialize (userData, &signature, sizeof (signature));
	serialize (userData, &save, sizeof (save));
	if (saveShape) {
		m_childShape->Serialize(serialize, userData);
	}
	dgSerializeMarker(serialize, userData);
}


void dgCollisionInstance::SetScale (const dgVector& scale)
{
	dgFloat32 scaleX = dgAbs (scale.m_x);
	dgFloat32 scaleY = dgAbs (scale.m_y);
	dgFloat32 scaleZ = dgAbs (scale.m_z);
	dgAssert (scaleX > dgFloat32 (0.0f));
	dgAssert (scaleY > dgFloat32 (0.0f));
	dgAssert (scaleZ > dgFloat32 (0.0f));

	if (IsType(dgCollision::dgCollisionCompound_RTTI)) {
		dgAssert (m_scaleType == m_unit);
		dgCollisionCompound* const compound = (dgCollisionCompound*) m_childShape;
		compound->ApplyScale(scale);
	} else if ((dgAbs (scaleX - scaleY) < dgFloat32 (1.0e-4f)) && (dgAbs (scaleX - scaleZ) < dgFloat32 (1.0e-4f))) {
		if ((dgAbs (scaleX - dgFloat32 (1.0f)) < dgFloat32 (1.0e-4f))) {
			m_scaleType = m_unit;
			m_scale	= dgVector (dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f));	
			m_maxScale = m_scale;	
			m_invScale = m_scale;
		} else {
			m_scaleType = m_uniform;
			m_scale	= dgVector (scaleX, scaleX, scaleX, dgFloat32 (0.0f));	
			m_maxScale = m_scale;	
			m_invScale = dgVector (dgFloat32 (1.0f) / scaleX, dgFloat32 (1.0f) / scaleX, dgFloat32 (1.0f) / scaleX, dgFloat32 (0.0f));	
		}
	} else {
		m_scaleType = m_nonUniform;
		m_maxScale = dgMax(scaleX, scaleY, scaleZ);
		m_scale	= dgVector (scaleX, scaleY, scaleZ, dgFloat32 (0.0f));	
		m_invScale = dgVector (dgFloat32 (1.0f) / scaleX, dgFloat32 (1.0f) / scaleY, dgFloat32 (1.0f) / scaleZ, dgFloat32 (0.0f));	
	}
}

void dgCollisionInstance::SetGlobalScale (const dgVector& scale)
{
	// calculate current matrix
	dgMatrix matrix(dgGetIdentityMatrix());
	matrix[0][0] = m_scale.m_x;
	matrix[1][1] = m_scale.m_y;
	matrix[2][2] = m_scale.m_z;
	matrix = m_aligmentMatrix * matrix * m_localMatrix;

	// extract the original local matrix
	dgMatrix transpose (matrix.Transpose());
	dgVector globalScale (dgSqrt (transpose[0].DotProduct(transpose[0]).GetScalar()), dgSqrt (transpose[1].DotProduct(transpose[1]).GetScalar()), dgSqrt (transpose[2].DotProduct(transpose[2]).GetScalar()), dgFloat32 (1.0f));
	dgVector invGlobalScale (dgFloat32 (1.0f) / globalScale.m_x, dgFloat32 (1.0f) / globalScale.m_y, dgFloat32 (1.0f) / globalScale.m_z, dgFloat32 (1.0f));
	dgMatrix localMatrix (m_aligmentMatrix.Transpose() * m_localMatrix);
	localMatrix.m_posit = matrix.m_posit * invGlobalScale;
	dgAssert (localMatrix.m_posit.m_w == dgFloat32 (1.0f));

	if ((dgAbs (scale[0] - scale[1]) < dgFloat32 (1.0e-4f)) && (dgAbs (scale[0] - scale[2]) < dgFloat32 (1.0e-4f))) {
		m_localMatrix = localMatrix;
		m_localMatrix.m_posit = m_localMatrix.m_posit * scale | dgVector::m_wOne;
		m_aligmentMatrix = dgGetIdentityMatrix();
		SetScale (scale);
	} else {
		
		// create a new scale matrix 
		localMatrix[0] = localMatrix[0] * scale;
		localMatrix[1] = localMatrix[1] * scale;
		localMatrix[2] = localMatrix[2] * scale;
		localMatrix[3] = localMatrix[3] * scale;
		localMatrix[3][3] = dgFloat32 (1.0f);

		// decompose into to align * scale * local
		localMatrix.PolarDecomposition (m_localMatrix, m_scale, m_aligmentMatrix);

		m_localMatrix = m_aligmentMatrix * m_localMatrix;
		m_aligmentMatrix = m_aligmentMatrix.Transpose();

		dgAssert (m_localMatrix.TestOrthogonal());
		dgAssert (m_aligmentMatrix.TestOrthogonal());

//dgMatrix xxx1 (dgGetIdentityMatrix());
//xxx1[0][0] = m_scale.m_x;
//xxx1[1][1] = m_scale.m_y;
//xxx1[2][2] = m_scale.m_z;
//dgMatrix xxx (m_aligmentMatrix * xxx1 * m_localMatrix);

		bool isIdentity = true;
		for (dgInt32 i = 0; i < 3; i ++) {
			isIdentity &= dgAbs (m_aligmentMatrix[i][i] - dgFloat32 (1.0f)) < dgFloat32 (1.0e-5f);
			isIdentity &= dgAbs (m_aligmentMatrix[3][i]) < dgFloat32 (1.0e-5f);
		}
		m_scaleType = isIdentity ? m_nonUniform : m_global;

		m_maxScale = dgMax(m_scale[0], m_scale[1], m_scale[2]);
		m_invScale = dgVector (dgFloat32 (1.0f) / m_scale[0], dgFloat32 (1.0f) / m_scale[1], dgFloat32 (1.0f) / m_scale[2], dgFloat32 (0.0f));	
	}
}


void dgCollisionInstance::SetLocalMatrix (const dgMatrix& matrix)
{
	m_localMatrix = matrix;
	m_localMatrix[0][3] = dgFloat32 (0.0f);
	m_localMatrix[1][3] = dgFloat32 (0.0f);
	m_localMatrix[2][3] = dgFloat32 (0.0f);
	m_localMatrix[3][3] = dgFloat32 (1.0f);
	dgAssert(m_localMatrix.TestOrthogonal());
}


void dgCollisionInstance::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	m_childShape->DebugCollision (GetScaledTransform(matrix), callback, userData);
}


dgMatrix dgCollisionInstance::CalculateInertia () const
{
	if (IsType(dgCollision::dgCollisionMesh_RTTI)) {
		return dgGetZeroMatrix();
	} else {
		return m_childShape->CalculateInertiaAndCenterOfMass (m_aligmentMatrix, m_scale, m_localMatrix);
	}
}


dgInt32 dgCollisionInstance::CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const
{
	dgInt32 count = 0;
	dgAssert(normal.m_w == dgFloat32 (0.0f));
	switch (m_scaleType)
	{
		case m_unit:
		{
			count = m_childShape->CalculatePlaneIntersection (normal, point, contactsOut);
			break;
		}
		case m_uniform:
		{
			dgVector point1 (m_invScale * point);
			count = m_childShape->CalculatePlaneIntersection (normal, point1, contactsOut);
			for (dgInt32 i = 0; i < count; i ++) {
				contactsOut[i] = m_scale * contactsOut[i];
			}
			break;
		}

		case m_nonUniform:
		{
			// support((p * S), n) = S * support (p, n * transp(S)) 
			dgVector point1 (m_invScale * point);
			dgVector normal1 (m_scale * normal);
			normal1 = normal1.Normalize();
			count = m_childShape->CalculatePlaneIntersection (normal1, point1, contactsOut);
			for (dgInt32 i = 0; i < count; i ++) {
				contactsOut[i] = m_scale * contactsOut[i];
			}
			break;
		}

		case m_global:
		default:
		{
			dgVector point1 (m_aligmentMatrix.UntransformVector (m_invScale * point));
			dgVector normal1 (m_aligmentMatrix.UntransformVector (m_scale * normal));
			normal1 = normal1.Normalize();
			count = m_childShape->CalculatePlaneIntersection (normal1, point1, contactsOut);
			for (dgInt32 i = 0; i < count; i ++) {
				contactsOut[i] = m_scale * m_aligmentMatrix.TransformVector(contactsOut[i]);
			}
		}
	}
	return count;
}


void dgCollisionInstance::CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const
{
	switch (m_scaleType)
	{
		case m_unit:
		{
			m_childShape->CalcAABB (matrix, p0, p1);
			p0 -= m_padding;
			p1 += m_padding;
			break;
		}

		case m_uniform:
		case m_nonUniform:
		{
			dgMatrix matrix1 (matrix);
			matrix1[0] = matrix1[0].Scale(m_scale.m_x);
			matrix1[1] = matrix1[1].Scale(m_scale.m_y);
			matrix1[2] = matrix1[2].Scale(m_scale.m_z);
			m_childShape->CalcAABB (matrix1, p0, p1);
			p0 -= m_padding;
			p1 += m_padding;
			break;
		}

		case m_global:
		default:
		{
			dgMatrix matrix1 (matrix);
			matrix1[0] = matrix1[0].Scale(m_scale.m_x);
			matrix1[1] = matrix1[1].Scale(m_scale.m_y);
			matrix1[2] = matrix1[2].Scale(m_scale.m_z);
			m_childShape->CalcAABB (m_aligmentMatrix * matrix1, p0, p1);
			p0 -= m_padding;
			p1 += m_padding;
			break;
		}
	}

	dgAssert (p0.m_w == dgFloat32 (0.0f));
	dgAssert (p1.m_w == dgFloat32 (0.0f));
}


dgFloat32 dgCollisionInstance::RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, OnRayPrecastAction preFilter, const dgBody* const body, void* const userData) const
{
	if (!preFilter || preFilter(body, this, userData)) {
		switch(m_scaleType)
		{
			case m_unit:
			{
				dgFloat32 t = m_childShape->RayCast (localP0, localP1, maxT, contactOut, body, userData, preFilter);
				if (t <= maxT) {
					if (!(m_childShape->IsType(dgCollision::dgCollisionMesh_RTTI) || m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI))) {
						contactOut.m_shapeId0 = GetUserDataID();
						contactOut.m_shapeId1 = GetUserDataID();
					}
					if (!m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI)) {
						contactOut.m_collision0 = this;
						contactOut.m_collision1 = this;
					}
				}
				return t;
			}

			case m_uniform:
			{
				dgVector p0 (localP0 * m_invScale);
				dgVector p1 (localP1 * m_invScale);
				dgFloat32 t = m_childShape->RayCast (p0, p1, maxT, contactOut, body, userData, preFilter);
				if (t <= maxT) {
					if (!(m_childShape->IsType(dgCollision::dgCollisionMesh_RTTI) || m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI))) {
						contactOut.m_shapeId0 = GetUserDataID();
						contactOut.m_shapeId1 = GetUserDataID();
					}
					if (!m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI)) {
						contactOut.m_collision0 = this;
						contactOut.m_collision1 = this;
					}
				}
				return t;
			}

			case m_nonUniform:
			{
				dgVector p0 (localP0 * m_invScale);
				dgVector p1 (localP1 * m_invScale);
				dgFloat32 t = m_childShape->RayCast (p0, p1, maxT, contactOut, body, userData, preFilter);
				if (t <= maxT) {
					if (!(m_childShape->IsType(dgCollision::dgCollisionMesh_RTTI) || m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI))) {
						contactOut.m_shapeId0 = GetUserDataID();
						contactOut.m_shapeId1 = GetUserDataID();
						dgVector n (m_invScale * contactOut.m_normal);
						contactOut.m_normal = n.Normalize();
					}
					if (!m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI)) {
						contactOut.m_collision0 = this;
						contactOut.m_collision1 = this;
					}
				}
				return t;
			}

			case m_global:
			default:
			{
				dgVector p0 (m_aligmentMatrix.UntransformVector (localP0 * m_invScale));
				dgVector p1 (m_aligmentMatrix.UntransformVector (localP1 * m_invScale));
				dgFloat32 t = m_childShape->RayCast (p0, p1, maxT, contactOut, body, userData, preFilter);
				if (t <= maxT) {
					if (!(m_childShape->IsType(dgCollision::dgCollisionMesh_RTTI) || m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI))) {
						contactOut.m_shapeId0 = GetUserDataID();
						contactOut.m_shapeId1 = GetUserDataID();
						dgVector n (m_aligmentMatrix.RotateVector(m_invScale * contactOut.m_normal));
						contactOut.m_normal = n.Normalize();
					}
					if (!(m_childShape->IsType(dgCollision::dgCollisionCompound_RTTI))) {
						contactOut.m_collision0 = this;
						contactOut.m_collision1 = this;
					}
				}
				return t;
			}
		}
	}
	return dgFloat32 (1.2f);
}


void dgCollisionInstance::CalculateBuoyancyAcceleration (const dgMatrix& matrix, const dgVector& origin, const dgVector& gravity, const dgVector& fluidPlane, dgFloat32 fluidDensity, dgFloat32 fluidViscosity, dgVector& unitForce, dgVector& unitTorque)
{
	dgMatrix globalMatrix (m_localMatrix * matrix);

	unitForce = dgVector (dgFloat32 (0.0f));
	unitTorque = dgVector (dgFloat32 (0.0f));
	dgVector volumeIntegral (m_childShape->CalculateVolumeIntegral (globalMatrix, fluidPlane, *this));
	if (volumeIntegral.m_w > dgFloat32 (0.0f)) {
		dgVector buoyanceCenter (volumeIntegral - origin);

		dgVector force (gravity.Scale (-fluidDensity * volumeIntegral.m_w));
		dgVector torque (buoyanceCenter.CrossProduct(force));

		unitForce += force;
		unitTorque += torque;
	}
}