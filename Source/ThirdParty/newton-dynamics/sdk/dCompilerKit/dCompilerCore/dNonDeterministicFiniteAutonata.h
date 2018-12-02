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


#ifndef __dNonDeterministicFiniteAutonata_h_
#define __dNonDeterministicFiniteAutonata_h_

#include "dFiniteAutomata.h"
#include "dChatertSetMap.h"


#define D_ASCII_SET_MAX_SIZE  2048


class dAutomataState;



class dNonDeterministicFiniteAutonata: public dFiniteAutomata
{
	public:
	dNonDeterministicFiniteAutonata();
	dNonDeterministicFiniteAutonata(const char* const regularExpression);
	virtual ~dNonDeterministicFiniteAutonata();

	bool IsValid() const;
	const dChatertSetMap& GetChatertSetMap() const;

	dAutomataState* GetStartState() const;
	dAutomataState* GetExitState() const;
//	dAutomataState* CreateDeterministicFiniteAutomaton () const;

	

	protected:
	enum Operations;

	class dStateConstructPair
	{
		public: 
		dStateConstructPair ();
		dStateConstructPair (dAutomataState* start, dAutomataState* accepting);
		dAutomataState* GetStart() const;
		dAutomataState* GetAccepting() const;

		private:
		dAutomataState* m_start;
		dAutomataState* m_accepting;
	};

	class dAutomataStateConstructStack
	{
		public:
		dAutomataStateConstructStack ();
		bool IsEmpty() const;
		dStateConstructPair Pop ();
		void Push (dAutomataState* const start, dAutomataState* const accepting);

		private:
		int m_index;
		dStateConstructPair m_pool[1024];
	};

	virtual void ShiftID();
	virtual bool IsOperator (int charater) const;
	virtual bool CheckInsertConcatenation (int left, int right) const;

	virtual void PreProcessExpression (const char* const regularExpression);
	void CompileExpression(const char* const regularExpression);	
	int GetChar();
	void ParseExpresionToNFA ();
	void DeleteNFA (dAutomataState* const startdAutomataState);
	

	void Match (int token);
	void PushId (int charater);
	void PushSet (const char* const set, int size);

	void ReduceUnionDiagram();
	void ReduceConcatenationDiagram();
	void ReduceZeroOrMoreDiagram ();
	void ReduceOneOrMoreDiagram();
	void ReduceZeroOrOneDiagram();
	void ReduceBalancedCharacterExpresion(char openChar, char closingChar);

	void UnionExpression ();
	void ConcatenationExpression ();
	void UnuaryExpression ();
	int BracketedExpression (char* const set, int size);

	



	static int SortStates (const void *ptr0, const void *ptr1);

	bool m_error;
	int m_token;
	int m_stateID;
	int m_regularExpressionIndex;	
	char m_regularExpression[D_ASCII_SET_MAX_SIZE];	
	dAutomataState* m_startState; 
	dAutomataState* m_acceptingState; 
	dAutomataStateConstructStack m_stack;

	dChatertSetMap m_charaterSetMap;

	static char m_asciiSet[128];
	static char m_asciiSetButNewLine[128];
};


#endif