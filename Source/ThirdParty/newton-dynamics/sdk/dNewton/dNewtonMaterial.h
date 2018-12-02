/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_MATERIAL_H_
#define _D_NEWTON_MATERIAL_H_

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"

class dNewtonBody;
class dNewtonCollision;

class dNewtonMaterial: virtual public dNewtonAlloc
{
	public:
	dNewtonMaterial(dLong mask = -1)
		:dNewtonAlloc()
		,m_collisionMask (mask)
		,m_materailID(0)
	{
	}

	dNewtonMaterial(const dNewtonMaterial& copy)
		:dNewtonAlloc()
		,m_collisionMask (copy.m_collisionMask)
		,m_materailID(copy.m_materailID)
	{
	}

	virtual ~dNewtonMaterial() 
	{
	}

	void SetMaterialId(char id)
	{
		m_materailID = id;
	}

	int GetMaterialId() const 
	{
		return m_materailID;
	}

	void SetCollisionMask(dLong mask) 
	{
		m_collisionMask = mask;
	}

	dLong GetCollisionMask() const
	{
		return m_collisionMask;
	}
	
	protected:
	dLong m_collisionMask;
	char m_materailID;
};

class dNewtonContactMaterial
{
	public:
	dNewtonContactMaterial(void* const materialHandle)
		:m_materialHandle(materialHandle)
	{
	}

	CNEWTON_API dNewtonBody* GetBody0 () const;
	CNEWTON_API dNewtonBody* GetBody1 () const;

	CNEWTON_API void* GetFirstContact() const;
	CNEWTON_API void* GetNextContact(void* const contact) const;
	CNEWTON_API void RemoveContact(void* const contact) const; 

	CNEWTON_API dNewtonCollision* GetShape0 (const void* const contact);
	CNEWTON_API dNewtonCollision* GetShape1 (const void* const contact);

	CNEWTON_API void SetContactRestitution (const void* const contact, dFloat restitution);
	CNEWTON_API void SetContactFrictionCoef (const void* const contact, dFloat staticFrictionCoef, dFloat kineticFrictionCoef, int index);

	CNEWTON_API void RotateTangentDirections (const void* const contact, const dFloat* const directionVector);

	private:
	void* m_materialHandle; 
};

#endif
