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
// Auto generated Parser Generator class: dAssemblerParser.h
//

#ifndef __dAssemblerParser_h__
#define __dAssemblerParser_h__

#ifdef _MSC_VER
#pragma warning (disable: 4702) // warning C4702: unreachable code
#pragma warning (disable: 4100) // warning C4100: unreferenced formal parameter
#pragma warning (disable: 4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif


#include <dList.h>
#include <dString.h>

class dAssemblerLexical;

class dAssemblerParser
{
	public:
	enum dToken
	{
		ACCEPTING_TOKEN = 254, 
		ERROR_TOKEN = 255, 
		IMPORT = 256, 
		IMPORT_FILENAME,
		LITERAL,
		PRIVATE,
		BEGIN,
		END,
		INT,
		INTEGER,
		REGISTER,
		MOVE,
		LOADB,
		LOADW,
		LOADD,
		STOREB,
		STOREW,
		STORED,
		SLL,
		SRL,
		AND,
		OR,
		XOR,
		NOT,
		ADDI,
		ADD,
		SUB,
		MUL,
		DIV,
		ABS,
		NEG,
		BEQ,
		BNE,
		BLT,
		BLE,
		BGT,
		BGE,
		CALL,
		CALLR,
		RET,
		SYSCALL,
		JUMP,
		JUMPR,
		ENTER,
		EXIT,
		PUSH,
		POP,
		LOCALLABEL
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

		const dString& GetString() const 
		{
			return m_data;
		}

		//protected:
		int m_scannerLine;
		int m_scannerIndex;
		dToken m_token;
		dString m_data;
		//friend dAssemblerParser;
	};



	class dUserVariable: public dDefualtUserVariable
	{
		public:
		dUserVariable () 
			:dDefualtUserVariable ()
		{
		}
		
//		dUserVariable (dToken token, const char* const text, int scannerLine, int scannerIndex)
		dUserVariable (dToken token, const char* const text, int scannerLine = 0, int scannerIndex = 0)
			:dDefualtUserVariable (token, text, scannerLine, scannerIndex)
			,m_semanticValue(0)
		{
		}

		int m_semanticValue;
	};


	dAssemblerParser();
	virtual ~dAssemblerParser();
	virtual bool Parse(dAssemblerLexical& scanner);

	private:
	const dGotoEntry* FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const;
	const dActionEntry* FindAction (const dActionEntry* const list, int count, dToken token) const;
	const dActionEntry* GetNextAction (dList<dStackPair>& stack, dToken token, dAssemblerLexical& scanner) const;

	bool m_grammarError;
};

#endif
