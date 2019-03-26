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

#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonMesh.h"
#include "dNewtonCollision.h"


dNewtonCollision::dNewtonCollision(dCollsionType type, dLong collisionMask)
//	:dNewtonAlloc()
	:dNewtonMaterial(collisionMask)
	,m_shape (NULL)
	,m_userData(NULL)
	,m_type (type)
{
}


dNewtonCollision::dNewtonCollision(const dNewtonCollision& srcCollision, NewtonCollision* const shape)
//	:dNewtonAlloc()
	:dNewtonMaterial(srcCollision)
	,m_shape (shape)
	,m_userData(srcCollision.m_userData)
	,m_type (srcCollision.m_type)
{
	SetShape(shape);
}


dNewtonCollision::~dNewtonCollision()
{
	if (m_shape) {
		NewtonCollisionSetUserData (m_shape, NULL);
		NewtonDestroyCollision(m_shape);
	}
}

dNewtonCollision::dCollsionType dNewtonCollision::GetType() const
{
    return m_type;
}

void* dNewtonCollision::GetUserData() const
{
	return m_userData;
}

void dNewtonCollision::SetUserData(void* const userData)
{
	m_userData = userData;
}

dFloat dNewtonCollision::GetVolume () const
{
	return NewtonConvexCollisionCalculateVolume (m_shape);
}

void dNewtonCollision::SetMatrix (const dFloat* const matrix)
{
	NewtonCollisionSetMatrix(m_shape, matrix);
}

void dNewtonCollision::GetMatrix (dFloat* const matrix) const
{
	NewtonCollisionGetMatrix(m_shape, matrix);
}


void dNewtonCollision::SetScale(dFloat x, dFloat y, dFloat z)
{
	NewtonCollisionSetScale(m_shape, x, y, z);
}
void dNewtonCollision::GetScale(dFloat& x, dFloat& y, dFloat& z) const
{
	NewtonCollisionGetScale(m_shape, &x, &y, &z);
}


NewtonCollision* dNewtonCollision::GetShape() const
{
	return m_shape;
}

void dNewtonCollision::SetShape (NewtonCollision* const shape)
{
	m_shape = shape;
	NewtonCollisionSetUserData (m_shape, this);
}

void dNewtonCollision::DebugRender (void* userData, int vertexCount, const dFloat* faceVertec, int id)
{
	dDebugRenderer* const renderer = (dDebugRenderer*) userData;
	renderer->OnDrawFace (vertexCount, faceVertec, id);
}


void dNewtonCollision::DebugRender (const dFloat* const matrix, dDebugRenderer* const renderer) const
{
	NewtonCollisionForEachPolygonDo (m_shape, matrix, DebugRender, renderer);
}


void dNewtonCollision::CalculateAABB (const dFloat* const matrix, dFloat* const p0, dFloat* const p1) const
{
	NewtonCollisionCalculateAABB (m_shape, matrix, p0, p1);
}

//void dNewtonCollision::CalculateBuoyancyAcceleration (const dFloat* const matrix, const dFloat* const shapeOrigin, const dFloat* const gravityVector, const dFloat* const fluidPlane, dFloat fluidDensity, dFloat fluidViscosity, dFloat* const accel, dFloat* const alpha)
dFloat dNewtonCollision::CalculateBuoyancyVolume (const dFloat* const matrix, const dFloat* const shapeOrigin, const dFloat* const gravityVector, const dFloat* const fluidPlane, dFloat* const alpha)
{
	dAssert (0);
//	NewtonConvexCollisionCalculateBuoyancyAcceleration(m_shape, matrix, shapeOrigin, gravityVector, fluidPlane, fluidDensity, accel, alpha);
	return 0.0f;
}

dNewtonCollisionMesh::dNewtonCollisionMesh(dNewton* const world, dLong collisionMask)
	:dNewtonCollision(m_mesh, collisionMask)
{
	SetShape (NewtonCreateTreeCollision(world->GetNewton(), 0));
}

