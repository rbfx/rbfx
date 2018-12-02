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


#include "dLSCstdafx.h"
#include "dScriptClass.h"


dScriptClass::dScriptClass(void)
	:m_codeSgementSize(0)
	,m_codeSegment(NULL)

{
}

dScriptClass::~dScriptClass(void)
{
	if (m_codeSegment) {
		delete[] m_codeSegment;
	}
}

void dScriptClass::AddCode (dDAGClassNode* const classSymbols, dCIL& classCode)
{
	dAssert (0);
/*
	int size = classCode.GetCount() * 2 + 256;

	int paramIndex = 0;	
	int stackOffset = 0;	
	
	dTree<int, dString> parameters; 
	dTree<dString, int> jumpsOrigin; 
	dTree<int, dString> jumpTargets; 
	dTree<dString, int> functionCall; 
	
	int count = 0;
	dVirtualMachine::dOpCode* const tmp = new dVirtualMachine::dOpCode[size];
	memset (tmp, 0, size * sizeof (dVirtualMachine::dOpCode));

	for (dCIL::dListNode* node = classCode.GetFirst(); node; node = node->GetNext()) {
		dCILInstr& stmt = node->GetInfo();
		switch (stmt.m_instruction) 
		{
			case dCILInstr::m_function:	
			{
				m_symbolTable.Insert(count, (char*)stmt.m_arg0.m_label.GetStr());
				break;
			}

			case dCILInstr::m_enter:	
			{
				tmp[count].m_opcode = dVirtualMachine::enter;
				tmp[count].m_imm1 = stmt.m_extraInformation;
				for (int i = stmt.m_extraInformation; i; i = i >> 1) {
					stackOffset += (i & 1) * sizeof (unsigned);
				}
				count ++;
				break;
			}

			case dCILInstr::m_leave:	
			{
				tmp[count].m_opcode = dVirtualMachine::leave;
				tmp[count].m_imm1 = stmt.m_extraInformation;
				count ++;
				break;
			}


			case dCILInstr::m_argument:	
			{
				parameters.Insert(paramIndex * sizeof (unsigned) + stackOffset + sizeof (unsigned), stmt.m_arg0.m_label.GetStr());
				paramIndex ++;
				break;
			}

			case dCILInstr::m_loadBase:	
			{
				tmp[count].m_opcode = dVirtualMachine::loadBase;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = D_STACK_REGISTER_INDEX;
				tmp[count].m_imm3 = parameters.Find(stmt.m_arg2.m_label.GetStr())->GetInfo();
				count ++;
				break;
			}

			case dCILInstr::m_storeBase:	
			{
				tmp[count].m_opcode = dVirtualMachine::storeBase;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = D_STACK_REGISTER_INDEX;
				tmp[count].m_imm3 = parameters.Find(stmt.m_arg2.m_label.GetStr())->GetInfo();
				count ++;
				break;
			}


			case dCILInstr::m_label:	
			{
				jumpTargets.Insert(count, stmt.m_arg0.m_label.GetStr());
				break;
			}

			case dCILInstr::m_goto:	
			{
				jumpsOrigin.Insert(stmt.m_arg0.m_label.GetStr(), count);
				tmp[count].m_opcode = dVirtualMachine::jump;
				tmp[count].m_imm3 = 0;
				count ++;
				break;
			}

			case dCILInstr::m_load:	
			{
				tmp[count].m_opcode = dVirtualMachine::loadw;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = RegisterToIndex (stmt.m_arg1.m_label.GetStr());
				tmp[count].m_reg2 = RegisterToIndex (stmt.m_arg2.m_label.GetStr());
				tmp[count].m_imm4 = 0;
				count ++;
				break;
			}

			case dCILInstr::m_store:	
			{
				tmp[count].m_opcode = dVirtualMachine::storew;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = RegisterToIndex (stmt.m_arg1.m_label.GetStr());
				tmp[count].m_reg2 = RegisterToIndex (stmt.m_arg2.m_label.GetStr());
				tmp[count].m_imm4 = 0;
				count ++;
				break;
			}

			case dCILInstr::m_if:	
			{
				jumpsOrigin.Insert(stmt.m_arg2.m_label.GetStr(), count);
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = RegisterToIndex (stmt.m_arg1.m_label.GetStr());
				tmp[count].m_imm3 = 0;
				switch (stmt.m_operator)
				{
					case dCILInstr::m_different:
						tmp[count].m_opcode = dVirtualMachine::bne;
						break;

					case dCILInstr::m_identical:
						tmp[count].m_opcode = dVirtualMachine::beq;
						break;

					case dCILInstr::m_less:
						tmp[count].m_opcode = dVirtualMachine::blt;
						break;

					case dCILInstr::m_greather:
						tmp[count].m_opcode = dVirtualMachine::bgt;
						break;
				
					case dCILInstr::m_lessEqual:
						tmp[count].m_opcode = dVirtualMachine::ble;
						break;

					case dCILInstr::m_greatherEqual:
						tmp[count].m_opcode = dVirtualMachine::bge;
						break;


					default:
						dAssert (0);
				}
				count ++;
				break;
			}

			case dCILInstr::m_assigment:	
			{
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				if (stmt.m_operator == dCILInstr::m_nothing) {
					if (stmt.m_arg1.m_type == dCILInstr::m_intConst) {
						tmp[count].m_opcode = dVirtualMachine::movi;
						dString value (stmt.m_arg1.m_label.GetStr());
						tmp[count].m_imm3 = value.ToInteger();
					} else {
						tmp[count].m_opcode = dVirtualMachine::mov;
						tmp[count].m_reg1 = RegisterToIndex (stmt.m_arg1.m_label.GetStr());;
					}
				} else {
					tmp[count].m_reg1 = RegisterToIndex (stmt.m_arg1.m_label.GetStr());
					if (stmt.m_arg2.m_type == dCILInstr::m_intConst) {
						dString value (stmt.m_arg2.m_label.GetStr());
						switch (stmt.m_operator) 
						{
							case dCILInstr::m_add:
							{
								tmp[count].m_opcode = dVirtualMachine::addi;
								tmp[count].m_imm3 = value.ToInteger();
								break;
							}

							case dCILInstr::m_sub:
							{
								tmp[count].m_opcode = dVirtualMachine::addi;
								tmp[count].m_imm3 = -value.ToInteger();
								break;
							}

							case dCILInstr::m_mul:
							{
								tmp[count].m_opcode = dVirtualMachine::muli;
								tmp[count].m_imm3 = -value.ToInteger();
								break;
							}

							case dCILInstr::m_div:
							{
								tmp[count].m_opcode = dVirtualMachine::divi;
								tmp[count].m_imm3 = -value.ToInteger();
								break;
							}

							default:
								dAssert (0);
						}
					} else {
						tmp[count].m_reg2 = RegisterToIndex (stmt.m_arg2.m_label.GetStr());
						switch (stmt.m_operator) 
						{
							case dCILInstr::m_identical:
							{
								tmp[count].m_opcode = dVirtualMachine::eq;
								break;
							}

							case dCILInstr::m_lessEqual:
							{
								tmp[count].m_opcode = dVirtualMachine::le;
								break;
							}

							case dCILInstr::m_less:
							{
								tmp[count].m_opcode = dVirtualMachine::lt;
								break;
							}
							case dCILInstr::m_greather:
							{
								tmp[count].m_opcode = dVirtualMachine::gt;
								break;
							}


							case dCILInstr::m_add:
							{
								tmp[count].m_opcode = dVirtualMachine::add;
								break;
							}

							case dCILInstr::m_sub:
							{
								tmp[count].m_opcode = dVirtualMachine::sub;
								break;
							}

							case dCILInstr::m_mul:
							{
								tmp[count].m_opcode = dVirtualMachine::mul;
								break;
							}


							case dCILInstr::m_div:
							{
								tmp[count].m_opcode = dVirtualMachine::div;
								break;
							}


							default:
								dAssert (0);
						}
					}
				}
				count ++;
				break;
			}

			case dCILInstr::m_call:	
			{
				tmp[count].m_opcode = dVirtualMachine::call;
				tmp[count].m_reg0 = D_STACK_REGISTER_INDEX;
				tmp[count].m_imm2 = 0;
				functionCall.Insert (stmt.m_arg0.m_label.GetStr(), count);
				count ++;
				break;
			}

			case dCILInstr::m_ret:	
			{
				dString value (stmt.m_arg1.m_label.GetStr());
				tmp[count].m_opcode = dVirtualMachine::ret;
				tmp[count].m_reg0 = D_STACK_REGISTER_INDEX;
				tmp[count].m_imm2 = stmt.m_extraInformation;
				count ++;
				break;
			}


			case dCILInstr::m_push:	
			{
				tmp[count].m_opcode = dVirtualMachine::push;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = D_STACK_REGISTER_INDEX;
				count ++;
				break;
			}

			case dCILInstr::m_pop:	
			{
				tmp[count].m_opcode = dVirtualMachine::pop;
				tmp[count].m_reg0 = RegisterToIndex (stmt.m_arg0.m_label.GetStr());
				tmp[count].m_reg1 = D_STACK_REGISTER_INDEX;
				count ++;
				break;
			}


			case dCILInstr::m_nop:	
				break;
			
			default: 
				dAssert (0);
		}
	}

	m_codeSegment = new dVirtualMachine::dOpCode[count];
	memcpy (m_codeSegment, tmp, count * sizeof (count));
	delete tmp;

	for (int i = 0; i < count; i ++) {
		dVirtualMachine::dOpCode& code = m_codeSegment[i];
		dVirtualMachine::dInstruction instruction = dVirtualMachine::dInstruction (code.m_opcode);
		
		switch (instruction) 
		{
			case dVirtualMachine::call:
			{
				const dString& destFunctionName = functionCall.Find(i)->GetInfo();
				dTree<int, dString>::dTreeNode* const destNode = m_symbolTable.Find(destFunctionName);
				if (destNode) {
					int dest = destNode->GetInfo();
					code.m_imm2 = dest - (i + 1);
				}
				break;
			}

			case dVirtualMachine::jump:
			{
				int dest = jumpTargets.Find(jumpsOrigin.Find(i)->GetInfo())->GetInfo();
				code.m_imm1 = dest - (i + 1);
				break;
			}

			case dVirtualMachine::bgt:
			case dVirtualMachine::bge:
			case dVirtualMachine::ble:
			case dVirtualMachine::blt:
			case dVirtualMachine::bne:
			case dVirtualMachine::beq:
			{
				int dest = jumpTargets.Find(jumpsOrigin.Find(i)->GetInfo())->GetInfo();
				code.m_imm3 = dest - (i + 1);
				break;
			}

			case dVirtualMachine::eq:
			case dVirtualMachine::le:
			case dVirtualMachine::lt:
			case dVirtualMachine::gt:
			case dVirtualMachine::mov:
			case dVirtualMachine::add:
			case dVirtualMachine::sub:
			case dVirtualMachine::mul:
			case dVirtualMachine::div:
			case dVirtualMachine::movi:
			case dVirtualMachine::addi:
			case dVirtualMachine::muli:
			case dVirtualMachine::divi:
			case dVirtualMachine::push:
			case dVirtualMachine::pop:
			case dVirtualMachine::enter:
			case dVirtualMachine::leave:
			case dVirtualMachine::ret:
			case dVirtualMachine::loadw:
			case dVirtualMachine::storew:
			case dVirtualMachine::loadBase:
			case dVirtualMachine::storeBase:
				break;

			default:
				dAssert (0);

		}
	}
*/
}