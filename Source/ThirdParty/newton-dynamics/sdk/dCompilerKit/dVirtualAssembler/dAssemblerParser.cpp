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
// Auto generated Parser Generator class: dAssemblerParser.cpp
//

#include "dAssemblerCompiler.h"
#include "dAssemblerLexical.h"

// Newton virtual machine assembler grammar
// based loosely on a subset and a hybrid between the MIPS R3000 and the Intel 386 instructions set 


#include "dAssemblerParser.h"
#include <dList.h>

#define MAX_USER_PARAM	64

enum dAssemblerParser::ActionType
{
	dSHIFT = 0,
	dREDUCE,
	dACCEPT,
	dERROR
};

class dAssemblerParser::dActionEntry
{
	public:
	dActionEntry (short token, char errorItem, char stateType, short nextState, short ruleSymbols, short ruleIndex)
		:m_token(token), m_errorRule(errorItem), m_stateType (stateType), m_nextState(nextState), m_ruleSymbols(ruleSymbols), m_ruleIndex(ruleIndex)
	{
	}

	short m_token;
	char m_errorRule;
	char m_stateType;// 0 = shift, 1 = reduce, 2 = accept
	short m_nextState;
	short m_ruleSymbols;
	short m_ruleIndex;
};

class dAssemblerParser::dGotoEntry
{
	public:
	dGotoEntry (short token, short nextState)
		:m_token(token), m_nextState(nextState)
	{
	}

	short  m_token;
	short  m_nextState;
};



class dAssemblerParser::dStackPair
{
	public:
	dStackPair()
		:m_state(0), m_scannerLine(0), m_scannerIndex(0), m_token(dToken (0)), m_value()
	{
	}

	int m_state;
	int m_scannerLine;
	int m_scannerIndex;
	dToken m_token;
	dUserVariable m_value;
};


dAssemblerParser::dAssemblerParser()
{
}

dAssemblerParser::~dAssemblerParser()
{
}


const dAssemblerParser::dActionEntry* dAssemblerParser::FindAction (const dActionEntry* const actionList, int count, dToken token) const
{
	int i0 = 0;
	int i1 = count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;

		const dActionEntry& action = actionList[i];
		dToken actionToken (dToken(action.m_token));
		if (token <= actionToken) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		const dActionEntry& action = actionList[i];
		dToken actionToken (dToken(action.m_token));
		if (token == actionToken) {
			return& action;
		}
	}


	return NULL;
}

const dAssemblerParser::dGotoEntry* dAssemblerParser::FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const
{
	int i0 = 0;
	int i1 = count - 1;
	while ((i1 - i0) >= 4) {
		int i = (i1 + i0 + 1)>>1;

		const dGotoEntry& action = gotoList[i];
		dToken actionToken (dToken(action.m_token));
		if (token <= actionToken) {
			i1 = i;
		} else {
			i0 = i;
		}
	}

	for (int i = i0; i <= i1; i ++) {
		const dGotoEntry& action = gotoList[i];
		dToken actionToken (dToken(action.m_token));
		if (token == actionToken) {
			return &action;
		}
	}

	dAssert (0);
	return NULL;
}



