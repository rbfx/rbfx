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

#ifndef _DCIL_INSTRUC_LOAD_STORE_H_
#define _DCIL_INSTRUC_LOAD_STORE_H_

#include "dCIL.h"
#include "dCILInstr.h"

class dCILInstrArgument: public dCILSingleArgInstr
{
	public:
	dCILInstrArgument(dCIL& program, const dString& name, const dArgType& type);

	void Serialize(char* const textOut) const;
	virtual dCILInstrArgument* GetAsArgument() { return this; }

	virtual bool ApplySemanticReordering() { return false; }
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {}
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const;

	virtual int GetByteCodeSize() const { return 0; }
	
	// ***********************
	virtual dArg* GetGeneratedVariable () { return &m_arg0; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
};


class dCILInstrLocal: public dCILSingleArgInstr
{
	public:
	dCILInstrLocal (dCIL& program, const dString& name, const dArgType& type);
	void Serialize(char* const textOut) const;
	virtual dCILInstrLocal* GetAsLocal() { return this; }

	virtual bool ApplySemanticReordering () {return false;}
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {dAssert (0);}
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const;

	// ***********************
	virtual dArg* GetGeneratedVariable() { return &m_arg0; }
	virtual void GetUsedVariables(dList<dArg*>& variablesList) {}
};


class dCILInstrMove: public dCILTwoArgInstr
{
	public:
	dCILInstrMove (const dCILInstrMove& copy);
	dCILInstrMove (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1);
	virtual dCILInstr* Clone() const { return new dCILInstrMove (*this);}

	void Serialize(char* const textOut) const;
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const;

	virtual dCILInstrMove* GetAsMove() { return this; }
	virtual bool ApplyDeadElimination() const { return true; }
	virtual bool ApplySemanticReordering () {return false;}
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const; 
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const;
	virtual bool ReplaceArgument (const dArg& arg, const dArg& newArg);

	// ***********************
	virtual dArg* GetGeneratedVariable () { return &m_arg0; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
	virtual bool ApplyCopyPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary);
	virtual bool ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary);
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver);
};


class dCILInstrPhy: public dCILSingleArgInstr
{
	public:
	class dArgPair
	{
		public:
		dArgPair (const dArg& arg, const dBasicBlock* const block)
			:m_block(block) 
			,m_arg (arg)
		{
			dAssert (m_block);
		}

		dArgPair (const dArgPair& copy)
			:m_block (copy.m_block)
			,m_arg (copy.m_arg)
		{
			dAssert (m_block);
		}

		const dBasicBlock* m_block;
		dArg m_arg;
	};

	dCILInstrPhy (dCIL& program, const dString& name, const dArgType& type, const dBasicBlock* const basicBlock, dList<const dBasicBlock*>& sources);

	void Serialize(char* const textOut) const;
	virtual void EmitOpcode(dVirtualMachine::dOpCode* const codeOutPtr) const {}

	virtual dCILInstrPhy* GetAsPhi() { return this; }
	virtual bool ApplyDeadElimination() const { return true; }
	virtual bool ApplySemanticReordering() { return false; }
	virtual void AddUsedVariable(dInstructionVariableDictionary& dictionary) const {dAssert(0);}
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const {dAssert(0);}

	// ***********************
	virtual dArg* GetGeneratedVariable () { return &m_arg0; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg);
	virtual bool ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary);
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver);

	dList<dArgPair> m_sources;
};


class dCILInstrLoad: public dCILTwoArgInstr
{
	public:
	dCILInstrLoad (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1);
	void Serialize(char* const textOut) const;

	virtual bool ApplyDeadElimination() const 
	{
		return true;
	}

	virtual bool ApplySemanticReordering () {return false;};
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const;

	// ***********************
	virtual bool ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary) {dAssert (0); return false;}
	virtual dArg* GetGeneratedVariable () { return &m_arg0; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
};

class dCILInstrStore: public dCILTwoArgInstr
{
	public:
	dCILInstrStore (dCIL& program, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1);
	void Serialize(char* const textOut) const;

	virtual bool ApplySemanticReordering () {return false;}
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
};


class dCILInstrPush: public dCILSingleArgInstr
{
	public:
	dCILInstrPush(dCIL& program, const dString& name0, const dArgType& type0);
	void Serialize(char* const textOut) const;

	virtual bool ApplySemanticReordering() { return false; }
	virtual void AddUsedVariable(dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const {}

	// ***********************
	virtual dArg* GetGeneratedVariable() { return NULL; }
	virtual void GetUsedVariables(dList<dArg*>& variablesList) {}
};

class dCILInstrPop: public dCILSingleArgInstr
{
	public:
	dCILInstrPop(dCIL& program, const dString& name0, const dArgType& type0);
	void Serialize(char* const textOut) const;

	virtual bool ApplySemanticReordering() { return false; }
	virtual void AddUsedVariable(dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const {}

	// ***********************
	virtual dArg* GetGeneratedVariable() { return NULL; }
	virtual void GetUsedVariables(dList<dArg*>& variablesList) {};
};


#endif