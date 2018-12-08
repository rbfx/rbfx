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

#ifndef __dDeterministicFiniteAutonata_h_
#define __dDeterministicFiniteAutonata_h_


#include "dNonDeterministicFiniteAutonata.h"


class dDeterministicFiniteAutonata: public dFiniteAutomata 
{
	public:
	dDeterministicFiniteAutonata();
	dDeterministicFiniteAutonata(const char* const regularExpression);
	dDeterministicFiniteAutonata(const dNonDeterministicFiniteAutonata& nfa);
	
	virtual ~dDeterministicFiniteAutonata(void);

	virtual int FindMatch(const char* const text) const;

	protected:
		
	virtual dAutomataState* CreateTargetState (dTree<dAutomataState*,dAutomataState*>& subSet, int id);
	void CreateDeterministicFiniteAutomaton (const dNonDeterministicFiniteAutonata& nfa);

	private:
	void CopySet (const dNonDeterministicFiniteAutonata& nfa);
	bool CompareSets (dList<dAutomataState*>& setA, dTree<dAutomataState*,dAutomataState*>& setB) const;
	void MoveSymbol (int symbol, const dAutomataState* const state, dTree<dAutomataState*,dAutomataState*>& ouput) const;
	void EmptyTransitionClosure (const dTree<dAutomataState*,dAutomataState*>& set, dTree<dAutomataState*,dAutomataState*>& closureStates) const;

	protected:
	dAutomataState* m_deterministicFiniteAutomata; 
	dChatertSetMap m_charaterSetMap;
};


#endif