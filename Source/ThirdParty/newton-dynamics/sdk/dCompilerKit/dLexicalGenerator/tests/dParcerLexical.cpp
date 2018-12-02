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
//Auto generated Lexical Analyzer class: dParcerLexical.cpp
//

#include <dParcerCompiler.h>

#include "dParcerLexical.h"


dParcerLexical::dParcerLexical(const char* const data)
	:m_tokenString ("")
	,m_data(data)
	,m_index(0)
	,m_startIndex(0)
	,m_lineNumber(0)
{
}

dParcerLexical::~dParcerLexical()
{
}


void dParcerLexical::ReadBalancedExpresion (char open, char close)
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


void dParcerLexical::GetLexString ()
{
	int length = m_index - m_startIndex;
	m_tokenString = string (&m_data[m_startIndex], length);
	m_startIndex = m_index;
}


int dParcerLexical::GetNextStateIndex (char symbol, int count, const char* const characterSet) const
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

int dParcerLexical::NextToken ()
{
	static short transitionsCount[] = {
			64, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 63, 0, 0, 1, 25, 1, 1, 1, 1, 1, 
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 127, 127, 127, 
			127, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	static short transitionsStart[] = {
			0, 64, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 
			68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 131, 68, 68, 194, 195, 220, 221, 222, 223, 224, 
			225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 252, 379, 506, 
			633, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778};
	static short nextStateSet[] = {
			1, 1, 1, 1, 68, 42, 41, 35, 36, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 
			38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 
			38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 39, 40, 1, 1, 1, 1, 37, 37, 37, 37, 
			37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 
			37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 
			37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 
			37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 
			37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 
			37, 37, 34, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 
			46, 45, 44, 43, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 
			13, 12, 11, 10, 9, 2, 89, 85, 81, 77, 73, 69, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			70, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 71, 71, 71, 71, 71, 
			71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 
			71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 
			71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 
			71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 
			71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 8, 
			71, 71, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 70, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 70, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 
			72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 74, 75, 76, 7, 78, 79, 80, 6, 
			82, 83, 84, 5, 86, 87, 88, 4, 90, 91, 3};
	static char  nextCharacterSet[] = {
			9, 10, 13, 32, 37, 39, 47, 58, 59, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
			80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 95, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 
			109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 9, 10, 13, 32, 48, 49, 50, 51, 
			52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
			83, 84, 85, 86, 87, 88, 89, 90, 95, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 
			112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 
			68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 95, 
			97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 
			121, 122, 42, 33, 37, 38, 40, 41, 42, 43, 44, 45, 46, 47, 58, 59, 60, 61, 62, 63, 91, 92, 93, 94, 
			123, 124, 125, 126, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 
			39, 39, 39, 39, 39, 37, 108, 114, 115, 116, 117, 123, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
			13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
			37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 
			61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 
			85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 
			109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 1, 2, 3, 4, 5, 
			6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
			30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 
			54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 
			78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 
			102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 
			126, 127, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
			23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 
			47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 
			71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 
			95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 
			119, 120, 121, 122, 123, 124, 125, 126, 127, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
			40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 
			64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 
			88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 
			112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 110, 105, 111, 110, 111, 107, 101, 110, 
			116, 97, 114, 116, 105, 103, 104, 116, 101, 102, 116};
	
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
					{}
					state = 0;
					ch = NextChar();
					break;
				}
				case 2:
				{
					GetLexString ();
					{ return dParcerCompiler::GRAMMAR_SEGMENT;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 3:
				{
					GetLexString ();
					{ return dParcerCompiler::LEFT;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 4:
				{
					GetLexString ();
					{ return dParcerCompiler::RIGHT;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 5:
				{
					GetLexString ();
					{ return dParcerCompiler::START;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 6:
				{
					GetLexString ();
					{ return dParcerCompiler::TOKEN;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 7:
				{
					GetLexString ();
					{ return dParcerCompiler::UNION;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 8:
				{
					GetLexString ();
					{ m_tokenString.replace(0, 2, ""); m_tokenString.replace(m_tokenString.size() - 2, 2, ""); return dParcerCompiler::CODE_BLOCK;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 9:
				{
					GetLexString ();
					{ m_tokenString = "!"; return('!'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 10:
				{
					GetLexString ();
					{ m_tokenString = "%"; return('%'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 11:
				{
					GetLexString ();
					{ m_tokenString = "&"; return('&'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 12:
				{
					GetLexString ();
					{ m_tokenString = "("; return('('); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 13:
				{
					GetLexString ();
					{ m_tokenString = ")"; return(')'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 14:
				{
					GetLexString ();
					{ m_tokenString = "*"; return('*'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 15:
				{
					GetLexString ();
					{ m_tokenString = "+"; return('+'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 16:
				{
					GetLexString ();
					{ m_tokenString = ","; return(','); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 17:
				{
					GetLexString ();
					{ m_tokenString = "-"; return('-'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 18:
				{
					GetLexString ();
					{ m_tokenString = "."; return('.'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 19:
				{
					GetLexString ();
					{ m_tokenString = "/"; return('/'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 20:
				{
					GetLexString ();
					{ m_tokenString = ":"; return(':'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 21:
				{
					GetLexString ();
					{ m_tokenString = ";"; return(';'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 22:
				{
					GetLexString ();
					{ m_tokenString = "<"; return('<'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 23:
				{
					GetLexString ();
					{ m_tokenString = "="; return('='); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 24:
				{
					GetLexString ();
					{ m_tokenString = ">"; return('>'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 25:
				{
					GetLexString ();
					{ m_tokenString = "?"; return('?'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 26:
				{
					GetLexString ();
					{ m_tokenString = "["; return('['); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 27:
				{
					GetLexString ();
					{ m_tokenString = "\\"; return('\\'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 28:
				{
					GetLexString ();
					{ m_tokenString = "]"; return(']'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 29:
				{
					GetLexString ();
					{ m_tokenString = "^"; return('^'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 30:
				{
					GetLexString ();
					{ m_tokenString = "{"; return('{'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 31:
				{
					GetLexString ();
					{ m_tokenString = "|"; return('|'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 32:
				{
					GetLexString ();
					{ m_tokenString = "}"; return('}'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 33:
				{
					GetLexString ();
					{ m_tokenString = "~"; return('~'); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 34:
				{
					GetLexString ();
					{}
					state = 0;
					ch = NextChar();
					break;
				}
				case 35:
				{
					GetLexString ();
					{ return(dParcerCompiler::COLOM); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 36:
				{
					GetLexString ();
					{ return(dParcerCompiler::SIMICOLOM); }
					state = 0;
					ch = NextChar();
					break;
				}
				case 37:
				{
					GetLexString ();
					{ return dParcerCompiler::LITERAL;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 38:
				{
					GetLexString ();
					{ return dParcerCompiler::LITERAL;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 39:
				{
					GetLexString ();
					{ ReadBalancedExpresion ('{', '}'); return dParcerCompiler::USER_ACTION;}
					state = 0;
					ch = NextChar();
					break;
				}
				case 40:
				{
					GetLexString ();
					{ return(dParcerCompiler::OR); }
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


