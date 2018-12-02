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

#include "dFiniteAutomata.h"
#include "dAutomataState.h"
#include "dNonDeterministicFiniteAutonata.h"

//#define dTrace_NFA(x) dTrace(x)
#define dTrace_NFA(x) 

/*
char dNonDeterministicFiniteAutonata::m_asciiSet[] = 
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'W', 'V', 'X', 'Y', 'Z',  
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'w', 'v', 'x', 'y', 'z',  
	'@', '{', '|', '}', '[', ']', '^', '_', ':', ';', '<', '=', '>', '?',
	' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
	'\\', '\"', '\'', 
};
*/

char dNonDeterministicFiniteAutonata::m_asciiSet[] = {       1,  2,  3,  4,  5,  6,  7,  8,  9, 
														10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
														20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
														30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
														40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
														50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 
														60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
														70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
														80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
														90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
														100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 
														110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
														120, 121, 122, 123, 124, 125, 126, 127, 0};

char dNonDeterministicFiniteAutonata::m_asciiSetButNewLine[] = {  1,  2,  3,  4,  5,  6,  7,  8,  9, 
															     11, 12, 13, 14, 15, 16, 17, 18, 19, 
																20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
																30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
																40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
																50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 
																60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
																70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
																80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
																90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
																100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 
																110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
																120, 121, 122, 123, 124, 125, 126, 127, 0};


enum dNonDeterministicFiniteAutonata::Operations
{
	m_union = '|',						// a|b	
	m_concatenation = 0x7f,				// ab
	m_zeroOrMore = '*',					// a* 
	m_oneOrMore  = '+',					// a+ 
	m_zeroOrOne  = '?',					// a? 
	m_openParentesis = '(',				// (a)
	m_closeParentesis = ')',			// (a)  
	m_openSquareBrakect = '[',			// [a] 
	m_closeSquareBrakect = ']',			// [a] 
};

dNonDeterministicFiniteAutonata::dStateConstructPair::dStateConstructPair ()
{
}

dNonDeterministicFiniteAutonata::dStateConstructPair::dStateConstructPair (dAutomataState* start, dAutomataState* accepting)
	:m_start(start)
	,m_accepting(accepting)
{
}

dAutomataState* dNonDeterministicFiniteAutonata::dStateConstructPair::GetStart() const 
{ 
	return m_start;
}

dAutomataState* dNonDeterministicFiniteAutonata::dStateConstructPair::GetAccepting() const 
{ 
	return m_accepting;
}


dNonDeterministicFiniteAutonata::dAutomataStateConstructStack::dAutomataStateConstructStack ()
	:m_index(0)
{
}

bool dNonDeterministicFiniteAutonata::dAutomataStateConstructStack::IsEmpty() const
{
	return m_index == 0;
}

dNonDeterministicFiniteAutonata::dStateConstructPair dNonDeterministicFiniteAutonata::dAutomataStateConstructStack::Pop ()
{
	dAssert (m_index);
	return m_pool[--m_index];
}

void dNonDeterministicFiniteAutonata::dAutomataStateConstructStack::Push (dAutomataState* const start, dAutomataState* const accepting)
{
	m_pool[m_index++] = dStateConstructPair (start, accepting);
	dAssert (m_index <= sizeof (m_pool)/sizeof (m_pool[0]));
}







dNonDeterministicFiniteAutonata::dNonDeterministicFiniteAutonata()
	:dFiniteAutomata()
	,m_error(true)
	,m_token (0)
	,m_stateID (0)
	,m_regularExpressionIndex(0)
	,m_startState(NULL) 
	,m_acceptingState(NULL) 
	,m_stack()
{
}

dNonDeterministicFiniteAutonata::dNonDeterministicFiniteAutonata(const char* const regularExpression)
	:dFiniteAutomata()
	,m_error(false)
	,m_token (0)
	,m_stateID (0)
	,m_regularExpressionIndex(0)
	,m_startState(NULL) 
	,m_acceptingState(NULL) 
	,m_stack()
{
	CompileExpression(regularExpression);
}

dNonDeterministicFiniteAutonata::~dNonDeterministicFiniteAutonata(void)
{
	if (m_startState) {
		DeleteNFA (m_startState);
	}
}



