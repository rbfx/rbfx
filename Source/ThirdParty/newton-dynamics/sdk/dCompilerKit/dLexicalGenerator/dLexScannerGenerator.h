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

#ifndef __dLexScannerGenerator_h__
#define __dLexScannerGenerator_h__

#include "dLexCompiler.h"
#include "dFiniteAutomata.h"


class dLexScannerGenerator: public dDeterministicFiniteAutonata
{
	public:

	class dNextStateSymbolPair
	{
		public:
		char m_symbol;
		int m_nextState;
	};

	class dExpandedState;

	dLexScannerGenerator(const dNonDeterministicFiniteAutonata& nfa);
	virtual ~dLexScannerGenerator(void);
	
	void AddText (dString& output, const char* const fmt, ...) const;
	void ReplaceMacro (dString& data, const dString& newName, const char* const macro) const;
	void ReplaceAllMacros (dString& data, const dString& newName, const char* const macro) const;
	void LoadTemplateFile(const char* const templateName, dString& templateOuput) const;
	void SaveFile(const char* const fileName, const char* const extention, const dString& input) const;
	void CreateHeaderFile (const char* const fileName, const dString& className) const;
	void CreateCodeFile (const char* const fileName, const dString& className, const dString& userPreHeaderCode, const dString& userPostHeaderCode) const;

	dAutomataState* CreateState (int id);
	dAutomataState* CreateTargetState (dTree<dAutomataState*,dAutomataState*>& subSet, int id);

	static int CompareChar (const void* ch0, const void* ch1); 

	protected:
	int m_stateCount;
	const dNonDeterministicFiniteAutonata* m_nfa;

};


#endif