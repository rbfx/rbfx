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

#include "dLexScannerGenerator.h"

#ifdef _MSC_VER
	#pragma warning (disable: 4100) //  unreferenced formal parameter
#endif


class dLexScannerGenerator::dExpandedState: public dAutomataState
{
	public:
	dExpandedState (int id)
		:dAutomataState (id)
		,m_lineNumber(0)
		,m_hasUserAction (false)
		,m_userAction("")
	{
	}	

	~dExpandedState()
	{
	}

	int m_lineNumber;
	bool m_hasUserAction;	
	dString m_userAction;
};



dLexScannerGenerator::dLexScannerGenerator(const dNonDeterministicFiniteAutonata& nfa)
	:dDeterministicFiniteAutonata (), m_stateCount(0), m_nfa(&nfa)
{
	CreateDeterministicFiniteAutomaton (nfa);
	dAssert (m_charaterSetMap.GetSets().GetCount() == 0);

	dTree<dAutomataState*,dAutomataState*> filter;

	int stack = 1;
	dAutomataState* pool[128];

	pool[0] = m_startState;
	filter.Insert(pool[0], pool[0]);

	dList<dAutomataState*> stateList;
	while (stack) {
		stack --;

		dExpandedState* const state = (dExpandedState*) pool[stack];
		dAssert (filter.Find(state));

		if (state != m_startState) {
			if (state->m_exitState) {
				stateList.Addtop(state);
			} else {
				stateList.Append(state);
			}
		}

		for (dList<dAutomataState::dTransition>::dListNode* node = state->m_transtions.GetFirst(); node; node = node->GetNext()) {
			dAutomataState::dTransition& sourceTransition = node->GetInfo();
			dAutomataState* const targetState = sourceTransition.GetState();

			if (!filter.Find(targetState)) {
				pool[stack] = targetState;
				filter.Insert(targetState, targetState);
				stack ++;
				dAssert (stack < sizeof (pool)/sizeof (pool[0]));
			}
		}
	}
	stateList.Addtop(m_startState);

	int index = 0;
	for (dList<dAutomataState*>::dListNode* node = stateList.GetFirst(); node; node = node->GetNext()) {
		dAutomataState* const state = node->GetInfo();
		state->m_id = index;
		index ++;
	}

}

dLexScannerGenerator::~dLexScannerGenerator(void)
{
}


dAutomataState* dLexScannerGenerator::CreateState (int id)
{
	m_stateCount ++;
	return new dExpandedState (id); 
}

dAutomataState* dLexScannerGenerator::CreateTargetState (dTree<dAutomataState*,dAutomataState*>& subSet, int id)
{
	dExpandedState* const state = (dExpandedState*) dDeterministicFiniteAutonata::CreateTargetState (subSet, id);

	dExpandedState* userActionState = NULL;
	dTree<dAutomataState*,dAutomataState*>::Iterator iter (subSet);
	for (iter.Begin(); iter; iter ++) {
		dExpandedState* const subSetState = (dExpandedState*) iter.GetNode()->GetInfo();
		if (subSetState->m_hasUserAction) {
			if (!userActionState) {
				userActionState = subSetState;
			} else if (userActionState->m_id < subSetState->m_id) {
				userActionState = subSetState;
			}
		}
	}

	if (userActionState) {
		state->m_hasUserAction = true;
		state->m_lineNumber = userActionState->m_lineNumber;
		state->m_userAction = userActionState->m_userAction;
	}

	return state;
}


void dLexScannerGenerator::ReplaceMacro (dString& data, const dString& newName, const char* const macro) const
{
	int size = int (strlen(macro));
	int position = data.Find (macro);
	data.Replace(position, size, newName);
}


void dLexScannerGenerator::ReplaceAllMacros (dString& data, const dString& newName, const char* const macro) const
{
	int size = int (strlen (macro));
	for (int i = data.Find(macro); i != -1; i = data.Find(macro, i)) {
		data.Replace(i, size, newName);
	}
}