const dAssemblerParser::dActionEntry* dAssemblerParser::GetNextAction (dList<dStackPair>& stack, dToken token, dAssemblerLexical& scanner) const
{
	static short actionsCount[] = {
			4, 28, 4, 1, 4, 1, 4, 1, 1, 1, 4, 4, 1, 29, 29, 29, 1, 1, 1, 29, 1, 1, 1, 1, 
			1, 1, 1, 29, 1, 1, 29, 29, 1, 29, 29, 29, 1, 1, 29, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
			1, 1, 1, 1, 1, 1, 1, 5, 5, 29, 28, 1, 4, 30, 30, 30, 1, 1, 29, 4, 29, 29, 29, 29, 
			29, 1, 29, 29, 1, 28, 28, 1, 1, 2, 1, 4, 4, 4, 30, 1, 29, 29, 30, 30, 1, 1, 29, 29};
	static short actionsStart[] = {
			0, 4, 32, 36, 37, 41, 42, 46, 47, 48, 49, 53, 57, 58, 87, 116, 145, 146, 147, 148, 177, 178, 179, 180, 
			181, 182, 183, 184, 213, 214, 215, 244, 273, 274, 303, 332, 361, 362, 363, 392, 393, 394, 395, 396, 397, 398, 399, 213, 
			400, 401, 402, 403, 404, 362, 405, 406, 411, 416, 445, 473, 474, 478, 508, 538, 568, 569, 570, 599, 603, 632, 661, 690, 
			719, 748, 749, 778, 807, 808, 836, 864, 182, 865, 867, 868, 872, 876, 880, 910, 911, 940, 969, 538, 999, 362, 1000, 1029};
	static dActionEntry actionTable[] = {
			dActionEntry (254, 0, 1, 0, 0, 2), dActionEntry (256, 0, 0, 9, 0, 0), dActionEntry (260, 0, 0, 8, 0, 0), dActionEntry (262, 0, 0, 7, 0, 0), 
			dActionEntry (265, 0, 0, 45, 0, 0), dActionEntry (272, 0, 0, 42, 0, 0), dActionEntry (273, 0, 0, 21, 0, 0), dActionEntry (274, 0, 0, 32, 0, 0), 
			dActionEntry (275, 0, 0, 23, 0, 0), dActionEntry (276, 0, 0, 51, 0, 0), dActionEntry (277, 0, 0, 39, 0, 0), dActionEntry (278, 0, 0, 40, 0, 0), 
			dActionEntry (279, 0, 0, 46, 0, 0), dActionEntry (280, 0, 0, 22, 0, 0), dActionEntry (281, 0, 0, 29, 0, 0), dActionEntry (282, 0, 0, 17, 0, 0), 
			dActionEntry (283, 0, 0, 54, 0, 0), dActionEntry (284, 0, 0, 20, 0, 0), dActionEntry (285, 0, 0, 48, 0, 0), dActionEntry (286, 0, 0, 44, 0, 0), 
			dActionEntry (287, 0, 0, 43, 0, 0), dActionEntry (288, 0, 0, 41, 0, 0), dActionEntry (289, 0, 0, 18, 0, 0), dActionEntry (290, 0, 0, 50, 0, 0), 
			dActionEntry (291, 0, 0, 26, 0, 0), dActionEntry (292, 0, 0, 24, 0, 0), dActionEntry (293, 0, 0, 15, 0, 0), dActionEntry (295, 0, 0, 16, 0, 0), 
			dActionEntry (296, 0, 0, 49, 0, 0), dActionEntry (299, 0, 0, 52, 0, 0), dActionEntry (300, 0, 0, 12, 0, 0), dActionEntry (301, 0, 0, 33, 0, 0), 
			dActionEntry (254, 0, 1, 2, 1, 7), dActionEntry (256, 0, 1, 2, 1, 7), dActionEntry (260, 0, 1, 2, 1, 7), dActionEntry (262, 0, 1, 2, 1, 7), 
			dActionEntry (254, 0, 2, 0, 0, 0), dActionEntry (254, 0, 1, 2, 1, 6), dActionEntry (256, 0, 1, 2, 1, 6), dActionEntry (260, 0, 1, 2, 1, 6), 
			dActionEntry (262, 0, 1, 2, 1, 6), dActionEntry (258, 0, 0, 56, 0, 0), dActionEntry (254, 0, 1, 1, 1, 3), dActionEntry (256, 0, 1, 1, 1, 3), 
			dActionEntry (260, 0, 1, 1, 1, 3), dActionEntry (262, 0, 1, 1, 1, 3), dActionEntry (258, 0, 1, 7, 1, 12), dActionEntry (258, 0, 0, 58, 0, 0), 
			dActionEntry (60, 0, 0, 59, 0, 0), dActionEntry (254, 0, 1, 2, 1, 5), dActionEntry (256, 0, 1, 2, 1, 5), dActionEntry (260, 0, 1, 2, 1, 5), 
			dActionEntry (262, 0, 1, 2, 1, 5), dActionEntry (254, 0, 1, 0, 1, 1), dActionEntry (256, 0, 0, 9, 0, 0), dActionEntry (260, 0, 0, 8, 0, 0), 
			dActionEntry (262, 0, 0, 7, 0, 0), dActionEntry (264, 0, 1, 25, 1, 39), dActionEntry (261, 0, 1, 13, 1, 23), dActionEntry (265, 0, 1, 13, 1, 23), 
			dActionEntry (272, 0, 1, 13, 1, 23), dActionEntry (273, 0, 1, 13, 1, 23), dActionEntry (274, 0, 1, 13, 1, 23), dActionEntry (275, 0, 1, 13, 1, 23), 
			dActionEntry (276, 0, 1, 13, 1, 23), dActionEntry (277, 0, 1, 13, 1, 23), dActionEntry (278, 0, 1, 13, 1, 23), dActionEntry (279, 0, 1, 13, 1, 23), 
			dActionEntry (280, 0, 1, 13, 1, 23), dActionEntry (281, 0, 1, 13, 1, 23), dActionEntry (282, 0, 1, 13, 1, 23), dActionEntry (283, 0, 1, 13, 1, 23), 
			dActionEntry (284, 0, 1, 13, 1, 23), dActionEntry (285, 0, 1, 13, 1, 23), dActionEntry (286, 0, 1, 13, 1, 23), dActionEntry (287, 0, 1, 13, 1, 23), 
			dActionEntry (288, 0, 1, 13, 1, 23), dActionEntry (289, 0, 1, 13, 1, 23), dActionEntry (290, 0, 1, 13, 1, 23), dActionEntry (291, 0, 1, 13, 1, 23), 
			dActionEntry (292, 0, 1, 13, 1, 23), dActionEntry (293, 0, 1, 13, 1, 23), dActionEntry (295, 0, 1, 13, 1, 23), dActionEntry (296, 0, 1, 13, 1, 23), 
			dActionEntry (299, 0, 1, 13, 1, 23), dActionEntry (300, 0, 1, 13, 1, 23), dActionEntry (301, 0, 1, 13, 1, 23), dActionEntry (261, 0, 1, 12, 1, 19), 
			dActionEntry (265, 0, 1, 12, 1, 19), dActionEntry (272, 0, 1, 12, 1, 19), dActionEntry (273, 0, 1, 12, 1, 19), dActionEntry (274, 0, 1, 12, 1, 19), 
			dActionEntry (275, 0, 1, 12, 1, 19), dActionEntry (276, 0, 1, 12, 1, 19), dActionEntry (277, 0, 1, 12, 1, 19), dActionEntry (278, 0, 1, 12, 1, 19), 
			dActionEntry (279, 0, 1, 12, 1, 19), dActionEntry (280, 0, 1, 12, 1, 19), dActionEntry (281, 0, 1, 12, 1, 19), dActionEntry (282, 0, 1, 12, 1, 19), 
			dActionEntry (283, 0, 1, 12, 1, 19), dActionEntry (284, 0, 1, 12, 1, 19), dActionEntry (285, 0, 1, 12, 1, 19), dActionEntry (286, 0, 1, 12, 1, 19), 
			dActionEntry (287, 0, 1, 12, 1, 19), dActionEntry (288, 0, 1, 12, 1, 19), dActionEntry (289, 0, 1, 12, 1, 19), dActionEntry (290, 0, 1, 12, 1, 19), 
			dActionEntry (291, 0, 1, 12, 1, 19), dActionEntry (292, 0, 1, 12, 1, 19), dActionEntry (293, 0, 1, 12, 1, 19), dActionEntry (295, 0, 1, 12, 1, 19), 
			dActionEntry (296, 0, 1, 12, 1, 19), dActionEntry (299, 0, 1, 12, 1, 19), dActionEntry (300, 0, 1, 12, 1, 19), dActionEntry (301, 0, 1, 12, 1, 19), 
			dActionEntry (261, 0, 1, 20, 1, 28), dActionEntry (265, 0, 1, 20, 1, 28), dActionEntry (272, 0, 1, 20, 1, 28), dActionEntry (273, 0, 1, 20, 1, 28), 
			dActionEntry (274, 0, 1, 20, 1, 28), dActionEntry (275, 0, 1, 20, 1, 28), dActionEntry (276, 0, 1, 20, 1, 28), dActionEntry (277, 0, 1, 20, 1, 28), 
			dActionEntry (278, 0, 1, 20, 1, 28), dActionEntry (279, 0, 1, 20, 1, 28), dActionEntry (280, 0, 1, 20, 1, 28), dActionEntry (281, 0, 1, 20, 1, 28), 
			dActionEntry (282, 0, 1, 20, 1, 28), dActionEntry (283, 0, 1, 20, 1, 28), dActionEntry (284, 0, 1, 20, 1, 28), dActionEntry (285, 0, 1, 20, 1, 28), 
			dActionEntry (286, 0, 1, 20, 1, 28), dActionEntry (287, 0, 1, 20, 1, 28), dActionEntry (288, 0, 1, 20, 1, 28), dActionEntry (289, 0, 1, 20, 1, 28), 
			dActionEntry (290, 0, 1, 20, 1, 28), dActionEntry (291, 0, 1, 20, 1, 28), dActionEntry (292, 0, 1, 20, 1, 28), dActionEntry (293, 0, 1, 20, 1, 28), 
			dActionEntry (295, 0, 1, 20, 1, 28), dActionEntry (296, 0, 1, 20, 1, 28), dActionEntry (299, 0, 1, 20, 1, 28), dActionEntry (300, 0, 1, 20, 1, 28), 
			dActionEntry (301, 0, 1, 20, 1, 28), dActionEntry (258, 0, 1, 22, 1, 31), dActionEntry (264, 0, 1, 27, 1, 48), dActionEntry (264, 0, 1, 27, 1, 63), 
			dActionEntry (261, 0, 1, 15, 1, 29), dActionEntry (265, 0, 1, 15, 1, 29), dActionEntry (272, 0, 1, 15, 1, 29), dActionEntry (273, 0, 1, 15, 1, 29), 
			dActionEntry (274, 0, 1, 15, 1, 29), dActionEntry (275, 0, 1, 15, 1, 29), dActionEntry (276, 0, 1, 15, 1, 29), dActionEntry (277, 0, 1, 15, 1, 29), 
			dActionEntry (278, 0, 1, 15, 1, 29), dActionEntry (279, 0, 1, 15, 1, 29), dActionEntry (280, 0, 1, 15, 1, 29), dActionEntry (281, 0, 1, 15, 1, 29), 
			dActionEntry (282, 0, 1, 15, 1, 29), dActionEntry (283, 0, 1, 15, 1, 29), dActionEntry (284, 0, 1, 15, 1, 29), dActionEntry (285, 0, 1, 15, 1, 29), 
			dActionEntry (286, 0, 1, 15, 1, 29), dActionEntry (287, 0, 1, 15, 1, 29), dActionEntry (288, 0, 1, 15, 1, 29), dActionEntry (289, 0, 1, 15, 1, 29), 
			dActionEntry (290, 0, 1, 15, 1, 29), dActionEntry (291, 0, 1, 15, 1, 29), dActionEntry (292, 0, 1, 15, 1, 29), dActionEntry (293, 0, 1, 15, 1, 29), 
			dActionEntry (295, 0, 1, 15, 1, 29), dActionEntry (296, 0, 1, 15, 1, 29), dActionEntry (299, 0, 1, 15, 1, 29), dActionEntry (300, 0, 1, 15, 1, 29), 
			dActionEntry (301, 0, 1, 15, 1, 29), dActionEntry (264, 0, 1, 27, 1, 50), dActionEntry (264, 0, 1, 27, 1, 56), dActionEntry (264, 0, 1, 27, 1, 46), 
			dActionEntry (264, 0, 1, 27, 1, 52), dActionEntry (264, 0, 1, 26, 1, 41), dActionEntry (264, 0, 0, 63, 0, 0), dActionEntry (258, 0, 1, 21, 1, 30), 
			dActionEntry (261, 0, 1, 13, 1, 21), dActionEntry (265, 0, 1, 13, 1, 21), dActionEntry (272, 0, 1, 13, 1, 21), dActionEntry (273, 0, 1, 13, 1, 21), 
			dActionEntry (274, 0, 1, 13, 1, 21), dActionEntry (275, 0, 1, 13, 1, 21), dActionEntry (276, 0, 1, 13, 1, 21), dActionEntry (277, 0, 1, 13, 1, 21), 
			dActionEntry (278, 0, 1, 13, 1, 21), dActionEntry (279, 0, 1, 13, 1, 21), dActionEntry (280, 0, 1, 13, 1, 21), dActionEntry (281, 0, 1, 13, 1, 21), 
			dActionEntry (282, 0, 1, 13, 1, 21), dActionEntry (283, 0, 1, 13, 1, 21), dActionEntry (284, 0, 1, 13, 1, 21), dActionEntry (285, 0, 1, 13, 1, 21), 
			dActionEntry (286, 0, 1, 13, 1, 21), dActionEntry (287, 0, 1, 13, 1, 21), dActionEntry (288, 0, 1, 13, 1, 21), dActionEntry (289, 0, 1, 13, 1, 21), 
			dActionEntry (290, 0, 1, 13, 1, 21), dActionEntry (291, 0, 1, 13, 1, 21), dActionEntry (292, 0, 1, 13, 1, 21), dActionEntry (293, 0, 1, 13, 1, 21), 
			dActionEntry (295, 0, 1, 13, 1, 21), dActionEntry (296, 0, 1, 13, 1, 21), dActionEntry (299, 0, 1, 13, 1, 21), dActionEntry (300, 0, 1, 13, 1, 21), 
			dActionEntry (301, 0, 1, 13, 1, 21), dActionEntry (264, 0, 0, 65, 0, 0), dActionEntry (264, 0, 1, 27, 1, 47), dActionEntry (261, 0, 0, 67, 0, 0), 
			dActionEntry (265, 0, 0, 45, 0, 0), dActionEntry (272, 0, 0, 42, 0, 0), dActionEntry (273, 0, 0, 21, 0, 0), dActionEntry (274, 0, 0, 32, 0, 0), 
			dActionEntry (275, 0, 0, 23, 0, 0), dActionEntry (276, 0, 0, 51, 0, 0), dActionEntry (277, 0, 0, 39, 0, 0), dActionEntry (278, 0, 0, 40, 0, 0), 
			dActionEntry (279, 0, 0, 46, 0, 0), dActionEntry (280, 0, 0, 22, 0, 0), dActionEntry (281, 0, 0, 29, 0, 0), dActionEntry (282, 0, 0, 17, 0, 0), 
			dActionEntry (283, 0, 0, 54, 0, 0), dActionEntry (284, 0, 0, 20, 0, 0), dActionEntry (285, 0, 0, 48, 0, 0), dActionEntry (286, 0, 0, 44, 0, 0), 
			dActionEntry (287, 0, 0, 43, 0, 0), dActionEntry (288, 0, 0, 41, 0, 0), dActionEntry (289, 0, 0, 18, 0, 0), dActionEntry (290, 0, 0, 50, 0, 0), 
			dActionEntry (291, 0, 0, 26, 0, 0), dActionEntry (292, 0, 0, 24, 0, 0), dActionEntry (293, 0, 0, 15, 0, 0), dActionEntry (295, 0, 0, 16, 0, 0), 
			dActionEntry (296, 0, 0, 49, 0, 0), dActionEntry (299, 0, 0, 52, 0, 0), dActionEntry (300, 0, 0, 12, 0, 0), dActionEntry (301, 0, 0, 33, 0, 0), 
			dActionEntry (261, 0, 1, 13, 1, 26), dActionEntry (265, 0, 1, 13, 1, 26), dActionEntry (272, 0, 1, 13, 1, 26), dActionEntry (273, 0, 1, 13, 1, 26), 
			dActionEntry (274, 0, 1, 13, 1, 26), dActionEntry (275, 0, 1, 13, 1, 26), dActionEntry (276, 0, 1, 13, 1, 26), dActionEntry (277, 0, 1, 13, 1, 26), 
			dActionEntry (278, 0, 1, 13, 1, 26), dActionEntry (279, 0, 1, 13, 1, 26), dActionEntry (280, 0, 1, 13, 1, 26), dActionEntry (281, 0, 1, 13, 1, 26), 
			dActionEntry (282, 0, 1, 13, 1, 26), dActionEntry (283, 0, 1, 13, 1, 26), dActionEntry (284, 0, 1, 13, 1, 26), dActionEntry (285, 0, 1, 13, 1, 26), 
			dActionEntry (286, 0, 1, 13, 1, 26), dActionEntry (287, 0, 1, 13, 1, 26), dActionEntry (288, 0, 1, 13, 1, 26), dActionEntry (289, 0, 1, 13, 1, 26), 
			dActionEntry (290, 0, 1, 13, 1, 26), dActionEntry (291, 0, 1, 13, 1, 26), dActionEntry (292, 0, 1, 13, 1, 26), dActionEntry (293, 0, 1, 13, 1, 26), 
			dActionEntry (295, 0, 1, 13, 1, 26), dActionEntry (296, 0, 1, 13, 1, 26), dActionEntry (299, 0, 1, 13, 1, 26), dActionEntry (300, 0, 1, 13, 1, 26), 
			dActionEntry (301, 0, 1, 13, 1, 26), dActionEntry (264, 0, 1, 27, 1, 51), dActionEntry (261, 0, 1, 14, 1, 27), dActionEntry (265, 0, 1, 14, 1, 27), 
			dActionEntry (272, 0, 1, 14, 1, 27), dActionEntry (273, 0, 1, 14, 1, 27), dActionEntry (274, 0, 1, 14, 1, 27), dActionEntry (275, 0, 1, 14, 1, 27), 
			dActionEntry (276, 0, 1, 14, 1, 27), dActionEntry (277, 0, 1, 14, 1, 27), dActionEntry (278, 0, 1, 14, 1, 27), dActionEntry (279, 0, 1, 14, 1, 27), 
			dActionEntry (280, 0, 1, 14, 1, 27), dActionEntry (281, 0, 1, 14, 1, 27), dActionEntry (282, 0, 1, 14, 1, 27), dActionEntry (283, 0, 1, 14, 1, 27), 
			dActionEntry (284, 0, 1, 14, 1, 27), dActionEntry (285, 0, 1, 14, 1, 27), dActionEntry (286, 0, 1, 14, 1, 27), dActionEntry (287, 0, 1, 14, 1, 27), 
			dActionEntry (288, 0, 1, 14, 1, 27), dActionEntry (289, 0, 1, 14, 1, 27), dActionEntry (290, 0, 1, 14, 1, 27), dActionEntry (291, 0, 1, 14, 1, 27), 
			dActionEntry (292, 0, 1, 14, 1, 27), dActionEntry (293, 0, 1, 14, 1, 27), dActionEntry (295, 0, 1, 14, 1, 27), dActionEntry (296, 0, 1, 14, 1, 27), 
			dActionEntry (299, 0, 1, 14, 1, 27), dActionEntry (300, 0, 1, 14, 1, 27), dActionEntry (301, 0, 1, 14, 1, 27), dActionEntry (261, 0, 1, 13, 1, 22), 
			dActionEntry (265, 0, 1, 13, 1, 22), dActionEntry (272, 0, 1, 13, 1, 22), dActionEntry (273, 0, 1, 13, 1, 22), dActionEntry (274, 0, 1, 13, 1, 22), 
			dActionEntry (275, 0, 1, 13, 1, 22), dActionEntry (276, 0, 1, 13, 1, 22), dActionEntry (277, 0, 1, 13, 1, 22), dActionEntry (278, 0, 1, 13, 1, 22), 
			dActionEntry (279, 0, 1, 13, 1, 22), dActionEntry (280, 0, 1, 13, 1, 22), dActionEntry (281, 0, 1, 13, 1, 22), dActionEntry (282, 0, 1, 13, 1, 22), 
			dActionEntry (283, 0, 1, 13, 1, 22), dActionEntry (284, 0, 1, 13, 1, 22), dActionEntry (285, 0, 1, 13, 1, 22), dActionEntry (286, 0, 1, 13, 1, 22), 
			dActionEntry (287, 0, 1, 13, 1, 22), dActionEntry (288, 0, 1, 13, 1, 22), dActionEntry (289, 0, 1, 13, 1, 22), dActionEntry (290, 0, 1, 13, 1, 22), 
			dActionEntry (291, 0, 1, 13, 1, 22), dActionEntry (292, 0, 1, 13, 1, 22), dActionEntry (293, 0, 1, 13, 1, 22), dActionEntry (295, 0, 1, 13, 1, 22), 
			dActionEntry (296, 0, 1, 13, 1, 22), dActionEntry (299, 0, 1, 13, 1, 22), dActionEntry (300, 0, 1, 13, 1, 22), dActionEntry (301, 0, 1, 13, 1, 22), 
			dActionEntry (261, 0, 1, 13, 1, 24), dActionEntry (265, 0, 1, 13, 1, 24), dActionEntry (272, 0, 1, 13, 1, 24), dActionEntry (273, 0, 1, 13, 1, 24), 
			dActionEntry (274, 0, 1, 13, 1, 24), dActionEntry (275, 0, 1, 13, 1, 24), dActionEntry (276, 0, 1, 13, 1, 24), dActionEntry (277, 0, 1, 13, 1, 24), 
			dActionEntry (278, 0, 1, 13, 1, 24), dActionEntry (279, 0, 1, 13, 1, 24), dActionEntry (280, 0, 1, 13, 1, 24), dActionEntry (281, 0, 1, 13, 1, 24), 
			dActionEntry (282, 0, 1, 13, 1, 24), dActionEntry (283, 0, 1, 13, 1, 24), dActionEntry (284, 0, 1, 13, 1, 24), dActionEntry (285, 0, 1, 13, 1, 24), 
			dActionEntry (286, 0, 1, 13, 1, 24), dActionEntry (287, 0, 1, 13, 1, 24), dActionEntry (288, 0, 1, 13, 1, 24), dActionEntry (289, 0, 1, 13, 1, 24), 
			dActionEntry (290, 0, 1, 13, 1, 24), dActionEntry (291, 0, 1, 13, 1, 24), dActionEntry (292, 0, 1, 13, 1, 24), dActionEntry (293, 0, 1, 13, 1, 24), 
			dActionEntry (295, 0, 1, 13, 1, 24), dActionEntry (296, 0, 1, 13, 1, 24), dActionEntry (299, 0, 1, 13, 1, 24), dActionEntry (300, 0, 1, 13, 1, 24), 
			dActionEntry (301, 0, 1, 13, 1, 24), dActionEntry (264, 0, 0, 69, 0, 0), dActionEntry (258, 0, 0, 72, 0, 0), dActionEntry (261, 0, 1, 13, 1, 25), 
			dActionEntry (265, 0, 1, 13, 1, 25), dActionEntry (272, 0, 1, 13, 1, 25), dActionEntry (273, 0, 1, 13, 1, 25), dActionEntry (274, 0, 1, 13, 1, 25), 
			dActionEntry (275, 0, 1, 13, 1, 25), dActionEntry (276, 0, 1, 13, 1, 25), dActionEntry (277, 0, 1, 13, 1, 25), dActionEntry (278, 0, 1, 13, 1, 25), 
			dActionEntry (279, 0, 1, 13, 1, 25), dActionEntry (280, 0, 1, 13, 1, 25), dActionEntry (281, 0, 1, 13, 1, 25), dActionEntry (282, 0, 1, 13, 1, 25), 
			dActionEntry (283, 0, 1, 13, 1, 25), dActionEntry (284, 0, 1, 13, 1, 25), dActionEntry (285, 0, 1, 13, 1, 25), dActionEntry (286, 0, 1, 13, 1, 25), 
			dActionEntry (287, 0, 1, 13, 1, 25), dActionEntry (288, 0, 1, 13, 1, 25), dActionEntry (289, 0, 1, 13, 1, 25), dActionEntry (290, 0, 1, 13, 1, 25), 
			dActionEntry (291, 0, 1, 13, 1, 25), dActionEntry (292, 0, 1, 13, 1, 25), dActionEntry (293, 0, 1, 13, 1, 25), dActionEntry (295, 0, 1, 13, 1, 25), 
			dActionEntry (296, 0, 1, 13, 1, 25), dActionEntry (299, 0, 1, 13, 1, 25), dActionEntry (300, 0, 1, 13, 1, 25), dActionEntry (301, 0, 1, 13, 1, 25), 
			dActionEntry (264, 0, 1, 27, 1, 54), dActionEntry (264, 0, 1, 28, 1, 58), dActionEntry (264, 0, 1, 27, 1, 62), dActionEntry (264, 0, 1, 27, 1, 55), 
			dActionEntry (264, 0, 1, 27, 1, 61), dActionEntry (264, 0, 1, 27, 1, 60), dActionEntry (264, 0, 1, 27, 1, 44), dActionEntry (264, 0, 1, 27, 1, 45), 
			dActionEntry (264, 0, 1, 27, 1, 59), dActionEntry (264, 0, 1, 26, 1, 40), dActionEntry (264, 0, 1, 27, 1, 64), dActionEntry (264, 0, 1, 27, 1, 53), 
			dActionEntry (264, 0, 1, 25, 1, 38), dActionEntry (264, 0, 1, 27, 1, 49), dActionEntry (61, 0, 0, 76, 0, 0), dActionEntry (254, 0, 1, 4, 2, 10), 
			dActionEntry (256, 0, 1, 4, 2, 10), dActionEntry (260, 0, 1, 4, 2, 10), dActionEntry (262, 0, 1, 4, 2, 10), dActionEntry (61, 0, 1, 8, 1, 13), 
			dActionEntry (254, 0, 1, 8, 1, 13), dActionEntry (256, 0, 1, 8, 1, 13), dActionEntry (260, 0, 1, 8, 1, 13), dActionEntry (262, 0, 1, 8, 1, 13), 
			dActionEntry (259, 0, 0, 78, 0, 0), dActionEntry (265, 0, 1, 10, 0, 16), dActionEntry (272, 0, 1, 10, 0, 16), dActionEntry (273, 0, 1, 10, 0, 16), 
			dActionEntry (274, 0, 1, 10, 0, 16), dActionEntry (275, 0, 1, 10, 0, 16), dActionEntry (276, 0, 1, 10, 0, 16), dActionEntry (277, 0, 1, 10, 0, 16), 
			dActionEntry (278, 0, 1, 10, 0, 16), dActionEntry (279, 0, 1, 10, 0, 16), dActionEntry (280, 0, 1, 10, 0, 16), dActionEntry (281, 0, 1, 10, 0, 16), 
			dActionEntry (282, 0, 1, 10, 0, 16), dActionEntry (283, 0, 1, 10, 0, 16), dActionEntry (284, 0, 1, 10, 0, 16), dActionEntry (285, 0, 1, 10, 0, 16), 
			dActionEntry (286, 0, 1, 10, 0, 16), dActionEntry (287, 0, 1, 10, 0, 16), dActionEntry (288, 0, 1, 10, 0, 16), dActionEntry (289, 0, 1, 10, 0, 16), 
			dActionEntry (290, 0, 1, 10, 0, 16), dActionEntry (291, 0, 1, 10, 0, 16), dActionEntry (292, 0, 1, 10, 0, 16), dActionEntry (293, 0, 1, 10, 0, 16), 
			dActionEntry (295, 0, 1, 10, 0, 16), dActionEntry (296, 0, 1, 10, 0, 16), dActionEntry (299, 0, 1, 10, 0, 16), dActionEntry (300, 0, 1, 10, 0, 16), 
			dActionEntry (301, 0, 1, 10, 0, 16), dActionEntry (265, 0, 1, 8, 1, 13), dActionEntry (272, 0, 1, 8, 1, 13), dActionEntry (273, 0, 1, 8, 1, 13), 
			dActionEntry (274, 0, 1, 8, 1, 13), dActionEntry (275, 0, 1, 8, 1, 13), dActionEntry (276, 0, 1, 8, 1, 13), dActionEntry (277, 0, 1, 8, 1, 13), 
			dActionEntry (278, 0, 1, 8, 1, 13), dActionEntry (279, 0, 1, 8, 1, 13), dActionEntry (280, 0, 1, 8, 1, 13), dActionEntry (281, 0, 1, 8, 1, 13), 
			dActionEntry (282, 0, 1, 8, 1, 13), dActionEntry (283, 0, 1, 8, 1, 13), dActionEntry (284, 0, 1, 8, 1, 13), dActionEntry (285, 0, 1, 8, 1, 13), 
			dActionEntry (286, 0, 1, 8, 1, 13), dActionEntry (287, 0, 1, 8, 1, 13), dActionEntry (288, 0, 1, 8, 1, 13), dActionEntry (289, 0, 1, 8, 1, 13), 
			dActionEntry (290, 0, 1, 8, 1, 13), dActionEntry (291, 0, 1, 8, 1, 13), dActionEntry (292, 0, 1, 8, 1, 13), dActionEntry (293, 0, 1, 8, 1, 13), 
			dActionEntry (295, 0, 1, 8, 1, 13), dActionEntry (296, 0, 1, 8, 1, 13), dActionEntry (299, 0, 1, 8, 1, 13), dActionEntry (300, 0, 1, 8, 1, 13), 
			dActionEntry (301, 0, 1, 8, 1, 13), dActionEntry (257, 0, 0, 79, 0, 0), dActionEntry (254, 0, 1, 1, 2, 4), dActionEntry (256, 0, 1, 1, 2, 4), 
			dActionEntry (260, 0, 1, 1, 2, 4), dActionEntry (262, 0, 1, 1, 2, 4), dActionEntry (44, 0, 1, 24, 1, 36), dActionEntry (261, 0, 1, 24, 1, 36), 
			dActionEntry (265, 0, 1, 24, 1, 36), dActionEntry (272, 0, 1, 24, 1, 36), dActionEntry (273, 0, 1, 24, 1, 36), dActionEntry (274, 0, 1, 24, 1, 36), 
			dActionEntry (275, 0, 1, 24, 1, 36), dActionEntry (276, 0, 1, 24, 1, 36), dActionEntry (277, 0, 1, 24, 1, 36), dActionEntry (278, 0, 1, 24, 1, 36), 
			dActionEntry (279, 0, 1, 24, 1, 36), dActionEntry (280, 0, 1, 24, 1, 36), dActionEntry (281, 0, 1, 24, 1, 36), dActionEntry (282, 0, 1, 24, 1, 36), 
			dActionEntry (283, 0, 1, 24, 1, 36), dActionEntry (284, 0, 1, 24, 1, 36), dActionEntry (285, 0, 1, 24, 1, 36), dActionEntry (286, 0, 1, 24, 1, 36), 
			dActionEntry (287, 0, 1, 24, 1, 36), dActionEntry (288, 0, 1, 24, 1, 36), dActionEntry (289, 0, 1, 24, 1, 36), dActionEntry (290, 0, 1, 24, 1, 36), 
			dActionEntry (291, 0, 1, 24, 1, 36), dActionEntry (292, 0, 1, 24, 1, 36), dActionEntry (293, 0, 1, 24, 1, 36), dActionEntry (295, 0, 1, 24, 1, 36), 
			dActionEntry (296, 0, 1, 24, 1, 36), dActionEntry (299, 0, 1, 24, 1, 36), dActionEntry (300, 0, 1, 24, 1, 36), dActionEntry (301, 0, 1, 24, 1, 36), 
			dActionEntry (44, 0, 0, 80, 0, 0), dActionEntry (261, 0, 1, 17, 2, 42), dActionEntry (265, 0, 1, 17, 2, 42), dActionEntry (272, 0, 1, 17, 2, 42), 
			dActionEntry (273, 0, 1, 17, 2, 42), dActionEntry (274, 0, 1, 17, 2, 42), dActionEntry (275, 0, 1, 17, 2, 42), dActionEntry (276, 0, 1, 17, 2, 42), 
			dActionEntry (277, 0, 1, 17, 2, 42), dActionEntry (278, 0, 1, 17, 2, 42), dActionEntry (279, 0, 1, 17, 2, 42), dActionEntry (280, 0, 1, 17, 2, 42), 
			dActionEntry (281, 0, 1, 17, 2, 42), dActionEntry (282, 0, 1, 17, 2, 42), dActionEntry (283, 0, 1, 17, 2, 42), dActionEntry (284, 0, 1, 17, 2, 42), 
			dActionEntry (285, 0, 1, 17, 2, 42), dActionEntry (286, 0, 1, 17, 2, 42), dActionEntry (287, 0, 1, 17, 2, 42), dActionEntry (288, 0, 1, 17, 2, 42), 
			dActionEntry (289, 0, 1, 17, 2, 42), dActionEntry (290, 0, 1, 17, 2, 42), dActionEntry (291, 0, 1, 17, 2, 42), dActionEntry (292, 0, 1, 17, 2, 42), 
			dActionEntry (293, 0, 1, 17, 2, 42), dActionEntry (295, 0, 1, 17, 2, 42), dActionEntry (296, 0, 1, 17, 2, 42), dActionEntry (299, 0, 1, 17, 2, 42), 
			dActionEntry (300, 0, 1, 17, 2, 42), dActionEntry (301, 0, 1, 17, 2, 42), dActionEntry (44, 0, 1, 9, 1, 14), dActionEntry (261, 0, 1, 9, 1, 14), 
			dActionEntry (265, 0, 1, 9, 1, 14), dActionEntry (272, 0, 1, 9, 1, 14), dActionEntry (273, 0, 1, 9, 1, 14), dActionEntry (274, 0, 1, 9, 1, 14), 
			dActionEntry (275, 0, 1, 9, 1, 14), dActionEntry (276, 0, 1, 9, 1, 14), dActionEntry (277, 0, 1, 9, 1, 14), dActionEntry (278, 0, 1, 9, 1, 14), 
			dActionEntry (279, 0, 1, 9, 1, 14), dActionEntry (280, 0, 1, 9, 1, 14), dActionEntry (281, 0, 1, 9, 1, 14), dActionEntry (282, 0, 1, 9, 1, 14), 
			dActionEntry (283, 0, 1, 9, 1, 14), dActionEntry (284, 0, 1, 9, 1, 14), dActionEntry (285, 0, 1, 9, 1, 14), dActionEntry (286, 0, 1, 9, 1, 14), 
			dActionEntry (287, 0, 1, 9, 1, 14), dActionEntry (288, 0, 1, 9, 1, 14), dActionEntry (289, 0, 1, 9, 1, 14), dActionEntry (290, 0, 1, 9, 1, 14), 
			dActionEntry (291, 0, 1, 9, 1, 14), dActionEntry (292, 0, 1, 9, 1, 14), dActionEntry (293, 0, 1, 9, 1, 14), dActionEntry (295, 0, 1, 9, 1, 14), 
			dActionEntry (296, 0, 1, 9, 1, 14), dActionEntry (299, 0, 1, 9, 1, 14), dActionEntry (300, 0, 1, 9, 1, 14), dActionEntry (301, 0, 1, 9, 1, 14), 
			dActionEntry (44, 0, 0, 81, 0, 0), dActionEntry (44, 0, 1, 9, 1, 14), dActionEntry (261, 0, 1, 12, 2, 20), dActionEntry (265, 0, 1, 12, 2, 20), 
			dActionEntry (272, 0, 1, 12, 2, 20), dActionEntry (273, 0, 1, 12, 2, 20), dActionEntry (274, 0, 1, 12, 2, 20), dActionEntry (275, 0, 1, 12, 2, 20), 
			dActionEntry (276, 0, 1, 12, 2, 20), dActionEntry (277, 0, 1, 12, 2, 20), dActionEntry (278, 0, 1, 12, 2, 20), dActionEntry (279, 0, 1, 12, 2, 20), 
			dActionEntry (280, 0, 1, 12, 2, 20), dActionEntry (281, 0, 1, 12, 2, 20), dActionEntry (282, 0, 1, 12, 2, 20), dActionEntry (283, 0, 1, 12, 2, 20), 
			dActionEntry (284, 0, 1, 12, 2, 20), dActionEntry (285, 0, 1, 12, 2, 20), dActionEntry (286, 0, 1, 12, 2, 20), dActionEntry (287, 0, 1, 12, 2, 20), 
			dActionEntry (288, 0, 1, 12, 2, 20), dActionEntry (289, 0, 1, 12, 2, 20), dActionEntry (290, 0, 1, 12, 2, 20), dActionEntry (291, 0, 1, 12, 2, 20), 
			dActionEntry (292, 0, 1, 12, 2, 20), dActionEntry (293, 0, 1, 12, 2, 20), dActionEntry (295, 0, 1, 12, 2, 20), dActionEntry (296, 0, 1, 12, 2, 20), 
			dActionEntry (299, 0, 1, 12, 2, 20), dActionEntry (300, 0, 1, 12, 2, 20), dActionEntry (301, 0, 1, 12, 2, 20), dActionEntry (254, 0, 1, 5, 3, 17), 
			dActionEntry (256, 0, 1, 5, 3, 17), dActionEntry (260, 0, 1, 5, 3, 17), dActionEntry (262, 0, 1, 5, 3, 17), dActionEntry (261, 0, 1, 17, 2, 43), 
			dActionEntry (265, 0, 1, 17, 2, 43), dActionEntry (272, 0, 1, 17, 2, 43), dActionEntry (273, 0, 1, 17, 2, 43), dActionEntry (274, 0, 1, 17, 2, 43), 
			dActionEntry (275, 0, 1, 17, 2, 43), dActionEntry (276, 0, 1, 17, 2, 43), dActionEntry (277, 0, 1, 17, 2, 43), dActionEntry (278, 0, 1, 17, 2, 43), 
			dActionEntry (279, 0, 1, 17, 2, 43), dActionEntry (280, 0, 1, 17, 2, 43), dActionEntry (281, 0, 1, 17, 2, 43), dActionEntry (282, 0, 1, 17, 2, 43), 
			dActionEntry (283, 0, 1, 17, 2, 43), dActionEntry (284, 0, 1, 17, 2, 43), dActionEntry (285, 0, 1, 17, 2, 43), dActionEntry (286, 0, 1, 17, 2, 43), 
			dActionEntry (287, 0, 1, 17, 2, 43), dActionEntry (288, 0, 1, 17, 2, 43), dActionEntry (289, 0, 1, 17, 2, 43), dActionEntry (290, 0, 1, 17, 2, 43), 
			dActionEntry (291, 0, 1, 17, 2, 43), dActionEntry (292, 0, 1, 17, 2, 43), dActionEntry (293, 0, 1, 17, 2, 43), dActionEntry (295, 0, 1, 17, 2, 43), 
			dActionEntry (296, 0, 1, 17, 2, 43), dActionEntry (299, 0, 1, 17, 2, 43), dActionEntry (300, 0, 1, 17, 2, 43), dActionEntry (301, 0, 1, 17, 2, 43), 
			dActionEntry (261, 0, 1, 9, 1, 14), dActionEntry (265, 0, 1, 9, 1, 14), dActionEntry (272, 0, 1, 9, 1, 14), dActionEntry (273, 0, 1, 9, 1, 14), 
			dActionEntry (274, 0, 1, 9, 1, 14), dActionEntry (275, 0, 1, 9, 1, 14), dActionEntry (276, 0, 1, 9, 1, 14), dActionEntry (277, 0, 1, 9, 1, 14), 
			dActionEntry (278, 0, 1, 9, 1, 14), dActionEntry (279, 0, 1, 9, 1, 14), dActionEntry (280, 0, 1, 9, 1, 14), dActionEntry (281, 0, 1, 9, 1, 14), 
			dActionEntry (282, 0, 1, 9, 1, 14), dActionEntry (283, 0, 1, 9, 1, 14), dActionEntry (284, 0, 1, 9, 1, 14), dActionEntry (285, 0, 1, 9, 1, 14), 
			dActionEntry (286, 0, 1, 9, 1, 14), dActionEntry (287, 0, 1, 9, 1, 14), dActionEntry (288, 0, 1, 9, 1, 14), dActionEntry (289, 0, 1, 9, 1, 14), 
			dActionEntry (290, 0, 1, 9, 1, 14), dActionEntry (291, 0, 1, 9, 1, 14), dActionEntry (292, 0, 1, 9, 1, 14), dActionEntry (293, 0, 1, 9, 1, 14), 
			dActionEntry (295, 0, 1, 9, 1, 14), dActionEntry (296, 0, 1, 9, 1, 14), dActionEntry (299, 0, 1, 9, 1, 14), dActionEntry (300, 0, 1, 9, 1, 14), 
			dActionEntry (301, 0, 1, 9, 1, 14), dActionEntry (261, 0, 1, 16, 2, 35), dActionEntry (265, 0, 1, 16, 2, 35), dActionEntry (272, 0, 1, 16, 2, 35), 
			dActionEntry (273, 0, 1, 16, 2, 35), dActionEntry (274, 0, 1, 16, 2, 35), dActionEntry (275, 0, 1, 16, 2, 35), dActionEntry (276, 0, 1, 16, 2, 35), 
			dActionEntry (277, 0, 1, 16, 2, 35), dActionEntry (278, 0, 1, 16, 2, 35), dActionEntry (279, 0, 1, 16, 2, 35), dActionEntry (280, 0, 1, 16, 2, 35), 
			dActionEntry (281, 0, 1, 16, 2, 35), dActionEntry (282, 0, 1, 16, 2, 35), dActionEntry (283, 0, 1, 16, 2, 35), dActionEntry (284, 0, 1, 16, 2, 35), 
			dActionEntry (285, 0, 1, 16, 2, 35), dActionEntry (286, 0, 1, 16, 2, 35), dActionEntry (287, 0, 1, 16, 2, 35), dActionEntry (288, 0, 1, 16, 2, 35), 
			dActionEntry (289, 0, 1, 16, 2, 35), dActionEntry (290, 0, 1, 16, 2, 35), dActionEntry (291, 0, 1, 16, 2, 35), dActionEntry (292, 0, 1, 16, 2, 35), 
			dActionEntry (293, 0, 1, 16, 2, 35), dActionEntry (295, 0, 1, 16, 2, 35), dActionEntry (296, 0, 1, 16, 2, 35), dActionEntry (299, 0, 1, 16, 2, 35), 
			dActionEntry (300, 0, 1, 16, 2, 35), dActionEntry (301, 0, 1, 16, 2, 35), dActionEntry (261, 0, 1, 16, 2, 34), dActionEntry (265, 0, 1, 16, 2, 34), 
			dActionEntry (272, 0, 1, 16, 2, 34), dActionEntry (273, 0, 1, 16, 2, 34), dActionEntry (274, 0, 1, 16, 2, 34), dActionEntry (275, 0, 1, 16, 2, 34), 
			dActionEntry (276, 0, 1, 16, 2, 34), dActionEntry (277, 0, 1, 16, 2, 34), dActionEntry (278, 0, 1, 16, 2, 34), dActionEntry (279, 0, 1, 16, 2, 34), 
			dActionEntry (280, 0, 1, 16, 2, 34), dActionEntry (281, 0, 1, 16, 2, 34), dActionEntry (282, 0, 1, 16, 2, 34), dActionEntry (283, 0, 1, 16, 2, 34), 
			dActionEntry (284, 0, 1, 16, 2, 34), dActionEntry (285, 0, 1, 16, 2, 34), dActionEntry (286, 0, 1, 16, 2, 34), dActionEntry (287, 0, 1, 16, 2, 34), 
			dActionEntry (288, 0, 1, 16, 2, 34), dActionEntry (289, 0, 1, 16, 2, 34), dActionEntry (290, 0, 1, 16, 2, 34), dActionEntry (291, 0, 1, 16, 2, 34), 
			dActionEntry (292, 0, 1, 16, 2, 34), dActionEntry (293, 0, 1, 16, 2, 34), dActionEntry (295, 0, 1, 16, 2, 34), dActionEntry (296, 0, 1, 16, 2, 34), 
			dActionEntry (299, 0, 1, 16, 2, 34), dActionEntry (300, 0, 1, 16, 2, 34), dActionEntry (301, 0, 1, 16, 2, 34), dActionEntry (261, 0, 1, 8, 1, 13), 
			dActionEntry (265, 0, 1, 8, 1, 13), dActionEntry (272, 0, 1, 8, 1, 13), dActionEntry (273, 0, 1, 8, 1, 13), dActionEntry (274, 0, 1, 8, 1, 13), 
			dActionEntry (275, 0, 1, 8, 1, 13), dActionEntry (276, 0, 1, 8, 1, 13), dActionEntry (277, 0, 1, 8, 1, 13), dActionEntry (278, 0, 1, 8, 1, 13), 
			dActionEntry (279, 0, 1, 8, 1, 13), dActionEntry (280, 0, 1, 8, 1, 13), dActionEntry (281, 0, 1, 8, 1, 13), dActionEntry (282, 0, 1, 8, 1, 13), 
			dActionEntry (283, 0, 1, 8, 1, 13), dActionEntry (284, 0, 1, 8, 1, 13), dActionEntry (285, 0, 1, 8, 1, 13), dActionEntry (286, 0, 1, 8, 1, 13), 
			dActionEntry (287, 0, 1, 8, 1, 13), dActionEntry (288, 0, 1, 8, 1, 13), dActionEntry (289, 0, 1, 8, 1, 13), dActionEntry (290, 0, 1, 8, 1, 13), 
			dActionEntry (291, 0, 1, 8, 1, 13), dActionEntry (292, 0, 1, 8, 1, 13), dActionEntry (293, 0, 1, 8, 1, 13), dActionEntry (295, 0, 1, 8, 1, 13), 
			dActionEntry (296, 0, 1, 8, 1, 13), dActionEntry (299, 0, 1, 8, 1, 13), dActionEntry (300, 0, 1, 8, 1, 13), dActionEntry (301, 0, 1, 8, 1, 13), 
			dActionEntry (44, 0, 0, 82, 0, 0), dActionEntry (261, 0, 1, 16, 2, 33), dActionEntry (265, 0, 1, 16, 2, 33), dActionEntry (272, 0, 1, 16, 2, 33), 
			dActionEntry (273, 0, 1, 16, 2, 33), dActionEntry (274, 0, 1, 16, 2, 33), dActionEntry (275, 0, 1, 16, 2, 33), dActionEntry (276, 0, 1, 16, 2, 33), 
			dActionEntry (277, 0, 1, 16, 2, 33), dActionEntry (278, 0, 1, 16, 2, 33), dActionEntry (279, 0, 1, 16, 2, 33), dActionEntry (280, 0, 1, 16, 2, 33), 
			dActionEntry (281, 0, 1, 16, 2, 33), dActionEntry (282, 0, 1, 16, 2, 33), dActionEntry (283, 0, 1, 16, 2, 33), dActionEntry (284, 0, 1, 16, 2, 33), 
			dActionEntry (285, 0, 1, 16, 2, 33), dActionEntry (286, 0, 1, 16, 2, 33), dActionEntry (287, 0, 1, 16, 2, 33), dActionEntry (288, 0, 1, 16, 2, 33), 
			dActionEntry (289, 0, 1, 16, 2, 33), dActionEntry (290, 0, 1, 16, 2, 33), dActionEntry (291, 0, 1, 16, 2, 33), dActionEntry (292, 0, 1, 16, 2, 33), 
			dActionEntry (293, 0, 1, 16, 2, 33), dActionEntry (295, 0, 1, 16, 2, 33), dActionEntry (296, 0, 1, 16, 2, 33), dActionEntry (299, 0, 1, 16, 2, 33), 
			dActionEntry (300, 0, 1, 16, 2, 33), dActionEntry (301, 0, 1, 16, 2, 33), dActionEntry (261, 0, 1, 16, 2, 32), dActionEntry (265, 0, 1, 16, 2, 32), 
			dActionEntry (272, 0, 1, 16, 2, 32), dActionEntry (273, 0, 1, 16, 2, 32), dActionEntry (274, 0, 1, 16, 2, 32), dActionEntry (275, 0, 1, 16, 2, 32), 
			dActionEntry (276, 0, 1, 16, 2, 32), dActionEntry (277, 0, 1, 16, 2, 32), dActionEntry (278, 0, 1, 16, 2, 32), dActionEntry (279, 0, 1, 16, 2, 32), 
			dActionEntry (280, 0, 1, 16, 2, 32), dActionEntry (281, 0, 1, 16, 2, 32), dActionEntry (282, 0, 1, 16, 2, 32), dActionEntry (283, 0, 1, 16, 2, 32), 
			dActionEntry (284, 0, 1, 16, 2, 32), dActionEntry (285, 0, 1, 16, 2, 32), dActionEntry (286, 0, 1, 16, 2, 32), dActionEntry (287, 0, 1, 16, 2, 32), 
			dActionEntry (288, 0, 1, 16, 2, 32), dActionEntry (289, 0, 1, 16, 2, 32), dActionEntry (290, 0, 1, 16, 2, 32), dActionEntry (291, 0, 1, 16, 2, 32), 
			dActionEntry (292, 0, 1, 16, 2, 32), dActionEntry (293, 0, 1, 16, 2, 32), dActionEntry (295, 0, 1, 16, 2, 32), dActionEntry (296, 0, 1, 16, 2, 32), 
			dActionEntry (299, 0, 1, 16, 2, 32), dActionEntry (300, 0, 1, 16, 2, 32), dActionEntry (301, 0, 1, 16, 2, 32), dActionEntry (263, 0, 0, 84, 0, 0), 
			dActionEntry (265, 0, 1, 11, 3, 18), dActionEntry (272, 0, 1, 11, 3, 18), dActionEntry (273, 0, 1, 11, 3, 18), dActionEntry (274, 0, 1, 11, 3, 18), 
			dActionEntry (275, 0, 1, 11, 3, 18), dActionEntry (276, 0, 1, 11, 3, 18), dActionEntry (277, 0, 1, 11, 3, 18), dActionEntry (278, 0, 1, 11, 3, 18), 
			dActionEntry (279, 0, 1, 11, 3, 18), dActionEntry (280, 0, 1, 11, 3, 18), dActionEntry (281, 0, 1, 11, 3, 18), dActionEntry (282, 0, 1, 11, 3, 18), 
			dActionEntry (283, 0, 1, 11, 3, 18), dActionEntry (284, 0, 1, 11, 3, 18), dActionEntry (285, 0, 1, 11, 3, 18), dActionEntry (286, 0, 1, 11, 3, 18), 
			dActionEntry (287, 0, 1, 11, 3, 18), dActionEntry (288, 0, 1, 11, 3, 18), dActionEntry (289, 0, 1, 11, 3, 18), dActionEntry (290, 0, 1, 11, 3, 18), 
			dActionEntry (291, 0, 1, 11, 3, 18), dActionEntry (292, 0, 1, 11, 3, 18), dActionEntry (293, 0, 1, 11, 3, 18), dActionEntry (295, 0, 1, 11, 3, 18), 
			dActionEntry (296, 0, 1, 11, 3, 18), dActionEntry (299, 0, 1, 11, 3, 18), dActionEntry (300, 0, 1, 11, 3, 18), dActionEntry (301, 0, 1, 11, 3, 18), 
			dActionEntry (265, 0, 1, 10, 1, 15), dActionEntry (272, 0, 1, 10, 1, 15), dActionEntry (273, 0, 1, 10, 1, 15), dActionEntry (274, 0, 1, 10, 1, 15), 
			dActionEntry (275, 0, 1, 10, 1, 15), dActionEntry (276, 0, 1, 10, 1, 15), dActionEntry (277, 0, 1, 10, 1, 15), dActionEntry (278, 0, 1, 10, 1, 15), 
			dActionEntry (279, 0, 1, 10, 1, 15), dActionEntry (280, 0, 1, 10, 1, 15), dActionEntry (281, 0, 1, 10, 1, 15), dActionEntry (282, 0, 1, 10, 1, 15), 
			dActionEntry (283, 0, 1, 10, 1, 15), dActionEntry (284, 0, 1, 10, 1, 15), dActionEntry (285, 0, 1, 10, 1, 15), dActionEntry (286, 0, 1, 10, 1, 15), 
			dActionEntry (287, 0, 1, 10, 1, 15), dActionEntry (288, 0, 1, 10, 1, 15), dActionEntry (289, 0, 1, 10, 1, 15), dActionEntry (290, 0, 1, 10, 1, 15), 
			dActionEntry (291, 0, 1, 10, 1, 15), dActionEntry (292, 0, 1, 10, 1, 15), dActionEntry (293, 0, 1, 10, 1, 15), dActionEntry (295, 0, 1, 10, 1, 15), 
			dActionEntry (296, 0, 1, 10, 1, 15), dActionEntry (299, 0, 1, 10, 1, 15), dActionEntry (300, 0, 1, 10, 1, 15), dActionEntry (301, 0, 1, 10, 1, 15), 
			dActionEntry (62, 0, 0, 85, 0, 0), dActionEntry (263, 0, 0, 89, 0, 0), dActionEntry (264, 0, 0, 65, 0, 0), dActionEntry (264, 0, 0, 91, 0, 0), 
			dActionEntry (254, 0, 1, 4, 4, 11), dActionEntry (256, 0, 1, 4, 4, 11), dActionEntry (260, 0, 1, 4, 4, 11), dActionEntry (262, 0, 1, 4, 4, 11), 
			dActionEntry (254, 0, 1, 6, 1, 9), dActionEntry (256, 0, 1, 6, 1, 9), dActionEntry (260, 0, 1, 6, 1, 9), dActionEntry (262, 0, 1, 6, 1, 9), 
			dActionEntry (254, 0, 1, 3, 4, 8), dActionEntry (256, 0, 1, 3, 4, 8), dActionEntry (260, 0, 1, 3, 4, 8), dActionEntry (262, 0, 1, 3, 4, 8), 
			dActionEntry (44, 0, 1, 24, 3, 37), dActionEntry (261, 0, 1, 24, 3, 37), dActionEntry (265, 0, 1, 24, 3, 37), dActionEntry (272, 0, 1, 24, 3, 37), 
			dActionEntry (273, 0, 1, 24, 3, 37), dActionEntry (274, 0, 1, 24, 3, 37), dActionEntry (275, 0, 1, 24, 3, 37), dActionEntry (276, 0, 1, 24, 3, 37), 
			dActionEntry (277, 0, 1, 24, 3, 37), dActionEntry (278, 0, 1, 24, 3, 37), dActionEntry (279, 0, 1, 24, 3, 37), dActionEntry (280, 0, 1, 24, 3, 37), 
			dActionEntry (281, 0, 1, 24, 3, 37), dActionEntry (282, 0, 1, 24, 3, 37), dActionEntry (283, 0, 1, 24, 3, 37), dActionEntry (284, 0, 1, 24, 3, 37), 
			dActionEntry (285, 0, 1, 24, 3, 37), dActionEntry (286, 0, 1, 24, 3, 37), dActionEntry (287, 0, 1, 24, 3, 37), dActionEntry (288, 0, 1, 24, 3, 37), 
			dActionEntry (289, 0, 1, 24, 3, 37), dActionEntry (290, 0, 1, 24, 3, 37), dActionEntry (291, 0, 1, 24, 3, 37), dActionEntry (292, 0, 1, 24, 3, 37), 
			dActionEntry (293, 0, 1, 24, 3, 37), dActionEntry (295, 0, 1, 24, 3, 37), dActionEntry (296, 0, 1, 24, 3, 37), dActionEntry (299, 0, 1, 24, 3, 37), 
			dActionEntry (300, 0, 1, 24, 3, 37), dActionEntry (301, 0, 1, 24, 3, 37), dActionEntry (44, 0, 0, 92, 0, 0), dActionEntry (261, 0, 1, 19, 4, 65), 
			dActionEntry (265, 0, 1, 19, 4, 65), dActionEntry (272, 0, 1, 19, 4, 65), dActionEntry (273, 0, 1, 19, 4, 65), dActionEntry (274, 0, 1, 19, 4, 65), 
			dActionEntry (275, 0, 1, 19, 4, 65), dActionEntry (276, 0, 1, 19, 4, 65), dActionEntry (277, 0, 1, 19, 4, 65), dActionEntry (278, 0, 1, 19, 4, 65), 
			dActionEntry (279, 0, 1, 19, 4, 65), dActionEntry (280, 0, 1, 19, 4, 65), dActionEntry (281, 0, 1, 19, 4, 65), dActionEntry (282, 0, 1, 19, 4, 65), 
			dActionEntry (283, 0, 1, 19, 4, 65), dActionEntry (284, 0, 1, 19, 4, 65), dActionEntry (285, 0, 1, 19, 4, 65), dActionEntry (286, 0, 1, 19, 4, 65), 
			dActionEntry (287, 0, 1, 19, 4, 65), dActionEntry (288, 0, 1, 19, 4, 65), dActionEntry (289, 0, 1, 19, 4, 65), dActionEntry (290, 0, 1, 19, 4, 65), 
			dActionEntry (291, 0, 1, 19, 4, 65), dActionEntry (292, 0, 1, 19, 4, 65), dActionEntry (293, 0, 1, 19, 4, 65), dActionEntry (295, 0, 1, 19, 4, 65), 
			dActionEntry (296, 0, 1, 19, 4, 65), dActionEntry (299, 0, 1, 19, 4, 65), dActionEntry (300, 0, 1, 19, 4, 65), dActionEntry (301, 0, 1, 19, 4, 65), 
			dActionEntry (261, 0, 1, 6, 1, 9), dActionEntry (265, 0, 1, 6, 1, 9), dActionEntry (272, 0, 1, 6, 1, 9), dActionEntry (273, 0, 1, 6, 1, 9), 
			dActionEntry (274, 0, 1, 6, 1, 9), dActionEntry (275, 0, 1, 6, 1, 9), dActionEntry (276, 0, 1, 6, 1, 9), dActionEntry (277, 0, 1, 6, 1, 9), 
			dActionEntry (278, 0, 1, 6, 1, 9), dActionEntry (279, 0, 1, 6, 1, 9), dActionEntry (280, 0, 1, 6, 1, 9), dActionEntry (281, 0, 1, 6, 1, 9), 
			dActionEntry (282, 0, 1, 6, 1, 9), dActionEntry (283, 0, 1, 6, 1, 9), dActionEntry (284, 0, 1, 6, 1, 9), dActionEntry (285, 0, 1, 6, 1, 9), 
			dActionEntry (286, 0, 1, 6, 1, 9), dActionEntry (287, 0, 1, 6, 1, 9), dActionEntry (288, 0, 1, 6, 1, 9), dActionEntry (289, 0, 1, 6, 1, 9), 
			dActionEntry (290, 0, 1, 6, 1, 9), dActionEntry (291, 0, 1, 6, 1, 9), dActionEntry (292, 0, 1, 6, 1, 9), dActionEntry (293, 0, 1, 6, 1, 9), 
			dActionEntry (295, 0, 1, 6, 1, 9), dActionEntry (296, 0, 1, 6, 1, 9), dActionEntry (299, 0, 1, 6, 1, 9), dActionEntry (300, 0, 1, 6, 1, 9), 
			dActionEntry (301, 0, 1, 6, 1, 9), dActionEntry (44, 0, 0, 93, 0, 0), dActionEntry (261, 0, 1, 18, 4, 57), dActionEntry (265, 0, 1, 18, 4, 57), 
			dActionEntry (272, 0, 1, 18, 4, 57), dActionEntry (273, 0, 1, 18, 4, 57), dActionEntry (274, 0, 1, 18, 4, 57), dActionEntry (275, 0, 1, 18, 4, 57), 
			dActionEntry (276, 0, 1, 18, 4, 57), dActionEntry (277, 0, 1, 18, 4, 57), dActionEntry (278, 0, 1, 18, 4, 57), dActionEntry (279, 0, 1, 18, 4, 57), 
			dActionEntry (280, 0, 1, 18, 4, 57), dActionEntry (281, 0, 1, 18, 4, 57), dActionEntry (282, 0, 1, 18, 4, 57), dActionEntry (283, 0, 1, 18, 4, 57), 
			dActionEntry (284, 0, 1, 18, 4, 57), dActionEntry (285, 0, 1, 18, 4, 57), dActionEntry (286, 0, 1, 18, 4, 57), dActionEntry (287, 0, 1, 18, 4, 57), 
			dActionEntry (288, 0, 1, 18, 4, 57), dActionEntry (289, 0, 1, 18, 4, 57), dActionEntry (290, 0, 1, 18, 4, 57), dActionEntry (291, 0, 1, 18, 4, 57), 
			dActionEntry (292, 0, 1, 18, 4, 57), dActionEntry (293, 0, 1, 18, 4, 57), dActionEntry (295, 0, 1, 18, 4, 57), dActionEntry (296, 0, 1, 18, 4, 57), 
			dActionEntry (299, 0, 1, 18, 4, 57), dActionEntry (300, 0, 1, 18, 4, 57), dActionEntry (301, 0, 1, 18, 4, 57), dActionEntry (263, 0, 0, 89, 0, 0), 
			dActionEntry (261, 0, 1, 19, 6, 66), dActionEntry (265, 0, 1, 19, 6, 66), dActionEntry (272, 0, 1, 19, 6, 66), dActionEntry (273, 0, 1, 19, 6, 66), 
			dActionEntry (274, 0, 1, 19, 6, 66), dActionEntry (275, 0, 1, 19, 6, 66), dActionEntry (276, 0, 1, 19, 6, 66), dActionEntry (277, 0, 1, 19, 6, 66), 
			dActionEntry (278, 0, 1, 19, 6, 66), dActionEntry (279, 0, 1, 19, 6, 66), dActionEntry (280, 0, 1, 19, 6, 66), dActionEntry (281, 0, 1, 19, 6, 66), 
			dActionEntry (282, 0, 1, 19, 6, 66), dActionEntry (283, 0, 1, 19, 6, 66), dActionEntry (284, 0, 1, 19, 6, 66), dActionEntry (285, 0, 1, 19, 6, 66), 
			dActionEntry (286, 0, 1, 19, 6, 66), dActionEntry (287, 0, 1, 19, 6, 66), dActionEntry (288, 0, 1, 19, 6, 66), dActionEntry (289, 0, 1, 19, 6, 66), 
			dActionEntry (290, 0, 1, 19, 6, 66), dActionEntry (291, 0, 1, 19, 6, 66), dActionEntry (292, 0, 1, 19, 6, 66), dActionEntry (293, 0, 1, 19, 6, 66), 
			dActionEntry (295, 0, 1, 19, 6, 66), dActionEntry (296, 0, 1, 19, 6, 66), dActionEntry (299, 0, 1, 19, 6, 66), dActionEntry (300, 0, 1, 19, 6, 66), 
			dActionEntry (301, 0, 1, 19, 6, 66), dActionEntry (261, 0, 1, 19, 6, 67), dActionEntry (265, 0, 1, 19, 6, 67), dActionEntry (272, 0, 1, 19, 6, 67), 
			dActionEntry (273, 0, 1, 19, 6, 67), dActionEntry (274, 0, 1, 19, 6, 67), dActionEntry (275, 0, 1, 19, 6, 67), dActionEntry (276, 0, 1, 19, 6, 67), 
			dActionEntry (277, 0, 1, 19, 6, 67), dActionEntry (278, 0, 1, 19, 6, 67), dActionEntry (279, 0, 1, 19, 6, 67), dActionEntry (280, 0, 1, 19, 6, 67), 
			dActionEntry (281, 0, 1, 19, 6, 67), dActionEntry (282, 0, 1, 19, 6, 67), dActionEntry (283, 0, 1, 19, 6, 67), dActionEntry (284, 0, 1, 19, 6, 67), 
			dActionEntry (285, 0, 1, 19, 6, 67), dActionEntry (286, 0, 1, 19, 6, 67), dActionEntry (287, 0, 1, 19, 6, 67), dActionEntry (288, 0, 1, 19, 6, 67), 
			dActionEntry (289, 0, 1, 19, 6, 67), dActionEntry (290, 0, 1, 19, 6, 67), dActionEntry (291, 0, 1, 19, 6, 67), dActionEntry (292, 0, 1, 19, 6, 67), 
			dActionEntry (293, 0, 1, 19, 6, 67), dActionEntry (295, 0, 1, 19, 6, 67), dActionEntry (296, 0, 1, 19, 6, 67), dActionEntry (299, 0, 1, 19, 6, 67), 
			dActionEntry (300, 0, 1, 19, 6, 67), dActionEntry (301, 0, 1, 19, 6, 67)};

	bool errorMode = false;
	const dStackPair& stackTop = stack.GetLast()->GetInfo();
	int state = stackTop.m_state;
	int start = actionsStart[state];
	int count = actionsCount[state];

	const dActionEntry* const table = &actionTable[start];
	const dActionEntry* action = FindAction (table, count, token);
	while (!action && (stack.GetCount() > 1)) {
		errorMode = true; 

		// we found a syntax error, go into error recovering mode and find the token mark by a ". error" rule
		stack.Remove (stack.GetLast());

		const dStackPair& stackTop = stack.GetLast()->GetInfo();
		int state = stackTop.m_state;
		int start = actionsStart[state];
		int count = actionsCount[state];
		const dActionEntry* const table = &actionTable[start];
		action = FindAction (table, count, ERROR_TOKEN);
		if (action && !action->m_errorRule) {
			action = NULL;
		}
	}

	if (errorMode && action) {
		dStackPair& stackTop = stack.GetLast()->GetInfo();
		stackTop.m_token = ERROR_TOKEN;

		int state = action->m_nextState;
		int start = actionsStart[state];
		int count = actionsCount[state];
		const dActionEntry* const table = &actionTable[start];

		// find the next viable token to continues parsing
		while (!FindAction (table, count, token)) {
			token = dToken (scanner.NextToken());
			if (token == -1) {
				// reached end of the file, can not recover from this error;
				return NULL;
			}
		}
		action = FindAction (table, count, token);
		
		dStackPair& entry = stack.Append()->GetInfo();
		entry.m_state = state;
		entry.m_scannerLine = stackTop.m_scannerLine;
		entry.m_scannerIndex = stackTop.m_scannerIndex;
		entry.m_value = dUserVariable (ERROR_TOKEN, "error", entry.m_scannerLine, entry.m_scannerIndex);
		entry.m_token = token;
	}

	return action;
}


