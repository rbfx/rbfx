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

// dParserCompiler.cpp : Defines the entry point for the console application.
//

#include "dParserCompiler.h"
#include "dParserLexical.h"

#define DACCEPT_SYMBOL "$$$"
#define DACCEPTING_TOKEN 254	

#define DDEBUG_STATES

#define DERROR_SYMBOL	"error"
#define DERROR_TOKEN	255



//The Parcel input file consists of three sections, separated by a line containing only `%%'. 
//
//	definitions
//	%%
//	production rules
//	%%
//	user code

enum dParserCompiler::dTokenType
{
	TERMINAL,
	NONTERMINAL
};


enum dParserCompiler::ActionType
{
	dSHIFT = 0,
	dREDUCE,
	dACCEPT,
	dERROR
};

class dParserCompiler::dSymbolName
{
	class dDictionary : public dTree<int, dString>
	{
		public:
		dDictionary()
			:dTree<int, dString>()
		{
			m_empty = FindWord(dString(""));
		}

		dTreeNode* FindWord(const dString& word)
		{
			dDictionary::dTreeNode* node = Find(word);
			if (!node) {
				node = Insert(0, word);
			}
			return node;
		}
		dTreeNode* m_empty;
	};

	public:
	dSymbolName()
		:m_name(GetDictionary().m_empty)
	{
	}

	dSymbolName(const char* const word)
		:m_name(GetDictionary().FindWord(dString (word)))
	{
	}

	dSymbolName(const dString& word)
		:m_name(GetDictionary().FindWord(word))
	{
	}

	const dString& GetString() const
	{
		dAssert(m_name);
		return m_name->GetKey();
	}

	const char* GetStr() const
	{
		return GetString().GetStr();
	}

	bool operator== (const dSymbolName& src) const
	{
		return m_name == src.m_name;
	}

	bool operator!= (const dSymbolName& src) const
	{
		return m_name != src.m_name;
	}

	bool operator< (const dSymbolName& src) const
	{
		return m_name < src.m_name;
	}

	bool operator> (const dSymbolName& src) const
	{
		return m_name > src.m_name;
	}

	private:
	dDictionary& GetDictionary()
	{
		static dDictionary dictionary;
		return dictionary;
	}

	const dDictionary::dTreeNode* m_name;
};


class dParserCompiler::dTokenInfo
{
	public:
	dTokenInfo (int tokenId, dTokenType type, const dSymbolName& name)
		:m_tokenId (tokenId), m_type(type), m_name(name)
	{
	}

	int m_tokenId;
	dTokenType m_type;
	dSymbolName m_name;
};

class dParserCompiler::dGotoEntry
{
	public:
	dGotoEntry ()
	{
	}
	short  m_token;
	short  m_nextState;
};


class dParserCompiler::dActionEntry
{
	public:
	dActionEntry ()
	{
	}

	short m_token;
	char m_errorRule;
	char m_stateType;// 0 = shift, 1 = reduce, 2 = accept
	short m_nextState;
	short m_ruleSymbols;
	short m_ruleIndex;
};


class dParserCompiler::dSymbol
{
	public:
	dTokenType m_type;
	dToken m_token;
	dSymbolName m_name;
	dString m_operatorPrecendeceOverrride;
};


class dParserCompiler::dRuleInfo: public dParserCompiler::dSymbol, public dList<dParserCompiler::dSymbol>
{
	public:
	dRuleInfo()
		:dSymbol()
		,dList<dSymbol>()
		,m_ruleId()
		,m_ruleNumber(0)
		,m_ruleReduced(false)
		,m_shiftReduceErrorMark(false)
		,m_semanticActionCode("")
	{
	}

	dListNode* GetSymbolNodeByIndex (int index) const
	{
		int i = 0;
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			if (index == i) {
				return node;
			}
			i ++;
		}
		return NULL;
	}

	dSymbolName m_ruleId;
	int m_ruleNumber;
	bool m_ruleReduced;
	bool m_shiftReduceErrorMark;
	dString m_semanticActionCode;
};


class dParserCompiler::dProductionRule: public dList<dParserCompiler::dRuleInfo>
{
	public:

	dListNode* Find (dSymbolName name) const
	{
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			if (node->GetInfo().m_name == name) {
				return node;
			}
		}

		dAssert (0);
		return NULL;
	}
};

class dParserCompiler::dTransition
{
	public:
	dSymbolName m_name;
	dSymbolName m_symbol;
	dTokenType m_type;
	dState* m_targetState;
};

class dParserCompiler::dItem
{
	public:
	dItem ()
		:m_indexMarker(0)
		,m_lookAheadSymbol()
		,m_lastOperatorSymbol ()
		,m_ruleNode(NULL)
	{
	}

	int m_indexMarker;
	dSymbolName m_lookAheadSymbol;
	dSymbolName m_lastOperatorSymbol;
	dProductionRule::dListNode* m_ruleNode;
};

class dParserCompiler::dAction 
{
	public:
	ActionType m_type;
	int m_nextState;
	dItem* m_myItem;
	dProductionRule::dListNode* m_reduceRuleNode;
};

class dParserCompiler::dTokenStringPair 
{
	public:
	dToken m_token;
	dString m_info;
};


class dParserCompiler::dState: public dList<dParserCompiler::dItem>
{
	public:
	class dItemKey
	{
		public:
		dItemKey (const dItem& item)
			:m_lookAheadSymbol (item.m_lookAheadSymbol), m_rule (item.m_ruleNode)
		{
		}

		dItemKey (dSymbolName symbol, dProductionRule::dListNode* const rule)
			:m_lookAheadSymbol (symbol), m_rule (rule)
		{
		}

		bool operator< (const dItemKey& key) const
		{	
			if (m_lookAheadSymbol < key.m_lookAheadSymbol) {
				return true;	
			} else if (m_lookAheadSymbol == key.m_lookAheadSymbol) {
                if (m_rule->GetInfo().m_ruleNumber < key.m_rule->GetInfo().m_ruleNumber) {
					return true;	
				} 
			}
			return false;
		}

		bool operator> (const dItemKey& key) const
		{
			if (m_lookAheadSymbol > key.m_lookAheadSymbol) {
				return true;	
			} else if (m_lookAheadSymbol == key.m_lookAheadSymbol){
                if (m_rule->GetInfo().m_ruleNumber > key.m_rule->GetInfo().m_ruleNumber) {
					return true;	
				} 
			}
			return false;
		}

		dSymbolName m_lookAheadSymbol;
		dProductionRule::dListNode* m_rule;
	};

	dState (const dList<dItem>& itemSet)
		:m_key(), m_number(0), m_hasErroItem(false), m_goto(), m_actions(), m_transitions()
	{
		for (dListNode* node = itemSet.GetFirst(); node; node = node->GetNext()) {
			AddItem (node->GetInfo());
		}
	}

	void AddItem (const dItem& item)
	{
		dState::dListNode* const node = Append(item);

		static dSymbolName errorName(DERROR_SYMBOL);
		dItemKey key (item);
		dTree<dList<dState::dListNode*>, dItemKey>::dTreeNode* mapNode = m_itemMap.Find (key);
		if (!mapNode) {
			mapNode = m_itemMap.Insert (key);
		}

		if (item.m_indexMarker == 0) {
			dAssert (item.m_ruleNode);
			const dRuleInfo& ruleInfo = item.m_ruleNode->GetInfo();
			if (ruleInfo.GetCount()) {
				const dSymbol& symbol = ruleInfo.GetFirst()->GetInfo();
				if (symbol.m_name == errorName) {
					m_hasErroItem = true;
				}
			}
		}

		dList<dState::dListNode*>& bucket = mapNode->GetInfo();
		// check if bucket is not too big
		dAssert (bucket.GetCount() < 16);
		bucket.Append(node);
	}