dAutomataState* dNonDeterministicFiniteAutonata::GetStartState() const
{
	return m_startState;
}

dAutomataState* dNonDeterministicFiniteAutonata::GetExitState() const
{
	return m_acceptingState;
}


bool dNonDeterministicFiniteAutonata::IsValid() const
{
	return !m_error;
}

const dChatertSetMap& dNonDeterministicFiniteAutonata::GetChatertSetMap() const
{
	return m_charaterSetMap;
}

void dNonDeterministicFiniteAutonata::CompileExpression(const char* const regularExpression)
{
	// prepossess the expression for simples parsing 
	m_regularExpressionIndex = 0;
	PreProcessExpression (regularExpression);

	// build an NFA graph
	dTrace_NFA(("\n"));
	dTrace_NFA(("Expression: %s\n", regularExpression));

	ParseExpressionToNFA ();
}


void dNonDeterministicFiniteAutonata::DeleteNFA (dAutomataState* const startState)
{
	dList<dAutomataState*> statesList;
	startState->GetStateArray (statesList);
	for (dList<dAutomataState*>::dListNode* node = statesList.GetFirst(); node; node = node->GetNext()) {
		dAutomataState* const state = node->GetInfo();
		delete state;
	}
	m_startState = NULL;
	m_acceptingState = NULL;
}

// Get the next character from the expression
// it should handle scape characters, and concatenations
int dNonDeterministicFiniteAutonata::GetChar()
{
	int ch = m_regularExpression[m_regularExpressionIndex];
	if (ch) {
		m_regularExpressionIndex ++;
		if (ch == '\\') {
			ch = (ch << 8) + m_regularExpression[m_regularExpressionIndex ++];
		} 
	}
	return ch;
}


bool dNonDeterministicFiniteAutonata::CheckInsertConcatenation (int left, int right) const
{
	bool test = (((!IsOperator(left)) || (left == m_closeParentesis) || (left == m_closeSquareBrakect) || (left == m_zeroOrMore) || (left == m_oneOrMore) || (left == m_zeroOrOne)) && 
		         ((!IsOperator(right))|| (right == m_openParentesis)  || (right == m_openSquareBrakect))); 
	return test;
}

void dNonDeterministicFiniteAutonata::PreProcessExpression (const char* const regularExpression)
{
	dAssert (sizeof (m_regularExpression) > strlen (regularExpression));
	sprintf (m_regularExpression, "%s", regularExpression);

	char buffer[2048];
	int ch0 = GetChar();
	int count = 0;
	for (int ch1 = GetChar(); ch1; ch1 = GetChar()) {
		if (ch0 > 256) {
			buffer[count ++] = char (ch0>>8);
			dAssert (count < sizeof (buffer));
		}

		buffer[count ++] = char (ch0);
		dAssert (count < sizeof (buffer));

//		#ifdef _DEBUG
//		bool test0 = CheckInsertConcatenation (ch0, ch1);
//		bool test1 = (((!IsOperator(ch0)) || (ch0 == m_closeParentesis) || (ch0 == m_closeSquareBrakect) || (ch0 == m_zeroOrMore) || (ch0 == m_oneOrMore) || (ch0 == m_zeroOrOne)) && 
//					 ((!IsOperator(ch1)) || (ch1 == m_openParentesis)  || (ch1 == m_openSquareBrakect)));
//		_ASSERTE (test0 == test1);
//		#endif

		if (CheckInsertConcatenation (ch0, ch1)) {
			buffer[count ++] = char (m_concatenation);
			dAssert (count < sizeof (buffer));
		}
		ch0 = ch1;
	}
	if (ch0 > 256) {
		buffer[count ++] = char (ch0 >> 8);
		dAssert (count < sizeof (buffer));
	}
	buffer[count ++] = char (ch0);
	dAssert (count < sizeof (buffer));
	buffer[count ++] = 0;
	dAssert (count < sizeof (buffer));

	m_regularExpressionIndex = 0;
	sprintf (m_regularExpression, "%s", buffer);
}

