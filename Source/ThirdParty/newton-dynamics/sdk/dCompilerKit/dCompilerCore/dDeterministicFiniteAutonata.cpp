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
#include "dDeterministicFiniteAutonata.h"
#include "dNonDeterministicFiniteAutonata.h"

dDeterministicFiniteAutonata::dDeterministicFiniteAutonata()
	:dFiniteAutomata()
	,m_deterministicFiniteAutomata(NULL)
{
}

dDeterministicFiniteAutonata::dDeterministicFiniteAutonata(const char* const regularExpression)
	:dFiniteAutomata()
	,m_deterministicFiniteAutomata(NULL)
{
	dNonDeterministicFiniteAutonata nfa (regularExpression);
	CreateDeterministicFiniteAutomaton(nfa);
}

dDeterministicFiniteAutonata::dDeterministicFiniteAutonata(const dNonDeterministicFiniteAutonata& nfa)
	:dFiniteAutomata()
	,m_deterministicFiniteAutomata(NULL)
{
	CreateDeterministicFiniteAutomaton(nfa);
}



dDeterministicFiniteAutonata::~dDeterministicFiniteAutonata(void)
{
	if (m_deterministicFiniteAutomata) {
		dList<dAutomataState*> statesList;
		m_deterministicFiniteAutomata->GetStateArray (statesList);
		for (dList<dAutomataState*>::dListNode* node = statesList.GetFirst(); node; node = node->GetNext()) {
			dAutomataState* const state = node->GetInfo();
			delete state;
		}
	}

}


void dDeterministicFiniteAutonata::CopySet (const dNonDeterministicFiniteAutonata& nfa)
{
	dList<dAutomataState*> stateList;
	m_deterministicFiniteAutomata->GetStateArray (stateList);

	dTree<int, int> setFilter;
	for (dList<dAutomataState*>::dListNode* node = stateList.GetFirst(); node; node = node->GetNext()) {
		dAutomataState* const state = node->GetInfo();
		for (dList<dAutomataState::dTransition>::dListNode* transitionNode = state->m_transtions.GetFirst(); transitionNode; transitionNode = transitionNode->GetNext()) {
			dAutomataState::dTransition& transition = transitionNode->GetInfo();
			dAutomataState::dCharacter ch (transition.GetCharater());
			if (ch.m_type == dAutomataState::CHARACTER_SET) {
				setFilter.Insert (ch.m_symbol, ch.m_symbol);
			}
		}
	}

	const dChatertSetMap& charaterSet = nfa.GetChatertSetMap();
	dTree<int, int>::Iterator iter (setFilter);
	for (iter.Begin(); iter; iter ++) {
		dAutomataState::dCharacter ch (iter.GetNode()->GetInfo());
		const dChatertSetMap::ChatertSet* const set = charaterSet.FindSet (ch.m_info);
		_ASSERTE (set);
		m_charaterSetMap.AddSet(set->GetSet(), set->GetLength());
	}
}




void dDeterministicFiniteAutonata::MoveSymbol (int symbol, const dAutomataState* const state, dTree<dAutomataState*,dAutomataState*>& ouput) const
{
	for (dList<dAutomataState*>::dListNode* stateNode = state->m_myNFANullStates.GetFirst(); stateNode; stateNode = stateNode->GetNext()) {
		const dAutomataState* const state = stateNode->GetInfo();
		for (dList<dAutomataState::dTransition>::dListNode* transitionNode = state->m_transtions.GetFirst(); transitionNode; transitionNode = transitionNode->GetNext()) {
			dAutomataState::dTransition& thans = transitionNode->GetInfo();
			dAutomataState::dCharacter ch (thans.GetCharater());
			if (ch.m_symbol == symbol) {
				dAutomataState* const target = thans.GetState();
				ouput.Insert(target, target);
			}
		}
	}
}