	dSymbolName GetKey() const
	{
		return m_key;
	}

	void CalculateKey ()
	{
		int keylength = 0;
		char key[1024 * 64];
		key[0] = 0;
		for (dState::dListNode* itemNode = GetFirst(); itemNode; itemNode = itemNode->GetNext()) {
			dItem& item = itemNode->GetInfo();
			int index = 0;
			dAssert((keylength + item.m_lookAheadSymbol.GetString().Size()) < sizeof(key));
			strcpy(&key[keylength], item.m_lookAheadSymbol.GetString().GetStr());
			keylength += item.m_lookAheadSymbol.GetString().Size();

			for (dRuleInfo::dListNode* node = item.m_ruleNode->GetInfo().GetFirst(); node; node = node->GetNext()) {
				if (index == item.m_indexMarker) {
					dAssert((keylength + 1) < sizeof(key));
					key[keylength] = '.';
					keylength++;
					key[keylength] = 0;
				}
				const dSymbol& info = node->GetInfo();
				//key = dCRC64 (info.m_name.GetStr(), key);
				dAssert((keylength + info.m_name.GetString().Size()) < sizeof(key));
				strcpy(&key[keylength], info.m_name.GetString().GetStr());
				keylength += info.m_name.GetString().Size();
				index ++;
			}
			if (item.m_indexMarker == item.m_ruleNode->GetInfo().GetCount()) {
				//key = dCRC64 (".", key);
				dAssert((keylength + 1) < sizeof(key));
				key[keylength] = '.';
				keylength++;
				key[keylength] = 0;
			}
		}
		m_key = key;
	}

	dListNode* FindItem (dProductionRule::dListNode* const rule, int marker, dSymbolName lookAheadSymbol) const
	{
		dItemKey key(lookAheadSymbol, rule);
		dTree<dList<dState::dListNode*>, dItemKey>::dTreeNode* const mapNode = m_itemMap.Find(key);
		if (mapNode) {
			dList<dState::dListNode*>& bucket = mapNode->GetInfo();
			for (dList<dState::dListNode*>::dListNode* node = bucket.GetFirst(); node; node = node->GetNext()) {
				dState::dListNode* const nodeItem = node->GetInfo();
				const dItem& item = nodeItem->GetInfo();
				if (item.m_indexMarker == marker) {
					dAssert(item.m_ruleNode == rule);
					dAssert(item.m_lookAheadSymbol == lookAheadSymbol);
					return nodeItem;
				}
			}
		}
		return NULL;
	}

	void Trace(FILE* const debugFile) const
	{
		if (debugFile) {
			fprintf(debugFile, "state %d:\n", m_number);
			for (dState::dListNode* itemNode = GetFirst(); itemNode; itemNode = itemNode->GetNext()) {
				dItem& item = itemNode->GetInfo();
				fprintf(debugFile, "%s -> ", item.m_ruleNode->GetInfo().m_name.GetStr());

				int index = 0;
				bool hasIndex = false;
				dRuleInfo::dListNode* node = item.m_ruleNode->GetInfo().GetFirst();
				for (; node; node = node->GetNext()) {

					if (index == item.m_indexMarker) {
						fprintf(debugFile, ". ");
						hasIndex = true;
						break;
					}
					const dSymbol& info = node->GetInfo();
					fprintf(debugFile, "%s ", info.m_name.GetStr());
					index++;
				}

				for (; node; node = node->GetNext()) {
					const dSymbol& info = node->GetInfo();
					fprintf(debugFile, "%s ", info.m_name.GetStr());
				}
				if (!hasIndex) {
					fprintf(debugFile, ". ");
				}
				fprintf(debugFile, ":: %s\n", item.m_lookAheadSymbol.GetStr());
			}
			fprintf(debugFile, "\n");
		}
	}

	dSymbolName m_key;
	int m_number;
	bool m_hasErroItem;
	dTree<dState*, dSymbolName> m_goto;
	dTree<dAction, dSymbolName> m_actions;
	dList<dTransition> m_transitions;
	dTree<dList<dState::dListNode*>, dItemKey> m_itemMap;
};

class dParserCompiler::dOperatorsAssociation: public dList <dSymbolName>
{
	public:
	enum dAssoctivity
	{
		m_left,
		m_right
	};

	bool FindOperator(dSymbolName symbol) const
	{
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			if (symbol == node->GetInfo()) {
				return true;
			}
		}
		return false;
	}

	int m_prioprity;
	dAssoctivity m_associativity;
};

class dParserCompiler::dOperatorsPrecedence: public dList <dOperatorsAssociation>
{
	public:
	const dOperatorsAssociation* FindAssociation (dSymbolName symbol) const
	{
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			const dOperatorsAssociation& association = node->GetInfo();
			if (association.FindOperator(symbol)) {
				return &node->GetInfo();
			}
		}

		return NULL;
	}

	void SaveLastOperationSymbol (const dState* const state) const
	{
		for (dState::dListNode* node = state->GetFirst(); node; node = node->GetNext()) {
			dItem& item = node->GetInfo();
			const dRuleInfo& ruleInfo = item.m_ruleNode->GetInfo();
			for (dRuleInfo::dListNode* infoSymbolNode = ruleInfo.GetFirst(); infoSymbolNode; infoSymbolNode = infoSymbolNode->GetNext()) {
				const dSymbol& infoSymbol = infoSymbolNode->GetInfo();
				if (infoSymbol.m_operatorPrecendeceOverrride.Size()) {
					if (FindAssociation(infoSymbol.m_operatorPrecendeceOverrride)) {
						item.m_lastOperatorSymbol = infoSymbol.m_operatorPrecendeceOverrride;
					}

				} else {
					if (FindAssociation(infoSymbol.m_name)) {
						item.m_lastOperatorSymbol = infoSymbol.m_name;
					}
				}
			}
		}
	}
};

dParserCompiler::dParserCompiler(const dString& inputRules, const char* const outputFileName, const char* const scannerClassName)
	:m_shiftReduceExpectedWarnings(0)
{
	// scan the grammar into a list of rules.
	int lastTerminalToken;
	dProductionRule ruleList;
	dOperatorsPrecedence operatorPrecedence;
	dTree<dTokenInfo, dSymbolName> symbolList;
	dString userCodeBlock;
	dString userVariableClass ("");
	dString endUserCode ("\n");

	// scan grammar to a set of LR(1) rules
	symbolList.Insert(dTokenInfo (DACCEPTING_TOKEN, TERMINAL, DACCEPT_SYMBOL), DACCEPT_SYMBOL);
	symbolList.Insert(dTokenInfo(DERROR_TOKEN, TERMINAL, DERROR_SYMBOL), DERROR_SYMBOL);
	ScanGrammarFile(inputRules, ruleList, symbolList, operatorPrecedence, userCodeBlock, userVariableClass, endUserCode, lastTerminalToken);

	// convert the rules into a NFA.
	dTree<dState*, dSymbolName> stateList;
	CanonicalItemSets (stateList, ruleList, symbolList, operatorPrecedence, outputFileName);
//	fclose (debugFile);

	// create a LR(1) parsing table from the NFA graphs
	const dSymbolName& startSymbol = ruleList.GetFirst()->GetInfo().m_name;
	BuildParsingTable (stateList, startSymbol, operatorPrecedence);

	//Write Parser class and header file
	dString className (GetClassName(outputFileName));
	GenerateHeaderFile (className, scannerClassName, outputFileName, symbolList, userVariableClass);
	GenerateParserCode (className, scannerClassName, outputFileName, symbolList, stateList, userCodeBlock, endUserCode, lastTerminalToken);

	dTree<dState*, dSymbolName>::Iterator iter(stateList);
	for (iter.Begin(); iter; iter ++) {
		dState* const state = iter.GetNode()->GetInfo();
		delete state;
	}
}