bool dNonDeterministicFiniteAutonata::IsOperator (int character) const
{
	static char operation[] = {m_union, char (m_concatenation), m_zeroOrMore, m_oneOrMore, m_zeroOrOne, 
							   m_openParentesis, m_closeParentesis, m_openSquareBrakect, m_closeSquareBrakect};
	for (int i = 0; i < sizeof (operation) / sizeof (operation[0]); i ++) {
		if (character == operation[i]) {
			return true;
		}
	}
	return false;
}

void dNonDeterministicFiniteAutonata::Match (int token)
{
	if (m_token == token) {
		m_token = GetChar();
	} else {
		dTrace_NFA(("parse error\n"));
		m_error = true;
	}
}


void dNonDeterministicFiniteAutonata::PushId (int charater)
{
	dAutomataState* const startState = CreateState (m_stateID ++);
	dAutomataState* const acceptingState = CreateState (m_stateID ++);

	dAutomataState::dCharacter charInfo (GetScapeChar (charater), dAutomataState::CHARACTER);
	startState->m_transtions.Append(dAutomataState::dTransition(charInfo, acceptingState));

	m_stack.Push(startState, acceptingState);

	dTrace_NFA(("Push ", charater));
	if (charater > 256) {
		dTrace_NFA(("\\"));
	}
	dTrace_NFA(("%c\n", charater));
}

void dNonDeterministicFiniteAutonata::PushSet (const char* const set, int size)
{
	dAutomataState* const startState = CreateState (m_stateID ++);
	dAutomataState* const acceptingState = CreateState (m_stateID ++);
	
	int setId = m_charaterSetMap.AddSet(set, size);
	dAutomataState::dCharacter charInfo (setId, dAutomataState::CHARACTER_SET);
	startState->m_transtions.Append(dAutomataState::dTransition(charInfo, acceptingState));

	m_stack.Push(startState, acceptingState);

	dTrace_NFA(("Push charSet_%d\n", setId));
}



void dNonDeterministicFiniteAutonata::ReduceUnionDiagram()
{
	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		return;
	}
	dStateConstructPair rightOperand (m_stack.Pop());

	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		m_stack.Push(rightOperand.GetStart(), rightOperand.GetAccepting());
		return;
	}
	dStateConstructPair leftOperand (m_stack.Pop());

	dAutomataState* const startState = CreateState (m_stateID ++);
	dAutomataState* const acceptingState = CreateState (m_stateID ++);

	startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), leftOperand.GetStart()));
	startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), rightOperand.GetStart()));
	leftOperand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), acceptingState));
	rightOperand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), acceptingState));

	m_stack.Push(startState, acceptingState);
	dTrace_NFA(("operator union\n"));
}


void dNonDeterministicFiniteAutonata::ReduceConcatenationDiagram()
{
	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		return;
	}
	dStateConstructPair rightOperand (m_stack.Pop());

	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		m_stack.Push(rightOperand.GetStart(), rightOperand.GetAccepting());
		return;
	}
	dStateConstructPair leftOperand (m_stack.Pop());

	leftOperand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), rightOperand.GetStart()));

	m_stack.Push(leftOperand.GetStart(), rightOperand.GetAccepting());
	dTrace_NFA(("operator concatenation\n"));
}

void dNonDeterministicFiniteAutonata::ReduceZeroOrMoreDiagram()
{
	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		return;
	}
	dStateConstructPair operand (m_stack.Pop());

	dAutomataState* const startState = CreateState (m_stateID ++);
	dAutomataState* const acceptingState = CreateState (m_stateID ++);

	startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), acceptingState));
	startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), operand.GetStart()));
	operand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), acceptingState));
	operand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), operand.GetStart()));

	m_stack.Push(startState, acceptingState);

	dTrace_NFA(("operator zeroOrMore\n"));
}

void dNonDeterministicFiniteAutonata::ReduceOneOrMoreDiagram()
{
	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		return;
	}
	dStateConstructPair operand (m_stack.Pop());

	dAutomataState* const startState = CreateState (m_stateID ++);
	dAutomataState* const acceptingState = CreateState (m_stateID ++);

	startState->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), operand.GetStart()));
	operand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), acceptingState));
	operand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), operand.GetStart()));

	m_stack.Push(startState, acceptingState);

	dTrace_NFA(("operator oneOrMore\n"));
}



