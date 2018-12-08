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
#include "dRegisterInterferenceGraph.h"
#include "dConditionalConstantPropagationSolver.h"

dCILInstrLabel::dCILInstrLabel(dCIL& program, const dString& label)
	:dCILInstr(program)
	,m_label(label)
{
}

bool dCILInstrLabel::IsBasicBlockBegin() const
{
	return true;
}

dCILInstrLabel* dCILInstrLabel::GetAsLabel()
{
	return this;
}

const dString& dCILInstrLabel::GetLabel() const
{
	return m_label;
}


void dCILInstrLabel::Serialize(char* const textOut) const
{
	sprintf (textOut, "%s:\n", GetLabel().GetStr());
}

dCILInstrGoto::dCILInstrGoto(dCIL& program, const dString& label)
	:dCILInstr(program)
	,m_label(label)
	,m_tagetNode(NULL)
{
}

dCILInstrGoto::dCILInstrGoto(dCIL& program, dCILInstrLabel* const target)
	:dCILInstr(program)
	,m_label(target->GetLabel())
	,m_tagetNode(target->GetNode())
{
}

bool dCILInstrGoto::IsBasicBlockEnd() const
{
	return true;
}

void dCILInstrGoto::Serialize(char* const textOut) const
{
	sprintf (textOut, "\tgoto %s\n", m_label.GetStr());
}

void dCILInstrGoto::SetLabel (const dString& label)
{
	m_label = label;
}

const dString& dCILInstrGoto::GetLabel() const 
{ 
	return m_label; 
}

void dCILInstrGoto::SetTarget (dCILInstrLabel* const target)
{
	m_tagetNode = target->GetNode();
	SetLabel(target->GetLabel());
	dAssert (m_label == target->GetLabel());
}

dList<dCILInstr*>::dListNode* dCILInstrGoto::GetTarget () const
{
	return m_tagetNode;
}

dCILInstrGoto* dCILInstrGoto::GetAsGoto()
{
	return this;
}

void dCILInstrGoto::ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver)
{
	dCILInstrLabel* const target = GetTarget()->GetInfo()->GetAsLabel();
	dConditionalConstantPropagationSolver::dBlockEdgeKey key1(GetBasicBlock(), target->GetBasicBlock());
	if (!solver.m_executableEdges.Find(key1)) {
		solver.m_executableEdges.Insert(1, key1);
		solver.m_blockWorklist.Append(target->GetBasicBlock());
	}
}


dCILInstrConditional::dCILInstrConditional(dCIL& program, dOperator operation, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1, const dString& target0, const dString& target1)
	:dCILTwoArgInstr(program, dArg (name0, type0), dArg(name1, type1))
	,m_label0(target0)
	,m_label1(target1)
	,m_targetNode0(NULL)
	,m_targetNode1(NULL)
	,m_operator(operation)
{
}


dCILInstrConditional::dCILInstrConditional(dCIL& program, dOperator operation, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1, dCILInstrLabel* const target0, dCILInstrLabel* const target1)
	:dCILTwoArgInstr(program, dArg(name0, type0), dArg(name1, type1))
	,m_label0(target0->GetLabel())
	,m_label1(target1->GetLabel())
	,m_targetNode0(target0->GetNode())
	,m_targetNode1(target1->GetNode())
	,m_operator(operation)
{
}

void dCILInstrConditional::Serialize(char* const textOut) const
{
	const char* const assignOperator = GetOperatorString(m_operator);
	if (m_targetNode1) {
		sprintf(textOut, "\tif (%s %s %s %s %s) goto %s else goto %s\n", 
								m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr(), 
								assignOperator,
								m_arg1.GetTypeName().GetStr(), m_arg1.m_label.GetStr(),
								m_label0.GetStr(), m_label1.GetStr());
	} else {
		dAssert(0);
		//sprintf(textOut, "\tif (%s %s %s %s %s) goto %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr(), m_arg1.m_label.GetStr());
	}
}

void dCILInstrConditional::SetLabels (const dString& label0, const dString& label1)
{
	m_label0 = label0;
	m_label1 = label1;
}

