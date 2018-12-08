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


#include "dNewtonLuaStdafx.h"
#include "dNewtonLuaLex.h"
#include "dNewtonLuaParcer.h"
#include "dNewtonLuaCompiler.h"


#define TRACE_INSTRUCTION(x)  x->Trace()

dNewtonLuaCompiler::dLuaClosure::dLuaClosure()
	:dCIL()
	,m_parent(NULL)
	,m_returnLabel(NewLabel())
	,m_returnVariable(NewTemp())
{
}

dNewtonLuaCompiler::dLuaClosure::~dLuaClosure()
{
}

void dNewtonLuaCompiler::dLuaClosure::RemoveAll()
{
	dCIL::RemoveAll();
	m_children.RemoveAll();
}

dNewtonLuaCompiler::dLuaClosure* dNewtonLuaCompiler::dLuaClosure::AddClosure(dLuaClosure* const parent)
{
	dNewtonLuaCompiler::dLuaClosure* const closureNode = &m_children.Append()->GetInfo();
	closureNode->m_parent = parent;
	return closureNode;
}

dNewtonLuaCompiler::dNewtonLuaCompiler()
	:dNewtonLuaParcer()
	,m_closures()
	,m_currentClosure(NULL)
{
	dUserVariable functionName;
	functionName.m_data = "_main";
	EmitFunctionDeclaration(functionName, dUserVariable());
}

dNewtonLuaCompiler::~dNewtonLuaCompiler()
{
}

