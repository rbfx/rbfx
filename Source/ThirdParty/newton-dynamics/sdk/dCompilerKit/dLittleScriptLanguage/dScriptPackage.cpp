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

#include "dLSCstdafx.h"
#include "dScriptPackage.h"

dScriptPackage::dScriptPackage(void)
{
}

dScriptPackage::~dScriptPackage(void)
{
}

void dScriptPackage::Load (const char* const packageFileName)
{
	dAssert (0);
}

void dScriptPackage::Save (const char* const packageFileName)
{

}

/*
void dScriptPackage::AddClass (dDAGClassNode* const classSymbols, dCIL& classCode)
{
	dString name (classSymbols->m_name);
	if (m_classList.Find(name)) {
		m_classList.Remove(name);
	}

	dTree<dScriptClass, dString>::dTreeNode* const node = m_classList.Insert(name);
	node->GetInfo().AddCode(classSymbols, classCode);
}
*/