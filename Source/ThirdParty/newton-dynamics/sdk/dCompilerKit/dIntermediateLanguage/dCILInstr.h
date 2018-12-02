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

#ifndef  _DCIL_INSTRUC_H_
#define  _DCIL_INSTRUC_H_

class dCIL;
class dCILInstrPop;
class dCILInstrPush;
class dCILInstrNop;
class dCILInstrPhy;
class dCILInstrCall;
class dCILInstrMove;
class dCILInstrGoto;
class dCILInstrLocal;
class dCILInstrLabel;
class dCILInstrReturn;
class dCILInstrArgument;
class dCILInstrFunction;
class dCILSingleArgInstr;
class dCILInstrFunctionEnd;
class dCILInstrConditional;
class dConditionalConstantPropagationSolver;
class dCILInstrThreeArgArithmetic;

class dWorkList;
class dBasicBlock;
class dVariablesDictionary;
class dStatementBlockDictionary;
class dRegisterInterferenceGraph;
class dInstructionVariableDictionary;

class dCILInstr
{
	public:
	enum dOperator
	{
		m_add,
		m_sub,
		m_mul,
		m_div,
		m_mod,
		m_equal,
		m_identical,
		m_different,
		m_less,
		m_lessEqual,
		m_greather,
		m_greatherEqual,
		m_operatorsCount,
	};

	enum dIntrisicType
	{
		m_void,
		m_bool,
		m_byte,
		m_short,
		m_int,
		m_long,
		m_float,
		m_double,
		m_string,
		m_classPointer,
		m_constInt,
		m_constFloat,
		m_constString,
		m_luaType,
	};

	class dMapTable 
	{
		public:
		dIntrisicType m_intrinsicType;
		dString m_name;
	};

	class dArgType
	{
		public:
		dArgType ()
			:m_intrinsicType(m_int)
//			,m_isPointer(false)
		{
		}

		dArgType (dIntrisicType intrinsicType)
			:m_intrinsicType(intrinsicType)
//			,m_isPointer(false)
		{
		}

		int GetSizeInByte() const;

		dString GetTypeName () const;
		void SetType (const dArgType& type);

		dIntrisicType m_intrinsicType;
//		bool m_isPointer;
	};

	class dArg: public dArgType
	{
		public:
		dArg ()
			:dArgType()
			,m_label("")
		{
		}

		dArg (const dString& label, const dArgType& type)
			:dArgType(type)
			,m_label(label)
		{
		}

		const dArgType& GetType () const
		{
			return *this;
		}

		void SetType (const dArgType& type);
		void SetType (dIntrisicType intrinsicType, bool pointer);
		dString m_label;
	};


	dCILInstr (dCIL& program);
	dCILInstr(const dCILInstr& copy);

	virtual ~dCILInstr ();

	virtual dCILInstr* Clone() const { dAssert(0); return NULL;}

	dCIL* GetCil() const { return m_cil; }
	int GetUniqueID() const { return m_uniqueId; }
	dBasicBlock* GetBasicBlock() const {return m_basicBlock; }
	dList<dCILInstr*>::dListNode* GetNode() const { return m_myNode; }

	virtual int GetByteCodeSize() const { return 1; }
	virtual int GetByteCodeOffset () const { return m_byteCodeOffset; }
	virtual void SetByteCodeOffset (int offset) { m_byteCodeOffset = offset; }

	virtual bool IsBasicBlockEnd() const { return false; }
	virtual bool IsBasicBlockBegin() const { return false; }

	virtual dCILInstrNop* GetAsNop() { return NULL;}
	virtual dCILInstrPhy* GetAsPhi() { return NULL;}
	virtual dCILInstrPop* GetAsPop() { return NULL;}
	virtual dCILInstrPush* GetAsPush() { return NULL;}
	virtual dCILInstrMove* GetAsMove() { return NULL;}
	virtual dCILInstrCall* GetAsCall() { return NULL;}
	virtual dCILInstrGoto* GetAsGoto() { return NULL;}
	virtual dCILInstrLocal* GetAsLocal() { return NULL;}
	virtual dCILInstrLabel* GetAsLabel() { return NULL;}
	virtual dCILInstrReturn* GetAsReturn() { return NULL;}
	virtual dCILInstrConditional* GetAsIF() { return NULL;}
	virtual dCILInstrArgument* GetAsArgument() { return NULL;}
	virtual dCILInstrFunction* GetAsFunction() { return NULL;}
	virtual dCILSingleArgInstr* GetAsSingleArg()  { return NULL;}
	virtual dCILInstrFunctionEnd* GetAsFunctionEnd() { return NULL;}
	virtual dCILInstrThreeArgArithmetic* GetAsThreeArgArithmetic() { return NULL;}