void dCILInstrConditional::SetTargets (dCILInstrLabel* const target0, dCILInstrLabel* const target1)
{
	dAssert(target0);
	m_targetNode0 = target0->GetNode();

	if (target1) {
		dAssert(target1->GetLabel() == m_label1);
		m_targetNode1 = target1->GetNode();
	} else {
		m_targetNode1 = NULL;
	}
	SetLabels (target0->GetLabel(), target1->GetLabel());
}

dList<dCILInstr*>::dListNode* dCILInstrConditional::GetTrueTarget () const
{
	return m_targetNode0;
}

dList<dCILInstr*>::dListNode* dCILInstrConditional::GetFalseTarget () const
{
	return m_targetNode1;
}

const dString& dCILInstrConditional::GetTrueLabel() const
{
	return m_label0;
}

const dString& dCILInstrConditional::GetFalseLabel() const
{
	return m_label1;
}


dCILInstrConditional* dCILInstrConditional::GetAsIF()
{
	return this;
}

bool dCILInstrConditional::IsBasicBlockEnd() const
{
	return true;
}


void dCILInstrConditional::GetUsedVariables (dList<dArg*>& variablesList)
{
	if (!((m_arg0.GetType().m_intrinsicType == m_constInt) || (m_arg0.GetType().m_intrinsicType == m_constFloat) || (m_arg0.GetType().m_intrinsicType == m_constString))) {
		variablesList.Append(&m_arg0);
	}

	if (!((m_arg1.GetType().m_intrinsicType == m_constInt) || (m_arg1.GetType().m_intrinsicType == m_constFloat) || (m_arg1.GetType().m_intrinsicType == m_constString))) {
		variablesList.Append(&m_arg1);
	}
}

void dCILInstrConditional::AddUsedVariable (dInstructionVariableDictionary& dictionary) const 
{
	dAssert(0);
/*
	dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg0.m_label);
	node->GetInfo().Append(m_myNode);
*/
}


void dCILInstrConditional::EmitOpcode(dVirtualMachine::dOpCode* const codeOutPtr) const
{
	dAssert(0);
/*
	dVirtualMachine::dOpCode& code = codeOutPtr[m_byteCodeOffset];
	code.m_type2.m_opcode = m_mode == m_ifnot ? unsigned(dVirtualMachine::m_bneq) : unsigned(dVirtualMachine::m_beq);
	code.m_type2.m_reg0 = RegisterToIndex(m_arg0.m_label);
	code.m_type2.m_imm2 = m_targetNode0->GetInfo()->GetByteCodeOffset() - (m_byteCodeOffset + GetByteCodeSize()); 
*/
}

bool dCILInstrConditional::ReplaceArgument (const dArg& arg, const dArg& newArg)
{
	bool change = false;
	if (arg.m_label == m_arg0.m_label) {
		if (m_arg0.m_label != newArg.m_label) {
			change = true;
			m_arg0 = newArg;
		}
	}

	if (arg.m_label == m_arg1.m_label) {
		if (m_arg1.m_label != newArg.m_label) {
			change = true;
			m_arg1 = newArg;
		}
	}
	return change;
}

void dCILInstrConditional::AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph)
{
	dList<dArg*> variablesList;
	GetUsedVariables (variablesList);
	for(dList<dArg*>::dListNode * node = variablesList.GetFirst(); node; node = node->GetNext()) {
		dArg* const arg = node->GetInfo();
		arg->m_label = interferenceGraph.GetRegisterName(arg->m_label);
	}
}

