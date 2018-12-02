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


#include "dVirtualMachine.h"


dVirtualMachine::dNemonic dVirtualMachine::m_nemonics[] = 
{
	{ m_nop, "nop" },
	{ m_enter, "enter" },
	{ m_leave, "leave" },

/*
	{mov,		"mov"},

	{addi,		"addi"},		
	{add,		"add"},
	{sub,		"sub"},		
	{mul,		"mul"},
	{div,		"div"}, 
	{abs,		"abs"},
	{neg,		"neg"},

	{and,		"and"},		
	{or,		"or"},		
	{xor,		"xor"},
	{not,		"xor"},

	{sll,		"sll"},		
	{srl,		"srl"},		

	{loadb,		"loadb"},		
	{loadw,		"loadw"},		
	{loadd,		"loadd"},		

	{storeb,	"storeb"},	
	{storew,	"storew"},	
	{stored,	"stored"},	

	{beq,		"beq"},		
	{bne,		"bne"},		
	{blt,		"blt"},		
	{ble,		"ble"},		
	{bgt,		"bgt"},		
	{bget,		"bget"},		

	{syscall,	"syscall"},		
	{jump,		"jump"},		

	{push,		"push"},		 
	{pop,		"pop"},		
*/
	
};

dVirtualMachine::dVirtualMachine(void)
	:m_functionTable()
	,m_codeSegment(NULL)
	,m_codeSegmentSize(0)
	,m_programCounter(0)
{	
//	m_codeSegment = new dOpCode[m_codeMaxSize];
	dAssert(m_count <= (1 << D_OPCODE_FIELD));
	memset (m_integerRegisters, 0, sizeof (m_integerRegisters));
	//memset (m_integerRegisters, 0, sizeof (m_integerRegisters));
}

dVirtualMachine::~dVirtualMachine(void)
{
	if (m_codeSegment) {
		delete[] m_codeSegment;
	}
}

dVirtualMachine::dOpCode* dVirtualMachine::AllocCodeSegement(int sizeInWords)
{
/*
	while ((m_codeSegmentSize + sizeInWords) > m_codeMaxSize) {
		m_codeMaxSize *= 2;
		dOpCode* const codeSegment = new dOpCode[m_codeMaxSize];
		memcpy(codeSegment, m_codeSegment, m_codeSegmentSize * sizeof (dOpCode));
		delete[] m_codeSegment;
		m_codeSegment = codeSegment;
	}
*/	
	if (m_codeSegment) {
		delete[] m_codeSegment;
	}
	m_codeSegmentSize = sizeInWords;
	m_codeSegment = new dOpCode[sizeInWords];
	return m_codeSegment;
}

void dVirtualMachine::AddFunction (const dString& name, int byteCodeOffset, dFunctionDescription::dReturnType type)
{
	dFunctionDescription desc;
	desc.m_entryPoint = byteCodeOffset;
	desc.m_returnType = type;
	m_functionTable.Insert (desc, name);
}



bool dVirtualMachine::ExecuteFunction(const dString& name, const char* const paramFormat, ...)
{
	dFunctionTable::dTreeNode* const node = m_functionTable.Find(name);
	if (node) {
		va_list v_args;
		va_start(v_args, paramFormat);
		bool ret = ExecuteFunction (node->GetInfo().m_entryPoint, paramFormat, v_args);
		va_end(v_args);
		return ret;
	}

	return false;
}


bool dVirtualMachine::ExecuteFunction(int entryPoint, const char* const paramFormat, ...)
{
	va_list v_args;
	va_start(v_args, paramFormat);
	bool ret = ExecuteFunction (entryPoint, paramFormat, v_args);
	va_end(v_args);
	return ret;
}


bool dVirtualMachine::ExecuteFunction (int entryPoint, const char* const paramFormat, va_list argList)
{
	paramFormat;
//	unsigned int i = 0;
	for (int i = 0; i < 1; i++)
	{
		int arg = va_arg (argList, int);
		m_integerRegisters[1] = arg;
	}

	m_integerRegisters[D_STACK_REGISTER_INDEX] = 1;
	return Run (entryPoint);
}