dNewtonCollisionMesh::dNewtonCollisionMesh (dNewton* const world, const dNewtonMesh& mesh, dLong collisionMask)
	:dNewtonCollision(m_mesh, collisionMask)
{
	SetShape (NewtonCreateTreeCollisionFromMesh (world->GetNewton(), mesh.GetMesh(), 0));
}



dNewtonCollisionMesh::dNewtonCollisionMesh(const dNewtonCollisionMesh& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollision* dNewtonCollisionMesh::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionMesh(*this, shape);
}

void dNewtonCollisionMesh::BeginFace()
{
	NewtonTreeCollisionBeginBuild(m_shape);
}

void dNewtonCollisionMesh::AddFace(int vertexCount, const dFloat* const vertexPtr, int strideInBytes, int faceAttribute)
{
	NewtonTreeCollisionAddFace (m_shape, vertexCount, vertexPtr, strideInBytes, faceAttribute);
}

void dNewtonCollisionMesh::EndFace()
{
	//NewtonTreeCollisionEndBuild (m_shape, 1);
	NewtonTreeCollisionEndBuild (m_shape, 0);
}


dNewtonCollisionScene::dNewtonCollisionScene(dNewton* const world, dLong collisionMask)
	:dNewtonCollision(m_scene, collisionMask)
{
	SetShape (NewtonCreateSceneCollision(world->GetNewton(), 0));
}


dNewtonCollisionScene::dNewtonCollisionScene(const dNewtonCollisionScene& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollision* dNewtonCollisionScene::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionScene(*this, shape);
}

void dNewtonCollisionScene::BeginAddRemoveCollision()
{
	NewtonSceneCollisionBeginAddRemove (m_shape);	
}

void* dNewtonCollisionScene::AddCollision(const dNewtonCollision* const collision)
{
	return NewtonSceneCollisionAddSubCollision (m_shape, collision->GetShape());
}

void dNewtonCollisionScene::RemoveCollision (void* const handle)
{
	NewtonSceneCollisionRemoveSubCollision (m_shape, handle);
}

void dNewtonCollisionScene::EndAddRemoveCollision()
{
	NewtonSceneCollisionEndAddRemove(m_shape);	
}

void* dNewtonCollisionScene::GetFirstNode () const 
{
	return NewtonSceneCollisionGetFirstNode (m_shape);
}

void* dNewtonCollisionScene::GetNextNode (void* const collisionNode) const 
{
	return NewtonSceneCollisionGetNextNode (m_shape, collisionNode);
}

dNewtonCollision* dNewtonCollisionScene::GetChildFromNode(void* const collisionNode) const
{
	NewtonCollision* const collision = NewtonSceneCollisionGetCollisionFromNode (m_shape, collisionNode);
	return (dNewtonCollision*) NewtonCollisionGetUserData (collision);
}


dNewtonCollisionConvexHull::dNewtonCollisionConvexHull (dNewton* const world, const dNewtonMesh& mesh, dLong collisionMask)
	:dNewtonCollision(m_convex, collisionMask)
{
	SetShape (NewtonCreateConvexHullFromMesh (world->GetNewton(), mesh.GetMesh(), 0.001f, 0));
}

dNewtonCollisionConvexHull::dNewtonCollisionConvexHull(const dNewtonCollisionConvexHull& srcCollision, NewtonCollision* const shape) : dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionConvexHull::dNewtonCollisionConvexHull(dNewton* const world, int vertexCount, const dFloat* const vertexCloud, int strideInBytes, dFloat tolerance, dLong collisionMask) : dNewtonCollision(m_convex, collisionMask)
{
    SetShape(NewtonCreateConvexHull(world->GetNewton(), vertexCount, vertexCloud, strideInBytes, tolerance, 0, NULL));
}

dNewtonCollisionConvexHull::dNewtonCollisionConvexHull(NewtonCollision* const shape, dLong collisionMask) : dNewtonCollision(m_convex, collisionMask)
{
    SetShape(shape);
}

