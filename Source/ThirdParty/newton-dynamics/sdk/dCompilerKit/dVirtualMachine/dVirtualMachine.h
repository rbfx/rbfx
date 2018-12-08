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


#ifndef __dVirtualMachine_h__
#define __dVirtualMachine_h__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dContainersStdAfx.h>
#include <dTree.h>
#include <dString.h>


#ifdef _MSC_VER
#pragma warning (disable: 4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif


#define	D_VIRTUAL_MACHINE_NAME			"nvm"
#define	D_VIRTUAL_MACHINE_DESCRIPTION	"Newton Virtual Machine, 32 litle indian, 32 int registers, 32 double precision floating point registers"


#define D_OPCODE_FIELD					7
#define D_REGISTER_SET_FIELD			4

#define D_REGISTER_SYMBOL				"%r"
#define D_SPILL_REGISTER_SYMBOL			"spilled"

#define D_INTEGER_REGISTER_COUNT		(1<<D_REGISTER_SET_FIELD)
#define D_STACK_REGISTER_INDEX			(D_INTEGER_REGISTER_COUNT - 1)
#define D_FLOAT_REGISTER_COUNT			D_INTEGER_REGISTER_COUNT		


#define D_RETURN_REGISTER_INDEX			(0)
#define D_PARMETER_IN_REGISTER_START	(0)
#define D_PARMETER_IN_REGISTER_END		((D_INTEGER_REGISTER_COUNT - 1) >> 1)
#define D_PARMETER_IN_REGISTER_COUNT	(D_PARMETER_IN_REGISTER_END - D_PARMETER_IN_REGISTER_START)
#define D_CALLEE_SAVE_REGISTER_START	(D_PARMETER_IN_REGISTER_END + 1)


typedef unsigned dMachineIntRegister;
typedef double dMachineFloatRegister;

class dVirtualMachine
{
	public:
	enum dInstruction
	{
		m_nop,				// do nothing 
		m_enter,			//enter		imm1, imm2					imm1 = registerMask, imm2 local variable stack size 
		m_leave,			//exit		imm1, imm2					imm1 = registerMask, imm2 local variable stack size 

		m_mov,				//mov		Ri, Rj						R(i) = R(j)
		m_movi,				//mov		Ri, m_imm1					R(i) = m_imm1

		m_cmpeq,			//m_cmpeq	Ri, Rj, Rk					R(i) = R(j) == R(j)
		m_cmpeqi,			//m_cmpeqi	Ri, Rj, imm3				R(i) = R(j) == imm3

		m_beq,				//beq		Ri, imm2					if (R(i) == 0) PC = PC + imm2
		m_bneq,				//bneq		Ri, imm2					if (R(i) != 0) PC = PC + imm2

		m_addi,				//addi 		Ri, Rj, imm2				R(i) = R(j) + imm3
		m_add,				//add  		Ri, Rj, Rk					R(i) = R(j) + R(k)
		m_sub,				//sub  		Ri, Rj, Rk					R(i) = R(j) - R(k)

		m_calli,			//call		Ri, imm2					R(stack) -= sizeof (dOpCode); [R(sizeof (dOpCode))] = pc; pc == pc + imm2
		m_ret,				//ret		R1							pc = [R(stack)]; R(stack) += sizeof (dOpCode)
		/*
		
		bne,			//bneq		Ri, Rj, imm3				if (R(i) != R(j)) PC = PC + imm3
		ble,			//ble		Ri, Rj, imm3 				if (R(i) <= R(j)) PC = PC + imm3  (signed comparison)
		blt,			//blt		Ri, Rj, imm3  				if (R(i) <  R(j)) PC = PC + imm3  (signed comparison)
		bgt,			//bgt		Ri, Rj, imm3  				if (R(i) >  R(j)) PC = PC + imm3  (signed comparison)
		bge,			//bge		Ri, Rj, imm3  				if (R(i) >= R(j)) PC = PC + imm3  (signed comparison)
		jump,			//jump		imm1						pc += imm1
		
		
		
		muli,			//muli 		Ri, Rj, imm2				R(i) = R(j) + imm3
		divi,			//divi 		Ri, Rj, imm2				R(i) = R(j) + imm3
		eq,				//le		Ri, Rj, Rk,   				R(i) = (R(j) <= R(k)) ? -1, 0
		le,				//le		Ri, Rj, Rk,   				R(i) = (R(j) <= R(k)) ? -1, 0
		lt,				//le		Ri, Rj, Rk,   				R(i) = (R(j) <  R(k)) ? -1, 0
		gt,				//ge		Ri, Rj, Rk,   				R(i) = (R(j) <  R(k)) ? -1, 0
		push,			//push		Ri, Rj						R(j) -= 4; [R(j)] = Ri;
		pop,			//pop		Ri, Rj						Ri = [R(j)]; R(j) += 4;
		
		
		loadBase,		//loadbase	Ri, Rj, imm1				R(i) = dword at memory [R(j) + imm1]
		storeBase,		//storebase	Ri, Rj, imm1				dword at memory [R(j) + imm1] = R(i)
		loadw,			//loadw		Ri, [Rj + Rk * 4 + imm4]	R(i) = dword at memory [R(j) + R(k) * 4 + imm4]
		storew,			//storew	Ri, [Rj + Rk * 4 + imm4]	dword at memory [R(j) + R(k) * 4 + imm4] = Ri

		// type2 opcode
		callr,		// callr	Ri					rn -= 4; [rn] = pc + 4; pc == ri
		jumpr,		// jump		Ri					pc = ri
		mul,		// mul  	Ri, Rj				R(i) = lowpart (R(i) * R(j)) ; R(j) = upperpart (R(i) * R(j))
		div,		// div  	Ri, Rj				R(i) = num (R(i) / R(j)) ; R(j) = recidual (R(i) / R(j))
		abs,		// abs		Ri, Rj				R(i) = abs (R(j))
		neg,		// neg		Ri, Rj				R(i) = - R(j)
		and,		// and  	Ri, Rj				R(i) = R(i) & R(j)
		or,			// or		Ri, Rj				R(i) = R(j) | R(j)
		xor,		// xor		Ri, Rj				R(i) = R(j) ^ R(j)
		not,		// xor		Ri, Rj				R(i) = ~R(j)
		sll,		// sll		Ri, Rj				R(i) = R(i) << R(j)
		srl,		// srl		Ri, Rj				R(i) = R(i) >> R(j)
		loadb,		// loadb	Ri, imm32[Rj]		R(i) = byte at memory [R(j) + imm32]
		loadd,		// loadd	Ri, imm32[Rj]		R(i) = dword at memory [R(j) + imm32]
		storeb,		// storeb	Ri, imm32[Rj]		byte at memory [R(j) + imm32] = R(i)
		stored,		// stored	Ri, imm32[Rj]		dword at memory [R(j) + imm32] = R(i)
		syscall,	// syscall	Ri, Rj, imm32		execute a system call
		*/
		m_count
	};