dParserCompiler::~dParserCompiler()
{
}


void dParserCompiler::DisplayError (const char* format, ...) const
{
	va_list v_args;
	//char* const text = (char*) malloc (strlen (format) + 2048);
	char* const text = (char*) alloca (strlen (format) + 2048);
	dAssert (text);

	text[0] = 0;
	va_start (v_args, format);     
	vsprintf(text, format, v_args);
	va_end (v_args);            

	fprintf (stderr, text);
#ifdef _MSC_VER  
	OutputDebugStringA (text);
#endif	

	//free (text);
}


dString dParserCompiler::GetClassName(const char* const fileName) const
{
	char className[256];
	const char* ptr = strrchr (fileName, '/');
	if (ptr) {
		ptr ++;
	} else {
		ptr = strrchr (fileName, '\\');
		if (ptr) {
			ptr ++;
		} else {
			ptr = fileName;
		}
	}
	strcpy (className, ptr);
	strtok (className, ".");
	return dString (className);
}




void dParserCompiler::LoadTemplateFile(const char* const templateName, dString& templateOuput) const
{
	char path[2048];

	// in windows
	if (GetModuleFileName(NULL, path, sizeof(path))) { 

		char* const ptr = strrchr (path, '\\') + 1;
		sprintf (ptr, templateName);

		FILE* const templateFile = fopen (path, "rb");
		dAssert (templateFile);

		templateOuput.LoadFile(templateFile);
		fclose (templateFile);	
	} else {
		dAssert (0);
	}
}

void dParserCompiler::SaveFile(const char* const fileName, const char* const extention, const dString& input) const
{
	char path[2048];

	strcpy (path, fileName);
	char* const ptr1 = strrchr (path, '.');
	if (ptr1) {
		*ptr1 = 0;
	}
	strcat (path, extention);
	FILE* const headerFile = fopen (path, "wb");
	dAssert (headerFile);
	fprintf (headerFile, "%s", input.GetStr());
	fclose (headerFile);
}

void dParserCompiler::ScanGrammarFile(
	const dString& inputRules, 
	dProductionRule& ruleList, 
	dTree<dTokenInfo, dSymbolName>& symbolList,
	dOperatorsPrecedence& operatorPrecedence,
	dString& userCodeBlock,
	dString& userVariableClass,
	dString& endUserCode,
	int& lastTokenEnum)
{
	dSymbolName startSymbol;
	int tokenEnumeration = 256;
	int operatorPrecedencePriority = 0;

	dParserLexical lexical (inputRules.GetStr());
	LoadTemplateFile("dParserUserVariableTemplate_cpp.txt", userVariableClass);

	// scan the definition segment
	for (dToken token = dToken(lexical.NextToken()); token != GRAMMAR_SEGMENT; ) 
	{
		switch (int (token)) 
		{
			case START:
			{
				token = dToken(lexical.NextToken());
				startSymbol = lexical.GetTokenString();
				token = dToken(lexical.NextToken());
				break;
			}

			case TOKEN:
			{
				for (token = dToken(lexical.NextToken()); token == LITERAL; token = dToken(lexical.NextToken())) {
					//const char* const name = lexical.GetTokenString();
					dSymbolName name (lexical.GetTokenString());
					symbolList.Insert(dTokenInfo (tokenEnumeration, TERMINAL, name), name);
					tokenEnumeration ++;
				}
				break;
			}

			case LEFT:
			case RIGHT:
			{
				dOperatorsAssociation& association = operatorPrecedence.Append()->GetInfo();
				association.m_prioprity = operatorPrecedencePriority;
				operatorPrecedencePriority ++;
				switch (int (token))
				{
					case LEFT:
						association.m_associativity = dOperatorsAssociation::m_left;
						break;
					case RIGHT:
						association.m_associativity = dOperatorsAssociation::m_right;
						break;
				}

				for (token = dToken(lexical.NextToken()); (token == LITERAL) || ((token < 256) && !isalnum (token)); token = dToken(lexical.NextToken())) {
					dAssert (token != -1);
					association.Append(lexical.GetTokenString());
				}
				break;
			}


			case UNION:
			{
				token = dToken(lexical.NextToken());
				dAssert (token == SEMANTIC_ACTION);
				userVariableClass = lexical.GetTokenString() + 1;
				userVariableClass.Replace(userVariableClass.Size() - 1, 1, "");
				token = dToken(lexical.NextToken());
				break;
			}

			case CODE_BLOCK:
			{
				userCodeBlock += lexical.GetTokenString();
				token = dToken(lexical.NextToken());
				break;
			}

			case EXPECT:
			{
				token = dToken(lexical.NextToken());
				dAssert (token == INTEGER);
				m_shiftReduceExpectedWarnings = atoi (lexical.GetTokenString());
				token = dToken(lexical.NextToken());
				break;
			}

			default:;
			{
				dAssert (0);
				token = dToken(lexical.NextToken());
			}
		}
	}

	int ruleNumber = 1;
	lastTokenEnum = tokenEnumeration;

	// scan the production rules segment
	dToken token1 = dToken(lexical.NextToken());
	for (; (token1 != GRAMMAR_SEGMENT) && (token1 != -1); token1 = dToken(lexical.NextToken())) {
		//dTrace (("%s\n", lexical.GetTokenString()));
		switch (int (token1)) 
		{
			case LITERAL:
			{
				// add the first Rule;
				dRuleInfo& rule = ruleList.Append()->GetInfo();
				rule.m_token = token1;
				rule.m_type = NONTERMINAL;
				rule.m_name = lexical.GetTokenString();
				//rule.m_nameCRC = dCRC64 (lexical.GetTokenString());

				dTree<dTokenInfo, dSymbolName>::dTreeNode* nonTerminalIdNode = symbolList.Find(rule.m_name);
				if (!nonTerminalIdNode) {
					nonTerminalIdNode = symbolList.Insert(dTokenInfo (tokenEnumeration, NONTERMINAL, rule.m_name), rule.m_name);
					tokenEnumeration ++;
				}
				rule.m_ruleId = dString (nonTerminalIdNode->GetInfo().m_tokenId);
				rule.m_ruleNumber = ruleNumber;
				ruleNumber ++;

				token1 = ScanGrammarRule(lexical, ruleList, symbolList, ruleNumber, tokenEnumeration, operatorPrecedence); 
				break;
			}
			default:
				dAssert (0);
		}
	}

	dProductionRule::dListNode* firtRuleNode = ruleList.GetFirst();	
	dSymbolName empty;
	if (startSymbol != empty) {
		firtRuleNode = ruleList.Find (startSymbol);	
	}

	//Expand the Grammar Rule by adding an empty start Rule;
	const dRuleInfo& firstRule = firtRuleNode->GetInfo();

	dRuleInfo& rule = ruleList.Addtop()->GetInfo();
	rule.m_ruleNumber = 0;
	rule.m_ruleId = dString(tokenEnumeration);
	rule.m_token = firstRule.m_token;
	rule.m_type = NONTERMINAL;
	rule.m_name = dSymbolName (dString (firstRule.m_name.GetStr()) + dString("__"));
	symbolList.Insert(dTokenInfo (tokenEnumeration, rule.m_type, rule.m_name), rule.m_name);
	tokenEnumeration ++;
	
	dSymbol& symbol = rule.Append()->GetInfo();
	symbol.m_token = firstRule.m_token;
	symbol.m_type = firstRule.m_type;
	symbol.m_name = firstRule.m_name;

	// scan literal use code
	if (token1 == GRAMMAR_SEGMENT) {
		endUserCode = lexical.GetNextBuffer();
		//endUserCode += "\n";
	}
}


