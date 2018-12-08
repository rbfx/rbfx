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

#include "dFiniteAutomata.h"
#include "dChatertSetMap.h"


dChatertSetMap::ChatertSet::ChatertSet (const char* const set, int count, int id)
	:m_count (count)
	,m_id (id)
{
	dAssert (sizeof (m_characters) > count * sizeof (char));
	memcpy (m_characters, set, count * sizeof (char));
	m_characters[count] = 0;
	qsort (m_characters, count, sizeof (char), sort);

	int m = 0;
	for (int i = 0; i < m_count; ) {
		char ch = m_characters[i];
		for (i ++; (i < m_count) && (m_characters[i] == ch); i ++); 
		m_characters[m] = ch;
		m ++;
	}
	m_count = m;
	m_characters[m_count] = 0;
}

bool dChatertSetMap::ChatertSet::IsCharAMatch (int id) const
{
	int i0 = 0;
	int i1 = m_count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;
		if (id <= m_characters[i]) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		if (id == m_characters[i]) {
			return true;
		}
	}
	return false;
}

int dChatertSetMap::ChatertSet::GetId() const
{
	return m_id;
}

int dChatertSetMap::ChatertSet::GetLength () const 
{
	return m_count;
}

const char* dChatertSetMap::ChatertSet::GetSet () const 
{
	return m_characters;
}

int dChatertSetMap::ChatertSet::sort (const void* a, const void* b)
{
	int ch0 = *((char*)a);
	int ch1 = *((char*)b);
	if (ch0 < ch1) {
		return -1;
	} else if (ch0 > ch1) {
		return 1;
	} else {
		return 0;
	}
}


dChatertSetMap::dChatertSetMap ()
	:m_id (0)
{
}

dChatertSetMap::~dChatertSetMap()
{
}

const dTree<dList <dChatertSetMap::ChatertSet>::dListNode*, int>& dChatertSetMap::GetSets() const
{
	return m_table;
}

const dChatertSetMap::ChatertSet* dChatertSetMap::FindSet (int id) const
{
	dTree<dList <ChatertSet>::dListNode*, int>::dTreeNode* node = m_table.Find(id);
	return node ? &node->GetInfo()->GetInfo() : NULL; 
}

int dChatertSetMap::AddSet (const char* const set, int count)
{
	ChatertSet newSet (set, count, m_id);
	dCRCTYPE crc = dCRC64 (newSet.m_characters, 0);

	dTree<dList <ChatertSet>::dListNode*, dCRCTYPE>::dTreeNode* node = m_crcID.Find(crc);
	if (!node) {
		dList <ChatertSet>::dListNode* const setNode = m_sets.Append(newSet);
		m_table.Insert(setNode, newSet.m_id);
		m_id ++;
		node = m_crcID.Insert(setNode, crc);
	}

	dList <ChatertSet>::dListNode* const setNode = node->GetInfo();
	return setNode->GetInfo().m_id;
}


