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

// dLexCompiler.cpp : Defines the entry point for the console application.
//

#include "dLexCompiler.h"
#include "dLexScannerGenerator.h"

#ifdef _MSC_VER
	#pragma warning (disable: 4100) //  unreferenced formal parameter
#endif


enum dLexCompiler::dToken
{
	m_whiteSpace,
	m_comment,
	m_delimiter,
	m_codeBlock,
	m_internalSize,
	m_number,
	m_quatedString,
	m_literal,
	m_extendedRegularExpression,
	m_curlyBrace,
	m_end,
};



class dLexCompiler::dTokenData: public dDeterministicFiniteAutonata
{
	public:
	dTokenData (dToken token, const char* const regulatExpression)
		:dDeterministicFiniteAutonata (regulatExpression)
		,m_token(token)
	{
	}

	~dTokenData()
	{

	}
	dToken m_token;
};


dLexCompiler::dTokenDataList::dTokenDataList ()
	:dList<dTokenData*>()
{
}

dLexCompiler::dTokenDataList::~dTokenDataList ()
{
	DeleteAll();
}

void dLexCompiler::dTokenDataList::AddTokenData (dToken token, const char* const regulatExpression)
{
	dTokenData* const data = new dTokenData (token, regulatExpression);
	Append (data);
}

void dLexCompiler::dTokenDataList::DeleteAll()
{
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		delete node->GetInfo();
	}
	dList<dTokenData*>::RemoveAll();
}


dLexCompiler::dDefinitionsMap::dDefinitionsMap ()
{

}
dLexCompiler::dDefinitionsMap::~dDefinitionsMap ()
{
}



void dLexCompiler::dDefinitionsMap::PreProcessDefinitions (dString& regularExpressionWithMacros)
{
	int i0 = 0;
	for (int i1 = int (regularExpressionWithMacros.Find('{', i0)); i1 != -1; i1 = int (regularExpressionWithMacros.Find('{', i0))) {
		i0 = i1;
		int size = int (regularExpressionWithMacros.Size());
		for (i1 ++; (i1 <= size) && isalnum (regularExpressionWithMacros[i1]); i1 ++) {
		} 
		if (i1 < size) {
			dString expressionName (&regularExpressionWithMacros[i0 + 1], i1 - i0 - 1);
			dTreeNode* const node = Find(dCRC64 (expressionName.GetStr()));
			if (node) {
				dString expression (node->GetInfo());
				regularExpressionWithMacros.Replace(i0, i1 - i0 + 1, expression);
			} else {
				i0 ++;
			}
		} else {
			i0 ++;
		}
	}
}

void dLexCompiler::dDefinitionsMap::AddDefinition (dString& regularExpression, dString& key)
{
	PreProcessDefinitions (regularExpression);
	Insert (regularExpression, dCRC64 (key.GetStr()));
}


class dLexCompiler::dExpandedState: public dAutomataState
{
	public:
	dExpandedState (int id)
		:dAutomataState (id)
		,m_lineNumber(0)
		,m_hasUserAction (false)
		,m_userAction("")
	{
	}	

	~dExpandedState()
	{
	}

	int m_lineNumber;
	bool m_hasUserAction;	
	dString m_userAction;
};


class dLexCompiler::dExpandedNFA: public dNonDeterministicFiniteAutonata
{
public:
	dExpandedNFA ()
		:dNonDeterministicFiniteAutonata()
		,m_empty (true), m_lineNumber(0)
	{
	}

	dAutomataState* CreateState (int id)
	{
		dExpandedState* const state =  new dExpandedState (id); 
		state->m_lineNumber = m_lineNumber;
		return state;
	}

	void PushSet (const char* const set, int size)
	{
//		dNonDeterministicFiniteAutonata::PushSet (set, size);
		dAutomataState* const startState = CreateState (m_stateID ++);
		dAutomataState* const acceptingState = CreateState (m_stateID ++);

		for (int i = 0; i < size; i ++) {
			dAutomataState::dCharacter charInfo (GetScapeChar (set[i]), dAutomataState::CHARACTER);
			startState->m_transtions.Append(dAutomataState::dTransition(charInfo, acceptingState));
		}
		m_stack.Push(startState, acceptingState);
	}