dParserCompiler::dToken dParserCompiler::ScanGrammarRule(
	dParserLexical& lexical, 
	dProductionRule& rules, 
	dTree<dTokenInfo, dSymbolName>& symbolList,
	int& ruleNumber,
	int& tokenEnumeration,
	const dOperatorsPrecedence& operatorPrecedence)
{
	dRuleInfo* currentRule = &rules.GetLast()->GetInfo();
	dToken token = dToken(lexical.NextToken());
	do {
		dList<dTokenStringPair> ruleTokens;
		for (token = dToken(lexical.NextToken()); !((token == SIMICOLOM) || (token == OR)); token = dToken(lexical.NextToken())) {
			dAssert (token != -1);

			dTokenStringPair& pair = ruleTokens.Append()->GetInfo();
			pair.m_token = token;
			pair.m_info = lexical.GetTokenString();
		}
		
		dList<dTokenStringPair>::dListNode* lastNode = ruleTokens.GetLast();
		if (lastNode) {
			if (lastNode->GetInfo().m_token != SEMANTIC_ACTION) {
				lastNode = NULL;
			} else {
				currentRule->m_semanticActionCode = lastNode->GetInfo().m_info;
			}
		}
		for (dList<dTokenStringPair>::dListNode* node = ruleTokens.GetFirst(); node && (node != lastNode); node = node->GetNext()) {
			dTokenStringPair& pairOuter = node->GetInfo();

			if (pairOuter.m_token == LITERAL) {
				dSymbol& symbol = currentRule->Append()->GetInfo();
				symbol.m_token = pairOuter.m_token;
				symbol.m_name = pairOuter.m_info;
				//symbol.m_nameCRC = dCRC64 (symbol.m_name.GetStr());

				dTree<dTokenInfo, dSymbolName>::dTreeNode* symbolNode = symbolList.Find(symbol.m_name);
				if (!symbolNode) {
					symbolNode = symbolList.Insert(dTokenInfo (tokenEnumeration, NONTERMINAL, symbol.m_name), symbol.m_name);
					tokenEnumeration ++;
				}
				symbol.m_type = symbolNode->GetInfo().m_type;

			} else if (pairOuter.m_token < 256) {
				dAssert (pairOuter.m_info.Size() == 1);
				dSymbol& symbol = currentRule->Append()->GetInfo();
				symbol.m_name = pairOuter.m_info;
				//symbol.m_nameCRC = dCRC64 (symbol.m_name.GetStr());

				symbol.m_type = TERMINAL;
				symbol.m_token = LITERAL;
				symbolList.Insert(dTokenInfo (pairOuter.m_token, TERMINAL, symbol.m_name), symbol.m_name);

			} else if (pairOuter.m_token == PREC) {
				node = node->GetNext();
				for (dRuleInfo::dListNode* ruleNode = currentRule->GetLast(); ruleNode; ruleNode = ruleNode->GetPrev()) {
					dSymbol& symbol = ruleNode->GetInfo();
					if (operatorPrecedence.FindAssociation (symbol.m_name)) {
						dTokenStringPair& pair = node->GetInfo();		
						symbol.m_operatorPrecendeceOverrride = pair.m_info;
						break;
					}
				}
//			} else if (pair.m_token != SEMANTIC_ACTION) {
//				// no user action allowed in the middle of a sentence
//				_ASSERTE (pair.m_token == SEMANTIC_ACTION);
//			} else {
//				_ASSERTE (0);
			}
		}

		if (token == OR) {
			// this is a rule with multiples sentences alternates, add new rule with the same name Non terminal
			dRuleInfo& rule = rules.Append()->GetInfo();
			rule.m_ruleNumber = ruleNumber;
			ruleNumber ++;
			rule.m_ruleId = currentRule->m_ruleId;
			rule.m_token = currentRule->m_token;
			rule.m_type = NONTERMINAL;
			//rule.m_name += currentRule->m_name;
			rule.m_name = currentRule->m_name;
			//rule.m_nameCRC = currentRule->m_nameCRC;
			currentRule = &rule;
		}

	} while (token != SIMICOLOM);

	return token;

}


// generates the canonical Items set for a LR(1) grammar
void dParserCompiler::CanonicalItemSets (
	dTree<dState*, dSymbolName>& stateMap,
	const dProductionRule& ruleList, 
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dOperatorsPrecedence& operatorPrecedence,
	const dString& fileName)
{
	dList<dItem> itemSet;
	dList<dState*> stateList;
	FILE* debugFile = NULL;

	fileName;
	#ifdef D_WRITE_STATE_TRANSITION_GRAPH
	dString debugFileName(fileName);
	debugFileName += ".txt";
	debugFile = fopen(debugFileName.GetStr(), "w");
	#endif

	// start by building an item set with only the first rule
	dItem& item = itemSet.Append()->GetInfo();
	item.m_indexMarker = 0;
	//item.m_lookAheadSymbolCRC = DACCEPT_SYMBOL;
	item.m_lookAheadSymbol = DACCEPT_SYMBOL;
	item.m_ruleNode = ruleList.GetFirst();

	// build a rule info map
	dTree<dList<void*>, dSymbolName> ruleMap;
	for (dProductionRule::dListNode* ruleNode = ruleList.GetFirst(); ruleNode; ruleNode = ruleNode->GetNext()) {
		dRuleInfo& info = ruleNode->GetInfo();

		dTree<dList<void*>, dSymbolName>::dTreeNode* node = ruleMap.Find(info.m_name);
		if (!node) {
			node = ruleMap.Insert(info.m_name);
		}
		dList<void*>& entry = node->GetInfo();
		entry.Append(ruleNode);
	}

	// find the closure for the first this item set with only the first rule
	dState* const stateOuter = Closure (itemSet, symbolList, ruleMap);
	operatorPrecedence.SaveLastOperationSymbol (stateOuter);

	stateMap.Insert(stateOuter, stateOuter->GetKey());
	stateList.Append(stateOuter);

	stateOuter->Trace(debugFile);

	// now for each state found 
	int stateNumber = 1;
	for (dList<dState*>::dListNode* node = stateList.GetFirst(); node; node = node->GetNext()) {
		dState* const state = node->GetInfo();
		dTree<dTokenInfo, dSymbolName>::Iterator iter (symbolList);
		for (iter.Begin(); iter; iter ++) {
			dSymbolName symbol (iter.GetKey());
			dState* const newState = Goto (state, symbol, symbolList, ruleMap);
			dAssert (newState);
			if (newState->GetCount()) {
				const dTokenInfo& tokenInfo = iter.GetNode()->GetInfo();
				dTransition& transition = state->m_transitions.Append()->GetInfo();
				transition.m_symbol = symbol;
				transition.m_name = tokenInfo.m_name; 
				transition.m_type = tokenInfo.m_type;
				dAssert (transition.m_symbol == transition.m_name);
				transition.m_targetState = newState;

				dTree<dState*, dSymbolName>::dTreeNode* const targetStateNode = stateMap.Find(newState->GetKey());
				if (!targetStateNode) {
					newState->m_number = stateNumber;

					stateNumber ++;
					stateMap.Insert(newState, newState->GetKey());
					newState->Trace(debugFile);
					stateList.Append(newState);

					operatorPrecedence.SaveLastOperationSymbol (newState);

				} else {
					transition.m_targetState = targetStateNode->GetInfo();
					delete newState;
				}
			} else {
				delete newState;
			}
		}
		//dTrace (("state#:%d   items: %d   transitions: %d\n", state->m_number, state->GetCount(), state->m_transitions.GetCount()));
	}

	if (debugFile) {
		fclose(debugFile);
	}
}