void dCILInstrConditional::ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver)
{
//Trace();
/*
	if ((m_arg0.GetType().m_intrinsicType == m_constInt) || (m_arg0.GetType().m_intrinsicType == m_constFloat)) {
		dAssert (0);
		return;
	} else {
		dAssert (solver.m_variablesList.Find(m_arg0.m_label));
		dConstantPropagationSolver::dVariable& variable = solver.m_variablesList.Find(m_arg0.m_label)->GetInfo();

		if (variable.m_type == dConstantPropagationSolver::dVariable::m_constant) {
			dAssert(GetTrueTarget());
			dAssert(GetFalseTarget());

			//int condition = variable.m_constValue.ToInteger();
			//if (m_mode == dCILInstrConditional::m_ifnot) {
			//	condition = !condition;
			//}

			dCILInstrLabel* target;
			if (condition) {
				target = GetTrueTarget()->GetInfo()->GetAsLabel();
			} else {
				target = GetFalseTarget()->GetInfo()->GetAsLabel();
			}

			dConstantPropagationSolver::dBlockEdgeKey key1(GetBasicBlock(), target->GetBasicBlock());
			if (!solver.m_executableEdges.Find(key1)) {
				solver.m_executableEdges.Insert(1, key1);
				solver.m_blockWorklist.Append(target->GetBasicBlock());
			}

		} else if (variable.m_type == dConstantPropagationSolver::dVariable::m_variableValue) {
			dConstantPropagationSolver::dBlockEdgeKey key0 (GetBasicBlock(), m_targetNode0->GetInfo()->GetBasicBlock());
			if (!solver.m_executableEdges.Find(key0)) {
				solver.m_executableEdges.Insert(1, key0);
				solver.m_blockWorklist.Append(m_targetNode0->GetInfo()->GetBasicBlock());
			}

			dConstantPropagationSolver::dBlockEdgeKey key1(GetBasicBlock(), m_targetNode1->GetInfo()->GetBasicBlock());
			if (!solver.m_executableEdges.Find(key1)) {
				solver.m_executableEdges.Insert(1, key1);
				solver.m_blockWorklist.Append(m_targetNode1->GetInfo()->GetBasicBlock());
			}

//		} else if (variable.m_type == dConstantPropagationSolver::dVariable::m_variableValue) {
//			dAssert(0);
		}
	}
*/

	if (!((m_arg0.GetType().m_intrinsicType == m_constInt) || (m_arg0.GetType().m_intrinsicType == m_constFloat) || (m_arg0.GetType().m_intrinsicType == m_constString))) {
		dAssert(solver.m_variablesList.Find(m_arg0.m_label));
		//dConstantPropagationSolver::dVariable& variable = solver.m_variablesList.Find(m_arg0.m_label)->GetInfo();
		dConditionalConstantPropagationSolver::dBlockEdgeKey key0(GetBasicBlock(), m_targetNode0->GetInfo()->GetBasicBlock());
		if (!solver.m_executableEdges.Find(key0)) {
			solver.m_executableEdges.Insert(1, key0);
			solver.m_blockWorklist.Append(m_targetNode0->GetInfo()->GetBasicBlock());
		}

		dConditionalConstantPropagationSolver::dBlockEdgeKey key1(GetBasicBlock(), m_targetNode1->GetInfo()->GetBasicBlock());
		if (!solver.m_executableEdges.Find(key1)) {
			solver.m_executableEdges.Insert(1, key1);
			solver.m_blockWorklist.Append(m_targetNode1->GetInfo()->GetBasicBlock());
		}
	}

	if (!((m_arg1.GetType().m_intrinsicType == m_constInt) || (m_arg1.GetType().m_intrinsicType == m_constFloat) || (m_arg1.GetType().m_intrinsicType == m_constString))) {
		dAssert(solver.m_variablesList.Find(m_arg1.m_label));
		//dConstantPropagationSolver::dVariable& variable = solver.m_variablesList.Find(m_arg1.m_label)->GetInfo();
		dConditionalConstantPropagationSolver::dBlockEdgeKey key0(GetBasicBlock(), m_targetNode0->GetInfo()->GetBasicBlock());
		if (!solver.m_executableEdges.Find(key0)) {
			solver.m_executableEdges.Insert(1, key0);
			solver.m_blockWorklist.Append(m_targetNode0->GetInfo()->GetBasicBlock());
		}

		dConditionalConstantPropagationSolver::dBlockEdgeKey key1(GetBasicBlock(), m_targetNode1->GetInfo()->GetBasicBlock());
		if (!solver.m_executableEdges.Find(key1)) {
			solver.m_executableEdges.Insert(1, key1);
			solver.m_blockWorklist.Append(m_targetNode1->GetInfo()->GetBasicBlock());
		}
	}
}

dCILInstrReturn::dCILInstrReturn(dCIL& program, const dString& name, const dArgType& type)
	:dCILSingleArgInstr (program, dArg (name, type))
{
}

