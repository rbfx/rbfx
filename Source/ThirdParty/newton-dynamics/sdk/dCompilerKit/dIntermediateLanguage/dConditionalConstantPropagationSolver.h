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

#ifndef _dConstantPropagationSolver_H_
#define _dConstantPropagationSolver_H_


#include "dCILstdafx.h"
#include "dCIL.h"
#include "dCILInstr.h"
#include "dBasicBlocksGraph.h"


class dConditionalConstantPropagationSolver
{
	public:
	class dInstructionMap: public dTree<int, dCILInstr*>
	{
		public:
		dInstructionMap ();
	};

	class dVariable
	{
		public:
		enum dValueTypes
		{
			m_constant,
			m_undefined,
			m_variableValue,
		};

		dVariable(dCILInstr* const instruction, dCILInstr::dArg* const variable);
		dVariable& operator= (const dVariable& copy);

		dValueTypes m_type;
		dString m_constValue;
		dCILInstr::dArg* m_variable;
		dCILInstr* const m_instruction;
	};

	class dBlockEdgeKey
	{
		public:
		dBlockEdgeKey(const dBasicBlock* const blockHigh, const dBasicBlock* const blockLow);
		bool operator<(const dBlockEdgeKey& src) const;
		bool operator>(const dBlockEdgeKey& src) const;

		const dBasicBlock* m_blockHigh;
		const dBasicBlock* m_blockLow;
	};


	dConditionalConstantPropagationSolver (dBasicBlocksGraph* const graph);
	bool Solve();
	void UpdateLatice (const dCILInstr::dArg& arg, const dString& value, dVariable::dValueTypes type);

	void Trace();

	dBasicBlocksGraph* m_graph; 
	dTree<int, dBasicBlock*> m_visited;
	dList<dBasicBlock*> m_blockWorklist;
	dTree<dInstructionMap, dString> m_uses;
	dList<dCILInstr*> m_instructionsWorklist;
	dTree<dVariable, dString> m_variablesList;
	dTree<int, dBlockEdgeKey> m_executableEdges;
};

#endif