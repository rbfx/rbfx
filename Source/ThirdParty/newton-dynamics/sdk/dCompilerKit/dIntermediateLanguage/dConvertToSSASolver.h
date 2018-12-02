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

#ifndef _dConvertToSSASolver_H_
#define _dConvertToSSASolver_H_


#include "dCILstdafx.h"
#include "dCIL.h"
#include "dCILInstr.h"
#include "dBasicBlocksGraph.h"


class dConvertToSSASolver
{
	public:
	class dStatementBucket: public dTree <const dCILInstr*, const dBasicBlock*>
	{
		public:
		dStatementBucket()
			:dTree <const dCILInstr*, const dBasicBlock*>()
			,m_index(1)
		{
		}

		int m_index;
		dList<int> m_stack;
		dCILInstr::dArg m_variable;
	};

	class dFrontierList: public dList<const dBasicBlock*>
	{
		public:
		dFrontierList ()
			:dList<const dBasicBlock*>()
		{
		}
	};

	dConvertToSSASolver (dBasicBlocksGraph* const graph);
	void Solve();
	
	private:
	void BuildDomicanceFrontier();
	void BuildDomicanceFrontier(const dBasicBlock* const root);
	void RenameVariables (const dBasicBlock* const root, dTree <dStatementBucket, dString>& stack) const;

	dBasicBlocksGraph* m_graph; 
	dTree<dFrontierList, const dBasicBlock*> m_dominanceFrontier; 
};

#endif