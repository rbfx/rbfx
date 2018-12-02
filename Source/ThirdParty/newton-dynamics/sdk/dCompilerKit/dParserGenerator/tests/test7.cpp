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
//Auto generated Parser Generator class: test7.cpp
//


#include <stdafx.h>
#include "main.h"
#include "Lexical.h"

//#define YYDEBUG 1
//#define YYERROR_VERBOSE
//#define YYSTYPE LexicalData

//static	int tab;
//static void GenerateTabs (char* const tabs)
//{
//	tabs[0] = 0;
//	for (int i = 0; i < tab; i ++) {
//		strcat (tabs, "\t");   
//	}
//}
//extern char yytext[];
//int yyerror(char *s)
//{
//	fflush(stdout);
//	printf("\n%*s\n%*s\n", column, "^", column, s);
//	return 0;
//}

#include "test7.h"
#include <dList.h>

#define MAX_USER_PARAM	64

enum test7::ActionType
{
	dSHIFT = 0,
	dREDUCE,
	dACCEPT,
	dERROR
};


class test7::dActionEntry
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

class test7::dGotoEntry
{
	public:
	dGotoEntry (short token, short nextState)
		:m_token(token), m_nextState(nextState)
	{
	}

	short  m_token;
	short  m_nextState;
};



class test7::dStackPair
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


test7::test7()
{
}

test7::~test7()
{
}


const test7::dActionEntry* test7::FindAction (const dActionEntry* const actionList, int count, dToken token) const
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

const test7::dGotoEntry* test7::FindGoto (const dGotoEntry* const gotoList, int count, dToken token) const
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

	_ASSERT (0);
	return NULL;
}



