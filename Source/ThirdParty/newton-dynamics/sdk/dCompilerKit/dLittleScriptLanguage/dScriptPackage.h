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


#ifndef __dScriptPackage_H_
#define __dScriptPackage_H_

#include "dScriptClass.h"

class dDAGClassNode;

class dScriptPackage
{
	public:
	dScriptPackage();
	virtual ~dScriptPackage(void);

	void Load (const char* const packageFileName);
	void Save (const char* const packageFileName);

//	void AddClass (dDAGClassNode* const classSymbols, dCIL& classCode);

	dTree<dScriptClass, dString> m_classList;
};


#endif