void dNonDeterministicFiniteAutonata::ReduceZeroOrOneDiagram()
{
	if (m_stack.IsEmpty()) {
		m_error = true;
		dTrace_NFA(("Parce stack underflow error\n"));
		return;
	}
	dStateConstructPair operand (m_stack.Pop());

	dAutomataState* const start = CreateState (m_stateID ++);
	dAutomataState* const accepting = CreateState (m_stateID ++);

	start->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), accepting));
	start->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), operand.GetStart()));
	operand.GetAccepting()->m_transtions.Append(dAutomataState::dTransition(dAutomataState::dCharacter(), accepting));

	m_stack.Push(start, accepting);

	dTrace_NFA(("operator zeroOrOne\n"));
}


// UnionExpression				: ConcatenateExpression MoreUnionExpression
// MoreUnionExpression			: m_union ConcatenateExpression MoreUnionExpression | nothing
void dNonDeterministicFiniteAutonata::UnionExpression ()
{
	ConcatenationExpression ();
	for (;;) {
		if (m_token == m_union) {
			Match (m_union);
			ConcatenationExpression ();

			ReduceUnionDiagram();

		} else {
			break;
		}
	}
}


// ConcatenationExpression		: UnuaryExpression MoreConcatenationExpression
// MoreConcatenationExpression	: m_concatenation UnuaryExpression MoreConcatenationExpression | nothing
void dNonDeterministicFiniteAutonata::ConcatenationExpression ()
{
	UnuaryExpression ();
	for (;;) {
		if (m_token == m_concatenation) {
			Match (m_concatenation);
			UnuaryExpression ();

			ReduceConcatenationDiagram();
		} else {
			break;
		}
	}
}

// UnuaryExpression : m_zeroOrMore | m_oneOrMore | m_zeroOrOne | BracketedExpression | nothing
void dNonDeterministicFiniteAutonata::UnuaryExpression ()
{
	ShiftID ();
	if (m_token == m_zeroOrMore) {
		Match (m_zeroOrMore);
		ReduceZeroOrMoreDiagram ();

	} else if (m_token == m_oneOrMore) {
		Match (m_oneOrMore);

		ReduceOneOrMoreDiagram ();

	} else if (m_token == m_zeroOrOne) {
		Match (m_zeroOrOne);

		ReduceZeroOrOneDiagram ();
	}
}

// BracketedExpression  : '^' BracketedExpression
// BracketedExpression  : '^' m_concatenation BracketedExpression
// BracketedExpression  : CHARACTER m_concatenation BracketedExpression
// BracketedExpression  : CHARACTER  m_concatenation '-' CHARACTER
// BracketedExpression  : CHARACTER
// BracketedExpression  : nothing
int dNonDeterministicFiniteAutonata::BracketedExpression (char* const set, int size)
{
	if (m_token != ']') {
		if (m_token == '^') {
			Match ('^');
			if (m_token == m_concatenation) {
				Match (m_concatenation);
			}
			char exclusiveAsciiSet[D_ASCII_SET_MAX_SIZE];
			int exclusiveSize = BracketedExpression (exclusiveAsciiSet, 0);

			for (int i = 0; i < m_asciiSet[i]; i ++) {
				char ch = m_asciiSet[i];
				int j = 0;
				for (; j < exclusiveSize; j ++) {
					if (ch == exclusiveAsciiSet[j]) {
						break;
					}
				}
				if (j == exclusiveSize) {
					set[size] = ch;
					size ++;
					dAssert (size < D_ASCII_SET_MAX_SIZE);
				}
			}

		} else {
			int ch = m_token;
			Match (m_token);
			if (m_token == m_concatenation) {
				Match (m_token);
				if (m_token == '-') {
					Match (m_token);
					Match (m_token);

					int ch1 = m_token;
					for (int i = ch; i <= ch1; i ++) {
						set[size] = char (i);
						size ++;			
					}

					Match (m_token);
					if (m_token == m_concatenation) {
						Match (m_token);
					}
					size = BracketedExpression (set, size);
				

				} else {
					if ((ch>>8) == '\\') {
						ch = GetScapeChar (ch);
					}
					set[size] = char (ch);
					size ++;
					dAssert (size < D_ASCII_SET_MAX_SIZE);
					size = BracketedExpression (set, size);
				}

			} else {
				if ((ch>>8) == '\\') {
					ch = GetScapeChar (ch);
				}
				set[size] = char (ch);
				size ++;
				dAssert (size < D_ASCII_SET_MAX_SIZE);
			}
		}
	}

	return size;
}