void dDeterministicFiniteAutonata::EmptyTransitionClosure (const dTree<dAutomataState*,dAutomataState*>& set, dTree<dAutomataState*,dAutomataState*>& closureStates) const
{

	int stack = 0;
	dAutomataState* stackPool[2048];

	dTree<dAutomataState*,dAutomataState*>::Iterator iter (set);
	for (iter.Begin(); iter; iter ++) {
		dAutomataState* const state = iter.GetNode()->GetInfo();
		stackPool[stack] = state;
		stack ++;
		_ASSERTE (stack  < sizeof (stackPool) / sizeof (stackPool[0]));
		closureStates.Insert(state, state);
	}

	while(stack) {
		stack --;
		dAutomataState* const state = stackPool[stack];
		for (dList<dAutomataState::dTransition>::dListNode* node = state->m_transtions.GetFirst(); node; node = node->GetNext()) {
			dAutomataState::dTransition& transition = node->GetInfo();
			dAutomataState::dCharacter ch (transition.GetCharater());
			if (ch.m_symbol == 0) {
				dAutomataState* const targetState = transition.GetState();
				if(!closureStates.Find(targetState)) {
					closureStates.Insert(targetState, targetState);
					stackPool[stack] = targetState;
					stack ++;
					_ASSERTE (stack  < sizeof (stackPool) / sizeof (stackPool[0]));
				}
			}
		}
	}
}

bool dDeterministicFiniteAutonata::CompareSets (dList<dAutomataState*>& setA, dTree<dAutomataState*,dAutomataState*>& setB) const
{
	if (setA.GetCount() == setB.GetCount()) {
		for (dList<dAutomataState*>::dListNode* node = setA.GetFirst(); node; node = node->GetNext()) {
			if (!setB.Find(node->GetInfo())) {
				return false;
			}
		}
		return true;
	}
	return false;
}


dAutomataState* dDeterministicFiniteAutonata::CreateTargetState (dTree<dAutomataState*,dAutomataState*>& subSet, int id)
{
	dAutomataState* const state = CreateState (id);
	dTree<dAutomataState*,dAutomataState*>::Iterator iter (subSet);
	for (iter.Begin(); iter; iter ++) {
		dAutomataState* const subSetState = iter.GetNode()->GetInfo();
		state->m_myNFANullStates.Append(subSetState);
		if (subSetState->m_exitState) {
			state->m_exitState = true;
		}
	}
	return state;
}


// Subset Construction for converting a Nondeterministic Finite Automaton (NFA) to a Deterministic Finite Automaton (DFA)
// algorithm from book Compilers Principles and Tools: by Alfred V.Aho, Ravi Sethi and Jeffrey D. Ullman 
void dDeterministicFiniteAutonata::CreateDeterministicFiniteAutomaton (const dNonDeterministicFiniteAutonata& nfa)
{
	if (nfa.IsValid()) {
		dTree<int, int> symbolsList;
		dList<dAutomataState*> stateArray;

		nfa.GetStartState()->GetStateArray(stateArray);
		for (dList<dAutomataState*>::dListNode* node = stateArray.GetFirst(); node; node = node->GetNext()) {
			dAutomataState* const state = node->GetInfo();
			for (dList<dAutomataState::dTransition>::dListNode* transitionNode = state->m_transtions.GetFirst(); transitionNode; transitionNode = transitionNode->GetNext()) {
				dAutomataState::dTransition& transition = transitionNode->GetInfo();
				dAutomataState::dCharacter ch (transition.GetCharater());
				if (ch.m_symbol) {
					symbolsList.Insert(ch.m_symbol, ch.m_symbol);
				}
			}
		}


		dTree<dAutomataState*,dAutomataState*> NFAmap;
		dTree<dAutomataState*,dAutomataState*> subSet;

		int stateID = 0;
		NFAmap.Insert(nfa.GetStartState(), nfa.GetStartState());

		dAutomataState* stackPool[2048];

		EmptyTransitionClosure (NFAmap, subSet);
	//	dAutomataState* const startState = new dAutomataState(subSet, stateID ++);
		dAutomataState* const startState = CreateTargetState(subSet, stateID ++);

		dList<dAutomataState*> newStatesList;
		int stack = 1;
		stackPool[0] = startState;

		newStatesList.Append(startState);

		while (stack) {
			stack --;
			dAutomataState* const state = stackPool[stack];

			dTree<int, int>::Iterator iter (symbolsList);
			for (iter.Begin(); iter; iter ++) {
				int ch = iter.GetNode()->GetInfo();

				dTree<dAutomataState*,dAutomataState*> moveSet;
				MoveSymbol (ch, state, moveSet);

				if (moveSet.GetCount()) {

					dTree<dAutomataState*,dAutomataState*> subSet;
					EmptyTransitionClosure (moveSet, subSet);

					dAutomataState* foundState = NULL;
					for (dList<dAutomataState*>::dListNode* node = newStatesList.GetFirst(); node; node = node->GetNext()) {
						dAutomataState* const state = node->GetInfo();
						if (CompareSets(state->m_myNFANullStates, subSet)) {
							foundState = state;
							break;
						}
					}

					if (foundState) {
						state->m_transtions.Append(dAutomataState::dTransition(ch, foundState));

					} else {

						dAutomataState* const targetState = CreateTargetState(subSet, stateID ++);
						stackPool[stack] = targetState;
						stack ++;
						_ASSERTE (stack < sizeof (stackPool)/sizeof (stackPool[0]));

						newStatesList.Append(targetState);
						state->m_transtions.Append(dAutomataState::dTransition(ch, targetState));
					}
				}
			}
		}


		//	int index = 0;
		//	dAutomataState** const stateSort = new dAutomataState*[stateArray.GetCount()];
		//	for (dList<dAutomataState*>::dListNode* node = stateArray.GetFirst(); node; node = node->GetNext()) {
		//		stateSort[index] = node->GetInfo();
		//		index ++;
		//	}
		//	qsort (stateSort, index, sizeof (dAutomataState*), SortStates);
		//	for (int i = 0; i < index; i ++) {
		//		dAutomataState* const state = stateSort[i];
		//		state->m_id = i;
		//	}
		//	delete stateSort;
		
		m_deterministicFiniteAutomata = startState;
		CopySet (nfa);
	}
}