bool dVirtualMachine::Run (int entryPoint)
{
	m_programCounter = entryPoint;

int stackMem[100] ;

	while (m_integerRegisters[D_STACK_REGISTER_INDEX] > 0) {
		dOpCode code (m_codeSegment[m_programCounter]);
		m_programCounter ++;
		dInstruction instruction = dInstruction (code.m_type0.m_opcode);
		switch (instruction) 
		{
			//enter	imm1, imm2					imm1 = registerMask, imm2 local variable stack size 
			case m_enter:
			{
				dAssert (0);
/*
				int stack = m_integerRegisters[D_STACK_REGISTER_INDEX];
				int saveRegIndex = D_CALLER_SAVE_REGISTER_COUNT;
				for (unsigned mask = code.m_type1.m_imm1; mask; mask = mask >>= 1) {
					if (mask & 1) {
						stackMem[stack] = m_integerRegisters[saveRegIndex];
						stack ++;
					}
					saveRegIndex ++;
				}
				if (code.m_type1.m_imm2) {
					dAssert (0);
				}

				m_integerRegisters[D_STACK_REGISTER_INDEX] = stack;
*/
				break;
			}

			//leave imm1, imm2			imm1 = registerMask, imm2 local variable stack size 
			case m_leave:
			{
				dAssert(0);
/*
				int stack = m_integerRegisters[D_STACK_REGISTER_INDEX];
				int saveRegIndex = D_INTEGER_REGISTER_COUNT;

				if (code.m_type1.m_imm2) {
					dAssert(0);
				}

				for (unsigned mask = (code.m_type1.m_imm1 << D_CALLER_SAVE_REGISTER_COUNT); mask; mask = mask <<= 1) {
					saveRegIndex --;
					if (mask & (1<<(D_INTEGER_REGISTER_COUNT - 1))) {
						stack --;
						m_integerRegisters[saveRegIndex] = stackMem[stack];
					}
				}

				m_integerRegisters[D_STACK_REGISTER_INDEX] = stack;
*/
				break;
			}

			//call		Ri, imm2					R(stack) -= sizeof (dOpCode); [R(sizeof (dOpCode))] = pc; pc == pc + imm2
			case m_calli:
			{
				stackMem[m_integerRegisters[D_STACK_REGISTER_INDEX]] = m_programCounter;
				m_integerRegisters[D_STACK_REGISTER_INDEX] ++;
				m_programCounter += code.m_type2.m_imm2;
				break;
			}

			//ret		R1							R(stack) += sizeof (dOpCode); pc = [R(stack)]; 
			case m_ret:
			{
				m_integerRegisters[D_STACK_REGISTER_INDEX] -= 1;
				m_programCounter = stackMem[m_integerRegisters[D_STACK_REGISTER_INDEX]];
				break;
			}

			//mov Ri, Rj						R(i) = R(j)
			case m_mov:
			{
				m_integerRegisters[code.m_type3.m_reg0] = m_integerRegisters[code.m_type3.m_reg1];
				break;
			}

			//movi Ri, m_imm1						R(i) = m_imm1
			case m_movi:
			{
				unsigned temp = code.m_type2.m_imm2;
				m_integerRegisters[code.m_type2.m_reg0] = temp;
				break;
			}


			//m_cmpeqi	Ri, Rj, imm3				R(i) = R(j) == imm3
			case m_cmpeqi:
			{
				 unsigned temp = code.m_type3.m_imm3;
				 m_integerRegisters[code.m_type3.m_reg0] = (m_integerRegisters[code.m_type3.m_reg1] == temp) ? ~0 : 0;
				 break;
			}

			//bneq		Ri, imm2					if (R(i) != 0) PC = PC + imm2
			case m_bneq:
			{
				if (!m_integerRegisters[code.m_type2.m_reg0]) {
					int offset = code.m_type2.m_imm2;
					m_programCounter += offset;
				}
				break;
			}

			//addi 		Ri, Rj, imm2				R(i) = R(j) + imm3
			case m_addi:				
			{
			   int tmp = code.m_type3.m_imm3;
			   m_integerRegisters[code.m_type3.m_reg0] = unsigned (int (m_integerRegisters[code.m_type3.m_reg1]) + tmp);
			   break;
			}

			//add  		Ri, Rj, Rk					R(i) = R(j) + R(k)
			case m_add:
			{
				m_integerRegisters[code.m_type4.m_reg0] = m_integerRegisters[code.m_type4.m_reg1] + m_integerRegisters[code.m_type4.m_reg2];
				break;
			}

			case m_nop:
			default:
				dAssert (0);
		}
	}
	return true;
}