// id	: m_openSquareBrakect BracketedExpression m_closeSquareBrakect
// id	: m_openParentesis UnionExpression m_closeParentesis
// id	: m_balancedCharacterExpression CHARACTER UnionExpression m_balancedCharacterExpression CHARACTER
// id	: .
// id	: CHARACTER
void dNonDeterministicFiniteAutonata::ShiftID()
{
	if (m_token == m_openParentesis) {
		Match (m_openParentesis);
		UnionExpression ();
		Match (m_closeParentesis);

	} else if (m_token == m_openSquareBrakect) {
		char asciiSet[D_ASCII_SET_MAX_SIZE];
		Match (m_openSquareBrakect);
		int size = BracketedExpression (asciiSet, 0);
		dAssert (size < D_ASCII_SET_MAX_SIZE);
		asciiSet[size] = 0;


		PushSet (asciiSet, size);
		Match (m_closeSquareBrakect);

	} else if (m_token == '.') {
		PushSet (m_asciiSetButNewLine, int (strlen (m_asciiSetButNewLine)));

		Match (m_token);

	} else if (!IsOperator (m_token)) {
		PushId (m_token);

		Match (m_token);
	} else {
		Match (m_token);
		dTrace_NFA(("parse error\n"));
	}
}

// Thompson's algorithm for constructing a Nondeterministic Finite Automaton (NFA) from a Basic Regular Expressions (BRE)
// algorithm from book Compilers Principles and Tools: by Alfred V.Aho, Ravi Sethi and Jeffrey D. Ullman 
//
// Productions Rule for a top down grammar, stack based Regular expression grammar
//
// UnionExpression				: ConcatenateExpression MoreUnionExpression
// MoreUnionExpression			: m_union ConcatenateExpression MoreUnionExpression | nothing
//
// ConcatenationExpression		: UnuaryExpression MoreConcatenationExpression
// MoreConcatenationExpression	: m_concatenation UnuaryExpression MoreConcatenationExpression  | nothing
//
// UnuaryExpression				: m_zeroOrMore | m_oneOrMore | m_zeroOrOne| nothing
//
// BracketedExpression  		: '^' BracketedExpression
// BracketedExpression  		: '^' m_concatenation BracketedExpression
// BracketedExpression  		: CHARACTER m_concatenation BracketedExpression
// BracketedExpression  		: CHARACTER  m_concatenation '-' CHARACTER
// BracketedExpression  		: CHARACTER
// BracketedExpression  		: nothing
//
// id							: m_openSquareBrakect BracketedExpression m_closeSquareBrakect
// id							: m_openParentesis UnionExpression m_closeParentesis
// id							: .
// id							: CHARACTER 
void dNonDeterministicFiniteAutonata::ParseExpressionToNFA ()
{
	m_error = false;
	for (m_token = GetChar(); m_token;) {
		UnionExpression ();
	}

	if (m_error) {
		dAssert (0);
		while (!m_stack.IsEmpty()) {
			dStateConstructPair operand (m_stack.Pop());
			DeleteNFA (operand.GetStart());
		}
	} else {
		dStateConstructPair operand (m_stack.Pop());
		m_startState = operand.GetStart(); 
		m_acceptingState = operand.GetAccepting();
		m_acceptingState->m_exitState = true;
	}
}




int dNonDeterministicFiniteAutonata::SortStates (const void *ptr0, const void *ptr1)
{
	dAutomataState* const state0 = *(dAutomataState**) ptr0;
	dAutomataState* const state1 = *(dAutomataState**) ptr1;

	if (state0->m_exitState == state1->m_exitState) {
		return 0;
	}
	if (state0->m_exitState) {
		return -1;
	}
	return 1;
}

