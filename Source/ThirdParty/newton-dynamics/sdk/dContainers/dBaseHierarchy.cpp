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

#include "dContainersStdAfx.h"
#include "dBaseHierarchy.h"


dBaseHierarchy::dBaseHierarchy (const dBaseHierarchy &clone)
{
	Clear ();
	SetNameID (clone.m_name.GetStr());
	for (dBaseHierarchy* obj = clone.m_child; obj; obj = obj->m_sibling) {
		dBaseHierarchy* const newObj = obj->CreateClone ();
		newObj->Attach (this);
	}
}


dBaseHierarchy::~dBaseHierarchy () 
{
	Detach();
	while (m_child) {
		delete m_child;
	}
}



void dBaseHierarchy::Attach (dBaseHierarchy* const parentArg, bool addFirst)
{
	m_parent = parentArg;
	if (m_parent->m_child) {
		if (addFirst) {
			m_sibling = m_parent->m_child;
			m_parent->m_child = this;
		} else {
			dBaseHierarchy* obj = m_parent->m_child;
			for (; obj->m_sibling; obj = obj->m_sibling);
			obj->m_sibling = this;
		}
	} else {
		m_parent->m_child = this;
	}
}


void dBaseHierarchy::Detach ()
{
 	if (m_parent) {
		if (m_parent->m_child == this) {
			m_parent->m_child = m_sibling;
		} else {
			dBaseHierarchy* ptr = m_parent->m_child;
			for (; ptr->m_sibling != this; ptr = ptr->m_sibling);
			ptr->m_sibling = m_sibling;
		}
		m_parent = NULL;
		m_sibling = NULL;
	}
}
	

dBaseHierarchy* dBaseHierarchy::GetRoot() const
{
	const dBaseHierarchy* root = this;
	for (; root->m_parent; root = root->m_parent);
	return (dBaseHierarchy*)root;
}


dBaseHierarchy* dBaseHierarchy::GetFirst() const
{
	dBaseHierarchy* ptr = (dBaseHierarchy*) this;
	for (; ptr->m_child; ptr = ptr->m_child);
	return ptr;
}

dBaseHierarchy* dBaseHierarchy::GetNext() const
{
	if (m_sibling) {
		return m_sibling->GetFirst();
	}

	dBaseHierarchy* ptr = m_parent;
	dBaseHierarchy* x = (dBaseHierarchy *)this;
	for (; ptr && (x == ptr->m_sibling); ptr = ptr->m_parent) {
		x = ptr;
	}
	return ptr;
}



dBaseHierarchy* dBaseHierarchy::GetLast() const
{
	dBaseHierarchy* ptr = (dBaseHierarchy*) this;
	for (; ptr->m_sibling; ptr = ptr->m_sibling);
	return ptr;
}


dBaseHierarchy* dBaseHierarchy::GetPrev() const
{

	if (m_child) {
		return m_child->GetNext();
	}

	dBaseHierarchy* ptr = m_parent;
	dBaseHierarchy* x = (dBaseHierarchy *)this;
	for (; ptr && (x == ptr->m_child); ptr = ptr->m_child) {
		x = ptr;
	}
	return ptr;
}


dBaseHierarchy* dBaseHierarchy::Find (dCRCTYPE nameCRC) const 
{
	if (nameCRC == GetNameID()) {
		return (dBaseHierarchy*)this;
	} else {
		for (dBaseHierarchy* ptr = GetFirst(); ptr && (ptr != this); ptr = ptr->GetNext()) {
			if (nameCRC == ptr->GetNameID()) {
				return ptr;
			}
		}
	}

	return NULL;
}









