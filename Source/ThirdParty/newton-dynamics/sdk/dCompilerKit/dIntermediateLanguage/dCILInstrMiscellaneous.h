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

#ifndef _DCIL_INSTRUC_MISCELANEOUS_H_
#define _DCIL_INSTRUC_MISCELANEOUS_H_


#include "dCIL.h"
#include "dCILInstr.h"

class dCILInstrReturn;
class dCILInstrFunction;


class dCILInstrNop: public dCILInstr
{
	public:
	dCILInstrNop(dCIL& program);
	virtual bool ApplySemanticReordering () { return false;}
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {}
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) {};
	void Serialize(char* const textOut) const;

	virtual int GetByteCodeSize() const { return 0; }
	virtual dCILInstrNop* GetAsNop() { return this; }

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg) {return false;}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver) {}

	virtual bool IsDefineOrUsedVariable() { return false; }

	dString m_comment;
};

class dCILInstrFunction: public dCILInstr
{
	public:
	dCILInstrFunction (dCIL& program, const dString& name, const dArgType& type);

	void Serialize (char* const textOut) const;
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const {}
	dList<dArg>::dListNode* AddParameter (const dString& name, const dArgType& type); 

	virtual dCILInstrFunction* GetAsFunction()
	{
		return this;
	}

	virtual bool ApplySemanticReordering () {return false;} 
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {dAssert (0);}
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {dAssert (0);}
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) {};

	virtual int GetByteCodeSize() const { return 0; }

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver) {}
	
	dArg m_name;
	dList<dArg> m_parameters;
};

class dCILInstrFunctionEnd : public dCILInstr
{
	public: 
	dCILInstrFunctionEnd (dCILInstrFunction* const functionBegin);

	void Serialize(char* const textOut) const;
	virtual void EmitOpcode(dVirtualMachine::dOpCode* const codeOutPtr) const {}

	virtual bool ApplySemanticReordering() { return false; }
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {dAssert (0);}
	virtual void AddDefinedVariable(dInstructionVariableDictionary& dictionary) const {}
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) {};

	virtual int GetByteCodeSize() const { return 0; }
	virtual dCILInstrFunctionEnd* GetAsFunctionEnd() { return this; }

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver) {}

	dCILInstrFunction* m_function;
};

#endif