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
#include "dCILInstrLoadStore.h"
#include "dConditionalConstantPropagationSolver.h"

dCILInstrArgument::dCILInstrArgument(dCIL& program, const dString& name, const dArgType& type)
	:dCILSingleArgInstr(program, dArg(name, type))
{
}

void dCILInstrArgument::Serialize(char* const textOut) const
{
	sprintf(textOut, "\targument %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
}


void dCILInstrArgument::AddDefinedVariable(dInstructionVariableDictionary& dictionary) const 
{ 
	dAssert(0);
//	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg0.m_label);
//	node->GetInfo().Append(m_myNode);
}


dCILInstrLocal::dCILInstrLocal (dCIL& program, const dString& name, const dArgType& type)
	:dCILSingleArgInstr (program, dArg (name, type))
{
}

void dCILInstrLocal::Serialize(char* const textOut) const
{
	sprintf (textOut, "\tlocal %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
}

void dCILInstrLocal::AddDefinedVariable (dInstructionVariableDictionary& dictionary) const  
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert (m_arg0.m_label);
	node->GetInfo().Append (m_myNode);
*/
}

dCILInstrMove::dCILInstrMove (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1)
	:dCILTwoArgInstr (program, dArg (name0, type0), dArg (name1, type1))
{
}

dCILInstrMove::dCILInstrMove (const dCILInstrMove& copy)
	:dCILTwoArgInstr (copy)
{
}


void dCILInstrMove::Serialize(char* const textOut) const
{
	sprintf(textOut, "\t%s %s = %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr(), m_arg1.GetTypeName().GetStr(), m_arg1.m_label.GetStr());
}

void dCILInstrMove::AddDefinedVariable(dInstructionVariableDictionary& dictionary) const
{ 
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg0.m_label);
	node->GetInfo().Append(m_myNode);
*/
}


void dCILInstrMove::AddUsedVariable (dInstructionVariableDictionary& dictionary) const
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg1.m_label);
	node->GetInfo().Append(m_myNode);
*/
}


void dCILInstrMove::GetUsedVariables (dList<dArg*>& variablesList)
{
	switch (m_arg1.GetType().m_intrinsicType) 
	{
		case m_constInt:
		case m_constFloat:
		case m_constString:
			break;
		default:
			variablesList.Append(&m_arg1);
	}
}

bool dCILInstrMove::ReplaceArgument (const dArg& arg, const dArg& newArg)
{
	bool change = false;
	if (arg.m_label == m_arg1.m_label) {
		if (m_arg1.m_label != newArg.m_label) {
			change = true;
			m_arg1 = newArg;
		}
	}
	return change;
}

bool dCILInstrMove::ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary)
{
	bool ret = false;
	if ((m_arg1.GetType().m_intrinsicType == dCILInstr::m_constInt) || (m_arg1.GetType().m_intrinsicType == dCILInstr::m_constFloat) || (m_arg1.GetType().m_intrinsicType == dCILInstr::m_constString)) {
		dStatementBlockDictionary::dTreeNode* const node = usedVariablesDictionary.Find(m_arg0.m_label);
		if (node) {
			//ret = true;
			dStatementBlockBucket::Iterator iter (node->GetInfo());
			for (iter.Begin(); iter; iter ++) {
				dCILInstr* const instruction = iter.GetKey()->GetInfo();
				if (!(instruction->GetAsReturn() || instruction->GetAsCall())) {
					ret |= instruction->ReplaceArgument(m_arg0, m_arg1);
					workList.Insert(instruction);
				}
			}
		}
	}
	return ret;
}

void dCILInstrMove::ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver)
{
	dAssert (0);
	dConditionalConstantPropagationSolver::dVariable::dValueTypes type = dConditionalConstantPropagationSolver::dVariable::m_undefined;
	dString value ("");
	if ((m_arg1.GetType().m_intrinsicType == m_constInt) || (m_arg1.GetType().m_intrinsicType == m_constFloat) || (m_arg1.GetType().m_intrinsicType == dCILInstr::m_constString)) {
		type = dConditionalConstantPropagationSolver::dVariable::m_constant;
		value = m_arg1.m_label;
	} else {
		dConditionalConstantPropagationSolver::dVariable& variable = solver.m_variablesList.Find(m_arg1.m_label)->GetInfo();
		type = variable.m_type;
		value = variable.m_constValue;
	}

	if (type != dConditionalConstantPropagationSolver::dVariable::m_undefined) {
		solver.UpdateLatice (m_arg0, value, type);
	}
}