	void PreProcessExpression (const char* const regularExpression)
	{
		char buffer[D_ASCII_SET_MAX_SIZE];

		dAssert (sizeof (m_regularExpression) > strlen (regularExpression));
		sprintf (m_regularExpression, "%s", regularExpression);

		int i = 0;
		for (int ch = GetChar(); ch; ch = GetChar()) {
			if ((ch>>8) == '\\') {
				switch (char (ch))
				{
					case '~':
					case '!':
					case '@':
					case '#':
					case '$':
					case '%':
					case '^':
					case '&':
					case '*':
					case '(':
					case ')':
					case '-':
					case '+':


					case '{':
					case '}':
					case '|':
					case ':':
					case '"':
					case '<':
					case '>':
					case '?':

					case '`':
					case '_':
					case '=':
					case '[':
					case ']':
					case '\\':
					case ';':
					case '\'':
					case ',':
					case '.':
					case '/':

					case 'n':
					case 'r':
					case 't':
					case 'v':
					case 'f':
						buffer[i] = '\\';
						i ++;
						break;

					case '1':
						if (m_regularExpression[m_regularExpressionIndex] == 'b') {
							GetChar();
							ch = 0x1b;
						}
						break;
					default:;
						ch = char (ch);
				}
			}
			buffer[i] = char (ch);
			i ++;
		}
		buffer[i] = 0;
		m_regularExpressionIndex = 0;
		dNonDeterministicFiniteAutonata::PreProcessExpression (buffer);
	}


	void AddExpression (dString& expression, const dString& userAction, int lineNumber)
	{
		m_lineNumber = lineNumber;
		if (m_empty) {
			m_empty = false;
			CompileExpression(expression.GetStr());

			dExpandedState* const acceptingState = (dExpandedState*) m_acceptingState;
			dAssert (acceptingState->m_exitState);
			acceptingState->m_hasUserAction = true;
			acceptingState->m_userAction = userAction;

		} else {
			dAutomataState* const startState0 = m_startState;
			dAutomataState* const acceptingState0 = m_acceptingState;
			
			CompileExpression(expression.GetStr());
			dExpandedState* const acceptingState = (dExpandedState*) m_acceptingState;
			dAssert (acceptingState->m_exitState);
			acceptingState->m_hasUserAction = true;
			acceptingState->m_userAction = userAction;

			dAutomataState* const startState1 = m_startState;
			dAutomataState* const acceptingState1 = m_acceptingState;
			
			m_startState = CreateState (m_stateID ++);
			m_acceptingState = CreateState (m_stateID ++);

			m_startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), startState0));
			m_startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), startState1));
			acceptingState0->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), m_acceptingState));
			acceptingState1->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), m_acceptingState));

			//dTrace_NFA(("operator union\n"));
		}
	}

	bool m_empty;
	int m_lineNumber;
};


//dLexCompiler::dLexCompiler(const char* const inputRules, const char* const outputFileName)
dLexCompiler::dLexCompiler(const dString& inputRules, const char* const outputFileName)
	:m_token (m_end)
	,m_lineNumber(1)
	,m_grammarTokenStart(0)
	,m_grammarTokenLength(0)
	,m_grammar (inputRules.GetStr())
	,m_tokenList()
{
	dString userPreHeaderCode (""); 
	dString userPostHeaderCode ("\n"); 

	// convert specification file into one single giant non deterministic finite automaton
	dExpandedNFA nfa;
	ParseDefinitions (nfa, userPreHeaderCode, userPostHeaderCode);

	// convert nfa to Deterministic Finite Automaton
	dLexScannerGenerator dfa (nfa);

	// save header and source files
	dString className (GetClassName(outputFileName));
	dfa.CreateHeaderFile (outputFileName, className);
	dfa.CreateCodeFile (outputFileName, className, userPreHeaderCode, userPostHeaderCode); 
}

