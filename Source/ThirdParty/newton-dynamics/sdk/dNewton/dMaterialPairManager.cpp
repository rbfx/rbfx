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

#include "dStdAfxNewton.h"
#include "dMaterialPairManager.h"


dMaterialPairManager::dMaterialPairManager ()
	:dNewtonAlloc()
	,m_default()
	,m_maxCount(8)
	,m_entryCount(0)
{
	memset (m_cachedKeys, -1, sizeof(m_cachedKeys));
	memset (m_cachedMaterial, 0, sizeof(m_cachedMaterial));
	m_keys = new unsigned[m_maxCount];
	m_pool = new dMaterialPair[m_maxCount];
}

dMaterialPairManager::~dMaterialPairManager ()
{
	delete[] m_keys;
	delete[] m_pool;
}


void dMaterialPairManager::AddPair (int materialId_0, int materialId_1, const dMaterialPair& pair)
{
	if (m_entryCount >= m_maxCount) {
		// the interaction pool is full we need to reallocate a larger Pool 
		unsigned* const newKeys = new unsigned[m_maxCount * 2];
		dMaterialPair* const newPool = new dMaterialPair[m_maxCount * 2];

		memcpy (newKeys, m_keys, m_entryCount * sizeof (unsigned));
		memcpy (newPool, m_pool, m_entryCount * sizeof (dMaterialPair));

		delete[] m_keys;
		delete[] m_pool;

		m_keys = newKeys;
		m_pool = newPool;
		m_maxCount *= 2;
	}

	unsigned key = MakeKey (materialId_0, materialId_1);
	int index = m_entryCount;
	for ( ; (index > 0) && m_keys[index-1] > key; index --) {
		m_keys[index] = m_keys[index - 1];
		m_pool[index] = m_pool[index - 1];
	}
	m_keys[index] = key;
	m_pool[index] = pair;
	m_entryCount ++;
}


const dMaterialPairManager::dMaterialPair* dMaterialPairManager::GetPair (int materialId_0, int materialId_1, int threadIndex) const
{
	unsigned key = MakeKey (materialId_0, materialId_1);

	if (m_cachedKeys[threadIndex] != key) {
		m_cachedKeys[threadIndex] = key;

		int index0 = 0;
		int index2 = m_entryCount - 1;
		unsigned key0 = m_keys[index0];
		unsigned key2 = m_keys[index2];
		while ((index2 - index0) > 4) {
			int index1 = (index0 + index2) >> 1;
			unsigned key1 = m_keys[index1];

			if (key < key1) {
				index2 = index1;
				key2 = key1;
			} else {
				index0 = index1;
				key0 = key1;
			}
		}

		index2 = dMin (m_entryCount, index2 + 4);
		for (int i = index0; (i <= index2) && (m_keys[i] <= key); i ++) {
			if (m_keys[i] == key) {
				m_cachedMaterial[threadIndex] = &m_pool[i];
				return m_cachedMaterial[threadIndex];
			}
		}
		m_cachedMaterial[threadIndex] = &m_default;
	}
	return m_cachedMaterial[threadIndex];
}

