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
//Auto generated Parser Generator class: dAssemblerCompiler.cpp
//

#include "dAssemblerCompiler.h"
#include "dAssemblerLexical.h"


#define D_CODE_SEGMNET_ALLOC_SHUNK_SIZE (1024 * 32)

dAssemblerCompiler::dAssemblerCompiler()
	:dAssemblerParser ()
//	,m_virtualMachine (NULL)
//	,m_globalSymbols ()
//	,m_codeSegmentSize(0)
//	,m_codeSegment (NULL)
//	,m_currentFunction(NULL)
{
	dAssert(0);
}

dAssemblerCompiler::~dAssemblerCompiler()
{
	dAssert(0);
//	if (m_codeSegment) {
//		delete[] m_codeSegment;
//	}
}

int dAssemblerCompiler::CompileSource (dVirtualMachine* const virtualMachine, const char* const source)
{
	dAssert(0);
/*
	dAssemblerLexical scanner (source);

	m_virtualMachine = virtualMachine;
	Parse(scanner);
*/
	return 0;
}

#if 0
bool dAssemblerCompiler::Parse(dAssemblerLexical& scanner)
{
	dAssert(0);
/*
	m_codeSegmentSize = 0;
	m_codeSegment = new short[D_CODE_SEGMNET_ALLOC_SHUNK_SIZE];
	dAssemblerParser::Parse(scanner);

//	scanner.ReStartScanner();
//	dAssemblerParser::Parse(scanner);
*/
	return true;
}

void dAssemblerCompiler::EmitByteCode (int count, const dVirtualMachine::dOpCode* const code)
{
	dAssert (0);
/*
	if ((m_codeSegmentSize + count) > D_CODE_SEGMNET_ALLOC_SHUNK_SIZE) {
		// realloc the code segment
		_ASSERTE (0);
	}

	for (int i = 0; i < count; i++) {
		m_codeSegment[m_codeSegmentSize] = code[i].m_bytecode;
		m_codeSegmentSize ++;
	}
*/
}


dAssemblerCompiler::dUserVariable dAssemblerCompiler::EmitDataType (const dUserVariable& dataType) const
{
	return dataType;
}

dAssemblerCompiler::dUserVariable dAssemblerCompiler::EmitSymbol (const dUserVariable& symbol) const
{
	return symbol;
}



void dAssemblerCompiler::EmitUnInitilizedDataDeclaration (const dUserVariable& type, const dUserVariable& id) const
{

}

void dAssemblerCompiler::EmitInitilizedDataDeclaration (const dUserVariable& type, const dUserVariable& id, const dUserVariable& initialValue) const
{

}



void dAssemblerCompiler::EmitLocalLabel (const dUserVariable& symbol) const
{
	dAssert (m_currentFunction);
	if (m_currentFunction->m_localSymbols.Find(symbol.m_data)) {
		// ERROR local symbol duplication
	}

	dLocalSymbol localSymbol;
	localSymbol.m_symbolLocationInByte = m_codeSegmentSize;
	localSymbol.m_type = localJumpLabel;
	dString label (symbol.m_data);
	label.Replace(label.Size()-1, 1, "");
	m_currentFunction->m_localSymbols.Insert(localSymbol, label);
}



dAssemblerCompiler::dUserVariable dAssemblerCompiler::TypeCheckRegister (const dUserVariable& symbol)
{
	dAssert (0);
	return dUserVariable(symbol);
/*
	dAssemblerCompiler::dUserVariable reg (symbol);

	int registerIndex = atoi (symbol.m_data.c_str() + 1);
	if ((registerIndex < 0) || (registerIndex >= D_REGISTER_COUNT)) {
		// error unknowns Register
		_ASSERTE (0);
	}

	reg.m_semanticValue = registerIndex;
	return reg;
*/
}



dAssemblerCompiler::dUserVariable dAssemblerCompiler::EmitIntegerConst (const dUserVariable& integerConst) const
{
	dAssemblerCompiler::dUserVariable constantValue (integerConst);
	constantValue.m_semanticValue = atoi (integerConst.m_data.GetStr());
	return constantValue;
}



void dAssemblerCompiler::EmitPushAndPop (const dUserVariable& instruction, const dUserVariable& registerMask)
{
	dAssert (0);
/*
	// check that the register mask is no making reference to non existing registers;
	int mask = registerMask.m_semanticValue;
	if (instruction.m_semanticValue == dVirtualMachine::push) {
		int reg = m_virtualMachine->GetRegisterCount() - 1;
		do {
			if (mask & (1<<31)) {
				dUserVariable register1;
				register1.m_semanticValue = reg; 
				EmitInstructionType2 (instruction, register1);
			}
			reg --;
			mask <<= 1;
		} while (mask);
	} else {
		int reg = 0;
		do {
			if (mask & 1) {
				dUserVariable register1;
				register1.m_semanticValue = reg; 
				EmitInstructionType2 (instruction, register1);
			}
			reg ++;
			mask >>= 1;
		} while (mask);
	}
*/
}


void dAssemblerCompiler::EmitInstructionType1_saveLocalAdress (const dUserVariable& instruction, const dUserVariable& symbol)
{
	dAssert (0);
}

