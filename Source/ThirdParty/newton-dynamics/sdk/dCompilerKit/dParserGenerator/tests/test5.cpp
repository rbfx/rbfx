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
//Auto generated Parcer Generator class: test5.cpp
//

#include "test5.h"
#include <dList.h>

#define MAX_USER_PARAM	64

enum test5::ActionType
{
	dSHIFT = 0,
	dREDUCE,
	dACCEPT,
	dERROR
};


class test5::dActionEntry
{
	public:
	dActionEntry (short token, short stateType,	short nextState, short ruleSymbols, short ruleIndex)
		:m_token(token), m_stateType(stateType), m_nextState(nextState), m_ruleSymbols(ruleSymbols), m_ruleIndex(ruleIndex)
	{
	}

	short m_token;
	short m_stateType;// 0 = shift, 1 = reduce, 2 = accept
	short m_nextState;
	short m_ruleSymbols;
	short m_ruleIndex;
};

class test5::dGotoEntry
{
	public:
	dGotoEntry (short token, short nextState)
		:m_token(token), m_nextState(nextState)
	{
	}

	short  m_token;
	short  m_nextState;
};



class test5::dStackPair
{
	public:
	class dUserVariable
	{
		public:
		dUserVariable () 
			:m_token (dToken (0)), m_data("")
		{
		}


		dUserVariable (dToken token, const char* const data)
			:m_token(token), m_data (data) 
		{
		}
		dToken m_token;
		string m_data;
	};


	dStackPair()
		:m_state(0), m_token(dToken (0)), m_value()
	{
	}

	int m_state;
	dToken m_token;
	dUserVariable m_value;
};


test5::test5()
{
}

test5::~test5()
{
}


bool test5::ErrorHandler (const string& line) const
{
	line;
	return false;
}

const test5::dActionEntry* test5::FindAction (const dActionEntry* const actionList, int count, dToken token) const
{

	int i0 = 0;
	int i1 = count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;

		const dActionEntry& action = actionList[i];
		if (token <= dToken(action.m_token)) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		const dActionEntry& action = actionList[i];
		if (token == dToken(action.m_token)) {
			return& action;
		}
	}

	_ASSERT (0);
	return NULL;
}

const test5::dGotoEntry* dAssemblerParcer::FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const
{
	int i0 = 0;
	int i1 = count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;

		const dGotoEntry& action = gotoList[i];
		if (token <= dToken(action.m_token)) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		const dGotoEntry& action = gotoList[i];
		if (token == dToken(action.m_token)) {
			return &action;
		}
	}

	_ASSERT (0);
	return NULL;
}


bool test5::Parce(lextest1& scanner)
{
	dList<dStackPair> stack;
	static short actionsCount[] = {2, 3, 3, 3, 2, 2, 3, 3, 2, 3};
	static short actionsStart[] = {0, 2, 5, 8, 0, 0, 11, 14, 0, 14};
	static dActionEntry actionTable[] = {
					dActionEntry (258, 0, 2, 0, 0), dActionEntry (266, 0, 3, 0, 0), 
					dActionEntry (0, 2, 0, 0, 0), dActionEntry (256, 0, 4, 0, 0), dActionEntry (257, 0, 5, 0, 0), 
					dActionEntry (0, 1, 0, 1, 5), dActionEntry (256, 1, 0, 1, 5), dActionEntry (257, 1, 0, 1, 5), 
					dActionEntry (0, 1, 0, 1, 4), dActionEntry (256, 1, 0, 1, 4), dActionEntry (257, 1, 0, 1, 4), 
					dActionEntry (0, 1, 0, 3, 2), dActionEntry (256, 0, 4, 0, 0), dActionEntry (257, 0, 8, 0, 0), 
					dActionEntry (0, 1, 0, 3, 3), dActionEntry (256, 0, 4, 0, 0), dActionEntry (257, 0, 5, 0, 0), 
			};

	static short gotoCount[] = {1, 0, 0, 0, 1, 1, 0, 0, 1, 0};
	static short gotoStart[] = {0, 1, 1, 1, 1, 2, 3, 3, 3, 4};
	static dGotoEntry gotoTable[] = {
					dGotoEntry (259, 1), dGotoEntry (259, 6), dGotoEntry (259, 7), dGotoEntry (259, 9), 
			};

	const int lastToken = 259;

	stack.Append ();
	dToken token = dToken (scanner.NextToken());
	for (;;) {
		const dStackPair& stackTop = stack.GetLast()->GetInfo();
		int start = actionsStart[stackTop.m_state];
		int count = actionsCount[stackTop.m_state];
		const dActionEntry* const action (FindAction (&actionTable[start], count, token));
		_ASSERTE (action);

		switch (action->m_stateType) 
		{
			case dSHIFT: 
			{
				dStackPair& entry = stack.Append()->GetInfo();
				entry.m_token = dToken (action->m_token);
				entry.m_state = action->m_nextState;
				entry.m_value = dStackPair::dUserVariable (entry.m_token, scanner.GetTokenString());
				token = dToken (scanner.NextToken());
				if (token == -1) {
					token = dToken (0);
				}

				break;
			}

			case dREDUCE: 
			{
				dStackPair parameter[MAX_USER_PARAM];

				int reduceCount = action->m_ruleSymbols;
				_ASSERTE (reduceCount < sizeof (parameter) / sizeof (parameter[0]));

				for (int i = 0; i < reduceCount; i ++) {
					parameter[reduceCount - i - 1] = stack.GetLast()->GetInfo();
					stack.Remove (stack.GetLast());
				}

				const dStackPair& stackTop = stack.GetLast()->GetInfo();
				int start = gotoStart[stackTop.m_state];
				int count = gotoCount[stackTop.m_state];
				const dGotoEntry* const gotoEntry = FindGoto (&gotoTable[start], count, dToken (action->m_nextState + lastToken));

				dStackPair& entry = stack.Append()->GetInfo();
				entry.m_state = gotoEntry->m_nextState;
				entry.m_token = dToken (gotoEntry->m_token);
				
				switch (action->m_ruleIndex) 
				{
					//do user semantic Actions
					case 5:// rule E : c 
						{}
						break;
					case 4:// rule E : { E } 
						{}
						break;
					case 2:// rule E : E sub E 
						{}
						break;
					case 3:// rule E : E sup E 
						{}
						break;

					default:;
				}

				break;

			}
	
			case dACCEPT: // 2 = accept
			{
				// program parced successfully, exit with successful code
				return true;
			}
			
			default:  
			{
				_ASSERTE (0);
				// syntact error parciing program
				//if (!ErrorHandler ("error")) {
				//}
				break;
			}
		}
	}
	return false;
}






