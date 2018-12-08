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
#include "dNewtonMaterial.h"

dNewtonBody* dNewtonContactMaterial::GetBody0 () const
{
	NewtonBody* const body = NewtonJointGetBody0 ((NewtonJoint*)m_materialHandle);
	return (dNewtonBody*) NewtonBodyGetUserData (body);
}

dNewtonBody* dNewtonContactMaterial::GetBody1 () const
{
	NewtonBody* const body = NewtonJointGetBody1 ((NewtonJoint*)m_materialHandle);
	return (dNewtonBody*) NewtonBodyGetUserData (body);
}


void* dNewtonContactMaterial::GetFirstContact() const
{
	return NewtonContactJointGetFirstContact ((NewtonJoint*)m_materialHandle);
}

void* dNewtonContactMaterial::GetNextContact(void* const contact) const
{
	return NewtonContactJointGetNextContact ((NewtonJoint*)m_materialHandle, contact);
}

void dNewtonContactMaterial::RemoveContact(void* const contact) const
{
	NewtonContactJointRemoveContact((NewtonJoint*)m_materialHandle, contact);
}

dNewtonCollision* dNewtonContactMaterial::GetShape0 (const void* const contact)
{
	NewtonBody* const body = NewtonJointGetBody0 ((NewtonJoint*)m_materialHandle);
	NewtonMaterial* const material = NewtonContactGetMaterial (contact);
	NewtonCollision* const collision = NewtonMaterialGetBodyCollidingShape (material, body);
	return (dNewtonCollision*)NewtonCollisionGetUserData(collision);
}

dNewtonCollision* dNewtonContactMaterial::GetShape1 (const void* const contact)
{
	NewtonBody* const body = NewtonJointGetBody1 ((NewtonJoint*)m_materialHandle);
	NewtonMaterial* const material = NewtonContactGetMaterial (contact);
	NewtonCollision* const collision = NewtonMaterialGetBodyCollidingShape (material, body);
	return (dNewtonCollision*)NewtonCollisionGetUserData(collision);
}

void dNewtonContactMaterial::SetContactRestitution (const void* const contact, dFloat restitution)
{
	NewtonMaterial* const material = NewtonContactGetMaterial (contact);
	NewtonMaterialSetContactElasticity(material, restitution);
}

void dNewtonContactMaterial::SetContactFrictionCoef (const void* const contact, dFloat staticFrictionCoef, dFloat kineticFrictionCoef, int index)
{
	NewtonMaterial* const material = NewtonContactGetMaterial (contact);
	NewtonMaterialSetContactFrictionCoef (material, staticFrictionCoef, kineticFrictionCoef, index);
}

void dNewtonContactMaterial::RotateTangentDirections (const void* const contact, const dFloat* const directionVector)
{
	NewtonMaterial* const material = NewtonContactGetMaterial (contact);
	NewtonMaterialContactRotateTangentDirections (material, directionVector);
}