void dAssemblerCompiler::EmitInstructionType4_saveLocalAdress (const dUserVariable& instruction, const dUserVariable& reg0, const dUserVariable& reg1, const dUserVariable& symbol)
{
	dAssert (0);
	dReference& reference = m_currentFunction->m_localReferences.Append()->GetInfo();

	reference.m_location = m_codeSegmentSize + 1;
	reference.m_symbol = symbol.m_data;

	EmitInstructionType4 (instruction, reg0, reg1, symbol);
}

void dAssemblerCompiler::EmitInstructionType1_saveGlobalAdress (const dUserVariable& instruction, const dUserVariable& symbol)
{
	dReference& reference = m_globalReferences.Append()->GetInfo();
	reference.m_location = m_codeSegmentSize + 1;
	reference.m_symbol = symbol.m_data;

	EmitInstructionType1 (instruction, symbol);
}

void dAssemblerCompiler::EmitInstructionType0 (const dUserVariable& instruction)
{
	dAssert (0);
/*
	dVirtualMachine::dOpCode bytecode[1];

	bytecode[0].m_opcode = instruction.m_semanticValue;
	bytecode[0].m_reg0 = 0;
	bytecode[0].m_reg1 = 0;
	EmitByteCode (sizeof (bytecode) / sizeof (bytecode[0]), bytecode);
*/
}


void dAssemblerCompiler::EmitInstructionType1 (const dUserVariable& instruction, const dUserVariable& immediate)
{
	dAssert (0);
	dVirtualMachine::dOpCode bytecode[3];

	bytecode[0].m_opcode = instruction.m_semanticValue;
	bytecode[0].m_reg0 = 0;
	bytecode[0].m_reg1 = 0;

	bytecode[1].m_bytecode = immediate.m_semanticValue & 0xffff;
	bytecode[2].m_bytecode = immediate.m_semanticValue >> 16;

	EmitByteCode (sizeof (bytecode) / sizeof (bytecode[0]), bytecode);
}


void dAssemblerCompiler::EmitInstructionType2 (const dUserVariable& instruction, const dUserVariable& reg)
{
	dAssert (0);
/*
	dVirtualMachine::dOpCode bytecode[1];
	bytecode[0].m_opcode = instruction.m_semanticValue;
	bytecode[0].m_reg0 = reg.m_semanticValue;
	bytecode[0].m_reg1 = 0;

	if (reg.m_semanticValue >= m_virtualMachine->GetRegisterCount()) {
		// error using illegal register
	}

	EmitByteCode (sizeof (bytecode) / sizeof (bytecode[0]), bytecode);
*/
}

void dAssemblerCompiler::EmitInstructionType3 (const dUserVariable& instruction, const dUserVariable& dst, const dUserVariable& src)
{
	dAssert (0);
/*
	dVirtualMachine::dOpCode bytecode[1];
	bytecode[0].m_opcode = instruction.m_semanticValue;
	bytecode[0].m_reg0 = dst.m_semanticValue;
	bytecode[0].m_reg1 = src.m_semanticValue;

	if (dst.m_semanticValue >= m_virtualMachine->GetRegisterCount()) {
		// error using illegal register
	}
	if (src.m_semanticValue >= m_virtualMachine->GetRegisterCount()) {
		// error using illegal register
	}

	EmitByteCode (sizeof (bytecode) / sizeof (bytecode[0]), bytecode);
*/
}

void dAssemblerCompiler::EmitInstructionType4 (const dUserVariable& instruction, const dUserVariable& dst, const dUserVariable& src, const dUserVariable& immediate)
{
	dAssert (0);
/*
	dVirtualMachine::dOpCode bytecode[3];
	bytecode[0].m_opcode = instruction.m_semanticValue;
	bytecode[0].m_reg0 = dst.m_semanticValue;
	bytecode[0].m_reg1 = src.m_semanticValue;

	if (dst.m_semanticValue >= m_virtualMachine->GetRegisterCount()) {
		// error using illegal register
	}
	if (src.m_semanticValue >= m_virtualMachine->GetRegisterCount()) {
		// error using illegal register
	}

	bytecode[1].m_bytecode = immediate.m_semanticValue & 0xffff;
	bytecode[2].m_bytecode = immediate.m_semanticValue >> 16;
	EmitByteCode (sizeof (bytecode) / sizeof (bytecode[0]), bytecode);
*/
}


void dAssemblerCompiler::EmitBeginFunction (const dUserVariable& name, const dUserVariable& functionScope)
{
	dAssert (0);
	if (m_globalSymbols.Find(name.m_data)) {
		// WARNING FUNTION DUPLICATION;
		dAssert (0);
	} 

	// add new symbol to the global symbol table; 
	dGlobalSymbol symbol;
	symbol.m_isPublic = (functionScope.m_data == "private") ? false : true;
	symbol.m_type = functionName;
	symbol.m_symbolLocationInByte = m_codeSegmentSize;

	dSymbolTable::dTreeNode* const symbolNode = m_globalSymbols.Insert (symbol, name.m_data);
	m_currentFunction = &symbolNode->GetInfo();
}


void dAssemblerCompiler::EmitEndFunction ()
{
	dAssert (0);
	dAssert (m_currentFunction);
	for (dList<dReference>::dListNode* node = m_currentFunction->m_localReferences.GetFirst(); node; node = node->GetNext()) {
		dAssert (0);
	}
}
#endif