bool dAssemblerParser::Parse(dAssemblerLexical& scanner)
{
	static short gotoCount[] = {
			8, 15, 0, 0, 0, 1, 0, 0, 1, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 2, 0, 0, 1, 0, 14, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 
			0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 1, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
	static short gotoStart[] = {
			0, 8, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 
			31, 31, 33, 33, 33, 34, 34, 48, 48, 48, 48, 48, 48, 49, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 
			52, 52, 52, 52, 52, 52, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 
			55, 55, 55, 55, 55, 56, 56, 56, 56, 57, 59, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 61, 62, 62};
	static dGotoEntry gotoTable[] = {
			dGotoEntry (302, 3), dGotoEntry (303, 11), dGotoEntry (304, 6), dGotoEntry (305, 10), dGotoEntry (306, 4), 
			dGotoEntry (307, 2), dGotoEntry (309, 5), dGotoEntry (313, 1), dGotoEntry (314, 30), dGotoEntry (315, 14), 
			dGotoEntry (316, 27), dGotoEntry (317, 34), dGotoEntry (318, 13), dGotoEntry (319, 35), dGotoEntry (320, 38), 
			dGotoEntry (321, 31), dGotoEntry (322, 19), dGotoEntry (323, 53), dGotoEntry (324, 37), dGotoEntry (327, 25), 
			dGotoEntry (328, 36), dGotoEntry (329, 47), dGotoEntry (330, 28), dGotoEntry (310, 55), dGotoEntry (310, 57), 
			dGotoEntry (304, 60), dGotoEntry (305, 10), dGotoEntry (306, 4), dGotoEntry (307, 2), dGotoEntry (309, 5), 
			dGotoEntry (313, 1), dGotoEntry (311, 61), dGotoEntry (326, 62), dGotoEntry (311, 64), dGotoEntry (315, 66), 
			dGotoEntry (316, 27), dGotoEntry (317, 34), dGotoEntry (318, 13), dGotoEntry (319, 35), dGotoEntry (320, 38), 
			dGotoEntry (321, 31), dGotoEntry (322, 19), dGotoEntry (323, 53), dGotoEntry (324, 37), dGotoEntry (327, 25), 
			dGotoEntry (328, 36), dGotoEntry (329, 47), dGotoEntry (330, 28), dGotoEntry (311, 68), dGotoEntry (310, 70), 
			dGotoEntry (325, 71), dGotoEntry (311, 73), dGotoEntry (310, 74), dGotoEntry (325, 75), dGotoEntry (312, 77), 
			dGotoEntry (308, 83), dGotoEntry (311, 86), dGotoEntry (308, 88), dGotoEntry (311, 87), dGotoEntry (311, 90), 
			dGotoEntry (308, 94), dGotoEntry (310, 95)};

	dList<dStackPair> stack;
	const int lastToken = 302;
	
	stack.Append ();
	m_grammarError = false;
	dToken token = dToken (scanner.NextToken());

	#ifdef D_DEBUG_PARCEL
	int oldLine = scanner.GetLineNumber();
	#endif

	for (bool terminate = false; !terminate;) {

		const dActionEntry* const action = GetNextAction (stack, token, scanner);
		if (!action) {
			terminate = true;
			fprintf (stderr, "unrecoverable parser error\n");
			dTrace (("unrecoverable parser error\n"));
		} else {
			switch (action->m_stateType) 
			{
				case dSHIFT: 
				{
					dStackPair& entry = stack.Append()->GetInfo();
					entry.m_state = action->m_nextState;
					entry.m_scannerLine = scanner.GetLineNumber();
					entry.m_scannerIndex = scanner.GetIndex();
					entry.m_value = dUserVariable (token, scanner.GetTokenString(), entry.m_scannerLine, entry.m_scannerIndex);

					#ifdef D_DEBUG_PARCEL
					if (scanner.GetLineNumber() != oldLine) {
						oldLine = scanner.GetLineNumber();
						dTrace (("\n"));
					}
					dTrace (("%s ", scanner.GetTokenString()));
					#endif
					
					token = dToken (scanner.NextToken());

					entry.m_token = token;
					if (token == -1) {
						token = ACCEPTING_TOKEN;
					}

					break;
				}

				case dREDUCE: 
				{
					dStackPair parameter[MAX_USER_PARAM];

					int reduceCount = action->m_ruleSymbols;
					dAssert (reduceCount < sizeof (parameter) / sizeof (parameter[0]));

					for (int i = 0; i < reduceCount; i ++) {
						parameter[reduceCount - i - 1] = stack.GetLast()->GetInfo();
						stack.Remove (stack.GetLast());
					}

					const dStackPair& stackTop = stack.GetLast()->GetInfo();
					int start = gotoStart[stackTop.m_state];
					int count = gotoCount[stackTop.m_state];
					const dGotoEntry* const table = &gotoTable[start];
					const dGotoEntry* const gotoEntry = FindGoto (table, count, dToken (action->m_nextState + lastToken));

					dStackPair& entry = stack.Append()->GetInfo();
					entry.m_state = gotoEntry->m_nextState;
					entry.m_scannerLine = scanner.GetLineNumber();
					entry.m_scannerIndex = scanner.GetIndex();
					entry.m_token = dToken (gotoEntry->m_token);
					
					switch (action->m_ruleIndex) 
					{
						//do user semantic Actions
						case 12:// dataType : INT 
{entry.m_value = ((dAssemblerCompiler*)this)->EmitDataType (parameter[0].m_value);}
break;

						case 39:// pushAndPop : POP 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::pop;}
break;

						case 28:// opcodeType0 : RET 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::ret;}
break;

						case 31:// opcodeType1_jump : JUMP 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::jump;}
break;

						case 48:// opcodeType4 : DIV 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::div;}
break;

						case 63:// opcodeType4 : BGT 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::bgt;}
break;

						case 29:// instructionType0 : opcodeType0 
{
			     		dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType0 (parameter[0].m_value);
				   }
break;

						case 50:// opcodeType4 : NEG 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::neg;}
break;

						case 56:// opcodeType4 : SRL 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::srl;}
break;

						case 46:// opcodeType4 : SUB 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::sub;}
break;

						case 52:// opcodeType4 : OR 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::or;}
break;

						case 41:// opcodeType2 : CALLR 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::callr;}
break;

						case 30:// opcodeType1 : CALL 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::call;}
break;

						case 47:// opcodeType4 : MUL 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::mul;}
break;

						case 51:// opcodeType4 : AND 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::and;}
break;

						case 27:// localLabel : LOCALLABEL 
{ 
				dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
				me->EmitLocalLabel (parameter[0].m_value);
			}
break;

						case 54:// opcodeType4 : NOT 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::not;}
break;

						case 58:// opcodeAddi : ADDI 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::addi;}
break;

						case 62:// opcodeType4 : BLE 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::ble;}
break;

						case 55:// opcodeType4 : SLL 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::sll;}
break;

						case 61:// opcodeType4 : BLT 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::blt;}
break;

						case 60:// opcodeType4 : BNE 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::bne;}
break;

						case 44:// opcodeType4 : MOVE 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::mov;}
break;

						case 45:// opcodeType4 : ADD 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::add;}
break;

						case 59:// opcodeType4 : BEQ 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::beq;}
break;

						case 40:// opcodeType2 : JUMPR 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::jumpr;}
break;

						case 64:// opcodeType4 : BGE 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::bge;}
break;

						case 53:// opcodeType4 : XOR 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::xor;}
break;

						case 38:// pushAndPop : PUSH 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::push;}
break;

						case 49:// opcodeType4 : ABS 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = dVirtualMachine::abs;}
break;

						case 10:// dataDeclaration : dataType literal 
{((dAssemblerCompiler*)this)->EmitUnInitilizedDataDeclaration (parameter[0].m_value, parameter[1].m_value);}
break;

						case 13:// literal : LITERAL 
{entry.m_value = ((dAssemblerCompiler*)this)->EmitSymbol (parameter[0].m_value);}
break;

						case 36:// registerMask : register 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = 1 << parameter[0].m_value.m_semanticValue;}
break;

						case 42:// instructionType2 : pushAndPop registerMask 
{
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitPushAndPop (parameter[0].m_value, parameter[1].m_value);
					}
break;

						case 14:// register : REGISTER 
{
				dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
				entry.m_value = me->TypeCheckRegister (parameter[0].m_value);
			}
break;

						case 17:// functionDeclaration : beginFunction instructionList END 
{
							dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
							me->EmitEndFunction ();
						}
break;

						case 43:// instructionType2 : opcodeType2 register 
{
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType2 (parameter[0].m_value, parameter[1].m_value);
					}
break;

						case 35:// instructionType1 : opcodeType1_jump literal 
{
			     		dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType1_saveLocalAdress (parameter[0].m_value, parameter[1].m_value);
				   }
break;

						case 34:// instructionType1 : opcodeType1_jump immidiate 
{
			     		dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType1 (parameter[0].m_value, parameter[1].m_value);
				   }
break;

						case 33:// instructionType1 : opcodeType1 literal 
{
			     		dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType1_saveGlobalAdress (parameter[0].m_value, parameter[1].m_value);
				   }
break;

						case 32:// instructionType1 : opcodeType1 immidiate 
{
			     		dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType1 (parameter[0].m_value, parameter[1].m_value);
				   }
break;

						case 18:// beginFunction : BEGIN literal visibility 
{ 
					dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
					me->EmitBeginFunction (parameter[1].m_value, parameter[2].m_value);
				}
break;

						case 11:// dataDeclaration : dataType literal = constantExpression 
{((dAssemblerCompiler*)this)->EmitInitilizedDataDeclaration (parameter[0].m_value, parameter[1].m_value, parameter[3].m_value);}
break;

						case 9:// constantExpression : INTEGER 
{entry.m_value = ((dAssemblerCompiler*)this)->EmitIntegerConst (parameter[0].m_value);}
break;

						case 37:// registerMask : registerMask , register 
{entry.m_value = parameter[0].m_value; entry.m_value.m_semanticValue = parameter[0].m_value.m_semanticValue | (1 << parameter[2].m_value.m_semanticValue);}
break;

						case 65:// instructionType4 : opcodeAddi register , constantExpression 
{ 
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						dUserVariable register0 (REGISTER, "r0");
						me->EmitInstructionType4 (parameter[0].m_value, parameter[1].m_value, register0, parameter[3].m_value);
					}
break;

						case 57:// instructionType3 : opcodeType4 register , register 
{ 
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType3 (parameter[0].m_value, parameter[1].m_value, parameter[3].m_value);
					}
break;

						case 66:// instructionType4 : opcodeAddi register , register , constantExpression 
{ 
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType4 (parameter[0].m_value, parameter[1].m_value, parameter[3].m_value, parameter[5].m_value);
					}
break;

						case 67:// instructionType4 : opcodeType4 register , register , literal 
{									 
						dAssemblerCompiler* const me = (dAssemblerCompiler*) this;
						me->EmitInstructionType4_saveLocalAdress (parameter[0].m_value, parameter[1].m_value, parameter[3].m_value, parameter[5].m_value);
					}
break;


						default:;
					}
					break;
				}
		
				case dACCEPT: // 2 = accept
				{
					// program parsed successfully, exit with successful code
					terminate = true;
					break;
				}
				
				default:  
				{
					dAssert (0);
					// syntax error parsing program
					//if (!ErrorHandler ("error")) {
					//}
					terminate = true;
					m_grammarError = true;
					break;
				}
			}
		}
	}
	return !m_grammarError;
}



