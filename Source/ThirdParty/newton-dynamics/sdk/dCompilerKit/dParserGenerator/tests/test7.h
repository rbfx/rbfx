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

//
//Auto generated Parser Generator class: test7.h
//

#ifndef __test7_h__
#define __test7_h__

#ifdef _MSC_VER
#pragma warning (disable: 4702) // warning C4702: unreachable code
#pragma warning (disable: 4100) // warning C4100: unreferenced formal parameter
#pragma warning (disable: 4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif


#include <string>
#include <dList.h>
using namespace std;

class xxxx;

class test7
{
	public:
	enum dToken
	{
		ACCEPTING_TOKEN = 254, 
		ERROR_TOKEN = 255, 
		_INT = 256, 
		_CHAR,
		_VOID,
		_CONST,
		_SHORT,
		_UNSIGNED,
		_FLOAT,
		_DOUBLE,
		_STRUCT,
		_UNION,
		_TYPEDEF,
		_IFDEF,
		_IFNDEF,
		_ELSE,
		_ENDIF,
		_DEFINE,
		_INCLUDE,
		_NUMERIC_CONSTANT,
		_LITERAL_IDENTIFIER,
		_QUOTED_CONSTANT
	};

	enum ActionType;
	class dStackPair;
	class dGotoEntry;
	class dActionEntry;
	class dDefualtUserVariable
	{
		public:
		dDefualtUserVariable () 
			:m_scannerLine (0), m_scannerIndex(0), m_token (dToken (0)), m_data("")
		{
		}

		dDefualtUserVariable (dToken token, const char* const data, int scannerLine, int scannerIndex)
			:m_scannerLine (scannerLine), m_scannerIndex(scannerIndex), m_token(token), m_data (data) 
		{
		}

		dToken GetToken() const 
		{
			return m_token;
		}

		const string& GetString() const 
		{
			return m_data;
		}

		//protected:
		int m_scannerLine;
		int m_scannerIndex;
		dToken m_token;
		string m_data;
		//friend test7;
	};


	class dUserVariable: public dDefualtUserVariable
	{
		public:
		dUserVariable () 
			:dDefualtUserVariable ()
		{
		}

		dUserVariable (dToken token, const char* const data, int scannerLine, int scannerIndex)
			:dDefualtUserVariable  (token, data, scannerLine, scannerIndex)
		{
		}
	};



	test7();
	virtual ~test7();
	virtual bool Parse(xxxx& scanner);

	private:
	const dGotoEntry* FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const;
	const dActionEntry* FindAction (const dActionEntry* const list, int count, dToken token) const;
	const dActionEntry* GetNextAction (dList<dStackPair>& stack, dToken token, xxxx& scanner) const;

	bool m_grammarError;
};

#endif
