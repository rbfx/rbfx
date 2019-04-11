/////////////////////////////////////////////////////////////////////////////
// Name:        dVariable.h
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
#include "dVariable.h"
#include <tinyxml.h>


dVariable::dVariable ()
	:m_type(m_int)
	,m_name()
	,m_integer(0)
{
}

dVariable::dVariable (const dVariable& me)
	:m_type (me.m_type) 
	,m_name(me.m_name)
{
	if (m_type == m_string){
		SetValue (me.m_data);
	} else {
		m_integer = me.m_integer;
	}
}

dVariable::~dVariable ()
{
	if ((m_type == m_string) && m_data){
		delete[] m_data;
	}
}

void dVariable::SetName (const char* const name)
{
//	strcpy (m_name, name);
	m_name = name;
}

const char* dVariable::GetName() const
{
	return m_name.GetStr();
}

dVariable::dType dVariable::GetType() const
{
	return m_type;
}

void dVariable::SetValue (int value)
{
	if ((m_type == m_string) && m_data){
		delete[] m_data;
	}
	m_type = m_int;
	m_integer = value;
}

void dVariable::SetValue (float value)
{
	if ((m_type == m_string) && m_data){
		delete[] m_data;
	}

	m_type = m_float;
	m_real = value;
}

void dVariable::SetValue (const char* value)
{
	if ((m_type == m_string) && m_data){
		delete[] m_data;
	}

	m_type = m_string;
	m_data = new char[strlen(value) + 1];
	strcpy (m_data, value);
}

int dVariable::GetInt () const
{
	return (m_type == m_int) ? m_integer : 0;
}

dFloat dVariable::GetFloat () const
{
	return (m_type == m_float) ? m_real : 0.0f;
}

const char* dVariable::GetString () const
{
	return (m_type == m_string) ? m_data : NULL;
}


dVariableList::dVariableList()
	:dTree<dVariable, dCRCTYPE>()
{
}

dVariableList::dVariableList(const dVariableList& me)
	:dTree<dVariable, dCRCTYPE>()
{
	Iterator iter (me);
	for (iter.Begin(); iter; iter++) {
		dVariable& var = iter.GetNode()->GetInfo();
		Insert(var, iter.GetNode()->GetKey());
	}
}

dVariableList::~dVariableList ()
{
}

dVariable* dVariableList::FindVariable(dCRCTYPE crc) const
{
	dTreeNode* node = Find (crc);
	return node ? &node->GetInfo() : NULL;
}

dVariable* dVariableList::FindVariable(const char* const name) const
{
	return FindVariable(dCRC64 (name));
}

dVariable* dVariableList::CreateVariable (const char* const name)
{
	dCRCTYPE crc = dCRC64(name);
	dTreeNode* node = Find (crc);
	if (!node) {
		node = Insert(crc);
		dVariable& var = node->GetInfo();
		var.SetName (name);
	}
	return &node->GetInfo();
}


void dVariableList::Serialize(TiXmlElement* const rootNode)
{
	if (GetCount()) {
		TiXmlElement* const dataNode = new TiXmlElement ("variables");

		rootNode->LinkEndChild(dataNode);
		Iterator iter (*this);
		for (iter.Begin(); iter; iter++) {
			dVariable& var = iter.GetNode()->GetInfo();

			switch (var.m_type) 
			{
				case dVariable::m_int:
				{
					TiXmlElement* variable = new TiXmlElement ("integer");
					dataNode->LinkEndChild(variable);
					variable->SetAttribute("name", var.m_name.GetStr());
					variable->SetAttribute("value", var.m_integer);
					break;
				}

				case dVariable::m_float:
				{
					TiXmlElement* variable = new TiXmlElement ("float");
					dataNode->LinkEndChild(variable);
					variable->SetAttribute("name", var.m_name.GetStr());
					variable->SetDoubleAttribute("value", var.m_real);
					break;
				}

				case dVariable::m_string:
				{
					TiXmlElement* variable = new TiXmlElement ("string");
					dataNode->LinkEndChild(variable);
					variable->SetAttribute("name", var.m_name.GetStr());
					variable->SetAttribute("value", var.m_data);
					break;
				}
			}
		}
	}
}



bool dVariableList::Deserialize(const dScene* const scene, TiXmlElement* const node)
{
	TiXmlElement* const dataNode = (TiXmlElement*) node->FirstChild ("variables");
	if (dataNode) {
		for (TiXmlElement* element = (TiXmlElement*) dataNode->FirstChild(); element; element = (TiXmlElement*) element->NextSibling()) {
			const char* className = element->Value();
			dVariable* var = CreateVariable (element->Attribute ("name"));
			if (!strcmp (className, "integer")) {
				dAssert (0);
			} else if (!strcmp (className, "float")) {
				dAssert (0);
			} else if (!strcmp (className, "string")) {
				var->SetValue(element->Attribute ("value"));
			}
		}
	}
	return true;
}