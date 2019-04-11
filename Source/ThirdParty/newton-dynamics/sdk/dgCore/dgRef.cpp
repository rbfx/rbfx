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

#include "dgStdafx.h"
#include "dgRef.h"
#include "dgList.h"
#include "dgTree.h"


dgRtti dgRef::m_rtti ("dgRef");

dgRefFlags::dgRefFlags()
{
	*this = 0;
	m_alive = true;
	m_ref = 1;
}

dgInt32 dgRefFlags::operator = (dgInt32 val)
{
	dgInt32* ptr;
	ptr = &(*(dgInt32*)this);
	*ptr = val;
	return val;
}



dgRef::dgRef()
{
	m_id = 0;
}

dgRef::dgRef(const char *name)
{
	SetName(name);
}

dgRef::dgRef(dgUnsigned32 idArg)
{
	SetNameID(idArg);
}

dgRef::dgRef(const dgRef &Clone)
{
	m_id = Clone.m_id;
}

dgRef::~dgRef()
{
}

dgRef *dgRef::AddRef()
{
	m_ref++;
	dgAssert(m_ref < ((1 << 24) - 1));
	return this;
}

dgInt32 dgRef::Release()
{
	m_ref--;
	if (m_ref) {
		return dgInt32(m_ref);
	}
	delete this;
	return 0;
}

dgRef *dgRef::CreateClone() const
{
	dgAssert(0);
	return NULL;
}


dgUnsigned32 dgRef::GetTypeId() const
{
	return m_rtti.GetTypeId();
}

bool dgRef::IsType(dgUnsigned32 typeId) const
{
	return m_rtti.IsTypeID(typeId);
}

dgUnsigned32 dgRef::GetRttiType()
{
	return m_rtti.GetTypeId();
}


bool dgRef::GetUserFlag0() const
{
	return m_userFlag0 ? true : false;
}

bool dgRef::GetUserFlag1() const
{
	return m_userFlag1 ? true : false;
}


void dgRef::SetUserFlag0(bool flags)
{
	m_userFlag0 = dgUnsigned8(flags);
}

void dgRef::SetUserFlag1(bool flags)
{
	m_userFlag1 = dgUnsigned8(flags);
}


bool dgRef::IsAlive() const
{
	return m_alive ? true : false;
}

void dgRef::Kill()
{
	m_alive = false;
}

void dgRef::Unkill()
{
	m_alive = true;
}

void dgRef::SetNameID(dgUnsigned32 newID)
{
	m_id = newID;
}

dgUnsigned32 dgRef::GetNameID() const
{
	return m_id;
}


const char* dgRef::GetName() const
{
	return dgInverseCRC(GetNameID());
}

dgInt32 dgRef::GetRefCount() const
{
	return dgInt32(m_ref);
}


void dgRef::SetName(const char *name)
{
	SetNameID(0);
	if (name) {
		SetNameID(dgCRC(name));
	}
}

bool dgRef::IsTypeByName(const char *typeName) const
{
	return IsType(dgCRC(typeName, (dgInt32)strlen(typeName)));
}


void dgRef::AttachRef (
	dgRef **oldObj, 
	dgRef *newObj)
{
	if (*oldObj) {
	   (*oldObj)->Release();
	}
	*oldObj = newObj;
	if (newObj) {
		newObj->AddRef();
	}
}



