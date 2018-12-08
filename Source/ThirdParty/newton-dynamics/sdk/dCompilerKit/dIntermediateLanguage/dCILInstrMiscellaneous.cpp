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

#include "dCILstdafx.h"
#include "dCIL.h"
#include "dCILInstr.h"
#include "dCILInstrBranch.h"
#include "dCILInstrMiscellaneous.h"


dCILInstrNop::dCILInstrNop(dCIL& program)
	:dCILInstr (program)
{
}

void dCILInstrNop::Serialize(char* const textOut) const
{
	//	sprintf(textOut, "\tnop\n");
	textOut[0] = 0;
}



dCILInstrFunction::dCILInstrFunction (dCIL& program, const dString& name, const dArgType& type)
	:dCILInstr (program)
	,m_name (name, type)
	,m_parameters()
{
}


dList<dCILInstr::dArg>::dListNode* dCILInstrFunction::AddParameter (const dString& name, const dArgType& type)
{
	return m_parameters.Append (dArg (name, type));
}


void dCILInstrFunction::Serialize(char* const textOut) const
{
	sprintf (textOut, "function %s %s (", m_name.GetTypeName().GetStr(), m_name.m_label.GetStr());
	for (dList<dArg>::dListNode* node = m_parameters.GetFirst(); node; node = node->GetNext()) { 
		char text[2048];
		const dArg& arg = node->GetInfo();
		if (node->GetNext()) {
			sprintf (text, "%s %s, ", arg.GetTypeName().GetStr(), arg.m_label.GetStr());
		} else {
			sprintf (text, "%s %s", arg.GetTypeName().GetStr(), arg.m_label.GetStr());
		}
		strcat (textOut, text);
	}

	strcat (textOut, ")\n");
}


dCILInstrFunctionEnd::dCILInstrFunctionEnd(dCILInstrFunction* const functionBegin)
	:dCILInstr(*functionBegin->GetCil())
	,m_function(functionBegin)
{
}

void dCILInstrFunctionEnd::Serialize(char* const textOut) const
{
	textOut[0] = '\n';
	textOut[1] = 0;
}