void dCILInstrReturn::Serialize(char* const textOut) const
{
	sprintf (textOut, "\tret %s %s\n", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr());
}

bool dCILInstrReturn::IsBasicBlockEnd() const
{
	return true;
}

bool dCILInstrReturn::ReplaceArgument(const dArg& arg, const dArg& newArg)
{
	//Trace ();
	bool change = false;
	if (arg.m_label == m_arg0.m_label) {
		if (m_arg0.m_label != newArg.m_label) {
			change = true;
			m_arg0 = newArg;
		}
	}
	return change;
}


dCILInstrReturn* dCILInstrReturn::GetAsReturn()
{
	return this;
}

void dCILInstrReturn::AddUsedVariable (dInstructionVariableDictionary& dictionary) const 
{
	dAssert(0);
/*
	dAssert (!m_arg0.GetType().m_isPointer);
	switch (m_arg0.GetType().m_intrinsicType) 
	{
		case m_int:
		{
			dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert(m_arg0.m_label);
			node->GetInfo().Append(m_myNode);
			break;
		}

		case m_void:
		case m_constInt:
			break;

		default:
			dAssert(0);
		}
*/
}


void dCILInstrReturn::AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph)
{
	dList<dArg*> variablesList;
	GetUsedVariables(variablesList);
	for (dList<dArg*>::dListNode * node = variablesList.GetFirst(); node; node = node->GetNext()) {
		dArg* const arg = node->GetInfo();
		arg->m_label = interferenceGraph.GetRegisterName(arg->m_label);
	}
}

int dCILInstrReturn::GetByteCodeSize() const 
{ 
	switch (m_arg0.GetType().m_intrinsicType)
	{
		case m_constInt:
			return 2;

		case m_int:
			break;

		default:
			dAssert(0);
	}

	return 1; 
}

void dCILInstrReturn::GetUsedVariables (dList<dArg*>& variablesList)
{
	switch (m_arg0.GetType().m_intrinsicType) 
	{
		case m_void:
		case m_constInt:
		case m_constFloat:
		case m_constString:
			break;
		default:
			variablesList.Append(&m_arg0);
	}
}

void dCILInstrReturn::EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const
{
	int offset = m_byteCodeOffset;
	switch (m_arg0.GetType().m_intrinsicType)
	{
		case m_constInt:
		{
			dVirtualMachine::dOpCode& code = codeOutPtr[offset];
			code.m_type2.m_opcode = unsigned(dVirtualMachine::m_movi);
			code.m_type2.m_reg0 = D_RETURN_REGISTER_INDEX;
			code.m_type2.m_imm2 = m_arg0.m_label.ToInteger();
			offset ++; 
			break;
		}

		case m_int:
			break;

		default:
			dAssert(0);
	}


	dVirtualMachine::dOpCode& code = codeOutPtr[offset];
	code.m_type2.m_opcode = unsigned(dVirtualMachine::m_ret);
	code.m_type2.m_reg0 = D_STACK_REGISTER_INDEX;
	code.m_type2.m_imm2 = 0;
}

/*
dCILInstrCall::dCILInstrCall(dCIL& program, const dString& returnValue, const dArgType& type, const dString& target, dList<dArg>& parameters)
	:dCILTwoArgInstr (program, dArg (returnValue, type), dArg (target, dArgType()))
	,m_tagetNode(NULL)
{
	for (dList<dArg>::dListNode* node = parameters.GetFirst(); node; node = node->GetNext()) {
		m_parameters.Append (node->GetInfo());
	}
}
*/

dCILInstrCall::dCILInstrCall(dCIL& program, const dString& returnValue, const dArgType& type, const dString& functionName)
	:dCILTwoArgInstr(program, dArg(returnValue, type), dArg(functionName, dArgType()))
//	,m_tagetNode(NULL)
{
}


void dCILInstrCall::AddArgument (const dArg& argument)
{
	m_parameters.Append (argument);
}

