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
//Auto generated Parser Generator class: test6.h
//

#ifndef __test6_h__
#define __test6_h__

#ifdef _MSC_VER
#pragma warning (disable: 4702) // warning C4702: unreachable code
#pragma warning (disable: 4100) // warning C4100: unreferenced formal parameter
#pragma warning (disable: 4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif


#include <string>
#include <dList.h>
using namespace std;

class xxxx;

class test6
{
	public:
	enum dToken
	{
		ACCEPTING_TOKEN = 254, 
		ERROR_TOKEN = 255, 
		LITERAL_IDENTIFIER = 256,
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
		int m_scannerLine;
		int m_scannerIndex;
		dToken m_token;
		string m_data;
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



	test6();
	virtual ~test6();
	virtual bool Parse(xxxx& scanner);

	private:
	const dGotoEntry* FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const;
	const dActionEntry* FindAction (const dActionEntry* const list, int count, dToken token) const;
	const dActionEntry* GetNextAction (dList<dStackPair>& stack, dToken token, xxxx& scanner) const;

	bool m_grammarError;
};

#endif
