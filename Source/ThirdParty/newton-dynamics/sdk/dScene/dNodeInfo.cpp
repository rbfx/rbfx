/////////////////////////////////////////////////////////////////////////////
// Name:        dNodeInfo.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 08:02:08
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////


#include "dSceneStdafx.h"
#include "dNodeInfo.h"
#include <tinyxml.h>

dInitRtti(dNodeInfo);

dTree<const dNodeInfo*, dCRCTYPE>& dNodeInfo::GetSingletonDictionary()
{
	static dTree<const dNodeInfo*, dCRCTYPE> dictionary;
	return dictionary;
}

dNodeInfo::dRegisterSingleton::dRegisterSingleton (const char* const className, const dNodeInfo* const singleton)
{
	dCRCTYPE crc = dCRC64 (className);
	dTree<const dNodeInfo*, dCRCTYPE>& dictionary = dNodeInfo::GetSingletonDictionary();
	dictionary.Insert (singleton, crc);
}

dNodeInfo::dNodeInfo() 
	:dClassInfo()
	,dVariableList() 
	,m_name()
	,m_uniqueID(-1)
	,m_editorFlags(0)
{
}

dNodeInfo::dNodeInfo(const dNodeInfo& me)
	:dClassInfo()
	,dVariableList(me) 
	,m_name(me.m_name)
	,m_uniqueID(me.m_uniqueID)
	,m_editorFlags(me.m_editorFlags)
{
}

dNodeInfo::~dNodeInfo(void)
{
}

dNodeInfo* dNodeInfo::MakeCopy () const
{
	return new dNodeInfo();
}

dNodeInfo* dNodeInfo::MetaFunction(dScene* const world) const
{
	return MakeCopy();
}

void dNodeInfo::ReplaceSingletonClass (const char* const className, const dNodeInfo* const singleton)
{
	dCRCTYPE crc = dCRC64 (className);
	dTree<const dNodeInfo*, dCRCTYPE>& dictionary = dNodeInfo::GetSingletonDictionary();
	dTree<const dNodeInfo*, dCRCTYPE>::dTreeNode* const node = dictionary.Find(crc);
	if (node) {
		node->GetInfo() = singleton;
	} else {
		dRegisterSingleton (className, singleton);
	}
}

dNodeInfo* dNodeInfo::CreateFromClassName (const char* const className, dScene* const world)
{
	dCRCTYPE crc = dCRC64 (className);
	dTree<const dNodeInfo*, dCRCTYPE>& dictionary = dNodeInfo::GetSingletonDictionary();
	dTree<const dNodeInfo*, dCRCTYPE>::dTreeNode* const node = dictionary.Find(crc);
	if (node) {
		const dNodeInfo* const singleton = node->GetInfo();
		return singleton->MetaFunction(world);
	} else {
		dAssert (0);
		//m_uniqueIDCounter ++;
	}
	return NULL;
}

const char* dNodeInfo::GetName () const
{
	return m_name.GetStr();
}

void dNodeInfo::SetName (const char* const name)
{
	m_name = name;
}

const char* dNodeInfo::GetClassName () const
{
	return "dNodeInfo";
}

const char* dNodeInfo::GetBaseClassName ()	const
{
	return "";
}

/*
dVariableList& dNodeInfo::GetVariableList()
{
	return m_variables; 
}

dVariable* dNodeInfo::FindVariable(const char* const name) const
{
	return m_variables.FindVariable(name);
}

dVariable* dNodeInfo::CreateVariable (const char* const name)
{
	return m_variables.CreateVariable(name);
}
*/


unsigned dNodeInfo::GetEditorFlags() const
{
	return m_editorFlags;
}

void dNodeInfo::SetEditorFlags(unsigned flags)
{
	m_editorFlags = flags;
}

void dNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	rootNode->SetAttribute("name", m_name.GetStr());
	rootNode->SetAttribute("nodeID", m_uniqueID);
	rootNode->SetAttribute("editorFlags", m_editorFlags);
	dVariableList::Serialize(rootNode);
}

bool dNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	SetName (rootNode->Attribute("name"));
	rootNode->Attribute("nodeID", (int*)&m_uniqueID);
	rootNode->Attribute("editorFlags", (int*)&m_editorFlags);

	dVariableList::Deserialize(scene, rootNode);

	return true;
}