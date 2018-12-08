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
//Auto generated Lexical Analyzer class: dParcerLexical.h
//

#ifndef __dParcerLexical_h__
#define __dParcerLexical_h__

#ifdef _MSC_VER
#pragma warning (disable: 4702) // warning C4702: unreachable code
#pragma warning (disable: 4100) // warning C4100: unreferenced formal parameter
#pragma warning (disable: 4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif


#include <string>
using namespace std;

class dParcerLexical
{
	public:
	dParcerLexical(const char* const data);
	virtual ~dParcerLexical();

	virtual int NextToken ();

	const char* GetTokenString() const
	{
		return m_tokenString.c_str();
	}

	const char* GetNextBuffer() const
	{
		return &m_data[m_index];
	}

	protected:
	int GetLineNumber () const
	{
		return m_lineNumber;
	}

	char NextChar ()
	{
		char ch = m_data[m_index++];
		if (ch == '\n') {
			m_lineNumber ++;
		}
		return ch;
	}

	void UnGetChar ()
	{
		char ch = m_data[--m_index];
		if (ch == '\n') {
			m_lineNumber --;
		}
	}

	void ReadBalancedExpresion (char open, char close);

	void GetLexString ();
	int GetNextStateIndex (char symbol, int count, const char* const sharacterSet) const;
//	bool IsCharInSet (char ch, const char* const set, int setSize) const;

	// local lexical variables
	string m_tokenString;
	const char* m_data;
	int m_index;
	int m_startIndex;
	int m_lineNumber;
};
#endif