void dLexScannerGenerator::LoadTemplateFile(const char* const templateName, dString& templateOuput) const
{
	char path[2048];

	// in windows
	if (GetModuleFileName(NULL, path, sizeof(path))) { 

		//	for Linux:
		//	char szTmp[32]; 
		//	sprintf(szTmp, "/proc/%d/exe", getpid()); 
		//	int bytes = MIN(readlink(szTmp, pBuf, len), len - 1); 
		//	if(bytes >= 0)
		//		pBuf[bytes] = '\0'; 
		char* const ptr = strrchr (path, '\\') + 1;
		sprintf (ptr, templateName);

		FILE* const templateFile = fopen (path, "rb");
		dTrace((path));
		dAssert (templateFile);

		templateOuput.LoadFile(templateFile);
		fclose (templateFile);	
	} else {
		dAssert (0);
	}
}


void dLexScannerGenerator::SaveFile(const char* const fileName, const char* const extention, const dString& input) const
{
	char path[2048];

	strcpy (path, fileName);
	char* const ptr1 = strrchr (path, '.');
	if (ptr1) {
		*ptr1 = 0;
	}
	strcat (path, extention);
	FILE* const headerFile = fopen (path, "wb");
	dAssert (headerFile);
	fprintf (headerFile, "%s", input.GetStr());
	fclose (headerFile);

}



void dLexScannerGenerator::CreateHeaderFile (const char* const fileName, const dString& className) const
{
	dString templateHeader;
	LoadTemplateFile("dLexicalTemplate_h.txt", templateHeader);

	ReplaceAllMacros (templateHeader, className, "$(className)");

	SaveFile(fileName, ".h", templateHeader);
}



int dLexScannerGenerator::CompareChar (const void* ptr0, const void* ptr1)
{
	const dNextStateSymbolPair& ch0 = *((dNextStateSymbolPair*) ptr0);
	const dNextStateSymbolPair& ch1 = *((dNextStateSymbolPair*) ptr1);

	if (ch0.m_symbol < ch1.m_symbol) {
		return -1;
	} else if (ch0.m_symbol > ch1.m_symbol) {
		return 1;
	}
	dAssert (0);
	return 0;
}


void dLexScannerGenerator::AddText (dString& output, const char* const fmt, ...) const
{
	va_list v_args;
	char text[2048];

	text[0] = '\t';
	text[1] = '\t';
	text[2] = '\t';
	text[3] = 0;
	va_start (v_args, fmt);     
	vsprintf(&text[3], fmt, v_args);
	va_end (v_args);            

	output += text;
}