dLexCompiler::~dLexCompiler()
{
}

dString dLexCompiler::GetClassName(const char* const fileName) const
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




void dLexCompiler::NextToken ()
{
	m_grammarTokenStart += m_grammarTokenLength;
	
	if (m_grammar[m_grammarTokenStart]) {
		int lineNumber = m_lineNumber;
		for (bool reStart = true; reStart;) {
			reStart = false;
			for (dTokenDataList::dListNode* node = m_tokenList.GetFirst(); node; node = node->GetNext()) {
				dTokenData* const dTokenData = node->GetInfo();
				const char* const text = &m_grammar[m_grammarTokenStart];
				int count = dTokenData->FindMatch (text);
				if (count >= 0) {
					const char* const textstring = &m_grammar[m_grammarTokenStart];
					for (int i = 0; i < count ; i ++) {
						if (textstring[i] == '\n') {
							lineNumber ++;
						}
					}

					m_grammarTokenLength = count;
					if ((dTokenData->m_token == m_whiteSpace) || (dTokenData->m_token == m_comment)) {
						m_grammarTokenStart += m_grammarTokenLength;
						reStart = true;
						break;
					} else {
						m_lineNumber = lineNumber;
						m_token = dTokenData->m_token;
						return;
					}
				} 
			}
		}
	}
	m_token = m_end;
}





// DefinitionExpression	: DefinitionBlock 
// DefinitionExpression	: DefinitionBlock DefinitionExpression | nothing
void dLexCompiler::ParseDefinitionExpression (dString& preheaderCode)
{
	for (ParseDefinitionBlock (preheaderCode); (m_token != m_end) && (m_token != m_delimiter); ParseDefinitionBlock (preheaderCode));
}

// DefinitionBlock		: m_comment
void dLexCompiler::ParseDefinitionBlock (dString& preheaderCode)
{
	if (m_token == m_comment) {
		dAssert (0);
//		MatchToken (m_token);

	} else if (m_token == m_codeBlock) {
		dString code (&m_grammar[m_grammarTokenStart] + 2, m_grammarTokenLength - 4) ;
		//preheaderCode.append(code);
		preheaderCode += code;
		MatchToken (m_token);

	} else if (m_token == m_internalSize) {
		//_ASSERTE (0);
		MatchToken (m_token);
		MatchToken (m_token);

	} else if (m_token == m_literal) {
		dString literal (&m_grammar[m_grammarTokenStart], m_grammarTokenLength);
		MatchToken (m_token);
		
		dString extendedRegularExpression (&m_grammar[m_grammarTokenStart], m_grammarTokenLength);
		MatchToken (m_token);

		m_defintions.AddDefinition(extendedRegularExpression, literal);

	} else {
		dAssert (0);
	}
}


void dLexCompiler::MatchToken (dToken token)
{
	if (m_token == token) {
		NextToken ();
	} else {
		dAssert (0);
		dTrace(("parse error\n"));
		//		m_error = true;
	}
}


