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

#ifndef __dScriptClass_H_
#define __dScriptClass_H_

class dDAGClassNode;

class dScriptClass
{
	public:
	dScriptClass(void);
	virtual ~dScriptClass(void);

	void AddCode (dDAGClassNode* const classSymbols, dCIL& classCode);

	int m_codeSgementSize;
	dVirtualMachine::dOpCode* m_codeSegment;

	dTree<int, dString> m_symbolTable;
};


#endif