dNewtonCollision* dNewtonCollisionConvexHull::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionConvexHull(*this, shape);
}

dNewtonCollisionCompound::dNewtonCollisionCompound (dNewton* const world, const dNewtonMesh& mesh, dLong collisionMask)
	:dNewtonCollision(m_compound, collisionMask)
{
	SetShape (NewtonCreateCompoundCollisionFromMesh (world->GetNewton(), mesh.GetMesh(), 0.001f, 0, 0));
	
	for (void* node = GetFirstNode(); node; node = GetNextNode(node)) {
		NewtonCollision* const collision = NewtonCompoundCollisionGetCollisionFromNode (m_shape, node);
		dAssert (NewtonCollisionGetType (collision) == SERIALIZE_ID_CONVEXHULL);
		new dNewtonCollisionConvexHull (collision, collisionMask);
	}
}


dNewtonCollisionCompound::dNewtonCollisionCompound(const dNewtonCollisionCompound& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionCompound::dNewtonCollisionCompound(dNewton* const world, dLong collisionMask) : dNewtonCollision(m_compound, collisionMask)
{
    SetShape(NewtonCreateCompoundCollision(world->GetNewton(), 0));
}

dNewtonCollisionCompound::dNewtonCollisionCompound(NewtonCollision* const shape, dLong collisionMask) : dNewtonCollision(m_compound, collisionMask)
{
    SetShape(shape);
}

dNewtonCollision* dNewtonCollisionCompound::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionCompound(*this, shape);
}

void dNewtonCollisionCompound::BeginAddRemoveCollision()
{
	NewtonCompoundCollisionBeginAddRemove (m_shape);	
}

void* dNewtonCollisionCompound::AddCollision(const dNewtonCollision* const collision)
{
	return NewtonCompoundCollisionAddSubCollision(m_shape, collision->GetShape());
}

void dNewtonCollisionCompound::RemoveCollision (void* const handle)
{
	NewtonCompoundCollisionRemoveSubCollision (m_shape, handle);
}

void dNewtonCollisionCompound::EndAddRemoveCollision()
{
	NewtonCompoundCollisionEndAddRemove(m_shape);	
}

void* dNewtonCollisionCompound::GetFirstNode () const 
{
	return NewtonCompoundCollisionGetFirstNode (m_shape);
}

void* dNewtonCollisionCompound::GetNextNode (void* const collisionNode) const 
{
	return NewtonCompoundCollisionGetNextNode (m_shape, collisionNode);
}

dNewtonCollision* dNewtonCollisionCompound::GetChildFromNode(void* const collisionNode) const
{
	NewtonCollision* const collision = NewtonCompoundCollisionGetCollisionFromNode (m_shape, collisionNode);
	return (dNewtonCollision*) NewtonCollisionGetUserData (collision);
}

dNewtonCollisionChamferedCylinder::dNewtonCollisionChamferedCylinder(const dNewtonCollisionChamferedCylinder& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionChamferedCylinder::dNewtonCollisionChamferedCylinder(dNewton* const world, dFloat radio, dFloat height, dLong collisionMask) : dNewtonCollision(m_chamferedCylinder, collisionMask)
{
    SetShape(NewtonCreateChamferCylinder(world->GetNewton(), radio, height, 0, NULL));
}

dNewtonCollision* dNewtonCollisionChamferedCylinder::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionChamferedCylinder(*this, shape);
}

dNewtonCollisionCylinder::dNewtonCollisionCylinder(const dNewtonCollisionCylinder& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionCylinder::dNewtonCollisionCylinder(dNewton* const world, dFloat radio0, dFloat radio1, dFloat height, dLong collisionMask) : dNewtonCollision(m_cylinder, collisionMask)
{
    SetShape(NewtonCreateCylinder(world->GetNewton(), radio0, radio1, height, 0, NULL));
}

dNewtonCollisionCylinder::dNewtonCollisionCylinder(NewtonCollision* const shape, dLong collisionMask) : dNewtonCollision(m_cylinder, collisionMask)
{
    SetShape(shape);
}

dNewtonCollision* dNewtonCollisionCylinder::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionCylinder(*this, shape);
}