// Generate the closure for a Set of Item  
dParserCompiler::dState* dParserCompiler::Closure (
	const dList<dItem>& itemSet, 
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dTree<dList<void*>, dSymbolName>& ruleMap) const
{
	dState* const state = new dState (itemSet);
	for (dState::dListNode* itemNodeOuter = state->GetFirst(); itemNodeOuter; itemNodeOuter = itemNodeOuter->GetNext()) {
		dItem& item = itemNodeOuter->GetInfo();

		dRuleInfo::dListNode* const symbolNode = item.m_ruleNode->GetInfo().GetSymbolNodeByIndex (item.m_indexMarker);
		if (symbolNode) {
			// get Beta token dString
			const dRuleInfo& rule = item.m_ruleNode->GetInfo();
			dRuleInfo::dListNode* ruleNodeOuter = rule.GetFirst();
			for (int i = 0; i < item.m_indexMarker; i ++) {
				ruleNodeOuter = ruleNodeOuter->GetNext();
			}

			dList<dSymbolName> firstSymbolList;
			for (ruleNodeOuter = ruleNodeOuter->GetNext(); ruleNodeOuter; ruleNodeOuter = ruleNodeOuter->GetNext()) {
				const dSymbol& symbol = ruleNodeOuter->GetInfo();
				firstSymbolList.Append(symbol.m_name);
			}

			firstSymbolList.Append(item.m_lookAheadSymbol);
			const dSymbol& sentenceSymbol = symbolNode->GetInfo();

			dTree<dList<void*>, dSymbolName>::dTreeNode* const ruleNodes = ruleMap.Find(sentenceSymbol.m_name);
			if (ruleNodes) {
				const dList<void*>& matchingRulesList = ruleNodes->GetInfo();
				dAssert (matchingRulesList);
				for (dList<void*>::dListNode* node = matchingRulesList.GetFirst(); node; node = node->GetNext()) {
					dProductionRule::dListNode* const ruleNode = (dProductionRule::dListNode*) node->GetInfo();
					dAssert (ruleNode->GetInfo().m_name == sentenceSymbol.m_name);
					dTree<int, dSymbolName> firstList;
					First (firstSymbolList, symbolList, ruleMap, firstList);
					dTree<int, dSymbolName>::Iterator firstIter (firstList);
					for (firstIter.Begin(); firstIter; firstIter ++) {
						dSymbolName symbol = firstIter.GetKey();
						dState::dListNode* const itemNode = state->FindItem(ruleNode, 0, symbol);
						if (!itemNode) {
							dItem newItem;
							newItem.m_indexMarker = 0;
							newItem.m_ruleNode = ruleNode;
							newItem.m_lookAheadSymbol = symbolList.Find (symbol)->GetInfo().m_name;
							state->AddItem(newItem);
						}
					}
				}
			}
		}
	}

	state->CalculateKey ();
	return state;
}


void dParserCompiler::First (
	const dList<dSymbolName>& symbolSet,
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dTree<dList<void*>, dSymbolName>& ruleMap,
	dTree<int, dSymbolName>& firstSetOut) const
{
	if (symbolSet.GetCount() > 1) {

		dList<dSymbolName>::dListNode* node = symbolSet.GetFirst();
		bool deriveEmpty = true;
		while ((deriveEmpty) && node) {
			dSymbolName symbolOuter = node->GetInfo();
			node = node->GetNext();

			dTree<int, dSymbolName> tmpFirst;
			dTree<int, dSymbolName> symbolListMark;
			First (symbolOuter, symbolListMark, symbolList, ruleMap, tmpFirst);
			dTree<int, dSymbolName>::Iterator iter (tmpFirst);
			deriveEmpty = false;  
			for (iter.Begin(); iter; iter ++) {
				dSymbolName symbol = iter.GetKey();
				static dSymbolName empty;
				if (symbol == empty) {
					deriveEmpty = true;  
				} else {
					firstSetOut.Insert(0, symbol);
				}
			}
		}
		if (deriveEmpty) {
			firstSetOut.Insert(0, 0);
		}

	} else  {
		dSymbolName symbol = symbolSet.GetFirst()->GetInfo();
		dTree<int, dSymbolName> symbolListMark;
		First (symbol, symbolListMark, symbolList, ruleMap, firstSetOut);
	}
}

void dParserCompiler::First (
	dSymbolName symbolOuter,
	dTree<int, dSymbolName>& symbolListMark,
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dTree<dList<void*>, dSymbolName>& ruleMap,
	dTree<int, dSymbolName>& firstSetOut) const
{
	if (symbolListMark.Find(symbolOuter)) {
		return;
	}
	symbolListMark.Insert(0, symbolOuter);

	dTree<dTokenInfo, dSymbolName>::dTreeNode* const nodeOuter = symbolList.Find(symbolOuter);
	dAssert (nodeOuter);
	if (nodeOuter->GetInfo().m_type == TERMINAL) {
		firstSetOut.Insert(0, symbolOuter);
	} else if (DoesSymbolDeriveEmpty (symbolOuter, ruleMap)) {
		firstSetOut.Insert(0, 0);
	} else {

		dTree<dList<void*>, dSymbolName>::dTreeNode* const ruleNodes = ruleMap.Find(symbolOuter);
		if (ruleNodes) {
			const dList<void*>& matchingRulesList = ruleNodes->GetInfo();
			for (dList<void*>::dListNode* node = matchingRulesList.GetFirst(); node; node = node->GetNext()) {
				dProductionRule::dListNode* const ruleInfoNode = (dProductionRule::dListNode*) node->GetInfo();
				const dRuleInfo& info = ruleInfoNode->GetInfo();

				bool allDeriveEmpty = true;
				for (dRuleInfo::dListNode* sentenceSymbolNode = info.GetFirst(); sentenceSymbolNode; sentenceSymbolNode = sentenceSymbolNode->GetNext()) {
					const dSymbol& sentenceSymnol = sentenceSymbolNode->GetInfo();
					if (!DoesSymbolDeriveEmpty (sentenceSymnol.m_name, ruleMap)) {
						allDeriveEmpty = false;
						dTree<int, dSymbolName> newFirstSetOut;
						First (sentenceSymnol.m_name, symbolListMark, symbolList, ruleMap, newFirstSetOut);
						dTree<int, dSymbolName>::Iterator iter (newFirstSetOut);
						for (iter.Begin(); iter; iter ++) {
							dSymbolName symbol = iter.GetKey();
							dAssert (symbol.GetString() != "");
							dAssert (symbolList.Find(symbol)->GetInfo().m_type == TERMINAL);
							firstSetOut.Insert(0, symbol);
						}
						break;
					}
				}
				if (allDeriveEmpty) {
					dTrace (("this could be a bug here, I am not sure if I should closure with the accepting state or not, need more debugging\n"))
//					firstSetOut.Insert(0, 0);
				}
			}
		}
	}
}