	struct dNemonic
	{
		dInstruction m_instruction;
		char* m_name;
	};

	union dOpCode
	{
		unsigned m_bytecode;

		struct 
		{
			unsigned	m_opcode	: D_OPCODE_FIELD;
			int			m_imm1		: 32 - D_OPCODE_FIELD;
		} m_type0;
/*
		struct 
		{
			unsigned	m_opcode	: D_OPCODE_FIELD;
			unsigned	m_imm1		: D_CALLER_SAVE_REGISTER_COUNT;
			unsigned	m_imm2		: 32 - (D_OPCODE_FIELD + D_CALLER_SAVE_REGISTER_COUNT);
		} m_type1;
*/
		struct 
		{
			unsigned	m_opcode	: D_OPCODE_FIELD;
			unsigned	m_reg0		: D_REGISTER_SET_FIELD;
			int			m_imm2		: 32 - (D_OPCODE_FIELD + D_REGISTER_SET_FIELD);
		} m_type2;

		struct 
		{
			unsigned	m_opcode	: D_OPCODE_FIELD;
			unsigned	m_reg0		: D_REGISTER_SET_FIELD;
			unsigned	m_reg1		: D_REGISTER_SET_FIELD;
			int			m_imm3		: 32 - (D_OPCODE_FIELD + D_REGISTER_SET_FIELD + D_REGISTER_SET_FIELD);
		} m_type3;

		struct 
		{
			unsigned	m_opcode	:  D_OPCODE_FIELD;
			unsigned	m_reg0		:  D_REGISTER_SET_FIELD;
			unsigned	m_reg1		:  D_REGISTER_SET_FIELD;
			unsigned	m_reg2		:  D_REGISTER_SET_FIELD;
			unsigned	m_imm4		:  (32 - D_OPCODE_FIELD - D_REGISTER_SET_FIELD - D_REGISTER_SET_FIELD - D_REGISTER_SET_FIELD);
		} m_type4;
	};

	class dFunctionDescription
	{
		public:
		enum dReturnType
		{
			m_void,
			m_intReg,
			m_floatReg
		};

		int m_entryPoint;
		dReturnType m_returnType;
	};

	class dFunctionTable: public dTree<dFunctionDescription, dString>
	{
		public: 
		dFunctionTable()
			:dTree<dFunctionDescription, dString>()
		{
		}
	};

	dVirtualMachine(void);
	virtual ~dVirtualMachine(void);

	dOpCode* AllocCodeSegement(int sizeInWords);
	void AddFunction (const dString& name, int byteCodeOffset, dFunctionDescription::dReturnType type);

	bool ExecuteFunction (int entryPoint, const char* const paramFormat, ...);
	bool ExecuteFunction (const dString& name, const char* const paramFormat, ...);

	protected:
	bool ExecuteFunction (int entryPoint, const char* const paramFormat, va_list argptr);

	bool Run (int entryPoint);

	dFunctionTable m_functionTable;
	dMachineIntRegister m_integerRegisters[D_INTEGER_REGISTER_COUNT];
	dMachineFloatRegister m_floatRegisters[D_FLOAT_REGISTER_COUNT];
	dOpCode* m_codeSegment;
	int m_codeSegmentSize;
	int m_programCounter;

	static dNemonic m_nemonics[]; 
};


#endif