dNewtonCollisionCone::dNewtonCollisionCone(const dNewtonCollisionCone& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionCone::dNewtonCollisionCone(dNewton* const world, dFloat radio, dFloat height, dLong collisionMask) : dNewtonCollision(m_cone, collisionMask)
{
    SetShape(NewtonCreateCone(world->GetNewton(), radio, height, 0, NULL));
}

dNewtonCollision* dNewtonCollisionCone::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionCone(*this, shape);
}

dNewtonCollisionCapsule::dNewtonCollisionCapsule(const dNewtonCollisionCapsule& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionCapsule::dNewtonCollisionCapsule(dNewton* const world, dFloat radio0, dFloat radio1, dFloat height, dLong collisionMask) : dNewtonCollision(m_capsule, collisionMask)
{
    SetShape(NewtonCreateCapsule(world->GetNewton(), radio0, radio1, height, 0, NULL));
}

dNewtonCollisionCapsule::dNewtonCollisionCapsule(NewtonCollision* const shape, dLong collisionMask) : dNewtonCollision(m_capsule, collisionMask)
{
    SetShape(shape);
}

dNewtonCollision* dNewtonCollisionCapsule::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionCapsule(*this, shape);
}

dNewtonCollisionSphere::dNewtonCollisionSphere(const dNewtonCollisionSphere& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionSphere::dNewtonCollisionSphere(dNewton* const world, dFloat radio, dLong collisionMask) : dNewtonCollision(m_sphere, collisionMask)
{
    SetShape(NewtonCreateSphere(world->GetNewton(), radio, 0, NULL));
}

dNewtonCollision* dNewtonCollisionSphere::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionSphere(*this, shape);
}

dNewtonCollisionBox::dNewtonCollisionBox(const dNewtonCollisionBox& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionBox::dNewtonCollisionBox(dNewton* const world, dFloat x, dFloat y, dFloat z, dLong collisionMask) : dNewtonCollision(m_box, collisionMask)
{
    SetShape(NewtonCreateBox(world->GetNewton(), x, y, z, 0, NULL));
}

dNewtonCollision* dNewtonCollisionBox::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionBox(*this, shape);
}

dNewtonCollisionNull::dNewtonCollisionNull(const dNewtonCollisionNull& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionNull::dNewtonCollisionNull(dNewton* const world) : dNewtonCollision(m_null, 0)
{
    SetShape(NewtonCreateNull(world->GetNewton()));
}

dNewtonCollision* dNewtonCollisionNull::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionNull(*this, shape);
}

dNewtonCollisionHeightField::dNewtonCollisionHeightField(const dNewtonCollisionHeightField& srcCollision, NewtonCollision* const shape) :dNewtonCollision(srcCollision, shape)
{

}

dNewtonCollisionHeightField::dNewtonCollisionHeightField(dNewton* const world, int width, int height, int gridsDiagonals, int elevationdataType, dFloat vertcalScale, dFloat horizontalScale_x, dFloat horizontalScale_z, const void* const elevationMap, const char* const attributeMap, dLong collisionMask) : dNewtonCollision(m_heighfield, collisionMask)
{
    SetShape(NewtonCreateHeightFieldCollision(world->GetNewton(), width, height, gridsDiagonals, elevationdataType, elevationMap, attributeMap, vertcalScale, horizontalScale_x, horizontalScale_z, 0));
}

dNewtonCollision* dNewtonCollisionHeightField::Clone(NewtonCollision* const shape) const
{
    return new dNewtonCollisionHeightField(*this, shape);
}

dNewtonCollision::dDebugRenderer::dDebugRenderer(dNewtonCollision* const me) :m_collision(me)
{

}
