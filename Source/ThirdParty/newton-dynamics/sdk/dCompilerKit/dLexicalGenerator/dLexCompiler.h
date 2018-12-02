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

#ifndef __dLexCompiler_h__
#define __dLexCompiler_h__


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <tchar.h>
#include <crtdbg.h>
#include <direct.h>

#include <dCRC.h>
#include <dTree.h>
#include <dList.h>
#include <dString.h>
#include <dMathDefines.h>
#include <dContainersStdAfx.h>


#include "dAutomataState.h"
#include "dChatertSetMap.h"
#include "dDeterministicFiniteAutonata.h"
#include "dNonDeterministicFiniteAutonata.h"


class dLexCompiler
{
	enum dToken;
	class dTokenData;
	class dExpandedNFA;
	class dExpandedState;


	class dTokenDataList: public dList<dTokenData*>
	{
		public:
		dTokenDataList ();
		~dTokenDataList ();
		void DeleteAll();
		void AddTokenData (dToken token, const char* const regulatExpression);
	};


	class dDefinitionsMap: public dTree<dString, dCRCTYPE>
	{
		public:
		dDefinitionsMap ();
		~dDefinitionsMap ();
		void PreProcessDefinitions (dString& regularExpressionWithDefinitions);
		void AddDefinition (dString& regularExpressionWithMacros, dString& key);
	};

	class dTransitionCountStart
	{
		public:
		int m_count;
		int m_start;
	};

	struct dTransitionType
	{
		public:
		dTransitionType ()
			:m_value(0)
		{
		}

		dTransitionType (unsigned val)
			:m_value(val)
		{
		}
		union {
			unsigned m_value;
			struct {
				unsigned  m_nextState	:14;
				unsigned  m_infoType	: 2;		// 0 m_value is a character, 1 m_value is a charcterSet, 
				unsigned  m_info		:16;
			};
		};
	};


	public:
	//dLexCompiler(const char* const inputRules, const char* const outputFileName);
	dLexCompiler(const dString& inputRules, const char* const outputFileName);
	~dLexCompiler();

	private:
	void NextToken ();
	void MatchToken (dToken token);
	void ParseDefinitionExpression (dString& preheaderCode);
	void ParseDefinitionBlock (dString& preheaderCode);
	void ParseDefinitions (dExpandedNFA &nfa, dString& userPreHeaderCode, dString& postHeaderCode);
	dString GetClassName(const char* const fileName) const;

	dToken m_token;
	int m_lineNumber;
	int m_grammarTokenStart;
	int m_grammarTokenLength;
	const char* m_grammar;
	dTokenDataList m_tokenList;
	dDefinitionsMap m_defintions;
};


#endif

// TODO: reference additional headers your program requires here