bool dParserCompiler::DoesSymbolDeriveEmpty (dSymbolName symbol, const dTree<dList<void*>, dSymbolName>& ruleMap) const
{
	dTree<dList<void*>, dSymbolName>::dTreeNode* const ruleNodes = ruleMap.Find(symbol);
	if (ruleNodes) {
		//const dList<void*>& matchingRulesList = ruleMap.Find(symbol)->GetInfo();
		const dList<void*>& matchingRulesList = ruleNodes->GetInfo();
		for (dList<void*>::dListNode* node = matchingRulesList.GetFirst(); node; node = node->GetNext()) {
			dProductionRule::dListNode* const ruleInfoNode = (dProductionRule::dListNode*) node->GetInfo();

			const dRuleInfo& info = ruleInfoNode->GetInfo();
			if (symbol == info.m_name) {
				if (!info.GetCount()) {
					return true;
				}
			}
		}
	}
	return false;
}

// generates the got state for this symbol
dParserCompiler::dState* dParserCompiler::Goto (
	const dState* const state, 
	const dSymbolName& symbol,
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dTree<dList<void*>, dSymbolName>& ruleMap) const
{
	dList<dItem> itemSet;

	// iterate over each item contained on state
	for (dState::dListNode* node = state->GetFirst(); node; node = node->GetNext()) {
		const dItem& item = node->GetInfo();

		int index = 0;
		const dRuleInfo& ruleInfo = item.m_ruleNode->GetInfo();
		// for this item get it rule, and iterate over ever rule symbol.  		
		for (dRuleInfo::dListNode* infoSymbolNode = ruleInfo.GetFirst(); infoSymbolNode; infoSymbolNode = infoSymbolNode->GetNext()) {
			// see if this rule symbol match symbol
			const dSymbol& infoSymbol = infoSymbolNode->GetInfo();
			if (infoSymbol.m_name == symbol) {
				// a symbol was found, now see if it is at the right position in the rule
				if (index == item.m_indexMarker) {
					// the rule match symbol at the right position, add this rule to the new item set.
					dItem& newItem = itemSet.Append()->GetInfo();
					newItem.m_indexMarker = index + 1;
					newItem.m_ruleNode = item.m_ruleNode;
					newItem.m_lookAheadSymbol = item.m_lookAheadSymbol;
					break;
				}
			}
			index ++;
		}
	}

	//find the closure for this new item set.
	dState* const closureState = Closure (itemSet, symbolList, ruleMap);
	return closureState;
}

void dParserCompiler::ReplaceMacro (dString& data, const dString& newName, const dString& macro) const
{
	int size = int(macro.Size());
	int position = int (data.Find (macro));
	dAssert (position != -1);
	data.Replace(position, size, newName);
}

void dParserCompiler::ReplaceAllMacros (dString& data, const dString& newName, const dString& macro) const
{
	int size = int (macro.Size());
	for (int i = data.Find (macro); i != -1; i = data.Find (macro)) {
		data.Replace(i, size, newName);
	}
}


void dParserCompiler::GenerateHeaderFile (
	const dString& className, 
	const dString& scannerClassName, 
	const char* const outputFileName, 
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	const dString& userVariableClass) 
{
	dString templateHeader;
	LoadTemplateFile("dParserTemplate_h.txt", templateHeader);

	ReplaceAllMacros (templateHeader, className, "$(className)");
	ReplaceAllMacros (templateHeader, scannerClassName, "$(scannerClass)");
	ReplaceMacro (templateHeader, userVariableClass, "$(userVariableClass)");

	dTree<dTree<dTokenInfo, dSymbolName>::dTreeNode*, int> sortToken;
	dTree<dTokenInfo, dSymbolName>::Iterator iter (symbolList);
	for (iter.Begin(); iter; iter ++) {
		const dTokenInfo& info = iter.GetNode()->GetInfo();
		if ((info.m_type == TERMINAL) && (info.m_tokenId >= 256)) {
			sortToken.Insert(iter.GetNode(), info.m_tokenId);
		}
	} 

	dTree<dTree<dTokenInfo, dSymbolName>::dTreeNode*, int>::Iterator iter1 (sortToken);
	bool first = true;
	char text[256];
	sprintf (text, " = %d, \n", DACCEPTING_TOKEN);
	dString enumdTokens ("\t\tACCEPTING_TOKEN");
	enumdTokens += text;

	sprintf (text, " = %d, \n", DERROR_TOKEN);
	enumdTokens += "\t\tERROR_TOKEN";
	enumdTokens += text;

	for (iter1.Begin(); iter1; iter1 ++) {
		dTree<dTokenInfo, dSymbolName>::dTreeNode* const node = iter1.GetNode()->GetInfo();
		const dString& name = node->GetInfo().m_name.GetString();
		enumdTokens += "\t\t";
		enumdTokens += name;
		if (first) {
			first = false;
			sprintf (text, " = %d, \n", iter1.GetKey());
			enumdTokens += text;
		} else {
			enumdTokens += ",\n";
		}
	}

	enumdTokens.Replace(enumdTokens.Size()-2, 2, "");
	ReplaceMacro (templateHeader, enumdTokens, "$(Tokens)");

	SaveFile(outputFileName, ".h", templateHeader);
}