	virtual bool IsDefineOrUsedVariable() { return true; }

	//virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const = 0;
	virtual void EmitOpcode (dVirtualMachine::dOpCode* const codeOutPtr) const 
	{
		dAssert(0); 
	}

	virtual bool ApplySemanticReordering () = 0;
	virtual void AddUsedVariable (dInstructionVariableDictionary& dictionary) const = 0;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const = 0;
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph) = 0;
	
	virtual void Serialize(char* const textOut) const;

	void Nullify();
	void ReplaceInstruction(dCILInstr* const newIntruction);
	virtual void Trace() const;
	static dIntrisicType GetTypeID(const dString& typeName);



	// ****************************
	virtual dArg* GetGeneratedVariable () {dAssert (0); return NULL;}
	virtual void GetUsedVariables (dList<dArg*>& variablesList) {dAssert (0);}
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg) {dAssert (0); return false;}

	virtual bool ApplyConstantFoldingSSA () {return false;}
	virtual bool ApplyCopyPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary) {return false;}
	virtual bool ApplySimpleConstantPropagationSSA (dWorkList& workList, dStatementBlockDictionary& usedVariablesDictionary) {return false;}
	virtual void ApplyConditionalConstantPropagationSSA (dConditionalConstantPropagationSolver& solver) {}

	dString RemoveSSAPostfix(const dString& name) const;
	dString MakeSSAName(const dString& name, int ssaPostfix) const;

	protected:
	const char* GetOperatorString(dOperator operatotion) const;

	dCIL* m_cil;
	dBasicBlock* m_basicBlock;
	dList<dCILInstr*>::dListNode* m_myNode;
	int m_byteCodeOffset;
	int m_uniqueId;
	static dMapTable m_maptable[];
	static dString m_ssaPosfix;
	
	friend class dBasicBlock;
	friend class dBasicBlocksGraph;
};


class dCILSingleArgInstr: public dCILInstr
{
	public:
	dCILSingleArgInstr(const dCILSingleArgInstr& copy)
		:dCILInstr(copy)
		,m_arg0(copy.m_arg0)
	{
	}

	dCILSingleArgInstr (dCIL& program, const dArg &arg)
		:dCILInstr (program)
		,m_arg0(arg)
	{
	}

	virtual dCILSingleArgInstr* GetAsSingleArg()  
	{ 
		return this; 
	}

	virtual bool ApplySemanticReordering ()
	{
		return false;
	}

	const dArg& GetArg0() const
	{
		return m_arg0;
	}

	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg);
	
	dArg m_arg0;
};

class dCILTwoArgInstr: public dCILSingleArgInstr
{
	public:
	dCILTwoArgInstr (const dCILTwoArgInstr& copy)
		:dCILSingleArgInstr(copy)
		,m_arg1(copy.m_arg1)
	{
	}

	dCILTwoArgInstr (dCIL& program, const dArg& arg0, const dArg& arg1)
		:dCILSingleArgInstr (program, arg0)
		,m_arg1(arg1)
	{
	}

	virtual bool ApplySemanticReordering () = 0;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const = 0;
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg);

	const dArg& GetArg1 () const 
	{
		return m_arg1;
	}

	dArg m_arg1;
};

class dCILThreeArgInstr: public dCILTwoArgInstr
{
	public:
	dCILThreeArgInstr(const dCILThreeArgInstr& copy)
		:dCILTwoArgInstr(copy)
		,m_arg2(copy.m_arg2)
	{
	}

	dCILThreeArgInstr (dCIL& program, const dArg& arg0, const dArg& arg1, const dArg& arg2)
		:dCILTwoArgInstr (program, arg0, arg1)
		,m_arg2(arg2)
	{
	}

	const dArg& GetArg2 () const 
	{
		return m_arg2;
	}

	virtual bool ApplySemanticReordering () = 0;
	virtual void AddDefinedVariable (dInstructionVariableDictionary& dictionary) const = 0;
	virtual void AssignRegisterName(const dRegisterInterferenceGraph& interferenceGraph);
	virtual bool ReplaceArgument(const dArg& arg, const dArg& newArg) { dAssert(0); return false; }

	dArg m_arg2;
};


#endif