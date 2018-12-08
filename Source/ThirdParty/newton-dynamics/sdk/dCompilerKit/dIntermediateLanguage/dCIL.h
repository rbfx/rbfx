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

#ifndef __dCIL_H_
#define __dCIL_H_

#include "dCILstdafx.h"
#include "dCILInstr.h"



#define D_TEMPRARY_SYMBOL "$t"

inline dString GetTemporaryVariableName(int index)
{
	char tmp[256];
	sprintf (tmp, "%s%d", D_TEMPRARY_SYMBOL, index);
	return tmp;
}

inline dString GetReturnVariableName()
{
	return GetTemporaryVariableName(0);
}

class dCILInstr;
class dCIL: public dList<dCILInstr*>
{
	public:
	class dReturnValue
	{
		public:
		dReturnValue ()
			:m_type(dCILInstr::m_int)
		{
			m_f = 0.0;
		}

		dCILInstr::dIntrisicType m_type;
		union {;
			dMachineIntRegister m_i;
			dMachineFloatRegister m_f;
		};
	};

	
	dCIL();
	virtual ~dCIL(void);

	dVirtualMachine* BuilExecutable();

	void Clear();
	void Trace();

	dString NewTemp (); 
	dString NewLabel (); 
	void ResetTemporaries();
	dListNode* NewStatement();
	int GetInstructionUniqurID();

	private:
	int m_mark;
	int m_tempIndex;
	int m_labelIndex;
	int m_cilUniqueID;
	bool m_commutativeOperator[dCILThreeArgInstr::m_operatorsCount];
	dCILThreeArgInstr::dOperator m_conditionals[dCILThreeArgInstr::m_operatorsCount];
	dCILThreeArgInstr::dOperator m_operatorComplement[dCILThreeArgInstr::m_operatorsCount];

	public:
	static dString m_phiSource;
	static dString m_pointerSize;
	static dString m_pointerDecoration;
	static dString m_variableUndercore;
	static dString m_functionArgument;
	static dString m_ssaPosifix;

	friend class dCILInstrIntergerLogical;
};


#endif