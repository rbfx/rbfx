/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __MATERIAL_MANAGER_H__
#define __MATERIAL_MANAGER_H__

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"



class dMaterialPairManager: public dNewtonAlloc
{
	public:
	class dMaterialPair
	{
		public:
		dMaterialPair()
		{
			m_restitution = dFloat(0.4f);
			m_staticFriction0 = dFloat(1.0f);
			m_staticFriction1 = dFloat(1.0f);
			m_kineticFriction0 = dFloat(0.7f);
			m_kineticFriction1 = dFloat(0.7f);
			memset (&m_userData, 0, sizeof (m_userData));
		}

		dLong  m_userData[8];
		dFloat m_restitution;
		dFloat m_staticFriction0;
		dFloat m_staticFriction1;
		dFloat m_kineticFriction0;
		dFloat m_kineticFriction1;

		private:
		friend class dMaterialPairManager;
	};

	CNEWTON_API dMaterialPairManager ();
	CNEWTON_API ~dMaterialPairManager ();
	
	dMaterialPair* GetDefaultPair ()
	{
		return &m_default;
	}
	
	CNEWTON_API void AddPair (int materialId_0, int materialId_1, const dMaterialPair& pair);
	CNEWTON_API const dMaterialPair* GetPair (int materialId_0, int materialId_1, int threadIndex = 0) const;

	private:
	unsigned MakeKey (int id0, int id1) const
	{
		id0 &= 0xff;
		id1 &= 0xff;
		return (id0 >= id1) ? (id1 << 8) + id0 : (id0 << 8) + id1;
	}
	

	mutable dMaterialPair m_default;
	mutable unsigned m_cachedKeys[16];
	mutable dMaterialPair* m_cachedMaterial[16];
	int m_maxCount;
	int m_entryCount;
	unsigned* m_keys;
	dMaterialPair* m_pool;
};
#endif