int dNewtonLuaCompiler::CompileSource (const char* const source)
{
	dNewtonLuaLex scanner(source);
	bool status = Parse(scanner);

#if 0
	if (status) {
		for (dList<dDAGClassNode*>::dListNode* node = m_classList.GetFirst(); node; node = node->GetNext()) {
			dDAGClassNode* const scripClass =  node->GetInfo();
			scripClass->ConnectParent (NULL);
		}

		dCIL cil;
		for (dList<dDAGClassNode*>::dListNode* node = m_classList.GetFirst(); node; node = node->GetNext()) {
			dDAGClassNode* const scripClass = node->GetInfo();
			scripClass->CompileCIL (cil);
		}

		for (dList<dDAGClassNode*>::dListNode* node = m_classList.GetFirst(); node; node = node->GetNext()) {
			dDAGClassNode* const scripClass = node->GetInfo();
			scripClass->Optimize(cil);
		}

/*
		for (dList<dDAGClassNode*>::dListNode* node = m_classList.GetFirst(); node; node = node->GetNext()) {
			dDAGClassNode* const scripClass = node->GetInfo();
			scripClass->ConvertToTarget (cil);
		}

		dVirtualMachine* const program = cil.BuilExecutable();
		delete program;
*/
	}
#endif
	return 0;
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitFunctionDeclaration(const dUserVariable& functionName, const dUserVariable& parameterList)
{
	m_currentClosure = m_closures.AddClosure(m_currentClosure);
	dCILInstrFunction* const function = new dCILInstrFunction(*m_currentClosure, functionName.GetString(), dCILInstr::dArgType(dCILInstr::m_luaType));

	dCILInstr::dArgType type(dCILInstr::m_luaType);
	for (dList<dString>::dListNode* node = parameterList.m_tokenList.GetFirst(); node; node = node->GetNext()) {
		const dString& varName = node->GetInfo();
		function->AddParameter (varName, type);
	}
	
	TRACE_INSTRUCTION(function);
	return dUserVariable(function);
}

void dNewtonLuaCompiler::CloseFunctionDeclaration()
{
	dLuaClosure::dListNode* const funtionNode = m_currentClosure->GetFirst();

	dLuaClosure::dListNode* const labelNode = funtionNode->GetNext();
	dAssert (labelNode->GetInfo()->GetAsLabel());

	dCILInstrFunction* const functionInstruction = funtionNode->GetInfo()->GetAsFunction();
	for (dList<dCILInstrFunction::dArg>::dListNode* argNode = functionInstruction->m_parameters.GetLast(); argNode; argNode = argNode->GetPrev()) {
		const dCILInstrFunction::dArg& arg = argNode->GetInfo();
		dCILInstrArgument* const localVariable = new dCILInstrArgument(*m_currentClosure, arg.m_label, arg.GetType());
		m_currentClosure->InsertAfter(labelNode, localVariable->GetNode());
		TRACE_INSTRUCTION(localVariable);
	}

	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dCILInstrLabel* const label = new dCILInstrLabel(*m_currentClosure, m_currentClosure->m_returnLabel);
	for (dLuaClosure::dListNode* node = m_currentClosure->GetFirst(); node; node = node->GetNext()) {
		dCILInstrGoto* const gotoJump = node->GetInfo()->GetAsGoto();
		if (gotoJump) {
			if (gotoJump->GetLabel() == m_currentClosure->m_returnLabel) {
				gotoJump->SetTarget(label);
			}
		}
	}
	dCILInstrReturn* const ret = new dCILInstrReturn(*m_currentClosure, m_currentClosure->m_returnVariable, type);
	dCILInstrFunctionEnd* const functionEnd = new dCILInstrFunctionEnd (m_currentClosure->GetFirst()->GetInfo()->GetAsFunction());

	TRACE_INSTRUCTION(label);
	TRACE_INSTRUCTION(ret);
	TRACE_INSTRUCTION(functionEnd);
m_currentClosure->Trace();

static int xxx;
xxx++;
	dBasicBlocksGraph basicBlocks;
	basicBlocks.Build(m_currentClosure->GetFirst());
basicBlocks.Trace();

	FixUnitializedReturnVariable();
basicBlocks.Trace();

	basicBlocks.ConvertToSSA();
basicBlocks.Trace();

	basicBlocks.OptimizeSSA();
basicBlocks.Trace();

	basicBlocks.RegistersAllocations();
m_currentClosure->Trace();
//basicBlocks.Trace();
	m_currentClosure = m_currentClosure->m_parent;
}

void dNewtonLuaCompiler::FixUnitializedReturnVariable()
{
	dCIL::dListNode* retNode = NULL; 
	for (retNode = m_currentClosure->GetLast(); retNode && !retNode->GetInfo()->GetAsReturn(); retNode = retNode->GetPrev());
	for (dCIL::dListNode* node = retNode->GetPrev(); node; node = node->GetPrev()) {
		dCILInstr::dArg* const var = node->GetInfo()->GetGeneratedVariable();
		if (var && var->m_label == m_currentClosure->m_returnVariable) {
			return;
		}
	}

	dCILInstrReturn* const retInstruction = retNode->GetInfo()->GetAsReturn();
	dString temp (m_currentClosure->NewTemp());
	dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, retInstruction->GetArg0().m_label, retInstruction->GetArg0().GetType(), '0', dCILInstr::m_constInt);
	m_currentClosure->InsertAfter (retNode->GetPrev(), move->GetNode());
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitBlockBeginning()
{
	dString label(m_currentClosure->NewLabel());
	dCILInstrLabel* const blockBegin = new dCILInstrLabel(*m_currentClosure, label);
	TRACE_INSTRUCTION(blockBegin);
	return dUserVariable(blockBegin);
}

/*
dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitFunctionParameter(const dUserVariable& prevParameter, const dUserVariable& parameter)
{
	dCILInstrArgument* const localVariable = new dCILInstrArgument(*m_currentClosure, parameter.GetString(), dCILInstr::dArgType(dCILInstr::m_luaType));
	dAssert (0);

	dUserVariable variable;
	TRACE_INSTRUCTION(localVariable);
	return dUserVariable(localVariable);
}
*/

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitParametersToLocalVariables(const dUserVariable& parameterList)
{
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dAssert (0);
/*
	for (dCILInstr* instruction = parameterList.m_node->GetInfo(); instruction; instruction = instruction->GetPrevius()) {
		dCILInstrArgument* const argumnet = instruction->GetAsArgument();
		dAssert(argumnet);
		dString local(m_currentClosure->NewTemp());
		dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, local, type, argumnet->GetArg0().m_label, type);
		m_currentClosure->m_argumnets.Append(move);
		TRACE_INSTRUCTION(move);
	}
*/
	return parameterList;
}


dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitFunctionCall(const dUserVariable& functionName, const dUserVariable& argumentsList)
{
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dString result(m_currentClosure->NewTemp());
	dString name(functionName.GetString());
	for (dList<dString>::dListNode* node = functionName.m_tokenList.GetFirst()->GetNext(); node; node = node->GetNext()) {
		name += '.';
		name += node->GetInfo();
	}

	dCILInstrCall* const functionCall = new dCILInstrCall(*m_currentClosure, result, type, name);

	for (dList<dCIL::dListNode*>::dListNode* node = argumentsList.m_nodeList.GetFirst(); node; node = node->GetNext()) {
		dCILSingleArgInstr* const argInstruction = node->GetInfo()->GetInfo()->GetAsSingleArg();
		const dCILInstr::dArg& arg = argInstruction->GetArg0();
		functionCall->AddArgument(arg);
	}

	TRACE_INSTRUCTION(functionCall);
	return dUserVariable(functionCall);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitLoadConstant(const dUserVariable& constName)
{
	dString localVariableAliasName(m_currentClosure->NewTemp());
	dCILInstr::dIntrisicType constType = dCILInstr::m_luaType;
	dCILInstr::dIntrisicType variableType = dCILInstr::m_luaType;
	int token = constName.GetToken();
	switch (token)
	{
		case _INTEGER:
			variableType = dCILInstr::m_int;
			constType = dCILInstr::m_constInt;
			break;
		default:
			dAssert(0);
	}

	dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, localVariableAliasName, dCILInstr::dArgType(variableType), constName.GetString(), dCILInstr::dArgType (constType));
	TRACE_INSTRUCTION(move);
	return dUserVariable(move);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitLoadVariable(const dUserVariable& varName)
{
/*
	// search for the defintions 
	// note: lua does no requred this
	dCILInstrMove* definition = NULL;
	for (dList<dCILInstrMove*>::dListNode* node = m_currentClosure->m_argumnets.GetFirst(); node; node = node->GetNext()) {
		dCILInstrMove* const intruction = node->GetInfo()->GetAsMove();
		if (intruction) {
			const dCILInstr::dArg& argName = intruction->GetArg1();
			if (argName.m_label == varName.GetString()) {
				definition = intruction;
				break;
			}
		}
	}
	dAssert(definition);
*/
	dString outVarName(m_currentClosure->NewTemp());
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, outVarName, type, varName.GetString(), type);
	TRACE_INSTRUCTION(move);
	return dUserVariable(move);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitLoadString(const dUserVariable& varName)
{
	dString outVarName(m_currentClosure->NewTemp());
	dCILInstr::dArgType type(dCILInstr::m_constString);
	dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, outVarName, dCILInstr::dArgType(dCILInstr::m_string), varName.GetString(), type);
	TRACE_INSTRUCTION(move);
	return dUserVariable(move);
}


dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitLocalVariableDeclaration(const dUserVariable& varNameList)
{
	dUserVariable outVarName;
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	for (dList<dString>::dListNode* node = varNameList.m_tokenList.GetFirst(); node; node = node->GetNext()) {
		const dString& varName = node->GetInfo();
		dCILInstrLocal* const localVariable = new dCILInstrLocal(*m_currentClosure, varName, type);
		m_currentClosure->m_localVariables.Append(localVariable);
		outVarName.m_tokenList.Append(localVariable->GetArg0().m_label);
		TRACE_INSTRUCTION(localVariable);
	}
	return outVarName;
}


dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitBinaryExpression(const dUserVariable& arg0Variable, const dUserVariable& binaryOperator, const dUserVariable& arg1Variable)
{
	int token = binaryOperator.GetToken();
	dCILThreeArgInstr::dOperator operation = dCILThreeArgInstr::m_operatorsCount;
	switch (token)
	{
		case _IDENTICAL:
			operation = dCILThreeArgInstr::m_identical;
			break;
		case _DIFFERENT:
			operation = dCILThreeArgInstr::m_different;
			break;
		case _LEFT_EQUAL:
			operation = dCILThreeArgInstr::m_lessEqual;
			break;

		case '<':
			operation = dCILThreeArgInstr::m_less;
			break;

		case '-':
			operation = dCILThreeArgInstr::m_sub;
			break;

		case '+':
			operation = dCILThreeArgInstr::m_add;
			break;

		case '*':
			operation = dCILThreeArgInstr::m_mul;
			break;

		default:
			dAssert(0);
	}

	dAssert (arg0Variable.m_nodeList.GetCount() == 1);
	dAssert (arg1Variable.m_nodeList.GetCount() == 1);
	dList<dCIL::dListNode*>::dListNode* arg0Node = arg0Variable.m_nodeList.GetFirst();
	dList<dCIL::dListNode*>::dListNode* arg1Node = arg1Variable.m_nodeList.GetFirst();

	dCILSingleArgInstr* const arg0Instruction = arg0Node->GetInfo()->GetInfo()->GetAsSingleArg();
	dCILSingleArgInstr* const arg1Instruction = arg1Node->GetInfo()->GetInfo()->GetAsSingleArg();
	dAssert(arg0Instruction);
	dAssert(arg1Instruction);
	const dCILInstr::dArg& arg0 = arg0Instruction->GetArg0();
	const dCILInstr::dArg& arg1 = arg1Instruction->GetArg0();

	dString outVarName(m_currentClosure->NewTemp());

	// for now just make result make the result a lua type, later for better optimization add type promotion
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dCILInstrIntergerLogical* const instruction = new dCILInstrIntergerLogical(*m_currentClosure, operation, outVarName, type, arg0.m_label, arg0.GetType(), arg1.m_label, arg1.GetType());

	TRACE_INSTRUCTION(instruction);
	return dUserVariable(instruction);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitAssigmentStatement(const dUserVariable& nameList, const dUserVariable& expressionList)
{
	//dList<dCIL::dListNode*>::dListNode* nameListNode = nameList.m_nodeList.GetFirst();
	dList<dString>::dListNode* nameListNode = nameList.m_tokenList.GetFirst();
	dList<dCIL::dListNode*>::dListNode* expressionListNode = expressionList.m_nodeList.GetFirst();
	dAssert(nameList.m_tokenList.GetCount() >= 1);
	dAssert(expressionList.m_nodeList.GetCount() >= 1);
	int count = dMin (nameList.m_tokenList.GetCount(), expressionList.m_nodeList.GetCount());

	dCILInstrMove* returnMove = NULL;
	for (int i = 0; i < count; i ++) {
		//dCILSingleArgInstr* const dst = nameListNode->GetInfo(); 
		const dString& dstName = nameListNode->GetInfo();

/*
		if (dest->GetAsLocal()) {
			dCILInstrLocal* const dstIntruction = dest->GetAsLocal();
			const dCILInstr::dArg& argName = dstIntruction->GetArg0();
			for (dList<dCILInstrLocal*>::dListNode* node = m_currentClosure->m_localVariables.GetFirst(); node; node = node->GetNext()) {
				dCILInstrLocal* const intruction = node->GetInfo()->GetAsLocal();
				dAssert(intruction);
				if (argName.m_label == intruction->GetArg0().m_label) {
					destArg = argName;
					break;
				}
			}
		}
*/


		dCILSingleArgInstr* const src = expressionListNode->GetInfo()->GetInfo()->GetAsSingleArg(); 
		dAssert(src);
		//dAssert(dst);
		//const dCILInstr::dArg& dstArg = dst->GetArg0();
		const dCILInstr::dArg& srcArg = src->GetArg0();
		dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, dstName, srcArg.GetType(), srcArg.m_label, srcArg.GetType());
		TRACE_INSTRUCTION(move);

		if (!returnMove) {
			returnMove = move;
		}

		nameListNode = nameListNode->GetNext();
		expressionListNode = expressionListNode->GetNext();
	}
	return dUserVariable(returnMove);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitReturn(const dUserVariable& expression)
{
	dCILInstr::dArgType type(dCILInstr::m_luaType);
	dCILSingleArgInstr* const exp = expression.m_nodeList.GetFirst()->GetInfo()->GetInfo()->GetAsSingleArg();
	dCILInstrMove* const move = new dCILInstrMove(*m_currentClosure, m_currentClosure->m_returnVariable, type, exp->GetArg0().m_label, exp->GetArg0().GetType());
	dCILInstrGoto* const gotoJump = new dCILInstrGoto(*m_currentClosure, m_currentClosure->m_returnLabel);

	TRACE_INSTRUCTION(move);
	TRACE_INSTRUCTION(gotoJump);
	return dUserVariable(move);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitIf(const dUserVariable& expression, const dUserVariable& thenBlock, const dUserVariable& elseBlock)
{
	dCIL::dListNode* const expressionNode = expression.m_nodeList.GetFirst()->GetInfo();
	dCILSingleArgInstr* const expressionInstruction = expressionNode->GetInfo()->GetAsSingleArg();
	dAssert(expressionInstruction);
	const dCILInstr::dArg& expressionArg = expressionInstruction->GetArg0();

	dString label(m_currentClosure->NewLabel());
	dCILInstrLabel* const exitLabel = new dCILInstrLabel(*m_currentClosure, label);
	TRACE_INSTRUCTION(exitLabel);

	dCIL::dListNode* const thenBlockNode = thenBlock.m_nodeList.GetFirst()->GetInfo();
	dCILInstrLabel* const thenBlockInstruction = thenBlockNode->GetInfo()->GetAsLabel();
	if (elseBlock.m_nodeList.GetCount()) {
		// this is an: if exp then block else block end
		dCIL::dListNode* const elseBlockNode = elseBlock.m_nodeList.GetFirst()->GetInfo();
		dCILInstrLabel* const elseBlockInstruction = elseBlockNode->GetInfo()->GetAsLabel();
		dCILInstrGoto* const gotoJump = new dCILInstrGoto(*m_currentClosure, exitLabel);
		TRACE_INSTRUCTION(gotoJump);
		m_currentClosure->InsertAfter(elseBlockNode->GetPrev(), gotoJump->GetNode());

		dString zero("0");
		dCILInstr::dArgType zeroType (dCILInstr::m_constInt);
		dCILInstrConditional* const conditional = new dCILInstrConditional(*m_currentClosure, dCILInstr::m_different, expressionArg.m_label, expressionArg.GetType(), zero, zeroType, elseBlockInstruction, thenBlockInstruction);
		m_currentClosure->InsertAfter (expressionNode, conditional->GetNode());
		TRACE_INSTRUCTION(conditional);

		dCILInstrGoto* const gotoJump1 = new dCILInstrGoto(*m_currentClosure, exitLabel);
		m_currentClosure->InsertAfter(exitLabel->GetNode()->GetPrev(), gotoJump1->GetNode());
		TRACE_INSTRUCTION(gotoJump1);

		//m_currentClosure->Trace();
		return dUserVariable(conditional);
	} else {
		dString zero("0");
		dCILInstr::dArgType zeroType(dCILInstr::m_constInt);
		dCILInstrConditional* const conditional = new dCILInstrConditional(*m_currentClosure, dCILInstr::m_different, expressionArg.m_label, expressionArg.GetType(), zero, zeroType, exitLabel, thenBlockInstruction);
		m_currentClosure->InsertAfter(expressionNode, conditional->GetNode());
		TRACE_INSTRUCTION(conditional);

		dCILInstrGoto* const gotoJump1 = new dCILInstrGoto(*m_currentClosure, exitLabel);
		m_currentClosure->InsertAfter(exitLabel->GetNode()->GetPrev(), gotoJump1->GetNode());
		TRACE_INSTRUCTION(gotoJump1);

		return dUserVariable(conditional);
	}
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitLabel()
{
	dString label(m_currentClosure->NewLabel());
	dCILInstrGoto* const gotoJump = new dCILInstrGoto(*m_currentClosure, label);
	dCILInstrLabel* const labelInstruction = new dCILInstrLabel(*m_currentClosure, label);
	gotoJump->SetTarget(labelInstruction);

	TRACE_INSTRUCTION(gotoJump);
	TRACE_INSTRUCTION(labelInstruction);
	return dUserVariable(labelInstruction);
}

dNewtonLuaCompiler::dUserVariable dNewtonLuaCompiler::EmitFor(const dUserVariable& iterName, const dUserVariable& startLoopLabel, const dUserVariable& testExpression, const dUserVariable& stepExpression, const dUserVariable& block)
{
	dCILSingleArgInstr* const arg0Instruction = iterName.m_nodeList.GetFirst()->GetInfo()->GetInfo()->GetAsSingleArg();
	dCILSingleArgInstr* const testInstruction = testExpression.m_nodeList.GetFirst()->GetInfo()->GetInfo()->GetAsSingleArg();
	dCILInstrLabel* const blockStartInstruction = block.m_nodeList.GetFirst()->GetInfo()->GetInfo()->GetAsLabel();
	dAssert(testInstruction);
	dAssert(arg0Instruction);
	dAssert(blockStartInstruction);
	const dCILInstr::dArg& arg0 = arg0Instruction->GetArg0();
	const dCILInstr::dArg& testArg = testInstruction->GetArg0();

//m_currentClosure->GetFirst()->GetInfo()->GetCil()->Trace();
	dCILInstrIntergerLogical* const instruction = new dCILInstrIntergerLogical(*m_currentClosure, dCILInstr::m_add, arg0.m_label, arg0.GetType(), arg0.m_label, arg0.GetType(), dString("1"), dCILInstr::dArgType(dCILInstr::m_constInt));

	// make a for loop to a do loop
	dCIL::dListNode* const loopstartNode = startLoopLabel.m_nodeList.GetFirst()->GetInfo();
	for (dCIL::dListNode* node = loopstartNode->GetNext()->GetInfo()->GetNode(); node != blockStartInstruction->GetNode(); node = node->GetNext()) {
		dCILInstr* const loopInstruction = node->GetInfo();
		dCILInstr* const copyLoop = loopInstruction->Clone();
		TRACE_INSTRUCTION(copyLoop);
	}
	dString exitLoopLabel(m_currentClosure->NewLabel());
	dCILInstrLabel* const exitLabelInstruction = new dCILInstrLabel(*m_currentClosure, exitLoopLabel);
	dCILInstrConditional* const loopConditional = new dCILInstrConditional(*m_currentClosure, dCILInstr::m_less, arg0.m_label, arg0.GetType(), testArg.m_label, testArg.GetType(), blockStartInstruction, exitLabelInstruction);
	m_currentClosure->InsertAfter(exitLabelInstruction->GetNode()->GetPrev(), loopConditional->GetNode());

	dCILInstrConditional* const loopBeginConditional = new dCILInstrConditional(*m_currentClosure, dCILInstr::m_greatherEqual, arg0.m_label, arg0.GetType(), testArg.m_label, testArg.GetType(), exitLabelInstruction, blockStartInstruction);
	m_currentClosure->InsertAfter(testInstruction->GetNode(), loopBeginConditional->GetNode());

	TRACE_INSTRUCTION(loopBeginConditional);
	TRACE_INSTRUCTION(loopConditional);
	TRACE_INSTRUCTION(exitLabelInstruction);

//m_currentClosure->GetFirst()->GetInfo()->GetCil()->Trace();
	return dUserVariable();
}