void dLexScannerGenerator::CreateCodeFile (const char* const fileName, const dString& className, const dString& userPreHeaderCode, const dString& userPostHeaderCode) const
{
	dString templateCode;
	LoadTemplateFile("dLexicalTemplate_cpp.txt", templateCode);

	ReplaceMacro (templateCode, userPreHeaderCode, "$(userIncludeCode)");
	ReplaceAllMacros (templateCode, className, "$(className)");

	dTree<dAutomataState*,dAutomataState*> filter;

	int stack = 1;
	dAutomataState* pool[128];

	pool[0] = m_startState;
	filter.Insert(pool[0], pool[0]);

	dTree<dExpandedState*, int> stateSort;
	while (stack) {
		stack --;

		dExpandedState* const state = (dExpandedState*) pool[stack];
		dAssert (filter.Find(state));
		stateSort.Insert(state, state->m_id);
		for (dList<dAutomataState::dTransition>::dListNode* node = state->m_transtions.GetFirst(); node; node = node->GetNext()) {
			dAutomataState::dTransition& sourceTransition = node->GetInfo();
			dAutomataState* const targetState = sourceTransition.GetState();

			if (!filter.Find(targetState)) {
				pool[stack] = targetState;
				filter.Insert(targetState, targetState);
				stack ++;
				dAssert (stack < sizeof (pool)/sizeof (pool[0]));
			}
		}
	}

	int nextEntry = 0;
	int nextStateNewLineCount = 0;
	int stateTransitionNewLineCount = 0;
	
	dString transtionsCount = "";
	dString transitionsStart = "";
	dString nextCharaterList = "";
	dString nextStateList = "";
	dString semanticAction = "";
	dTree <int, dString> stateTable;

	dTree<dExpandedState*, int>::Iterator iter (stateSort);
	for (iter.Begin(); iter; iter ++) {
		dExpandedState* const state = iter.GetNode()->GetInfo();
		if (state->m_exitState) {
			AddText (semanticAction, "\tcase %d:\n", state->m_id);
			AddText (semanticAction, "\t{\n");
			//AddText (semanticAction, "\t\tGetLexString ();\n");
			const char* const userAction = state->m_userAction.GetStr();
			AddText(semanticAction, "\t\t%s\n", userAction);
			if (!strstr(userAction, "return")) {
				AddText (semanticAction, "\t\tstate = 0;\n");
				AddText (semanticAction, "\t\tch = NextChar();\n");
				AddText (semanticAction, "\t\tbreak;\n");
			}
			AddText (semanticAction, "\t}\n");
		}

		int count = 0;
		dNextStateSymbolPair symbols[256];
		for (dList<dAutomataState::dTransition>::dListNode* node = state->m_transtions.GetFirst(); node; node = node->GetNext()) {

			const dAutomataState::dTransition& sourceTransition = node->GetInfo();
			dAutomataState* const targetState = sourceTransition.GetState();

			dAutomataState::dCharacter ch (sourceTransition.GetCharater());
			dAssert (ch.m_type == dAutomataState::CHARACTER);
			symbols[count].m_symbol = char (GetScapeChar(ch.m_info));
			symbols[count].m_nextState = targetState->m_id;
			count ++;
		}

		char text [256];
		qsort (symbols, count, sizeof (dNextStateSymbolPair), CompareChar);

		dString charSet ("");
		dString states ("");
		for (int i = 0; i < count;i ++) {
			sprintf (text, "%d, ", symbols[i].m_symbol); 
			charSet += text;

			sprintf (text, "%d, ", symbols[i].m_nextState); 
			states += text;
		}
		dString key (charSet + states);
		dTree <int, dString>::dTreeNode* node = stateTable.Find(key);

		if (!node) {
			node = stateTable.Insert(nextEntry, key);
			nextEntry += count;

			for (int i = 0; i < count; i ++) {
				if ((nextStateNewLineCount % 24) == 0) {
					nextCharaterList += "\n\t\t\t";
					nextStateList += "\n\t\t\t";
				}
				nextStateNewLineCount ++;

				sprintf (text, "%d, ", symbols[i].m_symbol); 
				nextCharaterList += text;

				sprintf (text, "%d, ", symbols[i].m_nextState); 
				nextStateList += text;
			}
		}
	
		if ((stateTransitionNewLineCount % 24) == 0) {
			transitionsStart += "\n\t\t\t";
			transtionsCount += "\n\t\t\t";
		}
		stateTransitionNewLineCount ++;

		sprintf (text, "%d, ", node->GetInfo()); 
		transitionsStart += text;

		sprintf (text, "%d, ", count); 
		transtionsCount += text;
	}

	nextCharaterList.Replace(nextCharaterList.Size() - 2, 2, "");
	ReplaceMacro (templateCode, nextCharaterList, "$(nextCharaterList)");

	nextStateList.Replace(nextStateList.Size()-2, 2, "");
	ReplaceMacro (templateCode, nextStateList, "$(nextStateList)");

	transitionsStart.Replace(transitionsStart.Size()-2, 2, "");
	ReplaceMacro (templateCode, transitionsStart, "$(transitionsStart)");

	transtionsCount.Replace(transtionsCount.Size()-2, 2, "");
	ReplaceMacro (templateCode, transtionsCount, "$(transitionsCount)");

	ReplaceMacro (templateCode, semanticAction, "$(semanticActionCode)");
	templateCode = templateCode + userPostHeaderCode;

	SaveFile(fileName, ".cpp", templateCode);

}
