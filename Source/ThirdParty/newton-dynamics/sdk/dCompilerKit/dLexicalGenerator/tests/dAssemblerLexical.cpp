 /* Copych1 (c) <2009> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

//
//Auto generated Lexical Analyzer class: dAssemblerLexical.cpp
//

#include <dVirtualMachine.h>
#include "dAssemblerParser.h"

//
// Newton virtual machine assembler grammar
// based loosely on the MIPS R3000 and the Intel 386 instructions sets 
//

#include "dAssemblerLexical.h"


dAssemblerLexical::dAssemblerLexical(const char* const data)
	:m_tokenString ("")
	,m_data(data)
	,m_index(0)
	,m_startIndex(0)
	,m_lineNumber(0)
{
}

dAssemblerLexical::~dAssemblerLexical()
{
}


void dAssemblerLexical::ReadBalancedExpresion (char open, char close)
{
	int count = 1;
	while (count) {
		int ch = NextChar();
		if (ch == '\n') {
			m_lineNumber ++;
		}

		if(ch == open) {
			count ++;
		} else if (ch == close) {
			count --;
		} else {
			if (ch == '\'')	{
				ch = NextChar();
				if (ch == '\\') {
					ch = NextChar();
				}
				ch = NextChar();
			} else if (ch == '\"') {
				for (ch = NextChar(); ch != '\"'; ch = NextChar()) {
					if (ch == '\\') {
						ch = NextChar();
					}
				}
			}
		}
	}

	string tmp (m_tokenString);
	GetLexString();
	m_tokenString = tmp + m_tokenString;
}


void dAssemblerLexical::GetLexString ()
{
	int length = m_index - m_startIndex;
	m_tokenString = string (&m_data[m_startIndex], length);
	m_startIndex = m_index;
}


int dAssemblerLexical::GetNextStateIndex (char symbol, int count, const char* const characterSet) const
{
	int i0 = 0;
	int i1 = count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;
		if (symbol <= characterSet[i]) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		if (symbol == characterSet[i]) {
			return i;
		}
	}
	return -1;
}

int dAssemblerLexical::NextToken ()
{
	static short transitionsCount[] = {
			2, 0, 0};
	static short transitionsStart[] = {
			0, 2, 2};
	static short nextStateSet[] = {
			1, 2};
	static char  nextCharacterSet[] = {
			46, 93};
	
	m_startIndex = m_index;

	int state = 0;
	int zeroCount = 2;
	char ch = NextChar();
	do {
		int transCount = transitionsCount[state];
		int tranStart = transitionsStart[state];
		int nextStateIndex = GetNextStateIndex (ch, transCount, &nextCharacterSet[tranStart]);
		if (nextStateIndex >= 0) {
			ch = NextChar();
			short* const stateArray = &nextStateSet[tranStart];
			state = stateArray[nextStateIndex];
		} else {
			UnGetChar ();
			switch (state) 
			{
				case 1:
				{
					GetLexString ();
					{return '.';}
					state = 0;
					ch = NextChar();
					break;
				}
				case 2:
				{
					GetLexString ();
					{return ']';}
					state = 0;
					ch = NextChar();
					break;
				}


				default:
				{
					// Lexical error
					return -1;
				}
			}
		}
		if (!ch) {
			zeroCount--;
		}
	} while (zeroCount);
	// Unknown pattern
	return -1;
}