void dLexCompiler::ParseDefinitions (dExpandedNFA& nfa, dString& preHeaderCode, dString& postHeaderCode) 
{
	// parse definitions
	{
		m_tokenList.AddTokenData (m_whiteSpace, "[ \n\r\t\v\f]+");
		m_tokenList.AddTokenData (m_comment, "(/\\*([^*]|[\r\n]|(\\*+([^*/]|[\r\n])))*\\*+/)|(//.*)");
		m_tokenList.AddTokenData (m_codeBlock, "%\\{([^%]|[\r\n]|(%+([^%}]|[\r\n])))*%+\\}");
		m_tokenList.AddTokenData (m_literal, "[a-zA-Z_][0-9a-zA-Z]*");
		m_tokenList.AddTokenData (m_number, "[0-9]+");
		m_tokenList.AddTokenData (m_internalSize, "%[pneako]");
		m_tokenList.AddTokenData (m_delimiter, "%%");
		m_tokenList.AddTokenData (m_extendedRegularExpression, "((\\[[^\\]]+\\])|(\\[\\]\\])|[^ \n\r\t\v\f[]+)+");
		for (NextToken(); (m_token != m_end) && (m_token != m_delimiter);) {
			ParseDefinitionExpression (preHeaderCode);
		}
		m_tokenList.DeleteAll();
	}


	// parse rules

	//	int initialState = 0;
	{
		//	0	m_whiteSpace,				
		//	1   m_action
		//	2	m_comment,						
		//	3	m_delimiter,
		//	4	m_verbatingText,
		//	5	m_intenalSize,
		//	6	m_number,
		//	7	m_quatedString,
		//	8	m_literal,
		//	9	m_extendedRegularExpression,

		m_tokenList.AddTokenData (m_whiteSpace, "[ \n\r\t\v\f]+");
		m_tokenList.AddTokenData (m_quatedString, "\"[^\" \t\v\n\f]*\"");
		m_tokenList.AddTokenData (m_delimiter, "%%");
		m_tokenList.AddTokenData (m_comment, "(/\\*([^*]|[\r\n]|(\\*+([^*/]|[\r\n])))*\\*+/)|(//.*)");
		m_tokenList.AddTokenData (m_extendedRegularExpression, "((\\[[^\\]]+\\])|(\\[\\]\\])|[^ \n\r\t\v\f[]+)+");

		for (NextToken(); (m_token != m_end) && (m_token != m_delimiter); ) {

			int lineNumber = m_lineNumber;
			dString expression (&m_grammar[m_grammarTokenStart], m_grammarTokenLength);
			m_defintions.PreProcessDefinitions(expression);
			dToken expressionToken (m_token);

			// until I get the balance expression feature working
			m_grammarTokenStart += m_grammarTokenLength;
			const char* const strOuter = &m_grammar[m_grammarTokenStart];
			int lengthOuter = 0;
			for (int ch = strOuter[lengthOuter]; ch && (ch != '{') ; ch = strOuter[lengthOuter]) {
				if (ch == '\n') {
					m_lineNumber ++;
				}
				lengthOuter++;
			}

			if (strOuter[lengthOuter] == '{') {

				m_grammarTokenStart += lengthOuter;
				const char* const str = &m_grammar[m_grammarTokenStart];
				int length = 1;

				int count = 1;
				while (count) {
					char ch = str[length++];
					if (ch == '\n') {
						m_lineNumber ++;
					}

					if(ch == '{') {
						count ++;
					} else if (ch == '}') {
						count --;
					} else {
						if (ch == '\'')	{
							ch = str[length++];;
							if (ch == '\\') {
								ch = str[length++];
							}
							ch = str[length++];;
						} else if (ch == '\"') {
							for (ch = str[length++]; ch != '\"'; ch = str[length++]) {
								if (ch == '\\') {
									ch = str[length++];;
								}
							}
						}
					}
				}

				dString userAction (str + 1, length - 2);
				switch (expressionToken) 
				{
					case m_quatedString:
					{
						//dString keyword (expression.substr(1, expression.length() - 2));
						dString keyword (expression.SubString(1, expression.Size() - 2));
						nfa.AddExpression(keyword, userAction, lineNumber);
						break;
					}

					case m_extendedRegularExpression:
					{
						nfa.AddExpression(expression, userAction, lineNumber);
						//dTrace ((semanticActionCode.c_str()));

						break;
					}
					default:;
					dAssert (0);
				}

				m_grammarTokenStart += length;
				m_grammarTokenLength = 0;
			}

			NextToken();
		}
	}

	if (m_token == m_delimiter) {
		postHeaderCode = &m_grammar[m_grammarTokenStart + m_grammarTokenLength];
		postHeaderCode += "\n";
	}
}


