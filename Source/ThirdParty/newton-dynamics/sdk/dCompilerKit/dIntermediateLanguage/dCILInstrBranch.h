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

#ifndef _DCIL_INSTRUC_BRANCH_H_
#define _DCIL_INSTRUC_BRANCH_H_


#include "dCIL.h"
#include "dCILInstr.h"


class dCILInstrLabel: public dCILInstr
{
	public:
	dCILInstrLabel(dCIL& program, const dString& label);
	void Serialize(char* const textOut) const;
	virtual int GetByteCodeSize() const { return 0; }
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const {}

	virtual bool ApplySemanticReordering () {return false;}
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {}
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}
	void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) {}
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg) {return false;}
	virtual bool IsBasicBlockBegin() const;

	const dString& GetLabel() const;
	virtual dCILInstrLabel* GetAsLabel();
	

	virtual bool IsDefineOrUsedVariable() { return false; }

	// ***********************
	virtual dArg* GetGeneratedVariable () {return NULL;}
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver) {}

	dString m_label;
};

class dCILInstrGoto: public dCILInstr
{
	public:
	dCILInstrGoto(dCIL& program, const dString& label);
	dCILInstrGoto(dCIL& program, dCILInstrLabel* const target);
	void Serialize(char* const textOut) const;

	virtual bool IsBasicBlockEnd() const;
	virtual dCILInstrGoto* GetAsGoto();

	const dString& GetLabel() const;
	void SetLabel (const dString& label);

	void SetTarget (dCILInstrLabel* const target);
	dList<dCILInstr*>::dListNode* GetTarget () const;

	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const {}
	virtual bool ApplySemanticReordering () {return false;};
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}

	void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) {}
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg) {return false;}

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver);

	virtual bool IsDefineOrUsedVariable() { return false; }

	dString m_label;
	dList<dCILInstr*>::dListNode* m_tagetNode;
};

class dCILInstrConditional: public dCILTwoArgInstr
{
	public:
	dCILInstrConditional(dCIL& program, dOperator operation, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1, const dString& target0, const dString& target1);
	dCILInstrConditional(dCIL& program, dOperator operation, const dString& name0, const dArgType& type0, const dString& name1, const dArgType& type1, dCILInstrLabel* const target0, dCILInstrLabel* const target1);
	void Serialize(char* const textOut) const;
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const;

	virtual dCILInstrConditional* GetAsIF();
	virtual bool IsBasicBlockEnd() const;
	void SetLabels (const dString& label0, const dString& label1);
	void SetTargets (dCILInstrLabel* const target0, dCILInstrLabel* const target1);

	virtual bool ApplySemanticReordering () {return false;}

	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}

	const dString& GetTrueLabel() const;
	const dString& GetFalseLabel() const;
	dList<dCILInstr*>::dListNode* GetTrueTarget () const;
	dList<dCILInstr*>::dListNode* GetFalseTarget () const;


	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
	virtual bool ReplaceArgument (const dArg& arg, const dArg& newArg);
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver);
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);

	dString m_label0;
	dString m_label1;
	dList<dCILInstr*>::dListNode* m_targetNode0;
	dList<dCILInstr*>::dListNode* m_targetNode1;
	dOperator m_operator;
};


class dCILInstrReturn: public dCILSingleArgInstr
{
	public:
	dCILInstrReturn(dCIL& program, const dString& name, const dArgType& type);
	void Serialize(char* const textOut) const;
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const; 

	virtual int GetByteCodeSize() const;
	virtual bool IsBasicBlockEnd() const;
	virtual dCILInstrReturn* GetAsReturn();

	virtual bool ApplySemanticReordering () {return false;}

	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const {}
	void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);

	// ***********************
	virtual dArg* GetGeneratedVariable () { return NULL; }
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg);
};

class dCILInstrCall: public dCILTwoArgInstr
{
	public:
	dCILInstrCall(dCIL& program, const dString& returnValue, const dArgType& type, const dString& functionName);
	void Serialize(char* const textOut) const;
	void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const;

	void AddArgument (const dArg& argument);
//	void SetTarget(dList<dCILInstr*>::dListNode* const node) {m_tagetNode = node; }

	virtual dCILInstrCall* GetAsCall() { return this; }

	virtual bool ApplySemanticReordering () {return false;}

	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const;
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);

	// ***********************
	virtual dArg* GetGeneratedVariable ();
	virtual void GetUsedVariables (dList<dArg*>& variablesList);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg);

	dList<dArg> m_parameters;
//	dList<dCILInstr*>::dListNode* m_tagetNode;
};

#endif