bool dCILInstrMove::ApplyCopyPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary)
{
	bool ret = false;
	if (!((m_arg1.GetType().m_intrinsicType == dCILInstr::m_constInt) || (m_arg1.GetType().m_intrinsicType == dCILInstr::m_constFloat) || (m_arg1.GetType().m_intrinsicType == dCILInstr::m_constString))) {
		dStatementBlockDictionary::dTreeNode* const node = usedVariablesDictionary.Find(m_arg0.m_label);
//Trace();
		if (node) {
			dStatementBlockBucket::Iterator iter(node->GetInfo());
			for (iter.Begin(); iter; iter++) {
				dCILInstr* const instruction = iter.GetKey()->GetInfo();
//instruction->Trace();
				ret |= instruction->ReplaceArgument(m_arg0, m_arg1);
				workList.Insert(instruction);
			}
		}
	}
	return ret;
}


void dCILInstrMove::EmitOpcode(dVirtualMachine::dOpCode* const codeOutPtr) const
{
	dVirtualMachine::dOpCode& code = codeOutPtr[m_byteCodeOffset];
	code.m_type3.m_opcode = unsigned (dVirtualMachine::m_mov);
	code.m_type3.m_reg0 = RegisterToIndex(m_arg0.m_label);
	code.m_type3.m_reg1 = RegisterToIndex(m_arg1.m_label);
	code.m_type3.m_imm3 = 0;
}


dCILInstrLoad::dCILInstrLoad (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1)
	:dCILTwoArgInstr (program, dArg (name0, type0), dArg (name1, type1))
{
}

void dCILInstrLoad::Serialize(char* const textOut) const
{
	sprintf (textOut, "\t%s %s = [%s %s]\n", m_arg0.GetTypeName().GetStr(),  m_arg0.m_label.GetStr(), m_arg1.GetTypeName().GetStr(), m_arg1.m_label.GetStr());
}


void dCILInstrLoad::GetUsedVariables (dList<dArg*>& variablesList)
{
	variablesList.Append(&m_arg1);
}

void dCILInstrLoad::AddDefinedVariable (dInstructionVariableDictionary& dictionary) const  
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert (m_arg0.m_label);
	node->GetInfo().Append (m_myNode);
*/
}

void dCILInstrLoad::AddUsedVariable (dInstructionVariableDictionary& dictionary) const 
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg1.m_label);
	node->GetInfo().Append(m_myNode);
*/
}


dCILInstrStore::dCILInstrStore (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1)
	:dCILTwoArgInstr (program, dArg (name0, type0), dArg (name1, type1))
{
}

void dCILInstrStore::Serialize(char* const textOut) const
{
	sprintf (textOut, "\t[%s %s] = %s %s\n", m_arg0.GetTypeName().GetStr(),  m_arg0.m_label.GetStr(), m_arg1.GetTypeName().GetStr(), m_arg1.m_label.GetStr());
}


void dCILInstrStore::AddUsedVariable (dInstructionVariableDictionary& dictionary) const 
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node0 = dictionary.Insert(m_arg0.m_label);
	node0->GetInfo().Append(m_myNode);

	dInstructionVariableDictionary::dTreeNode* const node1 = dictionary.Insert(m_arg1.m_label);
	node1->GetInfo().Append(m_myNode);
*/
}


void dCILInstrStore::GetUsedVariables(dList<dArg*>& variablesList)
{
	variablesList.Append(&m_arg0);
	variablesList.Append(&m_arg1);
}



dCILInstrPhy::dCILInstrPhy (dCIL& program, const dString& name, const dArgType& type, const dBasicBlock* const basicBlock, dList<const dBasicBlock*>& sources)
	:dCILSingleArgInstr(program, dArg(name, type))
{
	m_basicBlock = (dBasicBlock*)basicBlock;
	for (dList<const dBasicBlock*>::dListNode* node = sources.GetFirst(); node; node = node->GetNext()) {
		m_sources.Append (dArgPair (m_arg0, node->GetInfo()));
	}
}