void dParserCompiler::GenerateParserCode (
	const dString& className, 
	const dString& scannerClassName, 
	const char* const outputFileName, 
	const dTree<dTokenInfo, dSymbolName>& symbolList,
	dTree<dState*, dSymbolName>& stateList,
	const dString& userCode, 
	const dString& endUserCode,
	int lastTerminalTokenEnum)
{
	dString templateHeader ("");
	LoadTemplateFile("dParserTemplate _cpp.txt", templateHeader);

	int position = templateHeader.Find ("$(userCode)");
	templateHeader.Replace(position, 11, userCode);

	ReplaceAllMacros (templateHeader, className, "$(className)");
	ReplaceAllMacros (templateHeader, scannerClassName, "$(scannerClass)");

	char textOuter[256];
	sprintf (textOuter, "%d", lastTerminalTokenEnum);
	ReplaceMacro (templateHeader, textOuter, "&(lastTerminalToken)");

	dTree<dState*, int> sortedStates;
	dTree<dState*, dSymbolName>::Iterator stateIter (stateList);
	for (stateIter.Begin(); stateIter; stateIter ++) {
		dState* const state = stateIter.GetNode()->GetInfo();
		sortedStates.Insert(state, state->m_number);
	}

	dTree<int, dString> actionFilter;

	dString emptySematicAction ("");
	dString stateActionsStart ("");
	dString stateActionsCount ("");
	dString nextActionsStateList ("");
	dString sematicActions ("");
	int entriesCount = 0;

	int newLineCount = 0;
	int starAndCountIndex = 0;
	dTree<dState*, int>::Iterator sortStateIter (sortedStates);

	const char* const caseTabs0 = "\t\t\t\t\t\t";
	//const char* const caseTabs1 = "\t\t\t\t\t\t\t";
	for (sortStateIter.Begin(); sortStateIter; sortStateIter ++) {
		dState* const state = sortStateIter.GetNode()->GetInfo();

		int count = 0;
		dTree<dActionEntry, int> actionSort;
		dTree<dAction, dSymbolName>::Iterator actionIter (state->m_actions);
		for (actionIter.Begin(); actionIter; actionIter++) {
			count ++;

			dAction& action = actionIter.GetNode()->GetInfo();
			if (action.m_type == dSHIFT) {
				dSymbolName actionSymbol = actionIter.GetKey();
				dAssert (symbolList.Find(actionSymbol));

				dActionEntry entry;
				entry.m_stateType = char (action.m_type);
				entry.m_errorRule = state->m_hasErroItem ? 1 : 0;
				entry.m_ruleIndex = 0;
				entry.m_ruleSymbols = 0;
				entry.m_nextState = short (action.m_nextState);
				entry.m_token = short (symbolList.Find(actionSymbol)->GetInfo().m_tokenId);

				actionSort.Insert (entry, entry.m_token);

			} else if (action.m_type == dREDUCE) {

				dSymbolName actionSymbol = actionIter.GetKey();
				dAssert (symbolList.Find(actionSymbol));

				dRuleInfo& reduceRule = action.m_reduceRuleNode->GetInfo();
				dAssert (symbolList.Find(reduceRule.m_name));
				dAssert (symbolList.Find(reduceRule.m_name)->GetInfo().m_tokenId >= 256);

				dActionEntry entry;
				entry.m_stateType = char (action.m_type);
				entry.m_errorRule = 0; //state->m_hasErroItem ? 1 : 0;
				entry.m_ruleIndex = short (reduceRule.m_ruleNumber);
				entry.m_ruleSymbols = short (reduceRule.GetCount());
				entry.m_nextState = short (symbolList.Find(reduceRule.m_name)->GetInfo().m_tokenId - lastTerminalTokenEnum);
				entry.m_token = short (symbolList.Find(actionSymbol)->GetInfo().m_tokenId);
				actionSort.Insert (entry, entry.m_token);

				if (!reduceRule.m_ruleReduced && (reduceRule.m_semanticActionCode != emptySematicAction)) {
					// issue a sematic action code;

					reduceRule.m_ruleReduced = true;
					char text[128];
					dString userSematicAction (reduceRule.m_semanticActionCode);
					int symbolsCount = int (entry.m_ruleSymbols);
					for (int i = 0; i < symbolsCount; i ++) {

						sprintf (text, "%d", symbolsCount - i);
						dString macro ("$");
						macro += text;

						sprintf (text, "%d", symbolsCount - i - 1);
						dString macroVariable ("parameter[");
						macroVariable += text;
						macroVariable += "].m_value";
						ReplaceAllMacros (userSematicAction, macroVariable, macro);
					}
					ReplaceAllMacros (userSematicAction, "entry.m_value", "$$");

					sprintf (text, "%d:", reduceRule.m_ruleNumber);
					sematicActions += caseTabs0;
					sematicActions += "case "; 
					sematicActions += text; 
					//sematicActions += "// rule ";
					sematicActions += "// ";
					sematicActions += reduceRule.m_name.GetStr();
					sematicActions += " : ";
					for (dRuleInfo::dListNode* node = reduceRule.GetFirst(); node; node = node->GetNext()) {
						sematicActions+= node->GetInfo().m_name.GetString();
						sematicActions += " ";
					}
					sematicActions += "\n";
					sematicActions += userSematicAction;
					sematicActions += "\nbreak;\n\n";
				}

			} else {
				dAssert (action.m_type == dACCEPT);

				dActionEntry entry;
				entry.m_stateType = char (action.m_type);
				entry.m_errorRule = 0; //state->m_hasErroItem ? 1 : 0;
				entry.m_ruleIndex = 0;
				entry.m_ruleSymbols = 0;
				entry.m_nextState = 0;
				entry.m_token = DACCEPTING_TOKEN;
				actionSort.Insert (entry, entry.m_token);
			}
		}

		int actionIndex = entriesCount;
		dString stateActions ("");
		dTree<dActionEntry, int>::Iterator iter (actionSort);
		for (iter.Begin(); iter; iter ++) {
			const dActionEntry& entry = iter.GetNode()->GetInfo();
			sprintf (textOuter, "%d, %d, %d, %d, %d, %d, ", entry.m_token, entry.m_errorRule, entry.m_stateType, entry.m_nextState, entry.m_ruleSymbols, entry.m_ruleIndex);
			stateActions += textOuter;
			entriesCount ++;
		}

		dTree<int, dString>::dTreeNode* const stateActionNode = actionFilter.Find(stateActions);
		if (stateActionNode) {
			entriesCount = actionIndex;
			actionIndex =  stateActionNode->GetInfo();
		} else {
			actionFilter.Insert(actionIndex, stateActions);

			for (iter.Begin(); iter; iter ++) {
				if (newLineCount % 4 == 0) {
					nextActionsStateList += "\n\t\t\t";
				}
				newLineCount ++;
				const dActionEntry& entry = iter.GetNode()->GetInfo();
				sprintf (textOuter, "dActionEntry (%d, %d, %d, %d, %d, %d), ", entry.m_token, entry.m_errorRule, entry.m_stateType, entry.m_nextState, entry.m_ruleSymbols, entry.m_ruleIndex);
				nextActionsStateList += textOuter;
			}
		}

		if ((starAndCountIndex % 24) == 0) {
			stateActionsStart += "\n\t\t\t";
			stateActionsCount += "\n\t\t\t";
		}
		starAndCountIndex ++;

		sprintf (textOuter, "%d, ", actionIndex);
		stateActionsStart += textOuter;

		sprintf (textOuter, "%d, ", count);
		stateActionsCount += textOuter;
	}
	nextActionsStateList.Replace(nextActionsStateList.Size()-2, 2, "");
	stateActionsCount.Replace(stateActionsCount.Size()-2, 2, "");
	stateActionsStart.Replace(stateActionsStart.Size()-2, 2, "");

	ReplaceMacro (templateHeader, stateActionsCount, "$(actionsCount)");
	ReplaceMacro (templateHeader, stateActionsStart, "$(actionsStart)");
	ReplaceMacro (templateHeader, nextActionsStateList, "$(actionTable)");

	ReplaceMacro (templateHeader, sematicActions, "$(semanticActionsCode)");

	dString stateGotoStart ("");
	dString stateGotoCount ("");
	dString nextGotoStateList ("");
	entriesCount = 0;
	int newLine = 0;
	int gotoStateCount = 0;
	for (sortStateIter.Begin(); sortStateIter; sortStateIter ++) {

		char text[256];
		dState* const state = sortStateIter.GetNode()->GetInfo();

		int currentEntryuCount = entriesCount;

		int count = 0;
		dTree<dState*, dSymbolName>::Iterator gotoIter (state->m_goto);
		dTree<dTree<dState*, dSymbolName>::dTreeNode*, int> sortGotoActions;
		for (gotoIter.Begin(); gotoIter; gotoIter++) {
			int id = symbolList.Find(gotoIter.GetKey())->GetInfo().m_tokenId;
			sortGotoActions.Insert(gotoIter.GetNode(), id);
		}

		dTree<dTree<dState*, dSymbolName>::dTreeNode*, int>::Iterator iter1 (sortGotoActions);
		for (iter1.Begin(); iter1; iter1++) {
			count ++;
			if ((newLine % 5) == 0) {
				nextGotoStateList += "\n\t\t\t";
			}
			newLine ++;

			dTree<dState*, dSymbolName>::dTreeNode* const node = iter1.GetNode()->GetInfo();
			dState* const targetState = node->GetInfo();

			dGotoEntry entry;
			entry.m_nextState = short (targetState->m_number);
			entry.m_token = short(iter1.GetKey());

			sprintf (text, "dGotoEntry (%d, %d), ", entry.m_token, entry.m_nextState);
			nextGotoStateList += text;
			entriesCount ++;
		}

		if ((gotoStateCount % 24) == 0) {
			stateGotoStart += "\n\t\t\t";
			stateGotoCount += "\n\t\t\t";
		}
		gotoStateCount ++;

		sprintf (text, "%d, ", currentEntryuCount);
		stateGotoStart += text;

		sprintf (text, "%d, ", count);
		stateGotoCount += text;
	}

	nextGotoStateList.Replace(nextGotoStateList.Size()-2, 2, "");
	stateGotoCount.Replace(stateGotoCount.Size()-2, 2, "");
	stateGotoStart.Replace(stateGotoStart.Size()-2, 2, "");

	ReplaceMacro (templateHeader, stateGotoCount, "$(gotoCount)");
	ReplaceMacro (templateHeader, stateGotoStart, "$(gotoStart)");
	ReplaceMacro (templateHeader, nextGotoStateList, "$(gotoTable)");

	templateHeader += endUserCode;
	SaveFile(outputFileName, ".cpp", templateHeader);
}