int dDeterministicFiniteAutonata::FindMatch(const char* const text) const
{
	int count = -1;
	if (text[0]) {
		count = 0;
		int stack = 0;
		dAutomataState* state = m_deterministicFiniteAutomata;
		for(int i = 0; text[i]; i ++) {
			int extendChar = 0;
			int ch = text[i];
			if (ch == '\\') {
				i ++;
				extendChar = 1;
				_ASSERTE (text[i]);
				ch = (ch << 8) + text[i];
				ch = GetScapeChar (ch);
			}


			dAutomataState::dTransition* transition = NULL;
			for (dList<dAutomataState::dTransition>::dListNode* node = state->m_transtions.GetFirst(); node && !transition; node = node->GetNext()) {
				dAutomataState::dTransition*const trans = &node->GetInfo();
				dAutomataState::dCharacter symbol (trans->GetCharater());

				switch (symbol.m_type) 
				{
					case dAutomataState::CHARACTER: 
					{
						if (symbol.m_info == ch) {
							transition = trans;
						}
						break;
					}

					case dAutomataState::CHARACTER_SET:
					{
						const dChatertSetMap::ChatertSet* const characterSet = m_charaterSetMap.FindSet(symbol.m_info);
						if (characterSet) {
							if (characterSet->IsCharAMatch (GetScapeChar (ch))) {
								transition = trans;
							}
						}
						break;
					}


					case dAutomataState::NESTED_CHARACTER_INIT:
					{
						if (symbol.m_info == ch) {
							stack = 1;
							transition = trans;
						}
						break;
					}

					case dAutomataState::NESTED_CHARACTER_INC:
					{
						if (symbol.m_info == ch) {
							stack ++;
							transition = trans;
						}
						break;
					}

					case dAutomataState::NESTED_CHARACTER_DEC:
					{
						if (symbol.m_info == ch) {
							stack --;
							transition = trans;
							if (stack == 0) {
								if (state->m_exitState) {
									count ++;
									transition = NULL;
								}
							}
						}
						break;
					}
				}
			}


			if(transition) {
				count ++;
				count += extendChar;
				state = transition->GetState();
			} else {
				if (!state->m_exitState) {
					count = -1;
				}
				break;
			}
		}
	}

	return count;
}