void dCILInstrPhy::Serialize(char* const textOut) const
{
	sprintf(textOut, "\t%s %s = phi (", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
	for (dList<dArgPair>::dListNode* node = m_sources.GetFirst(); node; node = node->GetNext()) {
		char tmp[1024]; 

		dArgPair& pair = node->GetInfo();
		if (node->GetNext()) {
			sprintf(tmp, "%s %s, ", pair.m_arg.GetTypeName().GetStr(), pair.m_arg.m_label.GetStr());
		} else {
			sprintf(tmp, "%s %s", pair.m_arg.GetTypeName().GetStr(), pair.m_arg.m_label.GetStr());
		}
		strcat(textOut, tmp);
	}
	strcat(textOut, ")\n");
}

void dCILInstrPhy::GetUsedVariables (dList<dArg*>& variablesList)
{
	for (dList<dArgPair>::dListNode* node = m_sources.GetFirst(); node; node = node->GetNext()) {
		dArg* const arg = &node->GetInfo().m_arg;
		//if (arg->GetType().m_isPointer) {
		//	variablesList.Append(arg);
		//} else 
		{
			switch (arg->GetType().m_intrinsicType) 
			{
				case m_constInt:
				case m_constFloat:
				case m_constString:
					break;

				default:
					variablesList.Append(arg);
			}
		}
	}
}


bool dCILInstrPhy::ReplaceArgument(const dArg& arg, const dArg& newArg)
{
	bool change = false;
	for (dList<dArgPair>::dListNode* node = m_sources.GetFirst(); node; node = node->GetNext()) {
		dArgPair& pair = node->GetInfo();
		if (arg.m_label == pair.m_arg.m_label) {
			if (pair.m_arg.m_label != newArg.m_label) {
				change = true;
				pair.m_arg = newArg;
				break;
			}
		}
	}
	return change;
}

bool dCILInstrPhy::ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary)
{
	bool ret = false;
	if (m_sources.GetCount()) {
		const dArg& arg = m_sources.GetFirst()->GetInfo().m_arg;
		if ((arg.GetType().m_intrinsicType == dCILInstr::m_constInt) || (arg.GetType().m_intrinsicType == dCILInstr::m_constFloat) || (arg.GetType().m_intrinsicType == dCILInstr::m_constString)) {
			ret = true;
			for (dList<dArgPair>::dListNode* node = m_sources.GetFirst()->GetNext(); node; node = node->GetNext()) {
				const dArg& arg1 = node->GetInfo().m_arg;
				ret &= (arg1.m_label == arg.m_label);
			}
		}
	}

	if (ret) {
		const dArg& arg = m_sources.GetFirst()->GetInfo().m_arg;
		dCILInstrMove* const move = new dCILInstrMove (*m_cil, m_arg0.m_label, m_arg0.GetType(), arg.m_label, arg.GetType());
		ReplaceInstruction(move);
	}
	return ret;
}


void dCILInstrPhy::ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver)
{
//dAssert (0);
/*
	dList<dArg> constList;
	for (dList<dArgPair>::dListNode* node = m_sources.GetFirst(); node; node = node->GetNext()) {
		const dArg* const arg = node->GetInfo().m_intructionNode ? node->GetInfo().m_intructionNode->GetInfo()->GetGeneratedVariable() : &node->GetInfo().m_arg;

		if ((arg->GetType().m_intrinsicType == dCILInstr::m_constInt) || (arg->GetType().m_intrinsicType == dCILInstr::m_constFloat) || (arg.GetType().m_intrinsicType == dCILInstr::m_constString)) {
			constList.Append(*arg);
		} else {
			dAssert(solver.m_variablesList.Find(arg->m_label));
			dConstantPropagationSolver::dVariable& variable = solver.m_variablesList.Find(arg->m_label)->GetInfo();
			if (variable.m_type == dConstantPropagationSolver::dVariable::m_variableValue) {
				solver.UpdateLatice (m_arg0, m_arg0.m_label, dConstantPropagationSolver::dVariable::m_variableValue);
				return; 
			}
			if (variable.m_type != dConstantPropagationSolver::dVariable::m_undefined) {
				dAssert (variable.m_type == dConstantPropagationSolver::dVariable::m_constant);
				constList.Append (dArg (variable.m_constValue, arg->GetType()));
			}
		}
	}

	if (constList.GetCount()) {
		bool equal = true;
		const dString& value = constList.GetFirst()->GetInfo().m_label; 
		for (dList<dArg>::dListNode* node = constList.GetFirst()->GetNext(); node; node = node->GetNext()) {
			equal &= (value == node->GetInfo().m_label);
		}
		if (equal) {
			solver.UpdateLatice (m_arg0, value, dConstantPropagationSolver::dVariable::m_constant);
		} else {
			solver.UpdateLatice (m_arg0, value, dConstantPropagationSolver::dVariable::m_variableValue);
		}
	}
*/	
}


dCILInstrPush::dCILInstrPush(dCIL& program, const dString& name, const dArgType& type)
	:dCILSingleArgInstr(program, dArg(name, type))
{
}

void dCILInstrPush::Serialize(char* const textOut) const
{
	sprintf (textOut, "\tpush %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
}

void dCILInstrPush::AddUsedVariable (dInstructionVariableDictionary& dictionary) const
{
	dAssert(0);
}


dCILInstrPop::dCILInstrPop(dCIL& program, const dString& name, const dArgType& type)
	:dCILSingleArgInstr(program, dArg(name, type))
{
}

void dCILInstrPop::Serialize(char* const textOut) const
{
	sprintf(textOut, "\tpop %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
}

void dCILInstrPop::AddUsedVariable(dInstructionVariableDictionary& dictionary) const
{
	dAssert(0);
}