void dParserCompiler::BuildParsingTable (const dTree<dState*, dSymbolName>& stateList, const dSymbolName& startSymbol, const dOperatorsPrecedence& operatorPrecedence) const
{
	dTree<dState*, dSymbolName>::Iterator stateIter (stateList);

	const dSymbolName emptySymbol;

	const dSymbolName acceptingSymbol (DACCEPT_SYMBOL);
	// create Shift Reduce action table
	for (stateIter.Begin(); stateIter; stateIter ++) {
		dState* const state = stateIter.GetNode()->GetInfo();

		// add all shift actions first
		for (dList<dTransition>::dListNode* node = state->m_transitions.GetFirst(); node; node = node->GetNext()) {
			dTransition& transition = node->GetInfo();
			if (transition.m_type == TERMINAL) {

				// find item generating this shift action and mark it as used.
				const dState* const targetState = transition.m_targetState;
				dAssert (!state->m_actions.Find (transition.m_symbol));
				dTree<dAction, dSymbolName>::dTreeNode* const actionNode = state->m_actions.Insert (transition.m_symbol);
				dAction& action = actionNode->GetInfo();
				action.m_type = dSHIFT;
				action.m_myItem = NULL;
				action.m_nextState = targetState->m_number;
				action.m_reduceRuleNode = NULL;
			}
		}

		// add all reduce actions
		dList<dAction*> potencialConflictinActions;
		for (dState::dListNode* itemNode = state->GetFirst(); itemNode; itemNode = itemNode->GetNext()) {
			dItem& item = itemNode->GetInfo();
			const dRuleInfo& ruleInfo = item.m_ruleNode->GetInfo();
			if ((ruleInfo.m_ruleNumber == 0) && (item.m_indexMarker == 1)) {
				dTree<dAction, dSymbolName>::dTreeNode* const actionNode = state->m_actions.Insert (acceptingSymbol);
				dAssert (actionNode);
				dAction& action = actionNode->GetInfo();
				action.m_type = dACCEPT;
				action.m_myItem = &item;
				action.m_reduceRuleNode = NULL;
			} else if ((item.m_indexMarker == ruleInfo.GetCount()) && (ruleInfo.m_name != startSymbol)) {
				dTree<dAction, dSymbolName>::dTreeNode* actionNode = state->m_actions.Find (item.m_lookAheadSymbol);
				if (!actionNode) {
					actionNode = state->m_actions.Insert (item.m_lookAheadSymbol); 
					dAction& action = actionNode->GetInfo();
					action.m_type = dREDUCE;
					action.m_myItem = &item;
					action.m_nextState = 0;
					action.m_reduceRuleNode = item.m_ruleNode;
				} else {
					dAction& action = actionNode->GetInfo();
					action.m_myItem = &item;
					action.m_reduceRuleNode = item.m_ruleNode;
					potencialConflictinActions.Append (&actionNode->GetInfo());
				}
			}
		}

		// now resolve all conflicting actions
		if (potencialConflictinActions.GetCount()) {

			// resolve conflicting actions
			dList<dAction*>::dListNode* nextActionNode = NULL;
			for (dList<dAction*>::dListNode* actionNode = potencialConflictinActions.GetFirst(); actionNode; actionNode = nextActionNode) {
				dAction* const action = actionNode->GetInfo();

				if (action->m_type == dREDUCE) {
					// this is a reduce reduce conflict
					dAssert (0);
					dTrace (("This is a reduce Reduce conflict, resolve in favor of of first production rule\n")); 
				}
				nextActionNode = actionNode->GetNext();

				const dItem& item = *action->m_myItem;
				if (item.m_lastOperatorSymbol != emptySymbol) {
					const dOperatorsAssociation* const operatorAssosiation = operatorPrecedence.FindAssociation (item.m_lastOperatorSymbol);
					dAssert (operatorAssosiation);
					if (operatorAssosiation->m_associativity == dOperatorsAssociation::m_left) {

						const dOperatorsAssociation* const lookAheadOperatorAssosiation = operatorPrecedence.FindAssociation (item.m_lookAheadSymbol);
						if (!(lookAheadOperatorAssosiation && (lookAheadOperatorAssosiation->m_prioprity > operatorAssosiation->m_prioprity))) {
							action->m_type = dREDUCE;
						}
					}
					potencialConflictinActions.Remove(actionNode);
				}
			}

			// for any conflicting actions left, display warning
			for (dList<dAction*>::dListNode* actionNode = potencialConflictinActions.GetFirst(); actionNode; actionNode = actionNode->GetNext()) {
				dAction* const action = actionNode->GetInfo();

				dRuleInfo& rule = action->m_reduceRuleNode->GetInfo();
				dString sentence;
				sentence += rule.m_name.GetString();
				sentence += " : ";
				for (dRuleInfo::dListNode* node = rule.GetFirst(); node; node = node->GetNext()) {
					sentence += node->GetInfo().m_name.GetString();
					sentence += " ";
				}

				if (action->m_type == dSHIFT) {
					if (!rule.m_shiftReduceErrorMark) {
						rule.m_shiftReduceErrorMark = true;
						if (m_shiftReduceExpectedWarnings <= 0) {
							DisplayError ("\nstate %d: shift reduce warning resolved in favor of shift. on rule\n", state->m_number);
							DisplayError ("  %s\n", sentence.GetStr());
						}
						m_shiftReduceExpectedWarnings --;
					}
					
				} else {
					dAssert (0);
					DisplayError ("\nstate %d: reduce reduce error resolved in favor of first sentence. on rule\n", state->m_number);
					DisplayError ("  %s\n", sentence.GetStr());
				}
			}
		}
	}


	// create Goto Table
	for (stateIter.Begin(); stateIter; stateIter ++) {
		dState* const state = stateIter.GetNode()->GetInfo();
		for (dList<dTransition>::dListNode* node = state->m_transitions.GetFirst(); node; node = node->GetNext()) {
			dTransition& transition = node->GetInfo();
			if (transition.m_type == NONTERMINAL) {
				state->m_goto.Insert (transition.m_targetState, transition.m_symbol); 
			}
		}
	}

}