const test7::dActionEntry* test7::GetNextAction (dList<dStackPair>& stack, dToken token, xxxx& scanner) const
{
	static short actionsCount[] = {
			1, 1, 1, 8, 2, 8, 1, 8, 1, 8, 8, 6, 1, 8, 1, 1, 7, 8, 6, 8, 1, 1, 1, 8, 
			8, 8, 8, 8, 1, 1, 10, 1, 8, 5, 3, 1, 2, 2, 2, 2, 2, 5, 1, 2, 1, 8, 1, 7, 
			9, 3, 2, 2, 2, 2, 2, 4, 1, 2, 1, 1, 7, 9, 9, 9, 1, 8, 1, 8, 8, 8, 1, 2, 
			1, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 5, 2, 2, 2, 2, 2, 2, 2, 1, 1, 
			1, 7, 7, 1, 1, 1, 8, 9, 2, 2, 9, 8, 8, 3, 2, 1, 3, 8, 5, 8, 1, 1, 1, 1, 
			1, 1, 2, 1, 1, 1, 7, 7, 1, 8, 8, 8, 8, 9, 1, 1, 9, 1, 2, 2, 1, 2, 1, 1, 
			1, 1, 2, 1, 1, 1, 1, 1, 9, 1, 3, 9, 9, 1, 7, 7, 1, 1, 8, 7, 1, 9, 1, 8, 
			2, 2, 8, 8, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 7, 1, 8, 9, 9, 2, 1, 1, 1, 
			1, 1, 1, 1, 8, 9, 9, 9, 8, 1, 9, 1, 2, 1, 1, 1, 1, 8, 1, 1, 7, 1, 1, 9};
	static short actionsStart[] = {
			0, 1, 2, 3, 11, 13, 21, 22, 30, 31, 39, 47, 53, 54, 62, 63, 64, 71, 79, 85, 93, 94, 95, 96, 
			104, 112, 120, 128, 136, 137, 138, 148, 149, 157, 162, 165, 166, 168, 170, 172, 174, 176, 181, 182, 184, 185, 193, 64, 
			194, 203, 206, 208, 210, 212, 214, 216, 220, 221, 223, 224, 225, 232, 241, 250, 259, 120, 260, 261, 269, 277, 285, 286, 
			288, 289, 291, 292, 293, 295, 296, 297, 299, 301, 303, 305, 307, 309, 310, 315, 317, 319, 321, 323, 325, 327, 329, 330, 
			331, 332, 339, 346, 347, 348, 349, 357, 366, 368, 370, 379, 387, 395, 398, 400, 401, 404, 412, 417, 425, 426, 427, 428, 
			429, 430, 431, 433, 434, 435, 436, 443, 450, 451, 459, 467, 475, 483, 492, 493, 494, 503, 504, 506, 508, 509, 511, 512, 
			513, 514, 515, 517, 518, 519, 520, 521, 522, 531, 401, 532, 541, 550, 551, 558, 565, 566, 567, 575, 582, 583, 592, 593, 
			601, 603, 605, 613, 621, 622, 401, 623, 624, 625, 626, 627, 628, 629, 630, 631, 638, 639, 647, 656, 665, 667, 668, 669, 
			670, 671, 672, 673, 674, 682, 691, 700, 709, 717, 718, 727, 728, 730, 731, 732, 733, 734, 742, 743, 744, 751, 752, 753};
	static dActionEntry actionTable[] = {
			dActionEntry (268, 0, 0, 2, 0, 0), dActionEntry (254, 0, 2, 0, 0, 0), dActionEntry (274, 0, 0, 3, 0, 0), dActionEntry (264, 0, 0, 6, 0, 0), 
			dActionEntry (265, 0, 0, 14, 0, 0), dActionEntry (266, 0, 0, 16, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), dActionEntry (268, 0, 0, 20, 0, 0), 
			dActionEntry (271, 0, 0, 8, 0, 0), dActionEntry (272, 0, 0, 4, 0, 0), dActionEntry (274, 0, 0, 11, 0, 0), dActionEntry (60, 0, 0, 28, 0, 0), 
			dActionEntry (275, 0, 0, 27, 0, 0), dActionEntry (264, 0, 1, 4, 1, 9), dActionEntry (265, 0, 1, 4, 1, 9), dActionEntry (266, 0, 1, 4, 1, 9), 
			dActionEntry (267, 0, 1, 4, 1, 9), dActionEntry (268, 0, 1, 4, 1, 9), dActionEntry (271, 0, 1, 4, 1, 9), dActionEntry (272, 0, 1, 4, 1, 9), 
			dActionEntry (274, 0, 1, 4, 1, 9), dActionEntry (274, 0, 0, 29, 0, 0), dActionEntry (264, 0, 1, 4, 1, 5), dActionEntry (265, 0, 1, 4, 1, 5), 
			dActionEntry (266, 0, 1, 4, 1, 5), dActionEntry (267, 0, 1, 4, 1, 5), dActionEntry (268, 0, 1, 4, 1, 5), dActionEntry (271, 0, 1, 4, 1, 5), 
			dActionEntry (272, 0, 1, 4, 1, 5), dActionEntry (274, 0, 1, 4, 1, 5), dActionEntry (274, 0, 0, 30, 0, 0), dActionEntry (264, 0, 1, 2, 1, 3), 
			dActionEntry (265, 0, 1, 2, 1, 3), dActionEntry (266, 0, 1, 2, 1, 3), dActionEntry (267, 0, 1, 2, 1, 3), dActionEntry (268, 0, 1, 2, 1, 3), 
			dActionEntry (271, 0, 1, 2, 1, 3), dActionEntry (272, 0, 1, 2, 1, 3), dActionEntry (274, 0, 1, 2, 1, 3), dActionEntry (264, 0, 1, 4, 1, 6), 
			dActionEntry (265, 0, 1, 4, 1, 6), dActionEntry (266, 0, 1, 4, 1, 6), dActionEntry (267, 0, 1, 4, 1, 6), dActionEntry (268, 0, 1, 4, 1, 6), 
			dActionEntry (271, 0, 1, 4, 1, 6), dActionEntry (272, 0, 1, 4, 1, 6), dActionEntry (274, 0, 1, 4, 1, 6), dActionEntry (256, 0, 1, 12, 1, 31), 
			dActionEntry (257, 0, 1, 12, 1, 31), dActionEntry (260, 0, 1, 12, 1, 31), dActionEntry (261, 0, 1, 12, 1, 31), dActionEntry (262, 0, 1, 12, 1, 31), 
			dActionEntry (274, 0, 1, 12, 1, 31), dActionEntry (274, 0, 1, 13, 1, 28), dActionEntry (264, 0, 1, 4, 1, 12), dActionEntry (265, 0, 1, 4, 1, 12), 
			dActionEntry (266, 0, 1, 4, 1, 12), dActionEntry (267, 0, 1, 4, 1, 12), dActionEntry (268, 0, 1, 4, 1, 12), dActionEntry (271, 0, 1, 4, 1, 12), 
			dActionEntry (272, 0, 1, 4, 1, 12), dActionEntry (274, 0, 1, 4, 1, 12), dActionEntry (123, 0, 0, 32, 0, 0), dActionEntry (274, 0, 0, 33, 0, 0), 
			dActionEntry (256, 0, 0, 37, 0, 0), dActionEntry (257, 0, 0, 39, 0, 0), dActionEntry (260, 0, 0, 42, 0, 0), dActionEntry (261, 0, 0, 34, 0, 0), 
			dActionEntry (262, 0, 0, 43, 0, 0), dActionEntry (264, 0, 0, 35, 0, 0), dActionEntry (274, 0, 0, 38, 0, 0), dActionEntry (264, 0, 0, 6, 0, 0), 
			dActionEntry (265, 0, 0, 14, 0, 0), dActionEntry (266, 0, 0, 47, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), dActionEntry (268, 0, 0, 20, 0, 0), 
			dActionEntry (271, 0, 0, 8, 0, 0), dActionEntry (272, 0, 0, 4, 0, 0), dActionEntry (274, 0, 0, 11, 0, 0), dActionEntry (256, 0, 0, 51, 0, 0), 
			dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), dActionEntry (262, 0, 0, 57, 0, 0), 
			dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (264, 0, 1, 4, 1, 10), dActionEntry (265, 0, 1, 4, 1, 10), dActionEntry (266, 0, 1, 4, 1, 10), 
			dActionEntry (267, 0, 1, 4, 1, 10), dActionEntry (268, 0, 1, 4, 1, 10), dActionEntry (271, 0, 1, 4, 1, 10), dActionEntry (272, 0, 1, 4, 1, 10), 
			dActionEntry (274, 0, 1, 4, 1, 10), dActionEntry (274, 0, 1, 13, 1, 29), dActionEntry (270, 0, 0, 59, 0, 0), dActionEntry (40, 0, 0, 60, 0, 0), 
			dActionEntry (264, 0, 1, 4, 1, 8), dActionEntry (265, 0, 1, 4, 1, 8), dActionEntry (266, 0, 1, 4, 1, 8), dActionEntry (267, 0, 1, 4, 1, 8), 
			dActionEntry (268, 0, 1, 4, 1, 8), dActionEntry (271, 0, 1, 4, 1, 8), dActionEntry (272, 0, 1, 4, 1, 8), dActionEntry (274, 0, 1, 4, 1, 8), 
			dActionEntry (264, 0, 1, 4, 1, 11), dActionEntry (265, 0, 1, 4, 1, 11), dActionEntry (266, 0, 1, 4, 1, 11), dActionEntry (267, 0, 1, 4, 1, 11), 
			dActionEntry (268, 0, 1, 4, 1, 11), dActionEntry (271, 0, 1, 4, 1, 11), dActionEntry (272, 0, 1, 4, 1, 11), dActionEntry (274, 0, 1, 4, 1, 11), 
			dActionEntry (264, 0, 1, 4, 1, 7), dActionEntry (265, 0, 1, 4, 1, 7), dActionEntry (266, 0, 1, 4, 1, 7), dActionEntry (267, 0, 1, 4, 1, 7), 
			dActionEntry (268, 0, 1, 4, 1, 7), dActionEntry (271, 0, 1, 4, 1, 7), dActionEntry (272, 0, 1, 4, 1, 7), dActionEntry (274, 0, 1, 4, 1, 7), 
			dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), 
			dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (264, 0, 0, 6, 0, 0), dActionEntry (265, 0, 0, 14, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), 
			dActionEntry (264, 0, 1, 11, 2, 16), dActionEntry (265, 0, 1, 11, 2, 16), dActionEntry (266, 0, 1, 11, 2, 16), dActionEntry (267, 0, 1, 11, 2, 16), 
			dActionEntry (268, 0, 1, 11, 2, 16), dActionEntry (271, 0, 1, 11, 2, 16), dActionEntry (272, 0, 1, 11, 2, 16), dActionEntry (274, 0, 1, 11, 2, 16), 
			dActionEntry (274, 0, 0, 66, 0, 0), dActionEntry (123, 0, 0, 67, 0, 0), dActionEntry (264, 0, 1, 5, 2, 13), dActionEntry (265, 0, 1, 5, 2, 13), 
			dActionEntry (266, 0, 1, 5, 2, 13), dActionEntry (267, 0, 1, 5, 2, 13), dActionEntry (268, 0, 1, 5, 2, 13), dActionEntry (271, 0, 1, 5, 2, 13), 
			dActionEntry (272, 0, 1, 5, 2, 13), dActionEntry (273, 0, 1, 12, 1, 31), dActionEntry (274, 0, 1, 5, 2, 13), dActionEntry (275, 0, 0, 68, 0, 0), 
			dActionEntry (273, 0, 0, 69, 0, 0), dActionEntry (256, 0, 1, 15, 2, 34), dActionEntry (257, 0, 1, 15, 2, 34), dActionEntry (260, 0, 1, 15, 2, 34), 
			dActionEntry (261, 0, 1, 15, 2, 34), dActionEntry (262, 0, 1, 15, 2, 34), dActionEntry (264, 0, 1, 15, 2, 34), dActionEntry (265, 0, 1, 15, 2, 34), 
			dActionEntry (274, 0, 1, 15, 2, 34), dActionEntry (125, 0, 0, 70, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), dActionEntry (268, 0, 0, 20, 0, 0), 
			dActionEntry (271, 0, 0, 71, 0, 0), dActionEntry (274, 0, 0, 72, 0, 0), dActionEntry (40, 0, 1, 20, 1, 50), dActionEntry (42, 0, 1, 20, 1, 50), 
			dActionEntry (260, 0, 0, 76, 0, 0), dActionEntry (274, 0, 0, 77, 0, 0), dActionEntry (40, 0, 1, 19, 1, 41), dActionEntry (42, 0, 0, 79, 0, 0), 
			dActionEntry (40, 0, 1, 20, 1, 46), dActionEntry (42, 0, 1, 20, 1, 46), dActionEntry (40, 0, 1, 12, 1, 31), dActionEntry (42, 0, 1, 12, 1, 31), 
			dActionEntry (40, 0, 1, 20, 1, 47), dActionEntry (42, 0, 1, 20, 1, 47), dActionEntry (40, 0, 1, 20, 1, 56), dActionEntry (42, 0, 1, 20, 1, 56), 
			dActionEntry (40, 0, 1, 20, 1, 48), dActionEntry (42, 0, 1, 20, 1, 48), dActionEntry (256, 0, 0, 80, 0, 0), dActionEntry (262, 0, 0, 83, 0, 0), 
			dActionEntry (274, 0, 0, 38, 0, 0), dActionEntry (256, 0, 0, 84, 0, 0), dActionEntry (40, 0, 1, 20, 1, 49), dActionEntry (42, 0, 1, 20, 1, 49), 
			dActionEntry (40, 0, 0, 85, 0, 0), dActionEntry (264, 0, 1, 2, 2, 4), dActionEntry (265, 0, 1, 2, 2, 4), dActionEntry (266, 0, 1, 2, 2, 4), 
			dActionEntry (267, 0, 1, 2, 2, 4), dActionEntry (268, 0, 1, 2, 2, 4), dActionEntry (271, 0, 1, 2, 2, 4), dActionEntry (272, 0, 1, 2, 2, 4), 
			dActionEntry (274, 0, 1, 2, 2, 4), dActionEntry (274, 0, 0, 86, 0, 0), dActionEntry (264, 0, 1, 4, 1, 11), dActionEntry (265, 0, 1, 4, 1, 11), 
			dActionEntry (266, 0, 1, 4, 1, 11), dActionEntry (267, 0, 1, 4, 1, 11), dActionEntry (268, 0, 1, 4, 1, 11), dActionEntry (270, 0, 1, 1, 2, 2), 
			dActionEntry (271, 0, 1, 4, 1, 11), dActionEntry (272, 0, 1, 4, 1, 11), dActionEntry (274, 0, 1, 4, 1, 11), dActionEntry (42, 0, 1, 20, 1, 50), 
			dActionEntry (260, 0, 0, 87, 0, 0), dActionEntry (274, 0, 1, 20, 1, 50), dActionEntry (42, 0, 0, 88, 0, 0), dActionEntry (274, 0, 1, 19, 1, 41), 
			dActionEntry (42, 0, 1, 20, 1, 46), dActionEntry (274, 0, 1, 20, 1, 46), dActionEntry (42, 0, 1, 12, 1, 31), dActionEntry (274, 0, 1, 12, 1, 31), 
			dActionEntry (42, 0, 1, 20, 1, 47), dActionEntry (274, 0, 1, 20, 1, 47), dActionEntry (42, 0, 1, 20, 1, 56), dActionEntry (274, 0, 1, 20, 1, 56), 
			dActionEntry (42, 0, 1, 20, 1, 48), dActionEntry (256, 0, 0, 89, 0, 0), dActionEntry (262, 0, 0, 92, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), 
			dActionEntry (256, 0, 0, 93, 0, 0), dActionEntry (42, 0, 1, 20, 1, 49), dActionEntry (274, 0, 1, 20, 1, 49), dActionEntry (274, 0, 0, 95, 0, 0), 
			dActionEntry (254, 0, 1, 0, 4, 1), dActionEntry (41, 0, 0, 99, 0, 0), dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), 
			dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), 
			dActionEntry (125, 0, 0, 101, 0, 0), dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), 
			dActionEntry (261, 0, 0, 49, 0, 0), dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (264, 0, 0, 6, 0, 0), dActionEntry (265, 0, 0, 14, 0, 0), 
			dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (125, 0, 1, 16, 1, 35), dActionEntry (256, 0, 1, 16, 1, 35), dActionEntry (257, 0, 1, 16, 1, 35), 
			dActionEntry (260, 0, 1, 16, 1, 35), dActionEntry (261, 0, 1, 16, 1, 35), dActionEntry (262, 0, 1, 16, 1, 35), dActionEntry (264, 0, 1, 16, 1, 35), 
			dActionEntry (265, 0, 1, 16, 1, 35), dActionEntry (274, 0, 1, 16, 1, 35), dActionEntry (125, 0, 1, 18, 1, 40), dActionEntry (256, 0, 1, 18, 1, 40), 
			dActionEntry (257, 0, 1, 18, 1, 40), dActionEntry (260, 0, 1, 18, 1, 40), dActionEntry (261, 0, 1, 18, 1, 40), dActionEntry (262, 0, 1, 18, 1, 40), 
			dActionEntry (264, 0, 1, 18, 1, 40), dActionEntry (265, 0, 1, 18, 1, 40), dActionEntry (274, 0, 1, 18, 1, 40), dActionEntry (274, 0, 0, 104, 0, 0), 
			dActionEntry (62, 0, 0, 107, 0, 0), dActionEntry (256, 0, 1, 15, 3, 33), dActionEntry (257, 0, 1, 15, 3, 33), dActionEntry (260, 0, 1, 15, 3, 33), 
			dActionEntry (261, 0, 1, 15, 3, 33), dActionEntry (262, 0, 1, 15, 3, 33), dActionEntry (264, 0, 1, 15, 3, 33), dActionEntry (265, 0, 1, 15, 3, 33), 
			dActionEntry (274, 0, 1, 15, 3, 33), dActionEntry (264, 0, 1, 6, 3, 15), dActionEntry (265, 0, 1, 6, 3, 15), dActionEntry (266, 0, 1, 6, 3, 15), 
			dActionEntry (267, 0, 1, 6, 3, 15), dActionEntry (268, 0, 1, 6, 3, 15), dActionEntry (271, 0, 1, 6, 3, 15), dActionEntry (272, 0, 1, 6, 3, 15), 
			dActionEntry (274, 0, 1, 6, 3, 15), dActionEntry (264, 0, 1, 6, 3, 14), dActionEntry (265, 0, 1, 6, 3, 14), dActionEntry (266, 0, 1, 6, 3, 14), 
			dActionEntry (267, 0, 1, 6, 3, 14), dActionEntry (268, 0, 1, 6, 3, 14), dActionEntry (271, 0, 1, 6, 3, 14), dActionEntry (272, 0, 1, 6, 3, 14), 
			dActionEntry (274, 0, 1, 6, 3, 14), dActionEntry (270, 0, 0, 108, 0, 0), dActionEntry (262, 0, 0, 110, 0, 0), dActionEntry (274, 0, 0, 109, 0, 0), 
			dActionEntry (275, 0, 0, 111, 0, 0), dActionEntry (269, 0, 0, 112, 0, 0), dActionEntry (270, 0, 0, 113, 0, 0), dActionEntry (274, 0, 0, 114, 0, 0), 
			dActionEntry (270, 0, 0, 115, 0, 0), dActionEntry (40, 0, 1, 20, 2, 52), dActionEntry (42, 0, 1, 20, 2, 52), dActionEntry (123, 0, 1, 12, 1, 31), 
			dActionEntry (123, 0, 0, 116, 0, 0), dActionEntry (40, 0, 1, 19, 2, 42), dActionEntry (42, 0, 0, 117, 0, 0), dActionEntry (40, 0, 1, 20, 2, 54), 
			dActionEntry (42, 0, 1, 20, 2, 54), dActionEntry (40, 0, 1, 20, 2, 57), dActionEntry (42, 0, 1, 20, 2, 57), dActionEntry (40, 0, 1, 20, 2, 53), 
			dActionEntry (42, 0, 1, 20, 2, 53), dActionEntry (40, 0, 1, 20, 2, 55), dActionEntry (42, 0, 1, 20, 2, 55), dActionEntry (40, 0, 1, 20, 2, 51), 
			dActionEntry (42, 0, 1, 20, 2, 51), dActionEntry (42, 0, 0, 119, 0, 0), dActionEntry (125, 0, 0, 120, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), 
			dActionEntry (268, 0, 0, 20, 0, 0), dActionEntry (271, 0, 0, 71, 0, 0), dActionEntry (274, 0, 0, 121, 0, 0), dActionEntry (42, 0, 1, 20, 2, 52), 
			dActionEntry (274, 0, 1, 20, 2, 52), dActionEntry (42, 0, 0, 124, 0, 0), dActionEntry (274, 0, 1, 19, 2, 42), dActionEntry (42, 0, 1, 20, 2, 54), 
			dActionEntry (274, 0, 1, 20, 2, 54), dActionEntry (42, 0, 1, 20, 2, 57), dActionEntry (274, 0, 1, 20, 2, 57), dActionEntry (42, 0, 1, 20, 2, 53), 
			dActionEntry (274, 0, 1, 20, 2, 53), dActionEntry (42, 0, 1, 20, 2, 55), dActionEntry (274, 0, 1, 20, 2, 55), dActionEntry (42, 0, 1, 20, 2, 51), 
			dActionEntry (274, 0, 1, 20, 2, 51), dActionEntry (40, 0, 0, 126, 0, 0), dActionEntry (40, 0, 1, 12, 1, 31), dActionEntry (40, 0, 1, 25, 1, 66), 
			dActionEntry (41, 0, 1, 23, 1, 62), dActionEntry (256, 0, 1, 23, 1, 62), dActionEntry (257, 0, 1, 23, 1, 62), dActionEntry (260, 0, 1, 23, 1, 62), 
			dActionEntry (261, 0, 1, 23, 1, 62), dActionEntry (262, 0, 1, 23, 1, 62), dActionEntry (274, 0, 1, 23, 1, 62), dActionEntry (41, 0, 0, 128, 0, 0), 
			dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), 
			dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (59, 0, 0, 129, 0, 0), dActionEntry (274, 0, 0, 130, 0, 0), 
			dActionEntry (59, 0, 0, 132, 0, 0), dActionEntry (264, 0, 1, 8, 3, 32), dActionEntry (265, 0, 1, 8, 3, 32), dActionEntry (266, 0, 1, 8, 3, 32), 
			dActionEntry (267, 0, 1, 8, 3, 32), dActionEntry (268, 0, 1, 8, 3, 32), dActionEntry (271, 0, 1, 8, 3, 32), dActionEntry (272, 0, 1, 8, 3, 32), 
			dActionEntry (274, 0, 1, 8, 3, 32), dActionEntry (125, 0, 1, 16, 2, 36), dActionEntry (256, 0, 1, 16, 2, 36), dActionEntry (257, 0, 1, 16, 2, 36), 
			dActionEntry (260, 0, 1, 16, 2, 36), dActionEntry (261, 0, 1, 16, 2, 36), dActionEntry (262, 0, 1, 16, 2, 36), dActionEntry (264, 0, 1, 16, 2, 36), 
			dActionEntry (265, 0, 1, 16, 2, 36), dActionEntry (274, 0, 1, 16, 2, 36), dActionEntry (59, 0, 1, 12, 1, 31), dActionEntry (91, 0, 1, 12, 1, 31), 
			dActionEntry (59, 0, 0, 133, 0, 0), dActionEntry (91, 0, 0, 134, 0, 0), dActionEntry (125, 0, 0, 135, 0, 0), dActionEntry (256, 0, 0, 51, 0, 0), 
			dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), dActionEntry (262, 0, 0, 57, 0, 0), 
			dActionEntry (264, 0, 0, 6, 0, 0), dActionEntry (265, 0, 0, 14, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (264, 0, 1, 11, 4, 17), 
			dActionEntry (265, 0, 1, 11, 4, 17), dActionEntry (266, 0, 1, 11, 4, 17), dActionEntry (267, 0, 1, 11, 4, 17), dActionEntry (268, 0, 1, 11, 4, 17), 
			dActionEntry (271, 0, 1, 11, 4, 17), dActionEntry (272, 0, 1, 11, 4, 17), dActionEntry (274, 0, 1, 11, 4, 17), dActionEntry (264, 0, 1, 3, 4, 23), 
			dActionEntry (265, 0, 1, 3, 4, 23), dActionEntry (266, 0, 1, 3, 4, 23), dActionEntry (267, 0, 1, 3, 4, 23), dActionEntry (268, 0, 1, 3, 4, 23), 
			dActionEntry (271, 0, 1, 3, 4, 23), dActionEntry (272, 0, 1, 3, 4, 23), dActionEntry (274, 0, 1, 3, 4, 23), dActionEntry (269, 0, 1, 14, 2, 24), 
			dActionEntry (270, 0, 1, 14, 2, 24), dActionEntry (274, 0, 0, 137, 0, 0), dActionEntry (262, 0, 0, 139, 0, 0), dActionEntry (263, 0, 0, 138, 0, 0), 
			dActionEntry (123, 0, 0, 140, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), dActionEntry (268, 0, 0, 20, 0, 0), dActionEntry (271, 0, 0, 141, 0, 0), 
			dActionEntry (264, 0, 1, 3, 4, 18), dActionEntry (265, 0, 1, 3, 4, 18), dActionEntry (266, 0, 1, 3, 4, 18), dActionEntry (267, 0, 1, 3, 4, 18), 
			dActionEntry (268, 0, 1, 3, 4, 18), dActionEntry (271, 0, 1, 3, 4, 18), dActionEntry (272, 0, 1, 3, 4, 18), dActionEntry (274, 0, 1, 3, 4, 18), 
			dActionEntry (125, 0, 0, 144, 0, 0), dActionEntry (267, 0, 0, 12, 0, 0), dActionEntry (268, 0, 0, 20, 0, 0), dActionEntry (271, 0, 0, 71, 0, 0), 
			dActionEntry (274, 0, 0, 145, 0, 0), dActionEntry (264, 0, 1, 3, 4, 19), dActionEntry (265, 0, 1, 3, 4, 19), dActionEntry (266, 0, 1, 3, 4, 19), 
			dActionEntry (267, 0, 1, 3, 4, 19), dActionEntry (268, 0, 1, 3, 4, 19), dActionEntry (271, 0, 1, 3, 4, 19), dActionEntry (272, 0, 1, 3, 4, 19), 
			dActionEntry (274, 0, 1, 3, 4, 19), dActionEntry (125, 0, 0, 148, 0, 0), dActionEntry (40, 0, 1, 19, 3, 44), dActionEntry (40, 0, 1, 19, 3, 43), 
			dActionEntry (274, 0, 0, 150, 0, 0), dActionEntry (270, 0, 0, 152, 0, 0), dActionEntry (275, 0, 0, 153, 0, 0), dActionEntry (269, 0, 0, 154, 0, 0), 
			dActionEntry (270, 0, 0, 155, 0, 0), dActionEntry (270, 0, 0, 156, 0, 0), dActionEntry (274, 0, 1, 19, 3, 44), dActionEntry (274, 0, 1, 19, 3, 43), 
			dActionEntry (41, 0, 0, 160, 0, 0), dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), 
			dActionEntry (261, 0, 0, 49, 0, 0), dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (41, 0, 1, 23, 2, 63), 
			dActionEntry (256, 0, 1, 23, 2, 63), dActionEntry (257, 0, 1, 23, 2, 63), dActionEntry (260, 0, 1, 23, 2, 63), dActionEntry (261, 0, 1, 23, 2, 63), 
			dActionEntry (262, 0, 1, 23, 2, 63), dActionEntry (274, 0, 1, 23, 2, 63), dActionEntry (59, 0, 0, 162, 0, 0), dActionEntry (264, 0, 1, 9, 4, 60), 
			dActionEntry (265, 0, 1, 9, 4, 60), dActionEntry (266, 0, 1, 9, 4, 60), dActionEntry (267, 0, 1, 9, 4, 60), dActionEntry (268, 0, 1, 9, 4, 60), 
			dActionEntry (271, 0, 1, 9, 4, 60), dActionEntry (272, 0, 1, 9, 4, 60), dActionEntry (274, 0, 1, 9, 4, 60), dActionEntry (41, 0, 1, 12, 1, 31), 
			dActionEntry (44, 0, 1, 12, 1, 31), dActionEntry (256, 0, 1, 12, 1, 31), dActionEntry (257, 0, 1, 12, 1, 31), dActionEntry (260, 0, 1, 12, 1, 31), 
			dActionEntry (261, 0, 1, 12, 1, 31), dActionEntry (262, 0, 1, 12, 1, 31), dActionEntry (274, 0, 1, 12, 1, 31), dActionEntry (41, 0, 1, 24, 2, 64), 
			dActionEntry (44, 0, 0, 163, 0, 0), dActionEntry (256, 0, 1, 24, 2, 64), dActionEntry (257, 0, 1, 24, 2, 64), dActionEntry (260, 0, 1, 24, 2, 64), 
			dActionEntry (261, 0, 1, 24, 2, 64), dActionEntry (262, 0, 1, 24, 2, 64), dActionEntry (274, 0, 1, 24, 2, 64), dActionEntry (264, 0, 1, 17, 2, 58), 
			dActionEntry (265, 0, 1, 17, 2, 58), dActionEntry (266, 0, 1, 17, 2, 58), dActionEntry (267, 0, 1, 17, 2, 58), dActionEntry (268, 0, 1, 17, 2, 58), 
			dActionEntry (271, 0, 1, 17, 2, 58), dActionEntry (272, 0, 1, 17, 2, 58), dActionEntry (274, 0, 1, 17, 2, 58), dActionEntry (125, 0, 1, 18, 3, 37), 
			dActionEntry (256, 0, 1, 18, 3, 37), dActionEntry (257, 0, 1, 18, 3, 37), dActionEntry (260, 0, 1, 18, 3, 37), dActionEntry (261, 0, 1, 18, 3, 37), 
			dActionEntry (262, 0, 1, 18, 3, 37), dActionEntry (264, 0, 1, 18, 3, 37), dActionEntry (265, 0, 1, 18, 3, 37), dActionEntry (274, 0, 1, 18, 3, 37), 
			dActionEntry (273, 0, 0, 164, 0, 0), dActionEntry (59, 0, 0, 165, 0, 0), dActionEntry (125, 0, 1, 8, 3, 32), dActionEntry (256, 0, 1, 8, 3, 32), 
			dActionEntry (257, 0, 1, 8, 3, 32), dActionEntry (260, 0, 1, 8, 3, 32), dActionEntry (261, 0, 1, 8, 3, 32), dActionEntry (262, 0, 1, 8, 3, 32), 
			dActionEntry (264, 0, 1, 8, 3, 32), dActionEntry (265, 0, 1, 8, 3, 32), dActionEntry (274, 0, 1, 8, 3, 32), dActionEntry (40, 0, 0, 166, 0, 0), 
			dActionEntry (269, 0, 1, 14, 3, 26), dActionEntry (270, 0, 1, 14, 3, 26), dActionEntry (269, 0, 1, 14, 3, 25), dActionEntry (270, 0, 1, 14, 3, 25), 
			dActionEntry (270, 0, 0, 167, 0, 0), dActionEntry (262, 0, 0, 169, 0, 0), dActionEntry (274, 0, 0, 168, 0, 0), dActionEntry (270, 0, 0, 170, 0, 0), 
			dActionEntry (270, 0, 0, 171, 0, 0), dActionEntry (270, 0, 0, 172, 0, 0), dActionEntry (275, 0, 0, 173, 0, 0), dActionEntry (269, 0, 0, 174, 0, 0), 
			dActionEntry (270, 0, 0, 175, 0, 0), dActionEntry (270, 0, 0, 176, 0, 0), dActionEntry (274, 0, 0, 177, 0, 0), dActionEntry (40, 0, 1, 19, 4, 45), 
			dActionEntry (41, 0, 1, 12, 1, 31), dActionEntry (41, 0, 0, 179, 0, 0), dActionEntry (264, 0, 1, 3, 4, 23), dActionEntry (265, 0, 1, 3, 4, 23), 
			dActionEntry (266, 0, 1, 3, 4, 23), dActionEntry (267, 0, 1, 3, 4, 23), dActionEntry (268, 0, 1, 3, 4, 23), dActionEntry (270, 0, 1, 3, 4, 23), 
			dActionEntry (271, 0, 1, 3, 4, 23), dActionEntry (272, 0, 1, 3, 4, 23), dActionEntry (274, 0, 1, 3, 4, 23), dActionEntry (123, 0, 0, 180, 0, 0), 
			dActionEntry (264, 0, 1, 3, 4, 18), dActionEntry (265, 0, 1, 3, 4, 18), dActionEntry (266, 0, 1, 3, 4, 18), dActionEntry (267, 0, 1, 3, 4, 18), 
			dActionEntry (268, 0, 1, 3, 4, 18), dActionEntry (270, 0, 1, 3, 4, 18), dActionEntry (271, 0, 1, 3, 4, 18), dActionEntry (272, 0, 1, 3, 4, 18), 
			dActionEntry (274, 0, 1, 3, 4, 18), dActionEntry (264, 0, 1, 3, 4, 19), dActionEntry (265, 0, 1, 3, 4, 19), dActionEntry (266, 0, 1, 3, 4, 19), 
			dActionEntry (267, 0, 1, 3, 4, 19), dActionEntry (268, 0, 1, 3, 4, 19), dActionEntry (270, 0, 1, 3, 4, 19), dActionEntry (271, 0, 1, 3, 4, 19), 
			dActionEntry (272, 0, 1, 3, 4, 19), dActionEntry (274, 0, 1, 3, 4, 19), dActionEntry (274, 0, 1, 19, 4, 45), dActionEntry (41, 0, 0, 184, 0, 0), 
			dActionEntry (256, 0, 0, 51, 0, 0), dActionEntry (257, 0, 0, 53, 0, 0), dActionEntry (260, 0, 0, 56, 0, 0), dActionEntry (261, 0, 0, 49, 0, 0), 
			dActionEntry (262, 0, 0, 57, 0, 0), dActionEntry (274, 0, 0, 52, 0, 0), dActionEntry (41, 0, 1, 26, 1, 69), dActionEntry (256, 0, 1, 26, 1, 69), 
			dActionEntry (257, 0, 1, 26, 1, 69), dActionEntry (260, 0, 1, 26, 1, 69), dActionEntry (261, 0, 1, 26, 1, 69), dActionEntry (262, 0, 1, 26, 1, 69), 
			dActionEntry (274, 0, 1, 26, 1, 69), dActionEntry (59, 0, 0, 185, 0, 0), dActionEntry (274, 0, 0, 186, 0, 0), dActionEntry (264, 0, 1, 9, 5, 59), 
			dActionEntry (265, 0, 1, 9, 5, 59), dActionEntry (266, 0, 1, 9, 5, 59), dActionEntry (267, 0, 1, 9, 5, 59), dActionEntry (268, 0, 1, 9, 5, 59), 
			dActionEntry (271, 0, 1, 9, 5, 59), dActionEntry (272, 0, 1, 9, 5, 59), dActionEntry (274, 0, 1, 9, 5, 59), dActionEntry (41, 0, 1, 24, 3, 65), 
			dActionEntry (256, 0, 1, 24, 3, 65), dActionEntry (257, 0, 1, 24, 3, 65), dActionEntry (260, 0, 1, 24, 3, 65), dActionEntry (261, 0, 1, 24, 3, 65), 
			dActionEntry (262, 0, 1, 24, 3, 65), dActionEntry (274, 0, 1, 24, 3, 65), dActionEntry (93, 0, 0, 188, 0, 0), dActionEntry (125, 0, 1, 17, 2, 58), 
			dActionEntry (256, 0, 1, 17, 2, 58), dActionEntry (257, 0, 1, 17, 2, 58), dActionEntry (260, 0, 1, 17, 2, 58), dActionEntry (261, 0, 1, 17, 2, 58), 
			dActionEntry (262, 0, 1, 17, 2, 58), dActionEntry (264, 0, 1, 17, 2, 58), dActionEntry (265, 0, 1, 17, 2, 58), dActionEntry (274, 0, 1, 17, 2, 58), 
			dActionEntry (274, 0, 0, 189, 0, 0), dActionEntry (264, 0, 1, 3, 6, 22), dActionEntry (265, 0, 1, 3, 6, 22), dActionEntry (266, 0, 1, 3, 6, 22), 
			dActionEntry (267, 0, 1, 3, 6, 22), dActionEntry (268, 0, 1, 3, 6, 22), dActionEntry (271, 0, 1, 3, 6, 22), dActionEntry (272, 0, 1, 3, 6, 22), 
			dActionEntry (274, 0, 1, 3, 6, 22), dActionEntry (270, 0, 1, 14, 2, 24), dActionEntry (274, 0, 0, 190, 0, 0), dActionEntry (262, 0, 0, 192, 0, 0), 
			dActionEntry (263, 0, 0, 191, 0, 0), dActionEntry (264, 0, 1, 3, 6, 21), dActionEntry (265, 0, 1, 3, 6, 21), dActionEntry (266, 0, 1, 3, 6, 21), 
			dActionEntry (267, 0, 1, 3, 6, 21), dActionEntry (268, 0, 1, 3, 6, 21), dActionEntry (271, 0, 1, 3, 6, 21), dActionEntry (272, 0, 1, 3, 6, 21), 
			dActionEntry (274, 0, 1, 3, 6, 21), dActionEntry (264, 0, 1, 3, 6, 20), dActionEntry (265, 0, 1, 3, 6, 20), dActionEntry (266, 0, 1, 3, 6, 20), 
			dActionEntry (267, 0, 1, 3, 6, 20), dActionEntry (268, 0, 1, 3, 6, 20), dActionEntry (271, 0, 1, 3, 6, 20), dActionEntry (272, 0, 1, 3, 6, 20), 
			dActionEntry (274, 0, 1, 3, 6, 20), dActionEntry (270, 0, 1, 3, 4, 23), dActionEntry (123, 0, 0, 193, 0, 0), dActionEntry (270, 0, 1, 3, 4, 18), 
			dActionEntry (270, 0, 1, 3, 4, 19), dActionEntry (59, 0, 1, 12, 1, 31), dActionEntry (59, 0, 0, 196, 0, 0), dActionEntry (40, 0, 1, 22, 6, 61), 
			dActionEntry (270, 0, 0, 197, 0, 0), dActionEntry (270, 0, 0, 198, 0, 0), dActionEntry (270, 0, 0, 199, 0, 0), dActionEntry (41, 0, 1, 26, 2, 70), 
			dActionEntry (256, 0, 1, 26, 2, 70), dActionEntry (257, 0, 1, 26, 2, 70), dActionEntry (260, 0, 1, 26, 2, 70), dActionEntry (261, 0, 1, 26, 2, 70), 
			dActionEntry (262, 0, 1, 26, 2, 70), dActionEntry (274, 0, 1, 26, 2, 70), dActionEntry (59, 0, 0, 200, 0, 0), dActionEntry (264, 0, 1, 10, 6, 67), 
			dActionEntry (265, 0, 1, 10, 6, 67), dActionEntry (266, 0, 1, 10, 6, 67), dActionEntry (267, 0, 1, 10, 6, 67), dActionEntry (268, 0, 1, 10, 6, 67), 
			dActionEntry (271, 0, 1, 10, 6, 67), dActionEntry (272, 0, 1, 10, 6, 67), dActionEntry (274, 0, 1, 10, 6, 67), dActionEntry (41, 0, 1, 12, 1, 31), 
			dActionEntry (44, 0, 1, 12, 1, 31), dActionEntry (91, 0, 1, 12, 1, 31), dActionEntry (256, 0, 1, 12, 1, 31), dActionEntry (257, 0, 1, 12, 1, 31), 
			dActionEntry (260, 0, 1, 12, 1, 31), dActionEntry (261, 0, 1, 12, 1, 31), dActionEntry (262, 0, 1, 12, 1, 31), dActionEntry (274, 0, 1, 12, 1, 31), 
			dActionEntry (41, 0, 1, 27, 2, 71), dActionEntry (44, 0, 0, 163, 0, 0), dActionEntry (91, 0, 0, 201, 0, 0), dActionEntry (256, 0, 1, 27, 2, 71), 
			dActionEntry (257, 0, 1, 27, 2, 71), dActionEntry (260, 0, 1, 27, 2, 71), dActionEntry (261, 0, 1, 27, 2, 71), dActionEntry (262, 0, 1, 27, 2, 71), 
			dActionEntry (274, 0, 1, 27, 2, 71), dActionEntry (59, 0, 0, 202, 0, 0), dActionEntry (91, 0, 0, 203, 0, 0), dActionEntry (41, 0, 0, 204, 0, 0), 
			dActionEntry (40, 0, 0, 205, 0, 0), dActionEntry (270, 0, 1, 14, 3, 26), dActionEntry (270, 0, 1, 14, 3, 25), dActionEntry (270, 0, 0, 206, 0, 0), 
			dActionEntry (270, 0, 0, 207, 0, 0), dActionEntry (270, 0, 0, 208, 0, 0), dActionEntry (264, 0, 1, 7, 7, 30), dActionEntry (265, 0, 1, 7, 7, 30), 
			dActionEntry (266, 0, 1, 7, 7, 30), dActionEntry (267, 0, 1, 7, 7, 30), dActionEntry (268, 0, 1, 7, 7, 30), dActionEntry (271, 0, 1, 7, 7, 30), 
			dActionEntry (272, 0, 1, 7, 7, 30), dActionEntry (274, 0, 1, 7, 7, 30), dActionEntry (264, 0, 1, 3, 6, 22), dActionEntry (265, 0, 1, 3, 6, 22), 
			dActionEntry (266, 0, 1, 3, 6, 22), dActionEntry (267, 0, 1, 3, 6, 22), dActionEntry (268, 0, 1, 3, 6, 22), dActionEntry (270, 0, 1, 3, 6, 22), 
			dActionEntry (271, 0, 1, 3, 6, 22), dActionEntry (272, 0, 1, 3, 6, 22), dActionEntry (274, 0, 1, 3, 6, 22), dActionEntry (264, 0, 1, 3, 6, 21), 
			dActionEntry (265, 0, 1, 3, 6, 21), dActionEntry (266, 0, 1, 3, 6, 21), dActionEntry (267, 0, 1, 3, 6, 21), dActionEntry (268, 0, 1, 3, 6, 21), 
			dActionEntry (270, 0, 1, 3, 6, 21), dActionEntry (271, 0, 1, 3, 6, 21), dActionEntry (272, 0, 1, 3, 6, 21), dActionEntry (274, 0, 1, 3, 6, 21), 
			dActionEntry (264, 0, 1, 3, 6, 20), dActionEntry (265, 0, 1, 3, 6, 20), dActionEntry (266, 0, 1, 3, 6, 20), dActionEntry (267, 0, 1, 3, 6, 20), 
			dActionEntry (268, 0, 1, 3, 6, 20), dActionEntry (270, 0, 1, 3, 6, 20), dActionEntry (271, 0, 1, 3, 6, 20), dActionEntry (272, 0, 1, 3, 6, 20), 
			dActionEntry (274, 0, 1, 3, 6, 20), dActionEntry (264, 0, 1, 10, 7, 68), dActionEntry (265, 0, 1, 10, 7, 68), dActionEntry (266, 0, 1, 10, 7, 68), 
			dActionEntry (267, 0, 1, 10, 7, 68), dActionEntry (268, 0, 1, 10, 7, 68), dActionEntry (271, 0, 1, 10, 7, 68), dActionEntry (272, 0, 1, 10, 7, 68), 
			dActionEntry (274, 0, 1, 10, 7, 68), dActionEntry (93, 0, 0, 209, 0, 0), dActionEntry (125, 0, 1, 18, 6, 38), dActionEntry (256, 0, 1, 18, 6, 38), 
			dActionEntry (257, 0, 1, 18, 6, 38), dActionEntry (260, 0, 1, 18, 6, 38), dActionEntry (261, 0, 1, 18, 6, 38), dActionEntry (262, 0, 1, 18, 6, 38), 
			dActionEntry (264, 0, 1, 18, 6, 38), dActionEntry (265, 0, 1, 18, 6, 38), dActionEntry (274, 0, 1, 18, 6, 38), dActionEntry (273, 0, 0, 210, 0, 0), 
			dActionEntry (269, 0, 1, 14, 6, 27), dActionEntry (270, 0, 1, 14, 6, 27), dActionEntry (274, 0, 0, 211, 0, 0), dActionEntry (270, 0, 1, 3, 6, 22), 
			dActionEntry (270, 0, 1, 3, 6, 21), dActionEntry (270, 0, 1, 3, 6, 20), dActionEntry (41, 0, 1, 27, 4, 73), dActionEntry (44, 0, 0, 212, 0, 0), 
			dActionEntry (256, 0, 1, 27, 4, 73), dActionEntry (257, 0, 1, 27, 4, 73), dActionEntry (260, 0, 1, 27, 4, 73), dActionEntry (261, 0, 1, 27, 4, 73), 
			dActionEntry (262, 0, 1, 27, 4, 73), dActionEntry (274, 0, 1, 27, 4, 73), dActionEntry (93, 0, 0, 213, 0, 0), dActionEntry (41, 0, 0, 214, 0, 0), 
			dActionEntry (41, 0, 1, 27, 5, 74), dActionEntry (256, 0, 1, 27, 5, 74), dActionEntry (257, 0, 1, 27, 5, 74), dActionEntry (260, 0, 1, 27, 5, 74), 
			dActionEntry (261, 0, 1, 27, 5, 74), dActionEntry (262, 0, 1, 27, 5, 74), dActionEntry (274, 0, 1, 27, 5, 74), dActionEntry (59, 0, 0, 215, 0, 0), 
			dActionEntry (270, 0, 1, 14, 6, 27), dActionEntry (125, 0, 1, 18, 9, 39), dActionEntry (256, 0, 1, 18, 9, 39), dActionEntry (257, 0, 1, 18, 9, 39), 
			dActionEntry (260, 0, 1, 18, 9, 39), dActionEntry (261, 0, 1, 18, 9, 39), dActionEntry (262, 0, 1, 18, 9, 39), dActionEntry (264, 0, 1, 18, 9, 39), 
			dActionEntry (265, 0, 1, 18, 9, 39), dActionEntry (274, 0, 1, 18, 9, 39)};

	bool errorMode = false;
	const dStackPair& stackTop = stack.GetLast()->GetInfo();
	int state = stackTop.m_state;
	int start = actionsStart[state];
	int count = actionsCount[state];

	const dActionEntry* const table = &actionTable[start];
	const dActionEntry* action = FindAction (table, count, token);
	while (!action && (stack.GetCount() > 1)) {
		errorMode = true; 

		// we found a syntax error in go into error recovering mode, and find the token mark by a ". error" rule
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


bool test7::Parse(xxxx& scanner)
{
	static short gotoCount[] = {
			1, 0, 0, 15, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 4, 13, 4, 0, 0, 0, 0, 0, 
			0, 0, 8, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 4, 
			0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 6, 8, 0, 0, 1, 8, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 5, 0, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 3, 0, 3, 0, 0, 1, 0, 1, 
			0, 0, 0, 0, 1, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 5, 0, 0, 1, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static short gotoStart[] = {
			0, 1, 1, 1, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 21, 34, 38, 38, 38, 38, 38, 
			38, 38, 38, 46, 46, 46, 46, 46, 46, 46, 49, 49, 50, 50, 50, 50, 50, 50, 52, 52, 52, 52, 52, 52, 
			56, 56, 56, 56, 56, 56, 56, 56, 58, 58, 58, 60, 60, 66, 74, 74, 74, 75, 83, 83, 83, 83, 83, 83, 
			83, 83, 83, 83, 83, 83, 83, 83, 84, 84, 84, 84, 84, 84, 84, 87, 87, 88, 88, 88, 88, 88, 88, 88, 
			88, 88, 88, 93, 93, 94, 94, 94, 94, 94, 94, 102, 102, 102, 102, 102, 102, 105, 105, 108, 108, 108, 109, 109, 
			110, 110, 110, 110, 110, 111, 111, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 
			117, 117, 117, 117, 117, 118, 118, 118, 118, 118, 118, 121, 121, 121, 121, 126, 126, 126, 127, 127, 127, 127, 127, 127, 
			127, 127, 127, 127, 127, 127, 127, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 
			130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130};
	static dGotoEntry gotoTable[] = {
			dGotoEntry (276, 1), dGotoEntry (277, 21), dGotoEntry (278, 17), dGotoEntry (279, 24), dGotoEntry (280, 9), 
			dGotoEntry (281, 7), dGotoEntry (282, 10), dGotoEntry (283, 25), dGotoEntry (284, 23), dGotoEntry (285, 5), 
			dGotoEntry (286, 19), dGotoEntry (287, 13), dGotoEntry (288, 18), dGotoEntry (289, 15), dGotoEntry (291, 26), 
			dGotoEntry (298, 22), dGotoEntry (288, 31), dGotoEntry (288, 40), dGotoEntry (295, 44), dGotoEntry (296, 36), 
			dGotoEntry (297, 41), dGotoEntry (279, 48), dGotoEntry (280, 45), dGotoEntry (281, 7), dGotoEntry (282, 10), 
			dGotoEntry (283, 25), dGotoEntry (284, 23), dGotoEntry (285, 5), dGotoEntry (286, 19), dGotoEntry (287, 13), 
			dGotoEntry (288, 18), dGotoEntry (289, 46), dGotoEntry (291, 26), dGotoEntry (298, 22), dGotoEntry (288, 54), 
			dGotoEntry (295, 58), dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (284, 63), dGotoEntry (288, 54), 
			dGotoEntry (291, 65), dGotoEntry (292, 61), dGotoEntry (294, 62), dGotoEntry (295, 64), dGotoEntry (296, 50), 
			dGotoEntry (297, 55), dGotoEntry (279, 75), dGotoEntry (289, 74), dGotoEntry (290, 73), dGotoEntry (288, 78), 
			dGotoEntry (288, 81), dGotoEntry (297, 82), dGotoEntry (288, 40), dGotoEntry (295, 44), dGotoEntry (296, 36), 
			dGotoEntry (297, 41), dGotoEntry (288, 90), dGotoEntry (297, 91), dGotoEntry (288, 96), dGotoEntry (301, 94), 
			dGotoEntry (288, 54), dGotoEntry (295, 100), dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (299, 98), 
			dGotoEntry (300, 97), dGotoEntry (284, 63), dGotoEntry (288, 54), dGotoEntry (291, 65), dGotoEntry (293, 102), 
			dGotoEntry (294, 103), dGotoEntry (295, 64), dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (288, 105), 
			dGotoEntry (284, 63), dGotoEntry (288, 54), dGotoEntry (291, 65), dGotoEntry (292, 106), dGotoEntry (294, 62), 
			dGotoEntry (295, 64), dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (297, 118), dGotoEntry (279, 123), 
			dGotoEntry (289, 74), dGotoEntry (290, 122), dGotoEntry (297, 125), dGotoEntry (288, 54), dGotoEntry (295, 100), 
			dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (300, 127), dGotoEntry (288, 131), dGotoEntry (284, 63), 
			dGotoEntry (288, 54), dGotoEntry (291, 65), dGotoEntry (293, 136), dGotoEntry (294, 103), dGotoEntry (295, 64), 
			dGotoEntry (296, 50), dGotoEntry (297, 55), dGotoEntry (279, 143), dGotoEntry (289, 74), dGotoEntry (290, 142), 
			dGotoEntry (279, 147), dGotoEntry (289, 74), dGotoEntry (290, 146), dGotoEntry (297, 149), dGotoEntry (288, 151), 
			dGotoEntry (297, 157), dGotoEntry (288, 54), dGotoEntry (295, 161), dGotoEntry (296, 50), dGotoEntry (297, 55), 
			dGotoEntry (302, 158), dGotoEntry (303, 159), dGotoEntry (288, 178), dGotoEntry (279, 182), dGotoEntry (289, 74), 
			dGotoEntry (290, 181), dGotoEntry (288, 54), dGotoEntry (295, 161), dGotoEntry (296, 50), dGotoEntry (297, 55), 
			dGotoEntry (303, 183), dGotoEntry (288, 187), dGotoEntry (279, 195), dGotoEntry (289, 74), dGotoEntry (290, 194)};

	dList<dStackPair> stack;
	const int lastToken = 276;
	
	stack.Append ();
	m_grammarError = false;
	dToken token = dToken (scanner.NextToken());
	for (bool terminate = false; !terminate;) {

		const dActionEntry* const action = GetNextAction (stack, token, scanner);
		if (!action) {
			terminate = true;
			fprintf (stderr, "unrecoverable parser error\n");
			DTRACE (("unrecoverable parser error\n"));
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
					_ASSERTE (reduceCount < sizeof (parameter) / sizeof (parameter[0]));

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
						case 9:// Statement : FunctionCallBackDeclaration 
{
	PrintFunctionCallbacks (parameter[0].m_value.m_literal);		
}
break;

						case 5:// Statement : HeaderInfo 
{
//PrintHeader (parameter[0].m_value.m_literal);
}
break;

						case 6:// Statement : DefineConstant 
{
	PrintDefine (parameter[0].m_value.m_literal);
}
break;

						case 31:// literalIndentifer : _LITERAL_IDENTIFIER 
{entry.m_value = parameter[0].m_value;}
break;

						case 10:// Statement : ClassMethod 
{
	PrintClassMethod (parameter[0].m_value.m_literal);
}
break;

						case 8:// Statement : StructuresDeclaration 
{
	PrintDataStrutures (parameter[0].m_value.m_literal);
}
break;

						case 7:// Statement : TypeDefStructures 
{
	PrintStrutureDefine (parameter[0].m_value.m_literal);
}
break;

						case 34:// StructuresDeclarationInit : _UNION { 
{
	char tabs[256];
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%sunion {\n", tabs);
	tab ++;
}
break;

						case 50:// DataInstanceType : _UNSIGNED 
{
	sprintf (entry.m_value.m_literal, "unsigned");
}
break;

						case 41:// DataType : DataInstanceType 
{entry.m_value = parameter[0].m_value;}
break;

						case 46:// DataInstanceType : _INT 
{
	sprintf (entry.m_value.m_literal, "int");
}
break;

						case 47:// DataInstanceType : _CHAR 
{
	sprintf (entry.m_value.m_literal, "char");			
}
break;

						case 56:// DataInstanceType : literalIndentifer 
{
	sprintf (entry.m_value.m_literal, "struct i%s", parameter[0].m_value.m_literal);
}
break;

						case 48:// DataInstanceType : VOID 
{
	sprintf (entry.m_value.m_literal, "void");
}
break;

						case 49:// DataInstanceType : _FLOAT 
{
	sprintf (entry.m_value.m_literal, "float");
}
break;

						case 35:// DataInstanceList : DataInstance 
{entry.m_value = parameter[0].m_value;}
break;

						case 40:// DataInstance : StructuresDeclaration 
{
	sprintf (entry.m_value.m_literal, "%s", parameter[0].m_value.m_literal);
}
break;

						case 33:// StructuresDeclarationInit : _STRUCT _LITERAL_IDENTIFIER { 
{
	char tabs[256];
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%sstruct i%s {\n", tabs, parameter[1].m_value.m_literal);
	tab ++;
}
break;

						case 14:// DefineConstant : _DEFINE literalIndentifer _NUMERIC_CONSTANT 
{
	sprintf (entry.m_value.m_literal, "#define %s\t%s\n", parameter[1].m_value.m_literal, parameter[2].m_value.m_literal);
}
break;

						case 52:// DataInstanceType : _UNSIGNED _SHORT 
{
	sprintf (entry.m_value.m_literal, "unsigned short");
}
break;

						case 42:// DataType : DataInstanceType * 
{
	sprintf (entry.m_value.m_literal, "%s*", parameter[0].m_value.m_literal);
}
break;

						case 54:// DataInstanceType : VOID _INT 
{
	sprintf (entry.m_value.m_literal, "const int");
}
break;

						case 57:// DataInstanceType : VOID literalIndentifer 
{
	sprintf (entry.m_value.m_literal, "%s struct i%s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 53:// DataInstanceType : VOID VOID 
{
	sprintf (entry.m_value.m_literal, "const void");
}
break;

						case 55:// DataInstanceType : VOID _FLOAT 
{
	sprintf (entry.m_value.m_literal, "const float");
}
break;

						case 51:// DataInstanceType : _SHORT _INT 
{
	sprintf (entry.m_value.m_literal, "short int");
}
break;

						case 66:// ClassMethodName : literalIndentifer 
{
	if (!strncmp (parameter[0].m_value.m_literal, "Newton", 6)) {
		sprintf (entry.m_value.m_literal, "%s", parameter[0].m_value.m_literal + 6);
	} else {
		entry.m_value = parameter[0].m_value;
	}	
}
break;

						case 62:// FunctionCallBackDeclarationParamsList : FunctionCallBackDeclarationParam 
{ entry.m_value = parameter[0].m_value; }
break;

						case 32:// StructuresDeclaration : StructuresDeclarationInit DataInstanceList StructuresDeclarationEnd 
{
	sprintf (entry.m_value.m_literal, "%s%s%s\n", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal, parameter[2].m_value.m_literal);
}
break;

						case 36:// DataInstanceList : DataInstanceList DataInstance 
{
	sprintf (entry.m_value.m_literal, "%s%s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 44:// DataType : DataInstanceType * * 
{
	sprintf (entry.m_value.m_literal, "%s**", parameter[0].m_value.m_literal);
}
break;

						case 43:// DataType : DataInstanceType * VOID 
{
	//sprintf (entry.m_value.m_literal, "%s*", parameter[0].m_value.m_literal);
	sprintf (entry.m_value.m_literal, "%s* const", parameter[0].m_value.m_literal);
}
break;

						case 63:// FunctionCallBackDeclarationParamsList : FunctionCallBackDeclarationParamsList FunctionCallBackDeclarationParam 
{
	sprintf (entry.m_value.m_literal, "%s%s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 60:// FunctionCallBackDeclaration : FunctionCallBackDeclarationName ( ) ; 
{
	sprintf (entry.m_value.m_literal, "%s ();\n", parameter[0].m_value.m_literal);
}
break;

						case 64:// FunctionCallBackDeclarationParam : DataType literalIndentifer 
{
	sprintf (entry.m_value.m_literal, "%s %s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 58:// StructuresDeclarationEnd : } ; 
{
	char tabs[256];
	tab --;
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%s};\n", tabs);
}
break;

						case 37:// DataInstance : DataType literalIndentifer ; 
{
	char tabs[256];
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%s%s %s;\n", tabs, parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 45:// DataType : DataInstanceType * * VOID 
{
	sprintf (entry.m_value.m_literal, "%s** const", parameter[0].m_value.m_literal);
	//sprintf (entry.m_value.m_literal, "%s**", parameter[0].m_value.m_literal);
}
break;

						case 69:// functionArgumentList : functionArgument 
{ entry.m_value = parameter[0].m_value; }
break;

						case 59:// FunctionCallBackDeclaration : FunctionCallBackDeclarationName ( FunctionCallBackDeclarationParamsList ) ; 
{
	sprintf (entry.m_value.m_literal, "%s (%s);\n", parameter[0].m_value.m_literal, parameter[2].m_value.m_literal);
}
break;

						case 65:// FunctionCallBackDeclarationParam : DataType literalIndentifer , 
{
	sprintf (entry.m_value.m_literal, "%s %s, ", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 61:// FunctionCallBackDeclarationName : _TYPEDEF DataType ( * literalIndentifer ) 
{
	SaveCallback (parameter[4].m_value.m_literal);
	sprintf (entry.m_value.m_literal, "typedef %s (*i%s)", parameter[1].m_value.m_literal, parameter[4].m_value.m_literal);
}
break;

						case 70:// functionArgumentList : functionArgumentList functionArgument 
{
	sprintf (entry.m_value.m_literal, "%s%s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
}
break;

						case 67:// ClassMethod : literalIndentifer DataType ClassMethodName ( ) ; 
{
	const char* callback = IsCallback (parameter[1].m_value.m_literal);
	if (callback) {
		sprintf (entry.m_value.m_literal, "-(i%s) %s;\n", callback, parameter[2].m_value.m_literal);
	} else {
		sprintf (entry.m_value.m_literal, "-(%s) %s;\n", parameter[1].m_value.m_literal, parameter[2].m_value.m_literal);
	}
}
break;

						case 71:// functionArgument : DataType literalIndentifer 
{
	const char* callback = IsCallback (parameter[0].m_value.m_literal);
	if (callback) {
		sprintf (entry.m_value.m_literal, ": (i%s) %s", callback, parameter[1].m_value.m_literal);
	} else {
		sprintf (entry.m_value.m_literal, ": (%s) %s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
	}
}
break;

						case 30:// TypeDefStructures : _TYPEDEF _STRUCT literalIndentifer { } literalIndentifer ; 
{
	sprintf (entry.m_value.m_literal, "typedef struct i%s{} i%s;\n", parameter[2].m_value.m_literal, parameter[5].m_value.m_literal);
}
break;

						case 68:// ClassMethod : literalIndentifer DataType ClassMethodName ( functionArgumentList ) ; 
{
	const char* callback = IsCallback (parameter[1].m_value.m_literal);
	if (callback) {
		sprintf (entry.m_value.m_literal, "-(i%s) %s%s;\n", callback, parameter[2].m_value.m_literal, parameter[4].m_value.m_literal);
	} else {
		sprintf (entry.m_value.m_literal, "-(%s) %s%s;\n", parameter[1].m_value.m_literal, parameter[2].m_value.m_literal, parameter[4].m_value.m_literal);
	}
}
break;

						case 38:// DataInstance : DataType literalIndentifer [ _NUMERIC_CONSTANT ] ; 
{
	char tabs[256];
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%s%s %s[%s];\n", tabs, parameter[0].m_value.m_literal, parameter[1].m_value.m_literal, parameter[3].m_value.m_literal);
}
break;

						case 73:// functionArgument : DataType literalIndentifer [ ] 
{
	sprintf (entry.m_value.m_literal, ": (%s*) %s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
 }
break;

						case 74:// functionArgument : DataType literalIndentifer [ ] , 
{
	sprintf (entry.m_value.m_literal, ": (%s*) %s", parameter[0].m_value.m_literal, parameter[1].m_value.m_literal);
 }
break;

						case 39:// DataInstance : DataType literalIndentifer [ _NUMERIC_CONSTANT ] [ _NUMERIC_CONSTANT ] ; 
{		
	char tabs[256];
	GenerateTabs (tabs);
	sprintf (entry.m_value.m_literal, "%s%s %s[%s][%s];\n", tabs, parameter[0].m_value.m_literal, parameter[1].m_value.m_literal, parameter[3].m_value.m_literal, parameter[6].m_value.m_literal);		
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
					_ASSERTE (0);
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






