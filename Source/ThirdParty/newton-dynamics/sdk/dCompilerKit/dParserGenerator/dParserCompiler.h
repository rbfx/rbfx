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

#ifndef __dParserCompiler_h__
#define __dParserCompiler_h__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <tchar.h>
#include <crtdbg.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <dTree.h>
#include <dList.h>
#include <dCRC.h>
#include <dString.h>
#include <dContainersStdAfx.h>


//#define D_WRITE_STATE_TRANSITION_GRAPH

class dParserLexical;

class dParserCompiler
{
	public:
	enum dToken
	{
		OR = 256,
		COLOM,
		SIMICOLOM,
		PREC,
		TOKEN,
		UNION,
		LEFT,
		RIGHT,
		START,
		LITERAL,
		EXPECT,
		INTEGER,
		CODE_BLOCK,
		SEMANTIC_ACTION,
		GRAMMAR_SEGMENT,
		PARCEL_ERROR,
	};

	enum dTokenType;
	enum ActionType;
	
	class dItem;
	class dState;
	class dGoto;
	class dAction;
	class dSymbol;
	class dRuleInfo;
	class dTokenInfo;
	class dGotoEntry;
	class ActionEntry;
	class dTransition;
//	class dDictionary;
	class dSymbolName;
	class dActionEntry;
	class dSentenceSymbol;
	class dProductionRule;
	class dTokenStringPair;
	class dOperatorsPrecedence;
	class dOperatorsAssociation;

	dParserCompiler(const dString& inputRules, const char* const outputFileName, const char* const scannerClassName);
	~dParserCompiler();

	protected:
	dString GetClassName(const char* const fileName) const;
	void LoadTemplateFile(const char* const fileName, dString& output) const;
	void SaveFile(const char* const fileName, const char* const extention, const dString& input) const;

	void ReplaceMacro (dString& data, const dString& newName, const dString& macro) const;
	void ReplaceAllMacros (dString& data, const dString& newName, const dString& macro) const;

	void ScanGrammarFile(const dString& inputRules, dProductionRule& rules, dTree<dTokenInfo, dSymbolName>& symbolList, dOperatorsPrecedence& operatorPrecence,
						 dString& userCodeBlock, dString& userVariableClass, dString& endUserCode, int& lastTokenEnum);
	dToken ScanGrammarRule(dParserLexical& lexical, dProductionRule& rules, dTree<dTokenInfo, dSymbolName>& symbolList, int& ruleNumber, int& tokenEnumeration, const dOperatorsPrecedence& operatorPrecence);
	bool DoesSymbolDeriveEmpty (dSymbolName symbol, const dTree<dList<void*>, dSymbolName>& ruleMap) const ;
	void First (dSymbolName symbol, dTree<int, dSymbolName>& symbolListMark, const dTree<dTokenInfo, dSymbolName>& symbolList, const dTree<dList<void*>, dSymbolName>& ruleMap, dTree<int, dSymbolName>& firstSetOut) const;
	void First (const dList<dSymbolName>& symbolSet, const dTree<dTokenInfo, dSymbolName>& symbolList, const dTree<dList<void*>, dSymbolName>& ruleMap, dTree<int, dSymbolName>& firstSetOut) const;
	dState* Goto (const dState* const state, const dSymbolName& symbol, const dTree<dTokenInfo, dSymbolName>& symbolList, const dTree<dList<void*>, dSymbolName>& ruleMap) const;
	dState* Closure (const dList<dItem>& itemSet, const dTree<dTokenInfo, dSymbolName>& symbolList, const dTree<dList<void*>, dSymbolName>& ruleMap) const;
	void BuildParsingTable (const dTree<dState*, dSymbolName>& stateList, const dSymbolName& startSymbol, const dOperatorsPrecedence& operatorPrecence) const;
	void CanonicalItemSets (dTree<dState*, dSymbolName>& states, const dProductionRule& rules, const dTree<dTokenInfo, dSymbolName>& symbolList, const dOperatorsPrecedence& operatorPrecence, const dString& fileName);

	void GenerateHeaderFile (const dString& className, const dString& scannerClassName, const char* const outputFileName, 
							 const dTree<dTokenInfo, dSymbolName>& symbolList, const dString& userVariableClass);
	void GenerateParserCode (const dString& className, const dString& scannerClassName, const char* const outputFileName, 
							 const dTree<dTokenInfo, dSymbolName>& symbolList, dTree<dState*, dSymbolName>& stateList, const dString& userCode, const dString& endUserCode, int lastTerminalTokenEnum);

	void DisplayError (const char* format, ...) const;

	mutable int m_shiftReduceExpectedWarnings;
};


#endif

// TODO: reference additional headers your program requires here