void dCILInstrCall::Serialize(char* const textOut) const
{
	//if (m_arg0.m_isPointer || (m_arg0.m_intrinsicType != m_void)) {
	if (m_arg0.m_intrinsicType != m_void) {
		sprintf (textOut, "\t%s %s = call %s (", m_arg0.GetTypeName().GetStr(), m_arg0.m_label.GetStr(), m_arg1.m_label.GetStr());
	} else {
		sprintf (textOut, "\t%s call %s (", m_arg0.GetTypeName().GetStr(), m_arg1.m_label.GetStr());
	}


	for (dList<dArg>::dListNode* node = m_parameters.GetLast(); node; node = node->GetPrev()) { 
		char text[2048];
		const dArg& arg = node->GetInfo();
		if (node->GetPrev()) {
			sprintf (text, "%s %s, ", arg.GetTypeName().GetStr(), arg.m_label.GetStr());
		} else {
			sprintf (text, "%s %s", arg.GetTypeName().GetStr(), arg.m_label.GetStr());
		}
		strcat (textOut, text);
	}
	strcat (textOut, ")\n");
}


bool dCILInstrCall::ReplaceArgument(const dArg& arg, const dArg& newArg)
{
	bool change = false;
	for (dList<dArg>::dListNode* node = m_parameters.GetLast(); node; node = node->GetPrev()) {
		dArg& paramArg = node->GetInfo();
		if (arg.m_label == paramArg.m_label) {
			if (paramArg.m_label != newArg.m_label) {
				change = true;
				//paramArg.m_label = newArg;
				paramArg = newArg;
			}
		}
	}

	return dCILTwoArgInstr::ReplaceArgument(arg, newArg) | change;
}

void dCILInstrCall::AddUsedVariable (dInstructionVariableDictionary& dictionary) const
{
	dAssert(0);
/*
	for (dList<dArg>::dListNode* node = m_parameters.GetFirst(); node; node = node->GetNext()) {
		const dArg& arg = node->GetInfo();
		dInstructionVariableDictionary::dTreeNode* const node0 = dictionary.Insert(arg.m_label);
		node0->GetInfo().Append(m_myNode);
	}
*/
}


void dCILInstrCall::AddDefinedVariable (dInstructionVariableDictionary& dictionary) const 
{
	dAssert(0);
/*
	if (m_arg0.GetType().m_isPointer || (m_arg0.GetType().m_intrinsicType != m_void)) {
		dInstructionVariableDictionary::dTreeNode* const node = dictionary.Insert (m_arg0.m_label);
		node->GetInfo().Append (m_myNode);
	}
*/	
}

void dCILInstrCall::GetUsedVariables(dList<dArg*>& variablesList)
{
	for (dList<dArg>::dListNode* node = m_parameters.GetFirst(); node; node = node->GetNext()) {
		variablesList.Append(&node->GetInfo());
	}
}



dCILInstr::dArg* dCILInstrCall::GetGeneratedVariable()
{
	//if (m_arg0.GetType().m_isPointer || (m_arg0.GetType().m_intrinsicType != m_void)) {
	if (m_arg0.GetType().m_intrinsicType != m_void) {
		return &m_arg0;
	} else {
		return NULL;
	}
}


void dCILInstrCall::AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph)
{
	//if (m_arg0.GetType().m_isPointer || (m_arg0.GetType().m_intrinsicType != m_void)) {
	if (m_arg0.GetType().m_intrinsicType != m_void) {
		m_arg0.m_label = interferenceGraph.GetRegisterName(m_arg0.m_label);
	}

	for (dList<dArg>::dListNode* node = m_parameters.GetFirst(); node; node = node->GetNext()) {
		dArg& arg = node->GetInfo();
		arg.m_label = interferenceGraph.GetRegisterName(arg.m_label);
	}
}

void dCILInstrCall::EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const 
{
//	dAssert (m_tagetNode);
	dAssert (0);
//	dVirtualMachine::dOpCode& code = codeOutPtr[m_byteCodeOffset];
//	code.m_type2.m_opcode = unsigned(dVirtualMachine::m_calli);
//	code.m_type2.m_reg0 = D_STACK_REGISTER_INDEX;
//	code.m_type2.m_imm2 = m_tagetNode->GetInfo()->GetByteCodeOffset() - (m_byteCodeOffset + GetByteCodeSize()); 
}
